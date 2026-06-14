/*******************************************************************************
 * Apache License, Version 2.0
 * Copyright (c) 2026 chciken/Niko
 ******************************************************************************/

#include <gtest/gtest.h>
#include <systemc.h>
#include <tlm.h>

#include "cartridge.h"
#include "utils.h"

const string tlm_boy_root = GetEnvVariable("TLMBOY_ROOT");
const string rom_dummy_path = tlm_boy_root + "/roms/dummy.gb";
const string rom_flappyboy_path = tlm_boy_root + "/roms/flappyboy.gb";
Cartridge* cart_mbc5;
Cartridge* cart_no_mbc;

TEST(CartridgeTestsMbc5, GameInfo) {
  ASSERT_EQ(cart_mbc5->game_info->GetCartridgeType(), "MBC5+BAT+RAM");
  ASSERT_EQ(cart_mbc5->game_info->GetLicenseCode(), "none");
  ASSERT_EQ(cart_mbc5->game_info->GetRamSize(), 32);
  ASSERT_EQ(cart_mbc5->game_info->GetRomSize(), 8);
  ASSERT_EQ(cart_mbc5->game_info->GetRegion(), "Japanese");
  ASSERT_EQ(cart_mbc5->game_info->GetSgbSupport(), false);
  ASSERT_EQ(cart_mbc5->game_info->GetTitle(), "DUMMY");
  ASSERT_EQ(cart_mbc5->game_info->GetUsesNewLicense(), true);
}

TEST(CartridgeTestsMbc5, Rom) {
  sc_time delay = SC_ZERO_TIME;
  u8 data = 0;

  // Read lower, fixed ROM Bank.
  auto read_payload = MakeSharedPayloadPtr(tlm::TLM_READ_COMMAND, 0x1000, &data);
  cart_mbc5->mbc->b_transport_rom(*read_payload, delay);
  ASSERT_EQ(data, 0x91u);

  // Read upper ROM Bank 0 (file offset 0x4000, first byte = 0xaf).
  read_payload = MakeSharedPayloadPtr(tlm::TLM_READ_COMMAND, 0x4000, &data);
  cart_mbc5->mbc->b_transport_rom(*read_payload, delay);
  ASSERT_EQ(data, 0xafu);
  ASSERT_EQ(cart_mbc5->mbc->GetRomInd(), 0);

  // Switch to Bank 1: write 1 to MBC5 ROM bank register (0x2000-0x2FFF).
  data = 1;
  auto write_payload = MakeSharedPayloadPtr(tlm::TLM_WRITE_COMMAND, 0x2000, &data);
  cart_mbc5->mbc->b_transport_rom(*write_payload, delay);
  ASSERT_EQ(cart_mbc5->mbc->GetRomInd(), 1);

  // Read upper ROM Bank 1(file offset 0x8000, first byte = 0xf8).
  read_payload = MakeSharedPayloadPtr(tlm::TLM_READ_COMMAND, 0x4000, &data);
  cart_mbc5->mbc->b_transport_rom(*read_payload, delay);
  ASSERT_EQ(data, 0xf8u);

  // Debug should read the same value.
  data = 0x00;
  read_payload = MakeSharedPayloadPtr(tlm::TLM_READ_COMMAND, 0x4000, &data);
  cart_mbc5->mbc->transport_dbg_rom(*read_payload);
  ASSERT_EQ(data, 0xf8u);

  // Read a second byte from the same bank to confirm the whole bank is mapped.
  read_payload = MakeSharedPayloadPtr(tlm::TLM_READ_COMMAND, 0x4001, &data);
  cart_mbc5->mbc->b_transport_rom(*read_payload, delay);
  ASSERT_EQ(data, 0x09u);

  // Switch to Bank 2: write 2 to 0x2000.
  data = 2;
  write_payload = MakeSharedPayloadPtr(tlm::TLM_WRITE_COMMAND, 0x2000, &data);
  cart_mbc5->mbc->b_transport_rom(*write_payload, delay);

  // Read Bank 2 (file offset 0xC000, first byte = 0x21).
  read_payload = MakeSharedPayloadPtr(tlm::TLM_READ_COMMAND, 0x4000, &data);
  cart_mbc5->mbc->b_transport_rom(*read_payload, delay);
  ASSERT_EQ(data, 0x21u);
  ASSERT_EQ(cart_mbc5->mbc->GetRomInd(), 2);
}

