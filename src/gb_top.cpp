/*******************************************************************************
 * Apache License, Version 2.0
 * Copyright (c) 2025 chciken/Niko
 ******************************************************************************/
#include "gb_top.h"

GbTop::GbTop(sc_module_name name, const Options& options)
    : sc_module(name),
      cartridge("cartridge", options.rom_path, options.boot_rom_path, options.symbol_file),
      apu("apu"),
      bus("bus"),
      cpu("cpu", options.wait_for_gdb, options.single_step),
      joy_pad("joy_pad"),
      video_ram(8192, "video_ram"),
      work_ram(4096, "work_ram"),
      work_ram_n(4096, "work_ram_n"),
      echo_ram(4096, "echo_work_ram_n", work_ram.GetDataPtr()),
      echo_ram_n(4096, "echo_ram_n", work_ram_n.GetDataPtr()),
      obj_attr_mem(160, "obj_attr_mem"),
      high_ram(127, "high_ram"),
      reg_if(1, "reg_if"),
      intr_enable(1, "intr_enable"),
      io_registers("io_registers"),
      ppu("ppu", options.headless, options.fps_cap, options.resolution_scaling, options.color_palette),
      serial("serial",reg_if.GetDataPtr()),
      timer("timer", reg_if.GetDataPtr()) {
  bus.AddBusMaster(&apu.init_socket);
  bus.AddBusMaster(&cpu.init_socket);
  bus.AddBusMaster(&ppu.init_socket);
  bus.AddBusMaster(&joy_pad.init_socket);
  bus.AddBusMaster(&io_registers.init_socket);
  cartridge.sig_unmap_rom_in(sig_unmap_rom);
  apu.sig_reload_length_square1_in(sig_reload_length_square1);
  apu.sig_reload_length_square2_in(sig_reload_length_square2);
  apu.sig_reload_length_noise_in(sig_reload_length_noise);
  apu.sig_reload_length_wave_in(sig_reload_length_wave);
  apu.sig_trigger_square1_in(sig_trigger_square1);
  apu.sig_trigger_square2_in(sig_trigger_square2);
  apu.sig_trigger_noise_in(sig_trigger_noise);
  apu.sig_trigger_wave_in(sig_trigger_wave);
  io_registers.sig_unmap_rom_out(sig_unmap_rom);
  io_registers.sig_reload_length_square1_out(sig_reload_length_square1);
  io_registers.sig_reload_length_square2_out(sig_reload_length_square2);
  io_registers.sig_reload_length_noise_out(sig_reload_length_noise);
  io_registers.sig_reload_length_wave_out(sig_reload_length_wave);
  io_registers.sig_trigger_square1_out(sig_trigger_square1);
  io_registers.sig_trigger_square2_out(sig_trigger_square2);
  io_registers.sig_trigger_noise_out(sig_trigger_noise);
  io_registers.sig_trigger_wave_out(sig_trigger_wave);

  bus.AddBusSlave(&cartridge.mbc->rom_socket_in, 0x0000, 0x7FFF);
  bus.AddBusSlave(&video_ram.targ_socket, 0x8000, 0x9FFF);
  bus.AddBusSlave(&cartridge.mbc->ram_socket_in, 0xA000, 0xBFFF);
  bus.AddBusSlave(&work_ram.targ_socket, 0xC000, 0xCFFF);
  bus.AddBusSlave(&work_ram_n.targ_socket, 0xD000, 0xDFFF);
  bus.AddBusSlave(&echo_ram.targ_socket, 0xE000, 0xEFFF);
  bus.AddBusSlave(&echo_ram_n.targ_socket, 0xF000, 0xFDFF);
  bus.AddBusSlave(&obj_attr_mem.targ_socket, 0xFE00, 0xFE9F);
  bus.AddBusSlave(&joy_pad.targ_socket, 0xFF00, 0xFF00);
  bus.AddBusSlave(&serial.targ_socket, 0xFF01, 0xFF03);
  bus.AddBusSlave(&timer.targ_socket, 0xFF04, 0xFF07);
  bus.AddBusSlave(&reg_if.targ_socket, 0xFF0F, 0xFF0F);
  bus.AddBusSlave(&io_registers.targ_socket, 0xFF10, 0xFF7F);
  bus.AddBusSlave(&high_ram.targ_socket, 0xFF80, 0xFFFE);
  bus.AddBusSlave(&intr_enable.targ_socket, 0xFFFF, 0xFFFF);
}
