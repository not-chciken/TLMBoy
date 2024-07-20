#pragma once
/*******************************************************************************
 * Apache License, Version 2.0
 * Copyright (c) 2023 chciken/Niko
 *
 * A struct for all the CLI arguments/options.
 ******************************************************************************/

#include "common.h"

namespace fs = std::filesystem;

struct Options {
  bool headless = false;
  bool symbol_file = false;
  bool single_step = false;
  bool wait_for_gdb = false;
  fs::path rom_path = "";
  fs::path boot_rom_path = "../roms/DMG_ROM.bin";
  i64 fps_cap = 60;
  i64 max_cycles = -1;

  // Parses the arguments from the CLI and sets option variables accordingly for the TLMBoy's main.
  void InitOpts(int argc, char* argv[]);
};
