/*******************************************************************************
 * Apache License, Version 2.0
 * Copyright (c) 2025 chciken/Niko
 ******************************************************************************/
#include "cartridge.h"

#include <format>
#include <string>

Cartridge::BankSwitchedMem::BankSwitchedMem(sc_module_name name, uint num_banks, uint bank_size)
    : GenericMemory(bank_size * num_banks, name), num_banks_(num_banks), bank_size_(bank_size) {
  DoBankSwitch(0);
}

void Cartridge::BankSwitchedMem::DoBankSwitch(u8 index) {
  assert(index < num_banks_);
  bank_data_ = &data_[index * bank_size_];
  current_bank_ind_ = index;
}

u8 Cartridge::BankSwitchedMem::GetCurrentBankIndex() {
  return current_bank_ind_;
}

void Cartridge::BankSwitchedMem::b_transport(tlm::tlm_generic_payload& trans, sc_time& delay [[maybe_unused]]) {
  u16 adr = static_cast<u16>(trans.get_address());
  assert(adr < bank_size_);
  tlm::tlm_command cmd = trans.get_command();
  unsigned char* ptr = trans.get_data_ptr();
  if (cmd == tlm::TLM_READ_COMMAND) {
    *ptr = bank_data_[adr];
    trans.set_response_status(tlm::TLM_OK_RESPONSE);
  } else if (cmd == tlm::TLM_WRITE_COMMAND) {
    bank_data_[adr] = *ptr;
    trans.set_response_status(tlm::TLM_OK_RESPONSE);
  } else {
    trans.set_response_status(tlm::TLM_COMMAND_ERROR_RESPONSE);
  }
}

uint Cartridge::BankSwitchedMem::transport_dbg(tlm::tlm_generic_payload& trans) {
  sc_time delay = sc_time(0, SC_NS);
  b_transport(trans, delay);
  return 1;
}

Cartridge::MemoryBankCtrler::MemoryBankCtrler(uint num_rom_banks, uint num_ram_banks, bool symbol_file)
    : rom_low(0x4000, "rom_low"),
      rom_high("rom_high", num_rom_banks, 0x4000),
      ext_ram("ext_ram", num_ram_banks, 0x2000) {
  rom_socket_in.register_b_transport(this, &MemoryBankCtrler::b_transport_rom);
  ram_socket_in.register_b_transport(this, &MemoryBankCtrler::b_transport_ram);
  rom_socket_in.register_transport_dbg(this, &MemoryBankCtrler::transport_dbg_rom);
  ram_socket_in.register_transport_dbg(this, &MemoryBankCtrler::transport_dbg_ram);
  rom_low_socket_out.bind(rom_low.targ_socket);
  rom_high_socket_out.bind(rom_high.targ_socket);
  ram_socket_out.bind(ext_ram.targ_socket);

  if (symbol_file)
    symfile_tracer_ = std::make_unique<SymfileTracer>();
}

Cartridge::MemoryBankCtrler::~MemoryBankCtrler() {
  if (symfile_tracer_) {
    std::ofstream file;
    file.open("trace.sym");
    symfile_tracer_->DumpTrace(file);
    file.close();
  }
}

// When debugging, we'll allow writes into the ROM.
uint Cartridge::MemoryBankCtrler::transport_dbg_rom(tlm::tlm_generic_payload& trans) {
  u16 adr = static_cast<u16>(trans.get_address());
  assert(adr < 0x8000);
  if (adr < 0x4000) {
    rom_low_socket_out->transport_dbg(trans);
  } else {
    trans.set_address(adr - 0x4000);
    rom_high_socket_out->transport_dbg(trans);
  }
  return 1;
}

uint Cartridge::MemoryBankCtrler::transport_dbg_ram(tlm::tlm_generic_payload& trans) {
  sc_time delay = sc_time(0, SC_NS);
  b_transport_ram(trans, delay);
  return 1;
}

void Cartridge::MemoryBankCtrler::UnmapBootRom() {
  rom_low.LoadFromFile(game_path_);
}

Cartridge::Rom::Rom(std::filesystem::path game_path, std::filesystem::path boot_path, bool symbole_file)
    : MemoryBankCtrler(1, 1, symbole_file) {
  assert(game_path != "");
  assert(boot_path != "");
  game_path_ = game_path;
  rom_low.LoadFromFile(game_path);
  rom_low.LoadFromFile(boot_path);
  rom_high.LoadFromFile(game_path, 0x4000);
}

