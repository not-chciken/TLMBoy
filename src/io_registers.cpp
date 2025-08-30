/*******************************************************************************
 * Apache License, Version 2.0
 * Copyright (c) 2025 chciken/Niko
 ******************************************************************************/

#include "io_registers.h"

IoRegisters::IoRegisters(sc_module_name name)
    : GenericMemory(0x80, name), sig_unmap_rom_out("sig_unmap_rom_out"), init_socket("init_socket") {
}

// TODO(niko): 160µs.
// In theory it skips 4 bits per entry as these aren't used.
void IoRegisters::DmaTransfer(const u8 byte) {
  u8 read_data;
  u8 write_data;
  sc_time delay = sc_time(0, SC_NS);
  auto read_payload = MakeSharedPayloadPtr(tlm::TLM_READ_COMMAND, 0xFFFFF, &read_data);
  auto write_payload = MakeSharedPayloadPtr(tlm::TLM_WRITE_COMMAND, 0xFFFFF, &write_data);
  const u16 start_address = byte * 0x100;

  for (u16 i = 0x0; i <= 0x9F; ++i) {
    u16 from_address = start_address + i;
    u16 to_address = 0xFE00 + i;
    read_payload->set_address(from_address);

    init_socket->b_transport(*read_payload, delay);

    write_data = read_data;
    write_payload->set_address(to_address);

    init_socket->b_transport(*write_payload, delay);
  }
}

void IoRegisters::b_transport(tlm::tlm_generic_payload& trans, sc_time& delay [[maybe_unused]]) {
  tlm::tlm_command cmd = trans.get_command();
  u16 adr = static_cast<u16>(trans.get_address());
  unsigned char ptr = *trans.get_data_ptr();

  if (cmd == tlm::TLM_READ_COMMAND) {
    unsigned char* ptr = trans.get_data_ptr();
    switch (adr) {
    case 0x00:  // 0xFF10
      *ptr = 0x80u | data_[adr];
      break;
    case 0x01:  // 0xFF11
      *ptr = 0x3Fu | data_[adr];
      break;
    case 0x03:  // 0xFF13
      *ptr = 0xFFu;
      break;
    case 0x04:  // 0xFF14
      *ptr = 0xBFu | data_[adr];
      break;
    case 0x05:  // 0xFF15: Not implemented.
      *ptr = 0xFFu;
      break;
    case 0x06:  // 0xFF16
      *ptr = 0x3Fu | data_[adr];
      break;
    case 0x08:  // 0xFF18
      *ptr = 0xFFu;
      break;
    case 0x09:  // 0xFF19
      *ptr = 0xBFu | data_[adr];
      break;
    case 0x0A:  // 0xFF1A
      *ptr = 0x7Fu | data_[adr];
      break;
    case 0x0B:  // 0xFF1B
      *ptr = 0xFFu;
      break;
    case 0x0C:  // 0xFF1C
      *ptr = 0x9Fu | data_[adr];
      break;
    case 0x0D:  // 0xFF1D
      *ptr = 0xFFu;
      break;
    case 0x0E:  // 0xFF1E
      *ptr = 0xBFu | data_[adr];
      break;
    case 0x0F:  // 0xFF1F: Not implemented.
      *ptr = 0xFFu;
      break;
    case 0x10:  // 0xFF20
      *ptr = 0xFFu;
      break;
    case 0x13:  // 0xFF23
      *ptr = 0xBFu | data_[adr];
      break;
    case 0x16:  // 0xFF26
      *ptr = 0x70u | data_[adr];
      break;
    case 0x17:  // 0xFF27: Not implemented.
    case 0x18:  // 0xFF28: Not implemented.
    case 0x19:  // 0xFF29: Not implemented.
    case 0x1a:  // 0xFF2a: Not implemented.
    case 0x1b:  // 0xFF2b: Not implemented.
    case 0x1c:  // 0xFF2c: Not implemented.
    case 0x1d:  // 0xFF2d: Not implemented.
    case 0x1e:  // 0xFF2e: Not implemented.
    case 0x1f:  // 0xFF2f: Not implemented.
      *ptr = 0xFFu;
      break;
    default:
      *ptr = data_[adr];
    }
    trans.set_response_status(tlm::TLM_OK_RESPONSE);
  } else if (cmd == tlm::TLM_WRITE_COMMAND) {
    trans.set_response_status(tlm::TLM_OK_RESPONSE);

    if (adr == 0x16 && (ptr & 0b10000000u) == 0) {
      u8 tmp = data_[0xa];
      memset(data_, 0, 0x20);  // Turning off APU resets all registers except wave RAM and 0xFF20.
      data_[0xa] = tmp;
    }

    if (adr < 0x20 && adr != 0x16 && (data_[0x16] & 0b10000000u) == 0)
      return;  // APU writes are ignored if APU is turned off.

    switch (adr) {
    case 0x00:  // 0xFF10
      data_[adr] = ptr;
      sig_reload_length_square1_out.write(!sig_reload_length_square1_out.read());
      break;
    case 0x04:  // 0xFF14
      data_[adr] = ptr;
      if (ptr & 0b10000000u)
        sig_trigger_square1_out.write(!sig_trigger_square1_out.read());
      break;
    case 0x06:  // 0xFF16
      data_[adr] = ptr;
      sig_reload_length_square2_out.write(!sig_reload_length_square2_out.read());
      break;
    case 0x09:  // 0xFF19
      data_[adr] = ptr;
      if (ptr & 0b10000000u)
        sig_trigger_square2_out.write(!sig_trigger_square2_out.read());
      break;
    case 0x0B:  // 0xFF1B
      sig_reload_length_wave_out.write(true);
      data_[adr] = ptr;
      break;
    case 0x10:  // 0xFF20
      sig_reload_length_noise_out.write(true);
      data_[adr] = ptr;
      break;
    case 0x13:  // 0xFF23
      data_[adr] = ptr;
      if (ptr & 0b10000000u)
        sig_trigger_noise_out.write(!sig_trigger_noise_out.read());
      break;
    case 0x16:  // 0xFF26
      data_[adr] = ptr & 0b10000000u;
      break;
    case 0x31:  // 0xFF41
      // Mode (first two bits) is read-only. Hence, some masking.
      data_[adr] = (ptr & 0x78) | (data_[adr] & 0x07);
      break;
    case 0x36:  // 0xFF46
      DmaTransfer(ptr);
      break;
    case 0x40:  // Writing "1" to 0xFF50 maps out the rom.
      if (ptr == 1)
        sig_unmap_rom_out.write(true);
      break;
    case 0x7F:
      // TODO(niko) warn!
      break;
    default:
      data_[adr] = ptr;
    }
  } else {
    trans.set_response_status(tlm::TLM_COMMAND_ERROR_RESPONSE);
  }
  return;
}
