#pragma once
/*******************************************************************************
 * Apache License, Version 2.0
 * Copyright (c) 2025 chciken/Niko
 *
 * This class implements the Game Boy's serial port.
 * It doesn't really send/receive data but some games (like Alleyway) require
 * a working serial interrupt for some reasons.
 ******************************************************************************/

#include <sysc/kernel/sc_simcontext.h>
#include <systemc.h>
#include <tlm.h>

#include "common.h"
#include "utils.h"

struct Serial : public sc_module {
  SC_HAS_PROCESS(Serial);

  static constexpr u8 kMaskClockSource = 0b1u;;
  static constexpr u8 kMaskTransferStart = 0b10000000u;

  Serial(sc_module_name name, u8* reg_if);

  bool ongoing_transmission;
  u8 reg_sc;   // SIO control register (0xFF02).
  u8* reg_if;  // Interrupt Flag register (0xFF0F).

  void SerialInterrupt();

  // SystemC interfaces
  sc_event interrupt_event;
  tlm_utils::simple_target_socket<Serial, gb_const::kBusDataWidth> targ_socket;
  uint transport_dbg(tlm::tlm_generic_payload& trans);
  void b_transport(tlm::tlm_generic_payload& trans, sc_time& delay);
};
