#ifndef WINDOWMANAGER_HPP
#define WINDOWMANAGER_HPP

extern "C" {
#include <X11/Xlib.h>
}
#include <memory>
#include <unordered_map>
#include "util.hpp"
#include "frame.hpp"
#include "bar.hpp"

#define XC_top_left_corner 134
#define XC_top_right_corner 136
#define XC_bottom_left_corner 12
#define XC_bottom_right_corner 14
#define XC_bottom_side 16
#define XC_left_side 70
#define XC_top_side 138
#define XC_right_side 96
#define XC_left_ptr 68

#define EDGE_GRAB_DISTANCE FRAME_BORDER_WIDTH+2

class WindowManager {
    public:
        // Establish connection to X server and create WindowManager instance
        static ::std::unique_ptr<WindowManager> Create();

        // Disconnects from X Server
        ~WindowManager();

        // Start WM
        void Start();

        Bar bar;

    private:

        // Main event loop
        void Run();

        // Setup
        void Setup();

        // Invoked internally by Create()
        WindowManager(Display* display);

        // Handle to underlying Xlib Display struct
        Display* display_;

        // Handle to root window
        const Window root_;

        // Maps top-level windows to their Frames
        ::std::unordered_map<Window, Frame> clients_;

        // Maps frame_win's to their Frame objects
        ::std::unordered_map<Window, Frame> frames_;

        // Xlib error handler. Must be static because its address is passed to Xlib
        static int OnXError(Display* display, XErrorEvent* e);

        // Xlib error handler to determine if another window manager is already running
        static int OnWMDetected(Display* display, XErrorEvent* e);

        // Whether another WM is already running
        static bool wm_detected_;

        // Cursor position at the start of window move/resize
        Position<int> drag_start_pos;
        
        // Position of affected window at start of move/resize
        Position<int> drag_start_frame_pos;

        // Size of affected window at start of window move/resize
        Size<int> drag_start_frame_size;

        // Frame that is being moved or resized
        Frame frame_being_moved_resized;

        // Frame that is about to be closed
        Frame frame_being_closed;

        // Button being pressed
        bool button_pressed;

        // Which corners of the frame were grabbed
        bool top, bottom, left, right;

        // Event handlers

        // When an X client application creates a top-level window, the WM receives CreateNotify event
        // However, newly created window is invisible and there is nothing that the WM needs to do
        void OnCreateNotify(const XCreateWindowEvent& e);
        void OnDestroyNotify(const XDestroyWindowEvent& e);
        void OnReparentNotify(const XReparentEvent& e);
        void OnMapNotify(const XMapEvent& e);
        void OnUnmapNotify(const XUnmapEvent& e);
        void OnConfigureNotify(const XConfigureEvent& e);
        void OnMapRequest(const XMapRequestEvent& e);
        void OnConfigureRequest(const XConfigureRequestEvent& e);
        void OnButtonPress(const XButtonEvent& e);
        void OnButtonRelease(const XButtonEvent& e);
        void OnMotionNotify(const XMotionEvent& e);
        void OnKeyPress(const XKeyEvent& e);
        void OnKeyRelease(const XKeyEvent& e);

        // Frames a top-level window
        void FrameWindow(Window w, bool was_created_before_wm);

        // Unframes a top-level window
        void UnFrame(Window w);

        // Closes a window(client)
        void CloseWindow(Window win_to_close);

        // Determine if the cursor is inside a window
        bool InsideWindow(Window win);

        // Update the cursor icon
        void UpdateCursor(const XEvent& ev);

        // Sends a message to a window
        // Returns true if it is successful
        bool SendMessage(Window win, Atom protocol);

        // Atoms
        const Atom WM_PROTOCOLS;
        const Atom WM_DELETE_WINDOW;

        // Cursors
        Cursor default_cursor;
        Cursor top_left_cursor;
        Cursor top_right_cursor;
        Cursor bottom_left_cursor;
        Cursor bottom_right_cursor;
        Cursor top_cursor;
        Cursor bottom_cursor;
        Cursor left_cursor;
        Cursor right_cursor;

};

#endif
