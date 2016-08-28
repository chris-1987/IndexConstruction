/***************************************************************************
*  Copyright (C) 2016 Yi Wu and Mengmeng Guo <wu.yi.christian@gmail.com>
*  Description: #include "myassumption.h"
**************************************************************************/

#include "indexconstruction.h"


//#define _IC_DEBUG

void wtl::computeIndex(std::string * &_fname_blocks, int32 _num_blocks) {
	uint_type total_num = 0; //record the number of characters in all the blocks (including the appending sentinel character)

	BlockInfo *info = new BlockInfo[_num_blocks];

	for (int32 i = 0; i <_num_blocks; ++i) {
		info[i].len_ch_blocks = MyIO<bool>::getFileLenCh(_fname_blocks[i]); // the number of characters in the file is an integral multiple of 8.
		info[i].len_blocks = info[i].len_ch_blocks * 8; // the number of characters
		info[i].len_ch_blocks += 1; // one more char to store the appending bit
		info[i].len_blocks += 1; // one more bit to store the appending bit
		info[i].start_offset = total_num;
		total_num += info[i].len_blocks;
	}

	for (int32 i = _num_blocks - 1; i >= 0; --i) {
		std::cout << "len_blocks:" << info[i].len_blocks << " start_offset:" << info[i].start_offset << "\n";
		// stage 1: compute the suffix array of the string. 
		char *buffer_x = nullptr; MemManager::allocMem<char>(buffer_x, info[i].len_ch_blocks); DataWrapper<bool> x(info[i].len_blocks, info[i].len_ch_blocks, buffer_x);
		MyIO<char>::read(x.getData(), _fname_blocks[i], info[i].len_ch_blocks - 1, std::ios_base::in | std::ios_base::binary, 0); //the source file doesn't include the sentinel and the length of the source file is an integral multiple of 8.
		x.set(info[i].len_blocks - 1, 0); //append the sentinel $

#ifdef _IC_DEBUG
		std::cout << "x: \n";
		for (int32 j = 0; j < info[i].len_blocks; ++j) {
			std::cout << x.get(j) << " ";
		}
		std::cin.get();
#endif
		//compute the suffix array
		etype * sa = nullptr; MemManager::allocMem<etype>(sa, info[i].len_blocks);
		std::cout << i<<"th block compute sa by saism..\n";
		SAISM<bool>(x, sa, info[i].len_blocks, 1, 0);
		std::cout <<i<< "th block compute sa end..\n";

#ifdef _IC_DEBUG

		std::cout << "sa: \n";
		for (int32 j = 0; j < info[i].len_blocks; ++j) {
			std::cout << sa[j] << " ";
		}
		std::cin.get();
#endif
		// stage 2: compute the bwt array of x and partial gtxy starting at x.
		char *buffer_bwt = nullptr; MemManager::allocMem<char>(buffer_bwt, info[i].len_ch_blocks); DataWrapper<bool> bwt(info[i].len_blocks, info[i].len_ch_blocks, buffer_bwt);
		uint_type num_c = info[i].len_blocks / K1 + (info[i].len_blocks % K1 ? 1 : 0) + 1;
		etype *c = nullptr; MemManager::allocMem<etype>(c, num_c);
		uint_type cnt = 0;
		uint_type leftmost_rank = 0;
		std::cout << i<<"th block compute bwt ..\n";
		computeBWTandPartialGTXY(x, sa, bwt, c, num_c, cnt, leftmost_rank, i, total_num, info);
		std::cout << i << "th block compute bwt end ..\n";
		MemManager::deAllocMem<etype>(sa, info[i].len_blocks);

#ifdef _IC_DEBUG

		std::cout << "bwt: \n";
		for (int32 j = 0; j < info[i].len_blocks; ++j) {
			std::cout << bwt.get(j) << " ";
		}
		std::cin.get();
		std::cout << "c: \n";
		for (int32 j = 0; j < num_c; ++j) {
			std::cout << c[j] << " ";
		}
		std::cin.get();

		std::cout << "leftmost_rank : " << leftmost_rank << std::endl;
#endif


		// stage 3: compute the gap array of x and partial gtxy starting at y.
		//computePartialGTXYandGap();
		uint_type num_gap = info[i].len_blocks + 1; // the size of gap array is len + 1
		etype *gap = nullptr; MemManager::allocMem<etype>(gap, num_gap);
		std::cout << i << "th block compute gap ..\n";
		computePartialGTXYandGap(x, bwt, c, num_c, cnt, gap, num_gap, i, _num_blocks, info, leftmost_rank, _fname_blocks);
		std::cout << i << "th block compute gap end..\n";
		MemManager::deAllocMem<char>(buffer_x, info[i].len_ch_blocks);
		MemManager::deAllocMem<char>(buffer_bwt, info[i].len_ch_blocks);
		MemManager::deAllocMem<etype>(c, num_c);
		MemManager::deAllocMem<etype>(gap, info[i].len_blocks + 1);
	}
	//int64 *sentinel_pos = new int64[_num_blocks + 2];
	// stage 4: merge the index.
	std::cout << "start to merge ..\n";
	mergeIndex(total_num, _num_blocks, info);
	std::cout << "merge over ..\n";

	// save _num_blocks,sentinel_pos,total_num in files in order
	
	/*for (int i = 0; i < _num_blocks; i++) {
		sentinel_pos[i] = info[i].sentinel_pos;
	}
	sentinel_pos[_num_blocks] = _num_blocks;
	sentinel_pos[_num_blocks + 1] = total_num;
	MyIO<int64>::write(sentinel_pos, std::string("merge_block_num.dat"), _num_blocks + 2, std::ios_base::out | std::ios_base::binary, 0);
*/
	delete[] info;
}


