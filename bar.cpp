#include "bar.hpp"
#include <X11/X.h>
extern "C" {
#include <X11/Xlib.h>
}
#include "util.hpp"
#include <cstdio>
#include <iostream>

void Bar::Create(Display *display, Window root) {

    Window returned_root;
    int x_root, y_root;
    unsigned width_root, height_root, border_width_root, depth_root;
    XGetGeometry(display, root, &returned_root, &x_root, &y_root, &width_root, &height_root, &border_width_root, &depth_root);


    bar_win = XCreateSimpleWindow(display, root, 0, height_root-BAR_HEIGHT, width_root, BAR_HEIGHT, BAR_BORDER_WIDTH, BAR_BORDER_COLOR, BAR_COLOR);

    XMapWindow(display, bar_win);
}
