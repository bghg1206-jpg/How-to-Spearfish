# MemoryReader

Small **read-only** Windows tool to practice `Base + Offset` pointer chains.
It never writes to another process.

## Build (Windows)
- MSVC: open "x64 Native Tools Command Prompt", `cd Tools\MemoryReader`, run `build.bat`
- or MinGW: `g++ -std=c++17 -O2 main.cpp -o memreader.exe`
- or CMake: `cmake -B build && cmake --build build --config Release`

## Usage
    memreader <process.exe> <module.dll> <type> <offset> [offset ...] [--watch <ms>]

All offsets except the last are followed as pointers; the last is added and read.

    memreader cs2.exe client.dll int 0xAAAA 0xBBBB --watch 500
    (placeholders: first read *(base+0xAAAA), then read int at result+0xBBBB)

Find the real offsets yourself (Cheat Engine pointer scan, or a community offset dumper).

## Safety
Use only on your own programs or offline games without anti-cheat. For CS2:
private server with bots and the `-insecure` launch option. Never next to a
VAC-secured game.
