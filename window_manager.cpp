#include "window_manager.hpp"
#include "frame.hpp"
#include <X11/Xlib.h>
extern "C" {
#include <X11/Xutil.h>
}
#include "util.hpp"
#include <cstdio>
#include <iostream>

using ::std::unique_ptr;
using namespace std;

bool WindowManager::wm_detected_;

unique_ptr<WindowManager> WindowManager::Create() {
    // 1. Open X Display
    Display* display = XOpenDisplay(nullptr);
    if(display == nullptr){
        fprintf(stderr, "Failed to open X display\n");
        return nullptr;
    }

    // 2. Construct WindowManager instance
    return unique_ptr<WindowManager>(new WindowManager(display));
}

WindowManager::WindowManager(Display* display) : display_(display), root_(DefaultRootWindow(display_)) {}

WindowManager::~WindowManager() {
    XCloseDisplay(display_);
}

void WindowManager::Start(){
    Setup();
    Run();
}

void WindowManager::Setup() {
    // 1. Initialization
    //  a. Selects events on root window. Error handler to exist gracefully if another WM is running
    wm_detected_ = false;
    XSetErrorHandler(&WindowManager::OnWMDetected);

    // Select events on the root
    XSelectInput(display_, root_, SubstructureRedirectMask | SubstructureNotifyMask | ButtonPressMask | ButtonReleaseMask | ButtonMotionMask);

    // Syncronously grab the the left button on the root
    XGrabButton(display_, Button1, AnyModifier, root_, false, Button1Mask, GrabModeSync, GrabModeAsync, None, None);


    XSync(display_, false);
    if(wm_detected_){
        fprintf(stderr, "Another window manager is running\n");
        exit(1);
    }

    //  b. Set error handler
    XSetErrorHandler(&WindowManager::OnXError);

    //  c. Grab X server to prevent windows from changes while framing them
    XGrabServer(display_);

    //  d. Frame existing top-level windows
    //   i. Query existing top-level windows
    Window returned_root, returned_parent;
    Window* top_level_windows;
    unsigned int num_top_level_windows;
    XQueryTree(display_, root_, &returned_root, &returned_parent, &top_level_windows, &num_top_level_windows);

    //   ii. Frame each top-level window
    for(unsigned int i = 0; i < num_top_level_windows; ++i){
        FrameWindow(top_level_windows[i], true);
    }

    //   ii. Free top-level window array
    XFree(top_level_windows);

    //  e. Ungrab X server
    XUngrabServer(display_);
}

