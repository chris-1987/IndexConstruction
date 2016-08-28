/***************************************************************************
*  Copyright (C) 2016 Yi Wu and Mengmeng Guo <wu.yi.christian@gmail.com>
*  Description: "myuint40.h"
**************************************************************************/

#ifndef _MY_UINT40_H
#define _MY_UINT40_H

#include <iostream>
#include "mycommon.h"

namespace wtl{
#if _MSC_VER
#pragma pack(push, 1)
#endif
template <typename HighType>
class uint_pair
{
public:
	//! lower part type, always 32-bit
	typedef uint32 low_type;
	//! higher part type, currently either 8-bit or 16-bit
	typedef HighType high_type;

private:
	low_type low;

	high_type high;

	static uint_type low_max(){
		return std::numeric_limits<low_type>::max();
	}

	static const size_t low_bits = 8 * sizeof(low_type);

	static uint_type high_max(){
		return std::numeric_limits<high_type>::max();
	}

	static const size_t high_bits = 8 * sizeof(high_type);

public:
	static const size_t digits = low_bits + high_bits;

	static const size_t bytes = sizeof(low_type) + sizeof(high_type);

	inline uint_pair(){}

	inline uint_pair(const low_type& l, const high_type& h) : low(l), high(h){}

	inline uint_pair(const uint_pair& a) : low(a.low), high(a.high){}

	inline uint_pair(const uint32& a) : low(a), high(0){}

	//! assumption: a >= 0
	inline uint_pair(const int32& a) : low(a), high(0){}


	//!assumption: no overflow
	inline uint_pair(const uint64& a) : low((low_type)(a & low_max())), high((high_type)((a >> low_bits) & high_max())){}

	//!assumption: a >= 0 and no overflow
	inline uint_pair(const int64& a) : low((low_type)(a & low_max())), high((high_type)((a >> low_bits) & high_max())) {}

	inline uint64 u64() const {
		return ((uint64)high) << low_bits | (uint64)low;
	}

	inline operator uint64 () const{
		return u64();
	}

	inline uint_pair& operator ++ (){
		if (low == low_max())
			++high, low = 0;
		else
			++low;
		return *this;
	}

	inline uint_pair& operator -- (){
		if (low == 0)
			--high, low = (low_type)low_max();
		else
			--low;
		return *this;
	}

	inline uint_pair& operator += (const uint_pair& b){
		uint64 add = (uint64)low + b.low;
		low = (low_type)(add & low_max());
		high = (high_type)(high + b.high + ((add >> low_bits) & high_max()));
		return *this;
	}

	inline bool operator == (const uint_pair& b) const{
		return (low == b.low) && (high == b.high);
	}

	inline bool operator != (const uint_pair& b) const{
		return (low != b.low) || (high != b.high);
	}

	inline bool operator < (const uint_pair& b) const{
		return (high < b.high) || (high == b.high && low < b.low);
	}

	inline bool operator <= (const uint_pair& b) const{
		return (high < b.high) || (high == b.high && low <= b.low);
	}

	inline bool operator > (const uint_pair& b) const{
		return (high > b.high) || (high == b.high && low > b.low);
	}

	// !assumption: b >= 0
	inline bool operator > (const int32 & b) const {
		return (high > 0 || high == 0 && low > b);
	}
	inline bool operator >= (const uint_pair& b) const
	{
		return (high > b.high) || (high == b.high && low >= b.low);
	}

	friend std::ostream& operator << (std::ostream& os, const uint_pair& a)
	{
		return os << a.u64();
	}

	static uint_pair min()
	{
		return uint_pair(std::numeric_limits<low_type>::min(),
			std::numeric_limits<high_type>::min());
	}

	static uint_pair max()
	{
		return uint_pair(std::numeric_limits<low_type>::max(),
			std::numeric_limits<high_type>::max());
	}
}
#if _MSC_VER
;
#pragma pack(pop)
#else
__attribute__((packed));
#endif

using uint40 = uint_pair<uint8>;

using uint48 = uint_pair<uint16>;

}

namespace std {

//! template class providing some numeric_limits fields for uint_pair types.
template <typename HighType>
class numeric_limits<wtl::uint_pair<HighType> >{
public:
	//! yes we have information about uint_pair
	static const bool is_specialized = true;

	//! return an uint_pair instance containing the smallest value possible
	static wtl::uint_pair<HighType> min(){
		return wtl::uint_pair<HighType>::min();
	}

	//! return an uint_pair instance containing the largest value possible
	static wtl::uint_pair<HighType> max(){
		return wtl::uint_pair<HighType>::max();
	}

	//! return an uint_pair instance containing the smallest value possible
	static wtl::uint_pair<HighType> lowest(){
		return min();
	}

	//! unit_pair types are unsigned
	static const bool is_signed = false;

	//! uint_pair types are integers
	static const bool is_integer = true;

	//! unit_pair types contain exact integers
	static const bool is_exact = true;

	//! unit_pair radix is binary
	static const int radix = 2;

	//! number of binary digits (bits) in uint_pair
	static const int digits = wtl::uint_pair<HighType>::digits;

	//! epsilon is zero
	static const wtl::uint_pair<HighType> epsilon(){
		return wtl::uint_pair<HighType>(0, 0);
	}

	//! rounding error is zero
	static const wtl::uint_pair<HighType> round_error(){
		return wtl::uint_pair<HighType>(0, 0);
	}

	//! no exponent
	static const int min_exponent = 0;

	//! no exponent
	static const int min_exponent10 = 0;

	//! no exponent
	static const int max_exponent = 0;

	//! no exponent
	static const int max_exponent10 = 0;

	//! no infinity
	static const bool has_infinity = false;
};

} // namespace std

#endif