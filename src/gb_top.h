#pragma once
/*******************************************************************************
 * Apache License, Version 2.0
 * Copyright (c) 2025 chciken/Niko
 *
 * This is the top level of the Game Boy.
 ******************************************************************************/
#include <filesystem>

#include "apu.h"
#include "bus.h"
#include "cartridge.h"
#include "common.h"
#include "cpu.h"
#include "game_info.h"
#include "generic_memory.h"
#include "io_registers.h"
#include "joypad.h"
#include "options.h"
#include "ppu.h"
#include "serial.h"
#include "timer.h"

struct GbTop : public sc_module {
  SC_HAS_PROCESS(GbTop);
  u8 reg_ie;
  Cartridge cartridge;
  Apu apu;
  Bus bus;
  Cpu cpu;
  JoyPad joy_pad;
  GenericMemory video_ram;
  GenericMemory work_ram;
  GenericMemory work_ram_n;
  GenericMemory echo_ram;
  GenericMemory echo_ram_n;
  GenericMemory obj_attr_mem;
  GenericMemory high_ram;
  GenericMemory reg_if;
  GenericMemory intr_enable;
  IoRegisters io_registers;
  Ppu ppu;
  Serial serial;
  Timer timer;
  sc_signal<bool> sig_unmap_rom;
  sc_signal<bool> sig_reload_length_square1;
  sc_signal<bool> sig_reload_length_square2;
  sc_signal<bool> sig_reload_length_wave;
  sc_signal<bool> sig_reload_length_noise;
  sc_signal<bool> sig_trigger_square1;
  sc_signal<bool> sig_trigger_square2;
  sc_signal<bool> sig_trigger_wave;
  sc_signal<bool> sig_trigger_noise;

  GbTop(sc_module_name name, const Options& options);
};
