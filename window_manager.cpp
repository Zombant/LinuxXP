#include "window_manager.hpp"
#include "frame.hpp"
#include <X11/X.h>
#include <X11/Xlib.h>
extern "C" {
#include <X11/Xutil.h>
}
#include "util.hpp"
#include <cstdio>
#include <iostream>
#include <algorithm>
#include <cstring>

using ::std::unique_ptr;
using namespace std;

bool WindowManager::wm_detected_;

unique_ptr<WindowManager> WindowManager::Create() {
    // Open X Display
    Display* display = XOpenDisplay(nullptr);
    if(display == nullptr){
        fprintf(stderr, "Failed to open X display\n");
        return nullptr;
    }

    // Construct WindowManager instance
    return unique_ptr<WindowManager>(new WindowManager(display));
}

WindowManager::WindowManager(Display* display) : display_(display), root_(DefaultRootWindow(display_)),
    WM_PROTOCOLS(XInternAtom(display_, "WM_PROTOCOLS", false)),
    WM_DELETE_WINDOW(XInternAtom(display_, "WM_DELETE_WINDOW", false)) {}

WindowManager::~WindowManager() {
    XCloseDisplay(display_);
}

void WindowManager::Start(){
    Setup();
    Run();
}

void WindowManager::Setup() {
    // Initialization
    // Selects events on root window. Error handler to exist gracefully if another WM is running
    wm_detected_ = false;
    XSetErrorHandler(&WindowManager::OnWMDetected);

    // Select events on the root
    XSelectInput(display_, root_, SubstructureRedirectMask | SubstructureNotifyMask | ButtonPressMask | ButtonReleaseMask | ButtonMotionMask
            | KeyPressMask | KeyReleaseMask | PointerMotionMask);

    // Syncronously grab the the left button on the root
    XGrabButton(display_, Button1, AnyModifier, root_, false, Button1Mask, GrabModeSync, GrabModeAsync, None, None);

    XGrabKey(display_, XKeysymToKeycode(display_, XK_r), Mod1Mask, root_, false, GrabModeAsync, GrabModeAsync);


    XSync(display_, false);
    if(wm_detected_){
        fprintf(stderr, "Another window manager is running\n");
        exit(1);
    }

    // Set error handler
    XSetErrorHandler(&WindowManager::OnXError);

    // Grab X server to prevent windows from changes while framing them
    XGrabServer(display_);

    // Frame existing top-level windows

    // Query existing top-level windows
    Window returned_root, returned_parent;
    Window* top_level_windows;
    unsigned int num_top_level_windows;
    XQueryTree(display_, root_, &returned_root, &returned_parent, &top_level_windows, &num_top_level_windows);

    // Frame each top-level window
    for(unsigned int i = 0; i < num_top_level_windows; ++i){
        FrameWindow(top_level_windows[i], true);
    }

    // Free top-level window array
    XFree(top_level_windows);

    // Create cursors
    top_left_cursor = XCreateFontCursor(display_, XC_top_left_corner);
    top_right_cursor = XCreateFontCursor(display_, XC_top_right_corner);
    bottom_left_cursor = XCreateFontCursor(display_, XC_bottom_left_corner);
    bottom_right_cursor = XCreateFontCursor(display_, XC_bottom_right_corner);
    bottom_left_cursor = XCreateFontCursor(display_, XC_bottom_left_corner);
    bottom_cursor = XCreateFontCursor(display_, XC_bottom_side);
    top_cursor = XCreateFontCursor(display_, XC_top_side);
    left_cursor = XCreateFontCursor(display_, XC_left_side);
    right_cursor = XCreateFontCursor(display_, XC_right_side);
    default_cursor = XCreateFontCursor(display_, XC_left_ptr);
    XDefineCursor(display_, root_, default_cursor);

    // Set up the bar
    bar.Create(display_, root_);

    // Ungrab X server
    XUngrabServer(display_);
}

void WindowManager::Run() {

    // Main event loop
    for (;;) {
        // Get next event
        XEvent e;
        XNextEvent(display_, &e);

        // Choose event
        switch (e.type) {
            case ReparentNotify:
                OnReparentNotify(e.xreparent);
                //printf("ReparentNotify\n");
                break;
            case MapRequest:
                OnMapRequest(e.xmaprequest);
                //printf("MapRequest\n");
                break;
            case ConfigureRequest:
                OnConfigureRequest(e.xconfigurerequest);
                //printf("ConfigureRequest\n");
                break;
            case UnmapNotify:
                OnUnmapNotify(e.xunmap);
                //printf("UnmapNotify\n");
                break;
            case ButtonPress:
                OnButtonPress(e.xbutton);
                UpdateCursor(e);
                //printf("ButtonPress\n");
                break;
            case ButtonRelease:
                OnButtonRelease(e.xbutton);
                UpdateCursor(e);
                //printf("ButtonRelease\n");
                break;
            case MotionNotify:
                OnMotionNotify(e.xmotion);
                UpdateCursor(e);
                //printf("MotionNotify\n");
                break;
            case KeyPress:
                OnKeyPress(e.xkey);
                //printf("KeyPress\n");
                break;
            // ...
            default:
                //printf("Ignored Event\n");
                break;
        }

        // Pass through Pointer clicks to the client
        XAllowEvents(display_, ReplayPointer, e.xbutton.time);
        XSync(display_, 0);

    }
}

