/**********************************************
 * Apache License, Version 2.0
 * Copyright (c) 2025 chciken/Niko
**********************************************/

#include "cpu.h"
#include "utils.h"

// Halts the Game Boy until an interrupt is triggered.
void Cpu::InstrHalt() {
  do {
    wait(4);  // Busy waiting.
  } while ((*reg_intr_enable_dmi & *reg_intr_pending_dmi) == 0);
}

// NOP, does nothing.
void Cpu::InstrNop() {
  wait(4);
}

// LD reg,(u16)
void Cpu::InstrLoad(Reg<u8> &reg, const u16 &addr_val) {
  reg = ReadBus(addr_val, GbCommand::kGbReadData);
  wait(16);
}

// LD reg8,(reg16)
void Cpu::InstrLoad(Reg<u8> &reg, Reg<u16> &addr_reg) {
  reg = ReadBus(addr_reg, GbCommand::kGbReadData);
  wait(8);
}

// LD reg16,u16
void Cpu::InstrLoadImm(Reg<u16> &reg) {
  reg = FetchNext2InstrBytes();
  wait(12);
}

// LD reg8,u8
void Cpu::InstrLoadImm(Reg<u8> &reg) {
  reg = FetchNextInstrByte();
  wait(8);
}

// "LD reg8,(reg16+)
void Cpu::InstrLoadInc(Reg<u8> &reg, Reg<u16> &addr_reg) {
  reg = ReadBus(addr_reg, GbCommand::kGbReadData);
  ++addr_reg;
  wait(8);
}

// LD reg8,(reg16-)
void Cpu::InstrLoadDec(Reg<u16> &addr_reg, Reg<u8> &reg) {
  reg = ReadBus(addr_reg, GbCommand::kGbReadData);
  --addr_reg;
  wait(8);
}

// LD (reg16+),reg8
void Cpu::InstrStoreInc(Reg<u16> &addr_reg, const u8 val) {
  WriteBus(addr_reg, val);
  ++addr_reg;
  wait(8);
}

// LD (reg16-),reg8
void Cpu::InstrStoreDec(Reg<u16> &addr_reg, const u8 val) {
  WriteBus(addr_reg, val);
  --addr_reg;
  wait(8);
}

// LD (reg16),reg8
void Cpu::InstrStore(const Reg<u16> &addr_reg, const Reg<u8> &reg) {
  WriteBus(addr_reg, reg);
  wait(8);
}

// LD (reg8+0xFF),reg8
void Cpu::InstrStore(const u8 &addr_reg, const Reg<u8> &reg) {
  WriteBus(static_cast<u16>(addr_reg) + 0xFF00, reg);
  wait(8);
}

// LD (u16),reg8
void Cpu::InstrStore(const Reg<u8> &reg) {
  WriteBus(FetchNext2InstrBytes(), reg);
  wait(16);
}

// LD (reg16), u8
void Cpu::InstrStoreImm(Reg<u16> &addr) {
  WriteBus(addr, FetchNextInstrByte());
  wait(12);
}

// LDH (u8+0xFF00), reg8
void Cpu::InstrStoreH(Reg<u8> &reg) {
  WriteBus(static_cast<u16>(FetchNextInstrByte()) + 0xFF00, reg);
  wait(12);
}

// LDH reg8,(u8+0xFF00)
void Cpu::InstrLoadH(Reg<u8> &reg) {
  u16 addr = static_cast<u16>(FetchNextInstrByte()) + 0xFF00;
  reg = ReadBus(addr, GbCommand::kGbReadData);
  assert(addr >= 0xFF00);
  wait(12);
}

// LDHL SP,n
void Cpu::InstrLoadHlSpRel() {
  i8 imm = FetchNextInstrByte();
  u16 reg = reg_file.SP;
  int result = static_cast<int>(reg + imm);
  reg_file.HL = result;
  SetFlagC(((reg ^ imm ^ (result & 0xFFFF)) & 0x100) == 0x100);
  SetFlagH(((reg ^ imm ^ (result & 0xFFFF)) & 0x10) == 0x10);
  SetFlagN(false);
  SetFlagZ(false);
  wait(12);
}

// LD SP,HL
void Cpu::InstrLoadSpHl() {
  reg_file.SP = reg_file.HL;
  wait(8);
}

// INC reg8
void Cpu::InstrInc(Reg<u8> &reg) {
  ++reg;
  SetFlagZ((reg == 0));
  SetFlagN(false);
  SetFlagH((reg & 0x0F) == 0);
  wait(4);
}

