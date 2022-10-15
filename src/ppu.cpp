/*******************************************************************************
 * Copyright (C) 2021 chciken
 * MIT License
 ******************************************************************************/
#include <memory>

#include "ppu.h"

bool Ppu::uiRenderBg = true;
bool Ppu::uiRenderSprites = true;
bool Ppu::uiRenderWndw = true;

Ppu::Ppu(sc_module_name name, bool headless)
  : sc_module(name) ,
    init_socket("init_socket"),
    clk("clk") {
  SC_CTHREAD(RenderLoop, clk);
  memset(window_buffer, 0, kGbScreenBufferHeight*kGbScreenBufferWidth);
  memset(bg_buffer, 0, kGbScreenBufferHeight*kGbScreenBufferWidth);
  memset(sprite_buffer, 0, kGbScreenWidth*kGbScreenHeight);
  if (headless) {
    game_wndw = std::make_unique<DummyWindow>();
    window_wndw = std::make_unique<DummyWindow>();
  } else {
    // Need typecast to create some temporaries with addresses for unique pointer reference arguments :/
    game_wndw = std::make_unique<GameWindow>((u32)kRenderWndwWidth, (u32)kRenderWndwHeight,
                                             (u32)kGbScreenWidth, (u32)kGbScreenHeight);
    window_wndw = std::make_unique<WindowWindow>(128*2, 128*2, 128, 128);
  }
}

Ppu::~Ppu() {
}

void Ppu::start_of_simulation() {
  tlm::tlm_dmi dmi_data;
  u8 *data_ptr;
  uint dummy;
  auto payload = MakeSharedPayloadPtr(tlm::TLM_READ_COMMAND, 0x0FFF, reinterpret_cast<void*>(&dummy));
  payload->set_address(0xFF40);  //  start of IO registers
  if (init_socket->get_direct_mem_ptr(*payload, dmi_data)) {
    assert(dmi_data.get_start_address() == 0);
    assert(dmi_data.get_end_address() >= 0x0B);
    data_ptr = reinterpret_cast<u8*>(dmi_data.get_dmi_ptr());
    reg_0xFF40   = &data_ptr[0xFF40 - 0xFF40];
    reg_0xFF41   = &data_ptr[0xFF41 - 0xFF40];
    reg_scroll_y = &data_ptr[0xFF42 - 0xFF40];
    reg_scroll_x = &data_ptr[0xFF43 - 0xFF40];
    reg_lcdc_y   = &data_ptr[0xFF44 - 0xFF40];
    reg_ly_comp  = &data_ptr[0xFF45 - 0xFF40];
    reg_dma      = &data_ptr[0xFF46 - 0xFF40];
    reg_bgp      = &data_ptr[0xFF47 - 0xFF40];
    reg_obp_0    = &data_ptr[0xFF48 - 0xFF40];
    reg_obp_1    = &data_ptr[0xFF49 - 0xFF40];
    reg_wndw_y   = &data_ptr[0xFF4A - 0xFF40];
    reg_wndw_x   = &data_ptr[0xFF4B - 0xFF40];
    *reg_lcdc_y = 0;
  } else {
    throw std::runtime_error("Could not get DMI for IO registers!");
  }

  payload->set_address(0x8000);  // start of video ram
  if (init_socket->get_direct_mem_ptr(*payload, dmi_data)) {
    assert(dmi_data.get_start_address() == 0);
    assert(dmi_data.get_end_address() >= 0x1C00);
    data_ptr = reinterpret_cast<u8*>(dmi_data.get_dmi_ptr());
    tile_data_table_low = &data_ptr[0x8000 - 0x8000];
    tile_data_table_up  = &data_ptr[0x8800 - 0x8000];
    tile_map_low        = &data_ptr[0x9800 - 0x8000];
    tile_map_up         = &data_ptr[0x9C00 - 0x8000];
  } else {
    throw std::runtime_error("Could not get DMI for video RAM!");
  }

  payload->set_address(0xFE00);  // start of oam table
  if (init_socket->get_direct_mem_ptr(*payload, dmi_data)) {
    assert(dmi_data.get_start_address() == 0);
    data_ptr = reinterpret_cast<u8*>(dmi_data.get_dmi_ptr());
    oam_table = &data_ptr[0xFE00 - 0xFE00];
  } else {
    throw std::runtime_error("Could not get DMI for OAM table!");
  }

  payload->set_address(0xFFFF);  // interrupt register
  if (init_socket->get_direct_mem_ptr(*payload, dmi_data)) {
    assert(dmi_data.get_start_address() == 0);
    data_ptr = reinterpret_cast<u8*>(dmi_data.get_dmi_ptr());
    reg_ie = &data_ptr[0xFFFF - 0xFFFF];
  } else {
    throw std::runtime_error("Could not get DMI for the interrupt register!");
  }

  payload->set_address(0xFF0F);  // interrupt pending register
  if (init_socket->get_direct_mem_ptr(*payload, dmi_data)) {
    assert(dmi_data.get_start_address() == 0);
    reg_intr_pending_dmi = reinterpret_cast<u8*>(dmi_data.get_dmi_ptr());
  } else {
    throw std::runtime_error("Could not get DMI for interrupt pending register!");
  }
}

