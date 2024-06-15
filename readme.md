# Content-aware resizing with Seam Carving

This project is based on the [paper](http://graphics.cs.cmu.edu/courses/15-463/2007_fall/hw/proj2/imret.pdf), which explores more applications for seam carving.

## Features

- Image resizing to a strictly smaller resolution.
- OpenGL based viewport with pan and zoom control.
- Support loading and saving a wide variety of image format, thanks to FreeImage. FreeImage is an open source image library. See http://freeimage.sourceforge.net for details.
- OS: Windows only

## Build

The project uses the CMake build system generator. It supplies an INSTALL target that can be customized with `CMAKE_INSTALL_PREFIX`.
