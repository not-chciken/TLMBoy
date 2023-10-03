#pragma once
/*******************************************************************************
 * Apache License, Version 2.0
 * Copyright (c) 2023 chciken/Niko
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

string GetEnvVariable(const string &name);
bool CompareFiles(const std::filesystem::path file1, const std::filesystem::path file2);

bool IsBitSet(u8 dat, u8 bit_index);
u8 SetBit(u8 dat, bool val, u8 bit_index);
void SetBit(u8* dat, bool val, u8 bit_index);

std::shared_ptr<tlm::tlm_generic_payload> MakeSharedPayloadPtr(tlm::tlm_command cmd, sc_dt::uint64 addr,
                                                               void* data = nullptr, bool dmi_allowed = false,
                                                               uint size = 1);
