#include "frame.hpp"
#include <X11/X.h>
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
    frame_win = XCreateSimpleWindow(display, root, attrs.x, attrs.y, attrs.width + CLIENT_OFFSET_X, attrs.height + CLIENT_OFFSET_Y, FRAME_BORDER_WIDTH, FRAME_BORDER_COLOR, FRAME_BG_COLOR);

    //unsigned long valuemask = CWBackPixel | CWBorderPixel | CWEventMask;
    //XSetWindowAttributes frame_attr;
    //frame_attr.border_pixel = FRAME_BORDER_COLOR;
    //frame_attr.background_pixel = FRAME_BG_COLOR;
    //frame_attr.event_mask = ButtonPressMask | ExposureMask;
    //frame_win = XCreateWindow(display, root, attrs.x, attrs.y, attrs.width + CLIENT_OFFSET_X, attrs.height + CLIENT_OFFSET_Y, FRAME_BORDER_WIDTH,
    //        DefaultDepth(display, screen_num), InputOutput, DefaultVisual(display, screen_num), valuemask, &frame_attr);

    XSelectInput(display, frame_win, ButtonPressMask | ButtonReleaseMask | SubstructureNotifyMask | SubstructureRedirectMask);

    // Close button
    close_win = XCreateSimpleWindow(display, frame_win, attrs.x+attrs.width-BUTTON_SIZE-2*BUTTON_BORDER_WIDTH, attrs.y, BUTTON_SIZE, BUTTON_SIZE, BUTTON_BORDER_WIDTH, BUTTON_BORDER_COLOR, BUTTON_BG_COLOR_R);
    max_win = XCreateSimpleWindow(display, frame_win, attrs.x+attrs.width-2*BUTTON_SIZE-4*BUTTON_BORDER_WIDTH-DISTANCE_BETWEEN_BUTTONS, attrs.y, BUTTON_SIZE, BUTTON_SIZE, BUTTON_BORDER_WIDTH, BUTTON_BORDER_COLOR, BUTTON_BG_COLOR_B);
    min_win = XCreateSimpleWindow(display, frame_win, attrs.x+attrs.width-3*BUTTON_SIZE-6*BUTTON_BORDER_WIDTH-2*DISTANCE_BETWEEN_BUTTONS, attrs.y, BUTTON_SIZE, BUTTON_SIZE, BUTTON_BORDER_WIDTH, BUTTON_BORDER_COLOR, BUTTON_BG_COLOR_B);

    // Add client to save set so it will be kept alive if WM crashes
    XAddToSaveSet(display, win_to_frame);

    // Reparent client window- triggers ReparentNotify which will be ignored
    XReparentWindow(display, win_to_frame, frame_win, CLIENT_OFFSET_X, CLIENT_OFFSET_Y);

    // Map frame- generates MapNotify which will be ignored
    XMapWindow(display, frame_win);

    // Map buttons
    XMapWindow(display, close_win);
    XMapWindow(display, max_win);
    XMapWindow(display, min_win);

}

void Frame::UpdateButtonLocations(Display *display) {
    XWindowAttributes attrs;
    XGetWindowAttributes(display, frame_win, &attrs);
    XMoveWindow(display, close_win, attrs.width-BUTTON_SIZE-BUTTON_BORDER_WIDTH*2, 0);
    XMoveWindow(display, max_win, attrs.width-2*BUTTON_SIZE-4*BUTTON_BORDER_WIDTH-DISTANCE_BETWEEN_BUTTONS, 0);
    XMoveWindow(display, min_win, attrs.width-3*BUTTON_SIZE-6*BUTTON_BORDER_WIDTH-2*DISTANCE_BETWEEN_BUTTONS, 0);
}

void Frame::ResizeFrame(Display *display, int width, int height) {
    XResizeWindow(display, frame_win, width, height);
    XResizeWindow(display, client_win, width-CLIENT_OFFSET_X, height-CLIENT_OFFSET_Y);
    UpdateButtonLocations(display);
}

void Frame::MoveFrame(Display *display, int x, int y) {
    XMoveWindow(display, frame_win, x, y);
}
