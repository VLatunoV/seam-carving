#pragma once
#include <memory>

#include "error.h"
#include "observer.h"
#include "saveHandler.h"

template <bool>
struct CarveHelper;

/// Represents one pixel.
struct Pixel {
	uint8_t r;
	uint8_t g;
	uint8_t b;
};

class Image {
	template<bool>
	friend struct CarveHelper;

public:
	/// We shouldn't need to copy images around.
	Image& operator=(const Image&) = delete;

	/// Return true if the image has size and data.
	operator bool();

	/// Copy the given image into this.
	void copyFrom(Image& other);

	/// Load an image given its path.
	/// The image can be any of the supported types by FreeImage library. Called by the ImageManager
	/// after checking that the given path is an image that we can read.
	Error load(const char* path);
	/// Save the image to the given file path.
	Error save(const char* path);

	/// @{
	/// Accessors.
	int getWidth() const;
	int getHeight() const;
	int getStride() const;
	const Pixel* getData();
	const float* getEnergy();
	/// @}

	/// Return true if the image is valid, i.e. it has dimensions and data.
	bool isValid() const;

	/// Find horizontal seams with lowest energies connecting both vertical borders and removes them.
	/// @param howMany Number of seams to remove.
	void carveRows(int howMany);

	/// Find vertical seams with lowest energies connecting both horizontal borders and removes them.
	/// @param howMany Number of seams to remove.
	void carveCols(int howMany);

private:
	int width = 0; /// Width in pixels.
	int height = 0; /// Height in pixels.
	int stride = 0; /// Offset in pixels to the next row.
	int capacity = 0; ///< Size of allocated arrays.
	/// Holds the image data as 8bit RGBA values. Alpha is not used.
	std::unique_ptr<Pixel[]> data;
	/// Holds the pixel energies used to do seam carving.
	std::unique_ptr<float[]> energy;

	/// Calculated the energies for the image.
	void computeEnergies();

	/// Allocates all memory.
	/// @param newCap Capacity.
	void allocMemory(int newCap);
};

class ImageManager
	: public Observable<ImageManagerObserver>
{
public:
	/// Check if the file is an image that we can load.
	/// @param path The file path.
	/// @return True if it can be loaded.
	bool accepts(const char* path);

	/// Return the current image to use.
	Image& getActiveImage();
	/// Return the original image.
	Image& getOriginalImage();

	/// Load the image from file.
	void triggerLoad(const char* path);
	/// Save the image to file. The user is prompted to choose the file name and location.
	void triggerSave();

	/// Start the seam carving. We remove seams until the image reaches the given size.
	/// If the image is smaller than the target size, we start over from the original.
	void triggerSeam(int targetWidth, int targetHeight);

private:
	/// Used to get the file path for the saved image.
	SaveImageHandler saveHandler;

	/// We always keep the original image. When we have to seam carve to a size lower than our current one, we can
	/// reuse the current carved image and reduce it. If any of the dimensions is larger, we start again from the
	/// original.
	/// @note The order matters, so we will get different results if the user asks for different sizes, but that is
	///	    not a problem).
	/// @{
	Image activeImage; ///< The image to show. Can be empty, in which case we show the original.
	Image originalImage; ///< The image that we load.
	/// @}

	/// Set to true when we apply seam carving to the image. When true, we use the active image.
	bool isSeamModified = false;
};
