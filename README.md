# Linux XP
__Work in progress__

X11 window manager written in C++ with the Xlib C libraries, made to resemble an OS with a similar name.

---

## Build
Make sure the Xlib headers are installed
```bash
git clone https://github.com/Zombant/LinuxXP
cd LinuxXP
make build
```

## Run
1. Add `exec /path/to/window_manager.o` to `~/.xinitrc`
2. Start X server with `startx`

## Usage
- Standard floating window movement controls (drag from titlebar, grab edges to resize)
- Alt-R to run dmenu (will be replaced)

---

##### Projects and Resources that helped me understand how window managers work:
- [dwm](https://dwm.suckless.org/ "dwm")
- [basicwm](https://github.com/jichu4n/basic_wm "basicwm")
- [C Xlib manual](https://www.x.org/releases/current/doc/libX11/libX11/libX11.html)
	- [HTML version](https://tronche.com/gui/x/xlib/)
- [tinywm](http://incise.org/tinywm.html)
- [Inter-client Communication Conventions manual](https://x.org/releases/X11R7.6/doc/xorg-docs/specs/ICCCM/icccm.html)
	- [HTML version](https://tronche.com/gui/x/icccm/)
- [Extended Window Manager Hints (EWMH) specifications](https://specifications.freedesktop.org/wm-spec/wm-spec-latest.html)
