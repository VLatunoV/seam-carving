#include <chrono>

#include "FreeImage.h"
#include "image.h"

/// Get load flags for a given image format.
static int getImageLoadFlags(FREE_IMAGE_FORMAT imgFormat) {
	int result = 0;
	if (imgFormat == FIF_JPEG) {
		result = JPEG_ACCURATE;
	}
	return result;
}

/// Return the deduced image format of the file given.
/// It tries to read up-to 16 bytes from the file, and if that doesn't work,
/// makes a guess from the file extension.
static FREE_IMAGE_FORMAT getImageFormat(const char* path) {
	FREE_IMAGE_FORMAT fif = FIF_UNKNOWN;
	// check the file signature and deduce its format
	fif = FreeImage_GetFileType(path, 0 /*not used*/);
	if (fif == FIF_UNKNOWN) {
		// try to guess the file format from the file extension
		fif = FreeImage_GetFIFFromFilename(path);
	}
	return fif;
}

inline static float toLinear(float x) {
	return (x <= 0.04045f)
		? (x / 12.92f)
		: powf((x+0.055f) / 1.055f, 2.4f);
}

inline static float toSRGB(float x) {
	return (x <= 0.0031308f)
		? (x * 12.92f)
		: 1.055f * powf(x, 1.0f/2.4f) - 0.055f;
}

inline static float computeLuma(const float& r, const float& g, const float& b) {
	return toSRGB(
		0.2126f * toLinear(r) +
		0.7152f * toLinear(g) +
		0.0722f * toLinear(b));
}

// ################################################################################################################################
// # Image
// ################################################################################################################################

Image::operator bool() {
	return isValid();
}

void Image::copyFrom(Image& other) {
	const int numPixels = other.stride * other.height;
	allocMemory(numPixels);

	width = other.width;
	height = other.height;
	stride = other.stride;
	memcpy(data.get(), other.data.get(), numPixels * sizeof(data[0]));
	memcpy(energy.get(), other.energy.get(), numPixels * sizeof(energy[0]));
}

Error Image::load(const char* path) {
	FREE_IMAGE_FORMAT imgFormat = getImageFormat(path);
	const int imgFlags = getImageLoadFlags(imgFormat);
	FIBITMAP* fib = FreeImage_Load(imgFormat, path, imgFlags);
	if (!fib) {
		return Error("Failed to load image");
	}

	// We want to pass 24 bit RGB values to OpenGL
	if (FreeImage_GetBPP(fib) != 24) {
		FIBITMAP* newImage = FreeImage_ConvertTo24Bits(fib);
		FreeImage_Unload(fib);
		fib = newImage;
		if (!fib || FreeImage_GetBPP(fib) != 24) {
			return Error("Failed to convert image to 8bit RGB");
		}
	}

	// Copy the image data to our internal memory.
	width = FreeImage_GetWidth(fib);
	height = FreeImage_GetHeight(fib);
	stride = width;

	const size_t numPixels = size_t(stride) * height;
	if (numPixels > 0xffffffff) {
		FreeImage_Unload(fib);
		return Error("Image too large to load");
	}
	allocMemory(numPixels);

	for (int row = 0; row < height; ++row) {
		// FreeImage stores the bottom of the image first (upside-down)
		RGBTRIPLE* src = reinterpret_cast<RGBTRIPLE*>(FreeImage_GetScanLine(fib, height - 1 - row));
		Pixel* dst = &data[row * width];
		Pixel* last = &data[(row+1) * width];
		for (; dst != last; ++dst, ++src) {
			static_assert(sizeof(dst->r) == sizeof(src->rgbtRed));
			dst->r = uint8_t(src->rgbtRed);
			dst->g = uint8_t(src->rgbtGreen);
			dst->b = uint8_t(src->rgbtBlue);
		}
	}

	FreeImage_Unload(fib);
	computeEnergies();
	return Error();
}

int Image::getWidth() const {
	return width;
}

int Image::getHeight() const {
	return height;
}

int Image::getStride() const {
	return stride;
}

const Pixel* Image::getData() {
	return data.get();
}

const float* Image::getEnergy() {
	return energy.get();
}

bool Image::isValid() const {
	return (width > 0) && (height > 0) && data;
}

/// Helper struct that implements the seam carving algorithm. It uses a template argument to determine
/// if it should carve (remove) rows or columns. We think of removing only columns, and with appropriate
/// dimension switch, we can do rows too. The virtual dimensions are set in the constructor.
/// @tparam doCols If true, it removes columns, otherwise it removes rows.
template <bool doCols>
struct CarveHelper {
	Image& image; ///< Reference to the image to carve.
	const int& rows; ///< Virtual rows.
	int& cols; ///< Virtual columns.
	/// Number of pixels to the next row of the index map. It is the number of columns of the image, but we can't use
	/// them directly, since they will change after each seam.
	const int idxStride = 0;
	/// Stores the offset of each pixel in the image. When removing a seam, remove it from this map and do
	/// changes only here.
	std::vector<int> idxMap;
	std::vector<float> dyn; ///< Dynamic table for computing the lowest energies.
	std::vector<int8_t> prev; ///< Keeps track of the pixel from which we got the lowest energy.
	std::vector<int> seam; ///< Stores the indices of the seam for each row or column.

