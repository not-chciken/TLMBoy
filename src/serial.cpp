/*******************************************************************************
 * Apache License, Version 2.0
 * Copyright (c) 2025 chciken/Niko
 ******************************************************************************/

#include "serial.h"

Serial::Serial(sc_module_name name, u8* reg_if) : sc_module(name), reg_if(reg_if) {
  SC_METHOD(SerialInterrupt);
  sensitive << interrupt_event;
  targ_socket.register_b_transport(this, &Serial::b_transport);
  targ_socket.register_transport_dbg(this, &Serial::transport_dbg);
}

void Serial::SerialInterrupt() {
  *reg_if |= gb_const::kSerialIOIf;
  reg_sc &= ~kMaskTransferStart;
}

void Serial::b_transport(tlm::tlm_generic_payload& trans, sc_time& delay [[maybe_unused]]) {
  tlm::tlm_command cmd = trans.get_command();
  unsigned char* ptr = trans.get_data_ptr();
  u16 adr = static_cast<u16>(trans.get_address());
  assert(adr < 0x2);

  if (cmd == tlm::TLM_READ_COMMAND) {
    switch (adr) {
    case 0:         // 0xFF01: Data to be written/read.
      *ptr = 0xFF;  // The game "Alleyway" requires 0xFF to be returned.
      break;
    case 1:
      *ptr = reg_sc;
      break;  // 0xFF02
    default:
      break;
    }
    trans.set_response_status(tlm::TLM_OK_RESPONSE);
  } else if (cmd == tlm::TLM_WRITE_COMMAND) {
    switch (adr) {
    case 0:   // 0xFF01: Data to be written/read.
      break;  // Do nothing as there is no receiving end.
    case 1:
      reg_sc = *ptr;
      break;  // 0xFF02
    default:
      break;
    }

    if (adr == 1 && (reg_sc & kMaskTransferStart)) {
      interrupt_event.notify(8 * 122, SC_US);
    }

    trans.set_response_status(tlm::TLM_OK_RESPONSE);
  } else {
    trans.set_response_status(tlm::TLM_COMMAND_ERROR_RESPONSE);
  }
}

uint Serial::transport_dbg(tlm::tlm_generic_payload& trans) {
  sc_time delay = sc_time(0, SC_NS);
  b_transport(trans, delay);
  return 1;
}