// INC reg16
void Cpu::InstrInc(Reg<u16> &reg) {
  ++reg;
  wait(8);
}

// INC (reg16)
void Cpu::InstrIncAddr(Reg<u16> &addr) {
  u8 val = ReadBus(addr, GbCommand::kGbReadData);
  ++val;
  WriteBus(addr, val);
  SetFlagZ((val == 0));
  SetFlagN(false);
  SetFlagH((val & 0x0F) == 0);
  wait(12);
}

// DEC (reg16)
void Cpu::InstrDecAddr(Reg<u16> &addr) {
  u8 val = ReadBus(addr, GbCommand::kGbReadData);
  --val;
  WriteBus(addr, val);
  SetFlagH((val & 0x0F) == 0x0F);
  SetFlagN(true);
  SetFlagZ((val == 0));
  wait(12);
}

// Rotate A left. Old bit 7 to carry.
void Cpu::InstrRlca() {
  reg_file.A = reg_file.A << 1 | reg_file.A >> 7;
  SetFlagH(false);
  SetFlagZ(false);
  SetFlagN(false);
  SetFlagC((reg_file.A & 0x01));
  wait(4);
}

// Rotate A right. Old bit 0 to carry.
void Cpu::InstrRrca() {
  u8 carry_flag = reg_file.A & gb_const::kMaskBit0;
  reg_file.A = reg_file.A >> 1 | carry_flag << 7;
  SetFlagC(carry_flag);
  SetFlagH(false);
  SetFlagN(false);
  SetFlagZ(false);
  wait(4);
}

// Rotate A right. Old bit 0 to carry.
void Cpu::InstrRra() {
  u8 carry_flag = reg_file.A & gb_const::kMaskBit0;
  u8 old_carry_flag = GetFlagC();
  reg_file.A = reg_file.A >> 1 | old_carry_flag << 7;
  SetFlagC(carry_flag);
  SetFlagH(false);
  SetFlagN(false);
  SetFlagZ(false);
  wait(4);
}

// Rotate reg8 left. Old bit 7 to Carry flag.
void Cpu::InstrRlc(Reg<u8> &reg) {
  reg = reg << 1 | reg >> 7;
  SetFlagH(false);
  SetFlagZ(reg == 0);
  SetFlagN(false);
  SetFlagC((reg & 0x01));
  wait(8);
}

// Rotate (reg16) left. Old bit 7 to Carry flag.
void Cpu::InstrRlc(const Reg<u16> &addr_reg) {
  u8 dat = ReadBus(addr_reg, GbCommand::kGbReadData);
  dat = dat << 1 | dat >> 7;
  WriteBus(addr_reg, dat);
  SetFlagH(false);
  SetFlagZ(dat == 0);
  SetFlagN(false);
  SetFlagC((dat & 0x01));
  wait(16);
}

// Rotate reg8 right. Old bit 0 to carry.
void Cpu::InstrRrc(Reg<u8> &reg) {
  reg = reg >> 1 | reg << 7;
  SetFlagH(false);
  SetFlagZ(reg == 0);
  SetFlagN(false);
  SetFlagC((reg & 0x80));
  wait(8);
}

// Rotate (reg16) right. Old bit 0 to carry.
void Cpu::InstrRrc(const Reg<u16> &addr_reg) {
  u8 dat = ReadBus(addr_reg, GbCommand::kGbReadData);
  dat = dat >> 1 | dat << 7;
  WriteBus(addr_reg, dat);
  SetFlagH(false);
  SetFlagZ(dat == 0);
  SetFlagN(false);
  SetFlagC((dat & 0x80));
  wait(16);
}

// LD (u16),SP
void Cpu::InstrStoreSp() {
  u16 addr = FetchNext2InstrBytes();
  WriteBus(addr, reg_file.SPlsb);
  WriteBus(addr + 1, reg_file.SPmsb);
  wait(20);
}

// ADD HL,reg16
void Cpu::InstrAddHl(Reg<u16> &reg) {
  SetFlagH((reg_file.HL & 0x0fff) + (reg & 0x0fff) > 0x0fff);
  u16 old_val = reg_file.HL;
  reg_file.HL += reg;
  SetFlagN(false);
  SetFlagC(reg_file.HL < old_val);
  wait(8);
}

