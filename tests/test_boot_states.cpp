
/*******************************************************************************
 * Copyright (C) 2022 chciken/Niko
 * MIT License
 *
 * This test boots up the Gameboy and compares the states of the register
 * against a golden file once per machine cycle.
 * The test simulates 100,000 machine cycles.
 ******************************************************************************/
#include <gtest/gtest.h>

#include "bus.h"
#include "cartridge.h"
#include "cpu.h"
#include "common.h"
#include "game_info.h"
#include "gb_top.h"
#include "generic_memory.h"
#include "io_registers.h"
#include "ppu.h"
#include "utils.h"

const std::string tlm_boy_root = GetEnvVariable("TLMBOY_ROOT");

TEST(BootTests, BootState) {
  GbTop test_top("test_top", tlm_boy_root + "/roms/dummy.bin",
                             tlm_boy_root + "/roms/DMG_ROM.bin");
  std::ofstream out("boot_states.txt");
  std::cout.rdbuf(out.rdbuf());

  sc_start(gb_const::kNsPerMachineCycle * 100000, SC_NS);
  ASSERT_TRUE(CompareFiles("boot_states.txt", tlm_boy_root + "/tests/golden_files/boot_states.txt"));
}

int sc_main(int argc, char* argv[]) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
