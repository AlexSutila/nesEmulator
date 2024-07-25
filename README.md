# DorcelessNESs - NES Emulator
A hobbyist, cross-platform, NES emulator written entirely in C++ using SDL2 for rendering. Future plans consist of improvements on the MMC1 support, potentially adding more mappers, USB controller support, and audio support (there is currently no audio at all).

![Untitled](https://user-images.githubusercontent.com/96510931/236361499-fad9ff59-ab35-4a69-abba-82153c960d53.png)

## Compatability
The games shown in the screenshot above are known to play quite well, however the compatability for games is not limited to the games shown in the screenshot alone.

As mentioned above, this is a hobbyist emulator and is by no means perfect. That being said even if a ROM for a specific game was originally implemented with a well tested mapper there is still a chance it might not work as expected. The table below shows the following mapper types supported as of now, there is still a chance that more will come:

| Mapper Type | State |
| --- | --- |
| Mapper 0 | Implemented, works well |
| Mapper 1 | Implemented, needs work |
| Mapper 2 | Implemented, works well |

## Compiling 
This emulator uses SDL2. The development package for SDL2 can be installed using the following if you're using apt:
```
sudo apt-get install libsdl2-dev
```

The makefile included can compile everything either with or without debug mode. The debugger I wrote for this is by no means user friendly.

Without debugging stuff:
```
make # Suggested as the debugger is yucky
```

With debugging stuff:
```
make debug
```

## Running
The final binary just takes a path to the ROM as it's only argument.
```
./nes ~/Documents/Path/To/Rom.nes
```

## Controls
There is currently only support for a regular keyboard, but talk about potential support for USB controller support. The keybinds are only configurable through the source code and are mapped as follows by default:
| Keyboard Button | Nes Controller Button |
| --- | --- |
| W | up |
| A | left |
| S | down |
| D | right |
| start | escape |
| select | backspace |
| L | A |
| J | B |

## Cheating
Game genie codes (both 6-character and 8-character) are supported, and multiple can be provided via commandline arguments. As an example, the link below shows cheat codes for mega man all of which can be provided at once:
- https://www.gamegenie.com/cheats/gamegenie/nes/mega_man.html

Running `./nes ~/Documents/Roms/nes/mm.nes OZSKPZVK SZKZGZSA AVVXLPSZ` will boot mega man with the following cheats:
- Infinite lives
- Infinite energy
- No harm from any enemies, except super villains

Same pattern follows for other ROMs supported, game genie codes for any game are pretty easy to find online.

There is also talk about potentially adding support for a 'turbo' like feature, similar to what Mesen has.
