
/*******************************************************************************
 * Apache License, Version 2.0
 * Copyright (c) 2022 chciken/Niko
 *
 * This test checks blarrg's cpuinst02 (interrupts)
 * See: https://forums.nesdev.org/viewtopic.php?t=9289
 ******************************************************************************/
#include <getopt.h>
#include <gtest/gtest.h>

#include "gb_top.h"
#include "options.h"
#include "utils.h"

Options options;

TEST(BlarrgTest, cpuinstr02) {
  string tlm_boy_root = GetEnvVariable("TLMBOY_ROOT");
  options.boot_rom_path = tlm_boy_root + "/roms/DMG_ROM.bin";
  options.rom_path = tlm_boy_root + "/roms/gb-test-roms/cpu_instrs/individual/02-interrupts.gb";

  GbTop test_top("test_top", options);
  sc_start(8, SC_SEC);
  test_top.ppu.game_wndw->SaveScreenshot("blarrgs_cpuinstr02.bmp");

  if (options.headless == true) {
    ASSERT_EQ(test_top.cpu.cpu_state, Cpu::CpuState::kTestPassed);
  } else {
    ASSERT_TRUE(CompareFiles("blarrgs_cpuinstr02.bmp", tlm_boy_root + "/tests/golden_files/blarrgs_cpuinstr02.bmp"));
  }
}

int sc_main(int argc, char* argv[]) {
  ::testing::InitGoogleTest(&argc, argv);
  options.InitOpts(argc, argv);

  return RUN_ALL_TESTS();
}