// ADD SP,i8
void Cpu::InstrAddSp() {
  i8 data = FetchNextInstrByte();
  u16 reg = reg_file.SP;
  int result = static_cast<int>(reg + data);
  reg_file.SP = static_cast<u16>(result);
  SetFlagH(((reg ^ data ^ (result & 0xFFFF)) & 0x10) == 0x10);
  SetFlagN(false);
  SetFlagC(((reg ^ data ^ (result & 0xFFFF)) & 0x100) == 0x100);
  SetFlagZ(false);
  wait(16);
}

// ADD A,reg8
void Cpu::InstrAddA(Reg<u8> &reg) {
  SetFlagH((reg_file.A & 0x0f) + (reg & 0x0f) > 0x0f);
  u8 old_val = reg;
  reg_file.A += reg;
  SetFlagN(false);
  SetFlagC(reg_file.A < old_val);
  SetFlagZ(reg_file.A == 0);
  wait(4);
}

// ADD A,(reg16)
void Cpu::InstrAddA(Reg<u16> &addr_reg) {
  u8 reg = ReadBus(addr_reg, GbCommand::kGbReadData);
  SetFlagH((reg_file.A & 0x0f) + (reg & 0x0f) > 0x0f);
  reg_file.A += reg;
  SetFlagN(false);
  SetFlagC(reg_file.A < reg);
  SetFlagZ(reg_file.A == 0);
  wait(8);
}

// ADD A,u8
void Cpu::InstrAddAImm() {
  u8 imm = FetchNextInstrByte();
  SetFlagH((reg_file.A & 0x0f) + (imm & 0x0f) > 0x0f);
  reg_file.A += imm;
  SetFlagN(false);
  SetFlagC(reg_file.A < imm);
  SetFlagZ(reg_file.A == 0);
  wait(8);
}

// ADC A,reg8: Add reg8 + carry flag to A.
void Cpu::InstrAddACarry(Reg<u8> &reg) {
  u8 carry_add = GetFlagC() ? 1 : 0;
  u8 old_val = reg_file.A;
  SetFlagH((reg_file.A & 0x0f) + (reg & 0x0f) + carry_add > 0x0f);
  reg_file.A += reg + carry_add;
  SetFlagN(false);
  SetFlagC((reg_file.A < old_val) || (reg_file.A == old_val && carry_add == 1));
  SetFlagZ(reg_file.A == 0);
  wait(4);
}

// ADC A,(reg16): Add (reg16) + carry flag to A.
void Cpu::InstrAddACarry(Reg<u16> &addr_reg) {
  u8 reg = ReadBus(addr_reg, GbCommand::kGbReadData);
  u8 carry_add = GetFlagC() ? 1 : 0;
  u8 old_val = reg_file.A;
  SetFlagH((reg_file.A & 0x0f) + (reg & 0x0f) + carry_add > 0x0f);
  reg_file.A += reg + carry_add;
  SetFlagN(false);
  SetFlagC(reg_file.A < old_val || (reg_file.A == old_val && carry_add == 1));
  SetFlagZ(reg_file.A == 0);
  wait(8);
}

// ADC A,u8: Add u8 + carry flag to A.
void Cpu::InstrAddACarryImm() {
  u8 reg = FetchNextInstrByte();
  u8 old_val = reg_file.A;
  u8 carry_add = GetFlagC() ? 1 : 0;
  SetFlagH((reg_file.A & 0x0f) + (reg & 0x0f) + carry_add > 0x0f);
  reg_file.A += reg + carry_add;
  SetFlagN(false);
  SetFlagC(reg_file.A < old_val || (reg_file.A == old_val && carry_add == 1));
  SetFlagZ(reg_file.A == 0);
  wait(8);
}

// SUB (d8): subtract immediate from register A.
void Cpu::InstrSubImm() {
  u8 pre_val = reg_file.A;
  int8_t imm = FetchNextInstrByte();
  reg_file.A -= imm;
  SetFlagC(reg_file.A > pre_val);
  SetFlagH(((pre_val & 0xf) - (imm & 0xf)) < 0);
  SetFlagZ(reg_file.A == 0);
  SetFlagN(true);
  wait(8);
}

// SUB reg8: Subtract reg8 from register A.
void Cpu::InstrSub(Reg<u8> &reg) {
  u8 pre_val = reg_file.A;
  u8 old_reg_val = reg;
  reg_file.A -= reg;
  SetFlagC(reg_file.A > pre_val);
  SetFlagH(((pre_val & 0xf) - (old_reg_val & 0xf)) < 0);
  SetFlagZ(reg_file.A == 0);
  SetFlagN(true);
  wait(4);
}

