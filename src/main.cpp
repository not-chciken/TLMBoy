
/*******************************************************************************
 * Copyright (C) 2022 chciken
 * MIT License
 *
 * Main file of the TLM Boy,
 * Here resides the top module which connects all submodule from CPU to PPU.
 ******************************************************************************/
#include "gb_top.h"
#include <getopt.h>

namespace options {
  std::string rom_path = "";
  bool headless = false;
}

int sc_main(int argc, char* argv[]) {

  const struct option long_opts[] =
  {
      {"rom-path", required_argument, 0, 'r'},
      {"help",     no_argument,       0, 'h'},
      {"headless", no_argument,       0, 'l'},
      0
  };

  int index;
  for (;;) {
    switch (getopt_long(argc, argv, "r:hl", long_opts, &index)) {
      case 'r':
        options::rom_path = std::string(optarg);
        continue;
      case 'l':
        options::headless = true;
        continue;
      case '?':
      case 'h':
      default :
        std::cout << "######### TLM Boy #########" << std::endl
                  << "Usage: tlmboy -r <path_to_rom>" << std::endl
                  << "Optional: --headless" << std::endl
                  << "          Runs the TLMBoy without any graphical output." << std::endl;
        return 0;
      case -1:
        break;
    }
    break;
  }

  if (options::rom_path.size() == 0) {
    std::cout << "No ROM path was specified!" << std::endl
              << "Usage: tlmboy -r <path_to_rom>" << std::endl
              << "Returning..." << std::endl;
    return 1;
  }

  GbTop gb_top("game_boy_top", options::rom_path, "../roms/DMG_ROM.bin", options::headless);
  std::cout << static_cast<std::string>(*gb_top.cartridge.game_info);
  sc_start();  // RUN!
  return 0;
}
