/*******************************************************************************
 * Apache License, Version 2.0
 * Copyright (c) 2023 chciken/Niko
 ******************************************************************************/

#include "options.h"

#include <algorithm>
#include <getopt.h>

void Options::InitOpts(int argc, char* argv[]) {
  const struct option long_opts[] = {{"boot-rom-path", required_argument, 0, 'b'},
                                     {"color-palette", required_argument, 0, 'c'},
                                     {"fps-cap", required_argument, 0, 'f'},
                                     {"headless", no_argument, 0, 'l'},
                                     {"help", no_argument, 0, 'h'},
                                     {"max-cycles", required_argument, 0, 'm'},
                                     {"rom-path", required_argument, 0, 'r'},
                                     {"resolution-scaling", required_argument, 0, 'e'},
                                     {"single-step", no_argument, 0, 's'},
                                     {"symbol-file", no_argument, 0, 'y'},
                                     {"wait-for-gdb", no_argument, 0, 'w'},
                                     {nullptr, 0, nullptr, 0}};

  int index;
  for (;;) {
    switch (getopt_long(argc, argv, "b:f:hm:lwr:sc:e:", long_opts, &index)) {
      case 'b':
        boot_rom_path = fs::path(optarg);
        continue;
      case 'c':
        color_palette = string(optarg);
        continue;
      case 'e':
        resolution_scaling = std::stoll(string(optarg));
        continue;
      case 'f':
        fps_cap = std::stoll(string(optarg));
        continue;
      case 'l':
        headless = true;
        continue;
      case 'm':
        max_cycles = std::stoll(string(optarg));
        continue;
      case 'r':
        rom_path = fs::path(optarg);
        continue;
      case 's':
        single_step = true;
        continue;
      case 'w':
        wait_for_gdb = true;
        continue;
      case 'y':
        symbol_file = true;
        continue;
      case '?':
      case 'h':
      default:
        std::cout << "######### TLM Boy #########" << std::endl
                  << "Usage: tlmboy -r <path_to_rom> -b <path_to_boot_rom" << std::endl
                  << "Optional:" << std::endl
                  << "          --color-palette" << std::endl
                  << "          Color palette hex string with four RGB colors from bright to dark." << std::endl
                  << "          Default: f2ffd9aaaaaa555555000000." << std::endl
                  << "          --fps-cap" << std::endl
                  << "          Limits the maximum frames per second. Default 60." << std::endl
                  << "          --headless" << std::endl
                  << "          Runs the TLMBoy without any graphical output." << std::endl
                  << "          --max-cycles" << std::endl
                  << "          Maximum number of clock cycles to run. Default -1 = infinite." << std::endl
                  << "          --resolution-scaling" << std::endl
                  << "          Scaling of the game window's resolution. A value of 1 corresponds to the original resolution of 160x144." << std::endl
                  << "          Default: 4." << std::endl
                  << "          --single-step" << std::endl
                  << "          Prints the CPU state before each instruction" << std::endl
                  << "          --symbole-file" << std::endl
                  << "          Traces accesses to the ROM and dumps a symbol file (trace.sym) on exit." << std::endl
                  << "          --wait-for-gdb" << std::endl
                  << "          Wait for a GDB remote connection on port 1337." << std::endl;
        exit(1);
      case -1:
        break;
    }
    break;
  }

  if (color_palette.size() != 24) {
    std::cerr << "Invalid argument: Color palette string needs to be of length 24!";
    std::exit(1);
  }

  if (!std::all_of(color_palette.begin(), color_palette.end(), [](unsigned char c){ return std::isxdigit(c); })) {
    std::cerr << "Invalid argument: Color palette needs to a hex string!!";
    std::exit(1);
  }
}