// SUB (reg16): Subtract (reg16) from register A.
void Cpu::InstrSub(Reg<u16> &addr_reg) {
  u8 reg = ReadBus(addr_reg, GbCommand::kGbReadData);
  u8 pre_val = reg_file.A;
  reg_file.A -= reg;
  SetFlagC(reg_file.A > pre_val);
  SetFlagH(((pre_val & 0xf) - (reg & 0xf)) < 0);
  SetFlagZ(reg_file.A == 0);
  SetFlagN(true);
  wait(8);
}

// SBC A,reg8: Subtract reg8 and carry from register A.
void Cpu::InstrSubCarry(Reg<u8> &reg) {
  u8 carry_sub = GetFlagC() ? 1 : 0;
  u8 old_val = reg_file.A;
  reg_file.A -= (reg + carry_sub);
  SetFlagC(reg_file.A > old_val || (reg_file.A == old_val && carry_sub == 1));
  SetFlagH(((old_val & 0xf) - (reg & 0xf) - carry_sub) < 0);
  SetFlagZ(reg_file.A == 0);
  SetFlagN(true);
  wait(4);
}

// SBC A,(reg16): Subtract (reg16) and carry from register A.
void Cpu::InstrSubCarry(Reg<u16> &addr_reg) {
  u8 reg = ReadBus(addr_reg, GbCommand::kGbReadData);
  u8 carry_sub = GetFlagC() ? 1 : 0;
  u8 old_val = reg_file.A;
  reg_file.A -= (reg + carry_sub);
  SetFlagC(reg_file.A > old_val || (reg_file.A == old_val && carry_sub == 1));
  SetFlagH(((old_val & 0xf) - (reg & 0xf) - carry_sub) < 0);
  SetFlagZ(reg_file.A == 0);
  SetFlagN(true);
  wait(8);
}

// SBC A, u8: Subtract immediate and carry form register A.
void Cpu::InstrSubCarryImm() {
  u8 dat = FetchNextInstrByte();
  u8 carry_sub = GetFlagC() ? 1 : 0;
  u8 old_val = reg_file.A;
  reg_file.A -= (dat + carry_sub);
  SetFlagC(reg_file.A > old_val || (reg_file.A == old_val && carry_sub == 1));
  SetFlagH(((old_val & 0xf) - (dat & 0xf) - carry_sub) < 0);
  SetFlagZ(reg_file.A == 0);
  SetFlagN(true);
  wait(8);
}

// CP A,reg8: Compare A with reg8.
// Basically an A-reg8  subtraction instruction but the results are thrown away.
void Cpu::InstrComp(Reg<u8> &reg) {
  u8 pre_val = reg_file.A;
  u8 result = reg_file.A - reg;
  SetFlagC(result > pre_val);
  SetFlagH(((pre_val & 0xf) - (reg & 0xf)) < 0);
  SetFlagZ(result == 0);
  SetFlagN(true);
  wait(4);
}

// CP A,(reg16): Compare A with (reg16).
// Basically an A-(reg16) subtraction instruction but the results are thrown away.
void Cpu::InstrComp(Reg<u16> &reg) {
  u8 pre_val = reg_file.A;
  u8 read_reg = ReadBus(reg, GbCommand::kGbReadData);
  u8 result = reg_file.A - read_reg;
  SetFlagC(result > pre_val);
  SetFlagH(((pre_val & 0xf) - (read_reg & 0xf)) < 0);
  SetFlagZ(result == 0);
  SetFlagN(true);
  wait(8);
}

// CP A,u8: Compare A with u8.
// Basically an A-u8 subtraction instruction but the results are thrown  away.
void Cpu::InstrCompImm() {
  u8 imm = FetchNextInstrByte();
  u8 pre_val = reg_file.A;
  u8 result = reg_file.A - imm;
  DBG_LOG_INST("d8 = 0x" << std::hex << static_cast<uint>(imm));
  SetFlagC(result > pre_val);
  SetFlagH(((pre_val & 0xf) - (imm & 0xf)) < 0);
  SetFlagZ(result == 0);
  SetFlagN(true);
  wait(8);
}

// DEC reg16
void Cpu::InstrDec(Reg<u16> &reg) {
  --reg;
  wait(8);
}

// DEC reg8
void Cpu::InstrDec(Reg<u8> &reg) {
  --reg;
  SetFlagH((reg & 0x0F) == 0x0F);
  SetFlagN(true);
  SetFlagZ((reg == 0));
  wait(4);
}