int WindowManager::OnWMDetected(Display* display, XErrorEvent* e) {
    // If another WM is running, the error code from XSelectInput is BadAccess.
    wm_detected_ = true;
    return 0;
}

int WindowManager::OnXError(Display* display, XErrorEvent* e) { /* Print e */return 0; }

// Application configures window to set initial size, position, etc.
// WM receives ConfigureRequest; WM doesn't need to change parameters because window is invisible so it
// calls XConfigureWindow with the same parameters
void WindowManager::OnConfigureRequest(const XConfigureRequestEvent& e) {
    XWindowChanges changes;

    // Copy fields from e to changes
    changes.x = e.x;
    changes.y = e.y;
    changes.width = e.width;
    changes.height = e.height;
    changes.border_width = e.border_width;
    changes.sibling = e.above;
    changes.stack_mode = e.detail;

    if(clients_.count(e.window)) {
        const Frame frame = clients_[e.window];

        // X server knows that this originates from WM and the WM will reveive ConfigureNotify instead of ConfigureRequest,
        // which can be ignored
        XConfigureWindow(display_, frame.frame_win, e.value_mask, &changes);
    }

    // Grant request by calling XConfigureWindow
    XConfigureWindow(display_, e.window, e.value_mask, &changes);
}

// Make the window visible on the screen
// Client calls XMapWindow() and sends MapRequest to the WM
void WindowManager::OnMapRequest(const XMapRequestEvent& e) {
    // 1. Frame or re-frame window
    FrameWindow(e.window, false);

    // Select button press on the client window

    // 2. Actually map the window
    XMapWindow(display_, e.window);
}

void WindowManager::OnUnmapNotify(const XUnmapEvent& e) {
    // If the window is a client the WM manages, unframe it upon UnmapNotify
    // Must check because the WM will receive UnmapNotify for a frame window it destroys
    if(!clients_.count(e.window)) {
        return;
    }

    // Ignore event if it was triggered by reparenting window that was mapped before WM started
    if(e.event == root_){
        return;
    }

    UnFrame(e.window);
}

void WindowManager::FrameWindow(Window w, bool was_created_before_wm) {

    // Retrieve attributes of window to frame
    XWindowAttributes x_window_attrs;
    XGetWindowAttributes(display_, w, &x_window_attrs);

    // Frame existing top-level windows that if they are visible and don't set override_redirect
    if(was_created_before_wm) {
        if(x_window_attrs.override_redirect || x_window_attrs.map_state != IsViewable) {
            return;
        }
    }

    Frame frame;
    frame.Create(display_, root_, w, x_window_attrs);

    // Save frame handle
    clients_[w] = frame;
    frames_[frame.frame_win] = frame;

    // Focus the newly created window
    //TODO: does not work yet
    XSetInputFocus(display_, w, root_, CurrentTime);
}


void WindowManager::UnFrame(Window w) {
    // Reverse steps taken in Frame()
    const Frame frame = clients_[w];

    // Unmap frame
    XUnmapWindow(display_, frame.frame_win);

    // Reparent client window back to root window
    XReparentWindow(display_, w, root_, 0, 0);

    // Remove client window from save set
    XRemoveFromSaveSet(display_, w);

    // Destroy frame
    XDestroyWindow(display_, frame.frame_win);
    XDestroyWindow(display_, frame.close_win);
    XDestroyWindow(display_, frame.max_win);
    XDestroyWindow(display_, frame.min_win);

    // Drop reference to frame handle
    clients_.erase(w);
    frames_.erase(frame.frame_win);

    //TODO: focus on the next client. For now, focus on the root window
    XSetInputFocus(display_, root_, RevertToNone, CurrentTime);

    // If there are no clients left, set the input focus to the root window
    if(clients_.empty())
        XSetInputFocus(display_, root_, RevertToNone, CurrentTime);
    
}

