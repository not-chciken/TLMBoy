#pragma once
/*******************************************************************************
 * Apache License, Version 2.0
 * Copyright (c) 2025 chciken/Niko
 *
 * This class implements a generic memory module.
 ******************************************************************************/
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <string>

#include "common.h"
#include "game_info.h"
#include "gb_const.h"

struct GenericMemory : public sc_module {
  SC_HAS_PROCESS(GenericMemory);
  tlm_utils::simple_target_socket<GenericMemory, gb_const::kBusDataWidth> targ_socket;

  GenericMemory(size_t memory_size, sc_module_name name, u8* data = nullptr);
  ~GenericMemory();
  GenericMemory(GenericMemory const&) = delete;
  void operator=(GenericMemory const&) = delete;

  u8* GetDataPtr();
  void SetMemData(u8* data, size_t size);
  virtual void LoadFromFile(std::filesystem::path path, int offset = 0);

  // SystemC interfaces
  virtual void b_transport(tlm::tlm_generic_payload& trans, sc_time& delay);
  virtual bool get_direct_mem_ptr(tlm::tlm_generic_payload& trans, tlm::tlm_dmi& dmi_data);
  unsigned int transport_dbg(tlm::tlm_generic_payload& trans);

 protected:
  u8* data_;
  size_t memory_size_;
  bool delete_data_;
};
