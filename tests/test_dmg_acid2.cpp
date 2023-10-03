
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

Options options;

TEST(PpuTest, DmgAcid2) {
  string tlm_boy_root = GetEnvVariable("TLMBOY_ROOT");
  options.boot_rom_path = tlm_boy_root + "/roms/DMG_ROM.bin";
  options.rom_path = tlm_boy_root + "/roms/dmg-acid2.gb";

  GbTop test_top("test_top", options);
  sc_start(5, SC_SEC);
  test_top.gb_ppu.game_wndw->SaveScreenshot("dmg-acid2.bmp");

  ASSERT_TRUE(CompareFiles("dmg-acid2.bmp", tlm_boy_root + "/tests/golden_files/dmg-acid2.bmp"));
}

int sc_main(int argc, char* argv[]) {
  ::testing::InitGoogleTest(&argc, argv);
  options.InitOpts(argc, argv);

  return RUN_ALL_TESTS();
}
