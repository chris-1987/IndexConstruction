/***************************************************************************
  *  Copyright (C) 2016 Yi Wu and Mengmeng Guo <wu.yi.christian@gmail.com>
  *  Description: "myutility.h"
  **************************************************************************/

#ifndef _MY_UTILITY_H
#define _MY_UTILITY_H

#include <iostream>
#include <fstream>
#include <random>
#include <chrono>
#include <string>
#include <memory>

#include "mycommon.h"

namespace wtl {
	
//@usage: for DataWrapper<bits>
template<int32 _bits>
class suint {};

template<>
class suint<2> {
public:
	static constexpr uint8 MASK[] = { 0xC0, 0x30, 0x0C, 0x03 };
	static constexpr uint8 SHIFT[] = { 6,4,2,0 };
};

template<>
class suint<4> {
public:
	static constexpr uint8 MASK[] = { 0xf0, 0x0f };
	static constexpr uint8 SHIFT[] = { 4,0 };
};

using uint2 = suint<2>;
using uint4 = suint<4>;


//@usage: data Wrapper
template<typename T>
class DataWrapper {
private:
	T * data;
	uint_type n;
public:
	DataWrapper(uint_type _n, T * _data);
	T get(uint_type _idx);
	void set(uint_type _idx, T _val);
	T * getData();
	void swapData(DataWrapper<T> & _dw);
	void setMember(uint_type _n, T * _data);
	uint_type numofelem();
};

template<typename T>
inline DataWrapper<T>::DataWrapper(uint_type _n = 0, T * _data = nullptr) :n(_n), data(_data) {
}

template<typename T>
inline T DataWrapper<T>::get(uint_type _idx) {
	return  data[_idx];
}

template<typename T>
inline void DataWrapper<T>::set(uint_type _idx, T _val) {
	data[_idx] = _val;
}

template<typename T>
inline T * DataWrapper<T>::getData() {
	return data;
}

template<typename T>
inline uint_type DataWrapper<T>::numofelem() {
	return n;
}

template<typename T>
inline void DataWrapper<T>::setMember(uint_type _n, T * _data) {
	n = _n;
	data = _data;
}

template<typename T>
inline void DataWrapper<T>::swapData(DataWrapper<T> & _dw) {
	char * tmp = data;
	data = _dw.getData();
	_dw.data = tmp;
}

template<>
class DataWrapper<bool> {
private:
	char * data;
	uint_type n_ch;
	uint_type n_bit;
public:
	DataWrapper(uint_type _n_bit, uint_type _n_ch, char * _data);
	bool get(uint_type _idx);
	void set(uint_type _idx, bool _val);
	char * getData();
	void swapData(DataWrapper<bool> & _dw);
	uint_type numofch();
	uint_type numofbit();
	void setMember(uint_type _n_bit, uint_type _n_ch, char * _data);
};

inline DataWrapper<bool>::DataWrapper(uint_type _n_bit = 0, uint_type _n_ch = 0, char * _data = nullptr) : n_bit(_n_bit), n_ch(_n_ch), data(_data) {
}


inline bool DataWrapper<bool>::get(uint_type _idx) {
	return  (data[(_idx) / 8] & MASK[(_idx) % 8]) ? 1 : 0;
}

inline void DataWrapper<bool>::set(uint_type _idx, bool _val) {
	data[_idx / 8] = _val ? (MASK[_idx % 8] | data[_idx / 8]) : ((~MASK[_idx % 8]) & data[_idx / 8]);
}

inline char * DataWrapper<bool>::getData() {
	return data;
}

inline uint_type DataWrapper<bool>::numofch() {
	return n_ch;
}

inline uint_type DataWrapper<bool>::numofbit() {
	return n_bit;
}

inline void DataWrapper<bool>::setMember(uint_type _n_bit, uint_type _n_ch, char * _data) {
	n_bit = _n_bit;
	n_ch = _n_ch;
	data = _data;
}

inline void DataWrapper<bool>::swapData(DataWrapper<bool> & _dw) {
	char * tmp = data;
	data = _dw.getData();
	_dw.data = tmp;
}



//@usage: specialization for uint2 and uint4
template<int32 bits>
class DataWrapper<wtl::suint<bits>> {
private:
	static constexpr int32 div = 8 / bits;
	char * data;
	uint_type n_ch;
	uint_type n;
public:
	DataWrapper(uint_type _n = 0, uint_type _n_ch = 0, char * _data = nullptr) : n(_n), n_ch(_n_ch), data(_data) {}
	char get(uint_type _idx) {
		return (data[_idx / div] >> wtl::suint<bits>::SHIFT[_idx % div]) & wtl::suint<bits>::MASK[div - 1];
	}
	void set(uint_type _idx, char _val) {
		data[_idx / div] = (_val << wtl::suint<bits>::SHIFT[_idx % div]) | (data[_idx / div] & ~wtl::suint<bits>::MASK[_idx % div]);
	}
	char * getData() {
		return data;
	}

