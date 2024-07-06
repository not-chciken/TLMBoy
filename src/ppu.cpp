/*******************************************************************************
 * Apache License, Version 2.0
 * Copyright (c) 2023 chciken/Niko
 ******************************************************************************/

#include "ppu.h"

#include <chrono>
#include <format>
#include <memory>
#include <ranges>
#include <thread>
#include <utility>
#include <vector>

bool Ppu::uiRenderBg = true;
bool Ppu::uiRenderSprites = true;
bool Ppu::uiRenderWndw = true;

// Interleaves two selected bits of two bit vectors and arranges them in a screen buffer friendly way.
// example a=0b00001000, b=00000000, pos=3, returns 0b00000010
// pos e [0,7], return value is always e[0,3]
constexpr u8 InterleaveBits(u8 a, u8 b, uint pos) {
  const u8 mask = 1u << pos;
  u8 res = (a & mask) == mask;
  res |= (b & mask) == mask ? 2 : 0;
  return res;
}

// Map "val" according to color palette given in "reg".
constexpr u8 MapColors(u8 val, u8 const *reg) { return (*reg >> val * 2) & 0b11; }

Ppu::Ppu(sc_module_name name, bool headless, int fps_cap) : sc_module(name), init_socket("init_socket"), clk("clk") {
  SC_CTHREAD(RenderLoop, clk);

  memset(bg_buffer, 0, kGbScreenBufferHeight * kGbScreenBufferWidth);
  memset(sprite_buffer, 0, kGbScreenWidth * kGbScreenHeight);
  memset(window_buffer, 0, kGbScreenBufferHeight * kGbScreenBufferWidth);

  if (headless) {
    game_wndw = std::make_unique<DummyWindow>();
    window_wndw = std::make_unique<DummyWindow>();
  } else {
    // Need typecast to create some temporaries with addresses for unique pointer reference arguments :/
    game_wndw = std::make_unique<GameWindow>((int)kRenderWndwWidth, (int)kRenderWndwHeight, (int)kGbScreenWidth,
                                             (int)kGbScreenHeight, "TLMBoy", fps_cap);
    window_wndw = std::make_unique<WindowWindow>(128 * 2, 128 * 2, 128, 128, "Tile Data Table");
  }
}

Ppu::~Ppu() {}

void Ppu::start_of_simulation() {
  tlm::tlm_dmi dmi_data;
  u8 *data_ptr;
  uint dummy;
  auto payload = MakeSharedPayloadPtr(tlm::TLM_READ_COMMAND, 0x0FFF, reinterpret_cast<void *>(&dummy));
  payload->set_address(0xFF40);  //  Start of IO registers.

  if (init_socket->get_direct_mem_ptr(*payload, dmi_data)) {
    assert(dmi_data.get_start_address() == 0);
    assert(dmi_data.get_end_address() >= 0x0B);
    data_ptr = reinterpret_cast<u8 *>(dmi_data.get_dmi_ptr());
    reg_lcdc = &data_ptr[0xFF40 - 0xFF40];
    reg_stat = &data_ptr[0xFF41 - 0xFF40];
    reg_scroll_y = &data_ptr[0xFF42 - 0xFF40];
    reg_scroll_x = &data_ptr[0xFF43 - 0xFF40];
    reg_lcdc_y = &data_ptr[0xFF44 - 0xFF40];
    reg_ly_comp = &data_ptr[0xFF45 - 0xFF40];
    reg_dma = &data_ptr[0xFF46 - 0xFF40];
    reg_bgp = &data_ptr[0xFF47 - 0xFF40];
    reg_obp_0 = &data_ptr[0xFF48 - 0xFF40];
    reg_obp_1 = &data_ptr[0xFF49 - 0xFF40];
    reg_wndw_y = &data_ptr[0xFF4A - 0xFF40];
    reg_wndw_x = &data_ptr[0xFF4B - 0xFF40];
    *reg_lcdc_y = 0;
  } else {
    throw std::runtime_error("Could not get DMI for IO registers!");
  }

  payload->set_address(0x8000);  // Start of video RAM.
  if (init_socket->get_direct_mem_ptr(*payload, dmi_data)) {
    assert(dmi_data.get_start_address() == 0);
    assert(dmi_data.get_end_address() >= 0x1C00);
    data_ptr = reinterpret_cast<u8 *>(dmi_data.get_dmi_ptr());
    tile_data_table_low = &data_ptr[0x8000 - 0x8000];
    tile_data_table_up = &data_ptr[0x8800 - 0x8000];
    tile_map_low = &data_ptr[0x9800 - 0x8000];
    tile_map_up = &data_ptr[0x9C00 - 0x8000];
  } else {
    throw std::runtime_error("Could not get DMI for video RAM!");
  }

  payload->set_address(0xFE00);  // Start of OAM table.
  if (init_socket->get_direct_mem_ptr(*payload, dmi_data)) {
    assert(dmi_data.get_start_address() == 0);
    data_ptr = reinterpret_cast<u8 *>(dmi_data.get_dmi_ptr());
    oam_table = &data_ptr[0xFE00 - 0xFE00];
  } else {
    throw std::runtime_error("Could not get DMI for OAM table!");
  }

  payload->set_address(0xFFFF);  // Interrupt register.
  if (init_socket->get_direct_mem_ptr(*payload, dmi_data)) {
    assert(dmi_data.get_start_address() == 0);
    data_ptr = reinterpret_cast<u8 *>(dmi_data.get_dmi_ptr());
    reg_ie = &data_ptr[0xFFFF - 0xFFFF];
  } else {
    throw std::runtime_error("Could not get DMI for the interrupt register!");
  }

  payload->set_address(0xFF0F);  // Interrupt pending register.
  if (init_socket->get_direct_mem_ptr(*payload, dmi_data)) {
    assert(dmi_data.get_start_address() == 0);
    reg_intr_pending_dmi = reinterpret_cast<u8 *>(dmi_data.get_dmi_ptr());
  } else {
    throw std::runtime_error("Could not get DMI for interrupt pending register!");
  }
}

