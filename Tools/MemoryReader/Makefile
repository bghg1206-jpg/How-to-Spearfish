CXX ?= g++
CXXFLAGS ?= -std=c++17 -O2 -Wall -Wextra

all: memreader practice_target

memreader: main.cpp
	$(CXX) $(CXXFLAGS) main.cpp -o memreader

practice_target: practice_target.cpp
	$(CXX) $(CXXFLAGS) practice_target.cpp -o practice_target

clean:
	rm -f memreader practice_target
