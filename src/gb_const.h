#pragma once
/*****************************************************************************************
 * Copyright (C) 2022 chiken
 * MIT License
 *
 * Here global constants of the gameboy reside.
 * Like the address of the interrupt registers, the bus width, etc.
 ****************************************************************************************/

#include <cstdint>

#include "utils.h"

namespace gb_const {
const u16 kAdrIntrFlag    = 0xFF0F;  // address of the interrupt flag register
const u8 kMaskVBlank     = 0b00000001;  // bit0: v blank interrupt mask
const u8 kMaskLCDC       = 0b00000010;  // bit1: lcdc interrupt mask TODO
const u8 kMaskTimerOf    = 0b00000100;  // bit2: Timer overflow interrupt.
const u8 kMaskSerialIO   = 0b00001000;  // bit3: serial IO transer interrupt TODO
const u8 kMaskH2L        = 0b00010000;  // bit4: transition from high to low p10-p13 TODO
const uint kBusDataWidth = 8;
const uint kBusAddrWidth = 16;

const u8 kMaskBit0 = 0b00000001;
const u8 kMaskBit1 = 0b00000010;
const u8 kMaskBit2 = 0b00000100;
const u8 kMaskBit3 = 0b00001000;
const u8 kMaskBit4 = 0b00010000;
const u8 kMaskBit5 = 0b00100000;
const u8 kMaskBit6 = 0b01000000;
const u8 kMaskBit7 = 0b10000000;

const i32 kNsPerClkCycle = 238;  // Refers to roughly 4.19MHz.
const i32 kNsPerMachineCycle = kNsPerClkCycle * 4; // Refers to roughly 1.05MHz
}  // namespace gb_const
