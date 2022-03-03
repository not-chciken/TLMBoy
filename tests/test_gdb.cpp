
/*******************************************************************************
 * Copyright (C) 2022 chciken/Niko
 * MIT License
 *
 * This test boots up the Gameboy and compares the states of the register
 * against a golden file once per machine cycle.
 * The test simulates 100,000 machine cycles.
 ******************************************************************************/
#include <cstdlib>
#include <gtest/gtest.h>
#include <string>

#include "utils.h"

const std::string tlm_boy_root = GetEnvVariable("TLMBOY_ROOT");

TEST(GdbTests, Boot) {
  std::string exe = tlm_boy_root + "/build/tlmboy_test";
  std::string rom_path = tlm_boy_root + "/roms/dummy.bin";
  std::string boot_rom_path = tlm_boy_root + "/roms/DMG_ROM.bin";
  std::string cmd = exe + " --headless --wait-for-gdb -r " + rom_path + " -b " + boot_rom_path;
  int ret = std::system(cmd.c_str());
  ASSERT_FALSE(ret);
}

int sc_main(int argc, char* argv[]) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
