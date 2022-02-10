#include "image.hpp"
#include <X11/Xlib.h>
#include <cstdio>
#include <iostream>

Pixmap LoadImage(const char *file, Display *display, Window root) {
    Imlib_Image img = imlib_load_image(file);
    if (!img) {
        fprintf(stderr, "Cannot load image: %s", file);
        exit(1);
    }

    imlib_context_set_image(img);

    int width = imlib_image_get_width();
    int height = imlib_image_get_height();

    Screen *scn;
    scn = DefaultScreenOfDisplay(display);

    Pixmap pix = XCreatePixmap(display, root, width, height, XDefaultDepthOfScreen(scn));

    imlib_context_set_display(display);
    imlib_context_set_visual(DefaultVisualOfScreen(scn));
    imlib_context_set_colormap(XDefaultColormapOfScreen(scn));
    imlib_context_set_drawable(pix);

    imlib_render_image_on_drawable(0, 0);

    return pix;

}
