/*******************************************************************************
 * Copyright (C) 2021 chciken
 * MIT License
 *
 ******************************************************************************/

#include "joypad.h"

JoyPad::JoyPad(sc_module_name name) :
  sc_module(name) {
  SC_THREAD(InputLoop);
  targ_socket.register_b_transport(this, &JoyPad::b_transport);
}

// This thread continously reads the inputs from SDL.
// Fine-tune the polling with kWaitMs.
void JoyPad::InputLoop() {
  while (1) {
    SDL_PollEvent(&event);
    switch (event.type) {
      case SDL_KEYDOWN:
        if (event.key.repeat == true) {
          break;
        }
        SetButton(event.key.keysym.sym, true);
        break;
      case SDL_KEYUP:
        if (event.key.repeat == true) {
          break;
        }
        SetButton(event.key.keysym.sym, false);
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
      but_up = pressed;
      break;
    case SDLK_DOWN:
      DBG_LOG_JP("button 'down' " << (pressed ? "pressed" : "released"));
      but_down = pressed;
      break;
    case SDLK_RIGHT:
      DBG_LOG_JP("button 'right' " << (pressed ? "pressed" : "released"));
      but_right = pressed;
      break;
    case SDLK_LEFT:
      DBG_LOG_JP("button 'left' " << (pressed ? "pressed" : "released"));
      but_left = pressed;
      break;
    case SDLK_a:
      DBG_LOG_JP("button 'A' " << (pressed ? "pressed" : "released"));
      but_a = pressed;
      break;
    case SDLK_s:
      DBG_LOG_JP("button 'B' " << (pressed ? "pressed" : "released"));
      but_b = pressed;
      break;
    case SDLK_o:
      DBG_LOG_JP("button 'select' " << (pressed ? "pressed" : "released"));
      but_select = pressed;
      break;
    case SDLK_p:
      DBG_LOG_JP("button 'start' " << (pressed ? "pressed" : "released"));
      but_start = pressed;
      break;
    default:
      break;
  }
}

u8 JoyPad::ReadReg() {
  if (!IsBitSet(reg_0xFF00, 4)) {
    SetBit(&reg_0xFF00, !but_right, 0);
    SetBit(&reg_0xFF00, !but_left, 1);
    SetBit(&reg_0xFF00, !but_up, 2);
    SetBit(&reg_0xFF00, !but_down, 3);
  }
  if (!IsBitSet(reg_0xFF00, 5)) {
    SetBit(&reg_0xFF00, !but_a, 0);
    SetBit(&reg_0xFF00, !but_b, 1);
    SetBit(&reg_0xFF00, !but_select, 2);
    SetBit(&reg_0xFF00, !but_start, 3);
  }
  return reg_0xFF00;
}

// Note, that writes only affect the higher nibble
void JoyPad::WriteReg(u8 dat) {
  reg_0xFF00 &= 0b00001111;
  reg_0xFF00 |= dat & 0b1111000;
}

void JoyPad::b_transport(tlm::tlm_generic_payload& trans, sc_time& delay) {
  tlm::tlm_command cmd = trans.get_command();
  u16 adr = static_cast<u16>(trans.get_address());
  u8* ptr = trans.get_data_ptr();
  assert(adr == 0);

  trans.set_response_status(tlm::TLM_OK_RESPONSE);
  if (cmd == tlm::TLM_READ_COMMAND) {
    *ptr = ReadReg();
  } else if (cmd == tlm::TLM_WRITE_COMMAND) {
    WriteReg(*ptr);
  } else {
    trans.set_response_status(tlm::TLM_COMMAND_ERROR_RESPONSE);
  }
}
