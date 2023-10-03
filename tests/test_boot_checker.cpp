
/*******************************************************************************
 * Apache License, Version 2.0
 * Copyright (c) 2023 chciken/Niko
 *
 * Collection of unit test which test the functionality of the bus and
 * the payload factory
 ******************************************************************************/



#include <iostream>
#include <cstdint>
#include <gtest/gtest.h>

#include "boot_checker.h"

TEST(BootCheckerTests, GenericTest) {
  BootChecker test_boot_checker();
  test_boot_checker.do_check();
}

int sc_main(int argc, char* argv[]) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
