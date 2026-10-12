// X11 overlay: a transparent, always-on-top, click-through text panel in the
// top-left screen corner. It shows the lines produced by the callback (the
// memreader chain output) and has an Insert-key menu. It draws only that text
// panel; it does not draw anything at in-game positions.
#pragma once

#include <functional>
#include <string>
#include <vector>

// Blocks until the user closes the overlay (menu item 2) or presses Ctrl+C in the
// terminal. `Labels` is toggled from the overlay menu. Returns false with `Error` set
// if the overlay cannot start (no X display, no ARGB visual, Insert already grabbed).
bool RunOverlay(const std::function<std::vector<std::string>()>& Produce, bool& Labels, std::string& Error);
