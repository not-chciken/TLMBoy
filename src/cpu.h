#pragma once
/*******************************************************************************
 * Apache License, Version 2.0
 * Copyright (c) 2022 chciken/Niko
 *
 * The SM83 CPU of the gameboy. Basically a Z80 clone with some special instructions
********************************************************************************/
#include <stdlib.h>

#include <iostream>
#include <memory>
#include <string>

#include "common.h"
#include "reg_file.h"
#include "gb_const.h"
#include "gdb_server.h"

struct Cpu : public sc_module {
  SC_HAS_PROCESS(Cpu);
  friend GdbServer;
  tlm_utils::simple_initiator_socket<Cpu, gb_const::kBusDataWidth> init_socket;
  sc_in_clk clk;

  // The Game Boy's register file.
  RegFile reg_file;

  // The states are a semihosting feature intended to communicate fail or success of tests.
  enum CpuState {
    kNominal,
    kTestPassed,
    kTestFailed,
  } cpu_state = kNominal;

  explicit Cpu(sc_module_name name, bool attachGdb = false);

 private:
  // Init() is called once at the beginning to setup the DMI pointers.
  void Init();

  // Interrupt master enable.
  bool intr_master_enable = false;
  // DMI pointer to the interrupt enable register at 0xffff.
  u8* reg_intr_enable_dmi = nullptr;
  // DMI pointer to register at 0xff0f indicating pending interrupts.
  u8* reg_intr_pending_dmi = nullptr;

  // Flag register setters and getter.
  void SetFlagC(bool val);
  void SetFlagH(bool val);
  void SetFlagN(bool val);
  void SetFlagZ(bool val);
  bool GetFlagC();
  bool GetFlagH();
  bool GetFlagN();
  bool GetFlagZ();

  // Write to bus/memory.
  void WriteBus(u16 addr, u8 data);
  // Write to bus/memory in debug mode. Also used by GDB.
  void WriteBusDebug(u16 addr, u8 data);
  // Read from bus/memory.
  u8 ReadBus(u16 addr);
  // Read from bus/memory in debug mode. Also used by GDB.
  u8 ReadBusDebug(u16 addr);
  // TODO(Niko): maybe static payload per function?
  std::shared_ptr<tlm::tlm_generic_payload> payload;

  // Executes on machine cycle (interrupts, fetch, decode, execute)
  void DoMachineCycle();
  void HandleInterrupts();
  u8 FetchNextInstrByte();
  u16 FetchNext2InstrBytes();

  // The GDB server can be used to establish a GDB remote connection.
  GdbServer gdb_server;
  // Halt the CPU. Used by the GDB server.
  void Halt();
  // Continue after halt. Used by the GDB server.
  void Continue();
  // If true, wait for a GDB remote connection before starting.
  bool attachGdb;
  // If true, GDB is currently halting the CPU
  bool halted_ = false;
  // TCP port for the GDB stub.
  const i32 gdb_port_ = 1337;

