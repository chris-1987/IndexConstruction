
//#include "indexconstruction.h"
//#include "myassumption.h"
//#include "saism.h"
//#include "mycommon.h"
//
//using wtl::uint_type;
//using wtl::DataWrapper;
//using wtl::int64;
//
//int main() {
//	auto seed = std::chrono::system_clock::now().time_since_epoch().count();
//	std::default_random_engine gen(seed);
//	std::binomial_distribution<int> dis(1, 0.4);
//
//
//	uint_type n_ch = 20;
//	uint_type n_bit = n_ch * 8;
//	char * data_ch = new char[n_ch + 1];
//	DataWrapper<bool> s(n_bit + 1, n_ch + 1, data_ch);
//	for (int64 i = 0; i < n_bit; ++i) {
//		s.set(i, dis(gen));
//	}
//	s.set(n_bit, 0); //the sentinel
//
//	uint_type *sa = new uint_type[n_bit + 1];
//	
//	wtl::SAISM(s, sa, n_bit + 1, 1, 0);
//	
//
//
//	////delete[] data_ch;
//	//
//	//std::fstream fin(std::string("input.dat"));
//	//fin.seekg(0, std::ios_base::end);
//	//myInt n_ch2 = fin.tellg();
//	//std::cout << "n_ch2: " << n_ch2 << std::endl;
//	//char * data_ch2 = new char[n_ch2];
//	//DataWrapper<bool> s2(n_ch2 * 8, n_ch2, data_ch2);
//	//MyIO<char>::read(s2.getData(), std::string("input.dat"), n_ch2, std::ios_base::in | std::ios_base::binary, 0);
//
//	////std::cout << "the input string: " << std::endl;
//	////myInt n_remain_toread = n_ch2;
//	////myInt n_blksize = SBUFSIZE;
//	////myInt blknum = n_remain_toread / n_blksize  + (n_remain_toread % n_blksize ? 1: 0);
//	////myInt n_next_read = 0;
//	////for (myInt i = blknum - 1; i >= 0; --i) {
//	////	n_next_read = n_remain_toread > n_blksize ? n_blksize : n_remain_toread;
//	////	for (myInt j = 0; j < n_next_read * 8; ++j) {
//	////		std::cout << s2.get((n_remain_toread - n_next_read) * 8 + j) << " ";
//	////	}
//	////	n_remain_toread -= n_next_read;
//	////	std::cout << std::endl;
//	////}
//
//	////std::cin.get();
//
//	//computeIndex(std::string("input.dat"));
//
//	//int n_ver_ch = n_ch2;
//	//int n_ver_bit = n_ver_ch * 8;
//
//	//char * s_ver_buffer = new char[n_ver_ch + 1]; DataWrapper<bool> s_ver(n_ver_bit + 1, n_ver_ch + 1, s_ver_buffer); for (myInt i = 0; i < n_ver_bit + 1; ++i) s_ver.set(i, s2.get(i)); s_ver.set(n_ver_bit, 0);
//	//char * bwt_ver_buffer = new char[n_ver_ch + 1]; DataWrapper<bool> bwt_ver(n_ver_bit + 1, n_ver_ch + 1, bwt_ver_buffer);
//	//int *sa_ver = new int[n_ver_bit + 1];
//	//char * st_ver_buffer = new char[n_ver_ch + 1]; DataWrapper<bool> st_ver(n_ver_bit + 1, n_ver_ch + 1, st_ver_buffer); for (myInt i = 0; i < n_ver_bit + 1; ++i) st_ver.set(i, 0);
//	//char * t_ver_buffer = new char[n_ver_ch + 1]; DataWrapper<bool> t_ver(n_ver_bit + 1, n_ver_ch + 1, t_ver_buffer);
//
//	//computeSA<bool>(s_ver, st_ver, n_ver_bit + 1, 0, 1, 1, 1, sa_ver, t_ver, 0);
//
//	////std::cout << "\nsa_ver:\n";
//	////for (myInt i = 0; i < n_ver_bit + 1; ++i) {
//	////	std::cout << sa_ver[i] << " ";
//	////}
//	////std::cin.get();
//
//	//for (myInt i = 1, j = 0; i < n_ver_bit + 1; ++i, ++j) {
//	//	if (sa_ver[i] == 0) {
//	//		bwt_ver.set(j, 0);
//	//	}
//	//	else {
//	//		bwt_ver.set(j, s_ver.get(sa_ver[i] - 1));
//	//	}
//	//}
//
//	////std::cout << "\nbwt_ver:\n";
//	////for (myInt i = 0; i < n_ver_bit; ++i) {
//	////	std::cout << bwt_ver.get(i) << " ";
//	////}
//	////std::cin.get();
//
//	////test
//	//std::cout << "\nproduced bwt:\n";
//	//char * test_buffer = new char[n_ch2];
//	//MyIO<char>::read(test_buffer, std::string("merge_bwt.dat"), n_ch2, std::ios_base::in | std::ios_base::binary, 0);
//	//DataWrapper<bool> testWrapper(n_ch2 * 8, n_ch2, test_buffer);
//	////std::cout << "\n test: \n";
//	////for (myInt i = 0; i < n_ch2 * 8; ++i)
//	////	std::cout << testWrapper.get(i) << " ";
//	////std::cin.get();
//
//	//for (myInt i = 0; i < n_ver_bit; ++i) {
//	//	if (testWrapper.get(i) != bwt_ver.get(i)) {
//	//		std::cout << "wrong";
//	//		std::cin.get();
//	//	}
//	//}
//
//	//std::cout << "ok" << std::endl;
//
//	//delete[] data_ch2;
//	//delete[] test_buffer;
//	//delete[] s_ver_buffer;
//	//delete[] bwt_ver_buffer;
//	//delete[] sa_ver;
//	//delete[] st_ver_buffer;
//	//delete[] t_ver_buffer;
//
//	//std::cin.get();
//
//
//	//std::cout << sizeof(size_t) << std::endl;
//	//std::cin.get();
//
//
//
//	myAssumption::myAssumption::checkAssumptions();
//
//	std::cin.get();
//}
//	
//
//
////#include "myutility.h"
////#include "saism.h"
////
////#define _SAISM_DEBUG
////
////int main() {
////	////test MyIO and DataWrapper
////	//auto seed1 = std::chrono::system_clock::now().time_since_epoch().count();
////	//std::default_random_engine gen1(seed1);
////	//std::binomial_distribution<int> dis1(1, 0.4);
////
////	//myInt n_bit = 634;
////	//myInt n_ch = n_bit / 8 + (n_bit % 8 ? 1 : 0);
////	//char * s_ch = new char[n_ch];
////	//bool * s_bool = new bool[n_bit];
////	//DataWrapper<bool> s_bit(n_bit, n_ch, s_ch);
////	//for (myInt i = 0; i < n_bit; ++i) {
////	//	s_bool[i] = dis1(gen1) ? 1 : 0;
////	//	s_bit.set(i, s_bool[i]);
////	//}
////
////
////	//for (myInt i = 0; i < n_bit; ++i) {
////	//	if (s_bool[i] != s_bit.get(i)) {
////	//		std::cout << "wrong";
////	//	}
////	//}
////
////	//std::cout << "pass1";
////	//std::cin.get();
////
////	//std::string fn_out("bitwritten.dat");
////	//MyIO<char>::write(s_bit.getData(), fn_out, s_bit.numofch(), std::ios_base::in | std::ios_base::out | std::ios_base::binary, 0); //openmode = rb+
////
////	//char * s_ch_cp = new char[n_ch];
////	//DataWrapper<bool> s_bit_cp(n_bit, n_ch, s_ch_cp);
////	//std::string fn_in("bitwritten.dat");
////	//MyIO<char>::read(s_bit_cp.getData(), fn_in, s_bit_cp.numofch(), std::ios_base::in | std::ios_base::binary, 0); //openmode = rb
////
////	//for (myInt i = 0; i < n_bit; ++i) {
////	//	if (s_bit_cp.get(i) != s_bit.get(i)){
////	//		std::cout << "wrong";
////	//		std::cin.get();
////	//	}
////	//}
////	//std::cout << "pass2";
////	//std::cin.get();
////	//
////
////	//auto seed2 = std::chrono::system_clock::now().time_since_epoch().count();
////	//std::default_random_engine gen2(seed2);
////	//std::binomial_distribution<int> dis2(1, 0.4);
////
////	//myInt n2_bit = 64;
////	//myInt n2_ch = n2_bit / 8 + (n2_bit % 8 ? 1 : 0);
////	//char * s2_ch = new char[n2_ch];
////	//bool * s2_bool = new bool[n2_bit];
////	//DataWrapper<bool> s2_bit(n2_bit, n2_ch, s2_ch);
////	//for (myInt i = 0; i < n2_bit; ++i) {
////	//	s2_bool[i] = dis2(gen2) ? 1 : 0;
////	//	s2_bit.set(i, s2_bool[i]);
////	//}
////
////
////	//for (myInt i = 0; i < n2_bit; ++i) {
////	//	if (s2_bool[i] != s2_bit.get(i)) {
////	//		std::cout << "wrong";
////	//	}
////	//}
////
////	//std::cout << "pass21";
////	//std::cin.get();
////
////	//std::string fn2_out("bitwritten.dat");
////	//MyIO<char>::write(s2_bit.getData(), fn2_out, s2_bit.numofch(), std::ios_base::in | std::ios_base::out | std::ios_base::binary, s_bit.numofch()); //rb+
////
////
////	//char * s2_ch_cp = new char[n2_ch];
////	//DataWrapper<bool> s2_bit_cp(n2_bit, n2_ch, s2_ch_cp);
////	//std::string fn2_in("bitwritten.dat");
////	//MyIO<char>::read(s2_bit_cp.getData(), fn2_in, s2_bit.numofch(), std::ios_base::in | std::ios_base::binary, s_bit.numofch()); //rb
////
////
////	//for (myInt i = 0; i < n2_bit; ++i) {
////	//	if (s2_bit_cp.get(i) != s2_bit.get(i)) {
////	//		std::cout << "wrong";
////	//		std::cin.get();
////	//	}
////	//}
////	//std::cout << "pass22";
////	//std::cin.get();
////
////	//
////
////	//char * s3_ch = new char[n_ch + n2_ch];
////	//DataWrapper<bool> s3_bit(n_ch * 8 + n2_bit, n_ch + n2_ch, s3_ch);
////	//MyIO<char>::read(s3_bit.getData(), fn2_in, n_ch + n2_ch, std::ios_base::in | std::ios_base::binary, 0);
////
////	//for (myInt i = 0; i < n_bit; ++i) {
////	//	if (s3_bit.get(i) != s_bit.get(i)) {
////	//		std::cout << "wrong";
////	//		std::cin.get();
////	//	}
////	//}
////
////	//for (myInt i = 0; i < n2_bit; ++i) {
////	//	if (s3_bit.get(i + n_ch * 8) != s2_bit.get(i)) {
////	//		std::cout << "wrong";
////	//		std::cin.get();
////	//	}
////	//}
////
////	//std::cout << "pass";
////	//delete[] s_ch;
////	//delete[] s_ch_cp;
////	//delete[] s_bool;
////	//delete[] s2_ch;
////	//delete[] s2_ch_cp;
////	//delete[] s2_bool;
////	//delete[] s3_ch;
////
////
////	//small instance 1:s = xy[0]=101000101001, x= 10100010100, y = 101#, st = 011111011110 -- pass
////	{
////		int n_bit = 12;
////		int n_ch = n_bit / 8 + (n_bit % 8 ? 1 : 0);
////		char s_ch[]= {-94, -112};
////		DataWrapper<bool> s_bit(n_bit, n_ch, s_ch); 
////		char st_ch[] ={ 125, -32};
////		DataWrapper<bool> st_bit(n_bit, n_ch, st_ch);
////		myInt r = 9;
////		bool tofch = 0;
////		myInt alpha = 1;
////		myInt n = 12;
////		myInt sa[12];
////		myInt level = 0;
////		
////		for (myInt i = 0; i < n_bit; ++i) std::cout << s_bit.get(i) << " "; std::cin.get();
////		for (myInt i = 0; i < n_bit; ++i) std::cout << st_bit.get(i) << " "; std::cin.get();
////
////
////		computeSA<bool>(s_bit, st_bit, n, r, tofch, false, 1, sa, 0);
////
////	}
////
//////	//small instance 2: s = xy[0] = 010101001110101100100, x = 01010100111010110010, y = 010#) -- pass
//////	{
//////		unique_ptr<vector<bool>> ptr_s(new vector<bool>{ 0,1,0,1,0,1,0,0,1,1,1,0,1,0,1,1,0,0,1,0,0 });
//////		unique_ptr<vector<bool>> ptr_st(new vector<bool>{ 0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,1,0,0,1,0 });
//////		myInt r = 3;
//////		bool tofch = 1;
//////		myInt alpha = 1;
//////		myInt n = 21;
//////		vector<myInt> *ptr_sa = new vector<myInt>(n);
//////		myInt level = 0;
//////		computeSA<bool>(ptr_s->begin(), ptr_s->end() - 1, ptr_st->begin(), ptr_st->end() - 1, r, tofch, alpha, n, ptr_sa->begin(), ptr_sa->end() - 1, level);
//////
//////		for (auto sait = ptr_sa->begin(); sait < ptr_sa->end(); ++sait) {
//////			auto i = *sait;
//////			for (auto j = i; j < n; ++j)
//////				std::cout << (*ptr_s)[j] << " ";
//////			std::cout << "1 0 #" << std::endl;
//////		}
//////		std::cin.get();
//////	}
//////
//////	//small instance 3: s = xy[0] = 10101111010011101011001001, x = 1010111101001110101100100, y = 11# -- pass
//////	{
//////		unique_ptr<vector<bool>> ptr_s(new vector<bool>{ 1,0,1,0,1,1,1,1,0,1,0,0,1,1,1,0,1,0,1,1,0,0,1,0,0,1 });
//////		unique_ptr<vector<bool>> ptr_st(new vector<bool>{ 1,1,1,1,0,0,1,1,1,1,1,1,0,1,1,1,1,1,1,1,1,1,1,1,1,0 });
//////		myInt r = 22;
//////		bool tofch = 0;
//////		myInt alpha = 1;
//////		myInt n = 26;
//////		vector<myInt> *ptr_sa = new vector<myInt>(n);
//////		myInt level = 0;
//////		computeSA<bool>(ptr_s->begin(), ptr_s->end() - 1, ptr_st->begin(), ptr_st->end() - 1, r, tofch, alpha, n, ptr_sa->begin(), ptr_sa->end() - 1, level);
//////
//////		for (auto sait = ptr_sa->begin(); sait < ptr_sa->end(); ++sait) {
//////			auto i = *sait;
//////			for (auto j = i; j < n; ++j)
//////				std::cout << (*ptr_s)[j] << " ";
//////			std::cout << "1 1 #" << std::endl;
//////		}
//////		std::cin.get();
//////	}
//////
//////	//small instance 3: s = xy[0] = 0000001, x = 00000, y = 1# -- pass
//////	{
//////		unique_ptr<vector<bool>> ptr_s(new vector<bool>{ 0,0,0,0,0,0,1 });
//////		unique_ptr<vector<bool>> ptr_st(new vector<bool>{ 1,1,1,1,1,1,0 });
//////		myInt r = 6;
//////		bool tofch = 0;
//////		myInt alpha = 1;
//////		myInt n = 7;
//////		vector<myInt> *ptr_sa = new vector<myInt>(n);
//////		myInt level = 0;
//////		computeSA<bool>(ptr_s->begin(), ptr_s->end() - 1, ptr_st->begin(), ptr_st->end() - 1, r, tofch, alpha, n, ptr_sa->begin(), ptr_sa->end() - 1, level);
//////
//////		for (auto sait = ptr_sa->begin(); sait < ptr_sa->end(); ++sait) {
//////			auto i = *sait;
//////			for (auto j = i; j < n; ++j)
//////				std::cout << (*ptr_s)[j] << " ";
//////			std::cout << "1 1 #" << std::endl;
//////		}
//////		std::cin.get();
//////	}
//////
//////
//////	//small instance 4: s = xy[0] = 1111110, x = 11111, y = 0# -- pass
//////	{
//////		unique_ptr<vector<bool>> ptr_s(new vector<bool>{ 1,1,1,1,1,1,0 });
//////		unique_ptr<vector<bool>> ptr_st(new vector<bool>{ 0,0,0,0,0,0,0 });
//////		myInt r = 0;
//////		bool tofch = 0;
//////		myInt alpha = 1;
//////		myInt n = 7;
//////		vector<myInt> *ptr_sa = new vector<myInt>(n);
//////		myInt level = 0;
//////		computeSA<bool>(ptr_s->begin(), ptr_s->end() - 1, ptr_st->begin(), ptr_st->end() - 1, r, tofch, alpha, n, ptr_sa->begin(), ptr_sa->end() - 1, level);
//////
//////		for (auto sait = ptr_sa->begin(); sait < ptr_sa->end(); ++sait) {
//////			auto i = *sait;
//////			for (auto j = i; j < n; ++j)
//////				std::cout << (*ptr_s)[j] << " ";
//////			std::cout << "#" << std::endl;
//////		}
//////		std::cin.get();
//////	}
//////
//////#endif
////
////}
