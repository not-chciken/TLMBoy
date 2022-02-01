/**********************************************
 * MIT License
 * Copyright (c) 2022 chciken/Niko
**********************************************/

#include "cpu.h"
#include "utils.h"

void Cpu::InstrHalt() {
  do {
    wait(4);  // TODO(niko): Maybe use a signal?
  } while ((*reg_intr_enable_dmi & *reg_intr_pending_dmi) == 0);
}

// NOP, does nothing
void Cpu::InstrNop() {
  wait(4);
}

void Cpu::InstrLoad(Reg<u8> &reg, const u16 &addr_val) {
  reg = ReadBus(addr_val);
  wait(16);
}

void Cpu::InstrLoad(Reg<u8> &reg, Reg<u16> &addr_reg) {
  reg = ReadBus(addr_reg);
  wait(8);
}

// LD reg,u16
void Cpu::InstrLoadImm(Reg<u16> &reg) {
  reg = FetchNext2InstrBytes();
  wait(12);
}

// LD reg, u8
void Cpu::InstrLoadImm(Reg<u8> &reg) {
  reg = FetchNextInstrByte();
  wait(8);
}

void Cpu::InstrLoadInc(Reg<u8> &reg, Reg<u16> &addr_reg) {
  reg = ReadBus(addr_reg);
  ++addr_reg;
  wait(8);
}

void Cpu::InstrLoadDec(Reg<u16> &addr_reg, Reg<u8> &reg) {
  reg = ReadBus(addr_reg);
  --addr_reg;
  wait(8);
}

void Cpu::InstrStoreInc(Reg<u16> &addr_reg, const u8 val) {
  WriteBus(addr_reg, val);
  ++addr_reg;
  wait(8);
}

void Cpu::InstrStoreDec(Reg<u16> &addr_reg, const u8 val) {
  WriteBus(addr_reg, val);
  --addr_reg;
  wait(8);
}

// LD (addr),reg
void Cpu::InstrStore(const Reg<u16> &addr_reg, Reg<u8> &reg) {
  WriteBus(addr_reg, reg);
  wait(8);
}

// LD (addr+0xFF),reg
void Cpu::InstrStore(const u8 &addr_reg, Reg<u8> &reg) {
  WriteBus(static_cast<u16>(addr_reg)+0xFF00, reg.val());
  wait(8);
}

void Cpu::InstrStore(Reg<u8> &reg) {
  WriteBus(FetchNext2InstrBytes(), reg.val());
  wait(16);
}

void Cpu::InstrStoreImm(Reg<u16> &addr) {
  WriteBus(addr, FetchNextInstrByte());
  wait(12);
}

void Cpu::InstrStoreH(Reg<u8> &reg) {
  WriteBus(static_cast<u16>(FetchNextInstrByte()) + 0xFF00, reg.val());
  wait(12);
}

void Cpu::InstrLoadH(Reg<u8> &reg) {
  u16 addr = static_cast<u16>(FetchNextInstrByte()) + 0xFF00;
  reg = ReadBus(addr);
  DBG_LOG_INST("reg = 0x" << std::hex << static_cast<uint>(reg));
  assert(addr >= 0xFF00);
  wait(12);
}

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

void Cpu::InstrLoadSpHl() {
  reg_file.SP = reg_file.HL.val();
  wait(8);
}

// INC <reg>
void Cpu::InstrInc(Reg<u8> &reg) {
  ++reg;
  SetFlagZ((reg == 0));
  SetFlagN(false);
  SetFlagH((reg & 0x0F) == 0);
  wait(4);
}

// INC <reg>
void Cpu::InstrInc(Reg<u16> &reg) {
  ++reg;
  wait(8);
}

void Cpu::InstrIncAddr(Reg<u16> &addr) {
  u8 val = ReadBus(addr);
  ++val;
  WriteBus(addr, val);
  SetFlagZ((val == 0));
  SetFlagN(false);
  SetFlagH((val & 0x0F) == 0);
  wait(12);
}

void Cpu::InstrDecAddr(Reg<u16> &addr) {
  u8 val = ReadBus(addr);
  --val;
  WriteBus(addr, val);
  SetFlagH((val & 0x0F) == 0x0F);
  SetFlagN(true);
  SetFlagZ((val == 0));
  wait(12);
}

