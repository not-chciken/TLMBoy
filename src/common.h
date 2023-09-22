#pragma once
/*******************************************************************************
 * MIT License
 * Copyright (c) 2022 chciken/Niko
********************************************************************************/

#include <cstdint>
#include <string>

#include "systemc.h"
#include "sysc/kernel/sc_simcontext.h"
#include "tlm.h"
#include "tlm_utils/simple_initiator_socket.h"
#include "tlm_utils/simple_target_socket.h"

using u8  = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;

using i8  = int8_t;
using i16 = int16_t;
using i32 = int32_t;
using i64 = int64_t;

using string = std::string;

#include "gb_const.h"
