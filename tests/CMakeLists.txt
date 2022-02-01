# Unit tests.
add_custom_target(tests)

# Blarrg's individual tests.
add_executable(test_blarrg_cpuinstr01 test_blarrg_cpuinstr01.cpp ${ALL_CPPS})
add_executable(test_blarrg_cpuinstr02 test_blarrg_cpuinstr02.cpp ${ALL_CPPS})
add_executable(test_blarrg_cpuinstr03 test_blarrg_cpuinstr03.cpp ${ALL_CPPS})
add_executable(test_blarrg_cpuinstr04 test_blarrg_cpuinstr04.cpp ${ALL_CPPS})
add_executable(test_blarrg_cpuinstr05 test_blarrg_cpuinstr05.cpp ${ALL_CPPS})
add_executable(test_blarrg_cpuinstr06 test_blarrg_cpuinstr06.cpp ${ALL_CPPS})
add_executable(test_blarrg_cpuinstr07 test_blarrg_cpuinstr07.cpp ${ALL_CPPS})
add_executable(test_blarrg_cpuinstr08 test_blarrg_cpuinstr08.cpp ${ALL_CPPS})
add_executable(test_blarrg_cpuinstr09 test_blarrg_cpuinstr09.cpp ${ALL_CPPS})
add_executable(test_blarrg_cpuinstr10 test_blarrg_cpuinstr10.cpp ${ALL_CPPS})
add_executable(test_blarrg_cpuinstr11 test_blarrg_cpuinstr11.cpp ${ALL_CPPS})

# Some unit tests and system tests.
add_executable(test_boot test_boot.cpp ${ALL_CPPS})
add_executable(test_boot_states test_boot_states.cpp ${ALL_CPPS})
add_executable(test_bus test_bus.cpp ${ALL_CPPS})
add_executable(test_cpu test_cpu.cpp ${ALL_CPPS})
add_executable(test_memory test_memory.cpp
               ${CMAKE_SOURCE_DIR}/src/generic_memory.cpp
               ${CMAKE_SOURCE_DIR}/src/utils.cpp)
add_executable(test_ppu test_ppu.cpp
               ${CMAKE_SOURCE_DIR}/src/utils.cpp
               ${CMAKE_SOURCE_DIR}/src/ppu.cpp)


add_dependencies(tests test_blarrg_cpuinstr01)
add_dependencies(tests test_blarrg_cpuinstr02)
add_dependencies(tests test_blarrg_cpuinstr03)
add_dependencies(tests test_blarrg_cpuinstr04)
add_dependencies(tests test_blarrg_cpuinstr05)
add_dependencies(tests test_blarrg_cpuinstr06)
add_dependencies(tests test_blarrg_cpuinstr07)
add_dependencies(tests test_blarrg_cpuinstr08)
add_dependencies(tests test_blarrg_cpuinstr09)
add_dependencies(tests test_blarrg_cpuinstr10)
add_dependencies(tests test_blarrg_cpuinstr11)
add_dependencies(tests test_boot)
add_dependencies(tests test_boot_states)
add_dependencies(tests test_bus)
add_dependencies(tests test_cpu)
add_dependencies(tests test_memory)
add_dependencies(tests test_ppu)


set(TEST_INCLUDE_PATHS /usr/local/systemc-2.3.3-debug/include
                       /usr/local/googletest/googletest/include
                       src/)
set(TEST_LIB_PATHS -L/usr/local/systemc-2.3.3-debug/lib-linux64
                   -L/usr/local/googletest/lib)
set(TEST_LIBS -lsystemc -lgtest -lpthread -lfmt -lSDL2 -lgcov)
set(TEST_COMPILE_OPTS --coverage -g -O0)


target_include_directories(test_blarrg_cpuinstr01 PUBLIC ${TEST_INCLUDE_PATHS})
target_link_libraries(test_blarrg_cpuinstr01 ${TEST_LIB_PATHS})
target_compile_options(test_blarrg_cpuinstr01 PUBLIC ${TEST_COMPILE_OPTS})
target_link_libraries(test_blarrg_cpuinstr01 ${TEST_LIBS})
add_test(NAME test_blarrg_cpuinstr01 COMMAND test_blarrg_cpuinstr01)

