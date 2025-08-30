/*******************************************************************************
 * Apache License, Version 2.0
 * Copyright (c) 2025 chciken/Niko
 ******************************************************************************/

#include "timer.h"

Timer::Timer(sc_module_name name, u8* reg_if) : sc_module(name), reg_if_(reg_if) {
  SC_THREAD(DivLoop);
  SC_THREAD(TimerLoop);
  targ_socket.register_b_transport(this, &Timer::b_transport);
  targ_socket.register_transport_dbg(this, &Timer::transport_dbg);
}

// Increments the div register at rate of 16384 Hz.
void Timer::DivLoop() {
  while (true) {
    wait(256 * gb_const::kNsPerClkCycle, sc_core::SC_NS);
    ++reg_div_;
  }
}

void Timer::TimerLoop() {
  while (true) {
    wait(cycles_per_inc_ * gb_const::kNsPerClkCycle, sc_core::SC_NS);
    if (reg_tac_ & kMaskTimerEnabled) {
      u8 old_val = reg_tima_++;
      if (old_val > reg_tima_) {  // Overflow case.
        reg_tima_ = reg_tma_;
        *reg_if_ |= gb_const::kTimerOfIf;
      }
    }
  }
}

void Timer::b_transport(tlm::tlm_generic_payload& trans, sc_time& delay [[maybe_unused]]) {
  u16 adr = static_cast<u16>(trans.get_address());
  u8* reg;

  switch (adr) {
  case 0:
    reg = &reg_div_;
    break;
  case 1:
    reg = &reg_tima_;
    break;
  case 2:
    reg = &reg_tma_;
    break;
  case 3:
    reg = &reg_tac_;
    break;
  default:
    assert(false);
  }

  unsigned char* ptr = trans.get_data_ptr();
  const tlm::tlm_command cmd = trans.get_command();

  if (cmd == tlm::TLM_READ_COMMAND) {
    *ptr = *reg;
    trans.set_response_status(tlm::TLM_OK_RESPONSE);
  } else if (cmd == tlm::TLM_WRITE_COMMAND) {
    *reg = *ptr;
    if (adr == 0) {
      *reg = 0;  // Every write to DIV resets it!
    }
    if (adr == 3) {
      switch (reg_tac_ & 0b11) {
      case 0:
        cycles_per_inc_ = 1024;
        break;
      case 1:
        cycles_per_inc_ = 16;
        break;
      case 2:
        cycles_per_inc_ = 64;
        break;
      case 3:
        cycles_per_inc_ = 256;
        break;
      default:
        assert(false);
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
