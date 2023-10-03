#pragma once
/*******************************************************************************
 * Apache License, Version 2.0
 * Copyright (c) 2023 chciken/Niko
 *
 * This class implements the Game Boy's PPU (Pixel Processing Unit).
 * Memory map:
 * tile_data_low   = 0x8000-0x8FFF
 * tile_data_high  = 0x8800-0x97FF
 * tile_map_low    = 0x9800-0x9BFF
 * tile_map_up     = 0x9C00-0x9FFF
 * Sources: https://www.youtube.com/watch?v=zQE1K074v3s
 * -> cool video for v blank and h blank interrupt
 ******************************************************************************/

#include <stdlib.h>
#include <unistd.h>

#include <bitset>
#include <cassert>
#include <filesystem>
#include <iostream>
#include <memory>
#include <sstream>

#include "SDL2/SDL.h"
#include "common.h"
#include "debug.h"

struct Ppu : public sc_module {
  SC_HAS_PROCESS(Ppu);

  static constexpr u8 renderColor[4][3] = {{242, 255, 217}, {170, 170, 170}, {85, 85, 85}, {0, 0, 0}};
  static const int kOamEntryBytes = 4;  // Bytes per OAM entry.
  static const int kNumOamEntries = 40;  // Number of OAM entries. Hence, 40 sprites can be displayed at max.
  static const int kTileLength = 8;  // Length of a normal tile in pixels.
  static const int kBytesPerTile = 16;

  static const int kGbScreenWidth = 160;
  static const int kGbScreenHeight = 144;
  static const int kGbScreenBufferWidth = 256;
  static const int kGbScreenBufferHeight = 256;
  static const int kRenderScaling = 4;
  static const int kRenderWndwWidth = kGbScreenWidth * kRenderScaling;
  static const int kRenderWndwHeight = kGbScreenHeight * kRenderScaling;

  // Masks for reg_lcdc.
  static const u8 kMaskLcdControl = 0b10000000;          // bit 7; 1 -> operate
  static const u8 kMaskWndwTileMapSlct = 0b01000000;     // bit 6; 0 -> 0x9800-0x9BFF. 1 -> 0x9C00-0x9FFF
  static const u8 kMaskWndwDisp = 0b00100000;            // bit 5; 0 -> off, 1 -> on
  static const u8 kMaskBgWndwTileDataSlct = 0b00010000;  // bit 4: 0 -> 0x8800-0x97FF, 1 -> 0x8000-0x8FFF
  static const u8 kMaskBgTileSlct = 0b00001000;          // bit 3: 0 -> 0x9800-0x9BFF, 1 -> 0x9C00-0x9FFF
  static const u8 KMaskObjSpriteSize = 0b00000100;       // bit 2: 0 -> 8*8, 1 -> 8*16
  static const u8 kMaskObjSpriteDisp = 0b00000010;       // bit 1: 0 -> off, 1 -> on
  static const u8 kMaskBgWndwDisp = 0b00000001;          // bit 0: 0 -> off, 1 -> on

  // Masks for sprite OAM entry.
  static const u8 kMaskSpritePriority = 0b10000000;  // bit 7: 0 -> on top of background 1 -> behind background
  static const u8 kMaskSpriteYFlip = 0b01000000;     // bit 6: flip y if 1
  static const u8 kMaskSpriteXFlip = 0b00100000;     // bit 5: flip x if 1
  static const u8 kMaskSpriteXPalNum = 0b00010000;   // bit 4: sprite palette number

  // Addresses.
  static const u16 kAdrRegLcdc = 0xFF40;
  static const u16 kAdrRegStat = 0xFF41;
  static const u16 kAdrRegScrollY = 0xFF42;
  static const u16 kAdrRegScrollX = 0xFF43;
  static const u16 kAdrRegLcdcY = 0xFF44;
  static const u16 kAdrRegLyComp = 0xFF45;
  static const u16 kAdrRegDma = 0xFF46;
  static const u16 kAdrRegBgp = 0xFF47;
  static const u16 kAdrRegObp0 = 0xFF48;
  static const u16 kAdrRegObp1 = 0xFF49;
  static const u16 kAdrRegWndwY = 0xFF4A;
  static const u16 kAdrRegWndwX = 0xFF4B;
  static const u16 kAdrTilemapLow = 0x9800;
  static const u16 kAdrTilemapHigh = 0x9C00;
  static const u16 kAdrTiledataLow = 0x8800;
  static const u16 kAdrTiledataHigh = 0x9C00;

