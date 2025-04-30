/**********************************************
 * Apache License, Version 2.0
 * Copyright (c) 2025 chciken/Niko
**********************************************/

#include "cpu.h"

#include <bitset>
#include <format>
#include <string>

#include "cpu_jumptable.cpp"
#include "cpu_ops.cpp"

Cpu::Cpu(sc_module_name name, bool attach_gdb, bool single_step):
    sc_module(name),
    gdb_server(this),
    attach_gdb_(attach_gdb),
    single_step_(single_step) {
  SC_CTHREAD(DoMachineCycle, clk);
}

Cpu::~Cpu() {
}

void Cpu::SetFlagC(bool val) {
  reg_file.F = SetBit(reg_file.F, val, kIndCFlag);
}

void Cpu::SetFlagH(bool val) {
  reg_file.F = SetBit(reg_file.F, val, kIndHFlag);
}

void Cpu::SetFlagN(bool val) {
  reg_file.F = SetBit(reg_file.F, val, kIndNFlag);
}

void Cpu::SetFlagZ(bool val) {
  reg_file.F = SetBit(reg_file.F, val, kIndZFlag);
}

bool Cpu::GetFlagC() const {
  return static_cast<bool>(kMaskCFlag & reg_file.F);
}

bool Cpu::GetFlagH() const {
  return static_cast<bool>(kMaskHFlag & reg_file.F);
}

bool Cpu::GetFlagN() const {
  return static_cast<bool>(kMaskNFlag & reg_file.F);
}

bool Cpu::GetFlagZ() const {
  return static_cast<bool>(kMaskZFlag & reg_file.F);
}

void Cpu::WriteBus(u16 addr, u8 data) {
  static sc_time delay = sc_time(0, SC_NS);  // Dummy delay.

  payload->set_command(tlm::TLM_WRITE_COMMAND);
  payload->set_address(addr);
  payload->set_data_ptr(reinterpret_cast<unsigned char*>(&data));
  init_socket->b_transport(*payload, delay);
}

void Cpu::WriteBusDebug(u16 addr, u8 data) {
  payload->set_command(tlm::TLM_WRITE_COMMAND);
  payload->set_address(addr);
  payload->set_data_ptr(reinterpret_cast<unsigned char*>(&data));
  init_socket->transport_dbg(*payload);

  if (payload->is_response_error()) {
    SC_REPORT_ERROR("TLM-2", "Response error from b_transport");
  }
}

u8 Cpu::ReadBus(u16 addr, GbCommand::Cmd cmd) {
  this->gbcmd.cmd = cmd;
  static sc_time delay = sc_time(0, SC_NS);  // Dummy delay.
  u8 data;
  payload->set_command(tlm::TLM_READ_COMMAND);
  payload->set_address(addr);
  payload->set_data_ptr(reinterpret_cast<unsigned char*>(&data));
  init_socket->b_transport(*payload, delay);
  if (payload->is_response_error()) {
    DBG_LOG_CPU("Transport status is:" << payload->get_response_string());
    throw std::runtime_error(std::format("Response error from transport_dbg!\n"
                             "Address=0x{:04x} Data=0x{:02x}", addr, data).c_str());
  }
  return data;
}

u8 Cpu::ReadBusDebug(u16 addr) {
  u8 data;
  payload->set_command(tlm::TLM_READ_COMMAND);
  payload->set_address(addr);
  payload->set_data_ptr(reinterpret_cast<unsigned char*>(&data));
  init_socket->transport_dbg(*payload);
  if (payload->is_response_error()) {
    data = 0;
    std::cout << std::format("Warning: Reading unmapped address at 0x{:04x}\n", addr);
  }
  return data;
}

u8 Cpu::FetchNextInstrByte() {
  u8 val = ReadBus(reg_file.PC, GbCommand::kGbReadInst);
  ++reg_file.PC;
  return val;
}

u16 Cpu::FetchNext2InstrBytes() {
  gbcmd.cmd = GbCommand::kGbReadInst;
  u16 lsb = static_cast<u16>(ReadBus(reg_file.PC, GbCommand::kGbReadInst));
  ++reg_file.PC;
  u16 msb = static_cast<u16>(ReadBus(reg_file.PC, GbCommand::kGbReadInst)) << 8;
  ++reg_file.PC;
  return msb | lsb;
}

// Halts the CPU SystemC thread. Use Continue() to ...continue.
// Don't confuse this with the halt instruction!
// Is used by the GDB server to wait for a connection.
void Cpu::Halt() {
  DBG_LOG_CPU("CPU halted");
  halted_ = true;
}

// Use to continue after calling Halt().
void Cpu::Continue() {
  DBG_LOG_CPU("CPU continues");
  halted_ = false;
}

// Initialize interrupt enable and pending DMI.
void Cpu::start_of_simulation() {
  InterruptModule::start_of_simulation();
  payload = new tlm::tlm_generic_payload;
  payload->set_command(tlm::TLM_IGNORE_COMMAND);
  payload->set_address(0);
  payload->set_data_ptr(nullptr);
  payload->set_extension(&gbcmd);
}

// TODO(niko): What happens if there are multiple interrupts???
// source: http://imrannazar.com/gameboy-emulation-in-javascript:-interrupts
// This has to be after the execute cycle
// Get DMI pointer to 0xFFFF
// bit 0: vblank on/off
// bit 1: LCD stat on/off
// bit 2: timer on/off
// bit 3: serial on/off
// bit 4: joypad on/off
// Furthermore the register 0xFF0F indicates whether an interrupt is pending
// using the same order as 0xFFFF.
void Cpu::HandleInterrupts() {
  // Interrupts only take place if interrupt master enable (IME) is true.
  if (!intr_master_enable)
    return;

  u8 intr = *reg_intr_enable_dmi & *reg_intr_pending_dmi;
  if (intr) {
    intr_master_enable = false;  // Disable IME.
    InstrPush(reg_file.PC);
    if (intr & gb_const::kVBlankIf) {
      DBG_LOG_CPU("Executing vblank ISR");
      reg_file.PC = 0x40;
      *reg_intr_pending_dmi &= ~gb_const::kVBlankIf;
      return;
    }
    if (intr & gb_const::kLCDCIf) {
      DBG_LOG_CPU("Executing LCDC status ISR");
      reg_file.PC = 0x48;
      *reg_intr_pending_dmi &= ~gb_const::kLCDCIf;
      return;
    }
    if (intr & gb_const::kTimerOfIf) {
      DBG_LOG_CPU("Executing timer ISR");
      reg_file.PC = 0x50;
      *reg_intr_pending_dmi &= ~gb_const::kTimerOfIf;
      return;
    }
    if (intr & gb_const::kSerialIOIf) {
      DBG_LOG_CPU("Executing serial transfer ISR");
      reg_file.PC = 0x58;
      *reg_intr_pending_dmi &= ~gb_const::kSerialIOIf;
      return;
    }
    if (intr & gb_const::kJoypadIf) {
      DBG_LOG_CPU("Executing joypad ISR");
      reg_file.PC = 0x60;
      *reg_intr_pending_dmi &= ~gb_const::kJoypadIf;
      return;
    }
  }
}
