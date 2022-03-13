
/************************************************************************************
 * Copyright (C) 2022 chciken/Niko
 * MIT License
 *
 * Collection of tests for "gdb_server.(cpp|h)", which implements the GDB
 * remote serial protocol.
 ************************************************************************************/

#include <cstdlib>
#include <chrono>
#include <future>
#include <gtest/gtest.h>
#include <string>

#include "utils.h"

using namespace std::chrono_literals;
const std::chrono::seconds kTestTimeoutS = 50s;

const std::string tlm_boy_root = GetEnvVariable("TLMBOY_ROOT");
const std::string exe = tlm_boy_root + "/build/tlmboy_test";
const std::string rom_path = tlm_boy_root + "/roms/dummy.bin";
const std::string boot_rom_path = tlm_boy_root + "/roms/DMG_ROM.bin";
const std::string cmd_test = exe + " --max-cycles=800 --headless --wait-for-gdb -r "
                  + rom_path + " -b " + boot_rom_path;
const std::string cmd_test_wo_max = exe + " --headless --wait-for-gdb -r "
                  + rom_path + " -b " + boot_rom_path;

// This test boots up the Gameboy and compares the states of the register
// against a golden file once per machine cycle using the GDB remote
TEST(GdbTests, SystemTest) {
  std::string boot_checker_path = tlm_boy_root + "/tests/gdb/boot_checker_gdb.py";
  std::string cmd_gdb = "z80-unknown-elf-gdb --batch -x " + boot_checker_path;
  std::cout << "Executing: " << cmd_test << std::endl
            << "Executing: " << cmd_gdb << std::endl;
  auto fut_tlm = std::async(std::launch::async, std::system, cmd_test.c_str());
  auto fut_gdb = std::async(std::launch::async, std::system, cmd_gdb.c_str());
  ASSERT_EQ(fut_tlm.wait_for(kTestTimeoutS), std::future_status::ready);
  ASSERT_EQ(fut_gdb.wait_for(kTestTimeoutS), std::future_status::ready);
  ASSERT_EQ(fut_tlm.get(), 0);
  ASSERT_EQ(fut_gdb.get(), 0);
}

void FailTest(std::string script_path) {
  std::string cmd_script = tlm_boy_root + script_path;
  std::cout << "Executing: " << cmd_script << std::endl
            << "Executing: " << cmd_test_wo_max << std::endl;
  auto fut_tlm = std::async(std::launch::async, std::system, cmd_test_wo_max.c_str());
  auto fut_tcp = std::async(std::launch::async, std::system, cmd_script.c_str());
  ASSERT_EQ(fut_tlm.wait_for(kTestTimeoutS), std::future_status::ready);
  ASSERT_EQ(fut_tcp.wait_for(kTestTimeoutS), std::future_status::ready);
  ASSERT_EQ(fut_tlm.get(), 1 << 8);
  ASSERT_EQ(fut_tcp.get(), 0);
}

// In this test a "-" is sent to trigger a protocol error.
TEST(GdbTests, ProtocolError) {
  FailTest("/tests/gdb/gdb_commands.bash ProtocolError");
}

// Sends a wrong checksum.
TEST(GdbTests, WrongChecksum) {
  FailTest("/tests/gdb/gdb_commands.bash WrongChecksum");
}

// Messages have to begin with a "$". This tests a fail of a message not beginning with $.
TEST(GdbTests, WrongBegin) {
  FailTest("/tests/gdb/gdb_commands.bash WrongBegin");
}

// GDB server throws an exception if the received message is too long.
TEST(GdbTests, MessageTooLong) {
  FailTest("/tests/gdb/gdb_commands.bash MessageTooLong");
}

int sc_main(int argc, char* argv[]) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
