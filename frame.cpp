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
    unsigned long valuemask = CWBackPixel | CWBorderPixel | CWEventMask;
    XSetWindowAttributes frame_attr;
    frame_attr.border_pixel = FRAME_BORDER_COLOR;
    frame_attr.background_pixel = FRAME_BG_COLOR;
    frame_attr.event_mask = ExposureMask | SubstructureNotifyMask | ButtonPressMask;
    frame_win = XCreateWindow(display, root, attrs.x, attrs.y, attrs.width + CLIENT_OFFSET_X, attrs.height + CLIENT_OFFSET_Y+BUTTON_PADDING*2, FRAME_BORDER_WIDTH,
            DefaultDepth(display, screen_num), InputOutput, DefaultVisual(display, screen_num), valuemask, &frame_attr);

    XSelectInput(display, frame_win, ExposureMask | SubstructureNotifyMask);

    // Close button
    close_win = XCreateSimpleWindow(display, frame_win, attrs.x+attrs.width-BUTTON_SIZE-2*BUTTON_BORDER_WIDTH-BUTTON_PADDING, attrs.y+BUTTON_PADDING, BUTTON_SIZE, BUTTON_SIZE, BUTTON_BORDER_WIDTH, BUTTON_BORDER_COLOR, BUTTON_BG_COLOR_R);

    max_win = XCreateSimpleWindow(display, frame_win, attrs.x+attrs.width-2*BUTTON_SIZE-4*BUTTON_BORDER_WIDTH-DISTANCE_BETWEEN_BUTTONS-BUTTON_PADDING, attrs.y+BUTTON_PADDING, BUTTON_SIZE, BUTTON_SIZE, BUTTON_BORDER_WIDTH, BUTTON_BORDER_COLOR, BUTTON_BG_COLOR_B);
    
    min_win = XCreateSimpleWindow(display, frame_win, attrs.x+attrs.width-3*BUTTON_SIZE-6*BUTTON_BORDER_WIDTH-2*DISTANCE_BETWEEN_BUTTONS-BUTTON_PADDING, attrs.y+BUTTON_PADDING, BUTTON_SIZE, BUTTON_SIZE, BUTTON_BORDER_WIDTH, BUTTON_BORDER_COLOR, BUTTON_BG_COLOR_B);

    // Add client to save set so it will be kept alive if WM crashes
    XAddToSaveSet(display, win_to_frame);

    // Reparent client window- triggers ReparentNotify which will be ignored
    XReparentWindow(display, win_to_frame, frame_win, CLIENT_OFFSET_X, CLIENT_OFFSET_Y+BUTTON_PADDING*2);

    // Map frame- generates MapNotify which will be ignored
    XMapWindow(display, frame_win);

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

void Frame::ResizeFrame(Display *display, int width, int height, int x_mouse, int y_mouse, int delta_x, int delta_y, bool top, bool bottom, bool left, bool right) {


    Window returned_root;
    int original_x, original_y;
    unsigned original_width, original_height, border_width, depth;
    XGetGeometry(display, frame_win, &returned_root, &original_x, &original_y, &original_width, &original_height, &border_width, &depth);

    if(top && left) {

        XResizeWindow(display, frame_win, width-2*delta_x, height-2*delta_y);
        XResizeWindow(display, client_win, width-2*delta_x-CLIENT_OFFSET_X, height-2*delta_y-CLIENT_OFFSET_Y-2*BUTTON_PADDING);
        MoveFrame(display, x_mouse, y_mouse);//TODO: Dont just warp to mouse
        UpdateButtonLocations(display);

    } else if(top && right) {

        XResizeWindow(display, frame_win, width, height-2*delta_y);
        XResizeWindow(display, client_win, width-CLIENT_OFFSET_X, height-2*delta_y-CLIENT_OFFSET_Y-2*BUTTON_PADDING);
        MoveFrame(display, original_x, y_mouse);//TODO: Dont just warp to mouse
        UpdateButtonLocations(display);

    } else if(bottom && left){

        XResizeWindow(display, frame_win, width-2*delta_x, height);
        XResizeWindow(display, client_win, width-2*delta_x-CLIENT_OFFSET_X, height-CLIENT_OFFSET_Y-2*BUTTON_PADDING);
        MoveFrame(display, x_mouse, original_y);
        UpdateButtonLocations(display);

    } else if(bottom && right) {

        //if(width < 1) { width = 1; }
        //if(height < CLIENT_OFFSET_Y+2*BUTTON_PADDING) { height = CLIENT_OFFSET_Y + 2*BUTTON_PADDING; }
        XResizeWindow(display, frame_win, width, height);
        XResizeWindow(display, client_win, width-CLIENT_OFFSET_X, height-CLIENT_OFFSET_Y-2*BUTTON_PADDING);
        UpdateButtonLocations(display);

    } else if(top) {

        XResizeWindow(display, frame_win, original_width, height-2*delta_y);
        XResizeWindow(display, client_win, original_width-CLIENT_OFFSET_X, height-2*delta_y-CLIENT_OFFSET_Y-2*BUTTON_PADDING);
        MoveFrame(display, original_x, y_mouse);//TODO: Dont just warp to mouse
        UpdateButtonLocations(display);

    } else if(bottom) {

        //if(height < CLIENT_OFFSET_Y+2*BUTTON_PADDING) { height = CLIENT_OFFSET_Y + 2*BUTTON_PADDING; }

        XResizeWindow(display, frame_win, original_width, height);
        XResizeWindow(display, client_win, original_width-CLIENT_OFFSET_X, height-CLIENT_OFFSET_Y-2*BUTTON_PADDING);
        UpdateButtonLocations(display);

    } else if(left) {

        XResizeWindow(display, frame_win, width-2*delta_x, original_height);
        XResizeWindow(display, client_win, width-2*delta_x-CLIENT_OFFSET_X, original_height-CLIENT_OFFSET_Y-2*BUTTON_PADDING);
        MoveFrame(display, x_mouse, original_y);
        UpdateButtonLocations(display);

    } else if(right) {

        XResizeWindow(display, frame_win, width, original_height);
        XResizeWindow(display, client_win, width-CLIENT_OFFSET_X, original_height-CLIENT_OFFSET_Y-2*BUTTON_PADDING);
        UpdateButtonLocations(display);

    } else {
        //Panic!
        return;
    }

    UpdateClientLocation(display);
}

void Frame::MoveFrame(Display *display, int x, int y) {
    XMoveWindow(display, frame_win, x, y);
    UpdateClientLocation(display);
}
