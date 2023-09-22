#pragma once
/*******************************************************************************
 * Copyright (C) 2022 chciken/Niko
 * MIT License
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

  GbTop(sc_module_name name, std::filesystem::path game_path, std::filesystem::path boot_path,
        bool headless = false, bool wait_for_gdb = false, bool single_step = false);
};