target_include_directories(test_blarrg_cpuinstr02 PUBLIC ${TEST_INCLUDE_PATHS})
target_link_libraries(test_blarrg_cpuinstr02 ${TEST_LIB_PATHS})
target_compile_options(test_blarrg_cpuinstr02 PUBLIC ${TEST_COMPILE_OPTS})
target_link_libraries(test_blarrg_cpuinstr02 ${TEST_LIBS})
add_test(NAME test_blarrg_cpuinstr02 COMMAND test_blarrg_cpuinstr02)

target_include_directories(test_blarrg_cpuinstr03 PUBLIC ${TEST_INCLUDE_PATHS})
target_link_libraries(test_blarrg_cpuinstr03 ${TEST_LIB_PATHS})
target_link_libraries(test_blarrg_cpuinstr03 ${TEST_LIBS})
target_compile_options(test_blarrg_cpuinstr03 PUBLIC ${TEST_COMPILE_OPTS})
add_test(NAME test_blarrg_cpuinstr03 COMMAND test_blarrg_cpuinstr03)

target_include_directories(test_blarrg_cpuinstr04 PUBLIC ${TEST_INCLUDE_PATHS})
target_link_libraries(test_blarrg_cpuinstr04 ${TEST_LIB_PATHS})
target_link_libraries(test_blarrg_cpuinstr04 ${TEST_LIBS})
target_compile_options(test_blarrg_cpuinstr04 PUBLIC ${TEST_COMPILE_OPTS})
add_test(NAME test_blarrg_cpuinstr04 COMMAND test_blarrg_cpuinstr04)

target_include_directories(test_blarrg_cpuinstr05 PUBLIC ${TEST_INCLUDE_PATHS})
target_link_libraries(test_blarrg_cpuinstr05 ${TEST_LIB_PATHS})
target_link_libraries(test_blarrg_cpuinstr05 ${TEST_LIBS})
target_compile_options(test_blarrg_cpuinstr05 PUBLIC ${TEST_COMPILE_OPTS})
add_test(NAME test_blarrg_cpuinstr05 COMMAND test_blarrg_cpuinstr05)

target_include_directories(test_blarrg_cpuinstr06 PUBLIC ${TEST_INCLUDE_PATHS})
target_link_libraries(test_blarrg_cpuinstr06 ${TEST_LIB_PATHS})
target_link_libraries(test_blarrg_cpuinstr06 ${TEST_LIBS})
target_compile_options(test_blarrg_cpuinstr06 PUBLIC ${TEST_COMPILE_OPTS})
add_test(NAME test_blarrg_cpuinstr06 COMMAND test_blarrg_cpuinstr06)

target_include_directories(test_blarrg_cpuinstr07 PUBLIC ${TEST_INCLUDE_PATHS})
target_link_libraries(test_blarrg_cpuinstr07 ${TEST_LIB_PATHS})
target_link_libraries(test_blarrg_cpuinstr07 ${TEST_LIBS})
target_compile_options(test_blarrg_cpuinstr07 PUBLIC ${TEST_COMPILE_OPTS})
add_test(NAME test_blarrg_cpuinstr07 COMMAND test_blarrg_cpuinstr07)

target_include_directories(test_blarrg_cpuinstr08 PUBLIC ${TEST_INCLUDE_PATHS})
target_link_libraries(test_blarrg_cpuinstr08 ${TEST_LIB_PATHS})
target_link_libraries(test_blarrg_cpuinstr08 ${TEST_LIBS})
target_compile_options(test_blarrg_cpuinstr08 PUBLIC ${TEST_COMPILE_OPTS})
add_test(NAME test_blarrg_cpuinstr08 COMMAND test_blarrg_cpuinstr08)