void WindowManager::OnMotionNotify(const XMotionEvent& e) {

    const Position<int> drag_pos(e.x_root, e.y_root);
    const Vector2D<int> delta(drag_pos.x - drag_start_pos.x, drag_pos.y - drag_start_pos.y);

    const Position<int> dest_frame_pos(drag_start_frame_pos.x + delta.x, drag_start_frame_pos.y + delta.y);

    //const Vector2D<int> size_delta(max(delta.x, -drag_start_frame_size.width), max(delta.y, -drag_start_frame_size.height));
    const Size<int> dest_frame_size(drag_start_frame_size.width + delta.x, drag_start_frame_size.height + delta.y);

    // Move/resize the frame that is to be moved/resize if the left button is pressed
    if((e.state & Button1Mask)) {
        //
        // Resize, else move
        if(top || bottom || left || right){
            Window returned_root;
            int original_x, original_y;
            unsigned original_width, original_height, border_width, depth;
            XGetGeometry(display_, frame_being_moved_resized.frame_win, &returned_root, &original_x, &original_y, 
                    &original_width, &original_height, &border_width, &depth);

            if(top && left) {
                frame_being_moved_resized.ResizeFrame(display_, dest_frame_size.width-2*delta.x, dest_frame_size.height-2*delta.y);
                frame_being_moved_resized.MoveFrame(display_, e.x_root, e.y_root);//TODO: Dont just warp to mouse
            } else if(top && right) {
                frame_being_moved_resized.ResizeFrame(display_, dest_frame_size.width, dest_frame_size.height-2*delta.y);
                frame_being_moved_resized.MoveFrame(display_, original_x, e.y_root);//TODO: Dont just warp to mouse
            } else if(bottom && left){
                frame_being_moved_resized.ResizeFrame(display_, dest_frame_size.width-2*delta.x, dest_frame_size.height);
                frame_being_moved_resized.MoveFrame(display_, e.x_root, original_y);
            } else if(bottom && right) {
                //if(width < 1) { width = 1; }
                //if(height < CLIENT_OFFSET_Y+2*BUTTON_PADDING) { height = CLIENT_OFFSET_Y + 2*BUTTON_PADDING; }
                frame_being_moved_resized.ResizeFrame(display_, dest_frame_size.width, dest_frame_size.height);
            } else if(top) {
                frame_being_moved_resized.ResizeFrame(display_, original_width, dest_frame_size.height-2*delta.y);
                frame_being_moved_resized.MoveFrame(display_, original_x, e.y_root);//TODO: Dont just warp to mouse

            } else if(bottom) {
                //if(height < CLIENT_OFFSET_Y+2*BUTTON_PADDING) { height = CLIENT_OFFSET_Y + 2*BUTTON_PADDING; }
                frame_being_moved_resized.ResizeFrame(display_, original_width, dest_frame_size.height);
            } else if(left) {
                frame_being_moved_resized.ResizeFrame(display_, dest_frame_size.width-2*delta.x, original_height);
                frame_being_moved_resized.MoveFrame(display_, e.x_root, original_y);
            } else if(right) {
                frame_being_moved_resized.ResizeFrame(display_, dest_frame_size.width, original_height);
            } else {
                return;
            }
            
        } else {
            frame_being_moved_resized.MoveFrame(display_, dest_frame_pos.x, dest_frame_pos.y);
        }
    }
}

void WindowManager::UpdateCursor(const XEvent& ev){

    const XMotionEvent e = ev.xmotion;

    Frame frame = frames_[e.subwindow];
    
    Window returned_root_frame;
    int x_frame, y_frame;
    unsigned width_frame, height_frame, border_width_frame, depth_frame;
    XGetGeometry(display_, frame.frame_win, &returned_root_frame, &x_frame, &y_frame, &width_frame, &height_frame, &border_width_frame, &depth_frame);

    if(!button_pressed && e.subwindow != None) {
        left = right = top = bottom = false;
        if(e.x < x_frame+EDGE_GRAB_DISTANCE)
            left = true;
        
        if(e.x > x_frame+width_frame-EDGE_GRAB_DISTANCE)
            right = true;
        
        if(e.y < y_frame+EDGE_GRAB_DISTANCE)
            top = true;
        
        if(e.y > y_frame+height_frame-EDGE_GRAB_DISTANCE)
            bottom = true;

        // Set cursor depending on corner hovered
        if(top && left)
            XDefineCursor(display_, root_, top_left_cursor);
        else if(top && right)
            XDefineCursor(display_, root_, top_right_cursor);
        else if(bottom && left)
            XDefineCursor(display_, root_, bottom_left_cursor);
        else if(bottom && right)
            XDefineCursor(display_, root_, bottom_right_cursor);
        else if(bottom)
            XDefineCursor(display_, root_, bottom_cursor);
        else if(top)
            XDefineCursor(display_, root_, top_cursor);
        else if(left)
            XDefineCursor(display_, root_, left_cursor);
        else if(right)
            XDefineCursor(display_, root_, right_cursor);
        else
            XDefineCursor(display_, root_, default_cursor);
    } else if (!button_pressed && e.subwindow == None){
        left = right = top = bottom = false;
        XDefineCursor(display_, root_, default_cursor);
    }
}

