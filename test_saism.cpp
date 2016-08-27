
#include <iostream>
#include <string>
#include <math.h>
#include <stdlib.h>
using namespace std;
#define DEBUGLEVEL 1
#include <time.h>
#if !defined( unix )
//#include <io.h>
#include <fcntl.h>
#endif


#include "indexconstruction.h"

using namespace wtl;


void SA_IS(unsigned char *s, int64 *SA, int64 n, int64 K, int64 cs, int64 level);

// comment to disable verifying the result SA

int main() {
	for (int32 rnd = 0; rnd < 1000; ++rnd) {
		std::cout << "test round: " << rnd << std::endl;
		int32 num_blocks = 10;
		string * fname_blocks = new string[num_blocks];

		for (int32 i = 0; i < num_blocks; ++i) {
			fname_blocks[i] = string("input" + std::to_string(i) + ".dat");
		}
		BlockInfo* info = new BlockInfo[num_blocks];

		////create data
		int64 total_num = 0;
		for (int32 i = 0; i < num_blocks; ++i) {
			auto seed = std::chrono::system_clock::now().time_since_epoch().count();
			std::default_random_engine gen(seed);
			std::binomial_distribution<int> dis(1, 0.4);

			//uint_type num_s_ch = MBUFSIZE * 2 + 100 * i;
			uint_type num_s_ch = 1000 + 100 * i;
			uint_type num_s = num_s_ch * 8;
			char * buf_s = nullptr; MemManager::allocMem(buf_s, num_s_ch);
			DataWrapper<bool> s(num_s, num_s_ch, buf_s);
			for (uint_type i = 0; i < num_s; ++i) {
				s.set(i, dis(gen));
				//if (i < num_blocks) {
				//	s.set(i, 0);
				//	cout << "0";
				//}
				//else {
				//	s.set(i, 1);
				//	cout << "1";
				//}
			}
			cout << "\n";
			MyIO<char>::write(s.getData(), fname_blocks[i], num_s_ch, std::ios_base::out | std::ios_base::binary, 0);
			MemManager::deAllocMem(buf_s, num_s_ch);
			total_num += num_s + 1; // add the sentinel
			info[i].len_blocks = num_s + 1;
			info[i].len_ch_blocks = num_s_ch + 1;
		}
		cout << "files have been produced.\n";

		//for (int32 i = num_blocks - 1; i >= 0; --i) {
		//	std::ifstream fin(fname_blocks[i]);
		//	fin.seekg(0, std::ios_base::end);
		//	info[i].len_ch_blocks = static_cast<int>(fin.tellg()) + 1;
		//	info[i].len_blocks = static_cast<int>(fin.tellg()) * 8 + 1;
		//	total_num += info[i].len_blocks;
		//	char * buffer = new char[info[i].len_ch_blocks - 1];
		//	MyIO<char>::read(buffer, fname_blocks[i], info[i].len_ch_blocks - 1, std::ios_base::in | std::ios_base::binary, 0);
		//	DataWrapper<bool> read_buffer(info[i].len_blocks - 1, info[i].len_ch_blocks - 1, buffer);
		//	for (int32 j = 0; j < info[i].len_blocks - 1; ++j)
		//		std::cout << read_buffer.get(j) << " ";
		//	std::cin.get();
		//}
		cout << "start to compute cde ..\n";
		auto start = std::chrono::system_clock::now();
		//write blocks num
		computeIndex(fname_blocks, num_blocks);
		auto stop = std::chrono::system_clock::now();
		cout << "compute cde over..\n";
		double duration = std::chrono::duration<double>(stop - start).count();
		cout << "compute over...spend time is " << duration << "s.\n";
		cout << "wtl::TEST::READ_NUM:" << wtl::TEST::READ_NUM << "\n";
		cout << "wtl::TEST::WRITE_NUM:" << wtl::TEST::WRITE_NUM << "\n";
		int64 io_volumn = wtl::TEST::READ_NUM + wtl::TEST::WRITE_NUM - total_num / 8;
		cout << "total I/O is " << io_volumn << "\n";
		double ratio_ss = duration * 1000000 / total_num;
		double ratio_is = static_cast<double>(io_volumn) / total_num;
		cout << "I/O ratio is " << ratio_is << "\n";
		cout << "time ratio is " << ratio_ss << "\n";
		//	cout << "The size is " << (static_cast<double>(total_num) /1024/1024) << " M\n";
		//	cout << "read  is " << (static_cast<double>(wtl::TEST::READ_NUM )/ 1024 / 1024) << "M.\n";
		//cout << "write  is " << (static_cast<double>(wtl::TEST::WRITE_NUM - (total_num / 8)) / 1024 / 1024) << "M.\n";
		//cout << "The ratio in size/seconds is " << ratio_ss << "byte/s.\n";
		//cout << "The ratio in io_volumn/size is " << ratio_is <<"byte/byte.\n";


		unsigned char * s_verify = new unsigned char[total_num + 1];

		uint_type idx = 0;
		for (int32 i = 0; i < num_blocks; ++i) {
			uint_type len_block = info[i].len_blocks - 1;
			uint_type len_ch_block = info[i].len_ch_blocks - 1;
			char * buf_block = new char[len_ch_block]; DataWrapper<bool> s_block(len_block, len_ch_block, buf_block);
			MyIO<char>::read(s_block.getData(), std::string(fname_blocks[i]), len_ch_block, std::ios_base::in | std::ios_base::binary, 0);
			for (uint_type j = 0; j < len_block; ++j) {
				s_verify[idx++] = s_block.get(j) + num_blocks + 1;
			}
			s_verify[idx++] = i + 1;
			delete[] buf_block;
		}
		s_verify[idx] = 0;

		int64 *sa_verify = new int64[total_num + 1];
		cout << "start to use sais to compute sa...\n";
		SA_IS(s_verify, sa_verify, total_num + 1, 255, 1, 0);
		cout << " use sais compute sa end...\n";
		/*DataWrapper<int64> e(total_num + 1, sa_verify);
		MyIO<int64>::write(e.getData(), "cmp_sa.dat", total_num + 1, std::ios_base::out | std::ios_base::binary, 0);
		delete[] sa_verify;
		delete[] s_verify;*/

		char * buf_d = new char[total_num / 8 + (total_num % 8 ? 1 : 0)];
		DataWrapper<bool> d(total_num, total_num / 8 + (total_num % 8 ? 1 : 0), buf_d);
		MyIO<char>::read(d.getData(), "merge_d.dat", total_num / 8 + (total_num % 8 ? 1 : 0), std::ios_base::in | std::ios_base::binary, 0);


		std::ifstream fin_e("merge_e.dat");
		fin_e.seekg(0, std::ios_base::end);
		etype len_fin_e = static_cast<uint64>(fin_e.tellg()) / sizeof(etype);
		std::cout << "len_fin_e: " << len_fin_e << std::endl;
		//std::cin.get();

		etype * buf_e = new etype[len_fin_e];
		DataWrapper<etype> e(len_fin_e, buf_e);
		MyIO<etype>::read(e.getData(), "merge_e.dat", len_fin_e, std::ios_base::in | std::ios_base::binary, 0);

		uint_type e_pos = 0;
		for (int64 i = 1; i < total_num + 1; ++i) {
			if (sa_verify[i] % K2 == K2 - 1 || sa_verify[i] == total_num - 1) {
				if (!d.get(i - 1)) {
					cout << "d..wrong...i:" << i << "\n";
					cin.get();
				}
				if (sa_verify[i] != e.get(e_pos++)) {
					cout << "e..wrong...i:" << i << "\n";
					cin.get();
				}
			}
			else {
				if (d.get(i - 1)) {
					cout << "d..wrong...i:" << i << "\n";
					cin.get();
				}
			}
		}

		std::cout << "d,e pass2\n";
		MemManager::deAllocMem<char>(buf_d, total_num / 8 + (total_num % 8 ? 1 : 0));
		MemManager::deAllocMem<etype>(buf_e, len_fin_e);


		char * bwt_verify = new char[total_num];
		uint_type * pos_sen_verify = new uint_type[num_blocks];
		for (int64 i = 1; i < total_num + 1; ++i) {
			if (sa_verify[i] != 0) {
				bwt_verify[i - 1] = s_verify[sa_verify[i] - 1];
				if (bwt_verify[i - 1] <= num_blocks) {
					std::cout << "ch: " << (uint32)s_verify[sa_verify[i] - 1] << " pos in T:" << total_num - 1 << " pos in BWT:" << i - 1 << std::endl;
					pos_sen_verify[(uint32)s_verify[sa_verify[i] - 1] - 1] = i - 1;
				}
			}
			else {
				bwt_verify[i - 1] = 0; // assumed to be the sentinel of the rightmost block, that is 0.
				std::cout << "ch: " << num_blocks << " pos in T:" << sa_verify[i] - 1 << " pos in BWT:" << i - 1 << std::endl;
				pos_sen_verify[num_blocks - 1] = i - 1;
			}
		}
		//std::cin.get();

		for (int64 i = 0; i < total_num; ++i) {
			if (bwt_verify[i] <= num_blocks) {
				bwt_verify[i] = 0;
			}
			if (bwt_verify[i] == num_blocks + 1) bwt_verify[i] = 0;
			if (bwt_verify[i] == num_blocks + 2) bwt_verify[i] = 1;
		}


		//computed pos_sen
		std::cout << "computed pos_sen:" << std::endl;
		uint_type * pos_sen = new uint_type[num_blocks];
		MyIO<uint_type>::read(pos_sen, std::string("merge_block_num.dat"), num_blocks, std::ios_base::in | std::ios_base::binary, 0);
		for (int i = 0; i < num_blocks; i++) {
			cout << "pos in BWT:" << pos_sen[i] << "\n";
		}

		std::cout << "verified pos_sen:" << std::endl;
		for (int i = 0; i < num_blocks; i++) {
			cout << "pos in BWT:" << pos_sen_verify[i] << "\n";
		}

		for (int i = 0; i < num_blocks; ++i) {
			if (pos_sen[i] != pos_sen_verify[i]) {
				std::cout << "wrong";
				std::cin.get();
			}
		}
		//std::cout << "num_blocks:" << pos_sen[num_blocks] << std::endl;
		//std::cout << "total:" << pos_sen[num_blocks + 1]<<std::endl;

		delete[] pos_sen;
		delete[] pos_sen_verify;
		std::cout << "pass3\n";


		char * buf_bwt = new char[total_num / 8 + (total_num % 8 ? 1 : 0)];
		DataWrapper<bool> bwt(total_num, total_num / 8 + (total_num % 8 ? 1 : 0), buf_bwt);
		MyIO<char>::read(bwt.getData(), "merge_bwt.dat", total_num / 8 + (total_num % 8 ? 1 : 0), std::ios_base::in | std::ios_base::binary, 0);

		for (int64 i = 0; i < total_num; ++i) {
			if (bwt_verify[i] != bwt.get(i))
				std::cout << "bwt wrong";
		}

		std::ifstream fin_c("merge_c.dat");
		fin_c.seekg(0, std::ios_base::end);
		etype len_fin_c = static_cast<uint64>(fin_c.tellg()) / sizeof(etype);
		std::cout << "len_fin_c: " << len_fin_c << std::endl;

		etype * buf_c = new etype[len_fin_c];
		DataWrapper<etype> c(len_fin_c, buf_c);
		MyIO<etype>::read(c.getData(), "merge_c.dat", len_fin_c, std::ios_base::in | std::ios_base::binary, 0);


		uint_type cnt_c = 0;
		uint_type cur_c = 0;
		if (c.get(cur_c) != static_cast<etype>(cnt_c)) {
			std::cout << "elem " << cur_c << " wrong";
			std::cin.get();
		}
		cur_c++;
		for (int64 i = 0; i < total_num; ++i) {
			if (bwt_verify[i] == 1) ++cnt_c;
			if (i % K2 == K2 - 1 || i == total_num - 1) {
				if (cnt_c != c.get(cur_c)) {
					std::cout << "elem " << cur_c << " wrong";
					std::cin.get();
				}
				cur_c++;
			}
		}

		std::cout << "pass4\n";
		//std::cin.get();


		delete[] info;
		delete[] fname_blocks;
		delete[] s_verify;
		delete[] sa_verify;
		delete[] bwt_verify;

		MemManager::deAllocMem<etype>(buf_c, len_fin_c);
		MemManager::deAllocMem<char>(buf_bwt, total_num / 8 + (total_num % 8 ? 1 : 0));
	}

	std::cin.get();
	std::cout << "over.\n";
}
//
//}
//
//
////#include <iostream>
////#include <chrono>
////#include <random>
////#include <fstream>
////
////#include "myutility.h"
////
////int main() {
//	//using wtl::uint40;
//
//	//int size = 50000;
//	//uint40 * data_uint40 = new uint40[size];
//	//uint64 * data_verify = new uint64[size];
//	//for (unsigned int i = 0; i < size; ++i) {
//	//	data_uint40[i] = std::numeric_limits<uint40::low_type>::min();
//	//	data_uint40[i]= i;
//	//	data_verify[i] = std::numeric_limits<uint40::low_type>::min();
//	//	data_verify[i]+= i;
//	//}
//
//	//wtl::MyIO<uint40>::write(data_uint40, "test.dat", size, std::ios_base::out | std::ios_base::binary, 0);
//
//
//	//std::ifstream fin("test.dat");
//	//fin.seekg(0, std::ios_base::end);
//	//std::cout << "sizeof uint40: " << sizeof(uint40) << " total len: " << fin.tellg() / sizeof(uint40) << std::endl;
//
//	//uint40 * data2_uint40 = new uint40[size];
//	//wtl::MyIO<uint40>::read(data2_uint40, "test.dat", size, std::ios_base::out | std::ios_base::binary, 0);
//
//	//
//	//for (unsigned int i = 0; i < size; ++i) {
//	//	if (data2_uint40[i] != data_uint40[i]) {
//	//		std::cout << "wrong";
//	//		std::cin.get();
//	//	}
//
//	//	if ((uint64)data2_uint40[i] != data_verify[i]) {
//	//		std::cout << "wrong";
//	//		std::cin.get();
//	//	}
//	//}
//	//
//	//std::cout << "pass";
//	//std::cin.get();
//
//
//
////}



//#include "myutility.h"
//#include "mytypes.h"
//
//int main() {
//	using wtl::int32;
//
//	int32 num_ch_buf_uint4 = 10000;
//	int32 num_buf_uint4 = 10000 * 2;
//	char * uint4_buf = new char[num_ch_buf_uint4];
//	wtl::DataWrapper<wtl::uint4> x(num_buf_uint4, num_ch_buf_uint4, uint4_buf);
//	for (int32 i = 0; i < num_buf_uint4; ++i) {
//		x.set(i, (wtl::uint8)(i % 16));
//	}
//
//	for (int32 i = 0; i < num_buf_uint4; ++i) {
//		std::cout << (int)x.get(i);
//		std::cin.get();
//	}
//
//	std::cin.get();
//}
