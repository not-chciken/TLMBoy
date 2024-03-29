#pragma once
/*******************************************************************************
 * Apache License, Version 2.0
 * Copyright (c) 2023 chciken/Niko
 *
 * This is the top level of the Game Boy.
 ******************************************************************************/
#include <filesystem>

#include "bus.h"
#include "cartridge.h"
#include "cpu.h"
#include "common.h"
#include "game_info.h"
#include "generic_memory.h"
#include "io_registers.h"
#include "joypad.h"
#include "options.h"
#include "ppu.h"
#include "timer.h"

struct GbTop : public sc_module {
  SC_HAS_PROCESS(GbTop);
  u8 reg_ie;  // Interrupt Enable at 0xFFFF.
  Cartridge cartridge;
  Bus gb_bus;
  Cpu gb_cpu;
  JoyPad joy_pad;
  GenericMemory video_ram;
  GenericMemory work_ram;
  GenericMemory work_ram_n;
  GenericMemory echo_ram;
  GenericMemory echo_ram_n;
  GenericMemory obj_attr_mem;
  GenericMemory high_ram;
  GenericMemory serial_transfer;
  GenericMemory reg_if;
  GenericMemory intr_enable;
  IoRegisters io_registers;
  Ppu gb_ppu;
  Timer gb_timer;
  sc_clock global_clk;
  sc_signal<bool> sig_unmap_rom;

  GbTop(sc_module_name name, const Options &options);
};
