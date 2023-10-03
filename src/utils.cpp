/*******************************************************************************
 * Apache License, Version 2.0
 * Copyright (c) 2023 chciken/Niko
 ******************************************************************************/

#include "utils.h"

string GetEnvVariable(const string &name) {
  char *t = std::getenv(name.c_str());

  if (t == nullptr) {
    throw std::runtime_error("Environment variable " + name + " not set!");
  }

  return string(t);
}

bool CompareFiles(const std::filesystem::path file1, const std::filesystem::path file2) {
  char buf1[1024], buf2[1024];

  std::ifstream f1_h(file1.string(), std::ios::in | std::ios::binary);
  if (!f1_h.is_open()) {
    std::cerr << "Cannot open file '" << file1.string() << "'\n";
    return false;
  }

  std::ifstream f2_h(file2.string(), std::ios::in | std::ios::binary);
  if (!f2_h.is_open()) {
    std::cerr << "Cannot open file '" << file2.string() << "'\n";
    return false;
  }

  do {
    f1_h.read(buf1, sizeof buf1);
    f2_h.read(buf2, sizeof buf2);

    if (f1_h.gcount() != f2_h.gcount()) {
      f1_h.close();
      f2_h.close();
      return false;
    }

    for (int i = 0; i < f1_h.gcount(); ++i)
      if (buf1[i] != buf2[i]) {
        f1_h.close();
        f2_h.close();
        return false;
      }
  } while (!f1_h.eof() && !f2_h.eof());

  f1_h.close();
  f2_h.close();
  return true;
}

bool IsBitSet(u8 dat, u8 bit_index) {
  assert(bit_index < 8);
  return dat & (1 << bit_index);
}

u8 SetBit(u8 dat, bool val, u8 bit_index) {
  assert(bit_index < 8);
  if (val) {
    dat |= (1 << bit_index);
  } else {
    dat &= ~(1 << bit_index);
  }
  return dat;
}

void SetBit(u8 *dat, bool val, u8 bit_index) {
  assert(bit_index < 8);
  if (val) {
    *dat |= (1 << bit_index);
  } else {
    *dat &= ~(1 << bit_index);
  }
}

std::shared_ptr<tlm::tlm_generic_payload> MakeSharedPayloadPtr(tlm::tlm_command cmd, sc_dt::uint64 addr, void *data,
                                                               bool dmi_allowed, uint size) {
  auto p = std::make_shared<tlm::tlm_generic_payload>();
  p->set_command(cmd);
  p->set_address(addr);
  p->set_data_ptr(reinterpret_cast<unsigned char *>(data));
  p->set_data_length(size);
  p->set_streaming_width(size);
  p->set_dmi_allowed(dmi_allowed);
  p->set_response_status(tlm::TLM_INCOMPLETE_RESPONSE);
  return p;
}
