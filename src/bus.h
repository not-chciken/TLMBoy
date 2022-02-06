#pragma once
/*******************************************************************************
 * Copyright (C) 2022 chciken
 * MIT License
 *
 * A generic bus model using TLM socket.
 * Use AddBusMaster to add a master socket and AddBusSlave to register slave sockets.
 * Note, that both start and end address are included.
 * Also note, that addresses are forwarded relatively.
 * For example, if there is a target with a range of 0x1000-0x1FFF and
 * the initiator sends a payload to address 0x1500, the target will receive a payload
 * with address 0x0500.
 ******************************************************************************/

#include <iostream>
#include <memory>
#include <vector>

#include "gb_const.h"
#include "systemc.h"
#include "sysc/kernel/sc_simcontext.h"
#include "tlm.h"
#include "tlm_utils/simple_initiator_socket.h"
#include "tlm_utils/simple_target_socket.h"

struct Bus : public sc_module {
  SC_HAS_PROCESS(Bus);
  explicit Bus(sc_module_name name);
  Bus(Bus const&) = delete;
  void operator=(Bus const&) = delete;

  void AddBusMaster(tlm::tlm_initiator_socket<gb_const::kBusDataWidth>* init_sock);
  void AddBusSlave(tlm::tlm_target_socket<gb_const::kBusDataWidth>* targ_sock,
                   const u16 addr_from, const u16 addr_to);

  // SystemC Interfaces
  void b_transport(tlm::tlm_generic_payload& trans, sc_time& delay);
  uint transport_dbg(tlm::tlm_generic_payload& trans);
  bool get_direct_mem_ptr(tlm::tlm_generic_payload& trans, tlm::tlm_dmi& dmi_data);

 private:
  struct BusSlave {
    u16 addr_from;
    u16 addr_to;
    std::shared_ptr<tlm_utils::simple_initiator_socket<Bus, gb_const::kBusDataWidth>> socket;
  };
  std::vector<std::shared_ptr<tlm_utils::simple_target_socket<Bus, gb_const::kBusDataWidth>>> bus_master_vec_;
  std::vector<BusSlave> bus_slave_vec_;
};
