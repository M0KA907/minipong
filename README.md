# minipong

Isometric pong for Game Boy Advance. Ball has real height (gravity,
floor bounce, shadow). First to 5 vs CPU.

## Build

Requires devkitARM + libtonc (devkitPro), optionally mGBA.

    make            # builds minipong.gba
    make test       # host-compiled unit tests (gcc)
    make run        # launch in mGBA
    make clean

## Controls

- D-pad Up/Down: move paddle along the court depth axis
- Start: start game / pause / restart after win
