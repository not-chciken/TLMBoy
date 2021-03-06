
/*******************************************************************************
 * Copyright (C) 2022 chciken/Niko
 * MIT License
 *
 * This test checks blarrg's cpuinst01 (special instructions)
 ******************************************************************************/
#include <getopt.h>
#include <gtest/gtest.h>
#include "gb_top.h"
#include "options.h"
#include "utils.h"

TEST(BlarrgTest, cpuinstr01) {
  std::string tlm_boy_root = GetEnvVariable("TLMBOY_ROOT");

  GbTop test_top("test_top", tlm_boy_root + "/roms/gb-test-roms/cpu_instrs/individual/01-special.gb",
                             tlm_boy_root + "/roms/DMG_ROM.bin", options::headless);
  sc_start(9, SC_SEC);
  test_top.gb_ppu.game_wndw->SaveScreenshot("blarrgs_cpuinstr01.bmp");

  if (options::headless == true) {
    ASSERT_EQ(test_top.gb_cpu.cpu_state, Cpu::CpuState::kTestPassed);
  } else {
    ASSERT_TRUE(CompareFiles("blarrgs_cpuinstr01.bmp", tlm_boy_root + "/tests/golden_files/blarrgs_cpuinstr01.bmp"));
  }
}

int sc_main(int argc, char* argv[]) {
  ::testing::InitGoogleTest(&argc, argv);

  const struct option long_opts[] = {
    {"headless", no_argument, 0, 'l'}, 0
  };

  for (;;) {
    int index;
    switch (getopt_long(argc, argv, "l", long_opts, &index)) {
      case 'l':
        options::headless = true; break;
        continue;
      default :
        break;
    }
    break;
  }
  return RUN_ALL_TESTS();
}
