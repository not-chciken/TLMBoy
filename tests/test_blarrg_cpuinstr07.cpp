
/*******************************************************************************
 * Copyright (C) 2022 chciken/Niko
 * MIT License
 *
 * This test checks blarrg's cpuinst07
 ******************************************************************************/
#include <gtest/gtest.h>
#include "gb_top.h"

TEST(BlarrgTest, cpuinstr07) {
  std::string tlm_boy_root = std::string(std::getenv("TLMBOY_ROOT"));
  GbTop test_top("test_top", tlm_boy_root + "/roms/blarrgs/cpu_instrs/individual/07-jr,jp,call,ret,rst.gb",
                             tlm_boy_root + "/roms/DMG_ROM.bin");
  sc_start(8, SC_SEC);
  test_top.gb_ppu.game_wndw.SaveScreenshot("blarrgs_cpuinstr07.bmp");
  ASSERT_TRUE(CompareFiles("blarrgs_cpuinstr07.bmp", tlm_boy_root + "/tests/golden_files/blarrgs_cpuinstr07.bmp"));
}

int sc_main(int argc, char* argv[]) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