void Cpu::InstrRlca() {
  reg_file.A = (reg_file.A.val() << 1) | (reg_file.A.val() >> (8 - 1));
  SetFlagH(false);
  SetFlagZ(false);
  SetFlagN(false);
  SetFlagC((reg_file.A & 0x01));  // TODO(niko): check this
  wait(4);
}

// Rotate A right through Carry flag.
void Cpu::InstrRrca() {
  u8 carry_flag = reg_file.A & gb_const::kMaskBit0;
  reg_file.A = (reg_file.A >> 1) | (carry_flag << 7);
  SetFlagC(carry_flag);
  SetFlagH(false);
  SetFlagN(false);
  SetFlagZ(false);

  wait(4);
}

void Cpu::InstrRra() {
  u8 carry_flag = reg_file.A & gb_const::kMaskBit0;
  u8 old_carry_flag = GetRegFlag(kMaskCFlag);

  reg_file.A = (reg_file.A >> 1) | (old_carry_flag << 7);

  SetFlagC(carry_flag);
  SetFlagH(false);
  SetFlagN(false);
  SetFlagZ(false);

  wait(4);
}

// Rotate n left. Old bit 7 to Carry flag.
void Cpu::InstrRlc(Reg<u8> &reg) {
  reg = (reg << 1) | (reg >> 7);

  SetFlagH(false);
  SetFlagZ(reg == 0);
  SetFlagN(false);
  SetFlagC((reg & 0x01));  // TODO(niko): check this
  wait(8);
}

void Cpu::InstrRlc(Reg<u16> &addr_reg) {
  u8 dat = ReadBus(addr_reg);
  dat = (dat << 1) | (dat >> 7);
  WriteBus(addr_reg, dat);

  SetFlagH(false);
  SetFlagZ(dat == 0);
  SetFlagN(false);
  SetFlagC((dat & 0x01));
  wait(16);
}

void Cpu::InstrRrc(Reg<u8> &reg) {
  reg = (reg.val() >> 1) | (reg.val() << (8 - 1));
  SetFlagH(false);
  SetFlagZ(reg == 0);
  SetFlagN(false);
  SetFlagC((reg & 0x80));
  wait(8);
}

void Cpu::InstrRrc(Reg<u16> &addr_reg) {
  u8 dat = ReadBus(addr_reg);
  dat = (dat >> 1) | (dat << (8 - 1));
  WriteBus(addr_reg, dat);
  SetFlagH(false);
  SetFlagZ(dat == 0);
  SetFlagN(false);
  SetFlagC((dat & 0x80));
  wait(16);
}

void Cpu::InstrRcca() {
  reg_file.A = (reg_file.A >> 1) | (reg_file.A << (8 - 1));
  SetFlagH(false);
  SetFlagZ(false);
  SetFlagN(false);
  SetFlagC((reg_file.A & 0x80));  // TODO(niko): check this
  wait(4);
}

void Cpu::InstrStoreSp() {
  u16 addr = FetchNext2InstrBytes();
  WriteBus(addr, reg_file.SPlsb);
  WriteBus(addr + 1, reg_file.SPmsb);
  wait(20);
}

void Cpu::InstrAddHl(Reg<u16> &reg) {
  SetFlagH((reg_file.HL & 0x0fff) + (reg & 0x0fff) > 0x0fff);
  u16 old_val = reg_file.HL;
  reg_file.HL += reg;
  SetFlagN(false);
  SetFlagC(reg_file.HL < old_val);
  wait(8);
}

void Cpu::InstrAddSp() {
  i8 data = FetchNextInstrByte();
  u16 reg = reg_file.SP;

  int result = static_cast<int>(reg + data);
  SetFlagH(((reg ^ data ^ (result & 0xFFFF)) & 0x10) == 0x10);
  SetFlagN(false);
  SetFlagC(((reg ^ data ^ (result & 0xFFFF)) & 0x100) == 0x100);
  SetFlagZ(false);

  reg_file.SP = static_cast<u16>(result);

  wait(16);
}

void Cpu::InstrAddA(Reg<u8> &reg) {
  SetFlagH((reg_file.A & 0x0f) + (reg & 0x0f) > 0x0f);
  u8 old_val = reg.val();
  reg_file.A += reg;
  SetFlagN(false);
  SetFlagC(reg_file.A < old_val);
  SetFlagZ(reg_file.A == 0);
  wait(4);
}

