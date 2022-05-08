#pragma once
/*******************************************************************************
 * Copyright (C) 2021 chciken
 * MIT License
 *
 * This class implements the Game Boy's joy pad.
 * An important role plays "reg_0xFF00" that is structured as follows:
 *
 *
 * bit 7-6: not used
 * bit 5: P15 select button keys*               P15        P14
 * bit 4: P14 select direction keys              |          |
 * bit 3: P13 in port (read only)      P10-------O-Right----O-A
 * bit 2: P12 in port (read only)                |          |
 * bit 1: P11 in port (read only)      P11-------O-Left-----O-B
 * bit 0: P10 in port (read only)                |          |
 *                                     P12-------O-Up-------O-Select
 *                                               |          |
 *                                     P13-------O-Down-----O-Start
 * Note: "0"=selected/pressed!
 *
 * Currently the following key mapping is used:
 * left, right, top, bottom arrow key as the control cross
 * A key = A button; S key = B button
 * O key = Select button; P key = Start Button
 ******************************************************************************/
#include "SDL2/SDL.h"
#include "systemc.h"
#include "sysc/kernel/sc_simcontext.h"
#include "tlm.h"

#include "common.h"
#include "debug.h"
#include "utils.h"

struct JoyPad : public sc_module {
  SC_HAS_PROCESS(JoyPad);
  u8 reg_0xFF00 = 0b00111111;
  const u32 kWaitMs = 10;
  bool but_up = false;
  bool but_down = false;
  bool but_left = false;
  bool but_right = false;
  bool but_a = false;
  bool but_b = false;
  bool but_start = false;
  bool but_select = false;
  SDL_Event event;
  tlm_utils::simple_target_socket<JoyPad, gb_const::kBusDataWidth> targ_socket;

  explicit JoyPad(sc_module_name name);

  void InputLoop();
  void SetButton(SDL_Keycode sym, bool pressed);
  u8 ReadReg();
  void WriteReg(u8 dat);
  void b_transport(tlm::tlm_generic_payload& trans, sc_time& delay);
  uint transport_dbg(tlm::tlm_generic_payload& trans);
};