void wtl::computeBWTandPartialGTXY(DataWrapper<bool> & _x, etype * _sa, DataWrapper<bool> & _bwt, etype * _c, uint_type _num_c, uint_type & _cnt, uint_type & _leftmost_rank, int32 _id_block, uint_type _total_num, BlockInfo * _info) {
	_leftmost_rank = 0; //the rank of xy in sa_x. 
	_cnt = 0; // the number of character '1's in x
	for (int64 i = 0; i < _num_c; ++i) _c[i] = 0;

	uint_type len_block = _info[_id_block].len_blocks;
	uint_type len_block_ch = _info[_id_block].len_ch_blocks;
	uint_type start_offset = _info[_id_block].start_offset;
	uint_type len_e = len_block / K2 + 2; // at most K2 + 2 sampling points (notice: the rightmost block) 
	char * wbuf_gtxy = nullptr; MemManager::allocMem<char>(wbuf_gtxy, len_block_ch); DataWrapper<bool> gtxy(len_block, len_block_ch, wbuf_gtxy);
	char * wbuf_d = nullptr; MemManager::allocMem<char>(wbuf_d, len_block_ch); DataWrapper<bool> d(len_block, len_block_ch, wbuf_d);
	etype * wbuf_e = nullptr; MemManager::allocMem<etype>(wbuf_e, len_e); DataWrapper<etype> e(len_e, wbuf_e);



	int64 cur_e = 0;
	bool flag_isafter = false;
	for (int64 i = 0; i < len_block; ++i) {
		//compute d,e
		if ((_sa[i] + start_offset) % K2 == K2 - 1 || _sa[i] + start_offset == _total_num - 1) { // the position indices of the sampling points : K2 - 1, 2 * K2 - 1, ...., (note that the rightmost element in the rightmost block is also a sampling point whether or not _total_num - 1 % K2 == K2 - 1 
			d.set(i, true);
			e.set(cur_e++, _sa[i] + start_offset);
		}
		else {
			d.set(i, false);
		}
		gtxy.set(_sa[i], flag_isafter == true ? 1 : 0); //compute gtxy
		_cnt = _x.get(_sa[i]) ? (_cnt + 1) : _cnt; //compute cnt
												   //compute _c and _bwt
		if (_sa[i] == ETYPE_MIN) {
			_leftmost_rank = i;//The heading character x[0] has no preceding character in the partial string XY. We set its preceding character to 0.
			_bwt.set(i, 0); // the preceding character of x[0] is the sentinel $.
			flag_isafter = true;
			_info[_id_block].sentinel_pos = i;
			//std::cout << "block phase: relative_sentinel_pos: " << i << std::endl;
		}
		else { //_sa[i] > 0 and thus _sa[i] - 1 >= 0
			_bwt.set(i, _x.get(_sa[i] - 1));
			if (_bwt.get(i)) {
				++_c[i / K1];
			}
			//_c[i / K1] = _bwt.get(i) ? (_c[i / K1] + 1) : _c[i / K1];
		}
	}

	//accumulate elements in c
	uint_type sum = 0, cur_num;
	for (int64 i = 0; i < _num_c; ++i) {
		cur_num = _c[i];
		_c[i] = sum;
		sum += cur_num;
	}

	MyIO<char>::write(gtxy.getData(), std::string("gt_blk" + std::to_string(_id_block) + ".dat"), len_block_ch, std::ios_base::out | std::ios_base::binary, 0);
	MyIO<char>::write(_bwt.getData(), std::string("bwt_blk" + std::to_string(_id_block) + ".dat"), len_block_ch, std::ios_base::out | std::ios_base::binary, 0);
	MyIO<char>::write(d.getData(), std::string("d_blk" + std::to_string(_id_block) + ".dat"), len_block_ch, std::ios_base::out | std::ios_base::binary, 0);
	MyIO<etype>::write(e.getData(), std::string("e_blk" + std::to_string(_id_block) + ".dat"), cur_e, std::ios_base::out | std::ios_base::binary, 0);

	_info[_id_block].num_sample_e = cur_e;

#ifdef _IC_DEBUG
	std::cout << "partial gtxy x[0,m):\n";
	for (int32 j = 0; j < len_block; ++j) {
		std::cout << gtxy.get(j) << " ";
	}

	std::cout << "\ncnt : " << _cnt << std::endl;
	std::cin.get();
#endif

	MemManager::deAllocMem<char>(wbuf_gtxy, len_block_ch);
	MemManager::deAllocMem<char>(wbuf_d, len_block_ch);
	MemManager::deAllocMem<etype>(wbuf_e, len_e);
}