	explicit CarveHelper(Image& _image)
		: image(_image)
		, rows(doCols ? _image.height : _image.width)
		, cols(doCols ? _image.width : _image.height)
	{
		const_cast<int&>(idxStride) = cols;
	}

	/// Removes @p howMany seams from the image with the lowest energy.
	void carve(int howMany) {
		if (howMany == 0) return;

		const int numPixels = image.stride * image.height;
		dyn.resize(numPixels);
		prev.resize(numPixels);
		memset(dyn.data(), 0x7f, numPixels * sizeof(dyn[0]));
		memset(prev.data(), 0, numPixels * sizeof(prev[0]));

		idxMap.resize(cols * rows);
		for (int r = 0; r < rows; ++r) {
			for (int c = 0; c < cols; ++c) {
				getIdx(r, c) = at(r, c);
			}
		}
		seam.resize(rows);

		// First pass. Compute the full dynamic table.
		for (int c = 0; c < cols; ++c) {
			const int offset = at(0, c);
			dyn[offset] = image.energy[offset];
		}
		for (int r = 1; r < rows; ++r) {
			int offset = at(r, 0);
			computePixel(r, 0, 0);
			computePixel(r, 0, 1);
			dyn[offset] += image.energy[offset];
			for (int c=1, cEnd=cols-1; c<cEnd; ++c) {
				offset = at(r, c);
				computePixel(r, c, 0);
				computePixel(r, c,-1);
				computePixel(r, c, 1);
				dyn[offset] += image.energy[offset];
			}
			offset = at(r, cols-1);
			computePixel(r, cols-1, 0);
			computePixel(r, cols-1,-1);
			dyn[offset] += image.energy[offset];
		}

		// Now that we have the dynamic table, we can find seams.
		while (howMany) {
			--howMany;

			// Find the start of the optimal seam
			int minSeam = cols-1;
			for (int c = cols-2; c >= 0; --c) {
				if (dyn[getIdx(rows-1, minSeam)] > dyn[getIdx(rows-1, c)]) {
					minSeam = c;
				}
			}

			// Find all pixels of the seam
			for (int r = rows-1; r >= 0; --r) {
				seam[r] = minSeam;
				minSeam += prev[getIdx(r, minSeam)];
			}

			// Remove the seam
			for (int r = 0; r < rows; ++r) {
				for (int c = seam[r]+1; c < cols; ++c) {
					const int offset = r*idxStride + c;
					idxMap[offset - 1] = idxMap[offset];
				}
			}

			--cols;

			// If we have to remove more seams, update the dynamic table
			if (howMany) {
				for (int r = 1; r < rows; ++r) {
					for (int c = std::max(0, seam[r]-r), cEnd = std::min(cols, seam[r]+r+1); c < cEnd; ++c) {
						const int offset = getIdx(r, c);
						dyn[offset] = 1e38f;
						computePixel(r, c, 0);
						if (c > 0) computePixel(r, c, -1);
						if (c+1 < cols) computePixel(r, c, 1);
						dyn[offset] += image.energy[offset];
					}
				}
			}
		}

		// After all seams are removed, compact the final image
		for (int r = 0; r < rows; ++r) {
			const int offset = doCols ? 1 : image.stride;
			int dst = at(r, 0);
			for (int c = 0; c < cols; ++c, dst += offset) {
				const int src = getIdx(r, c);
				if (dst == src) continue;
				image.data[dst] = image.data[src];
				image.energy[dst] = image.energy[src];
			}
		}
	}

	int at(const int& r, const int& c) {
		return doCols
			? c + r*image.stride
			: r + c*image.stride;
	}

	int& getIdx(const int& r, const int& c) {
		return idxMap[r*idxStride + c];
	}

	void computePixel(const int& r, const int& c, const int& _prev) {
		const int currOffset = getIdx(r, c);
		const int prevOffset = getIdx(r-1, c+_prev);
		if (dyn[currOffset] > dyn[prevOffset]) {
			dyn[currOffset] = dyn[prevOffset];
			prev[currOffset] = _prev;
		}
	}
};

void Image::carveRows(int howMany) {
	std::chrono::high_resolution_clock clock;
	std::chrono::time_point startTime = clock.now();
	CarveHelper<false> helper(*this);
	helper.carve(howMany);
	auto deltaTime = clock.now() - startTime;
	printf("Carve %d rows: %.03fms\n", howMany, 1e-6f * deltaTime.count());
}

