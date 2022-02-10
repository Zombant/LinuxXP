#ifndef BAR_HPP
#define BAR_HPP

extern "C" {
#include <X11/Xlib.h>
}
#include <memory>
#include <unordered_map>
#include "util.hpp"

#define BAR_HEIGHT 31
#define BAR_COLOR 0x0000ff
#define BAR_BORDER_COLOR 0x0000ee
#define BAR_BORDER_WIDTH 0

class Bar {
    public:
        void Create(Display *display, Window root);


        Window bar_win;

        Window start_button;

        Pixmap start_pix, start_pix_press, start_pix_hover;

    private:


};


#endif
