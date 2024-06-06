#include "image.h"
#include "FreeImage.h"

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
	if (FreeImage_GetBPP(fib) != 8) {
		FIBITMAP* newImage = FreeImage_ConvertTo8Bits(fib);
		FreeImage_Unload(fib);
		fib = newImage;
		if (!fib || FreeImage_GetBPP(fib) != 8) {
			return Error("Failed to convert image to 8bit RGB");
		}
	}

	// Copy the image data to our internal memory.
	width = FreeImage_GetWidth(fib);
	height = FreeImage_GetHeight(fib);

	const size_t destRowSize = size_t(width) * sizeof(data[0]) * 3;
	data = std::make_unique<uint8_t[]>(size_t(height) * destRowSize);

	for (int row = 0; row < height; ++row) {
		// FreeImage stores the bottom of the image first (upside-down)
		BYTE* src = FreeImage_GetScanLine(fib, height - 1 - row);
		uint8_t* dst = data.get() + destRowSize*(row);
		memcpy(dst, src, destRowSize);
	}

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

// ################################################################################################################################
// # ImageManager
// ################################################################################################################################

bool ImageManager::accepts(const char* path) {
	FREE_IMAGE_FORMAT imgFormat = getImageFormat(path);
	return FreeImage_FIFSupportsReading(imgFormat);
}

void ImageManager::triggerLoad(const char* path) {
	if (loadingImage) return;
	if (nextImage == nullptr) {
		nextImage = std::make_unique<Image>();
	}

	loadingImage = true;
	Error err = nextImage->load(path);
	currentImage = std::exchange(nextImage, nullptr);
	loadingImage = false;
}

Image* ImageManager::getCurrentImage() {
	return currentImage.get();
}
