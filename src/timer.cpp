/*******************************************************************************
 * Copyright (C) 2022 chciken/Niko
 * MIT License
 ******************************************************************************/

#include "timer.h"


Timer::Timer(sc_module_name name, u8 *reg_if)
    : sc_module(name), reg_if(reg_if) {
  SC_CTHREAD(TimerLoop, clk);
  SC_CTHREAD(DivLoop, clk);
  targ_socket.register_b_transport(this, &Timer::b_transport);
  targ_socket.register_transport_dbg(this, &Timer::transport_dbg);
}

void Timer::DivLoop() {
  while (true) {
      wait(256);
      ++reg_div;
  }
}

void Timer::TimerLoop() {
  u8 old_val;
  while (true) {
    wait(cycles_per_inc_);
    if (reg_tac & gb_const::kMaskBit2) {
      old_val = reg_tima++;
      if (old_val > reg_tima) {
        reg_tima = reg_tma;
        *reg_if |= gb_const::kMaskTimerOf;
      }
    }
  }
}

void Timer::b_transport(tlm::tlm_generic_payload& trans, sc_time& delay) {
  tlm::tlm_command cmd = trans.get_command();
  u16 adr = static_cast<u16>(trans.get_address());
  unsigned char* ptr = trans.get_data_ptr();
  u8 *reg;
  assert(adr < 0x4);
  switch (adr) {
    case 0: reg = &reg_div; break;
    case 1: reg = &reg_tima; break;
    case 2: reg = &reg_tma; break;
    case 3: reg = &reg_tac; break;
  }

  if (cmd == tlm::TLM_READ_COMMAND) {
    *ptr = *reg;
    trans.set_response_status(tlm::TLM_OK_RESPONSE);
  } else if (cmd == tlm::TLM_WRITE_COMMAND) {
    *reg = *ptr;
    if (adr == 0) {
      *reg = 0;  // Every write to DIV resets it!
    }
    if (adr == 3) {
      switch (reg_tac & 0b11) {
        case 0: cycles_per_inc_ = 1024; break;
        case 1: cycles_per_inc_ = 16; break;
        case 2: cycles_per_inc_ = 64; break;
        case 3: cycles_per_inc_ = 256; break;
        default: assert(false);
      }
    }
    trans.set_response_status(tlm::TLM_OK_RESPONSE);
  } else {
    trans.set_response_status(tlm::TLM_COMMAND_ERROR_RESPONSE);
  }
}

uint Timer::transport_dbg(tlm::tlm_generic_payload& trans) {
  sc_time delay = sc_time(0, SC_NS);
  b_transport(trans, delay);
  return 1;
}
