/***************************************************************************
*  Copyright (C) 2016 Yi Wu and Mengmeng Guo <wu.yi.christian@gmail.com>
*  Description: "mytypes.h"
**************************************************************************/
#ifndef _MY_TYPES_H
#define _MY_TYPES_H

namespace wtl{
#ifdef	_MSC_VER
	using int8 = __int8;
	using uint8 = unsigned __int8;
	using int16 = __int16;
	using uint16 = unsigned __int16;
	using int32 = __int32;
	using uint32 = unsigned __int32;
	using int64 = __int64;
	using uint64 = unsigned __int64;
#else
	using int8 = char;
	using uint8 = unsigned char;
	using int16 = short;
	using uint16 = unsigned short;
	using int32 = int;
	using uint32 = unsigned int;
	using int64 = long long int;
	using uint64 = unsigned long long int;
#endif

constexpr int32 my_pointer_size = sizeof(void *);

template<int32 ptrSize>
struct choose_int_types {};

template<>
struct choose_int_types<4> { //32-bit
	using int_type = int32;
	using unsigned_type = uint32;
};

template<>
struct choose_int_types<8> { //64-bit
	using int_type = int64;
	using unsigned_type = uint64;
};

using int_type = choose_int_types<my_pointer_size>::int_type;
using uint_type = choose_int_types<my_pointer_size>::unsigned_type;

} //namespace wtl


#endif // _MY_TYPES_H