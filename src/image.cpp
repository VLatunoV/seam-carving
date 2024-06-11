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
	return
		0.2126f * toLinear(r) +
		0.7152f * toLinear(g) +
		0.0722f * toLinear(b);
}

// ################################################################################################################################
// # Image
// ################################################################################################################################

Image::operator bool() {
	return isValid();
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

	const size_t srcRowSize = FreeImage_GetLine(fib);
	data = std::make_unique<uint8_t[]>(size_t(height) * srcRowSize);

	for (int row = 0; row < height; ++row) {
		// FreeImage stores the bottom of the image first (upside-down)
		RGBTRIPLE* src = reinterpret_cast<RGBTRIPLE*>(FreeImage_GetScanLine(fib, height - 1 - row));
		uint8_t* dst = data.get() + srcRowSize * size_t(row);
		for (int col = 0; col < width; ++col) {
			dst[0] = src->rgbtRed;
			dst[1] = src->rgbtGreen;
			dst[2] = src->rgbtBlue;
			src += 1;
			dst += 3;
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

const uint8_t* Image::getData() {
	return data.get();
}

bool Image::isValid() const {
	return (width > 0) && (height > 0) && data;
}

void Image::computeEnergies() {
	if (!isValid()) return;

	const size_t memSize = size_t(width) * height;
	energy = std::make_unique<float[]>(memSize);

	std::unique_ptr<float[]> luma = std::make_unique<float[]>(memSize);

	for (int colorIdx=0, intensIdx=0; intensIdx<memSize; colorIdx+=3, intensIdx++) {
		luma[intensIdx] = computeLuma(
			float(data[colorIdx]),
			float(data[colorIdx+1]),
			float(data[colorIdx+2])
		);
	}

	int offset = 0;
	int iterEnd = 0;
	// First row
	energy[offset] = (fabsf(luma[offset+1] - luma[offset]) + fabsf(luma[offset+width] - luma[offset]))*2.0f;
	++offset;
	for (iterEnd = offset+width-2; offset < iterEnd; ++offset) {
		energy[offset] = fabsf(luma[offset+1] - luma[offset-1]) + fabsf(luma[offset+width] - luma[offset])*2.0f;
	}
	energy[offset] = (fabsf(luma[offset] - luma[offset-1]) + fabsf(luma[offset+width] - luma[offset]))*2.0f;
	++offset;

	// Middle rows
	for (int row = 1; row < height-1; ++row) {
		energy[offset] = fabsf(luma[offset+1] - luma[offset])*2.0f + fabsf(luma[offset+width] - luma[offset-width]);
		++offset;
		for (iterEnd = offset+width-2; offset < iterEnd; ++offset) {
			energy[offset] = fabsf(luma[offset+1] - luma[offset-1]) + fabsf(luma[offset+width] - luma[offset-width]);
		}
		energy[offset] = fabsf(luma[offset] - luma[offset-1])*2.0f + fabsf(luma[offset+width] - luma[offset-width]);
		++offset;
	}

	// Last row
	energy[offset] = (fabsf(luma[offset+1] - luma[offset]) + fabsf(luma[offset] - luma[offset-width]))*2.0f;
	++offset;
	for (iterEnd = offset+width-2; offset < iterEnd; ++offset) {
		energy[offset] = fabsf(luma[offset+1] - luma[offset-1]) + fabsf(luma[offset] - luma[offset-width])*2.0f;
	}
	energy[offset] = (fabsf(luma[offset] - luma[offset-1]) + fabsf(luma[offset] - luma[offset-width]))*2.0f;
	++offset;
}

// ################################################################################################################################
// # ImageManager
// ################################################################################################################################

bool ImageManager::accepts(const char* path) {
	FREE_IMAGE_FORMAT imgFormat = getImageFormat(path);
	return FreeImage_FIFSupportsReading(imgFormat);
}

void ImageManager::triggerLoad(const char* path) {
	if (nextImage == nullptr) {
		nextImage = std::make_unique<Image>();
	}

	Error err = nextImage->load(path);
	currentImage = std::exchange(nextImage, nullptr);
	notify(&ImageManagerObserver::onImageChange);
}

Image* ImageManager::getCurrentImage() {
	return currentImage.get();
}
