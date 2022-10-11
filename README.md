![example workflow](https://github.com/not-chciken/TLMBoy/actions/workflows/build.yml/badge.svg)
[![Codacy Badge](https://app.codacy.com/project/badge/Coverage/4791a60cefd140328652ee67756c69b9)](https://www.codacy.com/gh/not-chciken/TLMBoy/dashboard?utm_source=github.com&utm_medium=referral&utm_content=not-chciken/TLMBoy&utm_campaign=Badge_Coverage)
[![Codacy Badge](https://app.codacy.com/project/badge/Grade/4791a60cefd140328652ee67756c69b9)](https://www.codacy.com/gh/not-chciken/TLMBoy/dashboard?utm_source=github.com&amp;utm_medium=referral&amp;utm_content=not-chciken/TLMBoy&amp;utm_campaign=Badge_Grade)
# TLMBoy
A Game Boy Simulator written in C++/SystemC TLM-2.0. <br>
This project also acts as my personal playground for new C++20 features.
Hence, a quite recent version of gcc or clang is required to compile it.
__Note: This project is currently under construction ;)__

The project is structured as follows:
```
├── roms/
|   # This directory includes some example ROMs/games and the boot ROM.
|   # Also used by the tests.
├── src/
│   # The heart of the project: the source code of every Game Boy
│   # module can be found here.
└── tests/
    # Collection of unit tests and system tests.
    └── golden_files
        # Golden files like screenshots.
```

# Building
Use the following commands to build the TLMBoy:
```bash
cd TLMBoy
mkdir build
cd build
cmake tlmboy ..
cmake --build . --target tlmboy --config Release
```
Dependencies:
-  [SystemC 2.3.3](https://github.com/accellera-official/systemc)
-  [fmt](https://github.com/fmtlib/fmt)
-  [SDL2](https://github.com/libsdl-org/SDL)
-  [googletest](https://github.com/google/googletest)

# Controls
| Keyboard  | Game Boy  |
|-----------|-----------|
| ←,↑,→,↓   | ←,↑,→,↓   |
| A         | A         |
| S         | B         |
| O         | Select    |
| P         | Start     |

Utilities:
Keyboard 0 = Do not render background
Keyboard 1 = Do not render sprites
Keyboard 2 = Do not render window
# Documentation
- [Overview] (https://www.chciken.com/tlmboy/2022/02/19/gameboy-systemc.html)
- [GDB Remote Serial Protocol](https://www.chciken.com/tlmboy/2022/04/03/gdb-z80.html)

# Command Line Arguments
- `--boot-rom-path=P`: Specifies the path `P` of the boot ROM. Points to "../roms/DMG_ROM.bin" by default.
- `--headless`: Run the TLMBoy without any graphical output. This is useful for CI environments.
- `--max-cycles=X`: Only execute a maximum number of `X` machine cycles.
- `--rom-path=P`: Specifies the ROM/game `P` that shall be executed.
- `--wait-for-gdb`: Wait for a GDB remote connection on port 1337.

# TODO
- Show full tile map in window (currently only the lower tile map is shown)
- Implement the sound processor
- Complete instructions (stop)
- BankSwitchMemory: Use seperate functions for loading of the boot ROM and the game.
- BankSwitchMemory: Implement enable/disable RAM
- BankSwitchMemory: Implement all MBC
- fmt::Format: use native c++ implementation once available
- use native c++ implementation for ranges once available (maybe with c++23)
- Fix bank size of ram
- Gzip golden trace files
- Find a simple and non-hacky way to get the number of clock cycles
- Checkpointing system