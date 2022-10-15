/*******************************************************************************
 * Apache License, Version 2.0
 * Copyright (c) 2022 chciken/Niko
******************************************************************************/
#include <string>

#include "bus.h"

Bus::Bus(sc_module_name name) : sc_module(name) {
}

void Bus::AddBusMaster(tlm::tlm_initiator_socket<gb_const::kBusDataWidth>* init_sock) {
  std::string name = std::string(init_sock->basename()) + "_bus_master_socket_"
                                 + std::to_string(bus_master_vec_.size());
  auto targ_sock = std::make_shared<tlm_utils::simple_target_socket<Bus, gb_const::kBusDataWidth>>(name.c_str());
  targ_sock->register_b_transport(this, &Bus::b_transport);
  targ_sock->register_transport_dbg(this, &Bus::transport_dbg);
  targ_sock->register_get_direct_mem_ptr(this, &Bus::get_direct_mem_ptr);
  init_sock->bind(*targ_sock);
  bus_master_vec_.push_back(targ_sock);
}

void Bus::AddBusSlave(tlm::tlm_target_socket<gb_const::kBusDataWidth>* targ_sock,
                      const u16 addr_from, const u16 addr_to) {
  BusSlave slave;
  slave.addr_from = addr_from;
  slave.addr_to = addr_to;
  std::string name = std::string(targ_sock->basename()) + "_bus_slave_socket_"
                                 + std::to_string(bus_slave_vec_.size());
  auto init_sock = std::make_shared<tlm_utils::simple_initiator_socket<Bus, gb_const::kBusDataWidth>>(name.c_str());
  slave.socket = init_sock;
  init_sock->bind(*targ_sock);
  bus_slave_vec_.push_back(slave);
}

void Bus::b_transport(tlm::tlm_generic_payload& trans, sc_time& delay) {
  const u16 adr = static_cast<u16>(trans.get_address());
  for (auto slave : bus_slave_vec_) {
    if ((slave.addr_from <= adr) && (adr <= slave.addr_to)) {
      auto socket = slave.socket;
      trans.set_address(adr - slave.addr_from);
      (*socket)->b_transport(trans, delay);
      return;
    }
  }
  trans.set_response_status(tlm::TLM_ADDRESS_ERROR_RESPONSE);
}

uint Bus::transport_dbg(tlm::tlm_generic_payload& trans) {
  const u16 adr = static_cast<u16>(trans.get_address());
  for (auto slave : bus_slave_vec_) {
    if ((slave.addr_from <= adr) && (adr <= slave.addr_to)) {
      auto socket = slave.socket;
      trans.set_address(adr - slave.addr_from);
      return (*socket)->transport_dbg(trans);
    }
  }
  trans.set_response_status(tlm::TLM_ADDRESS_ERROR_RESPONSE);
  return 0;
}

bool Bus::get_direct_mem_ptr(tlm::tlm_generic_payload& trans, tlm::tlm_dmi& dmi_data) {
  const u16 adr = static_cast<u16>(trans.get_address());
  for (auto slave : bus_slave_vec_) {
    if (slave.addr_from <= adr && adr <= slave.addr_to) {
      trans.set_address(adr - slave.addr_from);
      return (*slave.socket)->get_direct_mem_ptr(trans, dmi_data);
    }
  }
  return false;
}