void Cpu::InstrAddAImm() {
  u8 imm = FetchNextInstrByte();
  SetFlagH((reg_file.A & 0x0f) + (imm & 0x0f) > 0x0f);
  reg_file.A += imm;
  SetFlagN(false);
  SetFlagC(reg_file.A < imm);  // TODO(niko): does this work? check
  SetFlagZ(reg_file.A == 0);
  wait(8);
}

// Add reg + carry flag to A
void Cpu::InstrAddACarry(Reg<u8> &reg) {
  u8 carry_add = GetRegFlag(kMaskCFlag) ? 1 : 0;
  u8 old_val = reg_file.A.val();
  SetFlagH((reg_file.A & 0x0f) + (reg & 0x0f) + carry_add > 0x0f);
  reg_file.A += reg + carry_add;
  SetFlagN(false);
  SetFlagC((reg_file.A < old_val) || (reg_file.A == old_val && carry_add == 1));  // TODO(niko): does this work? check
  SetFlagZ(reg_file.A == 0);
  wait(4);
}

void Cpu::InstrAddACarry(Reg<u16> &addr_reg) {
  u8 reg = ReadBus(addr_reg);
  u8 carry_add = GetRegFlag(kMaskCFlag) ? 1 : 0;
  u8 old_val = reg_file.A.val();
  SetFlagH((reg_file.A & 0x0f) + (reg & 0x0f) + carry_add > 0x0f);
  reg_file.A += reg + carry_add;
  SetFlagN(false);
  SetFlagC(reg_file.A < old_val || (reg_file.A == old_val && carry_add == 1));  // TODO(niko): does this work? check
  SetFlagZ(reg_file.A == 0);
  wait(8);
}


void Cpu::InstrAddACarryImm() {
  u8 reg = FetchNextInstrByte();
  u8 old_val = reg_file.A.val();
  u8 carry_add = GetRegFlag(kMaskCFlag) ? 1 : 0;

  SetFlagH((reg_file.A & 0x0f) + (reg & 0x0f) + carry_add > 0x0f);
  reg_file.A += reg + carry_add;
  SetFlagN(false);
  SetFlagC(reg_file.A < old_val || (reg_file.A == old_val && carry_add == 1));  // TODO(niko): does this work? check
  SetFlagZ(reg_file.A == 0);
  wait(8);
}

void Cpu::InstrAddA(Reg<u16> &addr_reg) {
  u8 reg = ReadBus(addr_reg);
  SetFlagH((reg_file.A & 0x0f) + (reg & 0x0f) > 0x0f);
  reg_file.A += reg;
  SetFlagN(false);
  SetFlagC(reg_file.A < reg);  // TODO(niko): does this work? check
  SetFlagZ(reg_file.A == 0);
  wait(8);
}

// Subtract immediate from register a
void Cpu::InstrSubImm() {
  u8 pre_val = reg_file.A;
  int8_t imm = FetchNextInstrByte();
  reg_file.A -= imm;
  SetFlagC(reg_file.A > pre_val);
  SetFlagH(((pre_val & 0xf) - (imm & 0xf)) < 0);  // TODO(niko): check
  SetFlagZ(reg_file.A == 0);
  SetFlagN(true);
  wait(8);
}

// Subtract reg from register a
void Cpu::InstrSub(Reg<u8> &reg) {
  u8 pre_val = reg_file.A;
  u8 old_reg_val = reg;
  reg_file.A -= reg;
  SetFlagC(reg_file.A > pre_val);
  SetFlagH(((pre_val & 0xf) - (old_reg_val & 0xf)) < 0);  // TODO(niko): check
  SetFlagZ(reg_file.A == 0);
  SetFlagN(true);
  wait(4);
}

// Subtract (addr_reg) from register a
void Cpu::InstrSub(Reg<u16> &addr_reg) {
  u8 reg = ReadBus(addr_reg);
  u8 pre_val = reg_file.A;
  reg_file.A -= reg;
  SetFlagC(reg_file.A > pre_val);
  SetFlagH(((pre_val & 0xf) - (reg & 0xf)) < 0);  // TODO(niko): check
  SetFlagZ(reg_file.A == 0);
  SetFlagN(true);
  wait(8);
}

