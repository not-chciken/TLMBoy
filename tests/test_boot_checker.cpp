
/*******************************************************************************
 * Copyright (C) 2020 chciken
 * MIT License
 *
 * Collection of unit test which test the functionality of the bus and
 * the payload factory
 ******************************************************************************/



#include <iostream>
#include <cstdint>
#include <gtest/gtest.h>

#include "../src/boot_checker.h"


TEST(BootCheckerTests, GenericTest) {
  BootChecker test_boot_checker();
  test_boot_checker.do_check();
}

int sc_main(int argc, char* argv[]) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
