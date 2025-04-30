#pragma once
/*******************************************************************************
 * Apache License, Version 2.0
 * Copyright (c) 2025 chciken/Niko
 *
 * IO registers. Reside in 0xFF10-0xFF7F.
 ******************************************************************************/
#include <filesystem>

#include "common.h"
#include "generic_memory.h"
#include "joypad.h"

struct IoRegisters : public GenericMemory {
  IoRegisters();

  void DmaTransfer(const u8 byte);

  // SystemC interfaces.
  sc_out<bool> sig_unmap_rom_out;              // Unmaps the boot ROM.
  sc_out<bool> sig_reload_length_square1_out;  // Sound length register Square 1.
  sc_out<bool> sig_reload_length_square2_out;  // Sound length register Square 2.
  sc_out<bool> sig_reload_length_wave_out;     // Sound length register Wave.
  sc_out<bool> sig_reload_length_noise_out;    // Sound length register Noise.
  sc_out<bool> sig_trigger_square1_out;        // Trigger event Square 1.
  sc_out<bool> sig_trigger_square2_out;        // Trigger event Square 2.
  sc_out<bool> sig_trigger_wave_out;           // Trigger event Wave.
  sc_out<bool> sig_trigger_noise_out;          // Trigger event Noise.

  tlm_utils::simple_initiator_socket<IoRegisters, gb_const::kBusDataWidth> init_socket;
  void b_transport(tlm::tlm_generic_payload& trans, sc_time& delay [[maybe_unused]]) override;
};
