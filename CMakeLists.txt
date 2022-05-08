cmake_minimum_required(VERSION 3.0.0)
project(TLMBoy VERSION 1.0.0)
set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})

include(CTest)
include(CPack)
enable_testing()

set(CMAKE_CXX_FLAGS "-Wall")
set(CMAKE_CXX_STANDARD 20)
add_definitions(-DSC_DISABLE_API_VERSION_CHECK) # "Fix" c++ version issue.
add_definitions(-DSC_INCLUDE_DYNAMIC_PROCESSES)
if(NOT DEFINED SYSTEMC_PATH)
  set(SYSTEMC_PATH /usr/local/systemc-2.3.3)
endif()
include_directories(${CMAKE_SOURCE_DIR}/src)

set(ALL_CPPS
  ${CMAKE_SOURCE_DIR}/src/bus.cpp
  ${CMAKE_SOURCE_DIR}/src/cartridge.cpp
  ${CMAKE_SOURCE_DIR}/src/cpu.cpp
  ${CMAKE_SOURCE_DIR}/src/game_info.cpp
  ${CMAKE_SOURCE_DIR}/src/gb_top.cpp
  ${CMAKE_SOURCE_DIR}/src/gdb_server.cpp
  ${CMAKE_SOURCE_DIR}/src/generic_memory.cpp
  ${CMAKE_SOURCE_DIR}/src/joypad.cpp
  ${CMAKE_SOURCE_DIR}/src/ppu.cpp
  ${CMAKE_SOURCE_DIR}/src/tcp_server.cpp
  ${CMAKE_SOURCE_DIR}/src/timer.cpp
  ${CMAKE_SOURCE_DIR}/src/utils.cpp
)

# The main executable.
add_executable(tlmboy src/main.cpp ${ALL_CPPS})
target_include_directories(tlmboy PUBLIC "${SYSTEMC_PATH}/include")
target_link_libraries(tlmboy "-L${SYSTEMC_PATH}/lib-linux64")
target_link_libraries(tlmboy -lsystemc -lSDL2 -lfmt)
target_compile_options(tlmboy PUBLIC -O3)

# For test purposes
add_executable(tlmboy_test src/main.cpp ${ALL_CPPS})
target_include_directories(tlmboy_test PUBLIC "${SYSTEMC_PATH}/include")
target_link_libraries(tlmboy_test "-L${SYSTEMC_PATH}/lib-linux64")
target_link_libraries(tlmboy_test -lgcov -lsystemc -lSDL2 -lfmt)
target_compile_options(tlmboy_test PUBLIC -O0 -g --coverage)

# Add system tests and unit tests.
add_subdirectory(tests)