// Subtract reg and carry from register a
void Cpu::InstrSubCarry(Reg<u8> &reg) {
  u8 carry_sub = GetRegFlag(kMaskCFlag) ? 1 : 0;
  u8 old_val = reg_file.A;

  reg_file.A -= (reg + carry_sub);
  SetFlagC(reg_file.A > old_val || (reg_file.A == old_val && carry_sub == 1));
  SetFlagH(((old_val & 0xf) - (reg.val() & 0xf) - carry_sub) < 0);  // TODO(niko): check
  SetFlagZ(reg_file.A == 0);
  SetFlagN(true);
  wait(4);
}

// Subtract (addr_reg) and carry from register a
void Cpu::InstrSubCarry(Reg<u16> &addr_reg) {
  u8 reg = ReadBus(addr_reg);
  u8 carry_sub = GetRegFlag(kMaskCFlag) ? 1 : 0;
  u8 old_val = reg_file.A;
  reg_file.A -= (reg + carry_sub);
  SetFlagC(reg_file.A > old_val || (reg_file.A == old_val && carry_sub == 1));
  SetFlagH(((old_val & 0xf) - (reg & 0xf) - carry_sub) < 0);  // TODO(niko): check
  SetFlagZ(reg_file.A == 0);
  SetFlagN(true);
  wait(8);
}

void Cpu::InstrSubCarryImm() {
  u8 dat = FetchNextInstrByte();
  u8 carry_sub = GetRegFlag(kMaskCFlag) ? 1 : 0;
  u8 old_val = reg_file.A;
  reg_file.A -= (dat + carry_sub);
  SetFlagC(reg_file.A > old_val || (reg_file.A == old_val && carry_sub == 1));
  SetFlagH(((old_val & 0xf) - (dat & 0xf) - carry_sub) < 0);
  SetFlagZ(reg_file.A == 0);
  SetFlagN(true);
  wait(8);
}

// Compare A with n. This is basically an A - n  subtraction instruction
// but the results are thrown away.
void Cpu::InstrComp(Reg<u8> &reg) {
  u8 pre_val = reg_file.A.val();
  u8 result = reg_file.A.val() - reg;
  SetFlagC(result > pre_val);
  SetFlagH(((pre_val & 0xf) - (reg & 0xf)) < 0);  // TODO(niko): check
  SetFlagZ(result == 0);
  SetFlagN(true);
  wait(4);
}

void Cpu::InstrComp(Reg<u16> &reg) {
  u8 pre_val = reg_file.A.val();
  u8 read_reg = ReadBus(reg);
  u8 result = reg_file.A.val() - read_reg;
  SetFlagC(result > pre_val);
  SetFlagH(((pre_val & 0xf) - (read_reg & 0xf)) < 0);  // TODO(niko): check
  SetFlagZ(result == 0);
  SetFlagN(true);
  wait(8);
}

// Compare A with immediate value. This is basically an A - imm  subtraction instruction
// but the results are thrown  away.
void Cpu::InstrCompImm() {
  u8 imm = FetchNextInstrByte();
  u8 pre_val = reg_file.A;
  u8 result = reg_file.A - imm;
  DBG_LOG_INST("d8 = 0x" << std::hex << static_cast<uint>(imm));
  SetFlagC(result > pre_val);
  SetFlagH(((pre_val & 0xf) - (imm & 0xf)) < 0);  // TODO(niko): check
  SetFlagZ(result == 0);
  SetFlagN(true);
  wait(8);
}

void Cpu::InstrDec(Reg<u16> &reg) {
  --reg;
  wait(8);
}

void Cpu::InstrDec(Reg<u8> &reg) {
  --reg;
  DBG_LOG_INST("reg = 0x" << std::hex << static_cast<uint>(reg));
  SetFlagH((reg & 0x0F) == 0x0F);
  SetFlagN(true);
  SetFlagZ((reg == 0));
  wait(4);
}

void Cpu::InstrXor(Reg<u8> &reg) {
  reg_file.A ^= reg;
  SetFlagC(false);
  SetFlagH(false);
  SetFlagN(false);
  SetFlagZ(reg_file.A.val() == 0);
  wait(4);
}

void Cpu::InstrXor(Reg<u16> &addr_reg) {
  reg_file.A ^= ReadBus(addr_reg);
  SetFlagC(false);
  SetFlagH(false);
  SetFlagN(false);
  SetFlagZ(reg_file.A == 0);
  wait(8);
}

