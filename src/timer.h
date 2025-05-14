#pragma once
/*******************************************************************************
 * Apache License, Version 2.0
 * Copyright (c) 2025 chciken/Niko
 *
 * This class implements the Game Boy's timer.
 * Don't confuse it with the MBC3 battery buffered real time clock.
 *
 * FF04 - DIV - Divider Register (R/W)
 *  8 bit, Increments at a rate of 16384Hz. Writing into it resets it to 0x00.
 * FF05 - TIMA - Timer counter (R/W)
 *  Increments at a rate of defined by TAC. If this overflows => interrupt
 * FF06 - TMA - Timer Modulo (R/W)
 *  When TIMA overflows, it will be reset to this value.
 * FF07 - TAC - Timer Control (R/W)
 *  Bit  2   - Timer Enable
 *  Bits 1-0 - Input Clock Select
 *   00: CPU Clock / 1024 (DMG,   4096 Hz)
 *   01: CPU Clock / 16   (DMG, 262144 Hz)
 *   10: CPU Clock / 64   (DMG,  65536 Hz)
 *   11: CPU Clock / 256  (DMG,  16384 Hz)
 *
 * Note: The "Timer Enable" bit only affects the timer, the divider is always counting.
 * Source: https://gbdev.gg8.se/wiki/articles/Timer_and_Divider_Registers
 ******************************************************************************/

#include <sysc/kernel/sc_simcontext.h>
#include <systemc.h>
#include <tlm.h>

#include "common.h"
#include "debug.h"
#include "utils.h"

struct Timer : public sc_module {
  SC_HAS_PROCESS(Timer);

  static constexpr u8 kMaskTimerEnabled = 0b100u;

  Timer(sc_module_name name, u8* reg_if);

  // SystemC interfaces
  tlm_utils::simple_target_socket<Timer, gb_const::kBusDataWidth> targ_socket;
  void b_transport(tlm::tlm_generic_payload& trans, sc_time& delay);
  uint transport_dbg(tlm::tlm_generic_payload& trans);

 protected:
  void DivLoop();
  void TimerLoop();

  u8 reg_div_;
  u8 reg_tac_;
  u8 reg_tima_;
  u8 reg_tma_;
  u8* reg_if_;  // Interrupt flag register (0xFF0F).
  u32 cycles_per_inc_ = 1024;
};
