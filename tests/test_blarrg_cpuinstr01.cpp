
/*******************************************************************************
 * Copyright (C) 2022 chciken/Niko
 * MIT License
 *
 * This test checks blarrg's cpuinst01 (special instructions)
 ******************************************************************************/
#include <gtest/gtest.h>
#include "gb_top.h"

TEST(BlarrgTest, cpuinstr01) {
  std::string tlm_boy_root = std::string(std::getenv("TLMBOY_ROOT"));
  GbTop test_top("test_top", tlm_boy_root + "/roms/blarrgs/cpu_instrs/individual/01-special.gb",
                             tlm_boy_root + "/roms/DMG_ROM.bin");
  sc_start(9, SC_SEC);
  test_top.gb_ppu.game_wndw.SaveScreenshot("blarrgs_cpuinstr01.bmp");
  ASSERT_TRUE(CompareFiles("blarrgs_cpuinstr01.bmp", tlm_boy_root + "/tests/golden_files/blarrgs_cpuinstr01.bmp"));
}

int sc_main(int argc, char* argv[]) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