void WindowManager::Run() {

    // 2. Main event loop
    for (;;) {
        // 1. Get next event
        XEvent e;
        XNextEvent(display_, &e);

        // 2. Dispatch event
        switch (e.type) {
            case ReparentNotify:
                OnReparentNotify(e.xreparent);
                break;
            case MapRequest:
                OnMapRequest(e.xmaprequest);
                printf("MapRequest\n");
                break;
            case ConfigureRequest:
                OnConfigureRequest(e.xconfigurerequest);
                printf("ConfigureRequest\n");
                break;
            case UnmapNotify:
                OnUnmapNotify(e.xunmap);
                printf("UnmapNotify\n");
                break;
            case ButtonPress:
                OnButtonPress(e.xbutton);
                printf("ButtonPress\n");
                break;
            case ButtonRelease:
                OnButtonRelease(e.xbutton);
                printf("ButtonRelease\n");
                break;
            case MotionNotify:
                OnMotionNotify(e.xmotion);
                printf("MotionNotify\n");
                break;
            // ...
            default:
                printf("Ignored Event\n");
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

    // 1. Retrieve attributes of window to frame
    XWindowAttributes x_window_attrs;
    XGetWindowAttributes(display_, w, &x_window_attrs);

    // 2. Frame existing top-level windows that if they are visible and don't set override_redirect
    if(was_created_before_wm) {
        if(x_window_attrs.override_redirect || x_window_attrs.map_state != IsViewable) {
            return;
        }
    }

    Frame frame;
    frame.Create(display_, root_, w, x_window_attrs);

    // 7. Save frame handle
    clients_[w] = frame;
    frames_[frame.frame_win] = frame;

    printf("Framed window\n");
}


void WindowManager::UnFrame(Window w) {
    // Reverse steps taken in Frame()
    const Frame frame = clients_[w];

    // 1. Unmap frame
    XUnmapWindow(display_, frame.frame_win);

    // 2. Reparent client window back to root window
    XReparentWindow(display_, w, root_, 0, 0);

    // 3. Remove client window from save set
    XRemoveFromSaveSet(display_, w);

    // 4. Destroy frame
    XDestroyWindow(display_, frame.frame_win);
    XDestroyWindow(display_, frame.close_win);
    XDestroyWindow(display_, frame.max_win);
    XDestroyWindow(display_, frame.min_win);

    // 5. Drop reference to frame handle
    clients_.erase(w);
    frames_.erase(frame.frame_win);

    printf("Unframed window\n");
}

void WindowManager::OnMotionNotify(const XMotionEvent& e) {

    // Get the window of the frame that is to be moved
    Window frame_win_to_move = frame_being_moved.frame_win;

    const Position<int> drag_pos(e.x_root, e.y_root);
    const Vector2D<int> delta(drag_pos.x - drag_start_pos.x, drag_pos.y - drag_start_pos.y);

    // Move the frame that is to be moved if the left button is pressed
    if((e.state & Button1Mask)) {
        const Position<int> dest_frame_pos(drag_start_frame_pos.x + delta.x, drag_start_frame_pos.y + delta.y);
        frame_being_moved.MoveFrame(display_, dest_frame_pos.x, dest_frame_pos.y);
    } 

}
void WindowManager::OnButtonPress(const XButtonEvent& e){

    bool frame_button_pressed;

    // Don't handle button presses on root window
    // TODO: Right click on root will open a menu
    if(e.subwindow == None) {
        return;
    }
    
    // Raise clicked window to the top
    XRaiseWindow(display_, e.subwindow);

    // Get the frame that was clicked
    Frame frame = frames_[e.subwindow];

    // Handle clicks of frame buttons
    Window returned_root_frame;
    int x_frame, y_frame;
    unsigned width_frame, height_frame, border_width_frame, depth_frame;
    XGetGeometry(display_, frame.frame_win, &returned_root_frame, &x_frame, &y_frame, &width_frame, &height_frame, &border_width_frame, &depth_frame);

    Window returned_root;
    int x, y;
    unsigned width, height, border_width, depth;
    XGetGeometry(display_, frame.close_win, &returned_root, &x, &y, &width, &height, &border_width, &depth);
    //printf("Click position: X= %d, Y= %d\n", e.x-x_frame, e.y-y_frame);
    //printf("Close position: X= %d, Y= %d\n", x, y);
    //printf("Close width: Width= %d, Height=%d\n", width, height);
    if(e.x-x_frame > x && e.x-x_frame < x+width && e.y-y_frame > y-y_frame && e.y-y_frame < y+height){
        printf("Close win\n");
        frame_button_pressed = true;
    }

    XGetGeometry(display_, frame.max_win, &returned_root, &x, &y, &width, &height, &border_width, &depth);
    if(e.x-x_frame > x && e.x-x_frame < x+width && e.y-y_frame > y-y_frame && e.y-y_frame < y+height){
        printf("Max win\n");
        frame_button_pressed = true;
    }

    XGetGeometry(display_, frame.min_win, &returned_root, &x, &y, &width, &height, &border_width, &depth);
    if(e.x-x_frame > x && e.x-x_frame < x+width && e.y-y_frame > y-y_frame && e.y-y_frame < y+height){
        printf("Min win\n");
        frame_button_pressed = true;
    }



    // If the window clicked is a frame, prepare to move it
    if(frames_.count(e.subwindow) & !frame_button_pressed){

        // Set the frame to the frame that is being moved
        frame_being_moved = frame;

        // 1. Save intial cursor position
        drag_start_pos = Position<int>(e.x_root, e.y_root);
        //printf("Button Pressed: X: %d, Y: %d\n", e.x_root, e.y_root);
        // 2. Save initial window info
        Window returned_root;
        int x, y;
        unsigned width, height, border_width, depth;
        XGetGeometry(display_, frame.frame_win, &returned_root, &x, &y, &width, &height, &border_width, &depth);
        //printf("GetGeometry: X: %d, Y: %d\n", e.x_root, e.y_root);
        drag_start_frame_pos = Position<int>(x, y);
        drag_start_frame_size = Size<int>(width, height);

    }

    
}

void WindowManager::OnButtonRelease(const XButtonEvent& e){
   frame_being_moved = {}; 
}

// Functions that do nothing
void WindowManager::OnCreateNotify(const XCreateWindowEvent& e){}

void WindowManager::OnReparentNotify(const XReparentEvent& e){}

void WindowManager::OnMapNotify(const XMapEvent& e){}

void WindowManager::OnDestroyNotify(const XDestroyWindowEvent& e){}

void WindowManager::OnConfigureNotify(const XConfigureEvent& e){}

