#pragma once
/*******************************************************************************
 * Apache License, Version 2.0
 * Copyright (c) 2025 chciken/Niko
 *
 * Some generic utils for this and that.
 ******************************************************************************/

#include <algorithm>
#include <cassert>
#include <filesystem>
#include <ios>
#include <iostream>
#include <memory>
#include <string>

#include "common.h"

string GetEnvVariable(const string& name);
bool CompareFiles(const std::filesystem::path file1, const std::filesystem::path file2);

constexpr bool IsBitSet(u8 dat, uint bit_index) noexcept {
  assert(bit_index < 8);
  return dat & (1 << bit_index);
}

constexpr u8 SetBit(u8 dat, bool val, uint bit_index) noexcept {
  assert(bit_index < 8);
  return (dat & ~(1 << bit_index)) | (val << bit_index);
}

constexpr void SetBit(u8* dat, bool val, uint bit_index) noexcept {
  assert(bit_index < 8);
  *dat = (*dat & ~(1 << bit_index)) | (val << bit_index);
}

std::shared_ptr<tlm::tlm_generic_payload> MakeSharedPayloadPtr(tlm::tlm_command cmd, sc_dt::uint64 addr,
                                                               void* data = nullptr, bool dmi_allowed = false,
                                                               uint size = 1);
