#pragma once
#include <memory>

#include "error.h"

class Image {
public:
	/// We shouldn't need to copy images around.
	Image& operator=(const Image&) = delete;

	/// Return true if the image is valid.
	operator bool();

	/// Load an image given its path.
	/// The image can be any of the supported types by FreeImage library. Called by the ImageManager
	/// after checking that the given path is an image that we can read.
	Error load(const char* path);

	int getWidth() const;
	int getHeight() const;
	const uint8_t* getData();

	/// Return true if the image is valid, i.e. it has dimensions and data.
	bool isValid() const;

private:
	int width = 0;
	int height = 0;
	/// Holds the image data as 8bit RGB values.
	std::unique_ptr<uint8_t[]> data;
};

class ImageManager {
public:
	/// Check if the file is an image that we can load.
	/// @param path The file path.
	/// @return True if it can be loaded.
	bool accepts(const char* path);

	/// Loads the image from file.
	void triggerLoad(const char* path);

	/// Return the current image to use. Can be nullptr if it wasn't loaded yet.
	Image* getCurrentImage();

	/// Returns true if the current image is updated.
	bool hasNewImage() const;

private:
	std::unique_ptr<Image> currentImage; ///< The image to show.
	std::unique_ptr<Image> nextImage; ///< The image that we load.

	/// Set to true when we change the currentImage. When @p getCurrentImage() is called, we clear it.
	bool imageUpdated = false;
};
