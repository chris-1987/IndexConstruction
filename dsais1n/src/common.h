////////////////////////////////////////////////////////////
/// Copyright (c) 2017, Sun Yat-sen University,
/// All rights reserved
/// \file common.h
/// \brief Definitions of constants for implementing DSAIS-1n
///
/// Constants including uint_types, suffix/substring types and so on.
///
/// \author Yi Wu
/// \date 2017.5
///////////////////////////////////////////////////////////

#ifndef __COMMON_H
#define __COMMON_H

#include "stxxl/bits/common/uint_types.h"
#include "stxxl/vector" 
#include "stxxl/bits/containers/queue.h"
#include "stxxl/bits/containers/sorter.h"
#include "stxxl/bits/io/syscall_file.h"

#include <limits>
#include <vector>
#include <utility>



// alias for integral types
using uint8 = stxxl::uint8;

using uint16 = stxxl::uint16;

using uint32 = stxxl::uint32;

using uint64 = stxxl::uint64;

using uint40 = stxxl::uint40;

// L-type and S-type 
constexpr uint8 L_TYPE = 0;

constexpr uint8 S_TYPE = 1;

// memory allocation
constexpr uint64 K_512 = 512 * 1024;

constexpr uint64 K_1024 = 1024 * 1024;

constexpr uint64 MAX_MEM = 3 * 1024 * K_1024;

constexpr uint64 MAX_ITEM = 16 * K_1024;

// for MY_VECTOR
const uint64 VEC_BUF_RAM = 2 * K_1024; // 2M

const uint64 PHI_VEC_EM = 20 * K_1024; // 50M

static uint32 global_file_idx = 0;

#endif // __COMMON_H
