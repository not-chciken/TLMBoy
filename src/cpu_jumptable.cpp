/**********************************************
 * MIT License
 * Copyright (c) 2022 chciken/Niko
**********************************************/

#include "cpu.h"

void Cpu::DoMachineCycle() {
  if (attachGdb) {
    std::cout << "waiting for gdb to attach on port " << gdb_port_ << "..." << std::endl;
    gdb_server.InitBlocking(gdb_port_);
    std::cout << "gdb attached!" << std::endl;
  }

  while (1) {
    if (gdb_server.IsMsgPending()) {
      gdb_server.HandleMessages();
      usleep(1000);
    }

    if (halted_) {
      usleep(1000);
      continue;
    }

    if (gdb_server.BpReached(reg_file.PC)) {
      halted_ = true;
      gdb_server.SendBpReached();
      wait(4);
      continue;
    }

    HandleInterrupts();  // This disables IME and sets the PC in case of an interrupt.

    DBG_LOG_INST(sc_core::sc_time_stamp() << ": PC 0x" << std::hex
      << static_cast<uint>(reg_file.PC.val()) << " "<< std::dec);

    // Fetch.
    u8 instr_byte = FetchNextInstrByte();

    // Decode & execute.
    switch (instr_byte) {
      case 0x00:
        DBG_LOG_INST("NOP");
        InstrNop();
        break;
      case 0x01:
        DBG_LOG_INST("LD BC,u16");
        InstrLoadImm(reg_file.BC);
        break;
      case 0x02:
        DBG_LOG_INST("LD (BC),A");
        InstrStore(reg_file.BC, reg_file.A);
        break;
      case 0x03:
        DBG_LOG_INST("INC BC");
        InstrInc(reg_file.BC);
        break;
      case 0x04:
        DBG_LOG_INST("INC B");
        InstrInc(reg_file.B);
        break;
      case 0x05:
        DBG_LOG_INST("DEC B");
        InstrDec(reg_file.B);
        break;
      case 0x06:
        DBG_LOG_INST("LD B,u8");
        InstrLoadImm(reg_file.B);
        break;
      case 0x07:
        DBG_LOG_INST("RLCA");
        InstrRlca();
        break;
      case 0x08:
        DBG_LOG_INST("LD (a16),SP");
        InstrStoreSp();
        break;
      case 0x09:
        DBG_LOG_INST("ADD HL,BC");
        InstrAddHl(reg_file.BC);
        break;
      case 0x0A:
        DBG_LOG_INST("LD A,(BC)");
        InstrLoad(reg_file.A, reg_file.BC);
        break;
      case 0x0B:
        DBG_LOG_INST("DEC BC");
        InstrDec(reg_file.BC);
        break;
      case 0x0C:
        DBG_LOG_INST("INC C");
        InstrInc(reg_file.C);
        break;
      case 0x0D:
        DBG_LOG_INST("DEC C");
        InstrDec(reg_file.C);
        break;
      case 0x0E:
        DBG_LOG_INST("LD C,d8");
        InstrLoadImm(reg_file.C);
        break;
      case 0x0F:
        DBG_LOG_INST("RRCA");
        InstrRrca();
        break;
      case 0x10:
        DBG_LOG_INST("STOP 0");  // Halt CPU & LCD display until button pressed.
        DBG_LOG_INST("NOT IMPLEMENTED YET");  // TODO(niko): this is a special function
        throw std::runtime_error("Instruction 0x10 not implemented");
        break;
      case 0x11:
        DBG_LOG_INST("LD DE,d16");
        InstrLoadImm(reg_file.DE);
        break;
      case 0x12:
        DBG_LOG_INST("LD (DE),A");
        InstrStore(reg_file.DE, reg_file.A);
        break;
      case 0x13:
        DBG_LOG_INST("INC DE");
        InstrInc(reg_file.DE);
        break;
      case 0x14:
        DBG_LOG_INST("INC D");
        InstrInc(reg_file.D);
        break;
      case 0x15:
        DBG_LOG_INST("DEC D");
        InstrDec(reg_file.D);
        break;
      case 0x16:
        DBG_LOG_INST("LD D,d8");
        InstrLoadImm(reg_file.D);
        break;
      case 0x17:
        DBG_LOG_INST("RLA");
        InstrRotLeftA();
        break;
      case 0x18:
        DBG_LOG_INST("JR r8");
        InstrJump();
        break;
      case 0x19:
        DBG_LOG_INST("ADD HL,DE");
        InstrAddHl(reg_file.DE);
        break;
      case 0x1A:
        DBG_LOG_INST("LD A,(DE)");
        InstrLoad(reg_file.A, reg_file.DE);
        break;
      case 0x1B:
        DBG_LOG_INST("DEC DE");
        InstrDec(reg_file.DE);
        break;
      case 0x1C:
        DBG_LOG_INST("INC E");
        InstrInc(reg_file.E);
        break;
      case 0x1D:
        DBG_LOG_INST("DEC E");
        InstrDec(reg_file.E);
        break;
      case 0x1E:
        DBG_LOG_INST("LD E,d8");
        InstrLoadImm(reg_file.E);
        break;
      case 0x1F:
        DBG_LOG_INST("RRA");
        InstrRra();
        break;
      case 0x20:
        DBG_LOG_INST("JR NZ, r8");
        InstrJumpIf(!GetFlagZ());
        break;
      case 0x21:
        DBG_LOG_INST("LD HL, d16");
        InstrLoadImm(reg_file.HL);
        break;
      case 0x22:
        DBG_LOG_INST("LD (HL+), A");
        InstrStoreInc(reg_file.HL, reg_file.A);
        break;
      case 0x23:
        DBG_LOG_INST("INC HL");
        InstrInc(reg_file.HL);
        break;
      case 0x24:
        DBG_LOG_INST("INC H");
        InstrInc(reg_file.H);
        break;
      case 0x25:
        DBG_LOG_INST("DEC H");
        InstrDec(reg_file.H);
        break;
      case 0x26:
        DBG_LOG_INST("LD H,d8");
        InstrLoadImm(reg_file.H);
        break;
      case 0x27:
        DBG_LOG_INST("DAA");
        InstrDAA();
        break;
      case 0x28:
        DBG_LOG_INST("JR Z,r8");
        InstrJumpIf(GetFlagZ());
        break;
      case 0x29:
        DBG_LOG_INST("ADD HL,HL");
        InstrAddHl(reg_file.HL);
        break;
      case 0x2A:
        DBG_LOG_INST("LD A,(HL+)");
        InstrLoadInc(reg_file.A, reg_file.HL);
        break;
      case 0x2B:
        DBG_LOG_INST("DEC HL");
        InstrDec(reg_file.HL);
        break;
      case 0x2C:
        DBG_LOG_INST("INC L");
        InstrInc(reg_file.L);
        break;
      case 0x2D:
        DBG_LOG_INST("DEC L");
        InstrDec(reg_file.L);
        break;
      case 0x2E:
        DBG_LOG_INST("LD L,D8");
        InstrLoadImm(reg_file.L);
        break;
      case 0x2F:
        DBG_LOG_INST("CPL");
        InstrComplement();
        break;
      case 0x30:
        DBG_LOG_INST("JR NC,r8");
        InstrJumpIf(!GetFlagC());
        break;
      case 0x31:
        DBG_LOG_INST("LD SP,d16");
        InstrLoadImm(reg_file.SP);
        break;
      case 0x32:
        DBG_LOG_INST("LD (HL-), A");
        InstrStoreDec(reg_file.HL, reg_file.A);
        break;
      case 0x33:
        DBG_LOG_INST("INC SP");
        InstrInc(reg_file.SP);
        break;
      case 0x34:
        DBG_LOG_INST("INC (HL)");
        InstrIncAddr(reg_file.HL);
        break;
      case 0x35:
        DBG_LOG_INST("INC (HL)");
        InstrDecAddr(reg_file.HL);
        break;
      case 0x36:
        DBG_LOG_INST("LD (HL),d8");
        InstrStoreImm(reg_file.HL);
        break;
      case 0x37:
        DBG_LOG_INST("SCF");
        InstrSCF();
        break;
      case 0x38:
        DBG_LOG_INST("JR C,r8");
        InstrJumpIf(GetFlagC());
        break;
      case 0x39:
        DBG_LOG_INST("ADD HL,SO");
        InstrAddHl(reg_file.SP);
        break;
      case 0x3A:
        DBG_LOG_INST("LD A,(HL-)");
        InstrLoadDec(reg_file.HL, reg_file.A);
        break;
      case 0x3B:
        DBG_LOG_INST("DEC SP");
        InstrDec(reg_file.SP);
        break;
      case 0x3C:
        DBG_LOG_INST("INC A");
        InstrInc(reg_file.A);
        break;
      case 0x3D:
        DBG_LOG_INST("DEC A");
        InstrDec(reg_file.A);
        break;
      case 0x3E:
        DBG_LOG_INST("LD A,d8");
        InstrLoadImm(reg_file.A);
        break;
      case 0x3F:
        DBG_LOG_INST("CCF");
        InstrCCF();
        break;
      case 0x40:
        DBG_LOG_INST("LD B,B");
        InstrMov(reg_file.B, reg_file.B);
        break;
      case 0x41:
        DBG_LOG_INST("LD B,C");
        InstrMov(reg_file.B, reg_file.C);
        break;
      case 0x42:
        DBG_LOG_INST("LD B,D");
        InstrMov(reg_file.B, reg_file.D);
        break;
      case 0x43:
        DBG_LOG_INST("LD B,E");
        InstrMov(reg_file.B, reg_file.E);
        break;
      case 0x44:
        DBG_LOG_INST("LD B,H");
        InstrMov(reg_file.B, reg_file.H);
        break;
      case 0x45:
        DBG_LOG_INST("LD B,L");
        InstrMov(reg_file.B, reg_file.L);
        break;
      case 0x46:
        DBG_LOG_INST("LD B,(HL)");
        InstrLoad(reg_file.B, reg_file.HL);
        break;
      case 0x47:
        DBG_LOG_INST("LD B,L");
        InstrMov(reg_file.B, reg_file.A);
        break;
      case 0x48:
        DBG_LOG_INST("LD C,B");
        InstrMov(reg_file.C, reg_file.B);
        break;
      case 0x49:
        DBG_LOG_INST("LD C,C");
        InstrMov(reg_file.C, reg_file.C);
        break;
      case 0x4A:
        DBG_LOG_INST("LD C,D");
        InstrMov(reg_file.C, reg_file.D);
        break;
      case 0x4B:
        DBG_LOG_INST("LD C,E");
        InstrMov(reg_file.C, reg_file.E);
        break;
      case 0x4C:
        DBG_LOG_INST("LD C,H");
        InstrMov(reg_file.C, reg_file.H);
        break;
      case 0x4D:
        DBG_LOG_INST("LD C,L");
        InstrMov(reg_file.C, reg_file.L);
        break;
      case 0x4E:
        DBG_LOG_INST("LD C,(HL)");
        InstrLoad(reg_file.C, reg_file.HL);
        break;
      case 0x4F:
        DBG_LOG_INST("LD C,A");
        InstrMov(reg_file.C, reg_file.A);
        break;
      case 0x50:
        DBG_LOG_INST("LD D,B");
        InstrMov(reg_file.D, reg_file.B);
        break;
      case 0x51:
        DBG_LOG_INST("LD D,C");
        InstrMov(reg_file.D, reg_file.C);
        break;
      case 0x52:
        DBG_LOG_INST("LD D,D");
        InstrMov(reg_file.D, reg_file.D);
        break;
      case 0x53:
        DBG_LOG_INST("LD D,E");
        InstrMov(reg_file.D, reg_file.E);
        break;
      case 0x54:
        DBG_LOG_INST("LD D,H");
        InstrMov(reg_file.D, reg_file.H);
        break;
      case 0x55:
        DBG_LOG_INST("LD D,L");
        InstrMov(reg_file.D, reg_file.L);
        break;
      case 0x56:
        DBG_LOG_INST("LD D,(HL)");
        InstrLoad(reg_file.D, reg_file.HL);
        break;
      case 0x57:
        DBG_LOG_INST("LD D,A");
        InstrMov(reg_file.D, reg_file.A);
        break;
      case 0x58:
        DBG_LOG_INST("LD E,B");
        InstrMov(reg_file.E, reg_file.B);
        break;
      case 0x59:
        DBG_LOG_INST("LD E,C");
        InstrMov(reg_file.E, reg_file.C);
        break;
      case 0x5A:
        DBG_LOG_INST("LD E,D");
        InstrMov(reg_file.E, reg_file.D);
        break;
      case 0x5B:
        DBG_LOG_INST("LD E,E");
        InstrMov(reg_file.E, reg_file.E);
        break;
      case 0x5C:
        DBG_LOG_INST("LD E,H");
        InstrMov(reg_file.E, reg_file.H);
        break;
      case 0x5D:
        DBG_LOG_INST("LD E,L");
        InstrMov(reg_file.E, reg_file.L);
        break;
      case 0x5E:
        DBG_LOG_INST("LD E,(HL)");
        InstrLoad(reg_file.E, reg_file.HL);
        break;
      case 0x5F:
        DBG_LOG_INST("LD E,A");
        InstrMov(reg_file.E, reg_file.A);
        break;
      case 0x60:
        DBG_LOG_INST("LD H,B");
        InstrMov(reg_file.H, reg_file.B);
        break;
      case 0x61:
        DBG_LOG_INST("LD H,C");
        InstrMov(reg_file.H, reg_file.C);
        break;
      case 0x62:
        DBG_LOG_INST("LD H,D");
        InstrMov(reg_file.H, reg_file.D);
        break;
      case 0x63:
        DBG_LOG_INST("LD H,E");
        InstrMov(reg_file.H, reg_file.E);
        break;
      case 0x64:
        DBG_LOG_INST("LD H,H");
        InstrMov(reg_file.H, reg_file.H);
        break;
      case 0x65:
        DBG_LOG_INST("LD H,L");
        InstrMov(reg_file.H, reg_file.L);
        break;
      case 0x66:
        DBG_LOG_INST("LD H,(HL)");
        InstrLoad(reg_file.H, reg_file.HL);
        break;
      case 0x67:
        DBG_LOG_INST("LD H,A");
        InstrMov(reg_file.H, reg_file.A);
        break;
      case 0x68:
        DBG_LOG_INST("LD L,B");
        InstrMov(reg_file.L, reg_file.B);
        break;
      case 0x69:
        DBG_LOG_INST("LD L,C");
        InstrMov(reg_file.L, reg_file.C);
        break;
      case 0x6A:
        DBG_LOG_INST("LD L,D");
        InstrMov(reg_file.L, reg_file.D);
        break;
      case 0x6B:
        DBG_LOG_INST("LD L,E");
        InstrMov(reg_file.L, reg_file.E);
        break;
      case 0x6C:
        DBG_LOG_INST("LD L,H");
        InstrMov(reg_file.L, reg_file.H);
        break;
      case 0x6D:
        DBG_LOG_INST("LD L,L");
        InstrMov(reg_file.L, reg_file.L);
        break;
      case 0x6E:
        DBG_LOG_INST("LD L,(HL)");
        InstrLoad(reg_file.L, reg_file.HL);
        break;
      case 0x6F:
        DBG_LOG_INST("LD L,A");
        InstrMov(reg_file.L, reg_file.A);
        break;
      case 0x70:
        DBG_LOG_INST("LD (HL),B");
        InstrStore(reg_file.HL, reg_file.B);
        break;
      case 0x71:
        DBG_LOG_INST("LD (HL),C");
        InstrStore(reg_file.HL, reg_file.C);
        break;
      case 0x72:
        DBG_LOG_INST("LD (HL),D");
        InstrStore(reg_file.HL, reg_file.D);
        break;
      case 0x73:
        DBG_LOG_INST("LD (HL),E");
        InstrStore(reg_file.HL, reg_file.E);
        break;
      case 0x74:
        DBG_LOG_INST("LD (HL),H");
        InstrStore(reg_file.HL, reg_file.H);
        break;
      case 0x75:
        DBG_LOG_INST("LD (HL),L");
        InstrStore(reg_file.HL, reg_file.L);
        break;
      case 0x76:
        DBG_LOG_INST("HALT");
        InstrHalt();
        break;
      case 0x77:
        DBG_LOG_INST("LD (HL),A");
        InstrStore(reg_file.HL, reg_file.A);
        break;
      case 0x78:
        DBG_LOG_INST("LD A,B");
        InstrMov(reg_file.A, reg_file.B);
        break;
      case 0x79:
        DBG_LOG_INST("LD A,C");
        InstrMov(reg_file.A, reg_file.C);
        break;
      case 0x7A:
        DBG_LOG_INST("LD A,D");
        InstrMov(reg_file.A, reg_file.D);
        break;
      case 0x7B:
        DBG_LOG_INST("LD A,E");
        InstrMov(reg_file.A, reg_file.E);
        break;
      case 0x7C:
        DBG_LOG_INST("LD A,H");
        InstrMov(reg_file.A, reg_file.H);
        break;
      case 0x7D:
        DBG_LOG_INST("LD A,L");
        InstrMov(reg_file.A, reg_file.L);
        break;
      case 0x7E:
        DBG_LOG_INST("LD A,(HL)");
        InstrLoad(reg_file.A, reg_file.HL);
        break;
      case 0x7F:
        DBG_LOG_INST("LD A,A");
        InstrMov(reg_file.A, reg_file.A);
        break;
      case 0x80:
        DBG_LOG_INST("ADD A,B");
        InstrAddA(reg_file.B);
        break;
      case 0x81:
        DBG_LOG_INST("ADD A,C");
        InstrAddA(reg_file.C);
        break;
      case 0x82:
        DBG_LOG_INST("ADD A,D");
        InstrAddA(reg_file.D);
        break;
      case 0x83:
        DBG_LOG_INST("ADD A,E");
        InstrAddA(reg_file.E);
        break;
      case 0x84:
        DBG_LOG_INST("ADD A,H");
        InstrAddA(reg_file.H);
        break;
      case 0x85:
        DBG_LOG_INST("ADD A,L");
        InstrAddA(reg_file.L);
        break;
      case 0x86:
        DBG_LOG_INST("ADD A,(HL)");
        InstrAddA(reg_file.HL);
        break;
      case 0x87:
        DBG_LOG_INST("ADD A,A");
        InstrAddA(reg_file.A);
        break;
      case 0x88:
        DBG_LOG_INST("ADC A,B");
        InstrAddACarry(reg_file.B);
        break;
      case 0x89:
        DBG_LOG_INST("ADC A,C");
        InstrAddACarry(reg_file.C);
        break;
      case 0x8A:
        DBG_LOG_INST("ADC A,D");
        InstrAddACarry(reg_file.D);
        break;
      case 0x8B:
        DBG_LOG_INST("ADC A,E");
        InstrAddACarry(reg_file.E);
        break;
      case 0x8C:
        DBG_LOG_INST("ADC A,H");
        InstrAddACarry(reg_file.H);
        break;
      case 0x8D:
        DBG_LOG_INST("ADC A,L");
        InstrAddACarry(reg_file.L);
        break;
      case 0x8E:
        DBG_LOG_INST("ADC A,(HL)");
        InstrAddACarry(reg_file.HL);
        break;
      case 0x8F:
        DBG_LOG_INST("ADC A,A");
        InstrAddACarry(reg_file.A);
        break;
      case 0x90:
        DBG_LOG_INST("SUB B");
        InstrSub(reg_file.B);
        break;
      case 0x91:
        DBG_LOG_INST("SUB C");
        InstrSub(reg_file.C);
        break;
      case 0x92:
        DBG_LOG_INST("SUB D");
        InstrSub(reg_file.D);
        break;
      case 0x93:
        DBG_LOG_INST("SUB E");
        InstrSub(reg_file.E);
        break;
      case 0x94:
        DBG_LOG_INST("SUB H");
        InstrSub(reg_file.H);
        break;
      case 0x95:
        DBG_LOG_INST("SUB L");
        InstrSub(reg_file.L);
        break;
      case 0x96:
        DBG_LOG_INST("SUB (HL)");
        InstrSub(reg_file.HL);
        break;
      case 0x97:
        DBG_LOG_INST("SUB A");
        InstrSub(reg_file.A);
        break;
      case 0x98:
        DBG_LOG_INST("SBC A,B");
        InstrSubCarry(reg_file.B);
        break;
      case 0x99:
        DBG_LOG_INST("SBC A,C");
        InstrSubCarry(reg_file.C);
        break;
      case 0x9A:
        DBG_LOG_INST("SBC A,D");
        InstrSubCarry(reg_file.D);
        break;
      case 0x9B:
        DBG_LOG_INST("SBC A,E");
        InstrSubCarry(reg_file.E);
        break;
      case 0x9C:
        DBG_LOG_INST("SBC A,H");
        InstrSubCarry(reg_file.H);
        break;
      case 0x9D:
        DBG_LOG_INST("SBC A,L");
        InstrSubCarry(reg_file.L);
        break;
      case 0x9E:
        DBG_LOG_INST("SBC A,(HL)");
        InstrSubCarry(reg_file.HL);
        break;
      case 0x9F:
        DBG_LOG_INST("SBC A,A");
        InstrSubCarry(reg_file.A);
        break;
      case 0xA0:
        DBG_LOG_INST("AND B");
        InstrAnd(reg_file.B);
        break;
      case 0xA1:
        DBG_LOG_INST("AND C");
        InstrAnd(reg_file.C);
        break;
      case 0xA2:
        DBG_LOG_INST("AND D");
        InstrAnd(reg_file.D);
        break;
      case 0xA3:
        DBG_LOG_INST("AND E");
        InstrAnd(reg_file.E);
        break;
      case 0xA4:
        DBG_LOG_INST("AND H");
        InstrAnd(reg_file.H);
        break;
      case 0xA5:
        DBG_LOG_INST("AND L");
        InstrAnd(reg_file.L);
        break;
      case 0xA6:
        DBG_LOG_INST("AND (HL)");
        InstrAnd(reg_file.HL);
        break;
      case 0xA7:
        DBG_LOG_INST("AND A");
        InstrAnd(reg_file.A);
        break;
      case 0xA8:
        DBG_LOG_INST("XOR B");
        InstrXor(reg_file.B);
        break;
      case 0xA9:
        DBG_LOG_INST("XOR C");
        InstrXor(reg_file.C);
        break;
      case 0xAA:
        DBG_LOG_INST("XOR D");
        InstrXor(reg_file.D);
        break;
      case 0xAB:
        DBG_LOG_INST("XOR E");
        InstrXor(reg_file.E);
        break;
      case 0xAC:
        DBG_LOG_INST("XOR H");
        InstrXor(reg_file.H);
        break;
      case 0xAD:
        DBG_LOG_INST("XOR L");
        InstrXor(reg_file.L);
        break;
      case 0xAE:
        DBG_LOG_INST("XOR (HL)");
        InstrXor(reg_file.HL);
        break;
      case 0xAF:
        DBG_LOG_INST("XOR A");
        InstrXor(reg_file.A);
        break;
      case 0xB0:
        DBG_LOG_INST("OR B");
        InstrOr(reg_file.B);
        break;
      case 0xB1:
        DBG_LOG_INST("OR C");
        InstrOr(reg_file.C);
        break;
      case 0xB2:
        DBG_LOG_INST("OR D");
        InstrOr(reg_file.D);
        break;
      case 0xB3:
        DBG_LOG_INST("OR E");
        InstrOr(reg_file.E);
        break;
      case 0xB4:
        DBG_LOG_INST("OR H");
        InstrOr(reg_file.H);
        break;
      case 0xB5:
        DBG_LOG_INST("OR L");
        InstrOr(reg_file.L);
        break;
      case 0xB6:
        DBG_LOG_INST("OR (HL)");
        InstrOr(reg_file.HL);
        break;
      case 0xB7:
        DBG_LOG_INST("OR A");
        InstrOr(reg_file.A);
        break;
      case 0xB8:
        DBG_LOG_INST("CP B");
        InstrComp(reg_file.B);
        break;
      case 0xB9:
        DBG_LOG_INST("CP C");
        InstrComp(reg_file.C);
        break;
      case 0xBA:
        DBG_LOG_INST("CP D");
        InstrComp(reg_file.D);
        break;
      case 0xBB:
        DBG_LOG_INST("CP E");
        InstrComp(reg_file.E);
        break;
      case 0xBC:
        DBG_LOG_INST("CP H");
        InstrComp(reg_file.H);
        break;
      case 0xBD:
        DBG_LOG_INST("CP L");
        InstrComp(reg_file.L);
        break;
      case 0xBE:
        DBG_LOG_INST("CP (HL)");
        InstrComp(reg_file.HL);
        break;
      case 0xBF:
        DBG_LOG_INST("CP A");
        InstrComp(reg_file.A);
        break;
      case 0xC0:
        DBG_LOG_INST("RET NZ");
        InstrRetIf(!GetFlagZ());
        break;
      case 0xC1:
        DBG_LOG_INST("POP BC");
        InstrPop(reg_file.BC);
        break;
      case 0xC2:
        DBG_LOG_INST("JP NZ,a16");
        InstrJumpAddrIf(!GetFlagZ());
        break;
      case 0xC3:
        DBG_LOG_INST("JP a16");
        InstrJumpAddr();
        break;
      case 0xC4:
        DBG_LOG_INST("Call NZ,a16");
        InstrCallIf(!GetFlagZ());
        break;
      case 0xC5:
        DBG_LOG_INST("PUSH BC");
        InstrPush(reg_file.BC);
        break;
      case 0xC6:
        DBG_LOG_INST("ADD A, d8");
        InstrAddAImm();
        break;
      case 0xC7:
        DBG_LOG_INST("RST 00H");
        InstrRST(0x00);
        break;
      case 0xC8:
        DBG_LOG_INST("RET Z");
        InstrRetIf(GetFlagZ());
        break;
      case 0xC9:
        DBG_LOG_INST("RET");
        InstrRet();
        break;
      case 0xCA:
        DBG_LOG_INST("JP Z,a16");
        InstrJumpAddrIf(GetFlagZ());
        break;
      case 0xCC:
        DBG_LOG_INST("Call z,a16");
        InstrCallIf(GetFlagZ());
        break;
      case 0xCD:
        DBG_LOG_INST("Call a16");
        InstrCall();
        break;
      case 0xCE:
        DBG_LOG_INST("ADC A,d8");
        InstrAddACarryImm();
        break;
      case 0xCF:
        DBG_LOG_INST("RST 08H");
        InstrRST(0x08);
        break;
      case 0xD0:
        DBG_LOG_INST("RET NC");
        InstrRetIf(!GetFlagC());
        break;
      case 0xD1:
        DBG_LOG_INST("POP DE");
        InstrPop(reg_file.DE);
        break;
      case 0xD2:
        DBG_LOG_INST("JP NC, a16");
        InstrJumpAddrIf(!GetFlagC());
        break;
      case 0xD3:
        DBG_LOG_INST("STOP SIM");  // Special instruction.
        InstrEmu();
        break;
      case 0xD4:
        DBG_LOG_INST("CALL NC,a16");
        InstrCallIf(!GetFlagC());
        break;
      case 0xD5:
        DBG_LOG_INST("PUSH DE");
        InstrPush(reg_file.DE);
        break;
      case 0xD6:
        DBG_LOG_INST("SUB (d8)");
        InstrSubImm();
        break;
      case 0xD7:
        DBG_LOG_INST("RST 10H");
        InstrRST(0x10);
        break;
      case 0xD8:
        DBG_LOG_INST("RET C");
        InstrRetIf(GetFlagC());
        break;
      case 0xD9:
        DBG_LOG_INST("RETI");
        InstrRetI();
        break;
      case 0xDA:
        DBG_LOG_INST("JP C,a16");
        InstrJumpAddrIf(GetFlagC());
        break;
      // case 0xDB: Not implemented!
      case 0xDC:
        DBG_LOG_INST("CALL C,a16");
        InstrCallIf(GetFlagC());
        break;
      // case 0xDD: Not implemented!
      case 0xDE:
        DBG_LOG_INST("SBC A, d8");
        InstrSubCarryImm();
        break;
      case 0xDF:
        DBG_LOG_INST("RST 18H");
        InstrRST(0x18);
        break;
      case 0xE0:
        DBG_LOG_INST("LDH (a8), A");
        InstrStoreH(reg_file.A);
        break;
      case 0xE1:
        DBG_LOG_INST("POP HL");
        InstrPop(reg_file.HL);
        break;
      case 0xE2:
        DBG_LOG_INST("LD (C), A");
        InstrStore(reg_file.C.val(), reg_file.A);
        break;
      // case 0xE3: This one is not implemented!
      // case 0xE4: This one is not implemented!
      case 0xE5:
        DBG_LOG_INST("PUSH HL");
        InstrPush(reg_file.HL);
        break;
      case 0xE6:
        DBG_LOG_INST("AND d8");
        InstrAndImm();
        break;
      case 0xE7:
        DBG_LOG_INST("RST 20H");
        InstrRST(0x20);
        break;
      case 0xE8:
        DBG_LOG_INST("ADD SP, r8");
        InstrAddSp();
        break;
      case 0xE9:
        DBG_LOG_INST("JP (HL)");
        InstrJump(reg_file.HL);
        break;
      case 0xEA:
        DBG_LOG_INST("LD (a16), A");
        InstrStore(reg_file.A);
        break;
      // case 0xEB: Not implemented!
      // case 0xEC: Not implemented!
      // case 0xED: Not implemented!
      case 0xEE:
        DBG_LOG_INST("XOR d8");
        InstrXorImm();
        break;
      case 0xEF:
        DBG_LOG_INST("RST 28H");
        InstrRST(0x28);
        break;
      case 0xF0:
        DBG_LOG_INST("LDH A,(a8)");
        InstrLoadH(reg_file.A);
        break;
      case 0xF1:
        DBG_LOG_INST("POP AF");
        InstrPop(reg_file.AF);
        break;
      case 0xF2:
        DBG_LOG_INST("LD A,(C)");
        InstrLoadC();
        break;
      case 0xF3:
        DBG_LOG_INST("DI");
        InstrDI();
        break;
      // case 0xF4: This one is not implemented!
      case 0xF5:
        DBG_LOG_INST("PUSH AF");  // Note: AF is not writable!
        InstrPush(reg_file.AF);
        break;
      case 0xF6:
        DBG_LOG_INST("OR d8");
        InstrOrImm();
        break;
      case 0xF7:
        DBG_LOG_INST("RST 30H");
        InstrRST(0x30);
        break;
      case 0xF8:
        DBG_LOG_INST("LDHL SP,n");
        InstrLoadHlSpRel();
        break;
      case 0xF9:
        DBG_LOG_INST("LD SP,HL");
        InstrLoadSpHl();
        break;
      case 0xFA:
        DBG_LOG_INST("LD A, (a16)");
        InstrLoad(reg_file.A, FetchNext2InstrBytes());
        break;
      case 0xFB:
        DBG_LOG_INST("EI");
        InstrEI();
        break;
      // case 0xFC: This one is not implemented!
      // case 0xFD: This one is not implemented!
      case 0xFE:
        DBG_LOG_INST("CP d8");
        InstrCompImm();
        break;
      case 0xFF:
        DBG_LOG_INST("RST 38H");
        InstrRST(0x38);
        break;
      case 0xCB:  // special bit instruction is called
        instr_byte = FetchNextInstrByte();
        switch (instr_byte) {
          case 0x00:
            DBG_LOG_INST("RLC B");
            InstrRlc(reg_file.B);
            break;
          case 0x01:
            DBG_LOG_INST("RLC C");
            InstrRlc(reg_file.C);
            break;
          case 0x02:
            DBG_LOG_INST("RLC D");
            InstrRlc(reg_file.D);
            break;
          case 0x03:
            DBG_LOG_INST("RLC E");
            InstrRlc(reg_file.E);
            break;
          case 0x04:
            DBG_LOG_INST("RLC H");
            InstrRlc(reg_file.H);
            break;
          case 0x05:
            DBG_LOG_INST("RLC L");
            InstrRlc(reg_file.L);
            break;
          case 0x06:
            DBG_LOG_INST("RLC (HL)");
            InstrRlc(reg_file.HL);
            break;
          case 0x07:
            DBG_LOG_INST("RLC A");
            InstrRlc(reg_file.A);
            break;
          case 0x08:
            DBG_LOG_INST("RRC B");
            InstrRrc(reg_file.B);
            break;
          case 0x09:
            DBG_LOG_INST("RRC C");
            InstrRrc(reg_file.C);
            break;
          case 0x0A:
            DBG_LOG_INST("RRC D");
            InstrRrc(reg_file.D);
            break;
          case 0x0B:
            DBG_LOG_INST("RRC E");
            InstrRrc(reg_file.E);
            break;
          case 0x0C:
            DBG_LOG_INST("RRC H");
            InstrRrc(reg_file.H);
            break;
          case 0x0D:
            DBG_LOG_INST("RRC L");
            InstrRrc(reg_file.L);
            break;
          case 0x0E:
            DBG_LOG_INST("RRC (HL)");
            InstrRrc(reg_file.HL);
            break;
          case 0x0F:
            DBG_LOG_INST("RRC A");
            InstrRrc(reg_file.A);
            break;
          case 0x10:
            DBG_LOG_INST("RL B");
            InstrRotLeft(reg_file.B);
            break;
          case 0x11:
            DBG_LOG_INST("RL C");
            InstrRotLeft(reg_file.C);
            break;
          case 0x12:
            DBG_LOG_INST("RL D");
            InstrRotLeft(reg_file.D);
            break;
          case 0x13:
            DBG_LOG_INST("RL E");
            InstrRotLeft(reg_file.E);
            break;
          case 0x14:
            DBG_LOG_INST("RL H");
            InstrRotLeft(reg_file.H);
            break;
          case 0x15:
            DBG_LOG_INST("RL L");
            InstrRotLeft(reg_file.L);
            break;
          case 0x16:
            DBG_LOG_INST("RL (HL)");
            InstrRotLeft(reg_file.HL);
            break;
          case 0x17:
            DBG_LOG_INST("RL A");
            InstrRotLeft(reg_file.A);
            break;
          case 0x18:
            DBG_LOG_INST("RR B");
            InstrRotRight(reg_file.B);
            break;
          case 0x19:
            DBG_LOG_INST("RR C");
            InstrRotRight(reg_file.C);
            break;
          case 0x1A:
            DBG_LOG_INST("RR D");
            InstrRotRight(reg_file.D);
            break;
          case 0x1B:
            DBG_LOG_INST("RR E");
            InstrRotRight(reg_file.E);
            break;
          case 0x1C:
            DBG_LOG_INST("RR H");
            InstrRotRight(reg_file.H);
            break;
          case 0x1D:
            DBG_LOG_INST("RR L");
            InstrRotRight(reg_file.L);
            break;
          case 0x1E:
            DBG_LOG_INST("RR (HL");
            InstrRotRight(reg_file.HL);
            break;
          case 0x1F:
            DBG_LOG_INST("RR A");
            InstrRotRight(reg_file.A);
            break;
          case 0x20:
            DBG_LOG_INST("SLA B");
            InstrSLA(reg_file.B);
            break;
          case 0x21:
            DBG_LOG_INST("SLA C");
            InstrSLA(reg_file.C);
            break;
          case 0x22:
            DBG_LOG_INST("SLA D");
            InstrSLA(reg_file.D);
            break;
          case 0x23:
            DBG_LOG_INST("SLA E");
            InstrSLA(reg_file.E);
            break;
          case 0x24:
            DBG_LOG_INST("SLA H");
            InstrSLA(reg_file.H);
            break;
          case 0x25:
            DBG_LOG_INST("SLA L");
            InstrSLA(reg_file.L);
            break;
          case 0x26:
            DBG_LOG_INST("SLA (HL)");
            InstrSLA(reg_file.HL);
            break;
          case 0x27:
            DBG_LOG_INST("SLA A");
            InstrSLA(reg_file.A);
            break;
          case 0x28:
            DBG_LOG_INST("SRA B");
            InstrSRA(reg_file.B);
            break;
          case 0x29:
            DBG_LOG_INST("SRA C");
            InstrSRA(reg_file.C);
            break;
          case 0x2A:
            DBG_LOG_INST("SRA D");
            InstrSRA(reg_file.D);
            break;
          case 0x2B:
            DBG_LOG_INST("SRA E");
            InstrSRA(reg_file.E);
            break;
          case 0x2C:
            DBG_LOG_INST("SRA H");
            InstrSRA(reg_file.H);
            break;
          case 0x2D:
            DBG_LOG_INST("SRA L");
            InstrSRA(reg_file.L);
            break;
          case 0x2E:
            DBG_LOG_INST("SRA (HL)");
            InstrSRA(reg_file.HL);
            break;
          case 0x2F:
            DBG_LOG_INST("SRA A");
            InstrSRA(reg_file.A);
            break;
          case 0x30:
            DBG_LOG_INST("SWAP B");
            InstrSwap(reg_file.B);
            break;
          case 0x31:
            DBG_LOG_INST("SWAP C");
            InstrSwap(reg_file.C);
            break;
          case 0x32:
            DBG_LOG_INST("SWAP D");
            InstrSwap(reg_file.D);
            break;
          case 0x33:
            DBG_LOG_INST("SWAP E");
            InstrSwap(reg_file.E);
            break;
          case 0x34:
            DBG_LOG_INST("SWAP H");
            InstrSwap(reg_file.H);
            break;
          case 0x35:
            DBG_LOG_INST("SWAP L");
            InstrSwap(reg_file.L);
            break;
          case 0x36:
            DBG_LOG_INST("SWAP (HL)");
            InstrSwap(reg_file.HL);
            break;
          case 0x37:
            DBG_LOG_INST("SWAP A");
            InstrSwap(reg_file.A);
            break;
          case 0x38:
            DBG_LOG_INST("SRL B");
            InstrShiftRight(reg_file.B);
            break;
          case 0x39:
            DBG_LOG_INST("SRL C");
            InstrShiftRight(reg_file.C);
            break;
          case 0x3A:
            DBG_LOG_INST("SRL D");
            InstrShiftRight(reg_file.D);
            break;
          case 0x3B:
            DBG_LOG_INST("SRL E");
            InstrShiftRight(reg_file.E);
            break;
          case 0x3C:
            DBG_LOG_INST("SRL H");
            InstrShiftRight(reg_file.H);
            break;
          case 0x3D:
            DBG_LOG_INST("SRL L");
            InstrShiftRight(reg_file.L);
            break;
          case 0x3E:
            DBG_LOG_INST("SRL (HL)");
            InstrShiftRight(reg_file.HL);
            break;
          case 0x3F:
            DBG_LOG_INST("SRL A");
            InstrShiftRight(reg_file.A);
            break;
          case 0x40:
            DBG_LOG_INST("BIT 0,B");
            InstrBitN(0, reg_file.B);
            break;
          case 0x41:
            DBG_LOG_INST("BIT 0,C");
            InstrBitN(0, reg_file.C);
            break;
          case 0x42:
            DBG_LOG_INST("BIT 0,D");
            InstrBitN(0, reg_file.D);
            break;
          case 0x43:
            DBG_LOG_INST("BIT 0,E");
            InstrBitN(0, reg_file.E);
            break;
          case 0x44:
            DBG_LOG_INST("BIT 0,H");
            InstrBitN(0, reg_file.H);
            break;
          case 0x45:
            DBG_LOG_INST("BIT 0,L");
            InstrBitN(0, reg_file.L);
            break;
          case 0x46:
            DBG_LOG_INST("BIT 0,(HL)");
            InstrBitN(0, reg_file.HL);
            break;
          case 0x47:
            DBG_LOG_INST("BIT 0,A");
            InstrBitN(0, reg_file.A);
            break;
          case 0x48:
            DBG_LOG_INST("BIT 1,B");
            InstrBitN(1, reg_file.B);
            break;
          case 0x49:
            DBG_LOG_INST("BIT 1,C");
            InstrBitN(1, reg_file.C);
            break;
          case 0x4A:
            DBG_LOG_INST("BIT 1,D");
            InstrBitN(1, reg_file.D);
            break;
          case 0x4B:
            DBG_LOG_INST("BIT 1,E");
            InstrBitN(1, reg_file.E);
            break;
          case 0x4C:
            DBG_LOG_INST("BIT 1,H");
            InstrBitN(1, reg_file.H);
            break;
          case 0x4D:
            DBG_LOG_INST("BIT 1,L");
            InstrBitN(1, reg_file.L);
            break;
          case 0x4E:
            DBG_LOG_INST("BIT 1,(HL)");
            InstrBitN(1, reg_file.HL);
            break;
          case 0x4F:
            DBG_LOG_INST("BIT 1,A");
            InstrBitN(1, reg_file.A);
            break;
          case 0x50:
            DBG_LOG_INST("BIT 2,B");
            InstrBitN(2, reg_file.B);
            break;
          case 0x51:
            DBG_LOG_INST("BIT 2,C");
            InstrBitN(2, reg_file.C);
            break;
          case 0x52:
            DBG_LOG_INST("BIT 2,D");
            InstrBitN(2, reg_file.D);
            break;
          case 0x53:
            DBG_LOG_INST("BIT 2,E");
            InstrBitN(2, reg_file.E);
            break;
          case 0x54:
            DBG_LOG_INST("BIT 2,H");
            InstrBitN(2, reg_file.H);
            break;
          case 0x55:
            DBG_LOG_INST("BIT 2,L");
            InstrBitN(2, reg_file.L);
            break;
          case 0x56:
            DBG_LOG_INST("BIT 2,(HL)");
            InstrBitN(2, reg_file.HL);
            break;
          case 0x57:
            DBG_LOG_INST("BIT 2,A");
            InstrBitN(2, reg_file.A);
            break;
          case 0x58:
            DBG_LOG_INST("BIT 3,B");
            InstrBitN(3, reg_file.B);
            break;
          case 0x59:
            DBG_LOG_INST("BIT 3,C");
            InstrBitN(3, reg_file.C);
            break;
          case 0x5A:
            DBG_LOG_INST("BIT 3,D");
            InstrBitN(3, reg_file.D);
            break;
          case 0x5B:
            DBG_LOG_INST("BIT 3,E");
            InstrBitN(3, reg_file.E);
            break;
          case 0x5C:
            DBG_LOG_INST("BIT 3,H");
            InstrBitN(3, reg_file.H);
            break;
          case 0x5D:
            DBG_LOG_INST("BIT 3,L");
            InstrBitN(3, reg_file.L);
            break;
          case 0x5E:
            DBG_LOG_INST("BIT 3,(HL)");
            InstrBitN(3, reg_file.HL);
            break;
          case 0x5F:
            DBG_LOG_INST("BIT 3,A");
            InstrBitN(3, reg_file.A);
            break;
          case 0x60:
            DBG_LOG_INST("BIT 4,B");
            InstrBitN(4, reg_file.B);
            break;
          case 0x61:
            DBG_LOG_INST("BIT 4,C");
            InstrBitN(4, reg_file.C);
            break;
          case 0x62:
            DBG_LOG_INST("BIT 4,D");
            InstrBitN(4, reg_file.D);
            break;
          case 0x63:
            DBG_LOG_INST("BIT 4,E");
            InstrBitN(4, reg_file.E);
            break;
          case 0x64:
            DBG_LOG_INST("BIT 4,H");
            InstrBitN(4, reg_file.H);
            break;
          case 0x65:
            DBG_LOG_INST("BIT 4,L");
            InstrBitN(4, reg_file.L);
            break;
          case 0x66:
            DBG_LOG_INST("BIT 4,(HL)");
            InstrBitN(4, reg_file.HL);
            break;
          case 0x67:
            DBG_LOG_INST("BIT 4,A");
            InstrBitN(4, reg_file.A);
            break;
          case 0x68:
            DBG_LOG_INST("BIT 5,B");
            InstrBitN(5, reg_file.B);
            break;
          case 0x69:
            DBG_LOG_INST("BIT 5,C");
            InstrBitN(5, reg_file.C);
            break;
          case 0x6A:
            DBG_LOG_INST("BIT 5,D");
            InstrBitN(5, reg_file.D);
            break;
          case 0x6B:
            DBG_LOG_INST("BIT 5,E");
            InstrBitN(5, reg_file.E);
            break;
          case 0x6C:
            DBG_LOG_INST("BIT 5,H");
            InstrBitN(5, reg_file.H);
            break;
          case 0x6D:
            DBG_LOG_INST("BIT 5,L");
            InstrBitN(5, reg_file.L);
            break;
          case 0x6E:
            DBG_LOG_INST("BIT 5,(HL)");
            InstrBitN(5, reg_file.HL);
            break;
          case 0x6F:
            DBG_LOG_INST("BIT 5,A");
            InstrBitN(5, reg_file.A);
            break;
          case 0x70:
            DBG_LOG_INST("BIT 6,B");
            InstrBitN(6, reg_file.B);
            break;
          case 0x71:
            DBG_LOG_INST("BIT 6,C");
            InstrBitN(6, reg_file.C);
            break;
          case 0x72:
            DBG_LOG_INST("BIT 6,D");
            InstrBitN(6, reg_file.D);
            break;
          case 0x73:
            DBG_LOG_INST("BIT 6,E");
            InstrBitN(6, reg_file.E);
            break;
          case 0x74:
            DBG_LOG_INST("BIT 6,H");
            InstrBitN(6, reg_file.H);
            break;
          case 0x75:
            DBG_LOG_INST("BIT 6,L");
            InstrBitN(6, reg_file.L);
            break;
          case 0x76:
            DBG_LOG_INST("BIT 6,(HL)");
            InstrBitN(6, reg_file.HL);
            break;
          case 0x77:
            DBG_LOG_INST("BIT 6,A");
            InstrBitN(6, reg_file.A);
            break;
          case 0x78:
            DBG_LOG_INST("BIT 7,B");
            InstrBitN(7, reg_file.B);
            break;
          case 0x79:
            DBG_LOG_INST("BIT 7,C");
            InstrBitN(7, reg_file.C);
            break;
          case 0x7A:
            DBG_LOG_INST("BIT 7,D");
            InstrBitN(7, reg_file.D);
            break;
          case 0x7B:
            DBG_LOG_INST("BIT 7,E");
            InstrBitN(7, reg_file.E);
            break;
          case 0x7C:
            DBG_LOG_INST("BIT 7,H");
            InstrBitN(7, reg_file.H);
            break;
          case 0x7D:
            DBG_LOG_INST("BIT 7,L");
            InstrBitN(7, reg_file.L);
            break;
          case 0x7E:
            DBG_LOG_INST("BIT 7,(HL)");
            InstrBitN(7, reg_file.HL);
            break;
          case 0x7F:
            DBG_LOG_INST("BIT 7, A");
            InstrBitN(7, reg_file.A);
            break;
          case 0x80:
            DBG_LOG_INST("RES 0, B");
            InstrResetBit(0, reg_file.B);
            break;
          case 0x81:
            DBG_LOG_INST("RES 0, C");
            InstrResetBit(0, reg_file.C);
            break;
          case 0x82:
            DBG_LOG_INST("RES 0, D");
            InstrResetBit(0, reg_file.D);
            break;
          case 0x83:
            DBG_LOG_INST("RES 0, E");
            InstrResetBit(0, reg_file.E);
            break;
          case 0x84:
            DBG_LOG_INST("RES 0, H");
            InstrResetBit(0, reg_file.H);
            break;
          case 0x85:
            DBG_LOG_INST("RES 0, L");
            InstrResetBit(0, reg_file.L);
            break;
          case 0x86:
            DBG_LOG_INST("RES 0, (HL)");
            InstrResetBit(0, reg_file.HL);
            break;
          case 0x87:
            DBG_LOG_INST("RES 0, A");
            InstrResetBit(0, reg_file.A);
            break;
          case 0x88:
            DBG_LOG_INST("RES 1, B");
            InstrResetBit(1, reg_file.B);
            break;
          case 0x89:
            DBG_LOG_INST("RES 1, C");
            InstrResetBit(1, reg_file.C);
            break;
          case 0x8A:
            DBG_LOG_INST("RES 1, D");
            InstrResetBit(1, reg_file.D);
            break;
          case 0x8B:
            DBG_LOG_INST("RES 1, E");
            InstrResetBit(1, reg_file.E);
            break;
          case 0x8C:
            DBG_LOG_INST("RES 1, H");
            InstrResetBit(1, reg_file.H);
            break;
          case 0x8D:
            DBG_LOG_INST("RES 1, L");
            InstrResetBit(1, reg_file.L);
            break;
          case 0x8E:
            DBG_LOG_INST("RES 1, (HL)");
            InstrResetBit(1, reg_file.HL);
            break;
          case 0x8F:
            DBG_LOG_INST("RES 1, A");
            InstrResetBit(1, reg_file.A);
            break;
          case 0x90:
            DBG_LOG_INST("RES 2, B");
            InstrResetBit(2, reg_file.B);
            break;
          case 0x91:
            DBG_LOG_INST("RES 2, C");
            InstrResetBit(2, reg_file.C);
            break;
          case 0x92:
            DBG_LOG_INST("RES 2, D");
            InstrResetBit(2, reg_file.D);
            break;
          case 0x93:
            DBG_LOG_INST("RES 2, E");
            InstrResetBit(2, reg_file.E);
            break;
          case 0x94:
            DBG_LOG_INST("RES 2, H");
            InstrResetBit(2, reg_file.H);
            break;
          case 0x95:
            DBG_LOG_INST("RES 2, L");
            InstrResetBit(2, reg_file.L);
            break;
          case 0x96:
            DBG_LOG_INST("RES 2, HL");
            InstrResetBit(2, reg_file.HL);
            break;
          case 0x97:
            DBG_LOG_INST("RES 2, A");
            InstrResetBit(2, reg_file.A);
            break;
          case 0x98:
            DBG_LOG_INST("RES 3, B");
            InstrResetBit(3, reg_file.B);
            break;
          case 0x99:
            DBG_LOG_INST("RES 3, C");
            InstrResetBit(3, reg_file.C);
            break;
          case 0x9A:
            DBG_LOG_INST("RES 3, D");
            InstrResetBit(3, reg_file.D);
            break;
          case 0x9B:
            DBG_LOG_INST("RES 3, E");
            InstrResetBit(3, reg_file.E);
            break;
          case 0x9C:
            DBG_LOG_INST("RES 3, H");
            InstrResetBit(3, reg_file.H);
            break;
          case 0x9D:
            DBG_LOG_INST("RES 3, L");
            InstrResetBit(3, reg_file.L);
            break;
          case 0x9E:
            DBG_LOG_INST("RES 3, (HL)");
            InstrResetBit(3, reg_file.HL);
            break;
          case 0x9F:
            DBG_LOG_INST("RES 3, A");
            InstrResetBit(3, reg_file.A);
            break;
          case 0xA0:
            DBG_LOG_INST("RES 4, B");
            InstrResetBit(4, reg_file.B);
            break;
          case 0xA1:
            DBG_LOG_INST("RES 4, C");
            InstrResetBit(4, reg_file.C);
            break;
          case 0xA2:
            DBG_LOG_INST("RES 4, D");
            InstrResetBit(4, reg_file.D);
            break;
          case 0xA3:
            DBG_LOG_INST("RES 4, E");
            InstrResetBit(4, reg_file.E);
            break;
          case 0xA4:
            DBG_LOG_INST("RES 4, H");
            InstrResetBit(4, reg_file.H);
            break;
          case 0xA5:
            DBG_LOG_INST("RES 4, L");
            InstrResetBit(4, reg_file.L);
            break;
          case 0xA6:
            DBG_LOG_INST("RES 4, (HL)");
            InstrResetBit(4, reg_file.HL);
            break;
          case 0xA7:
            DBG_LOG_INST("RES 4, A");
            InstrResetBit(4, reg_file.A);
            break;
          case 0xA8:
            DBG_LOG_INST("RES 5, B");
            InstrResetBit(5, reg_file.B);
            break;
          case 0xA9:
            DBG_LOG_INST("RES 5, C");
            InstrResetBit(5, reg_file.C);
            break;
          case 0xAA:
            DBG_LOG_INST("RES 5, D");
            InstrResetBit(5, reg_file.D);
            break;
          case 0xAB:
            DBG_LOG_INST("RES 5, E");
            InstrResetBit(5, reg_file.E);
            break;
          case 0xAC:
            DBG_LOG_INST("RES 5, H");
            InstrResetBit(5, reg_file.H);
            break;
          case 0xAD:
            DBG_LOG_INST("RES 5, L");
            InstrResetBit(5, reg_file.L);
            break;
          case 0xAE:
            DBG_LOG_INST("RES 5, (HL)");
            InstrResetBit(5, reg_file.HL);
            break;
          case 0xAF:
            DBG_LOG_INST("RES 5, A");
            InstrResetBit(5, reg_file.A);
            break;
          case 0xB0:
            DBG_LOG_INST("RES 6, B");
            InstrResetBit(6, reg_file.B);
            break;
          case 0xB1:
            DBG_LOG_INST("RES 6, C");
            InstrResetBit(6, reg_file.C);
            break;
          case 0xB2:
            DBG_LOG_INST("RES 6, D");
            InstrResetBit(6, reg_file.D);
            break;
          case 0xB3:
            DBG_LOG_INST("RES 6, E");
            InstrResetBit(6, reg_file.E);
            break;
          case 0xB4:
            DBG_LOG_INST("RES 6, H");
            InstrResetBit(6, reg_file.H);
            break;
          case 0xB5:
            DBG_LOG_INST("RES 6, L");
            InstrResetBit(6, reg_file.L);
            break;
          case 0xB6:
            DBG_LOG_INST("RES 6, (HL)");
            InstrResetBit(6, reg_file.HL);
            break;
          case 0xB7:
            DBG_LOG_INST("RES 6, A");
            InstrResetBit(6, reg_file.A);
            break;
          case 0xB8:
            DBG_LOG_INST("RES 7, B");
            InstrResetBit(7, reg_file.B);
            break;
          case 0xB9:
            DBG_LOG_INST("RES 7, C");
            InstrResetBit(7, reg_file.C);
            break;
          case 0xBA:
            DBG_LOG_INST("RES 7, D");
            InstrResetBit(7, reg_file.D);
            break;
          case 0xBB:
            DBG_LOG_INST("RES 7, E");
            InstrResetBit(7, reg_file.E);
            break;
          case 0xBC:
            DBG_LOG_INST("RES 7, H");
            InstrResetBit(7, reg_file.H);
            break;
          case 0xBD:
            DBG_LOG_INST("RES 7, L");
            InstrResetBit(7, reg_file.L);
            break;
          case 0xBE:
            DBG_LOG_INST("RES 7, (HL)");
            InstrResetBit(7, reg_file.HL);
            break;
          case 0xBF:
            DBG_LOG_INST("RES 7, A");
            InstrResetBit(7, reg_file.A);
            break;
          case 0xC0:
            DBG_LOG_INST("SET 0, B");
            InstrSetBitN(0, reg_file.B);
            break;
          case 0xC1:
            DBG_LOG_INST("SET 0, C");
            InstrSetBitN(0, reg_file.C);
            break;
          case 0xC2:
            DBG_LOG_INST("SET 0, D");
            InstrSetBitN(0, reg_file.D);
            break;
          case 0xC3:
            DBG_LOG_INST("SET 0, E");
            InstrSetBitN(0, reg_file.E);
            break;
          case 0xC4:
            DBG_LOG_INST("SET 0, H");
            InstrSetBitN(0, reg_file.H);
            break;
          case 0xC5:
            DBG_LOG_INST("SET 0, L");
            InstrSetBitN(0, reg_file.L);
            break;
          case 0xC6:
            DBG_LOG_INST("SET 0, (HL)");
            InstrSetBitN(0, reg_file.HL);
            break;
          case 0xC7:
            DBG_LOG_INST("SET 0, A");
            InstrSetBitN(0, reg_file.A);
            break;
          case 0xC8:
            DBG_LOG_INST("SET 1, B");
            InstrSetBitN(1, reg_file.B);
            break;
          case 0xC9:
            DBG_LOG_INST("SET 1, C");
            InstrSetBitN(1, reg_file.C);
            break;
          case 0xCA:
            DBG_LOG_INST("SET 1, D");
            InstrSetBitN(1, reg_file.D);
            break;
          case 0xCB:
            DBG_LOG_INST("SET 1, E");
            InstrSetBitN(1, reg_file.E);
            break;
          case 0xCC:
            DBG_LOG_INST("SET 1, H");
            InstrSetBitN(1, reg_file.H);
            break;
          case 0xCD:
            DBG_LOG_INST("SET 1, L");
            InstrSetBitN(1, reg_file.L);
            break;
          case 0xCE:
            DBG_LOG_INST("SET 1, (HL)");
            InstrSetBitN(1, reg_file.HL);
            break;
          case 0xCF:
            DBG_LOG_INST("SET 1, A");
            InstrSetBitN(1, reg_file.A);
            break;
          case 0xD0:
            DBG_LOG_INST("SET 2, B");
            InstrSetBitN(2, reg_file.B);
            break;
          case 0xD1:
            DBG_LOG_INST("SET 2, C");
            InstrSetBitN(2, reg_file.C);
            break;
          case 0xD2:
            DBG_LOG_INST("SET 2, D");
            InstrSetBitN(2, reg_file.D);
            break;
          case 0xD3:
            DBG_LOG_INST("SET 2, E");
            InstrSetBitN(2, reg_file.E);
            break;
          case 0xD4:
            DBG_LOG_INST("SET 2, H");
            InstrSetBitN(2, reg_file.H);
            break;
          case 0xD5:
            DBG_LOG_INST("SET 2, L");
            InstrSetBitN(2, reg_file.L);
            break;
          case 0xD6:
            DBG_LOG_INST("SET 2, (HL)");
            InstrSetBitN(2, reg_file.HL);
            break;
          case 0xD7:
            DBG_LOG_INST("SET 2, A");
            InstrSetBitN(2, reg_file.A);
            break;
          case 0xD8:
            DBG_LOG_INST("SET 3, B");
            InstrSetBitN(3, reg_file.B);
            break;
          case 0xD9:
            DBG_LOG_INST("SET 3, C");
            InstrSetBitN(3, reg_file.C);
            break;
          case 0xDA:
            DBG_LOG_INST("SET 3, D");
            InstrSetBitN(3, reg_file.D);
            break;
          case 0xDB:
            DBG_LOG_INST("SET 3, E");
            InstrSetBitN(3, reg_file.E);
            break;
          case 0xDC:
            DBG_LOG_INST("SET 3, H");
            InstrSetBitN(3, reg_file.H);
            break;
          case 0xDD:
            DBG_LOG_INST("SET 3, L");
            InstrSetBitN(3, reg_file.L);
            break;
          case 0xDE:
            DBG_LOG_INST("SET 3, (HL)");
            InstrSetBitN(3, reg_file.HL);
            break;
          case 0xDF:
            DBG_LOG_INST("SET 3, A");
            InstrSetBitN(3, reg_file.A);
            break;
          case 0xE0:
            DBG_LOG_INST("SET 4, B");
            InstrSetBitN(4, reg_file.B);
            break;
          case 0xE1:
            DBG_LOG_INST("SET 4, C");
            InstrSetBitN(4, reg_file.C);
            break;
          case 0xE2:
            DBG_LOG_INST("SET 4, D");
            InstrSetBitN(4, reg_file.D);
            break;
          case 0xE3:
            DBG_LOG_INST("SET 4, E");
            InstrSetBitN(4, reg_file.E);
            break;
          case 0xE4:
            DBG_LOG_INST("SET 4, H");
            InstrSetBitN(4, reg_file.H);
            break;
          case 0xE5:
            DBG_LOG_INST("SET 4, L");
            InstrSetBitN(4, reg_file.L);
            break;
          case 0xE6:
            DBG_LOG_INST("SET 4, (HL)");
            InstrSetBitN(4, reg_file.HL);
            break;
          case 0xE7:
            DBG_LOG_INST("SET 4, A");
            InstrSetBitN(4, reg_file.A);
            break;
          case 0xE8:
            DBG_LOG_INST("SET 5, B");
            InstrSetBitN(5, reg_file.B);
            break;
          case 0xE9:
            DBG_LOG_INST("SET 5, C");
            InstrSetBitN(5, reg_file.C);
            break;
          case 0xEA:
            DBG_LOG_INST("SET 5, D");
            InstrSetBitN(5, reg_file.D);
            break;
          case 0xEB:
            DBG_LOG_INST("SET 5, E");
            InstrSetBitN(5, reg_file.E);
            break;
          case 0xEC:
            DBG_LOG_INST("SET 5, H");
            InstrSetBitN(5, reg_file.H);
            break;
          case 0xED:
            DBG_LOG_INST("SET 5, L");
            InstrSetBitN(5, reg_file.L);
            break;
          case 0xEE:
            DBG_LOG_INST("SET 5, (HL)");
            InstrSetBitN(5, reg_file.HL);
            break;
          case 0xEF:
            DBG_LOG_INST("SET 5, A");
            InstrSetBitN(5, reg_file.A);
            break;
          case 0xF0:
            DBG_LOG_INST("SET 6, B");
            InstrSetBitN(6, reg_file.B);
            break;
          case 0xF1:
            DBG_LOG_INST("SET 6, C");
            InstrSetBitN(6, reg_file.C);
            break;
          case 0xF2:
            DBG_LOG_INST("SET 6, D");
            InstrSetBitN(6, reg_file.D);
            break;
          case 0xF3:
            DBG_LOG_INST("SET 6, E");
            InstrSetBitN(6, reg_file.E);
            break;
          case 0xF4:
            DBG_LOG_INST("SET 6, H");
            InstrSetBitN(6, reg_file.H);
            break;
          case 0xF5:
            DBG_LOG_INST("SET 6, L");
            InstrSetBitN(6, reg_file.L);
            break;
          case 0xF6:
            DBG_LOG_INST("SET 6, (HL)");
            InstrSetBitN(6, reg_file.HL);
            break;
          case 0xF7:
            DBG_LOG_INST("SET 6, A");
            InstrSetBitN(6, reg_file.A);
            break;
          case 0xF8:
            DBG_LOG_INST("SET 7, B");
            InstrSetBitN(7, reg_file.B);
            break;
          case 0xF9:
            DBG_LOG_INST("SET 7, C");
            InstrSetBitN(7, reg_file.C);
            break;
          case 0xFA:
            DBG_LOG_INST("SET 7, D");
            InstrSetBitN(7, reg_file.D);
            break;
          case 0xFB:
            DBG_LOG_INST("SET 7, E");
            InstrSetBitN(7, reg_file.E);
            break;
          case 0xFC:
            DBG_LOG_INST("SET 7, H");
            InstrSetBitN(7, reg_file.H);
            break;
          case 0xFD:
            DBG_LOG_INST("SET 7, L");
            InstrSetBitN(7, reg_file.L);
            break;
          case 0xFE:
            DBG_LOG_INST("SET 7, (HL)");
            InstrSetBitN(7, reg_file.HL);
            break;
          case 0xFF:
            DBG_LOG_INST("SET 7, A");
            InstrSetBitN(7, reg_file.A);
            break;
          default:
            std::cout << "UNKNOWN INSTRUCTION: CB 0x" << std::hex <<static_cast<int>(instr_byte)
            << " at PC=0x" << std::hex <<static_cast<int>(reg_file.PC) << std::endl;
            exit(EXIT_FAILURE);
            break;
        }
        break;
      default:
        std::cout << "UNKNOWN INSTRUCTION: 0x" << std::hex <<static_cast<int>(instr_byte)
                  << " at PC=0x" << std::hex <<static_cast<int>(reg_file.PC) << std::endl;
        exit(EXIT_FAILURE);
        break;
    }
  }
}  // NOLINT(readability/fn_size)
