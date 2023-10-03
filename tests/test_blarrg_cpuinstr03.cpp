
/*******************************************************************************
 * Apache License, Version 2.0
 * Copyright (c) 2023 chciken/Niko
 *
 * This test checks blarrg's cpuinst03 (op sp,hl)
 ******************************************************************************/
#include <getopt.h>
#include <gtest/gtest.h>

#include "gb_top.h"
#include "options.h"
#include "utils.h"

Options options;

TEST(BlarrgTest, cpuinstr03) {
  string tlm_boy_root = GetEnvVariable("TLMBOY_ROOT");
  options.boot_rom_path = tlm_boy_root + "/roms/DMG_ROM.bin";
  options.rom_path = tlm_boy_root + "/roms/gb-test-roms/cpu_instrs/individual/03-op sp,hl.gb";

  GbTop test_top("test_top", options);
  sc_start(9, SC_SEC);
  test_top.gb_ppu.game_wndw->SaveScreenshot("blarrgs_cpuinstr03.bmp");

  if (options.headless == true) {
    ASSERT_EQ(test_top.gb_cpu.cpu_state, Cpu::CpuState::kTestPassed);
  } else {
    ASSERT_TRUE(CompareFiles("blarrgs_cpuinstr03.bmp", tlm_boy_root + "/tests/golden_files/blarrgs_cpuinstr03.bmp"));
  }
}

int sc_main(int argc, char* argv[]) {
  ::testing::InitGoogleTest(&argc, argv);
  options.InitOpts(argc, argv);

  return RUN_ALL_TESTS();
}
