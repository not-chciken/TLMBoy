#pragma once
/*******************************************************************************
 * Apache License, Version 2.0
 * Copyright (c) 2025 chciken/Niko
 *
 * Collection of classes that are all associated to the Game Boy's cartridge.
 * See: https://gbdev.gg8.se/wiki/articles/Memory_Bank_Controllers
 ******************************************************************************/
#include <filesystem>
#include <memory>

#include "common.h"
#include "game_info.h"
#include "generic_memory.h"
#include "symfile_tracer.h"

class Cartridge : public sc_module {
  SC_HAS_PROCESS(Cartridge);

  class BankSwitchedMem : public GenericMemory {
   public:
    BankSwitchedMem(sc_module_name name, uint num_banks, uint bank_size);
    void DoBankSwitch(u8 index);
    u8 GetCurrentBankIndex();
    void b_transport(tlm::tlm_generic_payload& trans, sc_time& delay);
    uint transport_dbg(tlm::tlm_generic_payload& trans);

   protected:
    uint current_bank_ind_;
    uint num_banks_;
    uint bank_size_;
    u8* bank_data_;
  };

  class MemoryBankCtrler {
   public:
    MemoryBankCtrler(uint num_rom_banks, uint num_ram_banks, bool symbol_file);
    virtual ~MemoryBankCtrler();

    GenericMemory rom_low;
    BankSwitchedMem rom_high;
    BankSwitchedMem ext_ram;
    tlm_utils::simple_target_socket<MemoryBankCtrler, gb_const::kBusDataWidth> rom_socket_in;
    tlm_utils::simple_target_socket<MemoryBankCtrler, gb_const::kBusDataWidth> ram_socket_in;
    tlm_utils::simple_initiator_socket<MemoryBankCtrler, gb_const::kBusDataWidth> rom_low_socket_out;
    tlm_utils::simple_initiator_socket<MemoryBankCtrler, gb_const::kBusDataWidth> rom_high_socket_out;
    tlm_utils::simple_initiator_socket<MemoryBankCtrler, gb_const::kBusDataWidth> ram_socket_out;
    virtual void b_transport_ram(tlm::tlm_generic_payload& trans, sc_time& delay) = 0;
    virtual void b_transport_rom(tlm::tlm_generic_payload& trans, sc_time& delay) = 0;
    virtual uint transport_dbg_ram(tlm::tlm_generic_payload& trans);
    virtual uint transport_dbg_rom(tlm::tlm_generic_payload& trans);
    virtual void UnmapBootRom();

   protected:
    std::filesystem::path game_path_;
    std::unique_ptr<SymfileTracer> symfile_tracer_;
  };

  class Rom : public MemoryBankCtrler {
   public:
    Rom(std::filesystem::path game_path, std::filesystem::path boot_path, bool symbole_file);
    void b_transport_rom(tlm::tlm_generic_payload& trans, sc_time& delay) override;
    void b_transport_ram(tlm::tlm_generic_payload& trans, sc_time& delay) override;
  };

  class Mbc1 : public MemoryBankCtrler {
   public:
    Mbc1(std::filesystem::path game_path, std::filesystem::path boot_path, bool symbol_file);
    virtual ~Mbc1() override;

    void b_transport_rom(tlm::tlm_generic_payload& trans, sc_time& delay) override;
    void b_transport_ram(tlm::tlm_generic_payload& trans, sc_time& delay) override;

   private:
    u8 rom_bank_low_bits;
    u8 the_two_bits_;
    u8 rom_ind_;
    u8 ram_ind_;
    bool more_ram_mode_;
    bool ram_enabled_;
    std::filesystem::path save_file;
  };

  class Mbc3 : public MemoryBankCtrler {
   public:
    Mbc3(std::filesystem::path game_path, std::filesystem::path boot_path, bool symbol_file);
    virtual ~Mbc3() override;

    void b_transport_rom(tlm::tlm_generic_payload& trans, sc_time& delay) override;
    void b_transport_ram(tlm::tlm_generic_payload& trans, sc_time& delay) override;

   private:
    u8 rom_ind_;
    u8 ram_ind_;
    uint rtc_reg_;
    bool ram_rtc_enabled_;
    bool rtc_mapped_;
    bool rtc_halted_;
    std::filesystem::path save_file;
  };

  class Mbc5 : public MemoryBankCtrler {
   public:
    Mbc5(std::filesystem::path game_path, std::filesystem::path boot_path, bool symbol_file);
    void b_transport_rom(tlm::tlm_generic_payload& trans, sc_time& delay) override;
    void b_transport_ram(tlm::tlm_generic_payload& trans, sc_time& delay) override;
    uint transport_dbg_ram(tlm::tlm_generic_payload& trans) override;

   private:
    u16 rom_ind_;
    u16 ram_ind_;
    u16 ram_bits_;
    u16 rom_bank_low_bits_;
    u16 rom_bank_high_bits_;
    bool ram_enabled_;
  };

 public:
  std::unique_ptr<GameInfo> game_info;  // Needed for MBC selection.
  std::unique_ptr<MemoryBankCtrler> mbc;
  sc_in<bool> sig_unmap_rom_in;

  Cartridge(sc_module_name name, std::filesystem::path game_path, std::filesystem::path boot_path,
            bool symbol_file = false);

  void SigHandler();

 private:
  std::filesystem::path game_path_;
  std::filesystem::path boot_path_;
};
