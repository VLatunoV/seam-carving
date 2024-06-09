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
	/// @param width Width of the available area.
	/// @param height Height of the available area.
	void update(int width, int height);

	/// Calls OpenGL drawing functions.
	void draw();
	/// @}

	/// Translate the image. It is based on the zoom level.
	/// @param xoffset Number of pixels the mouse moved.
	/// @param yoffset Number of pixels the mouse moved.
	void pan(int xoffset, int yoffset);

	/// Zoom the image.
	/// Based on the mouse position, it will also translate the image so that
	/// the zoom center is at the mouse position.
	/// @param scroll The scroll amount. This is a number of scroll wheel ticks (only for scroll
	///		wheels with stepped spin).
	/// @param mouseX Mouse position when scrolling.
	/// @param mouseY Mouse position when scrolling.
	void zoom(int scroll, int mouseX, int mouseY);

	/// Reset the zoom and pan.
	void resetTransform();

	// From ImageManagerObserver
	virtual void onImageChange() override;

private:
	ImageManager& imgManager; ///< Image manager to get the image.
	unsigned int textureID = 0; ///< OpenGL texture id.
	bool imageUpdated = false; ///< Set to true if the image has been updated. In this case we have to recreate the texture.

	int width = 0; ///< Canvas width.
	int height = 0; ///< Canvas height.

	int geomWidth = 0; ///< The image width being drawn.
	int geomHeight = 0; ///< The image height being drawn.
	int zoomValue = 0; ///< Zoom value, used for exponential zooming.
	int zoomSpeed = 2; ///< How much to zoom at each step.
	float centerX = 0.0f; ///< Center offset from (0, 0).
	float centerY = 0.0f; ///< Center offset from (0, 0).

	/// Updates the canvas available size.
	/// Used to compute texture offsets and relative size.
	/// @note It is called every frame, so we exit early if the size haven't changed.
	/// @param width Canvas width.
	/// @param height Canvas height.
	void updateCanvasSize(int width, int height);

	/// Sets correct image geometry based on the canvas size. It will tightly fit the given
	/// image dimensions inside the canvas size.
	/// @param imgW Image width.
	/// @param imgH Image height.
	void updateImageGeometry(int imgW, int imgH);

	/// (Re-)Creates the OpenGL texture. Has to be called from the main thread.
	void makeTexture();

	/// Map the interger zoom value to a real scale.
	/// @param value Zoom value.
	inline float calcScale(int value);
};