void Cartridge::Rom::b_transport_rom(tlm::tlm_generic_payload& trans, sc_time& delay) {
  u16 adr = static_cast<u16>(trans.get_address());
  tlm::tlm_command cmd = trans.get_command();
  assert(adr < 0x8000);
  if (cmd == tlm::TLM_WRITE_COMMAND) {
    // Some games like tetris try to write in the ROM...
    trans.set_response_status(tlm::TLM_OK_RESPONSE);
    return;
  }
  GbCommand* gbcmd;
  trans.get_extension<GbCommand>(gbcmd);
  if (adr < 0x4000) {
    if (gbcmd && symfile_tracer_)
      if (gbcmd->cmd == GbCommand::kGbReadData)
        symfile_tracer_->TraceAccess(0, adr);
    rom_low_socket_out->b_transport(trans, delay);
  } else {
    if (gbcmd && symfile_tracer_)
      if (gbcmd->cmd == GbCommand::kGbReadData)
        symfile_tracer_->TraceAccess(rom_high.GetCurrentBankIndex(), adr);
    trans.set_address(adr - 0x4000);
    rom_high_socket_out->b_transport(trans, delay);
  }
}

// There's no RAM for rom-only games.
// Yet some games like Alleyway try to write in the non-existing RAM...
void Cartridge::Rom::b_transport_ram(tlm::tlm_generic_payload& trans, sc_time& delay [[maybe_unused]]) {
  assert(static_cast<u16>(trans.get_address()) < 0x8000);

  tlm::tlm_command cmd = trans.get_command();
  trans.set_response_status(tlm::TLM_OK_RESPONSE);
  if (cmd == tlm::TLM_WRITE_COMMAND) {
    std::cout << "[WARNING] Tried to write into non-existing RAM!" << std::endl;
    return;
  } else if (cmd == tlm::TLM_READ_COMMAND) {
    unsigned char* data = trans.get_data_ptr();
    *data = 0;
    std::cout << "[WARNING] Tried to read from non-existing RAM!" << std::endl;
  }
}

Cartridge::Mbc1::Mbc1(std::filesystem::path game_path, std::filesystem::path boot_path, bool symbol_file)
    : MemoryBankCtrler(128, 4, symbol_file),
      rom_bank_low_bits(0),
      the_two_bits_(0),
      rom_ind_(0),
      ram_ind_(0),
      more_ram_mode_(false),
      ram_enabled_(false) {
  game_path_ = game_path;
  rom_low.LoadFromFile(game_path);
  rom_low.LoadFromFile(boot_path);
  rom_high.LoadFromFile(game_path, 0x4000);
}

void Cartridge::Mbc1::b_transport_rom(tlm::tlm_generic_payload& trans, sc_time& delay) {
  tlm::tlm_command cmd = trans.get_command();
  u16 adr = static_cast<u16>(trans.get_address());
  u8* ptr = reinterpret_cast<u8*>(trans.get_data_ptr());

  assert(adr < 0x8000);
  if (cmd == tlm::TLM_WRITE_COMMAND) {
    if (adr <= 0x1FFF) {
      ram_enabled_ = (*ptr & 0xA) == 0xA;
    } else if (adr >= 0x2000 && adr <= 0x3FFF) {
      rom_bank_low_bits = 0b00011111 & *ptr;
    } else if (adr >= 0x4000 && adr <= 0x5FFF) {
      the_two_bits_ = *ptr & 0b00000011;
    } else if (adr >= 0x6000 && adr <= 0x7FFF) {
      more_ram_mode_ = static_cast<bool>(*ptr & 0b0000001);
    }
    rom_ind_ = rom_bank_low_bits | (more_ram_mode_ ? 0 : (the_two_bits_ << 5));
    if (rom_ind_ == 0 || rom_ind_ == 0x20 || rom_ind_ == 0x40 || rom_ind_ == 0x60) {
      rom_ind_ += 1;  // It's not a bug, it's a feature!
    }
    ram_ind_ = more_ram_mode_ ? the_two_bits_ : 0;
    assert(0 < rom_ind_ && rom_ind_ < 128);
    assert(more_ram_mode_ || (!more_ram_mode_ && (ram_ind_ == 0)));  // Only one bank in 4/32 Mode.
    rom_high.DoBankSwitch(rom_ind_ - 1);
    ext_ram.DoBankSwitch(ram_ind_);
  }
  if (cmd == tlm::TLM_READ_COMMAND) {
    GbCommand* gbcmd;
    trans.get_extension<GbCommand>(gbcmd);
    if (adr <= 0x3FFF) {
      if (gbcmd && symfile_tracer_)
        if (gbcmd->cmd == GbCommand::kGbReadData)
          symfile_tracer_->TraceAccess(0, adr);
      rom_low_socket_out->b_transport(trans, delay);
    } else if (adr <= 0x7FFF) {
      if (gbcmd && symfile_tracer_)
        if (gbcmd->cmd == GbCommand::kGbReadData)
          symfile_tracer_->TraceAccess(rom_high.GetCurrentBankIndex(), adr);
      trans.set_address(adr - 0x4000);
      rom_high_socket_out->b_transport(trans, delay);
    }
  }
}