const uint Ppu::MapBgCols(const uint val) {
  return ((0b11 << val*2) & *reg_bgp) >> val*2;
}

void Ppu::DrawBgToLine(uint line_num) {
  u8* tile_data_table;
  u8* bg_tile_map;

  tile_data_table = (*reg_0xFF40 & kMaskBgWndwTileDataSlct) ?  tile_data_table_low : tile_data_table_up;
  bg_tile_map = (*reg_0xFF40 & kMaskBgTileSlct) ? tile_map_up : tile_map_low;

  const i32 y_tile_pixel = (*reg_scroll_y + line_num) % 8;
  if (*reg_0xFF40 & kMaskBgWndwDisp) {
    for (i32 i = 0; i < 160; ++i) {
      i32 bg_tile_ind = 32 * (((line_num + *reg_scroll_y) % 256) / 8) + ((*reg_scroll_x + i) % 256) / 8;
      u8 tile_ind = bg_tile_map[bg_tile_ind];
      if (tile_data_table == tile_data_table_up)
        tile_ind += 128;  // Using wraparound.
      i32 x_tile_pixel = (*reg_scroll_x + i) % 8;
      i32 pixel_ind = static_cast<i32>(tile_ind) * TILE_BYTES + 2 * y_tile_pixel;
      i32 res = InterleaveBits(tile_data_table[pixel_ind], tile_data_table[pixel_ind+1], 7-x_tile_pixel);
      bg_buffer[line_num][i] = res;
      assert(bg_tile_ind < 1024);
    }
  } else {
    for (i32 i = 0; i < 160; ++i) {
      bg_buffer[line_num][i] = 0;
    }
  }
}

void Ppu::DrawWndwToLine(int line_num) {
  int x_pos = *reg_wndw_x - 7;
  int y_pos = *reg_wndw_y;

  if (!(*reg_0xFF40 & kMaskWndwDisp)
      || (y_pos >= kGbScreenHeight)
      || (x_pos >= kGbScreenWidth)
      || (line_num < y_pos))
    return;

  u8* tile_data_table;
  u8* wndw_tile_map;
  tile_data_table = (*reg_0xFF40 & kMaskBgWndwTileDataSlct) ?  tile_data_table_low : tile_data_table_up;
  wndw_tile_map = (*reg_0xFF40 & kMaskWndwTileMapSlct) ? tile_map_up : tile_map_low;

  const i32 y_tile_pixel = window_line_ % 8;
  if (*reg_0xFF40 & kMaskBgWndwDisp) {
    for (i32 i = 0; i < 160; ++i) {
      if ((i - x_pos) < 0)
        continue;
      i32 wndw_tile_ind = 32 * (window_line_ / 8) + (i - x_pos) / 8;
      u8 tile_ind = wndw_tile_map[wndw_tile_ind];
      if (tile_data_table == tile_data_table_up)
        tile_ind += 128;  // Using wraparound.
      i32 x_tile_pixel = (i - x_pos) % 8;
      i32 pixel_ind = static_cast<i32>(tile_ind) * TILE_BYTES + 2 * y_tile_pixel;
      i32 res = InterleaveBits(tile_data_table[pixel_ind], tile_data_table[pixel_ind+1], 7-x_tile_pixel);
      window_buffer[line_num][i] = res;
      assert(wndw_tile_ind < 1024);
    }
  } else {
    for (i32 i = 0; i < 160; ++i) {
      window_buffer[line_num][i] = 0;
    }
  }
  ++window_line_;
}

