#pragma once
/*******************************************************************************
 * Copyright (C) 2022 chciken/Niko
 * MIT License
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
#include <string>

#include "debug.h"

class TcpServer {
 public:
    TcpServer();
    ~TcpServer();
    void Start(const int port);
    void AcceptClient();
    std::string RecvBlocking(uint length);
    void SendMsg(const char *msg);
    bool DataAvailable();

 private:
    const uint kMaxPacketSize = 4096;
    int socket_fd_;
    int client_fd_;
    sockaddr_in server_addr_;
    sockaddr_in client_addr_;
};
