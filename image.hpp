#ifndef IMAGE_HPP
#define IMAGE_HPP

extern "C" {
#include <X11/Xlib.h>
}
#include <Imlib2.h>
#include <cstdio>
#include <iostream>

Pixmap LoadImage(const char *file, Display *display, Window root);

#endif
