#pragma once

#include "myutility.h"
using namespace wtl;
template<typedef T>
class BufferWrapper {
private:
	std::string *fn;
	int32 buffer_size;
	int32 block_num;
	int32 block_size;
	T *buffer_all;
	T *pos_block;
	T **buffer_block;
	T *remian_to_read;
public:
	BufferWrapper(std::string &*fn, int32 block_num, int32 block_size) :fn(fn), block_size(block_size), block_num(block_num) {
		buffer_size = block_num * block_size;
		buffer_all = new T[buffer_size];
		pos_block = new T[block_num];
		init();
	}
	init() {
		int32 read_buffer_size = 0;
		for (int32 i = 0; i < block_num; i++) {
			pos_block[i] = 0;
			buffer_block[i] = &buffer_all[i*block_size];
			//initialize remian_to_read and buffer_block array
			std::ifstream fin(fn[i]);
			fin.seekg(0, std::ios_base::end);
			remian_to_read[i] = fin.tellg() / sizeof(T);
			read_buffer_size = remian_to_read[i] > block_size ? block_size : remian_to_read[i];
			fin.read((char *)buffer_block[i], read_buffer_size * sizeof(T) / sizeof(char));
			remian_to_read[i] -= read_buffer_size;
		}
		
	}

	bool isDone() {
		for (int32 i = 0; i < block_num; i++) {
			if (remian_to_read[i] > 0)return false;
		}
		return true;
	}
};