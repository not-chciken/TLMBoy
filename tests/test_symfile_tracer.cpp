/*******************************************************************************
 * Apache License, Version 2.0
 * Copyright (c) 2024 chciken/Niko
 *
 * This tests the SymfileTracer, which is used to identify data sections
 * in ROMs.
 ******************************************************************************/

#include <gtest/gtest.h>

#include <iostream>
#include <sstream>

#include "symfile_tracer.h"

TEST(SymfileTracerTests, SimpleAccess) {
  SymfileTracer sft;
  std::stringstream ss;
  std::string golden_out;

  sft.TraceAccess(0, 0);
  sft.DumpTrace(ss);
  golden_out =
      "00:0000 Data000000\n"
      "00:0000 .data:1\n";
  ASSERT_EQ(ss.str(), golden_out);
  ss.str("");

  sft.TraceAccess(0, 1);
  sft.DumpTrace(ss);
  golden_out =
      "00:0000 Data000000\n"
      "00:0000 .data:2\n";
  ASSERT_EQ(ss.str(), golden_out);
  ss.str("");

  for (int i = 0; i < 10; ++i) {
    sft.TraceAccess(1, 0xf + i);
  }
  sft.DumpTrace(ss);
  golden_out =
      "00:0000 Data000000\n"
      "00:0000 .data:2\n"
      "01:400f Data01400f\n"
      "01:400f .data:a\n";
  ASSERT_EQ(ss.str(), golden_out);
  ss.str("");

  sft.Clear();
  sft.DumpTrace(ss);
  ASSERT_EQ(ss.str(), std::string(""));
}

TEST(SymfileTracerTests, FullAccess) {
  SymfileTracer sft;
  std::stringstream ss;
  std::string golden_out;

  for (int i = 0; i < 0x4000; ++i) {
    sft.TraceAccess(0xe, static_cast<u16>(i));
  }
  for (int i = 0; i < 0x4000; ++i) {
    sft.TraceAccess(0xf, static_cast<u16>(i));
  }
  sft.DumpTrace(ss);
  golden_out =
      "0e:4000 Data0e4000\n"
      "0e:4000 .data:4000\n"
      "0f:4000 Data0f4000\n"
      "0f:4000 .data:4000\n";
  ASSERT_EQ(ss.str(), golden_out);
}

int sc_main(int argc, char* argv[]) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
