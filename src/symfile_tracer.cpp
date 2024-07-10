

#include "symfile_tracer.h"

#include <format>

SymfileTracer::SymfileTracer() {
  access_arr_ = new bool[kMaxBanks * kBankSize](0);  // 512 banks á 16kiB.
}

SymfileTracer::~SymfileTracer() {
  delete[] access_arr_;
}

void SymfileTracer::Clear() {
  std::memset(access_arr_, false, kMaxBanks * kBankSize);
}

void PrintSymbol(std::ostream& os, size_t bank, u16 start_adr, i32 len) {
  os << std::format("{:02x}:{:04x} Data{:02x}{:04x}\n", bank, start_adr, bank, start_adr);
  os << std::format("{:02x}:{:04x} .data:{:x}\n", bank, start_adr, len);
}

void SymfileTracer::DumpTrace(std::ostream& os) {
  for (size_t bank = 0; bank < kMaxBanks; ++bank) {
    bool in_access = false;
    u16 start_adr = 0;
    for (size_t adr = 0; adr < kBankSize; ++adr) {
      if (access_arr_[kBankSize * bank + adr]) {
        if (!in_access) {
          in_access = true;
          start_adr = adr;
        }
      } else {
        if (in_access) {
          in_access = false;
          i32 len = adr - start_adr;
          PrintSymbol(os, bank, start_adr, len);
        }
      }
    }
    if (in_access) {
      i32 len = 0x4000 - static_cast<i32>(start_adr);
      PrintSymbol(os, bank, start_adr, len);
    }
  }
}

void SymfileTracer::TraceAccess(u16 bank, u16 adr) {
  access_arr_[kBankSize * bank + adr] = true;
}
