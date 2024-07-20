/*******************************************************************************
 * Apache License, Version 2.0
 * Copyright (c) 2023 chciken/Niko
 ******************************************************************************/
#include "gb_top.h"

GbTop::GbTop(sc_module_name name, const Options &options)
    : sc_module(name),
      cartridge("cartridge", options.rom_path, options.boot_rom_path, options.symbol_file),
      gb_bus("gb_bus"),
      gb_cpu("gb_cpu", options.wait_for_gdb, options.single_step),
      joy_pad("joy_pad"),
      video_ram(8192, "video_ram"),
      work_ram(4096, "work_ram"),
      work_ram_n(4096, "work_ram_n"),
      echo_ram(4096, "echo_work_ram_n", work_ram.GetDataPtr()),
      echo_ram_n(4096, "echo_ram_n", work_ram_n.GetDataPtr()),
      obj_attr_mem(160, "obj_attr_mem"),
      high_ram(127, "high_ram"),
      serial_transfer(3, "serial_transfer"),
      reg_if(1, "reg_if"),
      intr_enable(1, "intr_enable"),
      gb_ppu("gb_ppu", options.headless, options.fps_cap),
      gb_timer("gb_timer", reg_if.GetDataPtr()),
      global_clk("global_clock", gb_const::kNsPerClkCycle, SC_NS, 0.5) {

  gb_bus.AddBusMaster(&gb_cpu.init_socket);
  gb_bus.AddBusMaster(&gb_ppu.init_socket);
  gb_bus.AddBusMaster(&joy_pad.init_socket);
  gb_bus.AddBusMaster(&io_registers.init_socket);
  cartridge.sig_unmap_rom_in(sig_unmap_rom);
  io_registers.sig_unmap_rom_out(sig_unmap_rom);

  gb_bus.AddBusSlave(&cartridge.mbc->rom_socket_in, 0x0000, 0x7FFF);
  gb_bus.AddBusSlave(&video_ram.targ_socket,        0x8000, 0x9FFF);
  gb_bus.AddBusSlave(&cartridge.mbc->ram_socket_in, 0xA000, 0xBFFF);
  gb_bus.AddBusSlave(&work_ram.targ_socket,         0xC000, 0xCFFF);
  gb_bus.AddBusSlave(&work_ram_n.targ_socket,       0xD000, 0xDFFF);
  gb_bus.AddBusSlave(&echo_ram.targ_socket,         0xE000, 0xEFFF);
  gb_bus.AddBusSlave(&echo_ram_n.targ_socket,       0xF000, 0xFDFF);
  gb_bus.AddBusSlave(&obj_attr_mem.targ_socket,     0xFE00, 0xFE9F);
  gb_bus.AddBusSlave(&joy_pad.targ_socket,          0xFF00, 0xFF00);
  gb_bus.AddBusSlave(&serial_transfer.targ_socket,  0xFF01, 0xFF03);
  gb_bus.AddBusSlave(&gb_timer.targ_socket,         0xFF04, 0xFF07);
  gb_bus.AddBusSlave(&reg_if.targ_socket,           0xFF0F, 0xFF0F);
  gb_bus.AddBusSlave(&io_registers.targ_socket,     0xFF10, 0xFF7F);
  gb_bus.AddBusSlave(&high_ram.targ_socket,         0xFF80, 0xFFFE);
  gb_bus.AddBusSlave(&intr_enable.targ_socket,      0xFFFF, 0xFFFF);

  gb_cpu.clk(global_clk);
  gb_ppu.clk(global_clk);
  gb_timer.clk(global_clk);
}
