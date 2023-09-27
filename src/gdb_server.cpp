/*******************************************************************************
 * Apache License, Version 2.0
 * Copyright (c) 2023 chciken/Niko
 ******************************************************************************/

#include "gdb_server.h"

#include <bit>
#include <byteswap.h>
#include <format>
#include <functional>
#include <iomanip>
#include <regex>
#include <sstream>
#include <stdexcept>

#include "cpu.h"

GdbServer::GdbServer(Cpu *cpu) : cpu_(cpu), is_attached_(false) {
  cmd_map["?"] = std::bind(&GdbServer::CmdHalted, this, std::placeholders::_1);
  cmd_map["g"] = std::bind(&GdbServer::CmdReadReg, this, std::placeholders::_1);
  cmd_map["G"] = std::bind(&GdbServer::CmdWriteReg, this, std::placeholders::_1);
  cmd_map["D"] = std::bind(&GdbServer::CmdDetach, this, std::placeholders::_1);
  cmd_map["m"] = std::bind(&GdbServer::CmdReadMem, this, std::placeholders::_1);
  cmd_map["M"] = std::bind(&GdbServer::CmdWriteMem, this, std::placeholders::_1);
  cmd_map["c"] = std::bind(&GdbServer::CmdContinue, this, std::placeholders::_1);
  cmd_map["Z"] = std::bind(&GdbServer::CmdInsertBp, this, std::placeholders::_1);
  cmd_map["z"] = std::bind(&GdbServer::CmdRemoveBp, this, std::placeholders::_1);
  cmd_map["qSupported"] = std::bind(&GdbServer::CmdSupported, this, std::placeholders::_1);
  cmd_map["qAttached"]  = std::bind(&GdbServer::CmdAttached, this, std::placeholders::_1);
}

// Waits for a gdb client to attach on the given port. Note, this function is blocking!
// Furthermore, it waits until GBD issues the halt command which is part of the
// initialization process and prevents the CPU from continuing.
void GdbServer::InitBlocking(const int port) {
  tcp_server_.Start(port);
  tcp_server_.AcceptClient();
  std::string msg;
  while (msg != "$qAttached#8f") {
    msg = RecvMsgBlocking();
  }
}

bool GdbServer::IsMsgPending() {
  return tcp_server_.DataAvailable() && is_attached_;
}

void GdbServer::HandleMessages() {
  while (IsMsgPending()) {
    RecvMsgBlocking();
  }
}

// Waits for a packet from the gdb client.
// Note, this function is blocking!
std::string GdbServer::RecvMsgBlocking() {
    std::string str_buffer("");  // Will contain the whole message, e.g: "$vMustReplyEmpty#3a".
    std::string resp = tcp_server_.RecvBlocking(1);
    str_buffer.append(resp);
    uint sum_to_check = 0;
    if (str_buffer.at(0) == '+') {
      DBG_LOG_GDB("GDB message received: " << resp);
      return str_buffer;
    }
    if (str_buffer.at(0) == '-') {
      throw std::logic_error("received '-', protocol error");
    }
    if (str_buffer.at(0) == '\x03') {  // Byte 0x03 is send if "Ctrl-C" is prssed.
      DBG_LOG_GDB("Recived Ctrl+C!");
      std::vector<std::string> dummy;
      CmdHalted(dummy);
      return str_buffer;
    }
    if (str_buffer.at(0) != '$') {
      DBG_LOG_GDB("string buffer: " + str_buffer);
      throw std::logic_error("message not beginning with '$'");
    }
    do {
      resp = tcp_server_.RecvBlocking(1);
      str_buffer.append(resp);
      sum_to_check += static_cast<int>(resp.at(0));
      if (str_buffer.length() > kMaxMessageLength) {
        throw std::logic_error("message is too long");
      }
    } while (resp.at(0) != '#');
    sum_to_check -= '#';

    std::string checksum_str = tcp_server_.RecvBlocking(2);
    const uint checksum = std::stoi(checksum_str, 0, 16);
    str_buffer.append(checksum_str);
    DBG_LOG_GDB("GDB message received: " << str_buffer);

    if ((sum_to_check & 0xff) != checksum) {
      std::stringstream ss;
      ss << "check sum incorrect! " << std::endl
         << "  checksum=" << checksum << std::endl
         << "  sum_to_check=" << sum_to_check << std::endl;
      throw std::logic_error(ss.str());
    }
    DecodeAndCall(str_buffer);
    return str_buffer;
}

