namespace fs = std::filesystem;

namespace options {
  bool headless = false;
  bool wait_for_gdb = false;
  fs::path rom_path = "";
  fs::path boot_rom_path = "../roms/DMG_ROM.bin";
}