void Ppu::DrawBgToLine(int line_num) {
  u8 *tile_data_table;
  u8 *bg_tile_map;

  tile_data_table = (*reg_lcdc & kMaskBgWndwTileDataSlct) ? tile_data_table_low : tile_data_table_up;
  bg_tile_map = (*reg_lcdc & kMaskBgTileSlct) ? tile_map_up : tile_map_low;

  const int y_tile_pixel = (*reg_scroll_y + line_num) % 8;
  if (*reg_lcdc & kMaskBgWndwDisp) {
    for (int i = 0; i < kGbScreenWidth; ++i) {
      int bg_tile_ind = 32 * (((line_num + *reg_scroll_y) % 256) / 8) + ((*reg_scroll_x + i) % 256) / 8;
      u8 tile_ind = bg_tile_map[bg_tile_ind];
      tile_ind += tile_data_table == tile_data_table_up ? 128 : 0;  // Using wraparound.
      int x_tile_pixel = (*reg_scroll_x + i) % 8;
      int pixel_ind = static_cast<int>(tile_ind) * kBytesPerTile + 2 * y_tile_pixel;
      u8 res = InterleaveBits(tile_data_table[pixel_ind], tile_data_table[pixel_ind + 1], 7 - x_tile_pixel);
      bg_buffer[line_num][i] = MapColors(res, reg_bgp);
      assert(bg_tile_ind < 1024);
    }
  } else {
    for (int i = 0; i < kGbScreenWidth; ++i) {
      bg_buffer[line_num][i] = 0;
    }
  }
}

void Ppu::DrawWndwToLine(int line_num) {
  int x_pos = *reg_wndw_x - 7;
  int y_pos = *reg_wndw_y;

  if (!(*reg_lcdc & kMaskWndwDisp) || (y_pos >= kGbScreenHeight) || (x_pos >= kGbScreenWidth) || (line_num < y_pos))
    return;

  u8 *tile_data_table;
  u8 *wndw_tile_map;
  tile_data_table = (*reg_lcdc & kMaskBgWndwTileDataSlct) ? tile_data_table_low : tile_data_table_up;
  wndw_tile_map = (*reg_lcdc & kMaskWndwTileMapSlct) ? tile_map_up : tile_map_low;

  const int y_tile_pixel = window_line_ % 8;
  if (*reg_lcdc & kMaskBgWndwDisp) {
    for (int i = 0; i < kGbScreenWidth; ++i) {
      if ((i - x_pos) < 0) {
        continue;
      }

      int wndw_tile_ind = 32 * (window_line_ / 8) + (i - x_pos) / 8;
      u8 tile_ind = wndw_tile_map[wndw_tile_ind];
      tile_ind += tile_data_table == tile_data_table_up ? 128 : 0;  // Using wraparound.
      int x_tile_pixel = (i - x_pos) % 8;
      int pixel_ind = static_cast<int>(tile_ind) * kBytesPerTile + 2 * y_tile_pixel;
      int res = InterleaveBits(tile_data_table[pixel_ind], tile_data_table[pixel_ind + 1], 7 - x_tile_pixel);
      bg_buffer[line_num][i] = MapColors(res, reg_bgp);
      assert(wndw_tile_ind < 1024);
    }
  } else {
    for (int i = 0; i < kGbScreenWidth; ++i) {
      bg_buffer[line_num][i] = 0;
    }
  }
  ++window_line_;
}

