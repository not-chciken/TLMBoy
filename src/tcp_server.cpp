/*******************************************************************************
 * Apache License, Version 2.0
 * Copyright (c) 2025 chciken/Niko
 ******************************************************************************/

#include "tcp_server.h"

#include <errno.h>
#include <sys/ioctl.h>

#include <format>
#include <sstream>
#include <stdexcept>

TcpServer::TcpServer() : socket_fd_(0), client_fd_(0) {
}

TcpServer::~TcpServer() {
  close(socket_fd_);
  close(client_fd_);
}

bool TcpServer::DataAvailable() {
  int count;
  ioctl(client_fd_, FIONREAD, &count);
  return static_cast<bool>(count);
}

void TcpServer::AcceptClient() {
  socklen_t cli_addr_size = sizeof(client_addr_);
  client_fd_ = ::accept(socket_fd_, (struct sockaddr*)&client_addr_, &cli_addr_size);
  if (client_fd_ == -1) {  // Accept failed.
    throw std::runtime_error("accept failed");
  }
}

void TcpServer::Start(int port) {
  socket_fd_ = socket(PF_INET, SOCK_STREAM, 0);  // Only local addresses.
  if (socket_fd_ == -1) {
    throw std::runtime_error("creating socket failed");
  }

  // Set socket for reuse (otherwise might have to wait 4 minutes every time socket is closed).
  int option = 1;
  setsockopt(socket_fd_, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));

  memset(&server_addr_, 0, sizeof(server_addr_));
  server_addr_.sin_family = PF_INET;
  server_addr_.sin_addr.s_addr = htonl(INADDR_ANY);
  server_addr_.sin_port = htons(port);

  int bindSuccess = ::bind(socket_fd_, reinterpret_cast<sockaddr*>(&server_addr_), sizeof(server_addr_));
  if (bindSuccess == -1) {
    throw std::runtime_error("binding socket failed");
  }
  int listenSuccess = ::listen(socket_fd_, 1);
  if (listenSuccess == -1) {
    throw std::runtime_error("listening failed");
  }
}

void TcpServer::SendMsg(const char* msg) {
  int succ = ::send(client_fd_, msg, strlen(msg), 0);
  if (succ == -1) {
    throw std::runtime_error(std::format("SendMsg failed with errno: {}\n", strerror(errno)));
  }
}

string TcpServer::RecvBlocking(uint length) {
  assert(length < kMaxPacketSize);
  char msg[kMaxPacketSize];
  ssize_t num_bytes = recv(client_fd_, msg, length, 0);
  msg[length] = '\0';
  if (num_bytes < 0) {
    throw std::runtime_error(std::format("recv failed with errno: {}\n", strerror(errno)));
  }
  return string(msg);
}