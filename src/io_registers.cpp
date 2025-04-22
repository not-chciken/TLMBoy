/*******************************************************************************
 * Apache License, Version 2.0
 * Copyright (c) 2023 chciken/Niko
 ******************************************************************************/

#include "io_registers.h"

IoRegisters::IoRegisters()
    : GenericMemory(0x80, "IoRegisters"), sig_unmap_rom_out("sig_unmap_rom_out"), init_socket("init_socket") {
}

// TODO(niko): 160µs. CONTINUE HERE
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
  unsigned char* ptr = trans.get_data_ptr();

  if (cmd == tlm::TLM_READ_COMMAND) {
    memcpy(ptr, &data_[adr], 1);
    trans.set_response_status(tlm::TLM_OK_RESPONSE);
  } else if (cmd == tlm::TLM_WRITE_COMMAND) {
    switch (adr) {
    case 0x00:  // 0xFF10
      memcpy(&data_[adr], ptr, 1);
      sig_reload_length_square1_out.write(!sig_reload_length_square1_out.read());
      break;
    case 0x04:  // 0xFF14
      memcpy(&data_[adr], ptr, 1);
      if (*ptr & 0b10000000u)
        sig_trigger_square1_out.write(!sig_trigger_square1_out.read());
      break;
    case 0x06:  // 0xFF16
      memcpy(&data_[adr], ptr, 1);
      sig_reload_length_square2_out.write(!sig_reload_length_square2_out.read());
      break;
    case 0x09:  // 0xFF19
      memcpy(&data_[adr], ptr, 1);
      if (*ptr & 0b10000000u)
        sig_trigger_square2_out.write(!sig_trigger_square2_out.read());
      break;
    case 0x0B:  // 0xFF1B
      sig_reload_length_wave_out.write(true);
      memcpy(&data_[adr], ptr, 1);
      break;
    case 0x10:  // 0xFF20
      sig_reload_length_noise_out.write(true);
      memcpy(&data_[adr], ptr, 1);
      break;
    case 0x13:  // 0xFF23
      memcpy(&data_[adr], ptr, 1);
      if (*ptr & 0b10000000u)
        sig_trigger_noise_out.write(!sig_trigger_noise_out.read());
      break;
    case 0x31:  // 0xFF41
      // Mode (first two bits) is read-only. Hence, some masking.
      data_[adr] = (*ptr & 0x78) | (data_[adr] & 0x07);
      break;
    case 0x36:  // 0xFF46
      DmaTransfer(*ptr);
      break;
    case 0x40:  // Writing "1" to 0xFF50 maps out the rom.
      if (*ptr == 1)
        sig_unmap_rom_out.write(true);
      break;
    case 0x7F:
      // TODO(niko) warn!
      break;
    default:
      memcpy(&data_[adr], ptr, 1);
    }
    trans.set_response_status(tlm::TLM_OK_RESPONSE);
  } else {
    trans.set_response_status(tlm::TLM_COMMAND_ERROR_RESPONSE);
  }
  return;
}