// XOR A,reg8
void Cpu::InstrXor(Reg<u8> &reg) {
  reg_file.A ^= reg;
  SetFlagC(false);
  SetFlagH(false);
  SetFlagN(false);
  SetFlagZ(reg_file.A == 0);
  wait(4);
}

// XOR A,(reg16)
void Cpu::InstrXor(Reg<u16> &addr_reg) {
  reg_file.A ^= ReadBus(addr_reg, GbCommand::kGbReadData);
  SetFlagC(false);
  SetFlagH(false);
  SetFlagN(false);
  SetFlagZ(reg_file.A == 0);
  wait(8);
}

// XOR A,u8
void Cpu::InstrXorImm() {
  reg_file.A ^= FetchNextInstrByte();
  SetFlagC(false);
  SetFlagH(false);
  SetFlagN(false);
  SetFlagZ(reg_file.A == 0);
  wait(8);
}

// AND A,reg8
void Cpu::InstrAnd(Reg<u8> &reg) {
  reg_file.A &= reg;
  SetFlagC(false);
  SetFlagH(true);
  SetFlagN(false);
  SetFlagZ(reg_file.A == 0);
  wait(4);
}

// AND A,reg16
void Cpu::InstrAnd(Reg<u16> &addr_reg) {
  reg_file.A &= ReadBus(addr_reg, GbCommand::kGbReadData);
  SetFlagC(false);
  SetFlagH(true);
  SetFlagN(false);
  SetFlagZ(reg_file.A == 0);
  wait(8);
}

// AND A,u8
void Cpu::InstrAndImm() {
  reg_file.A &= FetchNextInstrByte();
  SetFlagC(false);
  SetFlagH(true);
  SetFlagN(false);
  SetFlagZ(reg_file.A == 0);
  wait(8);
}

// OR A,reg8
void Cpu::InstrOr(Reg<u8> &reg) {
  reg_file.A |= reg;
  SetFlagC(false);
  SetFlagH(false);
  SetFlagN(false);
  SetFlagZ(reg_file.A == 0);
  wait(4);
}

// OR A,reg16
void Cpu::InstrOr(Reg<u16> &addr_reg) {
  reg_file.A |= ReadBus(addr_reg, GbCommand::kGbReadData);
  SetFlagC(false);
  SetFlagH(false);
  SetFlagN(false);
  SetFlagZ(reg_file.A == 0);
  wait(8);
}

// OR A,u8
void Cpu::InstrOrImm() {
  reg_file.A |= FetchNextInstrByte();
  SetFlagC(false);
  SetFlagH(false);
  SetFlagN(false);
  SetFlagZ(reg_file.A == 0);
  wait(8);
}

// JR PC+1+i8
void Cpu::InstrJump() {
  int tmp_pc = static_cast<int>(reg_file.PC);
  int tmp_instr = static_cast<int>(static_cast<int8_t>(FetchNextInstrByte()));
  reg_file.PC = static_cast<u16>(tmp_pc + 1 + tmp_instr);
  wait(12);
}

// JR reg16: Jump to address in reg16.
void Cpu::InstrJump(Reg<u16> &addr_reg) {
  reg_file.PC = addr_reg;
  wait(4);
}

// JR u16: Jump to u16.
void Cpu::InstrJumpAddr() {
  reg_file.PC = FetchNext2InstrBytes();
  wait(16);
}

// JP cond,reg: Jump to reg8+PC+1 if cond is true.
void Cpu::InstrJumpIf(bool cond) {
  if (cond) {
    int tmp_pc = static_cast<int>(reg_file.PC);
    int tmp_instr = static_cast<int>(static_cast<int8_t>(FetchNextInstrByte()));
    reg_file.PC = static_cast<u16>(tmp_pc + tmp_instr + 1);
    wait(12);
  } else {
    FetchNextInstrByte();
    wait(8);
  }
}

// JP cond,u16: Jump to u16 if cond is true.
void Cpu::InstrJumpAddrIf(bool cond) {
  u16 jmp_addr = FetchNext2InstrBytes();
  if (cond) {
    reg_file.PC = jmp_addr;
    wait(16);
  } else {
    wait(12);
  }
}

// BIT ind,reg8: set z flag if bit ind of register reg8 is 0.
// Note, this is a little counter-intuitive as the Z flag is set when the bit is 0.
void Cpu::InstrBitN(uint bit_index, Reg<u8> &reg) {
  SetFlagH(true);
  SetFlagN(false);
  SetFlagZ(!IsBitSet(reg, bit_index));
  wait(8);
}

