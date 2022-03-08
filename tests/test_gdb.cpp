
/************************************************************************************
 * Copyright (C) 2022 chciken/Niko
 * MIT License
 *
 * This test boots up the Gameboy and compares the states of the register
 * against a golden file once per machine cycle using the GDB remote serial protocol.
 ************************************************************************************/

#include <cstdlib>
#include <gtest/gtest.h>
#include <string>

#include "utils.h"

const std::string tlm_boy_root = GetEnvVariable("TLMBOY_ROOT");

TEST(GdbTests, SystemTest) {
  std::string exe = tlm_boy_root + "/build/tlmboy_test";
  std::string rom_path = tlm_boy_root + "/roms/dummy.bin";
  std::string boot_rom_path = tlm_boy_root + "/roms/DMG_ROM.bin &";
  std::string cmd_test = exe + " --max-cycles=800 --headless --wait-for-gdb -r " + rom_path + " -b " + boot_rom_path;

  std::string boot_checker_path = tlm_boy_root + "/tests/boot_checker_gdb.py";
  std::string cmd_gdb = "z80-unknown-elf-gdb --batch -x " + boot_checker_path;

  std::system(cmd_test.c_str());
  int ret = std::system(cmd_gdb.c_str());
  ASSERT_EQ(ret, 0);
}

int sc_main(int argc, char* argv[]) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
