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

struct ImageDeleter {
	void operator()(FIBITMAP* ptr) {
		FreeImage_Unload(ptr);
	}
};

/// Implementation of the @ref Image interface.
class ImageImpl: public Image {
	using ManagedFIB = std::unique_ptr<FIBITMAP, ImageDeleter>;

public:
	virtual Error load(const char* path) override;

private:
	ManagedFIB dataPtr; ///< Opaque pointer to the internal data.

	void clear();
};

Error ImageImpl::load(const char* path) {
	FREE_IMAGE_FORMAT imgFormat = getImageFormat(path);
	const int imgFlags = getImageLoadFlags(imgFormat);
	FIBITMAP* fib = FreeImage_Load(imgFormat, path, imgFlags);
	if (!fib) {
		return Error("Failed to load image");
	}

	dataPtr.reset(fib);

	return Error();
}

void ImageImpl::clear() {
	dataPtr.reset();
}

// ################################################################################################################################
// # ImageManager
// ################################################################################################################################

/// From the docs...
/// Generic image loader
/// @param lpszPathName Pointer to the full file name
/// @param flag Optional load flag constant
/// @return Returns the loaded dib if successful, returns NULL otherwise
static FIBITMAP* GenericLoader(const char* lpszPathName, int flag) {
	FREE_IMAGE_FORMAT fif = FIF_UNKNOWN;
	// check the file signature and deduce its format
	// (the second argument is currently not used by FreeImage)
	fif = FreeImage_GetFileType(lpszPathName, 0);
	if(fif == FIF_UNKNOWN) {
		// no signature ?
		// try to guess the file format from the file extension
		fif = FreeImage_GetFIFFromFilename(lpszPathName);
	}
	// check that the plugin has reading capabilities ...
	if((fif != FIF_UNKNOWN) && FreeImage_FIFSupportsReading(fif)) {
		// ok, let's load the file
		FIBITMAP *dib = FreeImage_Load(fif, lpszPathName, flag);
		// unless a bad file format, we are done !
		return dib;
	}
	return NULL;
}

bool ImageManager::accepts(const char* path) {
	FREE_IMAGE_FORMAT imgFormat = getImageFormat(path);
	return FreeImage_FIFSupportsReading(imgFormat);
}

void ImageManager::triggerLoad(const char* path) {
	if (loadingImage) return;

	loadingImage = true;
	Error err = nextImage->load(path);
	loadingImage = false;
}

