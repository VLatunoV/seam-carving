#include <Windows.h> // Need to include before gl ;(
#include <gl/GL.h>

#include "canvas.h"
#include "image.h"

Canvas::Canvas(ImageManager& _imgManager)
	: imgManager(_imgManager)
{}

void Canvas::update(int _width, int _height) {
	updateCanvasSize(_width, _height);

	if (imageUpdated) {
		makeTexture();
		imageUpdated = false;
	}
}

void Canvas::draw() {
	if (textureID == 0) {
		return;
	}

	const int canvasW = width / 2;
	const int canvasH = height / 2;
	const int geomW = geomWidth / 2;
	const int geomH = geomHeight / 2;
	const float scaleFactor = calcScale(zoomValue);

	glViewport(0, 0, width, height);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(-canvasW, width-canvasW, height-canvasH, -canvasH, 0, 1);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	// Multiplies the current matrix, so the order of transformations is reversed.
	// We want first scale, then rotate, then translate.
	glTranslatef(centerX, centerY, 0.0f);
	glScalef(scaleFactor, scaleFactor, 1.0f);
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, textureID);
	glBegin(GL_QUADS);
	glTexCoord2f(0.0f, 0.0f); glVertex2i(-1 - geomW,           -1 - geomH);
	glTexCoord2f(1.0f, 0.0f); glVertex2i(+1 + geomWidth-geomW, -1 - geomH);
	glTexCoord2f(1.0f, 1.0f); glVertex2i(+1 + geomWidth-geomW, +1 + geomHeight-geomH);
	glTexCoord2f(0.0f, 1.0f); glVertex2i(-1 - geomW,           +1 + geomHeight-geomH);
	glEnd();
}

void Canvas::pan(int xoffset, int yoffset) {
	centerX += float(xoffset);
	centerY += float(yoffset);
}

void Canvas::zoom(int scroll, int mouseX, int mouseY) {
	int value = scroll * zoomSpeed;
	const float nextZoom = calcScale(zoomValue+value);
	if (nextZoom < 0.05f || nextZoom > 1025.0f) {
		return;
	}

	const float scaleDelta = calcScale(value);
	float mOffsetX = float(mouseX - width/2);
	float mOffsetY = float(mouseY - height/2);
	// We move the image so that the point where the mouse is pointing becomes the center,
	// and then scale with 'value'.
	centerX += (scaleDelta - 1.0f) * (centerX - mOffsetX);
	centerY += (scaleDelta - 1.0f) * (centerY - mOffsetY);
	zoomValue += value;
}

void Canvas::resetTransform() {
	centerX = 0.0f;
	centerY = 0.0f;
	zoomValue = 0;
}

void Canvas::onImageChange() {
	Image* image = imgManager.getCurrentImage();
	const int imgW = image->getWidth();
	const int imgH = image->getHeight();
	updateImageGeometry(imgW, imgH);
	resetTransform();
	imageUpdated = true;
}

void Canvas::updateCanvasSize(int _width, int _height) {
	if (width == _width && height == _height) {
		return;
	}

	width = _width;
	height = _height;
	Image* image = imgManager.getCurrentImage();
	if (!image || !image->isValid()) {
		return;
	}

	const int imgW = image->getWidth();
	const int imgH = image->getHeight();
	updateImageGeometry(imgW, imgH);
}

void Canvas::updateImageGeometry(int imgW, int imgH) {
	double aspect = double(imgW) / double(imgH);
	if (int64_t(width) * imgH > int64_t(height) * imgW) {
		// Canvas size is wider than the image.
		geomHeight = height;
		geomWidth = int(aspect * double(height) + 0.5);
	} else {
		// Canvas is taller than the image.
		geomWidth = width;
		geomHeight = int(double(width) / aspect + 0.5);
	}
}

void Canvas::makeTexture() {
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
