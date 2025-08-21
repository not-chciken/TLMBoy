#pragma once
/*******************************************************************************
 * Apache License, Version 2.0
 * Copyright (c) 2025 chciken/Niko
 *
 * This class implements the Game Boy's joy pad.
 * An important role plays "reg_p1_" that is structured as follows:
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
#include <SDL2/SDL.h>
#include <sysc/kernel/sc_simcontext.h>
#include <systemc.h>
#include <tlm.h>

#include "common.h"
#include "debug.h"
#include "interrupt_module.h"
#include "utils.h"

struct JoyPad : public InterruptModule<JoyPad>, public sc_module {
  SC_HAS_PROCESS(JoyPad);

  explicit JoyPad(sc_module_name name);
  JoyPad(JoyPad const&) = delete;
  void operator=(JoyPad const&) = delete;

  void InputLoop();
  void SetButton(SDL_Keycode sym, bool pressed);
  u8 ReadReg();
  void WriteReg(u8 dat);

  // SystemC interfaces.
  tlm_utils::simple_target_socket<JoyPad, gb_const::kBusDataWidth> targ_socket;
  void start_of_simulation() override;
  void b_transport(tlm::tlm_generic_payload& trans, sc_time& delay);
  uint transport_dbg(tlm::tlm_generic_payload& trans);

 private:
  bool but_up_;
  bool but_down_;
  bool but_left_;
  bool but_right_;
  bool but_a_;
  bool but_b_;
  bool but_start_;
  bool but_select_;
  u8 reg_p1_;  // Register at 0xff00 for reading joy pad info.
  SDL_Event event;
};