  const u8 kMaskVBlankIE = 0b0001;
  const u8 kMaskLcdcStatIf = 0b0010;
  const u8 kMaskTimerIf = 0b0100;
  const u8 kMaskSerialIoIf = 0b1000;

  // User interfaces.
  static bool uiRenderBg;
  static bool uiRenderSprites;
  static bool uiRenderWndw;

  explicit Ppu(sc_module_name name, bool headless = false, int fps_cap = 60);
  ~Ppu();

  // PPU IO registers.
  u8 *reg_lcdc;  // LCDC flags.
  u8 *reg_stat;  // STAT flags.
  u8 *reg_scroll_y;  // Y scroll background.
  u8 *reg_scroll_x;  // X scroll background
  u8 *reg_lcdc_y;  // LCDC Y coordinate.
  u8 *reg_ly_comp;  // LY compare
  u8 *reg_dma;  // Direct memory access.
  u8 *reg_bgp;  // Background and window palette data.
  u8 *reg_obp_0;  // Object palette 0 data.
  u8 *reg_obp_1;  // Object palette 1 data.
  u8 *reg_wndw_y;  // Window y position.
  u8 *reg_wndw_x;  // Window x position.
  u8 *reg_ie;  // Interrupt enable.
  u8 *reg_intr_pending_dmi;

  u8 *tile_data_table_low;  // Lower tile data table: 0x8000-0x8FFF (overlaps with upper!).
  u8 *tile_data_table_up;  // Upper tile data table from 0x8800-0x97FF (overlaps with lower!).

  u8 *tile_map_low;  // Lower background and window tile map: 0x9800-0x9BFF.
  u8 *tile_map_up;  // Upper background and window tile map: 0x9C00-0x9FFF.

  u8 *oam_table;  // Object Attribute Memory (OAM) has 40x4 Byte blocks residing at 0xFE00-0xFE9F.

  u8 bg_buffer[kGbScreenBufferHeight][kGbScreenBufferWidth];
  u8 sprite_buffer[kGbScreenHeight][kGbScreenWidth];
  u8 window_buffer[kGbScreenBufferHeight][kGbScreenBufferWidth];

  void CheckLycInterrupt();
  void DrawBgToLine(int line_num);
  void DrawSpriteToLine(int line_num);
  void DrawWndwToLine(int line_num);
  void RenderLoop();

  string StateStr();

  // SystemC interfaces.
  tlm_utils::simple_initiator_socket<Ppu, gb_const::kBusDataWidth> init_socket;
  sc_in_clk clk;
  void start_of_simulation() override;

  class RenderWindow {
   public:
    RenderWindow(int width, int height, int log_width, int log_height, const char* title);
    RenderWindow();
    virtual ~RenderWindow();

    virtual void SaveScreenshot(const std::filesystem::path &file_path);
    virtual void DrawToScreen(Ppu &p) = 0;

   protected:
    SDL_Renderer *renderer;
    SDL_Window *window;
    int width;
    int height;
    int log_width;  // Logical width.
    int log_height;  // Logical height.
    string title;
  };

  std::unique_ptr<RenderWindow> game_wndw;
  std::unique_ptr<RenderWindow> window_wndw;

  // This the main window.
  class GameWindow : public RenderWindow {
   public:
    GameWindow(int width, int height, int log_width, int log_height, const char* title, int fps_cap);
    void DrawToScreen(Ppu &ppu) override;
   protected:
    int fps_cap;
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
    DummyWindow() : RenderWindow() {}
    void DrawToScreen(Ppu &ppu [[maybe_unused]]) override{};  // Do nothing.
    void SaveScreenshot(const std::filesystem::path &file_path [[maybe_unused]]) override{};  // Do nothing.
  };

 private:
  int window_line_;  // Internal window line counter.
};
