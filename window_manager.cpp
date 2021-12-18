#include "window_manager.hpp"
#include "frame.hpp"
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
    XSelectInput(display_, root_, SubstructureRedirectMask | SubstructureNotifyMask);
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
                //printf("Ignored Event\n");
                break;
        }


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
    printf("Resize\n");
}

// Make the window visible on the screen
// Client calls XMapWindow() and sends MapRequest to the WM
void WindowManager::OnMapRequest(const XMapRequestEvent& e) {
    // 1. Frame or re-frame window
    FrameWindow(e.window, false);

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

    // 8. Grab events for window management actions on client window
    //  a. Move windows with Alt + Left button
    XGrabButton(display_, Button1, Mod1Mask, w, false, ButtonPressMask | ButtonReleaseMask | ButtonMotionMask, GrabModeAsync, GrabModeAsync, None, None);
    //  b. Resize windows with Alt + Right button
    XGrabButton(display_, Button3, Mod1Mask, w, false, ButtonPressMask | ButtonReleaseMask | ButtonMotionMask, GrabModeAsync, GrabModeAsync, None, None);
    //  c. Kill windows with Alt + F4
    XGrabKey(display_, XKeysymToKeycode(display_, XK_F4), Mod1Mask, w, false, GrabModeAsync, GrabModeAsync); 
    //  d. Switch windows with Alt + TAB
    XGrabKey(display_, XKeysymToKeycode(display_, XK_Tab), Mod1Mask, w, false, GrabModeAsync, GrabModeAsync); 

    printf("Framed window\n");
}

void WindowManager::OnMotionNotify(const XMotionEvent& e) {
    Frame frame = clients_[e.window];
    const Position<int> drag_pos(e.x_root, e.y_root);
    const Vector2D<int> delta(drag_pos.x - drag_start_pos.x, drag_pos.y - drag_start_pos.y);

    if(e.state & Button1Mask) {
        // Alt + Left button: moves a window
        const Position<int> dest_frame_pos(drag_start_frame_pos.x + delta.x, drag_start_frame_pos.y + delta.y);
        //XMoveWindow(display_, frame.frame_win, dest_frame_pos.x, dest_frame_pos.y);
        frame.MoveFrame(display_, dest_frame_pos.x, dest_frame_pos.y);
    } else if(e.state * Button3Mask) {
        // Alt + Right button: resize window
        const Vector2D<int> size_delta(max(delta.x, -drag_start_frame_size.width), max(delta.y, -drag_start_frame_size.height));
        const Size<int> dest_frame_size(drag_start_frame_size.width+ size_delta.x, + drag_start_frame_size.height + size_delta.y);

        // Resize frame
        frame.ResizeFrame(display_, dest_frame_size.width, dest_frame_size.height);
    }
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

    // 5. Drop reference to frame handle
    clients_.erase(w);

    printf("Unframed window\n");
}

void WindowManager::OnButtonPress(const XButtonEvent& e){
    
    Frame frame = clients_[e.window];

    frame.dragged = true;

    // 1. Save intial cursor position
    drag_start_pos = Position<int>(e.x_root, e.y_root);

    // 2. Save initial window info
    Window returned_root;
    int x, y;
    unsigned width, height, border_width, depth;
    XGetGeometry(display_, frame.frame_win, &returned_root, &x, &y, &width, &height, &border_width, &depth);
    drag_start_frame_pos = Position<int>(x, y);
    drag_start_frame_size = Size<int>(width, height);


    // 3. Raise clicked window to the top
    XRaiseWindow(display_, frame.frame_win);


    
}

void WindowManager::OnButtonRelease(const XButtonEvent& e){
    Frame frame = clients_[e.window];
    frame.dragged = false;
}

// Functions that do nothing
void WindowManager::OnCreateNotify(const XCreateWindowEvent& e){}

void WindowManager::OnReparentNotify(const XReparentEvent& e){}

void WindowManager::OnMapNotify(const XMapEvent& e){}

void WindowManager::OnDestroyNotify(const XDestroyWindowEvent& e){}

void WindowManager::OnConfigureNotify(const XConfigureEvent& e){}