// Decodes a message and calls the corresponding function.
// message types: $packet-data#checksum
//                $sequence-id:packet-data#checksum
// from: https://sourceware.org/gdb/onlinedocs/gdb/Overview.html#Overview
void GdbServer::DecodeAndCall(const std::string &msg) {
  std::string msg_stripped = msg.substr(1, msg.length()-4);  // remove $-prefix and checksum suffix
  DBG_LOG_GDB("Stripped message: " << msg_stripped);
  std::vector<std::string> res = SplitMsg(msg_stripped);

  if (!res.empty()) {
    assert(cmd_map.contains(res[0]));  // Command not in map although in regex.
    tcp_server_.SendMsg(kMsgAck);
    cmd_map[res[0]](res);
  } else {
    CmdNotFound(res);
  }
}

// Returns a two digit hexadecimal checksum given a message string.
// For instance "08" or "a5".
std::string GdbServer::GetChecksumStr(const std::string &msg) {
  uint checksum = 0;
  for (const char& c : msg) {
    checksum += static_cast<uint>(c);
  }
  checksum &= 0xff;
  return std::format("{:02x}", checksum);
}

// This function packetifies your message by prepending "$"
// and appending the check sum.
std::string GdbServer::Packetify(std::string msg) {
  std::string checksum = GetChecksumStr(msg);
  msg.insert(0, "$");
  msg.append("#" + checksum);
  return msg;
}

// This functions checks if message complies to the GDB RSP standard.
// It uses a big chonky regex to find any matches.
// The return value is a vector of string that comprises the atomic parts of the message.
std::vector<std::string> GdbServer::SplitMsg(const std::string &msg) {
  static std::regex reg(
    R"(^(\?)|(D)|(g))"
    R"(|(c)([0-9]*))"
    R"(|(G)([0-9A-Fa-f]+))"
    R"(|(M)([0-9A-Fa-f]+),([0-9A-Fa-f]+):([0-9A-Fa-f]+))"
    R"(|(m)([0-9A-Fa-f]+),([0-9A-Fa-f]+))"
    R"(|([zZ])([0-1]),([0-9A-Fa-f]+),([0-9]))"
    R"(|(qAttached)$)"
    R"(|(qSupported):((?:[a-zA-Z-]+\+?;?)+))");
  std::vector<std::string> res;
  std::smatch sm;
  regex_match(msg, sm, reg);
  for (uint i = 1; i < sm.size(); ++i) {
    if (sm[i].str() != "") {
      res.push_back(sm[i].str());
    }
  }
  return res;
}

// Returns true if a breakpoint was reached.
bool GdbServer::BpReached(const u16 address) {
  return bp_set_.contains(address);
}

// Breakpoint reached indicated by sending a SIGTRAP signal.
void GdbServer::SendBpReached() {
  std::string msg_resp = Packetify(std::format("S{:02x}", SIGTRAP));
  DBG_LOG_GDB("sending breakpoint reached");
  tcp_server_.SendMsg(msg_resp.c_str());
}

// "D": for detaching.
void GdbServer::CmdDetach(const std::vector<std::string> &msg_split [[maybe_unused]]) {
  is_attached_ = false;
  cpu_->Continue();
  DBG_LOG_GDB("detaching");
  tcp_server_.SendMsg(kMsgOk);
}

// "qSupported": We only support hardware breakpoints.
void GdbServer::CmdSupported(const std::vector<std::string> &msg_split) {
  std::string msg_resp;
  if (msg_split[1].find("hwbreak+;") != std::string::npos) {
    msg_resp.append("hwbreak+;");
  }
  msg_resp = Packetify(msg_resp);
  DBG_LOG_GDB("sending supported features");
  tcp_server_.SendMsg(msg_resp.c_str());
}

// "qAttached"
void GdbServer::CmdAttached(const std::vector<std::string> &msg_split [[maybe_unused]]) {
  std::string msg_resp = Packetify("1");
  is_attached_ = true;
  DBG_LOG_GDB("replying server is attached to process");
  tcp_server_.SendMsg(msg_resp.c_str());
}

// "?"
void GdbServer::CmdHalted(const std::vector<std::string> &msg_split [[maybe_unused]]) {
  std::string msg_resp = Packetify(std::format("S{:02x}", SIGTRAP));
  cpu_->Halt();
  tcp_server_.SendMsg(msg_resp.c_str());
}

// Command not found.
void GdbServer::CmdNotFound(const std::vector<std::string> &msg_split [[maybe_unused]]) {
  tcp_server_.SendMsg(kMsgEmpty);
}

