/*******************************************************************************
 * Copyright (C) 2022 chciken/Niko
 * MIT License
 ******************************************************************************/

#include "gdb_server.h"

#include <bit>
#include <byteswap.h>
#include <functional>
#include <iomanip>
#include <regex>
#include <sstream>
#include <stdexcept>

#include "cpu.h"
#include "fmt/format.h"

GdbServer::GdbServer(Cpu *cpu) : cpu_(cpu), is_attached_(false) {
  cmd_map["?"] = std::bind(&GdbServer::CmdHalted, this, std::placeholders::_1);
  cmd_map["g"] = std::bind(&GdbServer::CmdReadReg, this, std::placeholders::_1);
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

// Waits for a packet from the gdb client
// Note, this function is blocking
std::string GdbServer::RecvMsgBlocking() {
    std::string str_buffer("");  // will contain the whole message, e.g: "$vMustReplyEmpty#3a"
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
    if (str_buffer.at(0) != '$') {
      DBG_LOG_GDB(str_buffer);
      throw std::logic_error("message not beginning with '$'");
    }
    do {
      resp = tcp_server_.RecvBlocking(1);
      str_buffer.append(resp);
      sum_to_check += static_cast<int>(resp.at(0));
      if (str_buffer.length() > 4096) {
        throw std::logic_error("message is too long");
      }
    } while (resp.at(0) != '#');
    sum_to_check -= '#';
    std::string checksum_str = tcp_server_.RecvBlocking(2);
    uint checksum = 0;
    checksum = std::stoi(checksum_str, 0, 16);
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

// decodes a message and calls the corresponding function
// message types: $packet-data#checksum
//                $sequence-id:packet-data#checksum
// from: https://sourceware.org/gdb/onlinedocs/gdb/Overview.html#Overview
void GdbServer::DecodeAndCall(std::string msg) {
  std::string msg_stripped = msg.substr(1, msg.length()-4);  // remove $-prefix and checksum suffix
  DBG_LOG_GDB("Stripped message: " << msg_stripped);
  std::vector<std::string> res = SplitMsg(msg_stripped);

  if (res.size() > 0) {
    if (cmd_map.contains(res[0])) {
        tcp_server_.SendMsg(kMsgAck);
        cmd_map[res[0]](res);
    } else {
        throw std::runtime_error("command not in map although in regex");
    }
  } else {
    CmdNotFound(res);
  }
}

// Returns a two digit hexadecimal checksum given a message string.
// For instance "08" or "a5".
std::string GdbServer::GetChecksumStr(const std::string msg) {
  uint checksum = 0;
  for (const char& c : msg) {
    checksum += static_cast<uint>(c);
  }
  checksum &= 0xff;
  return fmt::format("{:02x}", checksum);
}

// This function packetifies your message by prepending "$"
// and appending the check sum
std::string GdbServer::Packetify(std::string msg) {
  std::string checksum = GetChecksumStr(msg);
  msg.insert(0, "$");
  msg.append("#" + checksum);
  return msg;
}

std::vector<std::string> GdbServer::SplitMsg(const std::string msg) {
  std::vector<std::string> res;
  std::smatch sm;
  std::regex reg(
      R"(^(\?)|(D)|(g)|(c)([0-9]*)|(m)([0-9A-Fa-f]+),([0-9A-Fa-f]+))"
      R"(|(qSupported):((?:[a-zA-Z-]+\+?;?)+))"
      R"(|(M)([0-9A-Fa-f]+),([0-9A-Fa-f]+):([0-9A-Fa-f]+))"
      R"(|([zZ])([0-3]),([0-9A-Fa-f]+),([0-9]))"
      R"(|(qAttached)$)");
  regex_match(msg, sm, reg);
  for (uint i = 1; i < sm.size(); i++) {
    if (sm[i].str() != "") {
      res.push_back(sm[i].str());
    }
  }
  return res;
}

// returns true if a breakpoint was reached
bool GdbServer::BpReached(const u16 address) {
  return bp_set_.contains(address);
}

// send a POSIX signal to gdb
void GdbServer::SendSignal(const uint signal) {
  std::string msg_resp = fmt::format("S{:02x}", signal);
  std::string checksum = GetChecksumStr(msg_resp);
  msg_resp.insert(0, "$");
  msg_resp.append("#" + checksum);
  DBG_LOG_GDB("sending signal " << signal);
  tcp_server_.SendMsg(msg_resp.c_str());
}

// breakpoint reached indicated by sending a SIGTRAP signal
void GdbServer::SendBpReached() {
  std::string msg_resp = Packetify(fmt::format("S{:02x}", SIGTRAP));
  DBG_LOG_GDB("sending breakpoint reached");
  tcp_server_.SendMsg(msg_resp.c_str());
}

// "D"
void GdbServer::CmdDetach(std::vector<std::string> msg_split) {
  std::string msg_resp = "OK";
  is_attached_ = false;
  cpu_->Continue();
  msg_resp = Packetify(msg_resp);
  DBG_LOG_GDB("detaching");
  tcp_server_.SendMsg(msg_resp.c_str());
}

// "qSupported"
void GdbServer::CmdSupported(std::vector<std::string> msg_split) {
  std::string msg_resp;
  if (msg_split[1].find("hwbreak+;") != std::string::npos) {
    msg_resp.append("hwbreak+;");
  }
  msg_resp = Packetify(msg_resp);
  DBG_LOG_GDB("sending supported features");
  tcp_server_.SendMsg(msg_resp.c_str());
}

// "qAttached"
void GdbServer::CmdAttached(std::vector<std::string> msg_split) {
  std::string msg_resp = Packetify("1");
  is_attached_ = true;
  DBG_LOG_GDB("sending sever is attached to process");
  tcp_server_.SendMsg(msg_resp.c_str());
}

// "?"
void GdbServer::CmdHalted(std::vector<std::string> msg_split) {
  std::string msg_resp = Packetify(fmt::format("S{:02x}", SIGTRAP));
  cpu_->Halt();
  tcp_server_.SendMsg(msg_resp.c_str());
}

// Command not found
void GdbServer::CmdNotFound(std::vector<std::string> msg_split) {
  tcp_server_.SendMsg(kMsgEmpty);
}

// "g": Command read general registers
void GdbServer::CmdReadReg(std::vector<std::string> msg_split) {
  std::string msg_resp;
  msg_resp = fmt::format("{:04x}{:04x}{:04x}{:04x}{:04x}{:04x}{:x>{}}",
                         std::rotl(cpu_->reg_file.AF.val(), 8), std::rotl(cpu_->reg_file.BC.val(), 8),
                         std::rotl(cpu_->reg_file.DE.val(), 8), std::rotl(cpu_->reg_file.HL.val(), 8),
                         std::rotl(cpu_->reg_file.SP.val(), 8), std::rotl(cpu_->reg_file.PC.val(), 8),
                         "", 7*4);
  DBG_LOG_GDB("reading geeneral registers");
  msg_resp = Packetify(msg_resp);
  tcp_server_.SendMsg(msg_resp.c_str());
}

// "m": read memory
void GdbServer::CmdReadMem(std::vector<std::string> msg_split) {
  std::string msg_resp;
  std::string addr_str = msg_split[1];
  std::string length_str = msg_split[2];
  uint addr = std::stoi(addr_str, nullptr, 16);
  uint length = std::stoi(length_str, nullptr, 16);
  for (uint i = 0; i < length; i++) {
    u8 data = cpu_->ReadBusDebug(addr + i);
    msg_resp.append(fmt::format("{:02x}", data));
  }
  DBG_LOG_GDB("reading 0x" << length_str << " bytes at address 0x" << addr_str);
  msg_resp = Packetify(msg_resp);
  tcp_server_.SendMsg(msg_resp.c_str());
}

// "M": write memory
void GdbServer::CmdWriteMem(std::vector<std::string> msg_split) {
  std::string msg_resp = "OK";
  std::string addr_str = msg_split[1];
  std::string length_str = msg_split[2];
  std::string data_str = msg_split[3];
  uint addr = std::stoi(addr_str, nullptr, 16);
  uint length = std::stoi(length_str, nullptr, 16);
  for (uint i=0; i < length; i++) {
    u8 data = std::stoi(data_str.substr(i, 2), nullptr, 16);
    cpu_->WriteBusDebug(addr + i, data);
  }
  DBG_LOG_GDB("writing 0x" << length_str << " bytes at address 0x" << addr_str);
  DBG_LOG_GDB("data is:" << data_str);
  msg_resp = Packetify(msg_resp);
  tcp_server_.SendMsg(msg_resp.c_str());
}

// "Z": insert breakpoint
void GdbServer::CmdInsertBp(std::vector<std::string> msg_split) {
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

// "z": remove breakpoint
void GdbServer::CmdRemoveBp(std::vector<std::string> msg_split) {
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

// "c": continue execution
void GdbServer::CmdContinue(std::vector<std::string> msg_split) {
  cpu_->Continue();
}