void Cpu::InstrXorImm() {
  reg_file.A ^= FetchNextInstrByte();
  SetFlagC(false);
  SetFlagH(false);
  SetFlagN(false);
  SetFlagZ(reg_file.A.val() == 0);
  wait(8);
}

void Cpu::InstrAnd(Reg<u8> &reg) {
  reg_file.A &= reg;
  SetFlagC(false);
  SetFlagH(true);
  SetFlagN(false);
  SetFlagZ(reg_file.A == 0);
  wait(4);
}

void Cpu::InstrAnd(Reg<u16> &addr_reg) {
  reg_file.A &= ReadBus(addr_reg);
  SetFlagC(false);
  SetFlagH(true);
  SetFlagN(false);
  SetFlagZ(reg_file.A == 0);
  wait(8);
}

void Cpu::InstrAndImm() {
  reg_file.A &= FetchNextInstrByte();
  SetFlagC(false);
  SetFlagH(true);
  SetFlagN(false);
  SetFlagZ(reg_file.A == 0);
  wait(8);
}


void Cpu::InstrOr(Reg<u8> &reg) {
  reg_file.A |= reg;
  SetFlagC(false);
  SetFlagH(false);
  SetFlagN(false);
  SetFlagZ(reg_file.A == 0);
  wait(4);
}

void Cpu::InstrOr(Reg<u16> &addr_reg) {
  reg_file.A |= ReadBus(addr_reg);
  SetFlagC(false);
  SetFlagH(false);
  SetFlagN(false);
  SetFlagZ(reg_file.A == 0);
  wait(8);
}

void Cpu::InstrOrImm() {
  reg_file.A |= FetchNextInstrByte();
  SetFlagC(false);
  SetFlagH(false);
  SetFlagN(false);
  SetFlagZ(reg_file.A == 0);
  wait(8);
}

// TODO(niko): figure out the +1
void Cpu::InstrJump() {
  int tmp_pc = static_cast<int>(reg_file.PC);
  int tmp_instr = static_cast<int>(static_cast<int8_t>(FetchNextInstrByte()));
  reg_file.PC = static_cast<u16>(tmp_pc + 1 + tmp_instr);
  wait(12);
}

void Cpu::InstrJump(Reg<u16> &addr_reg) {
  reg_file.PC = addr_reg.val();
  wait(4);
}

void Cpu::InstrJumpAddr() {
  reg_file.PC = FetchNext2InstrBytes();
  wait(16);
}

// Jump to given address if no zero flag
// Note, need a lot of casting due to a signed value
void Cpu::InstrJumpNz() {
  if (!GetRegFlag(kMaskZFlag)) {
    int tmp_pc = static_cast<int>(reg_file.PC);
    int tmp_instr = static_cast<int>(static_cast<int8_t>(FetchNextInstrByte()));
    reg_file.PC = static_cast<u16>(tmp_pc + tmp_instr + 1);
    wait(12);
  } else {
    FetchNextInstrByte();
    wait(8);
  }
}

void Cpu::InstrJumpNzAddr() {
  if (!GetRegFlag(kMaskZFlag)) {
    reg_file.PC = FetchNext2InstrBytes();
    wait(16);
  } else {
    FetchNext2InstrBytes();
    wait(12);
  }
}


void Cpu::InstrJumpNc() {
  if (!GetRegFlag(kMaskCFlag)) {
    int tmp_pc = static_cast<int>(reg_file.PC);
    int tmp_instr = static_cast<int>(static_cast<int8_t>(FetchNextInstrByte()));
    reg_file.PC = static_cast<u16>(tmp_pc + tmp_instr + 1);
    wait(12);
  } else {
    FetchNextInstrByte();
    wait(8);
  }
}

void Cpu::InstrJumpZ() {
  if (GetRegFlag(kMaskZFlag)) {
    int tmp_pc = static_cast<int>(reg_file.PC);
    int tmp_instr = static_cast<int>(static_cast<int8_t>(FetchNextInstrByte()));
    reg_file.PC = static_cast<u16>(tmp_pc + tmp_instr + 1);
    wait(12);
  } else {
    FetchNextInstrByte();
    wait(8);
  }
}

