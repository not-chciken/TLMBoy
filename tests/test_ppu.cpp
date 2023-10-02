/*******************************************************************************
 * Apache License, Version 2.0
 * Copyright (c) 2023 chciken/Niko
 *
 * This files serves the testing of the Pixel Processing Unit.
 * It includes some simple unit tests which do not include SystemC
 * and test basic PPU data shuffling.
 * There's also a whole testbench which wires up the PpuStimulus with the PPU.
 * If everything works correctly, a flying "69" should be rendered.
 ******************************************************************************/

#include <getopt.h>
#include <gtest/gtest.h>
#include <systemc.h>
#include <tlm.h>
#include <tlm_utils/simple_initiator_socket.h>
#include <tlm_utils/simple_target_socket.h>

#include "gb_const.h"
#include "options.h"
#include "ppu.h"
#include "utils.h"

const std::string tlm_boy_root = GetEnvVariable("TLMBOY_ROOT");
Options options;

struct PpuStimulus : public sc_module {
  SC_HAS_PROCESS(PpuStimulus);
  sc_in_clk clk;
  tlm_utils::simple_target_socket<PpuStimulus, gb_const::kBusDataWidth> targ_socket;
  u8* memory;  // pointer to memory region

  // Cool tool to generate tiles: https://spkelly.net/tilegen/
  const u8 tile_data[16*8] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // tile0 = black
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x78, 0x38, 0x60, 0x20, 0x60, 0x20,  // tile1 = "6"
    0x7c, 0x3c, 0x7c, 0x24, 0x7c, 0x3c, 0x00, 0x00,
    0x00, 0x00, 0x7c, 0x3c, 0x6c, 0x24, 0x6c, 0x24,  // tile2 = "9"
    0x7c, 0x3c, 0x0c, 0x04, 0x7c, 0x3c, 0x00, 0x00,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,  // tile3 = white
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00,  // tile4 = light-gray
    0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00,
    0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF,  // tile5 = dark-gray
    0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF,
    0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA,  // tile6 = stripes0
    0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA,
    0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55,  // tile7 = stripes1
    0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55
  };

  PpuStimulus(u8 *memory, sc_module_name name)
      : sc_module(name),
        clk("clk"),
        targ_socket("targ_socket"),
        memory(memory) {
    targ_socket.register_get_direct_mem_ptr(this, &PpuStimulus::get_direct_mem_ptr);
    memset(memory, 0, 0x10000);
    memory[Ppu::kAdrRegLcdc] = 0b11110011;
    memory[Ppu::kAdrRegBgp] = 0b11011000;  // bg and wnw color mapping
    memory[Ppu::kAdrRegObp0] = 0b11011000;  // sprite color mapping

    for (uint i = 0; i < sizeof(tile_data); ++i)
        memory[0x8000+i] = tile_data[i];

    memory[Ppu::kAdrTilemapLow + 0] = 1;  // tilemap low
    memory[Ppu::kAdrTilemapLow + 1] = 2;
    memory[Ppu::kAdrTilemapHigh + 0] = 1;  // tilemap up
    memory[Ppu::kAdrTilemapHigh + 1] = 2;
    memory[Ppu::kAdrTilemapHigh + 2] = 3;
    memory[Ppu::kAdrTilemapHigh + 3] = 4;
    memory[Ppu::kAdrTilemapHigh + 4] = 5;
    memory[Ppu::kAdrTilemapHigh + 5] = 6;
    memory[Ppu::kAdrTilemapHigh + 6] = 0;
    memory[Ppu::kAdrTilemapHigh + 7] = 7;
    memory[Ppu::kAdrRegWndwY] = 100;
    memory[Ppu::kAdrRegWndwX] = 7;

    // Display a '6' as a sprite.
    memory[0xFE00] = 50;  // y position
    memory[0xFE01] = 50;  // x position
    memory[0xFE02] = 1;  // sprite index
    memory[0xFE03] = 0;  // flags

    SC_THREAD(StimulusLoop);
    sensitive << clk.pos();
  }

  void StimulusLoop() {
    while (1) {
      wait(70224);  // A complete screen refresh occurs every 70224 cycles.
      memory[Ppu::kAdrRegScrollY] += 1;
      memory[Ppu::kAdrRegScrollX] += 1;
    }
  }

  bool get_direct_mem_ptr(tlm::tlm_generic_payload& trans, tlm::tlm_dmi& dmi_data) {
    dmi_data.allow_read_write();
    const u16 adr = static_cast<u16>(trans.get_address());
    if (0xFF00 <= adr && adr <= 0xFF7F) {
      u8* data = &memory[adr];
      dmi_data.set_start_address(0x0);
      dmi_data.set_end_address(0x0B);
      dmi_data.set_dmi_ptr(reinterpret_cast<unsigned char*>(data));
      return true;
    } else if (0x8000 <= adr && adr <= 0x9FFF) {
      u8* data = &memory[0x8000];
      dmi_data.set_start_address(0x0);
      dmi_data.set_end_address(0x1FFF);
      dmi_data.set_dmi_ptr(reinterpret_cast<unsigned char*>(data));
      return true;
    } else if (0xFE00 <= adr && adr <= 0xFE9F) {
      u8* data = &memory[0xFE00];
      dmi_data.set_start_address(0x0);
      dmi_data.set_end_address(0x9F);
      dmi_data.set_dmi_ptr(reinterpret_cast<unsigned char*>(data));
      return true;
    } else if (0xFFFF == adr) {
      u8* data = &memory[0xFFFF];
      dmi_data.set_start_address(0x0);
      dmi_data.set_end_address(0x0);
      dmi_data.set_dmi_ptr(reinterpret_cast<unsigned char*>(data));
      return true;
    } else {
      return false;
    }
  }
};

struct Top : public sc_module {
  SC_HAS_PROCESS(Top);
  PpuStimulus test_stimulus;
  Ppu test_ppu;
  sc_clock global_clk;
  sc_signal<bool> intr_sig;
  u8 memory[0x10000];

  explicit Top(sc_module_name name [[maybe_unused]])
      : test_stimulus(memory, "test_stimulus"),
        test_ppu("test_ppu"),
        global_clk("global_clk", gb_const::kNsPerClkCycle, SC_NS, 0.5) {
    test_ppu.init_socket.bind(test_stimulus.targ_socket);

    // wire up the testbench
    test_ppu.clk(global_clk);
    test_stimulus(global_clk);
  }
};

// A PPU smoke test; if you see a screen with a scrolling 69 then everything is fine.
TEST(PpuTests, SmokeTest) {
  Top test_top("test_top");
  sc_start(4000, SC_MS);
  test_top.test_ppu.game_wndw->SaveScreenshot("test_ppu.bmp");

  ASSERT_TRUE(test_top.test_ppu.StateStr().size() != 0);

  if (options.headless == false) {
    ASSERT_TRUE(CompareFiles("test_ppu.bmp", tlm_boy_root + "/tests/golden_files/test_ppu.bmp"));
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
        options.headless = true; break;
        continue;
      default :
        break;
    }
    break;
  }
  return RUN_ALL_TESTS();
}