  // ALl of the SM83's instructions
  void InstrAddA(Reg<u8> &reg);
  void InstrAddA(Reg<u16> &addr_reg);
  void InstrAddACarry(Reg<u8> &reg);
  void InstrAddACarry(Reg<u16> &addr_reg);
  void InstrAddACarryImm();
  void InstrAddAImm();
  void InstrAddHl(Reg<u16> &reg);
  void InstrAddSp();
  void InstrAnd(Reg<u8> &reg);
  void InstrAnd(Reg<u16> &addr_reg);
  void InstrAndImm();
  void InstrBitN(uint bit_index, Reg<u8> &reg);
  void InstrBitN(uint bit_index, Reg<u16> &addr);
  void InstrCall();
  void InstrCallIf(bool cond);
  void InstrCCF();
  void InstrComp(Reg<u8> &reg);
  void InstrComp(Reg<u16> &reg);
  void InstrCompImm();
  void InstrComplement();
  void InstrDAA();
  void InstrDec(Reg<u8> &reg);
  void InstrDec(Reg<u16> &reg);
  void InstrDecAddr(Reg<u16> &addr);
  void InstrDI();
  void InstrEI();
  void InstrHalt();
  void InstrInc(Reg<u8> &reg);
  void InstrInc(Reg<u16> &reg);
  void InstrIncAddr(Reg<u16> &addr);
  void InstrJump();
  void InstrJump(Reg<u16> &addr_reg);
  void InstrJumpIf(bool cond);
  void InstrJumpAddr();
  void InstrJumpAddrIf(bool cond);
  void InstrLoad(Reg<u8> &reg, Reg<u16> &addr_reg);
  void InstrLoad(Reg<u8> &reg, const u16 &addr_val);
  void InstrLoadC();
  void InstrLoadImm(Reg<u8> &reg);
  void InstrLoadImm(Reg<u16> &reg);
  void InstrLoadInc(Reg<u8> &reg, Reg<u16> &addr_reg);
  void InstrLoadDec(Reg<u16> &addr_reg, Reg<u8> &reg);
  void InstrLoadH(Reg<u8> &reg);
  void InstrLoadHlSpRel();
  void InstrLoadSpHl();
  void InstrMov(Reg<u8> &reg_to, Reg<u8> &reg_from);
  void InstrNop();
  void InstrOr(Reg<u8> &reg);
  void InstrOr(Reg<u16> &addr_reg);
  void InstrOrImm();
  void InstrPop(Reg<u16> &reg);
  void InstrPopAF();
  void InstrPush(Reg<u16> &reg);
  void InstrResetBit(const uint bit, Reg<u8> &reg);
  void InstrResetBit(const uint bit, Reg<u16> &addr_reg);
  void InstrRet();
  void InstrRetI();
  void InstrRetIf(bool cond);
  void InstrRotLeft(Reg<u8> &reg);
  void InstrRotLeft(Reg<u16> &reg);
  void InstrRotLeftA();
  void InstrRotRight(Reg<u8> &reg);
  void InstrRotRight(Reg<u16> &reg);
  void InstrRlca();
  void InstrRlc(Reg<u8> &reg);
  void InstrRlc(const Reg<u16> &addr_reg);
  void InstrRrc(Reg<u8> &reg);
  void InstrRrc(const Reg<u16> &addr_reg);
  void InstrRra();
  void InstrRrca();
  void InstrRST(const u8 addr);
  void InstrSCF();
  void InstrSetBitN(uint bit_index, Reg<u8> &reg);
  void InstrSetBitN(uint bit_index, Reg<u16> &addr_reg);
  void InstrShiftRight(Reg<u8> &reg);
  void InstrShiftRight(Reg<u16> &addr_reg);
  void InstrSLA(Reg<u8> &reg);
  void InstrSLA(Reg<u16> &addr_reg);
  void InstrStore(const Reg<u16> &addr_reg, const Reg<u8> &reg);
  void InstrStore(const u8 &addr_reg, const Reg<u8> &reg);
  void InstrStore(const Reg<u8> &reg);
  void InstrStoreInc(Reg<u16> &addr_reg, const u8 val);
  void InstrStoreImm(Reg<u16> &addr);
  void InstrStoreDec(Reg<u16> &addr_reg, const u8 val);
  void InstrStoreH(Reg<u8> &reg);
  void InstrSubImm();
  void InstrSub(Reg<u8> &reg);
  void InstrSub(Reg<u16> &addr_reg);
  void InstrSubCarry(Reg<u8> &reg);
  void InstrSubCarry(Reg<u16> &addr_reg);
  void InstrSubCarryImm();
  void InstrSRA(Reg<u8> &reg);
  void InstrSRA(Reg<u16> &addr_reg);
  void InstrStoreSp();
  void InstrSwap(Reg<u8> &reg);
  void InstrSwap(Reg<u16> &addr_reg);
  void InstrXor(Reg<u8> &reg);
  void InstrXor(Reg<u16> &addr_reg);
  void InstrXorImm();

  // Not part of the original SM83 ISA. Used for semihosting.
  void InstrEmu();

  // Constants.
  // zero flag: math op is zero or two values match with CP instruction
  const u8 kMaskZFlag = 0b10000000;
  // subtract flag: last math instruction was a subtraction
  const u8 kMaskNFlag = 0b01000000;
  // half carry flag: carry occured from lower nibble in last math instruction
  const u8 kMaskHFlag = 0b00100000;
  // carry flag: carry occured from last math operation or A is smaller than value with CP instruction
  const u8 kMaskCFlag = 0b00010000;

  const u8 kIndCFlag = 4;  // Bit index of the C flag
  const u8 kIndHFlag = 5;  // Bit index of the H flag
  const u8 kIndNFlag = 6;  // Bit index of the N flag
  const u8 kIndZFlag = 7;  // Bit index of the Z flag
};
