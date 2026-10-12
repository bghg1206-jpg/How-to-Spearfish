# MemoryReader (Linux)

Small **read-only** terminal tool with a simple menu to practice `Base + Offset`
pointer chains. It reads `/proc/<pid>/maps` and `/proc/<pid>/mem` and never writes
to another process.

## Build
    make            # builds ./memreader and ./practice_target

## Try it on the practice target first
    ./practice_target &        # prints the offsets for its "Health" value
    ./memreader
    # 1) process: practice_target   2) module: practice_target
    # 4) offsets: (paste the numbers practice_target printed)   5) read once

Health counts down once per second, so you can see your chain is correct.

## Menu
1. select process (name, e.g. `cs2`)
2. select module (e.g. `libclient.so`; basename of a file in `/proc/<pid>/maps`)
3. set type: int / uint / int64 / float / double / ptr
4. set offsets (hex): all but the last are followed as pointers, the last is read
5. read once
6. live view (refreshes every 500 ms)
7. offset labels on/off

## Live view and the Insert menu
In the live view press **Insert** to open/close a menu:
- `1` toggles **offset labels**
- `q` stops the live view (also works with the menu closed, or Ctrl+C)

Labels ON marks every offset with its role:

    base    libfoo.so = 0x55659799C000
    ptr 1   [libfoo.so + 0x4028] -> 0x5565AFD902B0
    value   [ptr + 0x4] (int) at 0x5565AFD902B4 = 97

Labels OFF prints only the value. If your keyboard has no Insert key, use menu item 7.

## Permissions
Reading another process may need `sudo`, or `sudo sysctl kernel.yama.ptrace_scope=0`.

## Safety
Use only on your own programs or offline games without anti-cheat. For CS2: private
server with bots and the `-insecure` launch option. Never next to a VAC-secured game.