void Cartridge::Mbc1::b_transport_ram(tlm::tlm_generic_payload& trans, sc_time& delay) {
  assert(static_cast<u16>(trans.get_address()) <= 0x1FFF);
  if (ram_enabled_) {
    ram_socket_out->b_transport(trans, delay);
  } else {
    std::cout << "[WARNING] Tried to write into disabled RAM!" << std::endl;
  }
}

Cartridge::Mbc5::Mbc5(std::filesystem::path game_path, std::filesystem::path boot_path, bool symbol_file)
    : MemoryBankCtrler(512, 16, symbol_file), rom_ind_(0), ram_ind_(0), ram_enabled_(false) {
  game_path_ = game_path;
  rom_low.LoadFromFile(game_path);
  rom_low.LoadFromFile(boot_path);
  rom_high.LoadFromFile(game_path, 0x4000);
}

void Cartridge::Mbc5::b_transport_rom(tlm::tlm_generic_payload& trans, sc_time& delay) {
  tlm::tlm_command cmd = trans.get_command();
  u16 adr = static_cast<u16>(trans.get_address());
  u8* ptr = reinterpret_cast<u8*>(trans.get_data_ptr());

  assert(adr < 0x8000);
  if (cmd == tlm::TLM_WRITE_COMMAND) {
    if (adr <= 0x1FFF) {
      ram_enabled_ = (*ptr & 0xA) == 0xA;
    } else if (adr >= 0x2000 && adr <= 0x2FFF) {
      rom_bank_low_bits_ = *ptr;
    } else if (adr >= 0x3000 && adr <= 0x3FFF) {
      rom_bank_high_bits_ = *ptr & 1;
    } else if (adr >= 0x4000 && adr <= 0x5FFF) {
      ram_bits_ = *ptr & 0x0F;
    }
    ram_ind_ = ram_enabled_ ? ram_bits_ : 0;
    rom_ind_ = (rom_bank_high_bits_ << 8) & rom_bank_low_bits_;
    rom_high.DoBankSwitch(rom_ind_);  // TODO(niko): Bank 0
    ext_ram.DoBankSwitch(ram_ind_);
  }
  if (cmd == tlm::TLM_READ_COMMAND) {
    if (adr <= 0x3FFF) {
      rom_low_socket_out->b_transport(trans, delay);
    } else if (adr <= 0x7FFF) {
      trans.set_address(adr - 0x4000);
      rom_high_socket_out->b_transport(trans, delay);
    }
  }
}

void Cartridge::Mbc5::b_transport_ram(tlm::tlm_generic_payload& trans, sc_time& delay) {
  assert(static_cast<u16>(trans.get_address()) <= 0x1FFF);
  if (ram_enabled_) {
    ram_socket_out->b_transport(trans, delay);
  } else {
    assert(false);
  }
}

uint Cartridge::Mbc5::transport_dbg_ram(tlm::tlm_generic_payload& trans) {
  assert(static_cast<u16>(trans.get_address()) <= 0x1FFF);
  sc_time delay(0, SC_NS);
  ram_socket_out->b_transport(trans, delay);
  return 1;
}

Cartridge::Cartridge(sc_module_name name, std::filesystem::path game_path, std::filesystem::path boot_path,
                     bool symbol_file)
    : sc_module(name), sig_unmap_rom_in("sig_unmap_rom_in"), game_path_(game_path), boot_path_(boot_path) {
  game_info = std::make_unique<GameInfo>(game_path_);
  string cr_type = game_info->GetCartridgeType();

  if (cr_type == "ROM ONLY")
    mbc = std::make_unique<Rom>(game_path, boot_path, symbol_file);
  else if (cr_type == "MBC1"  // TODO(niko): finer granularity and more MBC types
           || cr_type == "MBC1+RAM" || cr_type == "MBC1+BAT+RAM")
    mbc = std::make_unique<Mbc1>(game_path, boot_path, symbol_file);
  else if (cr_type == "MBC5" || cr_type == "MBC5+RAM" || cr_type == "MBC5+BAT+RAM")
    mbc = std::make_unique<Mbc5>(game_path, boot_path, symbol_file);
  else
    throw std::runtime_error(std::format("Cartidge type {} not implemented", cr_type));

  SC_METHOD(SigHandler);
  dont_initialize();
  sensitive << sig_unmap_rom_in;
}

void Cartridge::SigHandler() {
  mbc->UnmapBootRom();
}
