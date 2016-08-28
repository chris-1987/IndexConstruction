/***************************************************************************
*  Copyright (C) 2016 Yi Wu and Mengmeng Guo <wu.yi.christian@gmail.com>
*  Description: #include "indexconstruction.h"
*  Usage: The input string consists of multiple blocks {B1, B2,..., Bk} (|Bi| <= 10G an k <= 10), compute the general suffix array of T = B1$B2$....Bk$. 
*  Assumption1 (important) : The alphabet of the input string is {0,1}. Each character is represented by one bit (0 or 1). We combine every 8 successive characters as a whole and store them in a char.
*  Assumption2 (important) : For computation convenience, the number of characters in each string is an integral multiple of 8.
*  Assumption3 (important) : We append a character $ (represented by character 0) to each Bi and concatenate them to T. We logically append a character # to the end of T. Note that the lexical order of the characters satisfies # < $ < 0 < 1, and for any two $s appending to Di and Dj, we have the former is smaller than the later if and only if i < j.
**************************************************************************/


#ifndef _INDEX_CONSTRUCTION_H
#define _INDEX_CONSTRUCTION_H

#include <fstream>
#include <iostream>

#include "saism.h"

namespace wtl {

struct BlockInfo {
	BlockInfo(uint_type _len_ch_blocks = 0, uint_type _len_blocks = 0, uint_type start_offset = 0, uint_type _num_sample_e = 0,uint_type sentinel_pos = 0)
		: len_ch_blocks(_len_ch_blocks), len_blocks(_len_blocks), start_offset(start_offset), num_sample_e(_num_sample_e){};

	uint_type len_ch_blocks;
	uint_type len_blocks;
	uint_type start_offset;
	uint_type num_sample_e;
	uint_type sentinel_pos;
};

//@usage: given a set of document files, compute the general FM-index.
//@param_in: 
//          _fname_blocks: the file names of the blocks.
//          _num_blocks: the number of characters in the blocks.
void computeIndex(std::string * &_fname_blocks, int32 _num_blocks);

//@usage: for the string x given block Bi, compute the bwt, c, cnt, leftmost_rank and the partial gtxy.
//@param_in:
//			_x: the input string, where the last character is an appending character for 
//          _sa: the suffix array of _x.
//			_num_x: the number of elements in _x and _sa.
//          _num_c: the size of the _c array.
//@param_out:
//          _bwt: the burrows-wheeler transform for _x.
//			_cnt: an counter for counting the number of 1 characters in _x.
//			_c: an counter array. We split bwt into multiple fixed-size blocks from left to right. For the i-th block, _c[i] represents the number of 1 characters in the previous blocks.
//			_leftmost_rank: the parameter represents the index position of the suffix starting at _x[0] in sa.
//          _id_block:  the index of the block, starting from 0.
void computeBWTandPartialGTXY(DataWrapper<bool> & _x, etype * _sa, DataWrapper<bool> & _bwt, etype * _c, uint_type _num_c, uint_type & _cnt, uint_type & _leftmost_rank, int32 _id_block, uint_type _total_num, BlockInfo * _info);

//@usage: for the string x given in block Bi, compute gtxy[n_x, n_x + n_y) and gap[0, n_x).
//@param_in:
//			_x: the block to be processed.
//			_bwt: the bwt of _x.
//			_num_x: the number of characters in _x.
//			_c: an counter array. We split bwt into multiple fixed-size blocks from left to right. For the i-th block, _c[i] represents the number of 1 characters in the previous blocks.
//			_num_c: the size of _c.
//			_cnt: an counter. This parameter represents the number of 1 characters in x.
//			_leftmost_rank: the parameter represents the index position of the suffix starting at x[0] in sa. 
//@param_out:
//			_id_block: the index of the block, start from 0.
//			_gap: the gap array. The i-th element in the array stores the number suffixes (starting at y) that is greater than the one starting at X[i - 1] and smaller than the one starting at X[i].
void computePartialGTXYandGap(DataWrapper<bool> & _x, DataWrapper<bool> & _bwt, etype * _c, uint_type _num_c, uint_type _cnt, etype * _gap, uint_type _num_gap, int32 _id_block, int32 _num_blocks, BlockInfo * info, uint_type _leftmost_rank, std::string * _fname_blocks);

//@usage: compute sufrank(chS).
//@param_in:
//			_ch: the character immediately to the leftside of S.
//			_k: the rank of S in the suffix array of x.
//			_bwt: the bwt of x.
//			_c: an counter array. Split bwt into multiple sub-blocks from left to right. For the i-th sub-block, _c[i] represents the number of 1 characters in previous blocks.
//			_leftmost_rank: This parameter represents the index position of the suffix starting at x[0] in sa_s. 
//@param_out:
//			return value: the return value is the suffix array of chS in the suffix array of x.
uint_type rankBWT(bool _ch, uint_type _k, DataWrapper<bool> & _bwt, etype * _c, uint_type _leftmost_rank);



//@usage: merge the partial bwt arrays according to the given gap arrays.
/*
* _fn_arr_bwt: filename array for partial bwt.
* _fn_arr_gap: filename array for partial gap.
* _fn_bwt_merge: filename for merged bwt.
*/
void mergeIndex(uint_type _total_num_bit, int32 _blknum, BlockInfo * _info);

}//namespace wtl
#endif
