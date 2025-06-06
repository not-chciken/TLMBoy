#pragma once
/*******************************************************************************
 * Apache License, Version 2.0
 * Copyright (c) 2025 chciken/Niko
 *
 * A GDB server for the Game Boy.
 * Allows you to attach with GDB via `target remote localhost:<port>`
 ******************************************************************************/
#include <map>
#include <set>
#include <string>
#include <vector>

#include "common.h"
#include "tcp_server.h"

class Cpu;

class GdbServer {
 public:
  explicit GdbServer(Cpu* cpu);

  bool BpReached(const u16 address);
  bool IsMsgPending();
  void HandleMessages();
  void InitBlocking(const int port);
  void SendBpReached();
  void SendSignal(const uint signal);

 private:
  bool is_attached_;
  Cpu* cpu_;
  std::map<string, std::function<void(const std::vector<string>)>> cmd_map;
  std::set<u16> bp_set_;
  TcpServer tcp_server_;

  char const* kMsgAck = "+";
  char const* kMsgEmpty = "+$#00";
  char const* kMsgOk = "+$OK#9a";

  static constexpr uint kMaxMessageLength = 4096;

  string RecvMsgBlocking();
  void DecodeAndCall(const string& msg);
  std::vector<string> SplitMsg(const string& msg);
  string GetChecksumStr(const string& msg);
  string Packetify(string msg);

  // All the GDBRSP commands.
  void CmdHalted(const std::vector<string>& msg_split);
  void CmdDetach(const std::vector<string>& msg_split);
  void CmdNotFound(const std::vector<string>& msg_split);
  void CmdReadReg(const std::vector<string>& msg_split);
  void CmdWriteReg(const std::vector<string>& msg_split);
  void CmdReadMem(const std::vector<string>& msg_split);
  void CmdWriteMem(const std::vector<string>& msg_split);
  void CmdContinue(const std::vector<string>& msg_split);
  void CmdInsertBp(const std::vector<string>& msg_split);
  void CmdRemoveBp(const std::vector<string>& msg_split);
  void CmdSupported(const std::vector<string>& msg_split);
  void CmdAttached(const std::vector<string>& msg_split);
};