// "g": Read general registers.
void GdbServer::CmdReadReg(const std::vector<std::string> &msg_split [[maybe_unused]]) {
  std::string msg_resp;
  msg_resp = std::format("{:04x}{:04x}{:04x}{:04x}{:04x}{:04x}{:x>{}}",
                         std::rotl(cpu_->reg_file.AF.val(), 8), std::rotl(cpu_->reg_file.BC.val(), 8),
                         std::rotl(cpu_->reg_file.DE.val(), 8), std::rotl(cpu_->reg_file.HL.val(), 8),
                         std::rotl(cpu_->reg_file.SP.val(), 8), std::rotl(cpu_->reg_file.PC.val(), 8),
                         "", 7*4);
  DBG_LOG_GDB("reading geeneral registers");
  msg_resp = Packetify(msg_resp);
  tcp_server_.SendMsg(msg_resp.c_str());
}

// "G": Write general registers.
// TODO(me): test this function!
void GdbServer::CmdWriteReg(const std::vector<std::string> &msg_split) {
  std::string data_str = msg_split[1];
  assert(data_str.size() >= 24);
  int i = 0;
  for (auto reg : cpu_->reg_file) {
    u16 data = std::stoi(data_str.substr(i, 4), nullptr, 16);
    reg->val(std::rotl(data, 8));
    i += 4;
  }
  DBG_LOG_GDB("writing into registers: " << data_str);
  tcp_server_.SendMsg(kMsgOk);
}

// "m": Read memory.
void GdbServer::CmdReadMem(const std::vector<std::string> &msg_split) {
  std::string msg_resp;
  std::string addr_str = msg_split[1];
  std::string length_str = msg_split[2];
  uint addr = std::stoi(addr_str, nullptr, 16);
  uint length = std::stoi(length_str, nullptr, 16);
  for (uint i = 0; i < length; ++i) {
    u8 data = cpu_->ReadBusDebug(addr + i);
    msg_resp.append(std::format("{:02x}", data));
  }
  DBG_LOG_GDB("reading 0x" << length_str << " bytes at address 0x" << addr_str);
  msg_resp = Packetify(msg_resp);
  tcp_server_.SendMsg(msg_resp.c_str());
}

// "M": Write memory.
void GdbServer::CmdWriteMem(const std::vector<std::string> &msg_split) {
  std::string addr_str = msg_split[1];
  std::string length_str = msg_split[2];
  std::string data_str = msg_split[3];
  uint addr = std::stoi(addr_str, nullptr, 16);
  uint length = std::stoi(length_str, nullptr, 16);
  for (uint i = 0; i < length; ++i) {
    u8 data = std::stoi(data_str.substr(i*2, 2), nullptr, 16);
    cpu_->WriteBusDebug(addr + i, data);
  }
  DBG_LOG_GDB("writing 0x" << length_str << " bytes at address 0x" << addr_str);
  DBG_LOG_GDB("data is:" << data_str);
  tcp_server_.SendMsg(kMsgOk);
}

// "Z": Insert breakpoint.
void GdbServer::CmdInsertBp(const std::vector<std::string> &msg_split) {
  std::string msg_resp = "";
  if (msg_split[1] == "0" || msg_split[1] == "1") {
    msg_resp = "OK";
    uint addr = std::stoi(msg_split[2], nullptr, 16);
    DBG_LOG_GDB("set breakpoint at address 0x" << msg_split[2]);
    bp_set_.insert(addr);
  } else {
    DBG_LOG_GDB("watchpoints aren't supported yet");
  }
  msg_resp = Packetify(msg_resp);
  tcp_server_.SendMsg(msg_resp.c_str());
}

// "z": Remove breakpoint.
void GdbServer::CmdRemoveBp(const std::vector<std::string> &msg_split) {
  std::string msg_resp = "";
  if (msg_split[1] == "0" || msg_split[1] == "1") {
    msg_resp = "OK";
    uint addr = std::stoi(msg_split[2], nullptr, 16);
    DBG_LOG_GDB("removed breakpoint at address 0x" << msg_split[2]);
    bp_set_.erase(addr);
  } else {
    DBG_LOG_GDB("watchpoints aren't supported yet");
  }
  msg_resp = Packetify(msg_resp);
  tcp_server_.SendMsg(msg_resp.c_str());
}

// "c": Continue execution.
void GdbServer::CmdContinue(const std::vector<std::string> &msg_split [[maybe_unused]]) {
  cpu_->Continue();
}