// BIT ind,(reg16): set z flag bit b of address (reg16)
// Note, this is a little counter-intuitive as the Z flag is set when the bit is 0
void Cpu::InstrBitN(uint bit_index, Reg<u16> &addr) {
  SetFlagH(true);
  SetFlagN(false);
  SetFlagZ(!IsBitSet(ReadBus(addr, GbCommand::kGbReadData), bit_index));
  wait(12);
}

// SET ind,reg8
void Cpu::InstrSetBitN(uint bit_index, Reg<u8> &reg) {
  reg = SetBit(reg, true, bit_index);
  wait(8);
}

// SET ind,(reg16)
void Cpu::InstrSetBitN(uint bit_index, Reg<u16> &addr_reg) {
  u8 tmp = ReadBus(addr_reg, GbCommand::kGbReadData);
  tmp = SetBit(tmp, true, bit_index);
  WriteBus(addr_reg, tmp);
  wait(16);
}

// CALL u16: Push address of next instruction onto stack and then jump to u16.
// Similar to most architectures, the stack grows downward.
void Cpu::InstrCall() {
  u16 jmp_addr = FetchNext2InstrBytes();
  WriteBus(--reg_file.SP, reg_file.PCmsb);
  WriteBus(--reg_file.SP, reg_file.PClsb);
  reg_file.PC = jmp_addr;
  wait(24);
}

// CALL cond,u16: Push address of next instruction onto stack and then jump to u16 if cond is true.
// Similar to most architectures, the stack grows downward.
void Cpu::InstrCallIf(bool cond) {
  u16 jmp_addr = FetchNext2InstrBytes();
  if (cond) {
    --reg_file.SP;
    WriteBus(reg_file.SP, reg_file.PCmsb);
    --reg_file.SP;
    WriteBus(reg_file.SP, reg_file.PClsb);
    reg_file.PC = jmp_addr;
    wait(24);
  } else {
    wait(12);
  }
}

// RET: Pop two bytes from stack & jump to that address.
void Cpu::InstrRet() {
  u16 lsb = static_cast<u16>(ReadBus(reg_file.SP, GbCommand::kGbReadData));
  ++reg_file.SP;
  u16 msb = static_cast<u16>(ReadBus(reg_file.SP, GbCommand::kGbReadData));
  msb <<= 8;
  ++reg_file.SP;
  reg_file.PC = msb | lsb;
  wait(16);
}

// RETI: Pop two bytes from stack and jump to that address and enable interrupts.
void Cpu::InstrRetI() {
  u16 lsb = static_cast<u16>(ReadBus(reg_file.SP, GbCommand::kGbReadData));
  ++reg_file.SP;
  u16 msb = static_cast<u16>(ReadBus(reg_file.SP, GbCommand::kGbReadData));
  msb <<= 8;
  ++reg_file.SP;
  reg_file.PC = msb | lsb;
  intr_master_enable = true;
  wait(16);
}

// PUSH reg16: Push register reg16 onto stack. Decrement Stack Pointer (SP) by two.
void Cpu::InstrPush(Reg<u16> &reg) {
  --reg_file.SP;
  WriteBus(reg_file.SP, static_cast<u8>(reg >> 8));  // msb
  --reg_file.SP;
  WriteBus(reg_file.SP, static_cast<u8>(reg & 0x00FF));  // lsb
  wait(16);
}

// POP reg16: Pop two bytes off stack into register pair nn. Increment Stack Pointer (SP) by two.
void Cpu::InstrPop(Reg<u16> &reg) {
  u8 f_tmp = reg_file.F & 0x0F;  // The lower four registers always read as zero!
  u16 lsb = static_cast<u16>(ReadBus(reg_file.SP, GbCommand::kGbReadData));
  ++reg_file.SP;
  u16 msb = static_cast<u16>(ReadBus(reg_file.SP, GbCommand::kGbReadData));
  ++reg_file.SP;
  msb <<= 8;
  reg = msb | lsb;
  reg_file.F = (reg_file.F & 0xF0) | f_tmp;
  wait(12);
}

// LD reg8,reg8: Register to register transfer.
void Cpu::InstrMov(Reg<u8> &reg_to, Reg<u8> &reg_from) {
    reg_to = reg_from;
    wait(4);
  }

// RL reg8
void Cpu::InstrRotLeft(Reg<u8> &reg) {
  bool old_carry = GetFlagC();
  SetFlagC(reg & 0b10000000);
  reg <<= 1;
  reg |= (old_carry ? 1 : 0);
  SetFlagZ(reg == 0);
  SetFlagN(false);
  SetFlagH(false);
  wait(8);
}

