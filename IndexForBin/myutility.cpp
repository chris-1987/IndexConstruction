/***************************************************************************
*  Copyright (C) 2016 Yi Wu and Mengmeng Guo <wu.yi.christian@gmail.com>
*  Description: #include "myutility.h"
**************************************************************************/

#include "myutility.h"


//random generator engine
void wtl::GenSeq::gen() {
	switch (d) {
		case Distribution::BIN: binGen(); break;
		case Distribution::UNI: uniGen(); break;
		default: std::cout << "do not support the specific distribution.\n"; //todo: error handle
	}
}

void wtl::GenSeq::binGen() {
	auto seed = std::chrono::system_clock::now().time_since_epoch().count();
	std::default_random_engine gen(seed);
	std::binomial_distribution<int> dis(1, 0.5);

	for (int64 i = 0; i < dw.numofbit(); ++i) {
		dw.set(i, static_cast<bool>(dis(gen)));
	}

	MyIO<char>::write(dw.getData(), fn, dw.numofch(), std::ios_base::out | std::ios_base::binary, 0);
}

void wtl::GenSeq::uniGen() {
	auto seed = std::chrono::system_clock::now().time_since_epoch().count();
	std::default_random_engine gen(seed);
	std::uniform_int_distribution<int> dis(0, 1);

	for (int64 i = 0; i < dw.numofbit(); ++i) {
		dw.set(i, static_cast<bool>(dis(gen)));
	}

	MyIO<char>::write(dw.getData(), fn, dw.numofch(), std::ios_base::out | std::ios_base::binary, 0);
}


//MemManager
wtl::uint_type const wtl::MemManager::total_byte = MEM_CAPACITY;
wtl::uint_type wtl::MemManager::used_byte = 0;
wtl::uint_type wtl::MemManager::avail_byte = MEM_CAPACITY;

wtl::int64 wtl::TEST::READ_NUM = 0;
wtl::int64 wtl::TEST::WRITE_NUM = 0;
//MyAssumption