	uint_type numofch() {
		return n_ch;
	}

	uint_type numofele() {
		return n;
	}
	void setMember(uint_type _n, uint_type _n_ch, char * _data) {
		n = _n;
		n_ch = _n_ch;
		data = _data;
	}

	void swapData(DataWrapper<wtl::uint2> & _dw) {
		char * tmp = data;
		data = _dw.getData();
		_dw.data = tmp;
	}
};


//@usage: random generator engine
enum class Distribution { UNI, BIN }; // UNI = uniform, BIN = binomial, GEO = geometry, NEG = negative binomial;

class GenSeq {
public:
	GenSeq(Distribution _d, std::string _fn, uint_type _n_bit, uint_type _n_ch, char* _data);
	void gen();
private:
	Distribution d;
	std::string fn;
	DataWrapper<bool> dw;
	void binGen();
	void uniGen();
};

inline GenSeq::GenSeq(Distribution _d, std::string _fn, uint_type _n_bit, uint_type _n_ch, char * _data) :d(_d), fn(_fn), dw(_n_bit, _n_ch, _data) {
}


//@usage: IO operations for preliminary types (char, int,...)
template<typename T>
class MyIO {
public:
	static void write(T * _ta, std::string _fn, uint_type _n, std::ios_base::openmode _mode, uint_type _offset);
	static void read(T * _ta, std::string _fn, uint_type _n, std::ios_base::openmode _mode, uint_type _offset);
	static uint_type getFileLenCh(std::string _fn);
};

template<typename T>
void MyIO<T>::write(T * _ta, std::string _fn, uint_type _n, std::ios_base::openmode _mode, uint_type _offset) {
	std::ofstream fout(_fn, _mode);
	fout.seekp(_offset, std::ios_base::beg);
	fout.write((char *)_ta, _n * sizeof(T) / sizeof(char));
	TEST::WRITE_NUM += _n * sizeof(T) / sizeof(char);
}

template<typename T>
void MyIO<T>::read(T * _ta, std::string _fn, uint_type _n, std::ios_base::openmode _mode, uint_type _offset) {
	std::ifstream fin(_fn, _mode);
	fin.seekg(_offset, std::ios_base::beg);
	fin.read((char *)_ta, _n * sizeof(T) / sizeof(char));
	TEST::READ_NUM += _n * sizeof(T) / sizeof(char);
}

template<typename T>
uint_type MyIO<T>::getFileLenCh(std::string _fn) {
	std::ifstream fin(_fn);
	fin.seekg(0, std::ios_base::end);
	return fin.tellg();
}

//@usage: memmory mananger (no need to instantiate an instance)
class MemManager {
public:
	template<typename T>
	static void allocMem(T * &data_ptr, uint_type _num_elem);

	template<typename T>
	static void deAllocMem(T * &data_ptr, uint_type _num_elem);

	static void getStatus() {
		std::cerr << "total_type: " << total_byte << " used_byte: " << used_byte << " avail_byte: " << avail_byte << std::endl;
	}

	static uint_type getAvail() {
		return avail_byte;
	}
private:
	MemManager() = delete;
	MemManager(const MemManager & _mm) = delete;
	MemManager & operator= (const MemManager & _mm) = delete;
	static const uint_type total_byte;
	static uint_type used_byte;
	static uint_type avail_byte;

};

template<typename T>
inline void MemManager::allocMem(T * &data_ptr, uint_type _num_elem) {
	if (avail_byte >= _num_elem * sizeof(T)) {
		data_ptr = new T[_num_elem];
		avail_byte -= _num_elem * sizeof(T);
		used_byte += _num_elem *sizeof(T);
	}
	else {
		std::cerr << "not enough available memory.\n";
		std::cin.get();
		exit(-1);
	}
}

template<typename T>
inline void MemManager::deAllocMem(T * &data_ptr, uint_type _num_elem) {
	avail_byte += _num_elem * sizeof(T);
	used_byte -= _num_elem * sizeof(T);
	delete[] data_ptr;
	data_ptr = nullptr;
}



// @usage: check whether the running environment conforms to the pre-conditions of the program. No need to instantiate an instance.
class myAssert {
public:
	static void checkAssert(bool expr, std::string  msg);

private:
	myAssert() = delete;
	myAssert(const myAssert &) = delete;
	myAssert & operator= (const myAssert &) = delete;
};

inline void myAssert::checkAssert(bool expr, std::string  msg){
	if (!expr) {
		std::cerr << msg;
		exit(-1);
	}
}

class TEST {
public:
	static int64 WRITE_NUM;
	static int64 READ_NUM;
};
} //namespace wtl

#endif