void Image::carveCols(int howMany) {
	std::chrono::high_resolution_clock clock;
	std::chrono::time_point startTime = clock.now();
	CarveHelper<true> helper(*this);
	helper.carve(howMany);
	auto deltaTime = clock.now() - startTime;
	printf("Carve %d cols: %.03fms\n", howMany, 1e-6f * deltaTime.count());
}

void Image::computeEnergies() {
	if (!isValid()) return;
	const int memSize = stride * height;
	std::unique_ptr<float[]> luma = std::make_unique<float[]>(memSize);

	std::chrono::high_resolution_clock clock;
	std::chrono::time_point startTime = clock.now();

	const float k = 1.0f / 255.0f;
	for (int idx=0; idx<memSize; ++idx) {
		luma[idx] = computeLuma(
			k * float(data[idx].r),
			k * float(data[idx].g),
			k * float(data[idx].b)
		);
	}

	int offset = 0;
	int iterEnd = 0;
	// First row
	energy[offset] = (fabsf(luma[offset+1] - luma[offset]) + fabsf(luma[offset+stride] - luma[offset]))*2.0f;
	++offset;
	for (iterEnd = offset+width-2; offset < iterEnd; ++offset) {
		energy[offset] = fabsf(luma[offset+1] - luma[offset-1]) + fabsf(luma[offset+stride] - luma[offset])*2.0f;
	}
	energy[offset] = (fabsf(luma[offset] - luma[offset-1]) + fabsf(luma[offset+stride] - luma[offset]))*2.0f;
	++offset;

	// Middle rows
	for (int row = 1; row < height-1; ++row) {
		energy[offset] = fabsf(luma[offset+1] - luma[offset])*2.0f + fabsf(luma[offset+stride] - luma[offset-stride]);
		++offset;
		for (iterEnd = offset+width-2; offset < iterEnd; ++offset) {
			energy[offset] = fabsf(luma[offset+1] - luma[offset-1]) + fabsf(luma[offset+stride] - luma[offset-stride]);
		}
		energy[offset] = fabsf(luma[offset] - luma[offset-1])*2.0f + fabsf(luma[offset+stride] - luma[offset-stride]);
		++offset;
	}

	// Last row
	energy[offset] = (fabsf(luma[offset+1] - luma[offset]) + fabsf(luma[offset] - luma[offset-stride]))*2.0f;
	++offset;
	for (iterEnd = offset+width-2; offset < iterEnd; ++offset) {
		energy[offset] = fabsf(luma[offset+1] - luma[offset-1]) + fabsf(luma[offset] - luma[offset-stride])*2.0f;
	}
	energy[offset] = (fabsf(luma[offset] - luma[offset-1]) + fabsf(luma[offset] - luma[offset-stride]))*2.0f;
	++offset;

	// Normalize energy to 1.0f
	float maxE = 0.0f;
	for (int i = 0; i < memSize; ++i) {
		if (maxE < energy[i]) {
			maxE = energy[i];
		}
	}
	maxE = 1.0f/maxE;
	for (int i = 0; i < memSize; ++i) {
		energy[i] *= maxE;
	}

	auto deltaTime = clock.now() - startTime;
	printf("Computed energies: %.03fms\n", 1e-6f * deltaTime.count());
}

void Image::allocMemory(int newCap) {
	if (newCap <= capacity) return;

	data = std::make_unique<Pixel[]>(newCap);
	energy = std::make_unique<float[]>(newCap);
	capacity = newCap;
}

// ################################################################################################################################
// # ImageManager
// ################################################################################################################################

bool ImageManager::accepts(const char* path) {
	FREE_IMAGE_FORMAT imgFormat = getImageFormat(path);
	return FreeImage_FIFSupportsReading(imgFormat);
}

Image& ImageManager::getActiveImage() {
	return isSeamModified ? activeImage : originalImage;
}

Image& ImageManager::getOriginalImage() {
	return originalImage;
}

void ImageManager::triggerLoad(const char* path) {
	Error err = originalImage.load(path);
	isSeamModified = false;

	notify(&ImageManagerObserver::onImageChange);
}

void ImageManager::triggerSeam(int newWidth, int newHeight) {
	Image* img = &getActiveImage();
	if (img->getWidth() == newWidth && img->getHeight() == newHeight) {
		return;
	}

	if (!isSeamModified || (img->getWidth() < newWidth || img->getHeight() < newHeight)) {
		activeImage.copyFrom(originalImage);
		isSeamModified = true;
		img = &activeImage;
	}

	const int diffWidth = img->getWidth() - newWidth;
	const int diffHeight = img->getHeight() - newHeight;
	img->carveCols(diffWidth);
	img->carveRows(diffHeight);

	notify(&ImageManagerObserver::onImageSeamed);
}