// RL (reg16)
void Cpu::InstrRotLeft(Reg<u16> &addr_reg) {
  u8 dat = ReadBus(addr_reg, GbCommand::kGbReadData);
  bool old_carry = GetFlagC();
  SetFlagC(dat & 0b10000000);
  dat <<= 1;
  dat |= (old_carry ? 1 : 0);
  WriteBus(addr_reg, dat);
  SetFlagZ(dat == 0);
  SetFlagN(false);
  SetFlagH(false);
  wait(16);
}

// RR reg8
void Cpu::InstrRotRight(Reg<u8> &reg) {
  bool old_carry = GetFlagC();
  SetFlagC(reg & 1);
  reg = reg >> 1;
  reg |= (old_carry ? 0b10000000 : 0);
  SetFlagZ(reg == 0);
  SetFlagN(false);
  SetFlagH(false);
  wait(8);
}

// RR (reg16)
void Cpu::InstrRotRight(Reg<u16> &addr_reg) {
  u8 dat = ReadBus(addr_reg, GbCommand::kGbReadData);
  bool old_carry = GetFlagC();
  SetFlagC(dat & 1);
  dat = dat >> 1;
  dat |= (old_carry ? 0b10000000 : 0);
  WriteBus(addr_reg, dat);
  SetFlagZ(dat == 0);
  SetFlagN(false);
  SetFlagH(false);
  wait(16);
}

// RLA: Note, GBCPUMan and opcode missmatch (zero flag)
void Cpu::InstrRotLeftA() {
  bool old_carry = GetFlagC();
  SetFlagC(reg_file.A & 0b10000000);
  reg_file.A = reg_file.A << 1;
  reg_file.A = reg_file.A | (old_carry ? 1 : 0);
  SetFlagZ(false);
  SetFlagN(false);
  SetFlagH(false);
  wait(4);
}

// SRL reg8
void Cpu::InstrShiftRight(Reg<u8> &reg) {
  u8 tmp = reg;
  reg = reg >> 1;
  SetFlagC(tmp & gb_const::kMaskBit0);
  SetFlagZ(reg == 0);
  SetFlagN(false);
  SetFlagH(false);
  wait(8);
}

// SRL reg16
void Cpu::InstrShiftRight(Reg<u16> &addr_reg) {
  u8 dat = ReadBus(addr_reg, GbCommand::kGbReadData);
  u8 tmp = dat;
  dat = dat >> 1;
  WriteBus(addr_reg, dat);
  SetFlagC(tmp & gb_const::kMaskBit0);
  SetFlagZ(dat == 0);
  SetFlagN(false);
  SetFlagH(false);
  wait(16);
}

// DI
void Cpu::InstrDI() {
  intr_master_enable = false;
  wait(4);
}

// EI
void Cpu::InstrEI() {
  intr_master_enable = true;
  wait(4);
}

// RET cond: Pop two bytes from stack & jump to that address if cond is true
void Cpu::InstrRetIf(bool cond) {
  if (cond) {
    u16 lsb = static_cast<u16>(ReadBus(reg_file.SP, GbCommand::kGbReadData));
    ++reg_file.SP;
    u16 msb = static_cast<u16>(ReadBus(reg_file.SP, GbCommand::kGbReadData));
    msb <<= 8;
    ++reg_file.SP;
    reg_file.PC = msb | lsb;
    wait(20);
  } else {
    wait(8);
  }
}

// SWAP reg8
void Cpu::InstrSwap(Reg<u8> &reg) {
  u8 old_upper_nib = reg >> 4;
  u8 old_lower_nib = reg << 4;
  reg = old_lower_nib | old_upper_nib;
  SetFlagC(false);
  SetFlagZ(reg == 0);
  SetFlagN(false);
  SetFlagH(false);
  wait(8);
}

// SWAP reg16
void Cpu::InstrSwap(Reg<u16> &addr_reg) {
  u8 dat = ReadBus(addr_reg, GbCommand::kGbReadData);
  u8 old_upper_nib = dat >> 4;
  u8 old_lower_nib = dat << 4;
  dat = old_lower_nib | old_upper_nib;
  WriteBus(addr_reg, dat);
  SetFlagC(false);
  SetFlagZ(dat == 0);
  SetFlagN(false);
  SetFlagH(false);
  wait(16);
}