TEST(CartridgeTestsMbc5, Ram) {
  sc_time delay = SC_ZERO_TIME;
  u8 data = 0;

  // Enable RAM by writing 0x0A to 0x0000 via the ROM socket (MBC5 spec).
  data = 0x0A;
  auto enable_payload = MakeSharedPayloadPtr(tlm::TLM_WRITE_COMMAND, 0x0000, &data);
  cart_mbc5->mbc->b_transport_rom(*enable_payload, delay);
  ASSERT_EQ(cart_mbc5->mbc->GetRamInd(), 0);
  ASSERT_EQ(cart_mbc5->mbc->ext_ram.GetCurrentBankIndex(), 0);

  // Write a value to RAM bank 0, address 0x0000.
  data = 0xBE;
  auto write_payload = MakeSharedPayloadPtr(tlm::TLM_WRITE_COMMAND, 0x0000, &data);
  cart_mbc5->mbc->b_transport_ram(*write_payload, delay);

  // Read it back and verify.
  data = 0x00;
  auto read_payload = MakeSharedPayloadPtr(tlm::TLM_READ_COMMAND, 0x0000, &data);
  cart_mbc5->mbc->b_transport_ram(*read_payload, delay);
  ASSERT_EQ(data, 0xBEu);

  // Debug should read the same value.
  data = 0x00;
  cart_mbc5->mbc->transport_dbg_ram(*read_payload);
  ASSERT_EQ(data, 0xBEu);

  // Switch to RAM bank 1 by writing 1 to 0x4000 via b_transport_rom.
  data = 1;
  auto bank_payload = MakeSharedPayloadPtr(tlm::TLM_WRITE_COMMAND, 0x4000, &data);
  cart_mbc5->mbc->b_transport_rom(*bank_payload, delay);
  ASSERT_EQ(cart_mbc5->mbc->GetRamInd(), 1);
  ASSERT_EQ(cart_mbc5->mbc->ext_ram.GetCurrentBankIndex(), 1);

  // Write a different value to Bank 1, same address.
  data = 0xEF;
  write_payload = MakeSharedPayloadPtr(tlm::TLM_WRITE_COMMAND, 0x0000, &data);
  cart_mbc5->mbc->b_transport_ram(*write_payload, delay);

  // Confirm Bank 1 holds the new value.
  data = 0x00;
  read_payload = MakeSharedPayloadPtr(tlm::TLM_READ_COMMAND, 0x0000, &data);
  cart_mbc5->mbc->b_transport_ram(*read_payload, delay);
  ASSERT_EQ(data, 0xEFu);

  // Switch back to Bank 0 and confirm the original value is still there.
  data = 0;
  bank_payload = MakeSharedPayloadPtr(tlm::TLM_WRITE_COMMAND, 0x4000, &data);
  cart_mbc5->mbc->b_transport_rom(*bank_payload, delay);

  data = 0x00;
  read_payload = MakeSharedPayloadPtr(tlm::TLM_READ_COMMAND, 0x0000, &data);
  cart_mbc5->mbc->b_transport_ram(*read_payload, delay);
  ASSERT_EQ(data, 0xBEu);

  // Also verify writes at the top of the RAM bank (0x1FFF).
  data = 0x42;
  write_payload = MakeSharedPayloadPtr(tlm::TLM_WRITE_COMMAND, 0x1FFF, &data);
  cart_mbc5->mbc->b_transport_ram(*write_payload, delay);

  data = 0x00;
  read_payload = MakeSharedPayloadPtr(tlm::TLM_READ_COMMAND, 0x1FFF, &data);
  cart_mbc5->mbc->b_transport_ram(*read_payload, delay);
  ASSERT_EQ(data, 0x42u);
}

