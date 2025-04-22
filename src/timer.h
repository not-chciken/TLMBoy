#pragma once
/*******************************************************************************
 * Apache License, Version 2.0
 * Copyright (c) 2023 chciken/Niko
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
  u8 reg_div = 0;
  u8 reg_tima = 0;
  u8 reg_tma = 0;
  u8 reg_tac = 0;
  u32 cycles_per_inc_ = 1024;
  sc_in_clk clk;
  u8* reg_if;  // Interrupt Flag register (0xFF0F).
  tlm_utils::simple_target_socket<Timer, gb_const::kBusDataWidth> targ_socket;

  Timer(sc_module_name name, u8* reg_if);

  // Increments the div register at rate of 16384Hz.
  void DivLoop();
  void TimerLoop();

  // SystemC interfaces
  void b_transport(tlm::tlm_generic_payload& trans, sc_time& delay);
  uint transport_dbg(tlm::tlm_generic_payload& trans);
};