void Cpu::InstrJumpC() {
  if (GetRegFlag(kMaskCFlag)) {
    int tmp_pc = static_cast<int>(reg_file.PC);
    int tmp_instr = static_cast<int>(static_cast<int8_t>(FetchNextInstrByte()));
    reg_file.PC = static_cast<u16>(tmp_pc + tmp_instr + 1);
    wait(12);
  } else {
    FetchNextInstrByte();
    wait(8);
  }
}

void Cpu::InstrJumpAddrIf(bool cond) {
  u16 jmp_addr = FetchNext2InstrBytes();
  if (cond) {
    reg_file.PC = jmp_addr;
    wait(16);
  } else {
    wait(12);
  }
}

// Set if bit b of register r is 0
// Note, this is a little counter-intuitive as the Z flag is set when the bit is 0
void Cpu::InstrBitN(uint bit_index, Reg<u8> &reg) {
  SetFlagH(true);
  SetFlagN(false);
  SetFlagZ(!IsBitSet(reg.val(), bit_index));
  wait(8);
}

// Set bit b of register reg
// Note, this is a little counter-intuitive as the Z flag is set when the bit is 0
void Cpu::InstrBitN(uint bit_index, Reg<u16> &addr) {
  SetFlagH(true);
  SetFlagN(false);
  SetFlagZ(!IsBitSet(ReadBus(addr.val()), bit_index));
  wait(12);
}


void Cpu::InstrSetBitN(uint bit_index, Reg<u8> &reg) {
  reg = SetBit(reg.val(), true, bit_index);
  wait(8);
}

void Cpu::InstrSetBitN(uint bit_index, Reg<u16> &addr_reg) {
  u8 tmp = ReadBus(addr_reg);
  tmp = SetBit(tmp, true, bit_index);
  WriteBus(addr_reg, tmp);
  wait(16);
}

// Push address of next instruction onto stack and then jump to address nn.
// Similar to most architectures, the stack grows downward.
void Cpu::InstrCall() {
  u16 jmp_addr = FetchNext2InstrBytes();
  WriteBus(--reg_file.SP, reg_file.PCmsb);
  WriteBus(--reg_file.SP, reg_file.PClsb);
  reg_file.PC = jmp_addr;
  wait(24);
}

// Push address of next instruction onto stack and then jump to address nn if condition is fulfilled.
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

// Pop two bytes from stack & jump to that address
void Cpu::InstrRet() {
  u16 lsb = static_cast<u16>(ReadBus(reg_file.SP.val()));
  ++reg_file.SP;
  u16 msb = static_cast<u16>(ReadBus(reg_file.SP.val()));
  msb <<= 8;
  ++reg_file.SP;
  reg_file.PC = msb | lsb;
  wait(16);
}

// Pop two bytes from stack & jump to that address and enable interrupts
void Cpu::InstrRetI() {
  u16 lsb = static_cast<u16>(ReadBus(reg_file.SP.val()));
  ++reg_file.SP;
  u16 msb = static_cast<u16>(ReadBus(reg_file.SP.val()));
  msb <<= 8;
  ++reg_file.SP;
  reg_file.PC = msb | lsb;
  intr_master_enable = true;
  wait(16);
}

// Push register pair nn onto stack. Decrement Stack Pointer (SP) twice
void Cpu::InstrPush(Reg<u16> &reg) {
  --reg_file.SP;
  WriteBus(reg_file.SP.val(), static_cast<u8>(reg.val() >> 8));  // msb
  --reg_file.SP;
  WriteBus(reg_file.SP.val(), static_cast<u8>(reg.val() & 0x00FF));  // lsb
  wait(16);
}

// Pop two bytes off stack into register pair nn. Increment Stack Pointer (SP) twice
void Cpu::InstrPop(Reg<u16> &reg) {
  u8 f_tmp = reg_file.F & 0x0F;  // The lower four registers always read as zero!
  u16 lsb = static_cast<u16>(ReadBus(reg_file.SP.val()));
  ++reg_file.SP;
  u16 msb = static_cast<u16>(ReadBus(reg_file.SP.val()));
  ++reg_file.SP;
  msb <<= 8;
  reg = msb | lsb;
  reg_file.F = (reg_file.F.val() & 0xF0) | f_tmp;
  wait(12);
}

void Cpu::InstrMov(Reg<u8> &reg_to, Reg<u8> &reg_from) {
    reg_to = reg_from.val();
    wait(4);
  }

