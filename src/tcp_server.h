#pragma once
/*******************************************************************************
 * Apache License, Version 2.0
 * Copyright (c) 2023 chciken/Niko
 *
 * Implementation of a TCP server.
 * Currently only used for the Remote GDB protocol.
 * Mostly inspired by https://github.com/elhayra/tcp_server_client
 * and a little bit of gem5.
 ******************************************************************************/

#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <cassert>
#include <cstring>
#include <functional>
#include <iostream>
#include <thread>

#include "common.h"
#include "debug.h"

class TcpServer {
 public:
  TcpServer();
  ~TcpServer();

  bool DataAvailable();
  void AcceptClient();
  void Start(int port);
  void SendMsg(const char* msg);
  string RecvBlocking(uint length);

 private:
  const uint kMaxPacketSize = 4096;
  int socket_fd_;
  int client_fd_;
  sockaddr_in server_addr_;
  sockaddr_in client_addr_;
};
