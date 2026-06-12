######################################################
# Apache License, Version 2.0
# Copyright (c) 2025 chciken/Niko
######################################################

cmake_minimum_required(VERSION 3.5.0)
project(TLMBoy VERSION 2.0.0)
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

if(NOT DEFINED ENV{SYSTEMC_PATH})
  set(SYSTEMC_PATH /opt/systemc)
  message(STATUS "SYSTEMC_PATH not set, assigning default.")
else()
  set(SYSTEMC_PATH "$ENV{SYSTEMC_PATH}")
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
  ${CMAKE_SOURCE_DIR}/src/serial.cpp
  ${CMAKE_SOURCE_DIR}/src/symfile_tracer.cpp
  ${CMAKE_SOURCE_DIR}/src/tcp_server.cpp
  ${CMAKE_SOURCE_DIR}/src/timer.cpp
  ${CMAKE_SOURCE_DIR}/src/utils.cpp
)

# The main executable.
add_executable(tlmboy src/main.cpp ${ALL_CPPS})
target_include_directories(tlmboy SYSTEM PUBLIC "${SYSTEMC_PATH}/include")
target_link_libraries(tlmboy "-L${SYSTEMC_PATH}/lib-linux64")
target_link_libraries(tlmboy libsystemc.a -lSDL2)
target_compile_options(tlmboy PUBLIC -O3 -g)

# The ultra-fast executable.
add_executable(tlmboy_fast src/main.cpp ${ALL_CPPS})
target_include_directories(tlmboy_fast SYSTEM PUBLIC "${SYSTEMC_PATH}/include")
target_link_libraries(tlmboy_fast "-L${SYSTEMC_PATH}/lib-linux64")
target_link_libraries(tlmboy_fast libsystemc.a -lSDL2)
target_compile_options(tlmboy_fast PUBLIC -O3 -flto -march=native -fcf-protection=none -g -DNDEBUG)
target_link_options(tlmboy_fast PUBLIC -flto -no-pie)

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

# For performance analysis using gperftools.
add_executable(tlmboy_analysis src/main.cpp ${ALL_CPPS})
target_include_directories(tlmboy_analysis SYSTEM PUBLIC "${SYSTEMC_PATH}/include")
target_link_libraries(tlmboy_analysis "-L${SYSTEMC_PATH}/lib-linux64")
target_link_libraries(tlmboy_analysis -lgcov -lsystemc -lSDL2)
target_compile_options(tlmboy_analysis PUBLIC -O3 -lprofiler -g)

# Copy the logo.
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/assets/tlmboy_icon.bmp DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

# Add system tests and unit tests.
add_subdirectory(tests)
