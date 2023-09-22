/*******************************************************************************
 * Copyright (C) 2021 chciken
 * MIT License
 *
 * This class implements the Game Boy's PPU (Pixel Processing Unit).
 * Memory map:
 * tile_data_low   = 0x8000-0x8FFF
 * tile_data_high  = 0x8800-0x97FF
 * tile_map_low    = 0x9800-0x9BFF
 * tile_map_up     = 0x9C00-0x9FFF
 ******************************************************************************/
#pragma once

#include <stdlib.h>
#include <unistd.h>

#include <bitset>
#include <cassert>
#include <filesystem>
#include <memory>
#include <iostream>
#include <sstream>
#include <string>

#include "common.h"
#include "debug.h"
#include "SDL2/SDL.h"

#define TILE_BYTES 16  // Number of bytes per tile.

// Sources: https://www.youtube.com/watch?v=zQE1K074v3s -> cool video for v blank and h blank interrupt

struct Ppu : public sc_module {
  SC_HAS_PROCESS(Ppu);

  static constexpr u8 renderColor[4][3] = {{242,255,217}, {170,170,170}, {85,85,85}, {0,0,0}};
  static const u8 kOamEntryBytes = 4;   // Bytes per OAM entry.
  static const u8 kNumOamEntries = 40;  // Number of OAM entries. Hence, 40 sprites can be displayed at max.
  static const u8 kTileLength    = 8;   // Length of a normal tile in pixels.

  static const int kGbScreenWidth  = 160;
  static const int kGbScreenHeight = 144;
  static const int kGbScreenBufferWidth = 256;
  static const int kGbScreenBufferHeight = 256;
  static const int kRenderScaling  = 4;
  static const int kRenderWndwWidth  = kGbScreenWidth * kRenderScaling;
  static const int kRenderWndwHeight = kGbScreenHeight * kRenderScaling;

  // Masks for reg_0xFF40.
  static const u8 kMaskLcdControl         = 0b10000000;  // bit 7; 1 -> operate
  static const u8 kMaskWndwTileMapSlct    = 0b01000000;  // bit 6; 0 -> 0x9800-0x9BFF. 1 -> 0x9C00-0x9FFF
  static const u8 kMaskWndwDisp           = 0b00100000;  // bit 5; 0 -> off, 1 -> on
  static const u8 kMaskBgWndwTileDataSlct = 0b00010000;  // bit 4: 0 -> 0x8800-0x97FF, 1 -> 0x8000-0x8FFF
  static const u8 kMaskBgTileSlct         = 0b00001000;  // bit 3: 0 -> 0x9800-0x9BFF, 1 -> 0x9C00-0x9FFF
  static const u8 KMaskObjSpriteSize      = 0b00000100;  // bit 2: 0 -> 8*8, 1 -> 8*16
  static const u8 kMaskObjSpriteDisp      = 0b00000010;  // bit 1: 0 -> off, 1 -> on
  static const u8 kMaskBgWndwDisp         = 0b00000001;  // bit 0: 0 -> off, 1 -> on

  // Masks for sprite oam entry.
  static const u8 kMaskSpritePriority = 0b10000000;  // bit 7: 0 -> on top of background 1 -> behind background
  static const u8 kMaskSpriteYFlip    = 0b01000000;  // bit 6: flip y if 1
  static const u8 kMaskSpriteXFlip    = 0b00100000;  // bit 5: flip x if 1
  static const u8 kMaskSpriteXPalNum  = 0b00010000;  // bit 4: sprite palette number

  // Addresses.
  static const u16 kAdrRegLcdc    = 0xFF40;
  static const u16 kAdrRegStat    = 0xFF41;
  static const u16 kAdrRegScrollY = 0xFF42;
  static const u16 kAdrRegScrollX = 0xFF43;
  static const u16 kAdrRegLcdcY   = 0xFF44;
  static const u16 kAdrRegLyComp  = 0xFF45;
  static const u16 kAdrRegDma     = 0xFF46;
  static const u16 kAdrRegBgp     = 0xFF47;
  static const u16 kAdrRegObp0    = 0xFF48;
  static const u16 kAdrRegObp1    = 0xFF49;
  static const u16 kAdrRegWndwY   = 0xFF4A;
  static const u16 kAdrRegWndwX   = 0xFF4B;
  static const u16 kAdrTilemapLow   = 0x9800;
  static const u16 kAdrTilemapHigh  = 0x9C00;
  static const u16 kAdrTiledataLow  = 0x8800;
  static const u16 kAdrTiledataHigh = 0x9C00;

  const u8 kMaskVBlankIE = 1;
  const u8 kMaskLcdcStatIf = 1 << 1;
  const u8 kMaskTimerIf = 1 << 2;
  const u8 kMaskSerialIoIf = 1 << 3;

