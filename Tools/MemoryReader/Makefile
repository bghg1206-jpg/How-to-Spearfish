CXX ?= g++
CXXFLAGS ?= -std=c++17 -O2 -Wall -Wextra

# The overlay needs X11 (libx11-dev + libxext-dev). Without them memreader still builds, just without overlay.
X11_LIBS := $(shell pkg-config --libs x11 xext 2>/dev/null)
ifneq ($(X11_LIBS),)
OVERLAY_FLAGS := -DWITH_OVERLAY $(shell pkg-config --cflags x11 xext)
OVERLAY_SRC := overlay.cpp
endif

all: memreader practice_target

memreader: main.cpp overlay.cpp overlay.h
	$(CXX) $(CXXFLAGS) $(OVERLAY_FLAGS) main.cpp $(OVERLAY_SRC) -o memreader $(X11_LIBS)

practice_target: practice_target.cpp
	$(CXX) $(CXXFLAGS) practice_target.cpp -o practice_target

clean:
	rm -f memreader practice_target