void Ppu::DrawSpriteToLine(int line_num) {
  if (!(kMaskObjSpriteDisp & *reg_0xFF40))
    return;

  int num_rendered_sprites = 0;  // Stop at a maximum of 10 sprites per line.
  u8* tile_data_table = tile_data_table_low;  // Sprites always use the low data table.
  for (int i = kNumOamEntries - 1; i >= 0; --i) {
    int pos_y            = oam_table[i*kOamEntryBytes] - 16;      // byte0 = y pos
    int pos_x            = oam_table[i*kOamEntryBytes + 1] - 8;   // byte1 = x pos
    int sprite_tile_ind  = oam_table[i*kOamEntryBytes + 2];       // byte2 = tile index
    int sprite_flags     = oam_table[i*kOamEntryBytes + 3];       // byte3 = flags //TODO(niko)
    const bool palette = IsBitSet(sprite_flags, 4);
    const bool x_flip = IsBitSet(sprite_flags, 5);
    const bool y_flip = IsBitSet(sprite_flags, 6);
    const bool obj_prio = IsBitSet(sprite_flags, 7);

    const bool is_big_sprite = static_cast<bool>(KMaskObjSpriteSize & *reg_0xFF40);
    const int sprite_height = is_big_sprite ? 16 : 8;  // Width of a sprite in pixels.

    if (((pos_y > line_num) || ((pos_y + sprite_height) <= line_num))
        || ((pos_x < -7) || (pos_x >= kGbScreenWidth))
        || (num_rendered_sprites >= 10))
    continue;
    ++num_rendered_sprites;

    if (is_big_sprite) {
      sprite_tile_ind &= 0xFE;  // Ignore the last bit in 8x16 mode.
    }

    const int y_tile_pixel = y_flip ? (is_big_sprite ? 15 : 7) - (line_num - pos_y)
                                    : line_num - pos_y;
    i32 pixel_ind = sprite_tile_ind * 16 + y_tile_pixel * 2;

    for (int j = 0; j < 8; ++j) {
      int x_draw = (pos_x + j);
      if (x_draw < 0 || x_draw >= kGbScreenWidth)
          continue;

      u32 res = InterleaveBits(tile_data_table[pixel_ind], tile_data_table[pixel_ind + 1], (x_flip ? j : 7 - j));
      if (res == 0)  // Color 0 is transparent.
          continue;
      sprite_buffer[line_num][x_draw] = res;
    }
  }
}

void Ppu::CheckLycInterrupt() {
  bool ly_coinc_interrupt = *reg_0xFF41 & gb_const::kMaskBit6;
  bool ly_coinc = *reg_ly_comp == *reg_lcdc_y;
  if (ly_coinc_interrupt && ly_coinc) {
    *reg_intr_pending_dmi |= kMaskLcdcStatIf;
  }
  SetBit(reg_0xFF41, ly_coinc, 2);
}

// A complete screen refresh occurs every 70224 cycles.
void Ppu::RenderLoop() {
  while (1) {
    for (int i = 0; i < 144; ++i) {
      // Mode = OAM-search (10).
      SetBit(reg_0xFF41, false, 0);
      SetBit(reg_0xFF41, true, 1);
      wait(80);

      // Mode = LCD transfer (11)
      SetBit(reg_0xFF41, true, 0);
      wait(168);

      // Mode = H-Blank (00).
      SetBit(reg_0xFF41, false, 0);
      SetBit(reg_0xFF41, false, 1);
      DrawBgToLine(i);
      DrawSpriteToLine(i);
      DrawWndwToLine(i);
      ++(*reg_lcdc_y);
      if (*reg_0xFF41 & gb_const::kMaskBit3) {
        *reg_intr_pending_dmi |= kMaskLcdcStatIf;
      }
      CheckLycInterrupt();
      wait(208);
    }
    window_line_ = 0;

    // Mode = V-Blank (01)
    SetBit(reg_0xFF41, true, 0);
    game_wndw->DrawToScreen(*this);
    window_wndw->DrawToScreen(*this);
    DBG_LOG_PPU(std::endl << PpuStateStr());
    *reg_intr_pending_dmi |= kMaskVBlankIE;  // V-Blank interrupt.

    for (int i = 0; i < 10; ++i) {
      ++(*reg_lcdc_y);
      CheckLycInterrupt();
      wait(456);  // The vblank period is 4560 cycles.
    }
    *reg_lcdc_y = 0;
    CheckLycInterrupt();
  }
}

std::string Ppu::PpuStateStr() {
  std::stringstream ss;
  ss << "#### PPU State ####" << std::dec <<std::endl
    << "LCD control: "          << static_cast<bool>(*reg_0xFF40 & kMaskLcdControl) << std::endl
    << "V-blank intr enabled: " << static_cast<bool>(*reg_ie & kMaskVBlankIE) << std::endl
    << "tile data select: "     << static_cast<bool>(*reg_0xFF40 & kMaskBgWndwTileDataSlct) << std::endl
    << "bg tile map select: "   << static_cast<bool>(*reg_0xFF40 & kMaskBgTileSlct) << std::endl
    << "wndw tile map select: " << static_cast<bool>(*reg_0xFF40 & kMaskWndwTileMapSlct) << std::endl
    << "LCD_Y: "    << static_cast<uint>(*reg_lcdc_y) << std::endl
    << "LY_COMP: "  << static_cast<uint>(*reg_ly_comp) << std::endl
    << "LYC Interrupt: " << static_cast<bool>(*reg_0xFF41 & gb_const::kMaskBit6) << std::endl
    << "rBGP: "     << std::bitset<8>(*reg_bgp) << std::endl
    << "scroll x: " << static_cast<uint>(*reg_scroll_x) << std::endl
    << "scroll y: " << static_cast<uint>(*reg_scroll_y) << std::endl;
  return ss.str();
}

