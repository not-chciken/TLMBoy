#pragma once
/*****************************************************************************************
 * Apache License, Version 2.0
 * Copyright (c) 2025 chciken/Niko
 *
 * Here global constants of the gameboy reside.
 * Like the address of the interrupt registers, the bus width, etc.
 ****************************************************************************************/

#include <cstdint>

#include "utils.h"

namespace gb_const {
constexpr u16 kAdrIntrFlag = 0xFF0F;  // Address of the interrupt flag register
constexpr u8 kVBlankIf = 1;           // bit0: V-blank interrupt mask.
constexpr u8 kLCDCIf = 1 << 1;        // bit1: LCDC interrupt mask.
constexpr u8 kTimerOfIf = 1 << 2;     // bit2: Timer overflow interrupt.
constexpr u8 kSerialIOIf = 1 << 3;    // bit3: Serial IO transfer interrupt.
constexpr u8 kJoypadIf = 1 << 4;      // bit4: Transition from high to low p10-p13.
constexpr uint kBusDataWidth = 8;
constexpr uint kBusAddrWidth = 16;

constexpr u8 kMaskBit0 = 0b00000001;
constexpr u8 kMaskBit1 = 0b00000010;
constexpr u8 kMaskBit2 = 0b00000100;
constexpr u8 kMaskBit3 = 0b00001000;
constexpr u8 kMaskBit4 = 0b00010000;
constexpr u8 kMaskBit5 = 0b00100000;
constexpr u8 kMaskBit6 = 0b01000000;
constexpr u8 kMaskBit7 = 0b10000000;

constexpr i32 kNsPerClkCycle = 238;                     // Refers to roughly 4.19 MHz.
constexpr i32 kNsPerMachineCycle = kNsPerClkCycle * 4;  // Refers to roughly 1.05 MHz
constexpr i32 kClockCycleFrequency = 4194304;
}  // namespace gb_const