void Ppu::DrawSpriteToLine(int line_num) {
  if (!(kMaskObjSpriteDisp & *reg_lcdc)) return;

  std::vector<std::pair<int, int>> sorted_oam;
  for (int i = 0; i < kNumOamEntries; ++i) {
    int x_pos = oam_table[i * kOamEntryBytes + 1];
    int pos_y = oam_table[i * kOamEntryBytes] - 16;
    int pos_x = oam_table[i * kOamEntryBytes + 1] - 8;
    bool is_big_sprite = static_cast<bool>(KMaskObjSpriteSize & *reg_lcdc);
    int sprite_height = is_big_sprite ? 16 : 8;  // Width of a sprite in pixels.

    if (((pos_y > line_num) || ((pos_y + sprite_height) <= line_num)) || ((pos_x < -7) || (pos_x >= kGbScreenWidth)))
      continue;

    sorted_oam.push_back(std::make_pair(i, x_pos));
  }

  std::stable_sort(sorted_oam.begin(), sorted_oam.end(),
                   [](std::pair<int, int> a, std::pair<int, int> b) { return a.second < b.second; });
  if (sorted_oam.size() > 10) sorted_oam.resize(10);  // Maximum 10 sprites per line.

  u8 *tile_data_table = tile_data_table_low;  // Sprites always use the low data table.
  for (auto &p : std::ranges::views::reverse(sorted_oam)) {
    int i = p.first;
    int pos_y = oam_table[i * kOamEntryBytes] - 16;           // byte0 = y pos
    int pos_x = oam_table[i * kOamEntryBytes + 1] - 8;        // byte1 = x pos
    int sprite_tile_ind = oam_table[i * kOamEntryBytes + 2];  // byte2 = tile index
    int sprite_flags = oam_table[i * kOamEntryBytes + 3];     // byte3 = flags
    bool is_big_sprite = static_cast<bool>(KMaskObjSpriteSize & *reg_lcdc);
    bool palette = IsBitSet(sprite_flags, 4);
    bool x_flip = IsBitSet(sprite_flags, 5);
    bool y_flip = IsBitSet(sprite_flags, 6);
    bool obj_prio = IsBitSet(sprite_flags, 7);

    if (is_big_sprite) {
      sprite_tile_ind &= 0xFE;  // Ignore the last bit in 8x16 mode.
    }

    int y_tile_pixel = y_flip ? (is_big_sprite ? 15 : 7) - (line_num - pos_y) : line_num - pos_y;
    int pixel_ind = sprite_tile_ind * 16 + y_tile_pixel * 2;

    for (int j = 0; j < 8; ++j) {
      int x_draw = (pos_x + j);
      if (x_draw < 0 || x_draw >= kGbScreenWidth) {
        continue;
      }

      u32 res = InterleaveBits(tile_data_table[pixel_ind], tile_data_table[pixel_ind + 1], (x_flip ? j : 7 - j));
      if (res == 0) {
        continue;  // Color 0 is transparent.
      }

      if (obj_prio) {
        if (bg_buffer[line_num][x_draw] == 0) {
          bg_buffer[line_num][x_draw] = MapColors(res, palette ? reg_obp_1 : reg_obp_0);
        }
      } else {
        sprite_buffer[line_num][x_draw] = MapColors(res, palette ? reg_obp_1 : reg_obp_0);
      }
    }
  }
}

