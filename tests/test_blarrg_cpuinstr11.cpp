
/*******************************************************************************
 * Copyright (C) 2022 chciken/Niko
 * MIT License
 *
 * This test checks blarrg's cpuinst10 (bit instructions)
 ******************************************************************************/
#include <gtest/gtest.h>
#include "gb_top.h"

TEST(BlarrgTest, cpuinstr11) {
  std::string tlm_boy_root = std::string(std::getenv("TLMBOY_ROOT"));
  GbTop test_top("test_top", tlm_boy_root + "/roms/blarrgs/cpu_instrs/individual/11-op a,(hl).gb",
                             tlm_boy_root + "/roms/DMG_ROM.bin");
  sc_start(25, SC_SEC);
  test_top.gb_ppu.game_wndw.SaveScreenshot("blarrgs_cpuinstr11.bmp");
  ASSERT_TRUE(CompareFiles("blarrgs_cpuinstr11.bmp", tlm_boy_root + "/tests/golden_files/blarrgs_cpuinstr11.bmp"));
}

int sc_main(int argc, char* argv[]) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
