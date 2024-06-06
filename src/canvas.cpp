#include <Windows.h> // Need to include before gl ;(
#include <gl/GL.h>

#include "canvas.h"
#include "image.h"

Canvas::Canvas(ImageManager& _imgManager)
	: imgManager(_imgManager)
{}

void Canvas::updateCanvasSize(int _width, int _height) {
	if (width == _width && height == _height) {
		return;
	}

	width = _width;
	height = _height;
}

void Canvas::makeTexture() {
	if (!imgManager.hasNewImage()) {
		return;
	}
	Image* image = imgManager.getCurrentImage();
	if (!image || !image->isValid()) {
		return;
	}
	if (textureID != 0) {
		glDeleteTextures(1, &textureID);
	}
	glGenTextures(1, &textureID);
	glBindTexture(GL_TEXTURE_2D, textureID);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexImage2D(
		GL_TEXTURE_2D,
		0, // level
		GL_RGB,
		image->getWidth(),
		image->getHeight(),
		0, // border
		GL_RGB,
		GL_UNSIGNED_BYTE,
		image->getData()
	);
	glBindTexture(GL_TEXTURE_2D, 0);
}

void Canvas::draw() {
	if (textureID == 0) {
		return;
	}

	glViewport(0, 0, width, height);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, width, height, 0, 0, 1);
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, textureID);
	glBegin(GL_QUADS);
	glTexCoord2f(0.0f, 0.0f); glVertex2i(0, 0);
	glTexCoord2f(1.0f, 0.0f); glVertex2i(width, 0);
	glTexCoord2f(1.0f, 1.0f); glVertex2i(width, height);
	glTexCoord2f(0.0f, 1.0f); glVertex2i(0, height);
	glEnd();
}
