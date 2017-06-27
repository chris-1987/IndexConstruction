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

constexpr uint64 MAX_MEM = 1 * 1024 * K_1024;

//constexpr uint64 MAX_MEM = 70;

constexpr uint64 MAX_ITEM = 16 * K_1024;

// for MY_VECTOR
//const uint64 PHYSICAL_VECTOR_CAPACITY = 50 * K_1024; // 30M
const uint64 PHYSICAL_VECTOR_CAPACITY = 3 * K_1024;

// for MY_PQ
//const uint64 MEM_HEAP_CAPACITY = 5;

#endif // __COMMON_H
