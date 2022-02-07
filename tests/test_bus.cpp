
/*******************************************************************************
 * Copyright (C) 2022 chciken/Niko
 * MIT License
 *
 * Collection of unit test which test the functionality of the bus and
 * the payload factory
 ******************************************************************************/

#include <gtest/gtest.h>
#include <systemc.h>
#include <tlm.h>
#include <tlm_utils/simple_initiator_socket.h>
#include <tlm_utils/simple_target_socket.h>

#include <cstdint>
#include <iostream>

#include "bus.h"
#include "gb_const.h"
#include "utils.h"

struct BusMaster : public sc_module {
  SC_HAS_PROCESS(BusMaster);
  tlm_utils::simple_initiator_socket<BusMaster, gb_const::kBusDataWidth> init_socket;

  explicit BusMaster(sc_module_name name) : sc_module(name), init_socket("init_socket") {
    SC_THREAD(BusMasterThread);
  }

  void BusMasterThread() {
    int data = 1337;
    auto payload = MakeSharedPayloadPtr(tlm::TLM_READ_COMMAND,
                                        0x0FFF,
                                        reinterpret_cast<void*>(&data));
    sc_time delay = sc_time(0, SC_NS);

    init_socket->b_transport(*payload, delay);
    ASSERT_EQ(payload->get_response_status(), tlm::TLM_OK_RESPONSE);
    ASSERT_EQ(data, 69) << "data is " << data;
    wait(5, SC_NS);

    payload->set_response_status(tlm::TLM_INCOMPLETE_RESPONSE);
    init_socket->b_transport(*payload, delay);
    ASSERT_EQ(payload->get_response_status(), tlm::TLM_OK_RESPONSE);
    ASSERT_EQ(data, 69) << "data is " << data;
    wait(5, SC_NS);

    payload->set_response_status(tlm::TLM_INCOMPLETE_RESPONSE);
    payload->set_address(0x1FFF);  // this should arrive at test_slave_1
    init_socket->b_transport(*payload, delay);
    ASSERT_EQ(payload->get_response_status(), tlm::TLM_OK_RESPONSE);

    payload->set_response_status(tlm::TLM_INCOMPLETE_RESPONSE);
    payload->set_address(0x3000);
    init_socket->b_transport(*payload, delay);
    ASSERT_EQ(payload->get_response_status(), tlm::TLM_ADDRESS_ERROR_RESPONSE);

    tlm::tlm_dmi dmi_data;
    u8 *data_ptr;
    payload->set_response_status(tlm::TLM_INCOMPLETE_RESPONSE);
    payload->set_address(0x6000);
    ASSERT_FALSE(init_socket->get_direct_mem_ptr(*payload, dmi_data));
    payload->set_address(0x1600);
    ASSERT_TRUE(init_socket->get_direct_mem_ptr(*payload, dmi_data));
    ASSERT_EQ(dmi_data.get_start_address(), static_cast<sc_dt::uint64>(0));
    ASSERT_EQ(dmi_data.get_end_address(), static_cast<sc_dt::uint64>(0xFFF));
    data_ptr = reinterpret_cast<u8*>(dmi_data.get_dmi_ptr());
    for (uint i = 0; i < 0x1000; i++) {
      data_ptr[i] = 23;
    }

    payload->set_address(0x0);
    ASSERT_TRUE(init_socket->get_direct_mem_ptr(*payload, dmi_data));
    ASSERT_EQ(dmi_data.get_start_address(), static_cast<sc_dt::uint64>(0x0000));
    ASSERT_EQ(dmi_data.get_end_address(), static_cast<sc_dt::uint64>(0xFFF));
    data_ptr = reinterpret_cast<u8*>(dmi_data.get_dmi_ptr());
    for (uint i = 0; i < 0x1000; i++) {
      data_ptr[i] = 23;
    }
  }
};

struct BusSlave : public sc_module {
  SC_HAS_PROCESS(BusSlave);
  tlm_utils::simple_target_socket<BusSlave, gb_const::kBusDataWidth> target_socket;
  u8 data[0x1000];

  explicit BusSlave(sc_module_name name): sc_module(name), target_socket("target_socket") {
    SC_THREAD(BusSlaveThread);
    target_socket.register_b_transport(this, &BusSlave::b_transport);
    target_socket.register_get_direct_mem_ptr(this, &BusSlave::get_direct_mem_ptr);
  }

  void BusSlaveThread() {
    wait(15, SC_NS);
    ASSERT_EQ(data[42], 23);
    ASSERT_EQ(data[0x0FFF], 23);
  }

  bool get_direct_mem_ptr(tlm::tlm_generic_payload& trans, tlm::tlm_dmi& dmi_data) {
    if (trans.get_address() < 0x1000) {
      dmi_data.allow_read_write();
      dmi_data.set_start_address(0);
      dmi_data.set_end_address(0x1000 - 1);
      dmi_data.set_dmi_ptr(reinterpret_cast<unsigned char*>(data));
      return true;
    }
    return false;
  }

  void b_transport(tlm::tlm_generic_payload& trans, sc_time& delay) {
    tlm::tlm_command cmd = trans.get_command();
    u16 addr = static_cast<u16>(trans.get_address());
    int* ptr = reinterpret_cast<int*>(trans.get_data_ptr());
    trans.set_response_status(tlm::TLM_OK_RESPONSE);
    *ptr = 69;

    ASSERT_EQ(addr, 0x0FFF);
    ASSERT_EQ(cmd, tlm::TLM_READ_COMMAND);
  }
};


struct Top : public sc_module {
  SC_HAS_PROCESS(Top);
  Bus test_bus;
  BusMaster test_master;
  BusSlave test_slave_0;
  BusSlave test_slave_1;

  explicit Top(sc_module_name name)
      : sc_module(name),
        test_bus("test_bus"),
        test_master("test_master"),
        test_slave_0("test_slave_0"),
        test_slave_1("test_slave_1") {
    test_bus.AddBusMaster(&test_master.init_socket);
    test_bus.AddBusSlave(&test_slave_0.target_socket, 0x0000, 0x0FFF);
    test_bus.AddBusSlave(&test_slave_1.target_socket, 0x1000, 0x1FFF);
  }
};

TEST(BusTests, GenericTest) {
  Top test_top("test_top");
  sc_start(30, SC_NS);
}

int sc_main(int argc, char* argv[]) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
