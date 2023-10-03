/*******************************************************************************
 * Apache License, Version 2.0
 * Copyright (c) 2023 chciken/Niko
 ******************************************************************************/

#include "options.h"

#include <getopt.h>

void Options::InitOpts(int argc, char* argv[]) {
  const struct option long_opts[] = {{"boot-rom-path", required_argument, 0, 'b'},
                                     {"fps-cap", required_argument, 0, 'f'},
                                     {"headless", no_argument, 0, 'l'},
                                     {"help", no_argument, 0, 'h'},
                                     {"max-cycles", required_argument, 0, 'm'},
                                     {"rom-path", required_argument, 0, 'r'},
                                     {"single-step", no_argument, 0, 's'},
                                     {"wait-for-gdb", no_argument, 0, 'w'},
                                     {nullptr, 0, nullptr, 0}};

  int index;
  for (;;) {
    switch (getopt_long(argc, argv, "b:f:hm:lwr:s", long_opts, &index)) {
      case 'b':
        boot_rom_path = fs::path(optarg);
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
      case '?':
      case 'h':
      default:
        std::cout << "######### TLM Boy #########" << std::endl
                  << "Usage: tlmboy -r <path_to_rom> -b <path_to_boot_rom" << std::endl
                  << "Optional:" << std::endl
                  << "          --fps-cap" << std::endl
                  << "          Limits the maximum frames per second. Default 60." << std::endl
                  << "          --headless" << std::endl
                  << "          Runs the TLMBoy without any graphical output." << std::endl
                  << "          --max-cycles" << std::endl
                  << "          Maximum number of clock cycles to run. Default -1 = infinite." << std::endl
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
