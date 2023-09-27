#pragma once
/*******************************************************************************
 * Apache License, Version 2.0
 * Copyright (c) 2023 chciken/Niko
 *
 * All the debug prints. Feel free to add further ones.
 ******************************************************************************/

#include <getopt.h>

#include <string>

namespace fs = std::filesystem;

namespace options {

bool headless = false;
bool single_step = false;
bool wait_for_gdb = false;
fs::path rom_path = "";
fs::path boot_rom_path = "../roms/DMG_ROM.bin";
i64 max_cycles = -1;

// Parses the arguments from the CLI and sets option variables accordingly for the TLMBoy's main.
inline void GetMainOptions(int argc, char* argv[]) {
  const struct option long_opts[] = {{"boot-rom-path", required_argument, 0, 'b'},
                                     {"help", no_argument, 0, 'h'},
                                     {"headless", no_argument, 0, 'l'},
                                     {"max-cycles", required_argument, 0, 'm'},
                                     {"rom-path", required_argument, 0, 'r'},
                                     {"single-step", no_argument, 0, 's'},
                                     {"wait-for-gdb", no_argument, 0, 'w'},
                                     {nullptr, 0, nullptr, 0}};

  int index;
  for (;;) {
    switch (getopt_long(argc, argv, "r:hlwbs:", long_opts, &index)) {
      case 'b':
        options::boot_rom_path = fs::path(optarg);
        continue;
      case 'r':
        options::rom_path = fs::path(optarg);
        continue;
      case 'l':
        options::headless = true;
        continue;
      case 'm':
        options::max_cycles = std::stoll(std::string(optarg));
        continue;
      case 's':
        options::single_step = true;
        continue;
      case 'w':
        options::wait_for_gdb = true;
        continue;
      case '?':
      case 'h':
      default:
        std::cout << "######### TLM Boy #########" << std::endl
                  << "Usage: tlmboy -r <path_to_rom>" << std::endl
                  << "Optional: --headless" << std::endl
                  << "          Runs the TLMBoy without any graphical output." << std::endl
                  << "          --single-step" << std::endl
                  << "          Prints the CPU state before each instruction" << std::endl
                  << "          --wait-for-gdb" << std::endl
                  << "          Wait for a GDB remote connection on port 1337." << std::endl;
        exit(1);
      case -1:
        break;
    }
    break;
  }
}

}  // namespace options