void wtl::computePartialGTXYandGap(DataWrapper<bool> & _x, DataWrapper<bool> & _bwt, etype * _c, uint_type _num_c, uint_type _cnt, etype * _gap, uint_type _num_gap, int32 _id_block, int32 _num_blocks, BlockInfo * info, uint_type _leftmost_rank, std::string *_fname_blocks) {
	for (int64 i = 0; i <_num_gap; ++i) _gap[i] = 0; // the size of gap is n_x + 1, where gap[n_x] records the number of suffixes in y that are greater than X[m-1]Y.

	uint_type len_blk_x = info[_id_block].len_blocks;
	uint_type len_ch_blk_x = info[_id_block].len_ch_blocks;

	if (_id_block == _num_blocks - 1) { //y is empty, no need to conduct further computation
		MyIO<etype>::write(_gap, std::string("gap_blk" + std::to_string(_id_block) + ".dat"), _num_gap, std::ios_base::out | std::ios_base::binary, 0);
	}
	else {
		for (int64 i = _num_blocks - 1; i > _id_block; --i) { // y
			uint_type len_ch_blk_y = info[i].len_ch_blocks;
			uint_type len_blk_y = info[i].len_blocks;
			char *buffer_blk_y = nullptr; MemManager::allocMem<char>(buffer_blk_y, len_ch_blk_y); //read buffer
			DataWrapper<bool> y_blk(len_blk_y, len_ch_blk_y, buffer_blk_y);
			char *buffer_blk_gty = nullptr; MemManager::allocMem<char>(buffer_blk_gty, len_ch_blk_y); //read buffer
			DataWrapper<bool> gty_blk(len_blk_y, len_ch_blk_y, buffer_blk_gty);
			char *buffer_blk_gtxy = nullptr; MemManager::allocMem<char>(buffer_blk_gtxy, len_ch_blk_y); //write buffer
			DataWrapper<bool> gtxy_blk(len_blk_y, len_ch_blk_y, buffer_blk_gtxy);

			MyIO<char>::read(y_blk.getData(), std::string(_fname_blocks[i]), len_ch_blk_y - 1, std::ios_base::in | std::ios_base::binary, 0);
			y_blk.set(len_blk_y - 1, 0); //the sentinel is not included in the file for y, we must append it. 
			MyIO<char>::read(gty_blk.getData(), std::string("gt_blk" + std::to_string(i) + ".dat"), len_ch_blk_y, std::ios_base::in | std::ios_base::binary, 0);

			gtxy_blk.set(len_blk_y - 1, 0); //the rightmost character is the sentinel and it is smaller than x[0]
			uint_type k = 1; // the rightmost character in y is larger than the rightmost character in x and smaller than other characteres in x, thus k = 1.
			++_gap[k]; //compute gap		
			for (int64 j = len_blk_y - 2; j >= 0; --j) {
				k = (y_blk.get(j) ? (len_blk_x - _cnt) : 1) + rankBWT(y_blk.get(j), k, _bwt, _c, _leftmost_rank); // _x[len_blk_x - 1] is the sentinel and must not be equal to any characters in y.
				if (k > _leftmost_rank) gtxy_blk.set(j, 1); //if k <= _leftmost_rank, then S < Y, we have gt = 0
				else gtxy_blk.set(j, 0);
				++_gap[k]; //compute gap		
			}
			MyIO<char>::write(gtxy_blk.getData(), std::string("gt_blk" + std::to_string(i) + ".dat"), len_ch_blk_y, std::ios_base::out | std::ios_base::binary, 0);
			MemManager::deAllocMem<char>(buffer_blk_y, len_ch_blk_y);
			MemManager::deAllocMem<char>(buffer_blk_gty, len_ch_blk_y);
			MemManager::deAllocMem<char>(buffer_blk_gtxy, len_ch_blk_y);
		}
		MyIO<etype>::write(_gap, std::string("gap_blk" + std::to_string(_id_block) + ".dat"), _num_gap, std::ios_base::out | std::ios_base::binary, 0);
	}

#ifdef _IC_DEBUG
	int64 sum_gtxy = 0;
	for (int64 j = _num_blocks - 1; j > _id_block; --j) {
		uint_type len_ch_blk_y = info[j].len_ch_blocks;
		uint_type len_blk_y = info[j].len_blocks;
		char *test_buffer_blk_gtxy = nullptr; MemManager::allocMem<char>(test_buffer_blk_gtxy, len_ch_blk_y); //write buffer
		DataWrapper<bool> gtxy_blk(len_blk_y, len_ch_blk_y, test_buffer_blk_gtxy);
		MyIO<char>::read(gtxy_blk.getData(), std::string("gt_blk" + std::to_string(j) + ".dat"), len_ch_blk_y, std::ios_base::out | std::ios_base::binary, 0);
		std::cout << "gtxy: \n";
		for (int64 h = 0; h < len_blk_y; ++h)
			std::cout << gtxy_blk.get(h) << " ";
		std::cin.get();
		delete test_buffer_blk_gtxy;
	}

	std::cin.get();
#endif


#ifdef _IC_DEBUG
	std::cout << "gap:\n";
	for (int32 j = 0; j < _num_gap; ++j) {
		std::cout << _gap[j] << " ";
	}
	std::cin.get();
#endif
}