TEST(CartridgeTestsNoMbc, GameInfo) {
  ASSERT_EQ(cart_no_mbc->game_info->GetCartridgeType(), "ROM ONLY");
  ASSERT_EQ(cart_no_mbc->game_info->GetLicenseCode(), "Unknown");
  ASSERT_EQ(cart_no_mbc->game_info->GetRamSize(), 0);
  ASSERT_EQ(cart_no_mbc->game_info->GetRomSize(), 0);
  ASSERT_EQ(cart_no_mbc->game_info->GetRegion(), "Unknown");
  ASSERT_EQ(cart_no_mbc->game_info->GetSgbSupport(), false);
  ASSERT_EQ(cart_no_mbc->game_info->GetTitle(), "BITNENFER ");
  ASSERT_EQ(cart_no_mbc->game_info->GetUsesNewLicense(), false);
}

TEST(CartridgeTestsNoMbc, Rom) {
  sc_time delay = SC_ZERO_TIME;
  u8 data = 0;

  // Read from the low ROM bank (0x0000–0x3FFF).
  auto read_payload = MakeSharedPayloadPtr(tlm::TLM_READ_COMMAND, 0x0000, &data);
  cart_no_mbc->mbc->b_transport_rom(*read_payload, delay);
  ASSERT_EQ(data, 0xc9u);

  read_payload = MakeSharedPayloadPtr(tlm::TLM_READ_COMMAND, 0x0101, &data);
  cart_no_mbc->mbc->b_transport_rom(*read_payload, delay);
  ASSERT_EQ(data, 0xc3u);

  // Read from the fixed high ROM bank (0x4000–0x7FFF); no bank switching.
  read_payload = MakeSharedPayloadPtr(tlm::TLM_READ_COMMAND, 0x40B5, &data);
  cart_no_mbc->mbc->b_transport_rom(*read_payload, delay);
  ASSERT_EQ(data, 0x01u);

  read_payload = MakeSharedPayloadPtr(tlm::TLM_READ_COMMAND, 0x40B6, &data);
  cart_no_mbc->mbc->b_transport_rom(*read_payload, delay);
  ASSERT_EQ(data, 0x02u);

  read_payload = MakeSharedPayloadPtr(tlm::TLM_READ_COMMAND, 0x40B7, &data);
  cart_no_mbc->mbc->b_transport_rom(*read_payload, delay);
  ASSERT_EQ(data, 0x03u);

  // Some games like Tetris try to write into the ROM. Nothing should happen
  data = 0xaau;
  auto write_payload = MakeSharedPayloadPtr(tlm::TLM_WRITE_COMMAND, 0x40B7, &data);
  cart_no_mbc->mbc->b_transport_rom(*write_payload, delay);
  ASSERT_EQ(write_payload->get_response_status(), tlm::TLM_OK_RESPONSE);

  read_payload = MakeSharedPayloadPtr(tlm::TLM_READ_COMMAND, 0x40B7, &data);
  cart_no_mbc->mbc->b_transport_rom(*read_payload, delay);
  ASSERT_EQ(data, 0x03u);
}

TEST(CartridgeTestsNoMbc, Ram) {
  sc_time delay = SC_ZERO_TIME;
  u8 data = 0xab;

  // Some games like Alleyway write into the non-existing RAM. Nothing should happen.
  auto payload = MakeSharedPayloadPtr(tlm::TLM_WRITE_COMMAND, 0x0000, &data);
  cart_no_mbc->mbc->b_transport_ram(*payload, delay);
  ASSERT_EQ(payload->get_response_status(), tlm::TLM_OK_RESPONSE);

  // Also reads from the non-existing RAM are a thing.
  payload = MakeSharedPayloadPtr(tlm::TLM_READ_COMMAND, 0x0000, &data);
  cart_no_mbc->mbc->b_transport_ram(*payload, delay);
  ASSERT_EQ(data, 0x00u);
  ASSERT_EQ(payload->get_response_status(), tlm::TLM_OK_RESPONSE);
}

int sc_main(int argc, char* argv[]) {
  cart_mbc5 = new Cartridge("cartridge_mbc5", rom_dummy_path, "", false, true);
  cart_no_mbc = new Cartridge("cartridge_no_mbc", rom_flappyboy_path, "");
  cart_mbc5->mbc->UnmapBootRom();
  cart_no_mbc->mbc->UnmapBootRom();
  sc_set_time_resolution(1.0, SC_NS);
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
