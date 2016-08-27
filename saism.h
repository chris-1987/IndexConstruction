/***************************************************************************
  *  Copyright (C) 2016 Yi Wu and Mengmeng Guo <wu.yi.christian@gmail.com>
  *  Description: "saism.h"
  *  Assumption1: compute at most 2GB for uint32 and 
  **************************************************************************/

#include <iostream>
#include "myutility.h"

namespace wtl {
	
//@usage: compute bucket size.
//@param:
//			_s: the string to be processed.
//			_num_s: the number of characters in _s.
//			_end: if true, compute the ending position of each bucket; otherwise, compute the starting position of each bucket. 
//			_bkt: the size of each character bucket is recorded in the bkt array.
template<typename T>
void getBuckets(DataWrapper<T> & _s, uint_type _num_s, etype * _bkt, uint_type _num_bkt, bool _end) {
	int_type i = 0;
	uint_type sum = 0;
	for (i = 0; i < _num_bkt; ++i) _bkt[i] = 0; // clear all buckets
	for (i = 0; i < _num_s; ++i) ++_bkt[_s.get(i)]; // compute the size of each bucket
	for (i = 0; i < _num_bkt; ++i) { sum += _bkt[i]; _bkt[i] = _end ? sum - 1 : sum - _bkt[i]; }
}

//@usage: induce L.
//@param:
//			_s: the string to be processed.
//			_t: the type array 
//			_num_s: the number of characters in _s.
//			_alpha: the largest character in _s.
//			_level: the recursion level.
//			_bkt: the bucket array.
//			_sa: the suffix array.		
template<typename T>
void induceSAL(DataWrapper<T> & _s, DataWrapper<bool> & _t, etype * _sa, uint_type _num_s, etype * _bkt, uint_type _alpha, int32 _level) {
	int_type i, j;
	getBuckets<T>(_s, _num_s, _bkt, _alpha + 1, false); // find heads of buckets
	if (_level == 0) ++_bkt[0]; // there are 0 characters in level 0 and the sentinel is assumed to be represented by 0.
	for (i = 0; i < _num_s; ++i) {
		if (_sa[i] != ETYPE_MAX && _sa[i] != ETYPE_MIN) {
			j = _sa[i] - 1; 
			if (!_t.get(j)) { // _sa[i] is unsigned, if _sa[i] == 0, then j does not exist. 
				_sa[_bkt[_s.get(j)]] = j; 
				++_bkt[_s.get(j)];
			}
		}
	}
}

//@usage: induce S.
//@param:
//			_s: the string to be processed.
//			_t: the type array 
//			_num_s: the number of characters in _s.
//			_alpha: the largest character in _s.
//			_level: the recursion level.
//			_bkt: the bucket array.
//			_sa: the suffix array.
template<typename T>
void void induceSAS(DataWrapper<T> & _s, DataWrapper<bool> & _t, etype * _sa, uint_type _num_s, etype * _bkt, uint_type _alpha, int32 _level) {
	(DataWrapper<T> & _s, DataWrapper<bool> & _t, etype * _sa, uint_type _num_s, etype * _bkt, uint_type _alpha, int32 _level) {
	int_type i, j;
	getBuckets<T>(_s, _num_s, _bkt, _alpha + 1, true); // find ends of buckets
	for (i = _num_s - 1; i >= 0; --i)
		if (_sa[i] != ETYPE_MAX && _sa[i] != ETYPE_MIN) {
			j = _sa[i] - 1;
			if (_t.get(j)) { // _sa[i] is unsigned, if _sa[i] == 0, then j == EMPTY, which does not exist. 
				_sa[_bkt[_s.get(j)]] = j;
				--_bkt[_s.get(j)];
			}
		}
}

//@notice: level == 0, the type of character is bool; level > 0, the type of character is etype.
//@param:
//			_s: the string to be processed.
//			_num_s: the number of characters in _s.
//			_alpha: the size of alphabet for _s.
//			_level: the recursion level.
//			_sa: the suffix array of _s.
template<typename T>
void SAISM(DataWrapper<T> & _s, etype *_sa, uint_type _num_s, uint_type _alpha, int32 _level) {
	int_type i, j;

	char * t_buf = nullptr; MemManager::allocMem<char>(t_buf, _num_s / 8 + 1); //_num_s - 1 is an integral multiple of 8.
	DataWrapper<bool> t(_num_s, _num_s / 8 + 1, t_buf);

	// stage 1: reduce the problem by at least 1/2
	// allocate space for t array and compute t.
	t.set(_num_s - 1, 1); t.set(_num_s - 2, 0); // s[n - 1] is the sentinel and must be s-type, s[n - 2] must be l-type.
	for (i = _num_s - 3; i >= 0; --i) t.set(i, (_s.get(i) < _s.get(i + 1) || (_s.get(i) == _s.get(i + 1) && t.get(i + 1))) ? 1 : 0);

	// allocate space for bkt array
	etype *bkt = nullptr; MemManager::allocMem<etype>(bkt, _alpha + 1); //bucket counters

	// sort all the S-substrings
	getBuckets<T>(_s, _num_s, bkt, _alpha + 1, true); // find ends of buckets
	
	//init sa
	for (i = 0; i < _num_s; ++i) {
		_sa[i] = ETYPE_MAX;
	}

	// find lms-chars in _s[1, _num_s - 3]
	for (i = _num_s - 3; i >= 1; --i) { 
		if (t.get(i) && !t.get(i - 1)) {
			_sa[bkt[_s.get(i)]] = i;
			--bkt[_s.get(i)];
		}
	}

	//_s[_num_s - 1] is the sentinel smallest than any other characters
	_sa[0] = _num_s - 1; 

	induceSAL<T>(_s, t, _sa, _num_s, bkt, _alpha, _level);
	induceSAS<T>(_s, t, _sa, _num_s, bkt, _alpha, _level);

	MemManager::deAllocMem<etype>(bkt, _alpha + 1);

	// compact all the sorted substrings into the first n1 items of s
	uint_type num_s1 = 0;
	for (i = 0; i < _num_s; ++i) {
		if (_sa[i] > 0 && t.get(_sa[i]) && !t.get(_sa[i] - 1)) {
			_sa[num_s1++] = _sa[i];
		}
	}

	for (i = num_s1; i < _num_s; ++i) _sa[i] = ETYPE_MAX; //init

	// find the lexicographic names of all substrings
	uint_type name = 0;
	int64 prev = -1;
	for (i = 0; i< num_s1; ++i) {
		uint_type pos = _sa[i]; bool diff = false;
		for (uint_type d = 0; d < _num_s; ++d) {
			if (prev == -1 || pos + d == _num_s - 1 || prev + d == _num_s - 1 || _s.get(pos + d) != _s.get(prev + d) || t.get(pos + d) != t.get(prev + d)) {
				diff = true;
				break;
			}
			else {
				if (d > 0 && ((t.get(pos + d) && !t.get(pos + d - 1)) || (t.get(prev + d) && !t.get(prev + d - 1)))) {
					break;
				}
			}
		}
		if (diff) {
			++name;
			prev = pos;
		}
		pos = pos / 2;
		_sa[num_s1 + pos] = name - 1;
	}
	for (i = _num_s - 1, j = _num_s - 1; i >= num_s1; --i)
		if (_sa[i] != ETYPE_MAX) _sa[j--] = _sa[i];

	// s1 is done now
	etype *sa1 = _sa;
	DataWrapper<etype> s1(num_s1, _sa + _num_s - num_s1);
	// stage 2: solve the reduced problem

	// recurse if names are not yet unique
	if (name < num_s1) {
		SAISM<etype>(s1, sa1, num_s1, name - 1, _level + 1);
	}
	else { // generate the suffix array of s1 directly
		for (i = 0; i < num_s1; ++i) {
			sa1[s1.get(i)] = i;
		}
	}

	// stage 3: induce the result for the original problem

	MemManager::allocMem<etype>(bkt, _alpha + 1); //bucket counters

	// put all left-most S characters into their buckets
	getBuckets(_s, _num_s, bkt, _alpha + 1, true); // find ends of buckets

	j = 0;
	for (i = 1; i < _num_s; ++i) {
		if (t.get(i) && !t.get(i - 1)) s1.set(j++,i);// get p1
	}
	for (i = 0; i < num_s1; ++i) sa1[i] = s1.get(sa1[i]); // get index in s1
	for (i = num_s1; i < _num_s; ++i) _sa[i] = ETYPE_MAX; // init SA[n1..n-1]
	for (i = num_s1 - 1; i >= 0; --i) {
		j = _sa[i]; _sa[i] = ETYPE_MAX;
		if (_level == 0 && i == 0) {
			_sa[0] = _num_s - 1;
		}
		else {
			_sa[bkt[_s.get(j)]] = j;
			--bkt[_s.get(j)];
		}
	}
	induceSAL<T>(_s, t, _sa, _num_s, bkt, _alpha, _level);
	induceSAS<T>(_s, t, _sa, _num_s, bkt, _alpha, _level);

	MemManager::deAllocMem<etype>(bkt, _alpha + 1);
	MemManager::deAllocMem<char>(t_buf, _num_s / 8 + 1);
}


} //namespace wtl