void Cpu::InstrRotLeft(Reg<u8> &reg) {
  bool old_carry = GetRegFlag(kMaskCFlag);
  SetFlagC(reg & 0b10000000);

  reg <<= 1;
  reg |= (old_carry ? 1 : 0);  // TODO(niko): check

  SetFlagZ(reg == 0);
  SetFlagN(false);
  SetFlagH(false);
  wait(8);
}

void Cpu::InstrRotLeft(Reg<u16> &addr_reg) {
  u8 dat = ReadBus(addr_reg);
  bool old_carry = GetRegFlag(kMaskCFlag);
  SetFlagC(dat & 0b10000000);

  dat <<= 1;
  dat |= (old_carry ? 1 : 0);
  WriteBus(addr_reg, dat);

  SetFlagZ(dat == 0);
  SetFlagN(false);
  SetFlagH(false);
  wait(16);
}

void Cpu::InstrRotRight(Reg<u8> &reg) {
  bool old_carry = GetRegFlag(kMaskCFlag);
  SetFlagC(reg & 1);

  reg = reg >> 1;
  reg |= (old_carry ? 0b10000000 : 0);  // TODO(niko): check

  SetFlagZ(reg == 0);
  SetFlagN(false);
  SetFlagH(false);
  wait(8);
}

void Cpu::InstrRotRight(Reg<u16> &addr_reg) {
  u8 dat = ReadBus(addr_reg);
  bool old_carry = GetRegFlag(kMaskCFlag);
  SetFlagC(dat & 1);

  dat = dat >> 1;
  dat |= (old_carry ? 0b10000000 : 0);
  WriteBus(addr_reg, dat);

  SetFlagZ(dat == 0);
  SetFlagN(false);
  SetFlagH(false);
  wait(16);
}

// gameboy cpu man and opcode missmatch (zero flag)
void Cpu::InstrRotLeftA() {
  bool old_carry = GetRegFlag(kMaskCFlag);
  SetFlagC(reg_file.A.val() & 0b10000000);

  reg_file.A.val(reg_file.A.val() << 1);
  reg_file.A.val(reg_file.A.val() | (old_carry ? 1 : 0));  // TODO(niko): check

  SetFlagZ(false);
  SetFlagN(false);
  SetFlagH(false);
  wait(4);
}

void Cpu::InstrShiftRight(Reg<u8> &reg) {
  u8 tmp = reg;
  reg = reg.val() >> 1;

  SetFlagC(tmp & gb_const::kMaskBit0);
  SetFlagZ(reg == 0);
  SetFlagN(false);
  SetFlagH(false);
  wait(8);
}

void Cpu::InstrShiftRight(Reg<u16> &addr_reg) {
  u8 dat = ReadBus(addr_reg);
  u8 tmp = dat;
  dat = dat >> 1;
  WriteBus(addr_reg, dat);

  SetFlagC(tmp & gb_const::kMaskBit0);
  SetFlagZ(dat == 0);
  SetFlagN(false);
  SetFlagH(false);
  wait(16);
}

void Cpu::InstrDI() {
  intr_master_enable = false;
  wait(4);
}

void Cpu::InstrEI() {
  intr_master_enable = true;  // TODO(niko): according to docu EI takes effect in the NEXT cycle
  wait(4);
}

// Pop two bytes from stack & jump to that address if non-zero
void Cpu::InstrRetNz() {
  if (!GetRegFlag(kMaskZFlag)) {
    u16 lsb = static_cast<u16>(ReadBus(reg_file.SP));
    ++reg_file.SP;
    u16 msb = static_cast<u16>(ReadBus(reg_file.SP));
    msb <<= 8;
    ++reg_file.SP;
    reg_file.PC = msb | lsb;
    wait(20);
  } else {
    wait(8);
  }
}

void Cpu::InstrRetIf(bool cond) {
  if (cond) {
    u16 lsb = static_cast<u16>(ReadBus(reg_file.SP));
    ++reg_file.SP;
    u16 msb = static_cast<u16>(ReadBus(reg_file.SP));
    msb <<= 8;
    ++reg_file.SP;
    reg_file.PC = msb | lsb;
    wait(20);
  } else {
    wait(8);
  }
}

