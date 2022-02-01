/*******************************************************************************
 * Copyright (C) 2022 chciken/Niko
 * MIT License
 *
 * This test connects a requester to a GenericMemory using TLM ports and
 * checks if the sent transaction was correctly received.
 ******************************************************************************/

#include <gtest/gtest.h>
#include <systemc.h>
#include <tlm.h>
#include <tlm_utils/simple_initiator_socket.h>
#include <tlm_utils/simple_target_socket.h>

#include "generic_memory.h"
#include "gb_const.h"
#include "utils.h"

#define SC_INCLUDE_DYNAMIC_PROCESSES

struct MemRequester : public sc_module {
  SC_HAS_PROCESS(MemRequester);
  tlm_utils::simple_initiator_socket<MemRequester, gb_const::kBusDataWidth> init_socket;

  explicit MemRequester(sc_module_name name)
      : sc_module(name), init_socket("init_socket") {
    SC_THREAD(MemRequesterThread);
  }

  void MemRequesterThread() {
    u8 data = 123;
    auto payload = MakeSharedPayloadPtr(tlm::TLM_WRITE_COMMAND,
                                        0x0FFF,
                                        reinterpret_cast<void*>(&data));
    sc_time delay = sc_time(0, SC_NS);

    init_socket->b_transport(*payload, delay);
    ASSERT_EQ(payload->get_response_status(), tlm::TLM_OK_RESPONSE);
    wait(5, SC_NS);
  }
};

struct Top : public sc_module {
  SC_HAS_PROCESS(Top);
  GenericMemory test_memory;
  MemRequester test_memrequester;

  explicit Top(sc_module_name name)
      : sc_module(name),
        test_memory(0x1000, "test_memory"),
        test_memrequester("test_memrequester") {
    SC_THREAD(TopThread);
    test_memrequester.init_socket.bind(test_memory.targ_socket);
  }

  void TopThread() {
    wait(10, SC_NS);
    u8 *dat = test_memory.GetDataPtr();
    ASSERT_EQ(dat[0x0FFF], 123);
  }
};

TEST(MemoryTests, GenericTest) {
  Top test_top("test_top");
  sc_start(30, SC_NS);
}

int sc_main(int argc, char* argv[]) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
