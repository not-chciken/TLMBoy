/*******************************************************************************
 * Apache License, Version 2.0
 * Copyright (c) 2025 chciken/Niko
 ******************************************************************************/

#include "joypad.h"

#include "ppu.h"

JoyPad::JoyPad(sc_module_name name)
    : sc_module(name),
      but_up_(false),
      but_down_(false),
      but_left_(false),
      but_right_(false),
      but_a_(false),
      but_b_(false),
      but_start_(false),
      but_select_(false),
      reg_p1_(0b00111111) {
  SC_THREAD(InputLoop);
  targ_socket.register_b_transport(this, &JoyPad::b_transport);
  targ_socket.register_transport_dbg(this, &JoyPad::transport_dbg);
}

// This thread continously reads the inputs from SDL. Fine-tune the polling with kWaitMs.
void JoyPad::InputLoop() {
  constexpr uint kWaitMs = 10;

  while (true) {
    SDL_PollEvent(&event);
    switch (event.type) {
    case SDL_KEYDOWN:
      if (event.key.repeat == true) {
        break;
      }
      switch (event.key.keysym.sym) {
      case SDLK_1:
        Ppu::uiRenderBg = false;
        break;
      case SDLK_2:
        Ppu::uiRenderSprites = false;
        break;
      case SDLK_3:
        Ppu::uiRenderWndw = false;
        break;
      case SDLK_SPACE:
        Ppu::uiTurboMode = true;
        break;
      default:
        SetButton(event.key.keysym.sym, true);
      }
      break;
    case SDL_KEYUP:
      if (event.key.repeat == true) {
        break;
      }
      switch (event.key.keysym.sym) {
      case SDLK_1:
        Ppu::uiRenderBg = true;
        break;
      case SDLK_2:
        Ppu::uiRenderSprites = true;
        break;
      case SDLK_3:
        Ppu::uiRenderWndw = true;
        break;
      case SDLK_SPACE:
        Ppu::uiTurboMode = false;
        break;
      default:
        SetButton(event.key.keysym.sym, false);
      }
      break;
    case SDL_WINDOWEVENT:
      if (event.window.event == SDL_WINDOWEVENT_CLOSE) {
        DBG_LOG_JP("window closed");
        sc_stop();
        return;
      }
      break;
    case SDL_QUIT:
      DBG_LOG_JP("SDL quit");
      sc_stop();
      return;
    default:
      break;
    }
    wait(kWaitMs, SC_MS);
  }
}

void JoyPad::SetButton(SDL_Keycode sym, bool pressed) {
  switch (sym) {
  case SDLK_UP:
    DBG_LOG_JP("button 'up' " << (pressed ? "pressed" : "released"));
    but_up_ = pressed;
    break;
  case SDLK_DOWN:
    DBG_LOG_JP("button 'down' " << (pressed ? "pressed" : "released"));
    but_down_ = pressed;
    break;
  case SDLK_RIGHT:
    DBG_LOG_JP("button 'right' " << (pressed ? "pressed" : "released"));
    but_right_ = pressed;
    break;
  case SDLK_LEFT:
    DBG_LOG_JP("button 'left' " << (pressed ? "pressed" : "released"));
    but_left_ = pressed;
    break;
  case SDLK_a:
    DBG_LOG_JP("button 'A' " << (pressed ? "pressed" : "released"));
    but_a_ = pressed;
    break;
  case SDLK_s:
    DBG_LOG_JP("button 'B' " << (pressed ? "pressed" : "released"));
    but_b_ = pressed;
    break;
  case SDLK_o:
    DBG_LOG_JP("button 'select' " << (pressed ? "pressed" : "released"));
    but_select_ = pressed;
    break;
  case SDLK_p:
    DBG_LOG_JP("button 'start' " << (pressed ? "pressed" : "released"));
    but_start_ = pressed;
    break;
  default:
    return;
  }

  if (pressed) {
    *reg_intr_pending_dmi |= gb_const::kJoypadIf;
  }
}

u8 JoyPad::ReadReg() {
  if (!IsBitSet(reg_p1_, 4)) {
    SetBit(&reg_p1_, !but_right_, 0);
    SetBit(&reg_p1_, !but_left_, 1);
    SetBit(&reg_p1_, !but_up_, 2);
    SetBit(&reg_p1_, !but_down_, 3);
  }
  if (!IsBitSet(reg_p1_, 5)) {
    SetBit(&reg_p1_, !but_a_, 0);
    SetBit(&reg_p1_, !but_b_, 1);
    SetBit(&reg_p1_, !but_select_, 2);
    SetBit(&reg_p1_, !but_start_, 3);
  }
  return reg_p1_;
}

// Note that writes only affect the higher nibble.
void JoyPad::WriteReg(u8 dat) {
  reg_p1_ &= 0b00001111;
  reg_p1_ |= dat & 0b1111000;
}

void JoyPad::start_of_simulation() {
  InterruptModule::start_of_simulation();
}

void JoyPad::b_transport(tlm::tlm_generic_payload& trans, sc_time& delay [[maybe_unused]]) {
  assert(static_cast<u16>(trans.get_address()) == 0);
  tlm::tlm_command cmd = trans.get_command();
  u8* ptr = trans.get_data_ptr();
  trans.set_response_status(tlm::TLM_OK_RESPONSE);

  if (cmd == tlm::TLM_READ_COMMAND) {
    *ptr = ReadReg();
  } else if (cmd == tlm::TLM_WRITE_COMMAND) {
    WriteReg(*ptr);
  } else {
    trans.set_response_status(tlm::TLM_COMMAND_ERROR_RESPONSE);
  }
}

uint JoyPad::transport_dbg(tlm::tlm_generic_payload& trans) {
  sc_time delay = sc_time(0, SC_NS);
  b_transport(trans, delay);
  return 1;
}