  // User interface.
  static bool uiRenderBg;
  static bool uiRenderSprites;
  static bool uiRenderWndw;

  void start_of_simulation() override;
  explicit Ppu(sc_module_name name, bool headless = false);
  ~Ppu();

  tlm_utils::simple_initiator_socket<Ppu, gb_const::kBusDataWidth> init_socket;
  sc_in_clk clk;  // clock for the PP

  u8 *reg_0xFF40;
  u8 *reg_0xFF41;
  u8 *reg_scroll_y;
  u8 *reg_scroll_x;
  u8 *reg_lcdc_y;
  u8 *reg_ly_comp;
  u8 *reg_dma;       // direct memory access TODO(me)
  u8 *reg_bgp;       // bg and window palette data TODO(me)
  u8 *reg_obp_0;     // object palette 0 data
  u8 *reg_obp_1;     // object palette 1 data
  u8 *reg_wndw_y;    // window y position
  u8 *reg_wndw_x;    // window x position
  u8 *reg_ie;        // interrupt enable TODO(me)
  u8 *reg_intr_pending_dmi;  // interrupt pendin TODO(me)

  u8 bg_buffer[kGbScreenHeight][kGbScreenWidth];
  u8 window_buffer[kGbScreenBufferHeight][kGbScreenBufferWidth];
  u8 sprite_buffer[kGbScreenHeight][kGbScreenWidth];

  // The GB has two tile data tables
  // first: 0x8000-0x8FFF, second from 0x8800-0x97FF (note the overlap!)
  u8* tile_data_table_low;
  u8* tile_data_table_up;

  // Every tile is 8x8 pixels and needs 16Byte
  // We can conclude that every pixel needs 0.25Byte or 2 bit
  // and the first data table can store 256 tiles
  // Thus, every pixel can have 4 colors (11=white, 10=dark-gray, 01=light-gray, 00=black)

  // The background and window tile map resides at 0x9800-0x9BFF for the lower table
  // and at 0x9C00-0x9FFF for the upper part
  // each tile map has 4kiB of RAM (=256 tiles)
  // together they have 6 kiB of RAM (=384 tiles) (NOTE: the areas do overlap!)
  u8* tile_map_low;
  u8* tile_map_up;
  // the object attribute memory has 40 4 Byte blocks (160B in total)
  // whereby block corresponds to a byte
  // Range from 0xFE00-0xFE9F
  u8* oam_table;

  // Maps the colours for the background and the window according to register rBGP (0xff47).
  constexpr u8 MapBgCols(const u8 val);
  void DrawBgToLine(uint line_num);
  void DrawSpriteToLine(int line_num);
  void DrawWndwToLine(int line_num);

  void CheckLycInterrupt();

  // interleaves two selected bits of two bit vectors
  // and arranges them in a screen buffer friendly way
  // example a=0b00001000, b=00000000, pos=3
  // returns 0b00000010
  // pos e [0,7]
  // return value is always e[0,3]
  static u8 InterleaveBits(const u8 a, const u8 b, const uint pos);

  void RenderLoop();

  std::string StateStr();

  class RenderWindow {
   public:
    RenderWindow(const uint width, const uint height, const uint log_width, const uint log_height);
    RenderWindow();
    virtual ~RenderWindow();

    // Saves a screenshot of the currently rendered frame.
    virtual void SaveScreenshot(const std::filesystem::path file_path);
    // Draw data to screen.
    virtual void DrawToScreen(Ppu &p) = 0;

   protected:
    SDL_Renderer *renderer;
    SDL_Window *window;
    uint width;
    uint height;
    uint log_width;  // Logical width.
    uint log_height;  // Logical height.
  };

  std::unique_ptr<RenderWindow> game_wndw;
  std::unique_ptr<RenderWindow> window_wndw;

  // This the main window.
  class GameWindow : public RenderWindow {
   public:
    using RenderWindow::RenderWindow;
    void DrawToScreen(Ppu &ppu) override;
  };

  // Used for displaying the window tiles. Intended for debugging/analysis.
  class WindowWindow : public RenderWindow {
   public:
    using RenderWindow::RenderWindow;
    void DrawToScreen(Ppu &ppu) override;
  };

  // For the headless mode. Doesn't create any windows, hence renders nothing.
  class DummyWindow : public RenderWindow {
   public:
    DummyWindow():RenderWindow() {}
    void DrawToScreen(Ppu &ppu [[maybe_unused]]) override {};  // Do nothing.
    void SaveScreenshot(const std::filesystem::path file_path [[maybe_unused]]) override {};  // Do nothing.
  };

 private:
  int window_line_;  // Internal window line counter.
  constexpr u8 MapSpriteCols(const u8 val, const u8* reg);
};
