#pragma once
#include <memory>

#include "error.h"

class Image {
public:
	virtual Error load(const char* path) = 0;
};

class ImageManager {
public:
	/// Check if the file is an image that we can load.
	/// @param path The file path.
	/// @return True if it can be loaded.
	bool accepts(const char* path);

	void triggerLoad(const char* path);

private:
	std::unique_ptr<Image> currentImage; ///< The image to show.
	std::unique_ptr<Image> nextImage; ///< The image that we load.

	bool loadingImage = false; ///< Set to true when we start loading an image.
};
