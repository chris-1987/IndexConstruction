/***************************************************************************
  *  Copyright (C) 2016 Yi Wu and Mengmeng Guo <wu.yi.christian@gmail.com>
  *  Description: "mycommon.h"
  **************************************************************************/

#ifndef _MY_COMMON_H
#define _MY_COMMON_H

#include <limits>
#include "mytypes.h"
#include "myuint.h"

namespace wtl {

//constant expression declaration and definition.
constexpr uint64 TBUFSIZE = 1 * 1024 * 1024 * 1024; //total buffer size in bytes
constexpr uint64 SBUFSIZE = 1 * 1024 * 1024;  //small buffer size
constexpr uint64 MBUFSIZE = 64 * 1024 * 1024;  //middle buffer size
constexpr uint64 LBUFSIZE = 512 * 1024 * 1024;  //large buffer size
constexpr int32 K1 = 40; // the interval between two sampling points in L (bwt), K1 = O(logn).
constexpr int32 K2 = 40; // the internval between two samping points in T, K2 = O(logn).
constexpr uint8 MASK[] = { 0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01 };//char bit mask
//constexpr uint_type MEM_CAPACITY = 1024 * 1024 * 1024 * 60; //available memory in byte
constexpr uint_type MEM_CAPACITY = 1024 * 1024 * 100; //available memory in byte
//using etype = uint32;
using etype = uint40; //element type
static const etype ETYPE_MAX = std::numeric_limits<etype>::max();
static const etype ETYPE_MIN = std::numeric_limits<etype>::min();

} //namespace wtl
#endif