// Pop two bytes from stack & jump to that address if carry is not zero
void Cpu::InstrRetNc() {
  if (!GetRegFlag(kMaskCFlag)) {
    u16 lsb = static_cast<u16>(ReadBus(reg_file.SP));
    ++reg_file.SP;
    u16 msb = static_cast<u16>(ReadBus(reg_file.SP));
    msb <<= 8;
    ++reg_file.SP;
    reg_file.PC = msb | lsb;
    wait(20);
  } else {
    wait(8);
  }
}

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

void Cpu::InstrSwap(Reg<u16> &addr_reg) {
  u8 dat = ReadBus(addr_reg.val());
  u8 old_upper_nib = dat >> 4;
  u8 old_lower_nib = dat << 4;
  dat = old_lower_nib | old_upper_nib;
  WriteBus(addr_reg.val(), dat);
  SetFlagC(false);
  SetFlagZ(dat == 0);
  SetFlagN(false);
  SetFlagH(false);
  wait(16);
}

void Cpu::InstrRST(const u8 addr) {
  --reg_file.SP;
  WriteBus(reg_file.SP, reg_file.PCmsb);
  --reg_file.SP;
  WriteBus(reg_file.SP, reg_file.PClsb);
  reg_file.PC = addr;
  wait(16);  // TODO(niko): PDF and opcode table differ
}

void Cpu::InstrResetBit(const uint bit, Reg<u8> &reg) {
  reg = reg.val() & ~(1 << bit);
  wait(8);
}

void Cpu::InstrResetBit(const uint bit, Reg<u16> &addr_reg) {
  u8 res = ReadBus(addr_reg.val());
  res = res & ~(1 << bit);
  WriteBus(addr_reg.val(), res);
  wait(16);
}

void Cpu::InstrComplement() {
  reg_file.A = ~reg_file.A.val();
  SetFlagH(true);
  SetFlagN(true);
  wait(4);
}

// SW breakpoint instruction which was added for gdb compliance
// This instruction is not port of the original GameBoy ISA
void Cpu::InstrSwBp() {
  if (attachGdb) {
    gdb_server.SendBpReached();
    halted_ = true;
  } else {
    throw std::runtime_error("encountered SW breakpoint, but GDB is not attached");
  }
  reg_file.PC -= 1;
}

// Shift left into carry
void Cpu::InstrSLA(Reg<u8> &reg) {
  u8 carry_bit = reg.val() & gb_const::kMaskBit7;
  reg = reg.val() << 1;

  SetFlagC(carry_bit);
  SetFlagH(false);
  SetFlagN(false);
  SetFlagZ(reg.val() == 0);
  wait(8);
}

void Cpu::InstrSLA(Reg<u16> &addr_reg) {
  u8 dat = ReadBus(addr_reg);
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
void Cpu::InstrStopSim() {
  sc_stop();
}

// Cool: explanation https://ehaskins.com/2018-01-30%20Z80%20DAA/
void Cpu::InstrDAA() {
    u8 reg = reg_file.A.val();

    u8 correction = GetRegFlag(kMaskCFlag) ? 0x60 : 0x00;

    if (GetRegFlag(kMaskHFlag) || (!GetRegFlag(kMaskNFlag) && ((reg & 0x0F) > 9))) {
        correction |= 0x06;
    }

    if (GetRegFlag(kMaskCFlag) || (!GetRegFlag(kMaskNFlag) && (reg > 0x99))) {
        correction |= 0x60;
    }

    if (GetRegFlag(kMaskNFlag)) {
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

void Cpu::InstrSCF() {
  SetFlagC(true);
  SetFlagH(false);
  SetFlagN(false);
  wait(4);
}

void Cpu::InstrCCF() {
  SetFlagC(!GetRegFlag(kMaskCFlag));
  SetFlagH(false);
  SetFlagN(false);
  wait(4);
}

// Shift n right into Carry. MSB doesn't change.
void Cpu::InstrSRA(Reg<u8> &reg) {
  u8 carry_bit = reg.val() & 1;
  u8 top_bit = reg.val() & gb_const::kMaskBit7;

  reg = (reg.val() >> 1) | top_bit;

  SetFlagC(carry_bit);
  SetFlagH(false);
  SetFlagN(false);
  SetFlagZ(reg.val() == 0);
  wait(8);
}

void Cpu::InstrSRA(Reg<u16> &addr_reg) {
  u8 dat = ReadBus(addr_reg);
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

void Cpu::InstrLoadC() {
  reg_file.A = ReadBus(0xFF00 + reg_file.C);
  wait(8);
}
