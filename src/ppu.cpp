/*******************************************************************************
 * Copyright (C) 2021 chciken
 * MIT License
 ******************************************************************************/
#include "ppu.h"

Ppu::Ppu(sc_module_name name)
  : sc_module(name) ,
    init_socket("init_socket"),
    clk("clk"),
    game_wndw(kRenderWndwWidth, kRenderWndwHeight, kGbScreenWidth, kGbScreenHeight),
    window_wndw(128*2, 128*2, 128, 128) {
  SC_THREAD(RenderLoop);
  sensitive << clk.pos();
  memset(window_buffer, 0, SCREEN_BUFFER_WIDTH*SCREEN_BUFFER_HEIGHT);  // TODO(niko): Does the gameboy really start at 0?
  memset(bg_buffer, 0, SCREEN_BUFFER_WIDTH*SCREEN_BUFFER_HEIGHT);
  memset(sprite_buffer, 0, kGbScreenWidth*kGbScreenHeight);
}

Ppu::~Ppu() {
}

void Ppu::InitRegisters() {
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
    assert(dmi_data.get_end_address() >= 0);
    data_ptr = reinterpret_cast<u8*>(dmi_data.get_dmi_ptr());
    oam_table = &data_ptr[0xFE00 - 0xFE00];
  } else {
    throw std::runtime_error("Could not get DMI for OAM table!");
  }

  payload->set_address(0xFFFF);  // interrupt register
  if (init_socket->get_direct_mem_ptr(*payload, dmi_data)) {
    assert(dmi_data.get_start_address() == 0);
    assert(dmi_data.get_end_address() >= 0);
    data_ptr = reinterpret_cast<u8*>(dmi_data.get_dmi_ptr());
    reg_ie = &data_ptr[0xFFFF - 0xFFFF];
  } else {
    throw std::runtime_error("Could not get DMI for the interrupt register!");
  }

  payload->set_address(0xFF0F);  // interrupt pending register
  if (init_socket->get_direct_mem_ptr(*payload, dmi_data)) {
    assert(dmi_data.get_start_address() == 0);
    assert(dmi_data.get_end_address() >= 0);
    reg_intr_pending_dmi = reinterpret_cast<u8*>(dmi_data.get_dmi_ptr());
  } else {
    throw std::runtime_error("Could not get DMI for interrupt pending register!");
  }
}

uint Ppu::MapBgCols(const uint val) {
  return ((0b11 << val*2) & *reg_bgp) >> val*2;
}

void Ppu::DrawBgToLine(uint line_num) {
  u8* tile_data_table;
  u8* bg_tile_map;

  tile_data_table = (*reg_0xFF40 & kMaskBgWndwTileDataSlct) ?  tile_data_table_low : tile_data_table_up;
  bg_tile_map = (*reg_0xFF40 & kMaskBgTileSlct) ? tile_map_up : tile_map_low;

  const u32 y_tile_pixel = (*reg_scroll_y + line_num) % 8;
  for (int i = 0; i < 160; i++) {;
    u32 bg_tile_ind = 32 * (((line_num + *reg_scroll_y) % 256) / 8) + ((*reg_scroll_x + i) % 256) / 8;
    u32 tile_ind = bg_tile_map[bg_tile_ind];
    u32 x_tile_pixel = (*reg_scroll_x + i) % 8;
    u32 pixel_ind = tile_ind * TILE_BYTES + 2 * y_tile_pixel;
    u32 res = InterleaveBits(tile_data_table[pixel_ind], tile_data_table[pixel_ind+1], 7-x_tile_pixel);
    bg_buffer[line_num][i] = res;
    assert(bg_tile_ind < 1024);
  }
}

void Ppu::DrawToBuffer() {
  u8* tile_data_table;
  u8* bg_tile_map;
  u8* wndw_tile_map;

  tile_data_table = (*reg_0xFF40 & kMaskBgWndwTileDataSlct) ?  tile_data_table_low : tile_data_table_up;
  bg_tile_map = (*reg_0xFF40 & kMaskBgTileSlct) ? tile_map_up : tile_map_low;
  wndw_tile_map = (*reg_0xFF40 & kMaskWndwTileMapSlct) ? tile_map_up : tile_map_low;

  // the screen is 18 tiles high and 20 pixels wide
  int bg_tile_ind;
  int wndw_tile_ind;
  u8 res;  // Result after interleaving the bytes.
  for (int j = 0; j < 32; j++) {
    for (int i = 0; i < 32; i++) {
      bg_tile_ind = bg_tile_map[j*32+i];
      wndw_tile_ind = wndw_tile_map[j*32+i];
      for (int l = 0; l < kTileLength; l++) {
        for (int k = 0; k < kTileLength; k++) {
          // int bg_pixel_ind = bg_tile_ind*TILE_BYTES+l*2;
          int wndw_pixel_ind = wndw_tile_ind*TILE_BYTES+l*2;

          // res = InterleaveBits( tile_data_table[bg_pixel_ind], tile_data_table[bg_pixel_ind+1], (kTileLength-k)-1 );
          // bg_buffer[j*kTileLength+l][i*kTileLength+k] = res;

          res = InterleaveBits(tile_data_table[wndw_pixel_ind], tile_data_table[wndw_pixel_ind+1], (kTileLength-k)-1);
          window_buffer[j*kTileLength+l][i*kTileLength+k] = res;
        }
      }
    }
  }

  // iterate through the oam table
  // smallest x always overlaps TODO(niko)
  // when sprites with the same x overlap the higher memory wins TODO(niko)
  // x=0 and y=0 hides a sprite sx-8 and s-8
  if (kMaskObjSpriteDisp & *reg_0xFF40) {
    u8* tile_data_table = tile_data_table_low;  // sprites always use the low data table
    for (uint i = 0; i < kNumOamEntries; i++) {
      uint pos_y            = oam_table[i*kOamEntryBytes];      // byte0 = y pos
      uint pos_x            = oam_table[i*kOamEntryBytes + 1];  // byte1 = x pos
      uint sprite_tile_ind  = oam_table[i*kOamEntryBytes + 2];  // byte2 = tile index
      uint sprite_flags     = oam_table[i*kOamEntryBytes + 3];  // byte3 = flags //TODO(niko)

      const bool is_big_sprite = static_cast<bool>(KMaskObjSpriteSize & *reg_0xFF40);
      if (is_big_sprite) {
        sprite_tile_ind &= 0b11111110;  // Ignore the last bit in 8x16 mode.
      }

      const uint sprite_height = is_big_sprite ? 16 : 8;  // Width of a sprite in pixels.
      const uint sprite_width = 8;  // Height of a sprite in pixels.
      const uint sprite_bytes = is_big_sprite ? 32 : 16;  // Bytes per sprite.

      for (uint l = 0; l < sprite_height; l++) {
        for (uint k = 0; k < sprite_width; k++) {
          uint sprite_pixel_ind = sprite_tile_ind * sprite_bytes + l*2;
          res = InterleaveBits(tile_data_table[sprite_pixel_ind], tile_data_table[sprite_pixel_ind+1], (sprite_width-k)-1);
          int x_draw = pos_x + k - 8;
          int y_draw = pos_y + l - 16;
          if ((x_draw < 0) | (x_draw >= 160) | (y_draw < 0) | (y_draw >= 144))
            continue;
          sprite_buffer[y_draw][x_draw] = res;
         // assert(sprite_pixel_ind < 3072);
        }
      }
    }
  }
}

