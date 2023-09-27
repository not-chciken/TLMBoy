
/*******************************************************************************
 * Apache License, Version 2.0
 * Copyright (c) 2023 chciken/Niko
 *
 * Testing the PPU using the Acid2 test from:
 * https://github.com/mattcurrie/dmg-acid2
 ******************************************************************************/

#include <getopt.h>
#include <gtest/gtest.h>
#include "gb_top.h"
#include "options.h"
#include "utils.h"

TEST(PpuTest, DmgAcid2) {
  std::string tlm_boy_root = GetEnvVariable("TLMBOY_ROOT");

  GbTop test_top("test_top", tlm_boy_root + "/roms/dmg-acid2.gb",
                             tlm_boy_root + "/roms/DMG_ROM.bin", options::headless);
  sc_start(5, SC_SEC);
  test_top.gb_ppu.game_wndw->SaveScreenshot("dmg-acid2.bmp");

  ASSERT_TRUE(CompareFiles("dmg-acid2.bmp", tlm_boy_root + "/tests/golden_files/dmg-acid2.bmp"));
}

int sc_main(int argc, char* argv[]) {
  ::testing::InitGoogleTest(&argc, argv);

  const struct option long_opts[] = {
    {"headless", no_argument, 0, 'l'},
    {nullptr, 0, nullptr, 0}
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