#pragma once
/*******************************************************************************
 * Apache License, Version 2.0
 * Copyright (c) 2022 chciken/Niko
 *
 * If a module uses/sets interrupts (CPU, PPU, Joypad),
 * it inherits from this class.
 * Basically provides an initiator socket and the corresponding interupt registers.
 * Calling start_of_simulation() in the child's class start_of_simulation
 * is obligatory.
********************************************************************************/

#include "common.h"

template <typename T>
class InterruptModule {
 public:
  InterruptModule() : init_socket("init_socket") {}
  tlm_utils::simple_initiator_socket<T, gb_const::kBusDataWidth> init_socket;

 protected:
  // DMI pointer to the interrupt enable register at 0xffff.
  u8* reg_intr_enable_dmi = nullptr;
  // DMI pointer to register at 0xff0f indicating pending interrupts.
  u8* reg_intr_pending_dmi = nullptr;

  // Initialize interrupt enable and pending DMI.
  void start_of_simulation() {
    if (reg_intr_enable_dmi == nullptr) {
      tlm::tlm_dmi dmi_data;
      uint dummy_data;
      auto payload = MakeSharedPayloadPtr(tlm::TLM_READ_COMMAND, 0xffff, reinterpret_cast<void*>(&dummy_data));
      if (init_socket->get_direct_mem_ptr(*payload, dmi_data)) {
        reg_intr_enable_dmi = reinterpret_cast<u8*>(dmi_data.get_dmi_ptr());
      } else {
        throw std::runtime_error("Could not get interrupt enable register DMI!");
      }
      payload->set_address(0xff0f);
      if (init_socket->get_direct_mem_ptr(*payload, dmi_data)) {
        reg_intr_pending_dmi = reinterpret_cast<u8*>(dmi_data.get_dmi_ptr());
      } else {
        throw std::runtime_error("Could not get interrupt pending register DMI!");
      }
    }
  }
};
