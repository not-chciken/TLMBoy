#pragma once
/*******************************************************************************
 * Apache License, Version 2.0
 * Copyright (c) 2023 chciken/Niko
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
  sc_out<bool> sig_unmap_rom_out;  // Unmaps the boot ROM.
  tlm_utils::simple_initiator_socket<IoRegisters, gb_const::kBusDataWidth> init_socket;
  void b_transport(tlm::tlm_generic_payload& trans, sc_time& delay [[maybe_unused]]) override;
};
