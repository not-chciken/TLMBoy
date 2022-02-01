#pragma once
/*******************************************************************************
 * Copyright (C) 2022 chciken/Niko
 * MIT License
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
 private:
    Cpu *cpu_;

 public:
    explicit GdbServer(Cpu *cpu);
    void InitBlocking(const int port);
    bool IsMsgPending();
    void HandleMessages();
    void SendSignal(const uint signal);
    bool BpReached(const u16 address);
    void SendBpReached();

 private:
    bool is_attached_;
    std::map<std::string, std::function<void(std::vector<std::string>)>> cmd_map;
    std::set<u16> bp_set_;
    TcpServer tcp_server_;
    char const *kMsgEmpty = "+$#00";
    char const *kMsgAck = "+";

    std::string RecvMsgBlocking();
    void DecodeAndCall(std::string msg);
    std::vector<std::string> SplitMsg(const std::string msg);
    std::string GetChecksumStr(const std::string msg);
    std::string Packetify(std::string msg);

    // All the commands.
    void CmdHalted(std::vector<std::string> msg_split);
    void CmdDetach(std::vector<std::string> msg_split);
    void CmdNotFound(std::vector<std::string> msg_split);
    void CmdReadReg(std::vector<std::string> msg_split);
    void CmdReadMem(std::vector<std::string> msg_split);
    void CmdWriteMem(std::vector<std::string> msg_split);
    void CmdContinue(std::vector<std::string> msg_split);
    void CmdInsertBp(std::vector<std::string> msg_split);
    void CmdRemoveBp(std::vector<std::string> msg_split);
    void CmdSupported(std::vector<std::string> msg_split);
    void CmdAttached(std::vector<std::string> msg_split);
};
