#pragma once
/*******************************************************************************
 * Apache License, Version 2.0
 * Copyright (c) 2024 chciken/Niko
 *
 * The SymfileTracer tracks memory accesses to identify data sections.
 * Adheres to the Game Boy symbol file format specification.
 * See: https://rgbds.gbdev.io/sym/
 ******************************************************************************/

#include <iostream>

#include "common.h"

struct SymfileTracer {
  constexpr static size_t kMaxBanks = 512;
  constexpr static size_t kBankSize = 0x4000;

  SymfileTracer();
  ~SymfileTracer();

  void Clear();
  void DumpTrace(std::ostream& os);
  void TraceAccess(u16 bank, u16 adr);

 private:
  bool* access_arr_;
};