void Ppu::CheckLycInterrupt() {
  bool ly_coinc_interrupt = *reg_stat & gb_const::kMaskBit6;
  bool ly_coinc = *reg_ly_comp == *reg_lcdc_y;

  if (ly_coinc_interrupt && ly_coinc) {
    *reg_intr_pending_dmi |= kMaskLcdcStatIf;
  }

  SetBit(reg_stat, ly_coinc, 2);
}

// A complete screen refresh occurs every 70224 cycles.
void Ppu::RenderLoop() {
  while (1) {
    for (int i = 0; i < kGbScreenHeight; ++i) {
      // Mode = OAM-search (10).
      SetBit(reg_stat, false, 0);
      SetBit(reg_stat, true, 1);
      wait(80);

      // Mode = LCD transfer (11)
      SetBit(reg_stat, true, 0);
      wait(168);

      // Mode = H-Blank (00).
      SetBit(reg_stat, false, 0);
      SetBit(reg_stat, false, 1);

      if (uiRenderBg) {
        DrawBgToLine(i);
      }
      if (uiRenderWndw) {
        DrawWndwToLine(i);
      }
      if (uiRenderSprites) {
        DrawSpriteToLine(i);
      }

      ++(*reg_lcdc_y);

      if (*reg_stat & gb_const::kMaskBit3) {
        *reg_intr_pending_dmi |= kMaskLcdcStatIf;
      }

      CheckLycInterrupt();
      wait(208);
    }
    window_line_ = 0;

    // Mode = V-Blank (01)
    SetBit(reg_stat, true, 0);
    game_wndw->DrawToScreen(*this);
    window_wndw->DrawToScreen(*this);
    DBG_LOG_PPU(std::endl << StateStr());
    *reg_intr_pending_dmi |= kMaskVBlankIE;  // V-Blank interrupt.

    for (int i = 0; i < 10; ++i) {
      wait(456);  // The vblank period is 4560 cycles.
      ++(*reg_lcdc_y);
      CheckLycInterrupt();
    }
    *reg_lcdc_y = 0;
    CheckLycInterrupt();
  }
}

string Ppu::StateStr() {
  std::stringstream ss;
  ss << "#### PPU State ####" << std::dec << std::endl
     << "LCD control: " << static_cast<bool>(*reg_lcdc & kMaskLcdControl) << std::endl
     << "V-blank intr enabled: " << static_cast<bool>(*reg_ie & kMaskVBlankIE) << std::endl
     << "tile data select: " << static_cast<bool>(*reg_lcdc & kMaskBgWndwTileDataSlct) << std::endl
     << "bg tile map select: " << static_cast<bool>(*reg_lcdc & kMaskBgTileSlct) << std::endl
     << "wndw tile map select: " << static_cast<bool>(*reg_lcdc & kMaskWndwTileMapSlct) << std::endl
     << "LCD_Y: " << static_cast<uint>(*reg_lcdc_y) << std::endl
     << "LY_COMP: " << static_cast<uint>(*reg_ly_comp) << std::endl
     << "LYC Interrupt: " << static_cast<bool>(*reg_stat & gb_const::kMaskBit6) << std::endl
     << "rBGP: " << std::bitset<8>(*reg_bgp) << std::endl
     << "scroll x: " << static_cast<uint>(*reg_scroll_x) << std::endl
     << "scroll y: " << static_cast<uint>(*reg_scroll_y) << std::endl;
  return ss.str();
}

Ppu::RenderWindow::RenderWindow(int width, int height, int log_width, int log_height, const char *title)
    : width(width), height(height), log_width(log_width), log_height(log_height), title(title) {
  SDL_Init(SDL_INIT_VIDEO);
  SDL_CreateWindowAndRenderer(width, height, 0, &window, &renderer);
  SDL_SetWindowTitle(window, title);
  SDL_RenderSetLogicalSize(renderer, log_width, log_height);
  SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
  SDL_RenderClear(renderer);
  SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);

  auto icon = SDL_LoadBMP("./tlmboy_icon.bmp");
  if (icon) {
    SDL_SetColorKey(icon, true, SDL_MapRGB(icon->format, 0, 0, 0));
    SDL_SetWindowIcon(window, icon);
    SDL_FreeSurface(icon);
  }

  texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, log_width, log_height);
}

Ppu::RenderWindow::RenderWindow() {}

Ppu::RenderWindow::~RenderWindow() {
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();
}