bool WindowManager::InsideWindow(Window win){

    Window root, child;
    int root_x, root_y, child_x, child_y;
    unsigned int returned_mask;
    XQueryPointer(display_, win, &root, &child, &root_x, &root_y, &child_x, &child_y, &returned_mask);

    Window returned_root;
    int x, y;
    unsigned width, height, border_width, depth;
    XGetGeometry(display_, win, &returned_root, &x, &y, &width, &height, &border_width, &depth);

    return child_x > 0 && child_x < width && child_y > 0 && child_y < height;

}

void WindowManager::OnButtonPress(const XButtonEvent& e){

    button_pressed = true;

    bool frame_button_pressed;

    // TODO: Right click on root will open a menu
    if(e.subwindow == None) {
        XSetInputFocus(display_, root_, RevertToNone, CurrentTime);
        return;
    }

    // Raise clicked window to the top
    XRaiseWindow(display_, e.subwindow);

    // Get the frame that was clicked
    Frame frame = frames_[e.subwindow];

    // Keep the client window focused
    // Revert to root if no subwindow is clicked, this way key combos still work
    XSetInputFocus(display_, frame.client_win, RevertToParent, CurrentTime);

    // Return if the click was inside the client window
    if(InsideWindow(frame.client_win)){
        printf("Client clicked\n");
        return;
    } 

    if(InsideWindow(frame.close_win)){
        printf("Close win\n");
        frame_being_closed = frame;
        frame_button_pressed = true;
    }

    if(InsideWindow(frame.max_win)){
        printf("Max win\n");
        frame_button_pressed = true;
    }

    if(InsideWindow(frame.min_win)){
        printf("Min win\n");
        frame_button_pressed = true;
    }

    // If the window clicked is a frame, prepare to move or resize it
    if(frames_.count(e.subwindow) & !frame_button_pressed){

        // Handle clicks of frame buttons
        Window returned_root_frame;
        int x_frame, y_frame;
        unsigned width_frame, height_frame, border_width_frame, depth_frame;
        XGetGeometry(display_, frame.frame_win, &returned_root_frame, &x_frame, &y_frame, &width_frame, &height_frame, &border_width_frame, &depth_frame);

        // Save intial cursor position
        drag_start_pos = Position<int>(e.x_root, e.y_root);

        drag_start_frame_pos = Position<int>(x_frame, y_frame);
        drag_start_frame_size = Size<int>(width_frame, height_frame);

        // Set the frame to the frame that is being moved or resized
        frame_being_moved_resized = frame;
    }
}

void WindowManager::OnKeyPress(const XKeyEvent& e){

    // If Alt-R is pressed, run dmenu
    if ((e.state & Mod1Mask) && (e.keycode == XKeysymToKeycode(display_, XK_r))) {
        system("dmenu_run -c -l 30 -bw 3 &");
    }
}

void WindowManager::CloseWindow(Window win_to_close){
    // First try sending close message
    if(!SendMessage(win_to_close, WM_DELETE_WINDOW)) {
        // Otherwise force close
        XKillClient(display_, win_to_close);
    }
}

bool WindowManager::SendMessage(Window win, Atom protocol){
    Atom *supported_protocols;
    int num_supported_protocols;
    if(XGetWMProtocols(display_, win, &supported_protocols, &num_supported_protocols) &&
            (std::find(supported_protocols, supported_protocols + num_supported_protocols, protocol) != supported_protocols + num_supported_protocols)){
        XEvent msg;
        memset(&msg, 0, sizeof(msg));
        msg.xclient.type = ClientMessage;
        msg.xclient.window = win;
        msg.xclient.message_type = WM_PROTOCOLS;
        msg.xclient.format = 32;
        msg.xclient.data.l[0] = protocol;
        msg.xclient.data.l[1] = CurrentTime;
        XSendEvent(display_, win, false, 0, &msg);
        return true;
    }
    return false;
}

void WindowManager::OnButtonRelease(const XButtonEvent& e){
    button_pressed = false;
    frame_being_moved_resized = {}; 

    // Close the frame_being_closed if the pointer is still in the close button on release
    if(InsideWindow(frame_being_closed.close_win)){
        CloseWindow(frame_being_closed.client_win);
        frame_being_closed = {};
    }

}

// Functions that do nothing
void WindowManager::OnCreateNotify(const XCreateWindowEvent& e){}

void WindowManager::OnReparentNotify(const XReparentEvent& e){}

void WindowManager::OnMapNotify(const XMapEvent& e){}

void WindowManager::OnDestroyNotify(const XDestroyWindowEvent& e){}

void WindowManager::OnConfigureNotify(const XConfigureEvent& e){}

