#include "frame.hpp"
#include <X11/X.h>
#include <Imlib2.h>
extern "C" {
#include <X11/Xlib.h>
}
#include "util.hpp"
#include <cstdio>
#include <iostream>

using namespace std;

Frame::~Frame() {

}

void Frame::Create(Display *display, Window root, Window win_to_frame, XWindowAttributes attrs) {

    // Save client window
    client_win = win_to_frame;

    // Screen number
    int screen_num = DefaultScreen(display);

    // Create frame- triggers CreateNotify which will be ignored
    unsigned long valuemask = CWBackPixel | CWBorderPixel | CWEventMask;
    XSetWindowAttributes frame_attr;
    frame_attr.border_pixel = FRAME_BORDER_COLOR;
    frame_attr.background_pixel = FRAME_BG_COLOR;
    frame_attr.event_mask = ExposureMask | SubstructureNotifyMask | ButtonPressMask;
    frame_win = XCreateWindow(display, root, attrs.x, attrs.y, attrs.width + CLIENT_OFFSET_X, attrs.height + CLIENT_OFFSET_Y+BUTTON_PADDING*2, FRAME_BORDER_WIDTH,
            DefaultDepth(display, screen_num), InputOutput, DefaultVisual(display, screen_num), valuemask, &frame_attr);
    printf("%d, %d\n", attrs.width, attrs.height);

    XSelectInput(display, frame_win, ExposureMask | SubstructureNotifyMask);

    // Close button
    close_win = XCreateSimpleWindow(display, frame_win, attrs.x+attrs.width-BUTTON_SIZE-2*BUTTON_BORDER_WIDTH-BUTTON_PADDING, attrs.y+BUTTON_PADDING, BUTTON_SIZE, BUTTON_SIZE, BUTTON_BORDER_WIDTH, BUTTON_BORDER_COLOR, BUTTON_BG_COLOR_R);

    max_win = XCreateSimpleWindow(display, frame_win, attrs.x+attrs.width-2*BUTTON_SIZE-4*BUTTON_BORDER_WIDTH-DISTANCE_BETWEEN_BUTTONS-BUTTON_PADDING, attrs.y+BUTTON_PADDING, BUTTON_SIZE, BUTTON_SIZE, BUTTON_BORDER_WIDTH, BUTTON_BORDER_COLOR, BUTTON_BG_COLOR_B);
    
    min_win = XCreateSimpleWindow(display, frame_win, attrs.x+attrs.width-3*BUTTON_SIZE-6*BUTTON_BORDER_WIDTH-2*DISTANCE_BETWEEN_BUTTONS-BUTTON_PADDING, attrs.y+BUTTON_PADDING, BUTTON_SIZE, BUTTON_SIZE, BUTTON_BORDER_WIDTH, BUTTON_BORDER_COLOR, BUTTON_BG_COLOR_B);

    // Add client to save set so it will be kept alive if WM crashes
    XAddToSaveSet(display, win_to_frame);

    // Reparent client window- triggers ReparentNotify which will be ignored
    XReparentWindow(display, win_to_frame, frame_win, CLIENT_OFFSET_X + CLIENT_PADDING_LEFT, CLIENT_OFFSET_Y+BUTTON_PADDING*2 + CLIENT_PADDING_TOP);

    //Pixmap window_pix = LoadImage("window.bmp", display, root);
    //XSetWindowBackgroundPixmap(display, frame_win, window_pix);

    // Map frame- generates MapNotify which will be ignored
    XMapWindow(display, frame_win);

    Pixmap close_pix = LoadImage("close.bmp", display, root);
    XSetWindowBackgroundPixmap(display, close_win, close_pix);

    Pixmap max_pix = LoadImage("maximize.bmp", display, root);
    XSetWindowBackgroundPixmap(display, max_win, max_pix);

    Pixmap min_pix = LoadImage("minimize.bmp", display, root);
    XSetWindowBackgroundPixmap(display, min_win, min_pix);

    // Map buttons
    XMapWindow(display, close_win);
    XMapWindow(display, max_win);
    XMapWindow(display, min_win);

    UpdateButtonLocations(display);

}

void Frame::UpdateButtonLocations(Display *display) {
    XWindowAttributes attrs;
    XGetWindowAttributes(display, frame_win, &attrs);
    XMoveWindow(display, close_win, attrs.width-BUTTON_SIZE-BUTTON_BORDER_WIDTH*2-BUTTON_PADDING, BUTTON_PADDING);
    XMoveWindow(display, max_win, attrs.width-2*BUTTON_SIZE-4*BUTTON_BORDER_WIDTH-DISTANCE_BETWEEN_BUTTONS-BUTTON_PADDING, BUTTON_PADDING);
    XMoveWindow(display, min_win, attrs.width-3*BUTTON_SIZE-6*BUTTON_BORDER_WIDTH-2*DISTANCE_BETWEEN_BUTTONS-BUTTON_PADDING, BUTTON_PADDING);
}

void Frame::UpdateClientLocation(Display *display) {
    XMoveWindow(display, client_win, CLIENT_OFFSET_X, CLIENT_OFFSET_Y+BUTTON_PADDING*2);
}

void Frame::ResizeFrame(Display *display, int width, int height){
    XResizeWindow(display, frame_win, width, height);
    XResizeWindow(display, client_win, width-CLIENT_OFFSET_X, height-CLIENT_OFFSET_Y-2*BUTTON_PADDING);
    UpdateButtonLocations(display);
    UpdateClientLocation(display);
}

void Frame::MoveFrame(Display *display, int x, int y) {
    XMoveWindow(display, frame_win, x, y);
    UpdateButtonLocations(display);
    UpdateClientLocation(display);
}

Pixmap Frame::LoadImage(const char *file, Display *display, Window root) {
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
