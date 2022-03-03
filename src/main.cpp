
/*******************************************************************************
 * Copyright (C) 2022 chciken
 * MIT License
 *
 * Main file of the TLM Boy,
 * Here resides the top module which connects all submodule from CPU to PPU.
 ******************************************************************************/
#include "gb_top.h"
#include "options.h"
#include <getopt.h>

int sc_main(int argc, char* argv[]) {

  const struct option long_opts[] = {
    {"boot-rom-path", required_argument, 0, 'b'},
    {"rom-path", required_argument,      0, 'r'},
    {"help",     no_argument,            0, 'h'},
    {"headless", no_argument,            0, 'l'},
    {"wait-for-gdb", no_argument,        0, 'w'}, 0
  };

  int index;
  for (;;) {
    switch (getopt_long(argc, argv, "r:hlwb:", long_opts, &index)) {
      case 'b':
        options::boot_rom_path = fs::path(optarg);
        continue;
      case 'r':
        options::rom_path = fs::path(optarg);
        continue;
      case 'l':
        options::headless = true;
        continue;
      case 'w':
        options::wait_for_gdb = true;
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

  if (options::rom_path.string().size() == 0) {
    std::cout << "No ROM path was specified!" << std::endl
              << "Usage: tlmboy -r <path_to_rom>" << std::endl
              << "Returning..." << std::endl;
    return 1;
  }
  if (fs::exists(options::rom_path) == false) {
    std::cout << "Given ROM path does not exist!" << std::endl;
    return 1;
  }
  if (fs::status(options::rom_path).type() == fs::file_type::directory) {
    std::cout << "Given ROM path is a directory! Need a file!" << std::endl;
    return 1;
  }

  GbTop gb_top("game_boy_top", options::rom_path, options::boot_rom_path,
               options::headless, options::wait_for_gdb);
  std::cout << static_cast<std::string>(*gb_top.cartridge.game_info);
  sc_start();  // RUN!
  return 0;
}
