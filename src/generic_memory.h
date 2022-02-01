#pragma once
/*******************************************************************************
 * Copyright (C) 2020 chciken
 * MIT License
 *
 * This class implements a generic memory module
 ******************************************************************************/
#define SC_INCLUDE_DYNAMIC_PROCESSES

#include <cstdint>
#include <filesystem>
#include <iostream>
#include <string>

#include "common.h"
#include "fmt/format.h"
#include "game_info.h"
#include "gb_const.h"

struct GenericMemory : public sc_module {
  SC_HAS_PROCESS(GenericMemory);
  tlm_utils::simple_target_socket<GenericMemory, gb_const::kBusDataWidth> targ_socket;

  GenericMemory(uint64_t memory_size, sc_module_name name, u8 *data = nullptr);
  ~GenericMemory();

  u8* GetDataPtr();
  void SetMemData(u8 *data, size_t size);
  virtual void LoadFromFile(std::filesystem::path path, int offset = 0);

  // SystemC interfaces
  virtual void b_transport(tlm::tlm_generic_payload& trans, sc_time& delay);
  virtual bool get_direct_mem_ptr(tlm::tlm_generic_payload& trans, tlm::tlm_dmi& dmi_data);
  unsigned int transport_dbg(tlm::tlm_generic_payload& trans);

 protected:
  u8 *data_;
  size_t memory_size_;
};
