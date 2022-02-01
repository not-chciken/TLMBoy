#pragma once
/*******************************************************************************
 * Copyright (C) 2021 chciken
 * MIT License
 *
 * All the debug prints. Feel free to add further ones.
 ******************************************************************************/

// Comment in/out the defines to meet your needs
// or add them to your compilation flags
// #define ENABLE_DEBUG_FUNC
// #define ENABLE_DBG_LOG_INST
// #define ENABLE_DBG_LOG_CPU
// #define ENABLE_DBG_LOG_CPU_REG
// #define ENABLE_DBG_LOG_GDB // GDB extension
// #define ENABLE_DBG_LOG_PPU // Pixel Processing Unit
// #define ENABLE_DBG_LOG_BSM // Bank Switch Memory
// #define ENABLE_DBG_LOG_IE // Interrupt Enable
// #define ENABLE_DBG_LOG_JP // Joypad

#if !defined(NDEBUG)
  #if defined(ENABLE_DEBUG_FUNC)
    #define DEBUG_FUNC(x) x
  #endif
  #if defined(ENABLE_DBG_LOG_INST)
    #define DBG_LOG_INST(x) (std::cout << "[" << __FILE__ << "] INST: " << x << std::endl)
  #endif
  #if defined(ENABLE_DBG_LOG_CPU)
    #define DBG_LOG_CPU(x) (std::cout << "[" << __FILE__ << "] CPU: " << x << std::endl)
  #endif
  #if defined(ENABLE_DBG_LOG_CPU_REG)
    #define DBG_LOG_CPU_REG(x) (std::cout << x << std::endl)
  #endif
  #if defined(ENABLE_DBG_LOG_GDB)
    #define DBG_LOG_GDB(x) (std::cout << "[" << __FILE__ << "] GDB: " << x << std::endl)
  #endif
  #if defined(ENABLE_DBG_LOG_PPU)
    #define DBG_LOG_PPU(x) (std::cout << "[" << __FILE__ << "] PPU: " << x << std::endl)
  #endif
  #if defined(ENABLE_DBG_LOG_BSM)
    #define DBG_LOG_BSM(x) (std::cout << "[" << __FILE__ << "] BSM: " << x << std::endl)
  #endif
  #if defined(ENABLE_DBG_LOG_IE)
    #define DBG_LOG_IE(x) (std::cout << "[" << __FILE__ << "] IE: " << x << std::endl)
  #endif
  #if defined(ENABLE_DBG_LOG_JP)
    #define DBG_LOG_JP(x) (std::cout << "[" << __FILE__ << "] JP: " << x << std::endl)
  #endif
#endif

#if !defined(DEBUG_FUNC)
  #define DEBUG_FUNC(x)
#endif
#if !defined(DBG_LOG_INST)
  #define DBG_LOG_INST(x)
#endif
#if !defined(DBG_LOG_CPU)
  #define DBG_LOG_CPU(x)
#endif
#if !defined(DBG_LOG_CPU_REG)
  #define DBG_LOG_CPU_REG(x)
#endif
#if !defined(DBG_LOG_GDB)
  #define DBG_LOG_GDB(x)
#endif
#if !defined(DBG_LOG_PPU)
  #define DBG_LOG_PPU(x)
#endif
#if !defined(DBG_LOG_BSM)
  #define DBG_LOG_BSM(x)
#endif
#if !defined(DBG_LOG_IE)
  #define DBG_LOG_IE(x)
#endif
#if !defined(DBG_LOG_JP)
  #define DBG_LOG_JP(x)
#endif
