/**********************************************
 * MIT License
 * Copyright (c) 2020 chciken/Niko
**********************************************/

#include "cpu.h"

#include <fmt/format.h>

#include <bitset>
#include <string>

#include "./cpu_jumptable.cpp"
#include "./cpu_ops.cpp"

Cpu::Cpu(sc_module_name name, bool attachGdb):
    sc_module(name),
    attachGdb(attachGdb),
    init_socket("init_socket"),
    halted_(false),
    irq_vblank("irq_vblank"),
    reg_file(),
    gdb_server(this) {
  SC_THREAD(EvalInstr);
  dont_initialize();
  reg_file.PC = 0x0000;  // Iniitliazed to 0x100 [GBCPUman p.63], this the value after booting, init value is 0x0000
  reg_file.SP = 0x0000;  // Iniitliazed to 0xFFFE [GBCPUman p.64], or is this also done by the bootloader? TODO(niko): find out NOLINT
  sensitive << clk.pos();
  intr_master_enable = false;   // TODO(niko): Is this true?
  reg_intr_enable_dmi = nullptr;

  payload = MakeSharedPayloadPtr(tlm::TLM_IGNORE_COMMAND, 0x0000, nullptr);
}


void Cpu::SetRegFlag(u8 flag_mask, bool val) {
  DBG_LOG_CPU("SetRegFlag: 0b" << std::bitset<8>(flag_mask) << "=>" << val);
  if (val) {
    reg_file.F = flag_mask | reg_file.F.val();
  } else {
    reg_file.F = (~flag_mask) & reg_file.F.val();
  }
}

void Cpu::SetFlagC(bool val) {
  reg_file.F = SetBit(reg_file.F.val(), val, kIndCFlag);
}

void Cpu::SetFlagH(bool val) {
  reg_file.F = SetBit(reg_file.F.val(), val, kIndHFlag);
}

void Cpu::SetFlagN(bool val) {
  reg_file.F = SetBit(reg_file.F.val(), val, kIndNFlag);
}

void Cpu::SetFlagZ(bool val) {
  reg_file.F = SetBit(reg_file.F.val(), val, kIndZFlag);
}


bool Cpu::GetRegFlag(const u8 flag_mask) {
  return static_cast<bool>(flag_mask & reg_file.F.val());
}

std::string Cpu::RegFileToString() {
  return fmt::format("AF:{:04x}\nBC:{:04x}\nDE:{:04x}\n"
                     "HL:{:04x}\nSP:{:04x}\nPC{:04x}\n",
                     reg_file.AF, reg_file.BC, reg_file.DE,
                     reg_file.HL, reg_file.SP, reg_file.PC);
}

void Cpu::WriteBus(u16 addr, u8 data) {
  sc_time delay = sc_time(0, SC_NS);

  payload->set_command(tlm::TLM_WRITE_COMMAND);
  payload->set_address(addr);
  payload->set_data_ptr(reinterpret_cast<unsigned char*>(&data));
  init_socket->b_transport(*payload, delay);

  // assert(!payload->is_response_error());
  return;
}

void Cpu::WriteBusDebug(u16 addr, u8 data) {
  payload->set_command(tlm::TLM_WRITE_COMMAND);
  payload->set_address(addr);
  payload->set_data_ptr(reinterpret_cast<unsigned char*>(&data));
  init_socket->transport_dbg(*payload);

  if (payload->is_response_error()) {
    SC_REPORT_ERROR("TLM-2", "Response error from b_transport");
  }
  return;
}

u8 Cpu::ReadBus(u16 addr) {
  u8 data;
  sc_time delay = sc_time(0, SC_NS);
  payload->set_command(tlm::TLM_READ_COMMAND);
  payload->set_address(addr);
  payload->set_data_ptr(reinterpret_cast<unsigned char*>(&data));
  init_socket->b_transport(*payload, delay);
  if (payload->is_response_error()) {
    SC_REPORT_ERROR("TLM-2", "Response error from b_transport");
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
    DBG_LOG_CPU("Transport status is:" << payload->get_response_string());
    SC_REPORT_ERROR("TLM-2", fmt::format("Response error from transport_dbg!"
                    "Address=0x{:04x} Data=0x{:02x}", addr, data).c_str());
  }
  return data;
}

u8 Cpu::FetchNextInstrByte() {
  u8 val = ReadBus(reg_file.PC);
  ++reg_file.PC;
  return val;
}

u16 Cpu::FetchNext2InstrBytes() {
  u16 lsb = static_cast<u16>(ReadBus(reg_file.PC));
  ++reg_file.PC;
  u16 msb = static_cast<u16>(ReadBus(reg_file.PC)) << 8;
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
void Cpu::Init() {
  if (reg_intr_enable_dmi == nullptr) {
    tlm::tlm_dmi dmi_data;
    uint dummy_data;
    auto payload = MakeSharedPayloadPtr(tlm::TLM_READ_COMMAND, 0xffff, reinterpret_cast<void*>(&dummy_data));
    if (init_socket->get_direct_mem_ptr(*payload, dmi_data)) {
      reg_intr_enable_dmi = reinterpret_cast<u8*>(dmi_data.get_dmi_ptr());
    } else {
      throw std::runtime_error("Could not get interrupt enable register DMI!");
    }
    payload->set_address(0xff0f);
    if (init_socket->get_direct_mem_ptr(*payload, dmi_data)) {
      reg_intr_pending_dmi = reinterpret_cast<u8*>(dmi_data.get_dmi_ptr());
    } else {
      throw std::runtime_error("Could not get interrupt pending register DMI!");
    }
  }
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
  // Interrupts only take place if intr_master_enable (interrupt master enable IME) is true.
  u8 intr = *reg_intr_enable_dmi & *reg_intr_pending_dmi;
  if (intr_master_enable && intr) {
    intr_master_enable = false;  // Disable IME.
    *reg_intr_pending_dmi &= ~intr;  // Clear pending interrupt.
    halted_ = false;
    InstrPush(reg_file.PC);
    if (intr & gb_const::kMaskBit0) {
      DBG_LOG_CPU("Executing vblank ISR");
      reg_file.PC = 0x40;
      return;
    }
    if (intr & gb_const::kMaskBit1) {
      DBG_LOG_CPU("Executing LCDC status ISR");
      reg_file.PC = 0x48;
      return;
    }
    if (intr & gb_const::kMaskBit2) {
      DBG_LOG_CPU("Executing timer ISR");
      reg_file.PC = 0x50;
      return;
    }
    if (intr & gb_const::kMaskBit3) {
      DBG_LOG_CPU("Executing serial transfer ISR");
      reg_file.PC = 0x58;
      return;
    }
    if (intr & gb_const::kMaskBit4) {
      DBG_LOG_CPU("Executing joypad ISR");
      reg_file.PC = 0x60;
      return;
    }
  }
}