// A complete screen refresh occurs every 70224 cycles.
void Ppu::RenderLoop() {
  while (1) {
    for (uint i=0; i < 144; i++) {
      SetBit(reg_0xFF41, false, 0);  // Mode = OAM-search (10).
      SetBit(reg_0xFF41, true, 1);
      wait(80);
      SetBit(reg_0xFF41, true, 0);  // Mode = LCD transfer (11)
      wait(168);
      DrawBgToLine(i);
      if (*reg_0xFF41 & gb_const::kMaskBit3) {
        *reg_intr_pending_dmi |= kMaskLcdcStatIf;  // Continue here
      }
      *reg_0xFF41 = 0b11111100;  // Mode = H-Blank (00).

      bool ly_coinc_interrupt = *reg_intr_pending_dmi & gb_const::kMaskBit6;
      bool ly_coinc = *reg_ly_comp == i;
      if (ly_coinc_interrupt && ly_coinc) {
          *reg_intr_pending_dmi |= kMaskLcdcStatIf;
      }
      SetBit(reg_0xFF41, ly_coinc, 2);

      wait(208);
      (*reg_lcdc_y)++;
    }
    SetBit(reg_0xFF41, true, 0);  // Mode = V-Blank (01)

    SDL_Delay(1);  // TODO(niko) make this correct with realtime things etc.
    DrawToBuffer();
    game_wndw.DrawToScreen(*this);
    window_wndw.DrawToScreen(*this);
    DBG_LOG_PPU(std::endl << PpuStateStr());

    // irq_vblank.write(true);
    *reg_intr_pending_dmi |= kMaskVBlankIE;  // V-Blank interrupt.

    for (uint i=0; i < 10; i++) {
      wait(456);  // The vblank period is 4560 cycles.
      (*reg_lcdc_y)++;
    }
    *reg_lcdc_y = 0;
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

Ppu::RenderWindow::RenderWindow(uint width, uint height, uint log_width, uint log_height)
    : width(width), height(height), log_width(log_width), log_height(log_height) {
  SDL_Init(SDL_INIT_VIDEO);
  SDL_CreateWindowAndRenderer(width, height, 0, &window, &renderer);
  SDL_RenderSetLogicalSize(renderer, log_width, log_height);
  SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
  SDL_RenderClear(renderer);
  SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
}

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
  for (uint j = 0; j < log_height; j++) {
    for (uint i = 0; i < log_width; i++) {
      val = p.bg_buffer[j][i];
      val = p.MapBgCols(val);
      SDL_SetRenderDrawColor(renderer,
                             renderColor[val][0],
                             renderColor[val][1],
                             renderColor[val][2], 255);
      SDL_RenderDrawPoint(renderer, i, j);
    }
  }

  // Window loop
  if ((*p.reg_0xFF40 & kMaskWndwDisp) && (*p.reg_0xFF40 & kMaskBgWndwDisp)) {
    for (uint j = *p.reg_wndw_y; j < kGbScreenHeight; j++) {
      for (uint i = *p.reg_wndw_x; i < kGbScreenWidth; i++) {
        val = p.window_buffer[j-*p.reg_wndw_y][i-*p.reg_wndw_x];
        val = p.MapBgCols(val);
        SDL_SetRenderDrawColor(renderer,
                               renderColor[val][0],
                               renderColor[val][1],
                               renderColor[val][2], 255);
        SDL_RenderDrawPoint(renderer, i, j);
      }
    }
  }

  // Sprite loop
  for (uint j = 0; j < kGbScreenHeight; j++) {
    for (uint i = 0; i < kGbScreenWidth; i++) {
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
  SDL_RenderPresent(renderer);
}

// TODO(me) render both, higher and lower, tile maps
void Ppu::WindowWindow::DrawToScreen(Ppu &p) {
  for (uint t = 0; t < 16; t++) {
    for (uint i = 0; i < 8; i++) {
      for (uint j = 0; j < 16; j++) {
        for (uint k = 0; k < 8; k++) {
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