wtl::uint_type wtl::rankBWT(bool _ch, uint_type _k, DataWrapper<bool> & _bwt, etype * _c, uint_type _leftmost_rank) {
	uint_type rank = 0;
	uint_type idx = _k / K1; //want to know number of _ch in [0, _k)  

							 //idx 
	uint_type tmp = _c[idx];
	rank += (idx == 0) ? 0 : (_ch ? tmp : idx * K1 - tmp); //if ch == 0 && _leftmost_rank < _k, contain the preceding character of x[0], where the preceding character belongs to the neighboring block.

	for (int64 i = idx * K1; i < _k; ++i) rank = (_bwt.get(i) == _ch) ? rank + 1 : rank; // if ch == 0 && _leftmost_rank < _k, contain the preceding character of x[0], where the preceding character belongs to the neighboring block.

	rank = (_leftmost_rank < _k && !_ch) ? rank - 1 : rank; // the preceding character of x[0] is the sentinel (represented by character 0), which belongs to the block to be scanned.

	return rank;
}

void wtl::mergeIndex(uint_type _total_num, int32 _num_blocks, BlockInfo * _info) {

	char fill_file_ch = '1';
	MyIO<char>::write(&fill_file_ch, std::string("merge_bwt.dat"), 1, std::ios_base::out | std::ios_base::binary, 0);
	MyIO<char>::write(&fill_file_ch, std::string("merge_c.dat"), 1, std::ios_base::out | std::ios_base::binary, 0);
	MyIO<char>::write(&fill_file_ch, std::string("merge_e.dat"), 1, std::ios_base::out | std::ios_base::binary, 0);
	MyIO<char>::write(&fill_file_ch, std::string("merge_d.dat"), 1, std::ios_base::out | std::ios_base::binary, 0);

	//reading partial bwt and gap.
	//uint_type read_bufsize = MemManager::getAvail() / 2 / _num_blocks / (sizeof(char) + sizeof(uint_type) * 8)  * 8; // the half of the remaining memory is allocated
	uint_type read_bufsize_bdg = SBUFSIZE * 8 / 512; //bwt, gap and d
	uint_type read_bufsize_e = SBUFSIZE / 512; //e
	uint_type write_bufsize_bd = SBUFSIZE * 8 / 512; //bwt and d
	uint_type write_bufsize_e = SBUFSIZE / 512; //e
	uint_type write_bufsize_c = SBUFSIZE / 512; //c

	uint_type * sentinel_pos = new uint_type[_num_blocks + 2];

	//buffers for reading bwt. 
	char *data_buf_bwt = nullptr; MemManager::allocMem<char>(data_buf_bwt, _num_blocks * read_bufsize_bdg / 8);
	DataWrapper<bool> **buf_bwt = new DataWrapper<bool>*[_num_blocks];
	for (int32 i = 0; i < _num_blocks; ++i) buf_bwt[i] = new DataWrapper<bool>(read_bufsize_bdg, read_bufsize_bdg / 8, data_buf_bwt + i * read_bufsize_bdg / 8);

	//buffers for reading gap.
	etype *data_buf_gap = nullptr; MemManager::allocMem<etype>(data_buf_gap, _num_blocks * read_bufsize_bdg);
	etype **buf_gap = new etype*[_num_blocks];
	for (int32 i = 0; i < _num_blocks; ++i) buf_gap[i] = data_buf_gap + i * read_bufsize_bdg;

	//buffers for reading d.
	char *data_buf_d = nullptr; MemManager::allocMem<char>(data_buf_d, _num_blocks * read_bufsize_bdg / 8);
	DataWrapper<bool> **buf_d = new DataWrapper<bool>*[_num_blocks];
	for (int32 i = 0; i < _num_blocks; ++i) buf_d[i] = new DataWrapper<bool>(read_bufsize_bdg, read_bufsize_bdg / 8, data_buf_d + i * read_bufsize_bdg / 8);

	//buffers for reading e.
	etype *data_buf_e = nullptr; MemManager::allocMem<etype>(data_buf_e, _num_blocks * read_bufsize_e);
	etype **buf_e = new etype*[_num_blocks];
	for (int32 i = 0; i < _num_blocks; ++i) buf_e[i] = data_buf_e + i * read_bufsize_e;

	//
	bool *sentinel_found = new bool[_num_blocks];
	for (int32 i = 0; i < _num_blocks; ++i) sentinel_found[i] = false;

	//the cursor indicating the element to be used in the buffers for bwt, gap and d.
	uint_type *read_cur_bdg = new uint_type[_num_blocks];
	for (int32 i = 0; i < _num_blocks; ++i) read_cur_bdg[i] = 0;

	//the cursor indicating the element to be used in the buffers for e.
	uint_type *read_cur_e = new uint_type[_num_blocks];
	for (int32 i = 0; i < _num_blocks; ++i) read_cur_e[i] = 0;

	//the number of elements in the buffers for bwt, gap and d.
	uint_type *read_len_bdg = new uint_type[_num_blocks];
	for (int32 i = 0; i < _num_blocks; ++i) read_len_bdg[i] = 0;

	//the number of elements in the buffers for e.
	uint_type *read_len_e = new uint_type[_num_blocks];
	for (int32 i = 0; i < _num_blocks; ++i) read_len_e[i] = 0;

	//the number of elements to be read for bwt, gap and d.
	uint_type *remain_to_read_bdg = new uint_type[_num_blocks];
	for (int32 i = 0; i < _num_blocks; ++i) remain_to_read_bdg[i] = _info[i].len_blocks; //_info[i].len_blocks - 1 is an integral multiple of 8. The number of elements in gap is _info[i].len_blocks + 1, but the final element doesn't take effect in the process of merging bwt.

																						 //the number of elements to be read for e.
	uint_type *remain_to_read_e = new uint_type[_num_blocks];
	for (int32 i = 0; i < _num_blocks; ++i) remain_to_read_e[i] = _info[i].num_sample_e;

	//the number of elements have been read for bwt, gap and d.
	uint_type *prev_read_bdg = new uint_type[_num_blocks];
	for (int32 i = 0; i < _num_blocks; ++i) prev_read_bdg[i] = 0;

	//the number of elements have been read for e.
	uint_type *prev_read_e = new uint_type[_num_blocks];
	for (int32 i = 0; i < _num_blocks; ++i) prev_read_e[i] = 0;

	//buffer for merge_bwt.
	char *data_buf_merge_bwt = nullptr; MemManager::allocMem<char>(data_buf_merge_bwt, write_bufsize_bd / 8);
	DataWrapper<bool> buf_merge_bwt(write_bufsize_bd, write_bufsize_bd / 8, data_buf_merge_bwt);

	//buffer for merge_d
	char *data_buf_merge_d = nullptr; MemManager::allocMem<char>(data_buf_merge_d, write_bufsize_bd / 8);
	DataWrapper<bool> buf_merge_d(write_bufsize_bd, write_bufsize_bd / 8, data_buf_merge_d);
	uint_type write_cur_bd = 0;
	uint_type remain_to_write_bd = _total_num;
	uint_type prev_written_bd = 0;

	//buffer for merge_e
	etype *buf_merge_e = nullptr; MemManager::allocMem<etype>(buf_merge_e, write_bufsize_e); //!important: K2 < 255
	uint_type write_cur_e = 0;
	uint_type remain_to_write_e = 0; for (int32 i = 0; i < _num_blocks; ++i) remain_to_write_e += remain_to_read_e[i];
	uint_type prev_written_e = 0;

	//buffer for merge_c
	etype *buf_merge_c = nullptr; MemManager::allocMem<etype>(buf_merge_c, write_bufsize_c);
	uint_type write_cur_c = 0;
	uint_type remain_to_write_c = _total_num / K2 + (_total_num % K2 ? 1 : 0) + 1; //records number of 1 in [0,0), [0, K2), [0, 2* K2), .....[0, total_num -1)
	uint_type prev_written_c = 0;



	//read the first block
	for (int32 i = 0; i < _num_blocks; ++i) {
		read_len_bdg[i] = (remain_to_read_bdg[i] >= read_bufsize_bdg) ? read_bufsize_bdg : remain_to_read_bdg[i];
		MyIO<char>::read(buf_bwt[i]->getData(), std::string("bwt_blk" + std::to_string(i) + ".dat"), read_len_bdg[i] / 8 + (read_len_bdg[i] % 8 ? 1 : 0), std::ios_base::in | std::ios_base::binary, prev_read_bdg[i] / 8); //prev_read[i] must be an integral multiple of 8.
		MyIO<char>::read(buf_d[i]->getData(), std::string("d_blk" + std::to_string(i) + ".dat"), read_len_bdg[i] / 8 + (read_len_bdg[i] % 8 ? 1 : 0), std::ios_base::in | std::ios_base::binary, prev_read_bdg[i] / 8);
		MyIO<etype>::read(buf_gap[i], std::string("gap_blk" + std::to_string(i) + ".dat"), read_len_bdg[i], std::ios_base::in | std::ios_base::binary, prev_read_bdg[i] * sizeof(etype));
		remain_to_read_bdg[i] -= read_len_bdg[i];
		prev_read_bdg[i] += read_len_bdg[i];

		read_len_e[i] = (remain_to_read_e[i] >= read_bufsize_e) ? read_bufsize_e : remain_to_read_e[i];
		MyIO<etype>::read(buf_e[i], std::string("e_blk" + std::to_string(i) + ".dat"), read_len_e[i], std::ios_base::in | std::ios_base::binary, prev_read_e[i] * sizeof(etype));
		remain_to_read_e[i] -= read_len_e[i];
		prev_read_e[i] += read_len_e[i];
	}

	uint_type cnt_c = 0;
	//the first element in c array is 0, the write buffer for c must be larger than 1
	buf_merge_c[write_cur_c] = cnt_c;
	++write_cur_c;

	for (int64 i = 0; i < _total_num; ++i) {
		for (int32 j = 0; j < _num_blocks; ++j) {
			if (read_cur_bdg[j] < read_len_bdg[j] && buf_gap[j][read_cur_bdg[j]] == ETYPE_MIN) {
				bool cur_bwt = buf_bwt[j]->get(read_cur_bdg[j]);
				bool cur_d = buf_d[j]->get(read_cur_bdg[j]);
				//record bwt
				buf_merge_bwt.set(write_cur_bd, cur_bwt);
				//record d 
				buf_merge_d.set(write_cur_bd, cur_d);
				//compute sentinel pos
				if (!sentinel_found[j] && (_info[j].sentinel_pos == (prev_read_bdg[j] - read_len_bdg[j] + read_cur_bdg[j]))) {
					sentinel_pos[(_num_blocks + j - 1) % _num_blocks] = i;
					sentinel_found[j] = true;
					std::cerr << "the block id: "<<j<< "merged phase: relative_sentinel_pos: " << prev_read_bdg[j] - read_len_bdg[j] + read_cur_bdg[j] << " absolute_sentinel_pos: "<< i<< std::endl;
				}
				++write_cur_bd; ++read_cur_bdg[j];
				
				//compute e
				if (cur_d) {
					uint_type cur_pos = buf_e[j][read_cur_e[j]];
					buf_merge_e[write_cur_e] = cur_pos;
					++write_cur_e; ++read_cur_e[j];
				}

				//compute c
				if (cur_bwt) {
					++cnt_c;
				}
				if (i % K2 == K2 - 1 || i == _total_num - 1) {
					buf_merge_c[write_cur_c] = cnt_c;
					++write_cur_c;
				}

				// read next part of bwt, gap and d
				if (read_cur_bdg[j] == read_len_bdg[j] && remain_to_read_bdg[j]) { //all elements have been visited and there's more elements to be visited in the external memory
					read_len_bdg[j] = (remain_to_read_bdg[j] >= read_bufsize_bdg) ? read_bufsize_bdg : remain_to_read_bdg[j];
					MyIO<etype>::read(buf_gap[j], std::string("gap_blk" + std::to_string(j) + ".dat"), read_len_bdg[j], std::ios_base::in | std::ios_base::binary, prev_read_bdg[j] * sizeof(etype));
					MyIO<char>::read(buf_bwt[j]->getData(), std::string("bwt_blk" + std::to_string(j) + ".dat"), read_len_bdg[j] / 8 + (read_len_bdg[j] % 8 ? 1 : 0), std::ios_base::in | std::ios_base::binary, prev_read_bdg[j] / 8);
					MyIO<char>::read(buf_d[j]->getData(), std::string("d_blk" + std::to_string(j) + ".dat"), read_len_bdg[j] / 8 + (read_len_bdg[j] % 8 ? 1 : 0), std::ios_base::in | std::ios_base::binary, prev_read_bdg[j] / 8);
					remain_to_read_bdg[j] -= read_len_bdg[j];
					prev_read_bdg[j] += read_len_bdg[j];
					read_cur_bdg[j] = 0;
				}

				//read next part of e
				if (read_cur_e[j] == read_len_e[j] && remain_to_read_e[j]) { //all elements have been visited and there's more elements to be visited in the external memory
					read_len_e[j] = (remain_to_read_e[j] >= read_bufsize_e) ? read_bufsize_e : remain_to_read_e[j];
					MyIO<etype>::read(buf_e[j], std::string("e_blk" + std::to_string(j) + ".dat"), read_len_e[j], std::ios_base::in | std::ios_base::binary, prev_read_e[j] * sizeof(etype));
					remain_to_read_e[j] -= read_len_e[j];
					prev_read_e[j] += read_len_e[j];
					read_cur_e[j] = 0;
				}

				//flush write buffers for bwt and d
				if (write_cur_bd == write_bufsize_bd) { // the buffer is full, flush it.
					MyIO<char>::write(buf_merge_bwt.getData(), std::string("merge_bwt.dat"), write_cur_bd / 8 + (write_cur_bd % 8 ? 1 : 0), std::ios_base::in | std::ios_base::out | std::ios_base::binary, prev_written_bd / 8);
					MyIO<char>::write(buf_merge_d.getData(), std::string("merge_d.dat"), write_cur_bd / 8 + (write_cur_bd % 8 ? 1 : 0), std::ios_base::in | std::ios_base::out | std::ios_base::binary, prev_written_bd / 8);
					remain_to_write_bd -= write_bufsize_bd;
					prev_written_bd += write_bufsize_bd;
					write_cur_bd = 0;
				}

				//flush write buffer for e
				if (write_cur_e == write_bufsize_e) {
					MyIO<etype>::write(buf_merge_e, std::string("merge_e.dat"), write_cur_e, std::ios_base::in | std::ios_base::out | std::ios_base::binary, prev_written_e * sizeof(etype));
					remain_to_write_e -= write_bufsize_e;
					prev_written_e += write_bufsize_e;
					write_cur_e = 0;
				}

				//flush write buffer for c
				if (write_cur_c == write_bufsize_c) {
					MyIO<etype>::write(buf_merge_c, std::string("merge_c.dat"), write_cur_c, std::ios_base::in | std::ios_base::out | std::ios_base::binary, prev_written_c * sizeof(etype));
					remain_to_write_c -= write_bufsize_c;
					prev_written_c += write_bufsize_c;
					write_cur_c = 0;
				}
				break;
			}
			else {
				--buf_gap[j][read_cur_bdg[j]];
			}
		}
	}

	//flush write buffers of bwt and d.
	MyIO<char>::write(buf_merge_bwt.getData(), std::string("merge_bwt.dat"), write_cur_bd / 8 + (write_cur_bd % 8 ? 1 : 0), std::ios_base::in | std::ios_base::out | std::ios_base::binary, prev_written_bd / 8);
	MyIO<char>::write(buf_merge_d.getData(), std::string("merge_d.dat"), write_cur_bd / 8 + (write_cur_bd % 8 ? 1 : 0), std::ios_base::in | std::ios_base::out | std::ios_base::binary, prev_written_bd / 8);
	remain_to_write_bd -= write_cur_bd;
	prev_written_bd += write_cur_bd;
	write_cur_bd = 0;

	//flush write buffer for e
	MyIO<etype>::write(buf_merge_e, std::string("merge_e.dat"), write_cur_e, std::ios_base::in | std::ios_base::out | std::ios_base::binary, prev_written_e * sizeof(etype));
	remain_to_write_e -= write_cur_e;
	prev_written_e += write_cur_e;
	write_cur_e = 0;

	//flush write buffer for c
	MyIO<etype>::write(buf_merge_c, std::string("merge_c.dat"), write_cur_c, std::ios_base::in | std::ios_base::out | std::ios_base::binary, prev_written_c * sizeof(etype));
	remain_to_write_c -= write_cur_c;
	prev_written_c += write_cur_c;
	write_cur_c = 0;

	sentinel_pos[_num_blocks] = _num_blocks;
	sentinel_pos[_num_blocks+1] = _total_num;
	MyIO<uint_type>::write(sentinel_pos, std::string("merge_block_num.dat"), _num_blocks+2, std::ios_base::binary | std::ios_base::out, 0);

	//test
	/*std::cout << "sentinel found: \n";
	for (int i = 0; i < _num_blocks; ++i)
		std::cout << sentinel_found[i] << std::endl;
	std::cin.get();*/

	delete[] sentinel_pos;
	delete[] sentinel_found;
	delete[] buf_bwt;
	delete[] buf_gap;
	delete[] buf_d;
	delete[] buf_e;
	delete[] read_cur_bdg;
	delete[] read_cur_e;
	delete[] read_len_bdg;
	delete[] read_len_e;
	delete[] remain_to_read_bdg;
	delete[] remain_to_read_e;
	delete[] prev_read_bdg;
	delete[] prev_read_e;
	MemManager::deAllocMem<char>(data_buf_bwt, _num_blocks * read_bufsize_bdg / 8);
	MemManager::deAllocMem<char>(data_buf_d, _num_blocks * read_bufsize_bdg / 8);
	MemManager::deAllocMem<etype>(data_buf_gap, _num_blocks * read_bufsize_bdg);
	MemManager::deAllocMem<etype>(data_buf_e, _num_blocks * read_bufsize_bdg);
	MemManager::deAllocMem<char>(data_buf_merge_bwt, write_bufsize_bd / 8);
	MemManager::deAllocMem<char>(data_buf_merge_d, write_bufsize_bd / 8);
	MemManager::deAllocMem<etype>(buf_merge_c, write_bufsize_c);
	MemManager::deAllocMem<etype>(buf_merge_e, write_bufsize_e);

}