target_include_directories(test_blarrg_cpuinstr09 PUBLIC ${TEST_INCLUDE_PATHS})
target_link_libraries(test_blarrg_cpuinstr09 ${TEST_LIB_PATHS})
target_link_libraries(test_blarrg_cpuinstr09 ${TEST_LIBS})
target_compile_options(test_blarrg_cpuinstr09 PUBLIC ${TEST_COMPILE_OPTS})
add_test(NAME test_blarrg_cpuinstr09 COMMAND test_blarrg_cpuinstr09)

target_include_directories(test_blarrg_cpuinstr10 PUBLIC ${TEST_INCLUDE_PATHS})
target_link_libraries(test_blarrg_cpuinstr10 ${TEST_LIB_PATHS})
target_link_libraries(test_blarrg_cpuinstr10 ${TEST_LIBS})
target_compile_options(test_blarrg_cpuinstr10 PUBLIC ${TEST_COMPILE_OPTS})
add_test(NAME test_blarrg_cpuinstr10 COMMAND test_blarrg_cpuinstr10)

target_include_directories(test_blarrg_cpuinstr11 PUBLIC ${TEST_INCLUDE_PATHS})
target_link_libraries(test_blarrg_cpuinstr11 ${TEST_LIB_PATHS})
target_link_libraries(test_blarrg_cpuinstr11 ${TEST_LIBS})
target_compile_options(test_blarrg_cpuinstr11 PUBLIC ${TEST_COMPILE_OPTS})
add_test(NAME test_blarrg_cpuinstr11 COMMAND test_blarrg_cpuinstr11)

target_include_directories(test_boot PUBLIC ${TEST_INCLUDE_PATHS})
target_link_libraries(test_boot ${TEST_LIB_PATHS})
target_link_libraries(test_boot ${TEST_LIBS})
target_compile_options(test_boot PUBLIC ${TEST_COMPILE_OPTS})
add_test(NAME test_boot COMMAND test_boot)

target_include_directories(test_boot_states PUBLIC ${TEST_INCLUDE_PATHS})
target_link_libraries(test_boot_states ${TEST_LIB_PATHS})
target_link_libraries(test_boot_states ${TEST_LIBS})
target_compile_options(test_boot_states PUBLIC ${TEST_COMPILE_OPTS} -DENABLE_DBG_LOG_CPU_REG)
add_test(NAME test_boot_states COMMAND test_boot_states)

target_include_directories(test_bus PUBLIC ${TEST_INCLUDE_PATHS})
target_link_libraries(test_bus ${TEST_LIB_PATHS})
target_link_libraries(test_bus ${TEST_LIBS})
target_compile_options(test_bus PUBLIC ${TEST_COMPILE_OPTS})
add_test(NAME test_bus COMMAND test_bus)

target_include_directories(test_memory PUBLIC ${TEST_INCLUDE_PATHS})
target_link_libraries(test_memory ${TEST_LIB_PATHS})
target_link_libraries(test_memory -lsystemc -lgtest -lpthread -lgcov -lfmt)
target_compile_options(test_memory PUBLIC ${TEST_COMPILE_OPTS})
add_test(NAME test_memory COMMAND test_memory)

target_include_directories(test_ppu PUBLIC ${TEST_INCLUDE_PATHS})
target_link_libraries(test_ppu ${TEST_LIB_PATHS})
target_link_libraries(test_ppu -lsystemc -lgtest -lpthread -lSDL2 -lgcov)
target_compile_options(test_ppu PUBLIC ${TEST_COMPILE_OPTS})
add_test(NAME test_ppu COMMAND test_ppu)

target_include_directories(test_cpu PUBLIC ${TEST_INCLUDE_PATHS})
target_link_libraries(test_cpu ${TEST_LIB_PATHS})
target_link_libraries(test_cpu ${TEST_LIBS})
target_compile_options(test_cpu PUBLIC ${TEST_COMPILE_OPTS})
add_test(NAME test_cpu COMMAND test_cpu)


# Let them run sequentially.
# Might algo work in parallel, but better be safe.
set_tests_properties(
  test_cpu
  test_bus
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