// RST u8
void Cpu::InstrRST(const u8 addr) {
  --reg_file.SP;
  WriteBus(reg_file.SP, reg_file.PCmsb);
  --reg_file.SP;
  WriteBus(reg_file.SP, reg_file.PClsb);
  reg_file.PC = addr;
  wait(16);  // TODO(niko): PDF and opcode table differ.
}

// RES bit, reg8
void Cpu::InstrResetBit(const uint bit, Reg<u8> &reg) {
  reg = reg & ~(1 << bit);
  wait(8);
}

// RES bit, (reg16)
void Cpu::InstrResetBit(const uint bit, Reg<u16> &addr_reg) {
  u8 res = ReadBus(addr_reg, GbCommand::kGbReadData);
  res = res & ~(1 << bit);
  WriteBus(addr_reg, res);
  wait(16);
}

// CPL: Complement of register A.
void Cpu::InstrComplement() {
  reg_file.A = ~reg_file.A;
  SetFlagH(true);
  SetFlagN(true);
  wait(4);
}

// SLA reg8: Shift left into carry.
void Cpu::InstrSLA(Reg<u8> &reg) {
  u8 carry_bit = reg & gb_const::kMaskBit7;
  reg = reg << 1;
  SetFlagC(carry_bit);
  SetFlagH(false);
  SetFlagN(false);
  SetFlagZ(reg == 0);
  wait(8);
}

// InstrSLA (reg16)
void Cpu::InstrSLA(Reg<u16> &addr_reg) {
  u8 dat = ReadBus(addr_reg, GbCommand::kGbReadData);
  u8 carry_bit = dat & gb_const::kMaskBit7;
  dat = dat << 1;
  WriteBus(addr_reg, dat);
  SetFlagC(carry_bit);
  SetFlagH(false);
  SetFlagN(false);
  SetFlagZ(dat == 0);
  wait(16);
}

// Not part of the original ISA. Stops the simulation.
// The value of register A determines the reason for stop.
void Cpu::InstrEmu() {
  switch (reg_file.B.val()) {
    case 0: cpu_state = kTestPassed; break;
    case 1: cpu_state = kTestFailed; break;
    case 2: sc_stop(); break;
    default: break;
  }
}

// DAA: Binary to binary coded decimal.
// Cool explanation here: https://ehaskins.com/2018-01-30%20Z80%20DAA/
void Cpu::InstrDAA() {
    u8 reg = reg_file.A;
    u8 correction = GetFlagC() ? 0x60 : 0x00;
    if (GetFlagH() || (!GetFlagN() && ((reg & 0x0F) > 9))) {
        correction |= 0x06;
    }
    if (GetFlagC() || (!GetFlagN() && (reg > 0x99))) {
        correction |= 0x60;
    }
    if (GetFlagN()) {
        reg = reg - correction;
    } else {
        reg = reg + correction;
    }
    if (((correction << 2) & 0x100) != 0) {
        SetFlagC(true);
    }
    SetFlagH(false);
    SetFlagZ(reg == 0);
    reg_file.A = reg;
    wait(4);
}

// SCF
void Cpu::InstrSCF() {
  SetFlagC(true);
  SetFlagH(false);
  SetFlagN(false);
  wait(4);
}

// CCF
void Cpu::InstrCCF() {
  SetFlagC(!GetFlagC());
  SetFlagH(false);
  SetFlagN(false);
  wait(4);
}

// Shift n right into Carry. MSB doesn't change.
void Cpu::InstrSRA(Reg<u8> &reg) {
  u8 carry_bit = reg & 1;
  u8 top_bit = reg & gb_const::kMaskBit7;
  reg = reg >> 1 | top_bit;
  SetFlagC(carry_bit);
  SetFlagH(false);
  SetFlagN(false);
  SetFlagZ(reg == 0);
  wait(8);
}

// SRA reg8
void Cpu::InstrSRA(Reg<u16> &addr_reg) {
  u8 dat = ReadBus(addr_reg, GbCommand::kGbReadData);
  u8 carry_bit = dat & 1;
  u8 top_bit = dat & gb_const::kMaskBit7;
  u8 result = (dat >> 1) | top_bit;
  WriteBus(addr_reg, result);
  SetFlagC(carry_bit);
  SetFlagH(false);
  SetFlagN(false);
  SetFlagZ(result == 0);
  wait(16);
}

// LD A,(C).
void Cpu::InstrLoadC() {
  reg_file.A = ReadBus(0xFF00 + reg_file.C, GbCommand::kGbReadData);
  wait(8);
}
