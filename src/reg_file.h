#pragma once
/*******************************************************************************
 * Copyright (C) 2022 chiken
 * MIT License
 *
 * Here reside the implementations of the registers and the register file.
 * Note: Doesn't work on a big endian machine (see below, compiler will warn you)!
 ******************************************************************************/
#include <bit>
#include <compare>
#include <cstdint>
#include <string>

template<typename T>
class Reg {
 public:
  Reg(std::string name, T &value) : name(name), value_(value) {
  }
  Reg(const Reg&) = delete;

  const std::string name;

  T val() {return value_;}
  void val(T val) {value_ = val;}
  void valByHexStr(const std::string &r) {
    value_ = std::stoi(r, nullptr, 16);
  }

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

struct RegFile {
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
      PC("SP", reinterpret_cast<u16&>(_dat[10])) {
    static_assert(std::endian::native == std::endian::little, "RegFile is not big endian compatible!");
  }

 public:
  std::vector<Reg<u16>*>::iterator begin() { return reg_vec_.begin(); }
  std::vector<Reg<u16>*>::iterator end()   { return reg_vec_.end(); }

 private:
  u8 _dat[12]{0};
  std::vector<Reg<u16>*> reg_vec_{&AF,&BC,&DE,&HL,&SP,&PC};
};
