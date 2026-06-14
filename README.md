# TLMBoy <img alt="TLMBoy icon" src="./assets/tlmboy_icon.svg" width="4%"><br>

![example workflow](https://github.com/not-chciken/TLMBoy/actions/workflows/build.yml/badge.svg)
[![Codacy Badge](https://app.codacy.com/project/badge/Coverage/4791a60cefd140328652ee67756c69b9)](https://www.codacy.com/gh/not-chciken/TLMBoy/dashboard?utm_source=github.com\&utm_medium=referral\&utm_content=not-chciken/TLMBoy\&utm_campaign=Badge_Coverage)
[![Codacy Badge](https://app.codacy.com/project/badge/Grade/4791a60cefd140328652ee67756c69b9)](https://www.codacy.com/gh/not-chciken/TLMBoy/dashboard?utm_source=github.com\&utm_medium=referral\&utm_content=not-chciken/TLMBoy\&utm_campaign=Badge_Grade)

A Game Boy Simulator written in C++/SystemC TLM-2.0.

## Building

Use the following commands to build the TLMBoy:

```bash
cd TLMBoy
mkdir build
cd build
cmake tlmboy ..
cmake --build . --target tlmboy --config Release
```

Dependencies:

* [SystemC 2.3.3](https://github.com/accellera-official/systemc)
* [SDL2](https://github.com/libsdl-org/SDL)
* For tests: [googletest](https://github.com/google/googletest)

## Usage

After building TLMBoy, emulating a game is as simple as:

```bash
cd build
./tlmboy -r <PATH_TO_GAME_ROM>
```

Currently, the homebrew game [FlappyBoy](https://github.com/bitnenfer/flappy-boy-asm.git) is included as an example.
The example can be launched with:

```bash
cd build
./tlmboy -r ../roms/flappyboy.gb
```

## Command Line Arguments

* `--boot-rom-path=X`: Specifies the path `X` of the boot ROM. Uses the standard DMG boot if no argument is provided.
* `--color-palette=X`: Color palette hex string with four RGB colors from bright to dark. Default: f2ffd9aaaaaa555555000000.
* `--fps-cap=X`: Limits the maximum frames per second to `X`. Defaults to the Game Boy's default frame rate of 60 fps. Use -1 for no limit.
* `--headless`: Run the TLMBoy without any graphical output. This is useful for CI environments.
* `--max-cycles=X`: Only execute a maximum number of `X` clock (not machine!) cycles.
* `--resolution-scaling=X`: Scaling of the game window's resolution. A value of 1 corresponds to the original resolution of 160x144. Default 4.
* `--rom-path=X`: Specifies the ROM/game `X` that shall be executed.
* `--single-step`: Prints the CPU state before the execution pf each instruction.
* `--symbol-file`: Traces accesses to the ROM and dumps a symbol file (trace.sym) on exit. The file can be used in debuggers and disassemblers.
* `--show-ext-game-window`: Show the extended game window that renders out-of-viewport background tiles.
* `--show-window-window`: Show the window tile data table.
* `--wait-for-gdb`: Wait for a GDB remote connection on port 1337.
* `--quick-boot`: Faster boot that skips the logo scrolling and data check.

## Impressions

### Games

| Alleyway                   | Super Mario Land       |
|:--------------------------:|:----------------------:|
| <img alt="Alleyway screenshot" src="./assets/alleyway.png" width="95%"> | <img alt="Super Mario Land screenshot" src="./assets/super_mario_land.png" width="95%"> |

| Tetris                   | The Jungle Book          |
|:------------------------:|:------------------------:|
| <img alt="Tetris screenshot" src="./assets/tetris.png" width="95%"> | <img alt="The Jungle Book screenshpt" src="./assets/jungle_book.png" width="95%"> |

### Color Palettes (`--color-palette`)

| Default                  | Original                 | [Cozy Camp](https://lospec.com/palette-list/cozy-camp) |
|:------------------------:|:------------------------:|:-------------------------------------------------------:|
| f2ffd9aaaaaa555555000000 | 9bbc0f8bac0f3062300f380f | f8eacfc07e5330455c071721 |
| <img alt="Default palette" src="./assets/palette_default.png" width="95%"> | <img alt="Original palette" src="./assets/palette_original.png" width="95%"> | <img alt="Cozy camp palette" src="./assets/palette_cozy_camp.png" width="95%"> |

### Extended Game Window (`--show-ext-game-window`)

<div align="center">
<img width="50%" alt="Extended screen view" src="./assets/extended_screen_zelda.gif"><br>
</div>

### Tile Data (`--show-window-window`)

| Screen                | Tile Data Table             | 
|:------------------------:|:------------------------:|
| <img alt="Default palette" src="./assets/screenshot_zelda.png" width="65%"> | <img alt="Original palette" src="./assets/tile_data_table_zelda.png" width="95%"> |


## Controls

| Keyboard  | Game Boy  |
|-----------|-----------|
| ←,↑,→,↓   | ←,↑,→,↓   |
| A         | A         |
| S         | B         |
| O         | Select    |
| P         | Start     |

Utilities:

| Keyboard  | Description                                  |
|-----------|----------------------------------------------|
| 1         | Press to disable/enable background rendering |
| 2         | Press to disable/enable window rendering     |
| 3         | Press to disable/enable sprites rendering    |
| SPACE     | Hold for turbo mode (3x speed)               |

## Documentation

* [Overview](https://www.chciken.com/tlmboy/2022/02/02/gameboy-systemc.html)
* [GDB Remote Serial Protocol](https://www.chciken.com/tlmboy/2022/04/03/gdb-z80.html)
