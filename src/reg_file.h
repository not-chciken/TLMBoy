#pragma once
/*******************************************************************************
 * Copyright (C) 2022 chiken
 * Apache License, Version 2.0
 *
 * Here reside the implementations of the registers and the register file.
 * Note: Doesn't work on a big endian machine (see below, compiler will warn you)!
 ******************************************************************************/
#include <bit>
#include <compare>
#include <cstdint>
#include <string>
#include <vector>

#include "common.h"

// Template implementation of a register.
template<typename T>
class Reg {
 public:
  const std::string name;
  Reg(std::string name, T &value) : name(name), value_(value) {}
  Reg(const Reg&) = delete;
  Reg& operator=(Reg& other) {
    value_ = other.value_;
    return *this;
  }

  // In case you explicitly want to set/get the value.
  T val() {return value_;}
  void val(T val) {value_ = val;}

  Reg& operator=(T value) {
    value_ = value;
    return *this;
  }

  operator T() const {return value_;}

  auto operator<=>(const Reg& r) const {
    return value_ <=> r.value_;
  }

  Reg& operator++() {
    value_++;
    return *this;
  }

  Reg& operator--() {
    value_--;
    return *this;
  }

  Reg& operator+=(const T& r) {
    value_ += r;
    return *this;
  }

  Reg& operator-=(const T& r) {
    value_ -= r;
    return *this;
  }

  Reg& operator^=(const T& r) {
    value_ ^= r;
    return *this;
  }

  Reg& operator&=(const T& r) {
    value_ &= r;
    return *this;
  }

  Reg& operator|=(const T& r) {
    value_ |= r;
    return *this;
  }

  Reg& operator<<=(const T& r) {
    value_ <<= r;
    return *this;
  }

 private:
    T &value_;
};

// Register file of the Game Boy.
// 8-bit registers: A, F, B, C, D, E, H, L
// 8-bit helper registers: SPmsb, SPlsb, PCmsb, PClsb
// 16-bit registers: AF, BC, DE, HL, SP, PC
// Note that registers might use a shared memory.
class RegFile {
static_assert(std::endian::native == std::endian::little, "RegFile is not big endian compatible!");

 public:
  Reg<u8> A, F, B, C, D, E, H, L, SPmsb, SPlsb, PCmsb, PClsb;
  Reg<u16> AF, BC, DE, HL, SP, PC;
  RegFile()
    : A("A", _dat[1]),
      F("F", _dat[0]),
      B("B", _dat[3]),
      C("C", _dat[2]),
      D("D", _dat[5]),
      E("E", _dat[4]),
      H("H", _dat[7]),
      L("L", _dat[6]),
      SPmsb("SPmsb", _dat[9]),
      SPlsb("SPlsb", _dat[8]),
      PCmsb("PCmsb", _dat[11]),
      PClsb("PClsb", _dat[10]),
      AF("AF", reinterpret_cast<u16&>(_dat[0])),
      BC("BC", reinterpret_cast<u16&>(_dat[2])),
      DE("DE", reinterpret_cast<u16&>(_dat[4])),
      HL("HL", reinterpret_cast<u16&>(_dat[6])),
      SP("SP", reinterpret_cast<u16&>(_dat[8])),
      PC("PC", reinterpret_cast<u16&>(_dat[10])) {
  }

  // For the pupose of range-based for-loops á la "for (auto reg : reg_file)".
  std::vector<Reg<u16>*>::iterator begin() { return reg_vec_.begin(); }
  std::vector<Reg<u16>*>::iterator end()   { return reg_vec_.end(); }

 private:
  u8 _dat[12]{0};  // Shared data of the registers.
  std::vector<Reg<u16>*> reg_vec_{&AF, &BC, &DE, &HL, &SP, &PC};  // See begin() and end().
};
