
/*******************************************************************************
 * Apache License, Version 2.0
 * Copyright (c) 2023 chciken/Niko
 *
 * This test boots up the Gameboy.
 ******************************************************************************/
#include <getopt.h>
#include <gtest/gtest.h>

#include "cartridge.h"
#include "cpu.h"
#include "bus.h"
#include "game_info.h"
#include "gb_top.h"
#include "generic_memory.h"
#include "io_registers.h"
#include "options.h"
#include "ppu.h"
#include "utils.h"

const std::string tlm_boy_root = GetEnvVariable("TLMBOY_ROOT");

struct Top : public GbTop {
  SC_HAS_PROCESS(Top);
  Top ()
      : GbTop("gb_top", tlm_boy_root + "/roms/dummy.bin",
                        tlm_boy_root + "/roms/DMG_ROM.bin") {
    std::cout << static_cast<std::string>(*cartridge.game_info);
    SC_METHOD(SigHandler);
    dont_initialize();
    sensitive << io_registers.sig_unmap_rom_out;
  }

  // Stop once the ROM gets unmapped
  void SigHandler() {
    sc_stop();
  }
};

TEST(BootTests, Boot) {
  Top test_top;
  sc_start();
  test_top.gb_ppu.game_wndw->SaveScreenshot("test_boot.bmp");
  ASSERT_EQ(test_top.cartridge.game_info->GetTitle(), "DUMMY");
  ASSERT_EQ(test_top.cartridge.game_info->GetLicenseCode(), "none");
  ASSERT_EQ(test_top.cartridge.game_info->GetCartridgeType(), "MBC5+BAT+RAM");
  ASSERT_EQ(test_top.cartridge.game_info->GetRomSize(), 8);
  ASSERT_EQ(test_top.cartridge.game_info->GetRamSize(), 32);
  if (options::headless == false) {
    ASSERT_TRUE(CompareFiles("test_boot.bmp", tlm_boy_root + "/tests/golden_files/test_boot.bmp"));
  }
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