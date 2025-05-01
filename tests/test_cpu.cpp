/*******************************************************************************
 * Apache License, Version 2.0
 * Copyright (c) 2023 chciken/Niko
 *
 * These tests check the functionality of the register file
 * and of a few instructions.
 ******************************************************************************/

#include <gtest/gtest.h>

#include "bus.h"
#include "cpu.h"
#include "generic_memory.h"

struct Top : public sc_module {
  SC_HAS_PROCESS(Top);
  Bus test_bus;
  Cpu test_cpu;
  GenericMemory test_memory;

  explicit Top (sc_module_name name)
      : sc_module(name),
        test_bus("test_bus"),
        test_cpu("test_cpu"),
        test_memory(65536, "test_memory") {
    test_bus.AddBusMaster(&test_cpu.init_socket);
    test_bus.AddBusSlave(&test_memory.targ_socket, 0x0000, 0xFFFF);
  }
};
Top test_top("test_top");

// Unit test: check register assignment
TEST(CpuTests, RegisterAssignment){
  Cpu *cpu = &test_top.test_cpu;
  cpu->reg_file.A = 1;
  cpu->reg_file.B = 1;
  cpu->reg_file.C = 1;
  cpu->reg_file.D = 1;
  cpu->reg_file.E = 1;
  cpu->reg_file.F = 2;
  cpu->reg_file.H = 1;
  cpu->reg_file.L = 1;
  cpu->reg_file.SP = 1;
  cpu->reg_file.PC = 1;
  ASSERT_EQ(cpu->reg_file.A, 1);
  ASSERT_EQ(cpu->reg_file.B, 1);
  ASSERT_EQ(cpu->reg_file.C, 1);
  ASSERT_EQ(cpu->reg_file.D, 1);
  ASSERT_EQ(cpu->reg_file.E, 1);
  ASSERT_EQ(cpu->reg_file.F, 2);
  ASSERT_EQ(cpu->reg_file.H, 1);
  ASSERT_EQ(cpu->reg_file.L, 1);
  ASSERT_EQ(cpu->reg_file.SP ,1);
  ASSERT_EQ(cpu->reg_file.PC ,1);
  ASSERT_EQ(cpu->reg_file.AF ,258);
  ASSERT_EQ(cpu->reg_file.BC ,257);
  ASSERT_EQ(cpu->reg_file.DE ,257);
  ASSERT_EQ(cpu->reg_file.HL ,257);
}

//Unit test: Opcodes 0x00-0x03
TEST(CpuTests, Firstopcodes) {
  test_top.test_cpu.reg_file.PC = 0x0100;
  u8 *data = test_top.test_memory.GetDataPtr();
  data[0x100] = 0x00;  // NOP, 4 cycles

  data[0x101] = 0x01;  // LD BC,0x4223, 12 cycles
  data[0x102] = 0x23;
  data[0x103] = 0x42;

  data[0x104] = 0x02;

  data[0x105] = 0x03;  // INC BC, 8 cycles

  sc_start(25 * gb_const::kNsPerClkCycle, SC_NS);
  ASSERT_EQ(test_top.test_cpu.reg_file.B, 0x42);
  ASSERT_EQ(test_top.test_cpu.reg_file.C, 0x24);
}

int sc_main(int argc, char* argv[]) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}