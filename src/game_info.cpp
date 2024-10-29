/*******************************************************************************
 * Apache License, Version 2.0
 * Copyright (c) 2023 chciken/Niko
 ******************************************************************************/

#include "game_info.h"

GameInfo::GameInfo(std::filesystem::path game_path) {
  std::ifstream file(game_path.string(), std::ios::binary | std::ios::ate);
  if (!file.is_open()) {
    throw std::runtime_error("GameInfo: Failed to open file: " + game_path.string());
  }
  std::streamsize size = file.tellg();
  file.seekg(0, std::ios::beg);
  char* data = new char[size];
  file.seekg(0, ios::beg);
  file.read(data, size);

  // Title
  for (uint i = kAdrTitleStart; i <= kAdrTitleEnd; i++) {
    if (data[i] == '\0')
      break;
    title_ += static_cast<char>(data[i]);
  }

  // License Code
  uses_new_license_ = (data[kAdrOldLicense] == 0x33);
  if (uses_new_license_) {
    string key;
    for (uint i = kAdrNewLicenseStart; i <= kAdrNewLicenseEnd; i++) {
      key += static_cast<char>(data[i]);
    }
    if (new_license_code_map.contains(key)) {
      license_code_ = new_license_code_map.at(key);
    } else {
      license_code_ = "Unknown";
    }
  } else {
    if (old_license_code_map.contains(data[kAdrOldLicense])) {
      license_code_ = old_license_code_map.at(data[kAdrOldLicense]);
    } else {
      license_code_ = "Unknown";
    }
  }

  // SGB
  if (data[kAdrSgbFunc] == 0) {
    sgb_support_ = false;
  } else if (data[kAdrSgbFunc] == 0x03) {
    sgb_support_ = false;
  } else {
    std::cerr << "Wrong SGB value: " << data[kAdrSgbFunc] << std::endl;
  }

  cartridge_type_ = cartridge_type_map.at(data[kAdrCartType]);
  rom_size_ = rom_size_map.at(data[kAdrRomSize]);
  ram_size_ = ram_size_map.at(data[kAdrRamSize]);
  region_ = region_map.contains(data[kAdrRegionCode]) ? region_map.at(data[kAdrRegionCode]) : "Unknown";

  delete[] data;
}

GameInfo::operator string() const {
  std::stringstream ss;
  ss << "########## Game Info ###########" << std::endl
     << "Title:          " << title_ << std::endl
     << "Dev/Publisher:  " << license_code_ << std::endl
     << "License type:   " << (uses_new_license_ ? "New" : "Old") << std::endl
     << "SGB support:    " << sgb_support_ << std::endl
     << "Cartridge type: " << cartridge_type_ << std::endl
     << "ROM size:       " << std::get<0>(rom_size_) << std::endl
     << "RAM size:       " << std::get<0>(ram_size_) << std::endl
     << "################################" << std::endl;
  return ss.str();
}

bool GameInfo::GetUsesNewLicense() {
  return uses_new_license_;
}

bool GameInfo::GetSgbSupport() {
  return sgb_support_;
}

string GameInfo::GetCartridgeType() {
  return cartridge_type_;
}

string GameInfo::GetLicenseCode() {
  return license_code_;
}

string GameInfo::GetRegion() {
  return region_;
}

string GameInfo::GetTitle() {
  return title_;
}

// Returns RAM size in kiB
uint GameInfo::GetRamSize() {
  return std::get<1>(ram_size_);
}

// Returns ROM size in number of banks
uint GameInfo::GetRomSize() {
  return std::get<1>(rom_size_);
}
