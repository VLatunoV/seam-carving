#pragma once

class ImageManager;

/// Handles drawing the image onto the screen and mouse input.
class Canvas {
public:
	Canvas(ImageManager& imgManager);

	/// Updates the canvas available size.
	/// Used to compute texture offsets and relative size.
	void updateCanvasSize(int width, int height);
	void makeTexture();
	void draw();

	void pan(int xoffset, int yoffset);
	void zoom(int value);

private:
	ImageManager& imgManager; ///< Image manager to get the image.
	unsigned int textureID = 0; ///< OpenGL texture id.

	int width = 0; ///< Canvas width.
	int height = 0; ///< Canvas height.
	int zoomExp = 0;
	float centerX = 0.0f;
	float centerY = 0.0f;
};
