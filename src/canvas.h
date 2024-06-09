#pragma once
#include "observer.h"

class ImageManager;

/// Handles drawing the image onto the screen and mouse input.
class Canvas
	: public ImageManagerObserver
{
public:
	Canvas(ImageManager& imgManager);

	/// Called from the main thread (UI).
	/// @{
	/// Called to update our internal state. We can change our size, recreate the
	/// texture, etc.
	void update(int width, int height);

	/// Calls OpenGL drawing functions.
	void draw();
	/// @}

	void pan(int xoffset, int yoffset);
	void zoom(int value);

	// From ImageManagerObserver
	virtual void onImageChange() override;

private:
	ImageManager& imgManager; ///< Image manager to get the image.
	unsigned int textureID = 0; ///< OpenGL texture id.
	bool imageUpdated = false; ///< Set to true if the image has been updated. In this case we have to recreate the texture.

	int width = 0; ///< Canvas width.
	int height = 0; ///< Canvas height.

	int geomWidth = 0;
	int geomHeight = 0;
	int zoomValue = 0;
	float centerX = 0.0f;
	float centerY = 0.0f;

	/// Updates the canvas available size.
	/// Used to compute texture offsets and relative size.
	/// @note It is called every frame, so we exit early if the size haven't changed.
	void updateCanvasSize(int width, int height);

	/// Sets correct image geometry based on the canvas size. It will tightly fit the given
	/// image dimensions inside the canvas size.
	/// @param imgW Image width.
	/// @param imgH Image height.
	void updateImageGeometry(int imgW, int imgH);

	/// (Re-)Creates the OpenGL texture. Has to be called from the main thread.
	void makeTexture();
};
