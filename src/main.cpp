
/*******************************************************************************
 * Apache License, Version 2.0
 * Copyright (c) 2023 chciken/Niko
 *
 * Main file of the TLM Boy,
 * Here resides the top module which connects all submodule from CPU to PPU.
 ******************************************************************************/

#include "gb_top.h"
#include "options.h"

int sc_main(int argc, char* argv[]) {
  Options options;
  options.InitOpts(argc, argv);

  if (options.rom_path.string().size() == 0) {
    std::cout << "No ROM path was specified!" << std::endl
              << "Usage: tlmboy -r <path_to_rom>" << std::endl
              << "Returning..." << std::endl;
    return 1;
  } else if (fs::exists(options.rom_path) == false) {
    std::cout << "Given ROM path does not exist!" << std::endl;
    return 1;
  } else if (fs::status(options.rom_path).type() == fs::file_type::directory) {
    std::cout << "Given ROM path is a directory! Need a file!" << std::endl;
    return 1;
  }

  GbTop gb_top("game_boy_top", options);

  std::cout << static_cast<std::string>(*gb_top.cartridge.game_info);

  if (options.max_cycles < 0) {
    sc_start();
  } else {
    sc_start(sc_time(gb_const::kNsPerClkCycle * options.max_cycles, SC_NS));
  }

  return 0;
}
