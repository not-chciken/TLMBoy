/*******************************************************************************
 * Copyright (C) 2022 chciken
 * MIT License
 ******************************************************************************/

#include "generic_memory.h"

GenericMemory::GenericMemory(size_t memory_size, sc_module_name name, u8 *data)
    : sc_module(name), targ_socket("targ_socket"), memory_size_(memory_size) {
  targ_socket.register_b_transport(this, &GenericMemory::b_transport);
  targ_socket.register_transport_dbg(this, &GenericMemory::transport_dbg);
  targ_socket.register_get_direct_mem_ptr(this, &GenericMemory::get_direct_mem_ptr);
  if (data == nullptr) {
    data_ = new u8[memory_size_]{0};
    delete_data_ = true;
  } else {
    data_ = data;
    delete_data_ = false;
  }
}

GenericMemory::~GenericMemory() {
  if (delete_data_) {
    delete [] data_;
  }
}

void GenericMemory::SetMemData(u8 *data, size_t size) {
  assert(size < memory_size_);
  std::memcpy(data_, data, size);
}

u8* GenericMemory::GetDataPtr() {
  return data_;
}

void GenericMemory::LoadFromFile(std::filesystem::path path, int offset) {
  std::ifstream file(path.string(), std::ios::binary | std::ios::ate);
  if (!file) {
    throw std::runtime_error(fmt::format("Could not read file '{}'!", path.string()));
  }
  std::streamsize size = file.tellg();
  assert(size >= offset);
  if (size - offset > static_cast<int>(memory_size_)) {
    size = static_cast<int>(memory_size_) - offset;
  } else {
    size = size - offset;
  }
  file.seekg(offset, std::ios::beg);

  if (file.read(reinterpret_cast<char*>(&data_[0]), size)) {
    return;
  } else {
    throw std::runtime_error("could not read file");
  }
  file.close();
}

void GenericMemory::b_transport(tlm::tlm_generic_payload& trans, sc_time& delay) {
  tlm::tlm_command cmd = trans.get_command();
  u16 adr = static_cast<u16>(trans.get_address());
  unsigned char* ptr = trans.get_data_ptr();

  if (cmd == tlm::TLM_READ_COMMAND) {
    *ptr = data_[adr];
    trans.set_response_status(tlm::TLM_OK_RESPONSE);
  } else if (cmd == tlm::TLM_WRITE_COMMAND) {
    data_[adr] = *ptr;
    trans.set_response_status(tlm::TLM_OK_RESPONSE);
  } else {
    trans.set_response_status(tlm::TLM_COMMAND_ERROR_RESPONSE);
  }
}

uint GenericMemory::transport_dbg(tlm::tlm_generic_payload& trans) {
  sc_time delay(0, SC_NS);
  b_transport(trans, delay);
  return 1;
}

bool GenericMemory::get_direct_mem_ptr(tlm::tlm_generic_payload& trans, tlm::tlm_dmi& dmi_data) {
  u16 adr = trans.get_address();
  assert(adr < memory_size_);
  dmi_data.allow_read_write();
  dmi_data.set_start_address(0);
  dmi_data.set_end_address(memory_size_ - 1);
  dmi_data.set_dmi_ptr(reinterpret_cast<unsigned char*>(&data_[adr]));
  return true;
}

