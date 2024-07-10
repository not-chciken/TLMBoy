######################################################
# Apache License, Version 2.0
# Copyright (c) 2022 chciken/Niko
######################################################

# Unit tests.
add_custom_target(tests)

# Blarrg's individual tests.
add_executable(test_blarrg_cpuinstr01 test_blarrg_cpuinstr01.cpp)
add_executable(test_blarrg_cpuinstr02 test_blarrg_cpuinstr02.cpp)
add_executable(test_blarrg_cpuinstr03 test_blarrg_cpuinstr03.cpp)
add_executable(test_blarrg_cpuinstr04 test_blarrg_cpuinstr04.cpp)
add_executable(test_blarrg_cpuinstr05 test_blarrg_cpuinstr05.cpp)
add_executable(test_blarrg_cpuinstr06 test_blarrg_cpuinstr06.cpp)
add_executable(test_blarrg_cpuinstr07 test_blarrg_cpuinstr07.cpp)
add_executable(test_blarrg_cpuinstr08 test_blarrg_cpuinstr08.cpp)
add_executable(test_blarrg_cpuinstr09 test_blarrg_cpuinstr09.cpp)
add_executable(test_blarrg_cpuinstr10 test_blarrg_cpuinstr10.cpp)
add_executable(test_blarrg_cpuinstr11 test_blarrg_cpuinstr11.cpp)

# Some unit tests and system tests.
add_executable(test_boot test_boot.cpp)
add_executable(test_boot_states test_boot_states.cpp)
add_executable(test_bus test_bus.cpp)
add_executable(test_cpu test_cpu.cpp)
add_executable(test_dmg_acid2 test_dmg_acid2.cpp)
add_executable(test_gdb test_gdb.cpp)
add_executable(test_memory test_memory.cpp)
add_executable(test_ppu test_ppu.cpp)
add_executable(test_symfile_tracer test_symfile_tracer.cpp)

set(TEST_INCLUDE_PATHS ${SYSTEMC_PATH}/include
                       src/)
set(TEST_LIB_PATHS -L${SYSTEMC_PATH}/lib-linux64)
set(TEST_LIBS -lsystemc -lgtest -lpthread -lSDL2 -lgcov tlmboy_static_test)
set(TEST_COMPILE_OPTS --coverage -g -O0)

macro(create_test_case test_name)
  add_dependencies(tests ${test_name})
  target_include_directories(${test_name} PUBLIC ${TEST_INCLUDE_PATHS})
  target_link_libraries(${test_name} ${TEST_LIB_PATHS})
  target_compile_options(${test_name} PUBLIC ${TEST_COMPILE_OPTS})
  target_link_libraries(${test_name} ${TEST_LIBS})
  add_test(NAME ${test_name} COMMAND ${test_name})
  set_tests_properties(${test_name} PROPERTIES TIMEOUT 300)
endmacro()

create_test_case(test_blarrg_cpuinstr01)
create_test_case(test_blarrg_cpuinstr02)
create_test_case(test_blarrg_cpuinstr03)
create_test_case(test_blarrg_cpuinstr04)
create_test_case(test_blarrg_cpuinstr05)
create_test_case(test_blarrg_cpuinstr06)
create_test_case(test_blarrg_cpuinstr07)
create_test_case(test_blarrg_cpuinstr08)
create_test_case(test_blarrg_cpuinstr09)
create_test_case(test_blarrg_cpuinstr10)
create_test_case(test_blarrg_cpuinstr11)
create_test_case(test_boot)
create_test_case(test_boot_states)
create_test_case(test_bus)
create_test_case(test_cpu)
create_test_case(test_dmg_acid2)
create_test_case(test_gdb)
create_test_case(test_memory)
create_test_case(test_ppu)
create_test_case(test_symfile_tracer)

target_compile_options(test_boot_states PUBLIC ${TEST_COMPILE_OPTS} -DENABLE_DBG_LOG_CPU_REG)

# Let them run sequentially. Might algo work in parallel, but better be safe.
set_tests_properties(
  test_cpu
  test_bus
  test_dmg_acid2
  test_gdb
  test_memory
  test_ppu
  test_boot
  test_boot_states
  test_blarrg_cpuinstr01
  test_blarrg_cpuinstr02
  test_blarrg_cpuinstr03
  test_blarrg_cpuinstr04
  test_blarrg_cpuinstr05
  test_blarrg_cpuinstr06
  test_blarrg_cpuinstr07
  test_blarrg_cpuinstr08
  test_blarrg_cpuinstr09
  test_blarrg_cpuinstr10
  test_blarrg_cpuinstr11
  PROPERTIES RUN_SERIAL TRUE
  ENVIRONMENT TLMBOY_ROOT=${CMAKE_SOURCE_DIR})