const u8 Ppu::InterleaveBits(u8 a, u8 b, const uint pos) {
  u8 mask = 0b00000001 << pos;
  a &= mask;
  b &= mask;
  a = (pos ? a >> (pos-1) : a << 1);
  b = b >> pos;
  return a | b;
}

Ppu::RenderWindow::RenderWindow(const uint width, const uint height, const uint log_width, const uint log_height)
    : width(width), height(height), log_width(log_width), log_height(log_height) {
  SDL_Init(SDL_INIT_VIDEO);
  SDL_CreateWindowAndRenderer(width, height, 0, &window, &renderer);
  SDL_RenderSetLogicalSize(renderer, log_width, log_height);
  SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
  SDL_RenderClear(renderer);
  SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
}

Ppu::RenderWindow::RenderWindow() {}

Ppu::RenderWindow::~RenderWindow() {
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();
}

void Ppu::RenderWindow::SaveScreenshot(const std::filesystem::path file_path) {
  const Uint32 format = SDL_PIXELFORMAT_ARGB8888;
  SDL_Surface *surface = SDL_CreateRGBSurfaceWithFormat(0, width, height, 32, format);
  SDL_RenderReadPixels(renderer, NULL, format, surface->pixels, surface->pitch);
  SDL_SaveBMP(surface, file_path.string().c_str());
  SDL_FreeSurface(surface);
}

void Ppu::GameWindow::DrawToScreen(Ppu &p) {
  int val;

  // If the screen is off, just draw a red background.
  if (!((*p.reg_0xFF40) & kMaskLcdControl)) {
    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
    SDL_RenderClear(renderer);
    return;
  }

  // Background loop
  if (uiRenderBg) {
    for (uint j = 0; j < log_height; ++j) {
      for (uint i = 0; i < log_width; ++i) {
        val = p.bg_buffer[j][i];
        val = p.MapBgCols(val);
        SDL_SetRenderDrawColor(renderer,
                              renderColor[val][0],
                              renderColor[val][1],
                              renderColor[val][2], 255);
        SDL_RenderDrawPoint(renderer, i, j);
      }
    }
  } else {
    SDL_SetRenderDrawColor(renderer, 242, 255, 217, 255);
    for (uint j = 0; j < log_height; ++j)
      for (uint i = 0; i < log_width; ++i)
        SDL_RenderDrawPoint(renderer, i, j);
  }

  // Window loop
  if (uiRenderWndw) {
    if ((*p.reg_0xFF40 & kMaskWndwDisp) && (*p.reg_0xFF40 & kMaskBgWndwDisp)) {
      for (uint j = *p.reg_wndw_y; j < kGbScreenHeight; ++j) {
        for (uint i = *p.reg_wndw_x; i < kGbScreenWidth; ++i) {
          val = p.window_buffer[j][i];
          val = p.MapBgCols(val);
          SDL_SetRenderDrawColor(renderer,
                                renderColor[val][0],
                                renderColor[val][1],
                                renderColor[val][2], 255);
          SDL_RenderDrawPoint(renderer, i, j);
        }
      }
    }
  }

  // Sprite loop
  if (uiRenderSprites) {
    for (uint j = 0; j < kGbScreenHeight; ++j) {
      for (uint i = 0; i < kGbScreenWidth; ++i) {
        val = p.sprite_buffer[j][i];
        p.sprite_buffer[j][i] = 0;
        if (val == 0)
          continue;  // Continues if val is 0, This implements transparancy!
        SDL_SetRenderDrawColor(renderer,
                              renderColor[val][0],
                              renderColor[val][1],
                              renderColor[val][2], 255);
        SDL_RenderDrawPoint(renderer, i, j);
      }
    }
  }

  SDL_RenderPresent(renderer);
}

// TODO(me) render both, higher and lower, tile maps
void Ppu::WindowWindow::DrawToScreen(Ppu &p) {
  for (uint t = 0; t < 16; ++t) {
    for (uint i = 0; i < 8; ++i) {
      for (uint j = 0; j < 16; ++j) {
        for (uint k = 0; k < 8; ++k) {
          uint val = p.InterleaveBits(p.tile_data_table_low[t*256+j*16+2*i],
                                      p.tile_data_table_low[t*256+j*16+2*i+1], 7-k);
          val = p.MapBgCols(val);
          SDL_SetRenderDrawColor(renderer,
                               renderColor[val][0],
                               renderColor[val][1],
                               renderColor[val][2], 255);
          SDL_RenderDrawPoint(renderer, j*8 + k , t*8 + i);
        }
      }
    }
  }
  SDL_RenderPresent(renderer);
}
