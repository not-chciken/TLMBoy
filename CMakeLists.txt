######################################################
# Apache License, Version 2.0
# Copyright (c) 2025 chciken/Niko
######################################################

cmake_minimum_required(VERSION 3.5.0)
project(TLMBoy VERSION 1.0.0)
set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})

include(CTest)
include(CPack)
enable_testing()

# Need at least g++ v13.1 for featues like std::format.
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS "13.1")
    message(FATAL_ERROR "Insufficient gcc version (${CMAKE_CXX_COMPILER_VERSION}). Need at least 13.1.")
  endif()
endif()

set(CMAKE_CXX_FLAGS "-Wall -Wextra")
set(CMAKE_CXX_STANDARD 20)
add_definitions(-DSC_DISABLE_API_VERSION_CHECK) # "Fix" c++ version issue.
add_definitions(-DSC_INCLUDE_DYNAMIC_PROCESSES)
if(NOT DEFINED SYSTEMC_PATH)
  set(SYSTEMC_PATH /usr/local/systemc-2.3.3)
endif()
include_directories(${CMAKE_SOURCE_DIR}/src)

set(ALL_CPPS
  ${CMAKE_SOURCE_DIR}/src/apu.cpp
  ${CMAKE_SOURCE_DIR}/src/bus.cpp
  ${CMAKE_SOURCE_DIR}/src/cartridge.cpp
  ${CMAKE_SOURCE_DIR}/src/common.cpp
  ${CMAKE_SOURCE_DIR}/src/cpu.cpp
  ${CMAKE_SOURCE_DIR}/src/game_info.cpp
  ${CMAKE_SOURCE_DIR}/src/gb_top.cpp
  ${CMAKE_SOURCE_DIR}/src/gdb_server.cpp
  ${CMAKE_SOURCE_DIR}/src/generic_memory.cpp
  ${CMAKE_SOURCE_DIR}/src/io_registers.cpp
  ${CMAKE_SOURCE_DIR}/src/joypad.cpp
  ${CMAKE_SOURCE_DIR}/src/options.cpp
  ${CMAKE_SOURCE_DIR}/src/ppu.cpp
  ${CMAKE_SOURCE_DIR}/src/symfile_tracer.cpp
  ${CMAKE_SOURCE_DIR}/src/tcp_server.cpp
  ${CMAKE_SOURCE_DIR}/src/timer.cpp
  ${CMAKE_SOURCE_DIR}/src/utils.cpp
)

# The main executable.
add_executable(tlmboy src/main.cpp ${ALL_CPPS})
target_include_directories(tlmboy SYSTEM PUBLIC "${SYSTEMC_PATH}/include")
target_link_libraries(tlmboy "-L${SYSTEMC_PATH}/lib-linux64")
target_link_libraries(tlmboy -lsystemc -lSDL2)
target_compile_options(tlmboy PUBLIC -O3 -flto)

# The debug version.
add_executable(tlmboy_debug src/main.cpp ${ALL_CPPS})
target_include_directories(tlmboy_debug SYSTEM PUBLIC "${SYSTEMC_PATH}/include")
target_link_libraries(tlmboy_debug "-L${SYSTEMC_PATH}/lib-linux64")
target_link_libraries(tlmboy_debug -lsystemc -lSDL2)
target_compile_options(tlmboy_debug PUBLIC -O0 -g)

# For test purposes
add_executable(tlmboy_test src/main.cpp ${ALL_CPPS})
target_include_directories(tlmboy_test SYSTEM PUBLIC "${SYSTEMC_PATH}/include")
target_link_libraries(tlmboy_test "-L${SYSTEMC_PATH}/lib-linux64")
target_link_libraries(tlmboy_test -lgcov -lsystemc -lSDL2)
target_compile_options(tlmboy_test PUBLIC -O0 -g --coverage)

# Static tlmboy lib for faster compilation of tests.
add_library(tlmboy_static_test STATIC ${ALL_CPPS})
target_include_directories(tlmboy_static_test SYSTEM PUBLIC "${SYSTEMC_PATH}/include")
target_link_libraries(tlmboy_static_test "-L${SYSTEMC_PATH}/lib-linux64")
target_link_libraries(tlmboy_static_test -lgcov -lsystemc -lSDL2)
target_compile_options(tlmboy_static_test PUBLIC -O0 -g --coverage)

# For performance analysis.
add_executable(tlmboy_performance src/main.cpp ${ALL_CPPS})
target_include_directories(tlmboy_performance SYSTEM PUBLIC "${SYSTEMC_PATH}/include")
target_link_libraries(tlmboy_performance "-L${SYSTEMC_PATH}/lib-linux64")
target_link_libraries(tlmboy_performance -lgcov -lsystemc -lSDL2)
target_compile_options(tlmboy_performance PUBLIC -O3 -lprofiler -g)

# Copy the logo.
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/assets/tlmboy_icon.bmp DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

# Add system tests and unit tests.
add_subdirectory(tests)
