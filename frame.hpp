#ifndef FRAME_HPP
#define FRAME_HPP

extern "C" {
#include <X11/Xlib.h>
}
#include <memory>
#include <unordered_map>
#include "util.hpp"

#define FRAME_BORDER_WIDTH 3
#define FRAME_BORDER_COLOR 0x0000aa
#define FRAME_BG_COLOR 0x0000ff

#define BUTTON_BORDER_WIDTH 1
#define BUTTON_BORDER_COLOR 0xffffff
#define BUTTON_BG_COLOR_B 0x0000ff
#define BUTTON_BG_COLOR_R 0xff0000
#define DISTANCE_BETWEEN_BUTTONS 2

#define CLIENT_OFFSET_X 0
#define CLIENT_OFFSET_Y 32

#define BUTTON_SIZE 30


class Frame {
    public:


        void Create(Display *display, Window root, Window win_to_frame, XWindowAttributes attrs);

        void MoveFrame(Display *display, int x, int y);

        void ResizeFrame(Display *display, int width, int height);

        ~Frame();

        // Master window of the frame
        Window frame_win;

        // Client window
        Window client_win;

        // Button windows
        Window min_win, max_win, close_win;

    private:

        void UpdateButtonLocations(Display *display);
        void UpdateClientLocation(Display *display);




};

#endif