void Ppu::RenderWindow::SaveScreenshot(const std::filesystem::path &file_path) {
  const Uint32 format = SDL_PIXELFORMAT_ARGB8888;
  SDL_Surface *surface = SDL_CreateRGBSurfaceWithFormat(0, width, height, 32, format);
  SDL_RenderReadPixels(renderer, NULL, format, surface->pixels, surface->pitch);
  SDL_SaveBMP(surface, file_path.string().c_str());
  SDL_FreeSurface(surface);
}

Ppu::GameWindow::GameWindow(int width, int height, int log_width, int log_height, const char *title, int fps_cap)
    : RenderWindow(width, height, log_width, log_height, title), fps_cap(fps_cap) {}

constexpr u32 ToTextureColor(u8 r, u8 g, u8 b) { return ((u32)r << 16) | ((u32)g << 8) | (u32)b; }

void Ppu::GameWindow::DrawToScreen(Ppu &p) {
  static u64 last_time = 0;

  u64 new_time = SDL_GetTicks64();
  u64 delta_time = new_time - last_time;
  int fps = 1000 / delta_time;

  if (fps > fps_cap) {
    int target_delta = 1000 / fps_cap;
    int sleep_time = target_delta - delta_time;
    std::this_thread::sleep_for(std::chrono::milliseconds(sleep_time));
    new_time = SDL_GetTicks64();
    fps = 1000 / (new_time - last_time);
  }

  last_time = new_time;

  SDL_SetWindowTitle(window, std::format("{}   FPS: {:03}", title, fps).c_str());

  // If the screen is off, just draw a red background.
  if (!((*p.reg_lcdc) & kMaskLcdControl)) {
    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
    SDL_RenderClear(renderer);
    return;
  }

  void *pixels_ptr;
  int pitch;
  SDL_LockTexture(texture, nullptr, &pixels_ptr, &pitch);
  u32 *pixels = static_cast<u32 *>(pixels_ptr);

  // Background and window loop.
  if (uiRenderBg) {
    for (int y = 0; y < log_height; ++y) {
      for (int x = 0; x < log_width; ++x) {
        int val = p.bg_buffer[y][x];
        pixels[y * log_width + x] = ToTextureColor(renderColor[val][0], renderColor[val][1], renderColor[val][2]);
      }
    }
  } else {
    for (int y = 0; y < log_height; ++y) {
      for (int x = 0; x < log_width; ++x) {
        pixels[y * log_width + x] = ToTextureColor(242, 255, 217);
      }
    }
  }

  // Sprite loop.
  if (uiRenderSprites) {
    for (int y = 0; y < log_height; ++y) {
      for (int x = 0; x < log_width; ++x) {
        int val = p.sprite_buffer[y][x];
        p.sprite_buffer[y][x] = 0;
        if (val == 0) continue;  // Continues if val is 0, This implements transparency!
        pixels[y * log_width + x] = ToTextureColor(renderColor[val][0], renderColor[val][1], renderColor[val][2]);
      }
    }
  }

  SDL_UnlockTexture(texture);
  SDL_RenderCopy(renderer, texture, nullptr, nullptr);
  SDL_RenderPresent(renderer);
}

void Ppu::WindowWindow::DrawToScreen(Ppu &p) {
  void *pixels_ptr;
  int pitch;
  SDL_LockTexture(texture, nullptr, &pixels_ptr, &pitch);
  u32 *pixels = static_cast<u32 *>(pixels_ptr);

  for (int t = 0; t < 16; ++t) {
    for (int i = 0; i < 8; ++i) {
      for (int j = 0; j < 16; ++j) {
        for (int k = 0; k < 8; ++k) {
          u8 val = InterleaveBits(p.tile_data_table_low[t * 256 + j * 16 + 2 * i],
                                  p.tile_data_table_low[t * 256 + j * 16 + 2 * i + 1], 7 - k);
          val = MapColors(val, p.reg_bgp);
          SDL_SetRenderDrawColor(renderer, renderColor[val][0], renderColor[val][1], renderColor[val][2], 255);
          const size_t x = j * 8 + k;
          const size_t y = t * 8 + i;
          pixels[y * log_width + x] = ToTextureColor(renderColor[val][0], renderColor[val][1], renderColor[val][2]);
        }
      }
    }
  }

  SDL_UnlockTexture(texture);
  SDL_RenderCopy(renderer, texture, nullptr, nullptr);
  SDL_RenderPresent(renderer);
}
