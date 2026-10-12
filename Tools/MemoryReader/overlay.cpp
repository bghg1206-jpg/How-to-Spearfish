#include "overlay.h"

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/shape.h>
#include <X11/keysym.h>
#include <signal.h>
#include <sys/select.h>

#include <algorithm>
#include <cstdio>
#include <cstring>

namespace
{
	volatile sig_atomic_t gStop = 0;
	void OnSigInt(int) { gStop = 1; }

	bool gGrabFailed = false;
	int OnXError(Display*, XErrorEvent* Event)
	{
		if (Event->error_code == BadAccess) gGrabFailed = true;
		return 0;
	}

	// Premultiplied ARGB, as expected by compositors for 32-bit windows.
	unsigned long Argb(unsigned A, unsigned R, unsigned G, unsigned B)
	{
		return (static_cast<unsigned long>(A) << 24) | (static_cast<unsigned long>(R * A / 255) << 16) |
		       (static_cast<unsigned long>(G * A / 255) << 8) | static_cast<unsigned long>(B * A / 255);
	}

	void Grab(Display* Dpy, Window Root, KeySym Sym, bool On)
	{
		const KeyCode Code = XKeysymToKeycode(Dpy, Sym);
		if (On) XGrabKey(Dpy, Code, AnyModifier, Root, False, GrabModeAsync, GrabModeAsync);
		else XUngrabKey(Dpy, Code, AnyModifier, Root);
	}
}

bool RunOverlay(const std::function<std::vector<std::string>()>& Produce, bool& Labels, std::string& Error)
{
	Display* Dpy = XOpenDisplay(nullptr);
	if (!Dpy)
	{
		Error = "cannot open X display (DISPLAY not set? Wayland-only sessions need XWayland)";
		return false;
	}
	const int Screen = DefaultScreen(Dpy);
	const Window Root = RootWindow(Dpy, Screen);

	XVisualInfo Vi;
	if (!XMatchVisualInfo(Dpy, Screen, 32, TrueColor, &Vi))
	{
		Error = "no 32-bit ARGB visual available (transparency not supported)";
		XCloseDisplay(Dpy);
		return false;
	}

	XFontStruct* Font = XLoadQueryFont(Dpy, "-misc-fixed-bold-r-normal--15-*-*-*-c-*-iso8859-1");
	if (!Font) Font = XLoadQueryFont(Dpy, "fixed");
	if (!Font)
	{
		Error = "no X font found";
		XCloseDisplay(Dpy);
		return false;
	}

	XSetWindowAttributes Attr;
	std::memset(&Attr, 0, sizeof(Attr));
	Attr.colormap = XCreateColormap(Dpy, Root, Vi.visual, AllocNone);
	Attr.border_pixel = 0;
	Attr.background_pixel = 0;
	Attr.override_redirect = True; // no window-manager frame, stays above normal windows
	const Window Win = XCreateWindow(Dpy, Root, 20, 20, 320, 60, 0, 32, InputOutput, Vi.visual,
	                                 CWColormap | CWBorderPixel | CWBackPixel | CWOverrideRedirect, &Attr);
	XShapeCombineRectangles(Dpy, Win, ShapeInput, 0, 0, nullptr, 0, ShapeSet, YXBanded); // empty input region: clicks pass through
	GC Gc = XCreateGC(Dpy, Win, 0, nullptr);
	XSetFont(Dpy, Gc, Font->fid);
	XMapWindow(Dpy, Win);

	// Insert is a global hotkey (the game will not see it while the overlay runs).
	gGrabFailed = false;
	XErrorHandler OldHandler = XSetErrorHandler(OnXError);
	Grab(Dpy, Root, XK_Insert, true);
	XSync(Dpy, False);

	bool Ok = !gGrabFailed;
	if (!Ok) Error = "could not grab the Insert key (another program already uses it)";

	struct sigaction Sa, OldSa;
	std::memset(&Sa, 0, sizeof(Sa));
	Sa.sa_handler = OnSigInt; // no SA_RESTART: select() returns on Ctrl+C
	sigaction(SIGINT, &Sa, &OldSa);
	gStop = Ok ? 0 : 1;

	bool MenuOpen = false;
	int CurW = 0, CurH = 0;
	const int LineH = Font->ascent + Font->descent + 3;

	while (!gStop)
	{
		std::vector<std::string> Lines;
		Lines.push_back("memreader overlay   [Insert] menu");
		for (const std::string& L : Produce()) Lines.push_back(L);
		if (MenuOpen)
		{
			Lines.push_back("");
			Lines.push_back("== MENU (Insert to close) ==");
			Lines.push_back(std::string("1) Offset labels: ") + (Labels ? "ON" : "OFF"));
			Lines.push_back("2) close overlay");
		}

		int W = 0;
		for (const std::string& L : Lines) W = std::max(W, XTextWidth(Font, L.c_str(), static_cast<int>(L.size())));
		W += 20;
		const int H = static_cast<int>(Lines.size()) * LineH + 14;
		if (W != CurW || H != CurH)
		{
			XResizeWindow(Dpy, Win, static_cast<unsigned>(W), static_cast<unsigned>(H));
			CurW = W;
			CurH = H;
		}
		XSetForeground(Dpy, Gc, Argb(190, 10, 10, 16));
		XFillRectangle(Dpy, Win, Gc, 0, 0, static_cast<unsigned>(W), static_cast<unsigned>(H));
		for (size_t I = 0; I < Lines.size(); ++I)
		{
			const bool Header = I == 0 || Lines[I].rfind("==", 0) == 0;
			XSetForeground(Dpy, Gc, Header ? Argb(255, 255, 220, 90) : Argb(255, 120, 255, 160));
			XDrawString(Dpy, Win, Gc, 10, 7 + Font->ascent + static_cast<int>(I) * LineH, Lines[I].c_str(), static_cast<int>(Lines[I].size()));
		}
		XRaiseWindow(Dpy, Win);
		XFlush(Dpy);

		// Wait up to 500 ms for a key press, then refresh the values.
		fd_set Fds;
		FD_ZERO(&Fds);
		FD_SET(ConnectionNumber(Dpy), &Fds);
		timeval Timeout{0, 500000};
		if (XPending(Dpy) == 0) select(ConnectionNumber(Dpy) + 1, &Fds, nullptr, nullptr, &Timeout);

		while (XPending(Dpy) > 0)
		{
			XEvent Event;
			XNextEvent(Dpy, &Event);
			if (Event.type != KeyPress) continue;
			const KeySym Sym = XLookupKeysym(&Event.xkey, 0);
			if (Sym == XK_Insert)
			{
				MenuOpen = !MenuOpen;
				Grab(Dpy, Root, XK_1, MenuOpen); // menu keys are only grabbed while the menu is open
				Grab(Dpy, Root, XK_2, MenuOpen);
			}
			else if (MenuOpen && Sym == XK_1) Labels = !Labels;
			else if (MenuOpen && Sym == XK_2) gStop = 1;
		}
	}

	sigaction(SIGINT, &OldSa, nullptr);
	Grab(Dpy, Root, XK_Insert, false);
	Grab(Dpy, Root, XK_1, false);
	Grab(Dpy, Root, XK_2, false);
	XSync(Dpy, False);
	XSetErrorHandler(OldHandler);
	XFreeGC(Dpy, Gc);
	XFreeFont(Dpy, Font);
	XDestroyWindow(Dpy, Win);
	XCloseDisplay(Dpy);
	gStop = 0;
	return Ok;
}
