#ifndef _BUILDER_H
#define _BUILDER_H

#include "common.h"
#include "io.h"
#include "sais.h"
#include "tuple.h"
#include "tuple_sorter.h"
#include "vector.h"
#include "sorter.h"
#include "pq_sub.h"
#include "pq_suf.h"

#include <string>
#include <fstream>
#include <cassert>


#define DEBUG_TEST4
//#define DEBUG_TEST3

//#define DEBUG_TEST2
//#define DEBUG_TEST1

#define STATISTICS_COLLECTION


template<typename alphabet_type, typename offset_type>
class DSAComputation;

/// \brief preprocess 
///
template<typename alphabet_type, typename offset_type>
class DSAIS{

private:
	typedef typename ExVector<alphabet_type>::vector alphabet_vector_type;

	typedef typename ExVector<offset_type>::vector offset_vector_type;

	typedef MyVector<alphabet_type> my_alphabet_vector_type;

	typedef MyVector<offset_type> my_offset_vector_type;

private:

	const std::string m_s_fname; ///< fname for input string

	const std::string m_sa_fname; ///< fname for suffix array

public:

	/// \brief ctor
	///
	DSAIS(const std::string & _s_fname, const std::string & _sa_fname) : m_s_fname(_s_fname), m_sa_fname(_sa_fname) {}

	/// \brief run
	///
	void run() {

		// append a sentinel
		stxxl::syscall_file *s_file = new stxxl::syscall_file(m_s_fname, stxxl::syscall_file::RDWR | stxxl::syscall_file::DIRECT);

		alphabet_vector_type *s_origin = new alphabet_vector_type(s_file);

		typename alphabet_vector_type::bufreader_type s_origin_reader(*s_origin);

		my_alphabet_vector_type *s_target = new my_alphabet_vector_type();

		uint64 s_origin_len = s_origin->size();

		for (; !s_origin_reader.empty(); ++s_origin_reader) {

			s_target->push_back(*s_origin_reader);
		}

		s_target->push_back(alphabet_type(0));

#ifdef STATISTICS_COLLECTION

		Logger::addIV(s_origin_len * sizeof(alphabet_type)); // read from s_origin (stxxl)
#endif

#ifdef DEBUG_TEST3

		std::cerr << "target s:";

		s_target->start_read();

		while (!s_target->is_eof()) {

			std::cerr << (uint32)s_target->get() << " ";

			s_target->next();
		}

		std::cerr << "\n";
#endif


		// clear
		delete s_origin; s_origin = nullptr;

		delete s_file; s_file = nullptr;

		// compute sa_reverse
		my_offset_vector_type *sa_reverse = nullptr;

		DSAComputation<alphabet_type, offset_type> dsac(s_target, 0, sa_reverse);

		dsac.run();

		// clear
		delete s_target; s_target = nullptr;

		// produce sa from sa_reverse (notice: pdu remains unchanged)
		stxxl::syscall_file *sa_file = new stxxl::syscall_file(m_sa_fname, stxxl::syscall_file::CREAT | stxxl::syscall_file::RDWR | stxxl::syscall_file::DIRECT);

		offset_vector_type *sa = new offset_vector_type(sa_file); 

		sa->resize(s_origin_len);

		typename offset_vector_type::bufwriter_type sa_writer(*sa);

		sa_reverse->start_read_reverse(); // start read reversely

		sa_reverse->next_remove_reverse(); // skip the sentinel

		while (!sa_reverse->is_eof()) {

			sa_writer << sa_reverse->get_reverse();

			sa_reverse->next_remove_reverse();
		}

		sa_writer.finish();

#ifdef STATISTICS_COLLECTION

		Logger::addOV(s_origin_len * sizeof(offset_type)); // pdu remains unchanged but OV keep increasing for generating sa (stxxl)
#endif

		// clear
		delete sa; sa = nullptr;

		delete sa_file; sa_file = nullptr;

		delete sa_reverse; sa_reverse = nullptr;

#ifdef STATISTICS_COLLECTION

		Logger::report(s_origin_len);
#endif

		return;		
	}
};

template<typename alphabet_type, typename offset_type>
class DSAComputation{

private:

	typedef MyVector<alphabet_type> alphabet_vector_type;

	typedef MyVector<offset_type> offset_vector_type;

	typedef MyVector<bool> bool_vector_type;

private:

	const alphabet_type ALPHA_MAX;

	const alphabet_type ALPHA_MIN;

	const offset_type OFFSET_MAX;

	const offset_type OFFSET_MIN;

private:

	struct BlockInfo{

	public:

		uint32 m_capacity; ///< capacity for multi-block

		uint64 m_beg_pos; ///< starting position, global

		uint64 m_end_pos; //< ending position, global

		uint64 m_size; ///< number of characters in the block, non-multi-block may exceed the capacity

		uint32 m_lms_num; ///< number of S*-substrs in the block

		uint8 m_id; ///< id

	public:

		/// \brief ctor
		///
		BlockInfo(const uint64 & _end_pos, const uint8 _id) : m_end_pos(_end_pos), m_id(_id) {

			if (sizeof(alphabet_type) <= sizeof(uint32)) {

				double div = sizeof(alphabet_type) + double(1 / 8) + sizeof(uint32) + sizeof(uint32);

				m_capacity = MAX_MEM / div;
			}
			else {

				double div = std::max(sizeof(alphabet_type) + double(1 / 8) + sizeof(uint32) + sizeof(uint32) + sizeof(uint32), 
					double(sizeof(alphabet_type)) + sizeof(alphabet_type) + sizeof(uint32) + sizeof(uint32));
		
				m_capacity = MAX_MEM / div;
			}

			std::cerr << "m_capacity: " << m_capacity << std::endl;

			m_size = 0;

			m_lms_num = 0;
		}

		/// \brief assign copy
		///
		BlockInfo& operator=(const BlockInfo & _bi) {

			m_capacity = _bi.m_capacity;

			m_beg_pos = _bi.m_beg_pos;

			m_end_pos = _bi.m_end_pos;

			m_size = _bi.m_size;

			m_lms_num = _bi.m_lms_num;

			m_id = _bi.m_id;

			return *this;
		}


		/// \brief check if the block is empty
		///
		bool is_empty() const {

			return m_size == 0;
		}

		/// \brief check if the block contains more than one S*-substr
		///
		bool is_multi() const {

			return m_lms_num > 1;
		}

		/// \brief check if the block contains only single S*-substr
		///
		bool is_single() const {

			return m_lms_num == 1;
		}

		/// \brief check if the block can afford the S*-substr of a certain size
		///
		/// \note if the block is empty, it can afford an S*-substr of any size
		bool try_fill(const uint64 _lms_size) {

			if (is_empty() || m_size + _lms_size - 1 <= m_capacity) {

				return true;
			}

			return false;
		}

		/// \brief fill the S*-substr into the block
		///
		/// \note If empty, insert all the characters; otherwise, insert all but the last character
		void fill(const uint64 _lms_size) {

			if (is_empty()) {

				m_size = _lms_size;
			}
			else {

				m_size = m_size + _lms_size - 1;
			}

			++m_lms_num;

			return;
		}
		
		/// \brief close the block
		///
		void close() {

			m_beg_pos = m_end_pos - m_size + 1;

			return;
		}
	};

private:

	std::vector<BlockInfo> m_blocks_info;

	std::vector<uint8> m_block_id_of_samplings;

	alphabet_vector_type *& m_s;

	const uint64 m_s_len;

	const uint32 m_level;

	offset_vector_type *& m_sa_reverse;

	std::vector<alphabet_vector_type*> m_sub_l_bwt_seqs;

	std::vector<alphabet_vector_type*> m_sub_s_bwt_seqs;

	offset_vector_type *m_s1; ///< reduced string
	
	std::vector<offset_vector_type*> m_lms_name_seqs;

	std::vector<alphabet_vector_type*> m_suf_l_bwt_seqs;

	std::vector<alphabet_vector_type*> m_suf_s_bwt_seqs;

public:

	DSAComputation(alphabet_vector_type *& _s, const uint32 _level, offset_vector_type *& _sa_reverse);

	void run();		 

	bool sortSStarGlobal();

	uint64 partitionS();

	void compute_block_id_of_samplings();

	uint8 getBlockId(const offset_type & _pos);
		
	void sortSStarBlock(const BlockInfo & _block_info);

	void sortSStarSingleBlock(const BlockInfo & _block_info);

	template<bool FORMAT>
	void sortSStarMultiBlock(const BlockInfo & _block_info);

	template<typename U>
	void getBuckets(const U * _s, const uint32 _s_size, uint32 * _bkt, const uint32 _bkt_num, const bool _end);

	void formatMultiBlock(const BlockInfo & _block_info, alphabet_type *_block, uint32 *_fblock, uint32 & _max_alpha);

	bool mergeSortedSStarGlobal();

	void sortSuffixGlobal(offset_vector_type *& _sa1_reverse);

	void sortSuffixSingleBlock(const BlockInfo & _block_info);

	void sortSuffixLeftmostBlock();

	template<bool FORMAT>
	void sortSuffixMultiBlock(const BlockInfo & _block_info);

	void mergeSortedSuffixGlobal();
};

/// \brief ctor
///
template<typename alphabet_type, typename offset_type>
DSAComputation<alphabet_type, offset_type>::DSAComputation(alphabet_vector_type *& _s, const uint32 _level, offset_vector_type *& _sa_reverse) : ALPHA_MAX(std::numeric_limits<alphabet_type>::max()), ALPHA_MIN(std::numeric_limits<alphabet_type>::min()), OFFSET_MAX(std::numeric_limits<offset_type>::max()), OFFSET_MIN(std::numeric_limits<offset_type>::min()), m_s(_s), m_s_len(m_s->size()), m_level(_level), m_sa_reverse(_sa_reverse){}

/// \brief run
///
/// execute the body 
template<typename alphabet_type, typename offset_type>
void DSAComputation<alphabet_type, offset_type>::run() {

	// sort S*-substrs
	bool is_unique = sortSStarGlobal();

#ifdef DEBUG_TEST3

	std::cerr << "m_s1:\n";

	m_s1->start_read();

	while (!m_s1->is_eof()) {

		std::cerr << m_s1->get() << " ";

		m_s1->next();
	}
#endif

	offset_vector_type *sa1_reverse = nullptr;

#ifdef DEBUG_TEST4

	std::cerr << "level: " << m_level << std::endl; 

	std::cerr << "unique: " << is_unique << std::endl;
#endif

	// check recursion condition
	if (is_unique == false) {

		if (MAX_MEM >= m_s1->size() * (sizeof(uint32) + sizeof(uint32) + sizeof(uint32) + (double)1 / 8)) {

			SAIS<offset_type>(m_s1, sa1_reverse);
		}
		else {

			DSAComputation<offset_type, offset_type> dsac_recurse(m_s1, m_level + 1, sa1_reverse);

			dsac_recurse.run();
		}
	}

	// sort suffixes
	sortSuffixGlobal(sa1_reverse);
}

/// \brief sort S*-substrs
///
template<typename alphabet_type, typename offset_type>
bool DSAComputation<alphabet_type, offset_type>::sortSStarGlobal() {

	uint64 lms_num = partitionS();
	
	if (lms_num == 0) {

		m_s1 = new offset_vector_type();

		m_s1->push_back(OFFSET_MIN);

		return true;
	}

	m_s->start_read_reverse(); // read s reversely to process the blocks from right to left

#ifdef DEBUG_TEST4
	std::cerr << "lms_num: " << lms_num << std::endl;
#endif

	for (uint8 i = 0; i < m_blocks_info.size() - 1; ++i) { // no need to process the leftmost block, cause it contains no S*-type substrs

		sortSStarBlock(m_blocks_info[i]);
	}


#ifdef DEBUG_TEST3

	for (uint8 i = 0; i < m_blocks_info.size() - 1; ++i) {

		std::cerr << "block id: " << (uint32) i << std::endl;
	
		std::cerr << "bwt_l: \n";

		m_sub_l_bwt_seqs[i]->start_read();

		while (!m_sub_l_bwt_seqs[i]->is_eof()) {

			std::cerr << (uint32)m_sub_l_bwt_seqs[i]->get() << " ";

			m_sub_l_bwt_seqs[i]->next();
		}

		std::cerr << std::endl;

		std::cerr << "bwt_s: \n";

		m_sub_s_bwt_seqs[i]->start_read();

		while (!m_sub_s_bwt_seqs[i]->is_eof()) {

			std::cerr << (uint32)m_sub_s_bwt_seqs[i]->get() << " ";

			m_sub_s_bwt_seqs[i]->next();
		}

		std::cerr << std::endl;
	}

	
#endif

	bool is_unique = mergeSortedSStarGlobal();

	return is_unique;	
}

/// \brief partition s into blocks
///
/// \return number of S*-substrs 
template<typename alphabet_type, typename offset_type>
uint64 DSAComputation<alphabet_type, offset_type>::partitionS() {

	std::cerr << "partition is started\n";

	// scan s leftward to find all the S*-substrs
	alphabet_type cur_ch, last_ch;

	uint8 cur_t, last_t;

	uint64 lms_end_pos, lms_size;
	
	uint64 lms_num = 0;

	m_s->start_read_reverse(); // prepare for reading s reversely

	lms_end_pos = m_s->size() - 1;

	BlockInfo block_info = BlockInfo(lms_end_pos, m_blocks_info.size());

	lms_size = 1, m_s->next_reverse(); // rightmost is the sentinel

	cur_ch = m_s->get_reverse(), cur_t = L_TYPE, ++lms_size, m_s->next_reverse(); // next on the left is L-type

	last_ch = cur_ch, last_t = cur_t;

	for (; !m_s->is_eof(); m_s->next_reverse()) {

		cur_ch = m_s->get_reverse();

		cur_t = (cur_ch < last_ch || (cur_ch == last_ch && last_t == S_TYPE)) ? S_TYPE : L_TYPE;

		if (cur_t == L_TYPE && last_t == S_TYPE) { // last_ch is S*-type, find an S*-substr

			++lms_num;

			if (block_info.try_fill(lms_size) == false) {

				block_info.close();

				m_blocks_info.push_back(block_info);

				block_info = BlockInfo(lms_end_pos, m_blocks_info.size());
			}

			block_info.fill(lms_size);

			lms_end_pos = lms_end_pos - lms_size + 1;

			lms_size = 1; // overlap an S*-character
		}

		++lms_size; // include current L-type

		last_ch = cur_ch, last_t = cur_t;
	}

	if (block_info.is_empty() == true) { // current block is empty, and there's no remaining S*-substrs, then it is leftmost block

		block_info.m_size = lms_end_pos + 1; // lms_end_pos - 0 + 1

		block_info.close();

		m_blocks_info.push_back(block_info);
	}
	else { // current block is not empty, close it and create a new block to carry the remaining characters

		block_info.close();

		m_blocks_info.push_back(block_info);

		block_info = BlockInfo(lms_end_pos, m_blocks_info.size());

		block_info.m_size = lms_end_pos + 1;

		block_info.close();

		m_blocks_info.push_back(block_info);
	}

	compute_block_id_of_samplings(); // compute block_id for samplings

	std::cerr << "partition is over\n";

	return lms_num;
}


/// \brief compute samplings' block_id
///
/// \note samplings are positioned at 0, 1 * m_capacity, 2 * m_capacity, ... m_s_len - 1
template<typename alphabet_type, typename offset_type>
void DSAComputation<alphabet_type, offset_type>::compute_block_id_of_samplings() {

	uint64 sampling_pos = 0;

	uint8 block_id = m_blocks_info.size() - 1;

	while (sampling_pos < m_s_len - 1) {

		while (true) {

			if (sampling_pos <= m_blocks_info[block_id].m_end_pos) {

				m_block_id_of_samplings.push_back(block_id);

				break;
			}

			--block_id;
		}

		sampling_pos += m_blocks_info[0].m_capacity;
	}

	sampling_pos = m_s_len - 1; // the sentinel is also a sampling

	while (true) {

		if (sampling_pos <= m_blocks_info[block_id].m_end_pos) {

			m_block_id_of_samplings.push_back(block_id);

			break;
		}

		--block_id;
	}

	return;
}

/// \brief return the block idx of the character at the given position
///
template<typename alphabet_type, typename offset_type>
uint8 DSAComputation<alphabet_type, offset_type>::getBlockId(const offset_type & _pos) {

	uint8 sampling_id = static_cast<uint8>(_pos / m_blocks_info[0].m_capacity); // _pos >= sampling_id * m_capacity

	uint8 block_id = m_block_id_of_samplings[sampling_id];

	while (true) {

		if (m_blocks_info[block_id].m_end_pos >= _pos) {

			return block_id;
		}

		--block_id;
	}

	std::cerr << "getBlockId is wrong\n";

	return 0;
}


/// \brief sort S*-substrs in the block
///
/// \note no need to process the leftmost block, because it contains no S*-substrs
template<typename alphabet_type, typename offset_type>
void DSAComputation<alphabet_type, offset_type>::sortSStarBlock(const BlockInfo & _block_info) {

	if (_block_info.is_single() == true) {

		sortSStarSingleBlock(_block_info);
	}

	if (_block_info.is_multi() == true) {

		if (sizeof(alphabet_type) <= sizeof(uint32)) {

			sortSStarMultiBlock<false>(_block_info);
		}
		else {

			sortSStarMultiBlock<true>(_block_info);
		}
	}

	return;
}

/// \brief contain single S*-substrs.
/// Record the preceding characters of [L...LS] when inducing L and those of [SS..L] when inducing S.
template<typename alphabet_type, typename offset_type>
void DSAComputation<alphabet_type, offset_type>::sortSStarSingleBlock(const BlockInfo & _block_info) {

	uint64 toread = _block_info.m_size;

	alphabet_type cur_ch, last_ch;

	// induce L	
	alphabet_vector_type *sub_l_bwt_seq = new alphabet_vector_type();

	last_ch = m_s->get_reverse(), m_s->next_reverse(), --toread; // rightmost is S*-type

	while (m_s->get_reverse() >= last_ch) { // if cur_ch is L-type

		cur_ch = m_s->get_reverse();

		sub_l_bwt_seq->push_back(cur_ch); // record the preceding of last_ch

		m_s->next_reverse(), --toread;

		last_ch = cur_ch;
	}

	sub_l_bwt_seq->push_back(m_s->get_reverse()); // push the preceding of the leftmost L-type

	// induce S
	alphabet_vector_type *sub_s_bwt_seq = new alphabet_vector_type();

	if (toread == 1) { 

		sub_s_bwt_seq->push_back(m_s->get_reverse()); // push the preceding of the leftmost L-type

		--toread; // do not perform m_s->next_reverse, because two successive blocks overlap the character
	}
	else {

		sub_s_bwt_seq->push_back(m_s->get_reverse()); // push the preceding of the leftmost L-type

		m_s->next_reverse(), --toread;

		while (toread > 1) {

			sub_s_bwt_seq->push_back(m_s->get_reverse());

			m_s->next_reverse(), --toread;
		}

		sub_s_bwt_seq->push_back(m_s->get_reverse());

		--toread; // do not perform m_s->next_reverse, because two successive blocks overlap the character
	}

	m_sub_l_bwt_seqs.push_back(sub_l_bwt_seq), m_sub_s_bwt_seqs.push_back(sub_s_bwt_seq);

	return;
} 

/// \brief contain multiple S*-substrs.
///
/// \note if alphabet_type > uint32, then format the block before inducing in RAM
template<typename alphabet_type, typename offset_type>
template<bool FORMAT>
void DSAComputation<alphabet_type, offset_type>::sortSStarMultiBlock(const BlockInfo & _block_info) {

	// load s into RAM, format s
	uint32 block_size = _block_info.m_size;

	alphabet_type *s = new alphabet_type[block_size];

	uint32 *fs = nullptr;

	uint32 max_alpha;

	if (FORMAT == true) {

		fs = new uint32[block_size];

		formatMultiBlock(_block_info, s, fs, max_alpha);
	}
	else {

		for (uint32 i = block_size - 1; i > 0; --i, m_s->next_reverse()) {

			s[i] = m_s->get_reverse();
		}
	
		s[0] = m_s->get_reverse(); // do not perform next_reverse, because two blocks overlap an S*-type character

		max_alpha = ALPHA_MAX;
	}

	char *t_buf = new char[block_size / 8 + 1]; BitWrapper t(t_buf); // L & S type array
	
	uint32 *sa = new uint32[block_size]; 

	for (uint32 i = 0; i < block_size; ++i) {

		sa[i] = UINT32_MAX;
	}

	uint32 *bkt = new uint32[max_alpha + 1];

	if (FORMAT == true) {
	
		getBuckets<uint32>(fs, block_size, bkt, max_alpha + 1, true);
	}
	else {
		
		getBuckets<alphabet_type>(s, block_size, bkt, max_alpha + 1, true);
	}

	// compute t and insert size-one lms
	uint32 lms_beg_pos, lms_end_pos, lms_size;

	lms_end_pos = block_size - 1, t.set(block_size - 1, S_TYPE), lms_size = 1; // rightmost is the sentinel

	if (FORMAT == true) {

		sa[bkt[fs[lms_end_pos]]--] = lms_end_pos;
	}
	else {

		sa[bkt[s[lms_end_pos]]--] = lms_end_pos;
	}

	t.set(block_size - 2, L_TYPE), ++lms_size; // next on the left is L-type

	for (uint32 i = block_size - 3; ; --i) {

		t.set(i, (s[i] < s[i + 1] || (s[i] == s[i + 1] && t.get(i + 1) == S_TYPE)) ? S_TYPE : L_TYPE);

		if (t.get(i) == L_TYPE && t.get(i + 1) == S_TYPE) { // s[i + 1] is S*-type

			lms_beg_pos = lms_end_pos - lms_size + 1;

			if (FORMAT == true) {

				sa[bkt[fs[lms_beg_pos]]--] = lms_beg_pos;
			}
			else {
			
				sa[bkt[s[lms_beg_pos]]--] = lms_beg_pos;
			}

			lms_end_pos = lms_beg_pos;

			lms_size = 1; // overlap an S*-character
		}

		++lms_size; // include current character

		if (i == 0) break;
	}

	// no need to insert the leftmost character (also S*-type) into sa, because it induces no preceding substr in current block
	
	// induce L-type and S-type substrs
	alphabet_vector_type *sub_l_bwt_seq = new alphabet_vector_type(), *sub_s_bwt_seq = new alphabet_vector_type();

	if (FORMAT == true) {

		getBuckets<uint32>(fs, block_size, bkt, max_alpha + 1, false);
	}
	else {

		getBuckets<alphabet_type>(s, block_size, bkt, max_alpha + 1, false);
	}

	for (uint32 i = 0; i < block_size; ++i) {

		if (sa[i] != UINT32_MAX) { // sa[i] != 0

			uint32 j = sa[i] - 1;

			sub_l_bwt_seq->push_back(s[j]); 

			if (t.get(j) == L_TYPE) {

				if (FORMAT == true) {

					sa[bkt[fs[j]]++] = j;
				}
				else {
		
					sa[bkt[s[j]]++] = j;
				}

				sa[i] = UINT32_MAX; // only reserve L*-type
			}
		}
	} 

	if (FORMAT == true) {
		
		getBuckets<uint32>(fs, block_size, bkt, max_alpha + 1, true);
	}
	else {
		
		getBuckets<alphabet_type>(s, block_size, bkt, max_alpha + 1, true);
	}

	for (uint32 i = block_size - 1; ; --i) {

		if (sa[i] != UINT32_MAX && sa[i] != 0) {

			uint32 j = sa[i] - 1;

			sub_s_bwt_seq->push_back(s[j]);

			if (t.get(j) == S_TYPE) {

				if (FORMAT == true) {

					sa[bkt[fs[j]]--] = j;
				}
				else {

					sa[bkt[s[j]]--] = j;
				}

				sa[i] = UINT32_MAX; // only reserve S*-type
			}
		}

		if (i == 0) break;
	}

	delete bkt; bkt = nullptr;

	// 
	m_sub_l_bwt_seqs.push_back(sub_l_bwt_seq), m_sub_s_bwt_seqs.push_back(sub_s_bwt_seq);

	delete [] s; s = nullptr;

	delete [] fs; fs = nullptr;

	delete [] t_buf; t_buf = nullptr;

	delete [] sa;

	return;

}

/// \brief get bucket end/start
///
template<typename alphabet_type, typename offset_type>
template<typename U>
void DSAComputation<alphabet_type, offset_type>::getBuckets(const U *_s, const uint32 _s_size, uint32 *_bkt, const uint32 _bkt_num, const bool _end) {

	uint32 sum = 0;

	for (uint32 i = 0; i < _bkt_num; ++i) {

		_bkt[i] = 0;
	}

	for (uint32 i = 0; i < _s_size; ++i) {

		++_bkt[_s[i]];
	}

	for (uint32 i = 0; i < _bkt_num; ++i) {

		sum += _bkt[i]; 
	
		_bkt[i] = _end ? (sum - 1) : (sum - _bkt[i]);
	}

	return;
}

/// \brief format block
///
template<typename alphabet_type, typename offset_type>
void DSAComputation<alphabet_type, offset_type>::formatMultiBlock(const BlockInfo & _block_info, alphabet_type *_block, uint32 *_fblock, uint32 & _max_alpha){

	uint32 block_size = _block_info.m_size;

	typedef Pair<alphabet_type, uint32> pair_type;

	typedef TupleAscCmp1<pair_type> pair_comparator_type;

	std::vector<pair_type> container(block_size);

	for (uint32 i = block_size - 1; i > 0; --i, m_s->next_reverse()) {

		container[i] = pair_type(m_s->get_reverse(), i);
	}

	container[0] = pair_type(m_s->get_reverse(), 0); // do not perform next_reverse, because two blocks overlap an S*-type character

	std::sort(container.begin(), container.end(), pair_comparator_type());

	alphabet_type pre_ch = container[0].first;

	uint32 pre_name = 0;

	_fblock[container[0].second] = pre_name;

	_block[container[0].second] = pre_ch;

	for (uint32 i = 1; i < block_size; ++i) {

		if (container[i].first != pre_ch) {

			pre_ch = container[i].first;

			pre_name = pre_name + 1;
		}

		_fblock[container[i].second] = pre_name;

		_block[container[i].second] = pre_ch;
	}	

	_max_alpha = pre_name;

	return;
}

/// \brief merge sorted S*-substrs in all the blocks
///
template<typename alphabet_type, typename offset_type>
bool DSAComputation<alphabet_type, offset_type>::mergeSortedSStarGlobal() {

	// sort the ending characters of S*-substrs
	typedef Pair<alphabet_type, offset_type> pair_type; // <ch, pos>
	
	typedef TupleAscCmp2<pair_type> pair_comparator_type; // sort in ascending order by the first two components
	
	typedef MySorter<pair_type, pair_comparator_type> sorter_type; // a sorter, sort elements in ascending order

	sorter_type *sorter_lms = new sorter_type(MAX_MEM);

	{
		uint8 cur_t, last_t;

		alphabet_type cur_ch, last_ch, lms_end_ch;

		uint64 lms_size, lms_end_pos;

		m_s->start_read_reverse();

		lms_end_pos = m_s->size() - 1, lms_end_ch = m_s->get_reverse(), lms_size = 1, m_s->next_reverse(); // rightmost is S*-type

		cur_ch = m_s->get_reverse(), cur_t = L_TYPE, ++lms_size, m_s->next_reverse(); // next on the left is L-type

		last_ch = cur_ch, last_t = cur_t;

		for (; !m_s->is_eof(); m_s->next_reverse()) {

			cur_ch = m_s->get_reverse();

			cur_t = (cur_ch < last_ch || (cur_ch == last_ch && last_t == S_TYPE)) ? S_TYPE : L_TYPE;

			if (cur_t == L_TYPE && last_t == S_TYPE) { // find an S*-substr

				sorter_lms->push(pair_type(lms_end_ch, lms_end_pos));

				lms_end_pos = lms_end_pos - lms_size + 1, lms_end_ch = last_ch, lms_size = 1; // overlap an S*-character
			}

			++lms_size; // include currently scanned character

			last_ch = cur_ch, last_t = cur_t;
		}

		sorter_lms->sort();

#ifdef DEBUG_TEST4
		std::cerr << "sorter size: " << sorter_lms->size() << std::endl;
#endif
	}	

	// induce L-type substrs
	typedef Triple<alphabet_type, offset_type, offset_type> triple_type; // <ch, rank, pos>

	typedef TupleDscCmp3<triple_type> triple_comparator_type; // compare two by their first 3 components in descending order

	typedef PQL_SUB<alphabet_type, offset_type, triple_type, triple_comparator_type> heap_type; // min-heap

	heap_type *pq_l = new heap_type(MAX_MEM); // sorter only maintains a O(n/M) heap in RAM

	alphabet_vector_type *sorted_l_ch = new alphabet_vector_type(); // heading chars for sorted lml

	offset_vector_type *sorted_l_pos = new offset_vector_type(); // starting positions for sorted lml

	bool_vector_type *sorted_l_diff = new bool_vector_type(); // diff flags for sorted lml (current and previous are diff or not)

	for (uint8 i = 0; i < m_blocks_info.size() - 1; ++i) { // skip the leftmost block, cause we didn't compute bwt for it 

		m_sub_l_bwt_seqs[i]->start_read(); 
	}

	{
		alphabet_type cur_bkt = (*(*sorter_lms)).first; // the original string contains at least one S*-substr, thus not empty

		offset_type name_cnt = 0; // name counter

		while (true) {

			// scan the L-type char bucket
			while (!pq_l->empty() && pq_l->top().first == cur_bkt) {
				
				bool lml_diff = true;

				// scan the first in the name bucket
				Pair<alphabet_type, offset_type> cur_str = pq_l->top(); // <ch, pos>

#ifdef DEBUG_TEST3
				std::cerr << "cur_str: " << "ch: " << (uint32)cur_str.first << " pos: " << cur_str.second << std::endl;
#endif

				pq_l->pop();

				// induce the preceding substr
				uint8 block_id = getBlockId(cur_str.second);

				alphabet_type pre_ch = m_sub_l_bwt_seqs[block_id]->get();

				m_sub_l_bwt_seqs[block_id]->next_remove();

				if (pre_ch >= cur_bkt) { // preceding is L-type

					pq_l->push(triple_type(pre_ch, name_cnt, cur_str.second - 1));
				}
				else { // current is L*-type

					(*sorted_l_ch).push_back(cur_bkt);

					(*sorted_l_pos).push_back(cur_str.second);

					(*sorted_l_diff).push_back(lml_diff); // actually, it must be different

					lml_diff = false;
				}
					
				// scan the remaining elements in the name bucket
				// must call pq_l->top() before calling pq_l->is_diff
				while(!pq_l->empty() && pq_l->top().first == cur_bkt && !pq_l->is_diff()) { 

					Pair<alphabet_type, offset_type> cur_str = pq_l->top();

#ifdef DEBUG_TEST2
				std::cerr << "cur_str: " << "ch: " << (uint32)cur_str.first << " pos: " << cur_str.second << std::endl;
#endif

					pq_l->pop();

					// induce the preceding substr
					uint8 block_id = getBlockId(cur_str.second);

					alphabet_type pre_ch = m_sub_l_bwt_seqs[block_id]->get();

					m_sub_l_bwt_seqs[block_id]->next_remove();

					if (pre_ch >= cur_bkt) { // preceding is L-type

						pq_l->push(triple_type(pre_ch, name_cnt, cur_str.second - 1));
					}
					else { // current is L*-type
			
						(*sorted_l_ch).push_back(cur_bkt);

						(*sorted_l_pos).push_back(cur_str.second);
			
						(*sorted_l_diff).push_back(lml_diff);

						lml_diff = false;
					}
				}

				name_cnt = name_cnt + 1; // increase by 1

				pq_l->flush(); //prepare for scanning the next name bucket (may be in the same char bucket) 
			}

			// scan the S-type char bucket
			while (!sorter_lms->empty() && (*(*sorter_lms)).first == cur_bkt) {

				// scan the first in the name bucket
				Pair<alphabet_type, offset_type> cur_str = *(*sorter_lms); // <ch, pos>

#ifdef DEBUG_TEST2
				std::cerr << "cur_str: " << "ch: " << (uint32)cur_str.first << " pos: " << cur_str.second << std::endl;
#endif

				++(*sorter_lms);

				// induce the preceding substr
				uint8 block_id = getBlockId(cur_str.second);

				alphabet_type pre_ch = m_sub_l_bwt_seqs[block_id]->get();

				m_sub_l_bwt_seqs[block_id]->next_remove();
				
				if (pre_ch > cur_bkt) {

					pq_l->push(triple_type(pre_ch, name_cnt, cur_str.second - 1));
				}							 

				// scan the remaining elements in the name bucket
				while (!sorter_lms->empty() && (*(*sorter_lms)).first == cur_bkt) {

					Pair<alphabet_type, offset_type> cur_str = *(*sorter_lms);

#ifdef DEBUG_TEST2
				std::cerr << "cur_str: " << "ch: " << (uint32)cur_str.first << " pos: " << cur_str.second << std::endl;
#endif

					++(*sorter_lms);

					// induce the preceding substr
					uint8 block_id = getBlockId(cur_str.second);

					alphabet_type pre_ch = m_sub_l_bwt_seqs[block_id]->get();

					m_sub_l_bwt_seqs[block_id]->next_remove();

					if (pre_ch > cur_bkt) {

						pq_l->push(triple_type(pre_ch, name_cnt, cur_str.second - 1));
					} 
				}						

				name_cnt = name_cnt + 1;

				pq_l->flush(); // prepare for scanning the next name bucket (for lms, the name bucket is also the char bucket)
			}

			if (!pq_l->empty()) {

				cur_bkt = pq_l->top().first;

				if (!sorter_lms->empty() && (*(*sorter_lms)).first < cur_bkt) {
				
					cur_bkt = (*(*sorter_lms)).first;
				}
			}
			else if (!sorter_lms->empty()) {

				cur_bkt = (*(*sorter_lms)).first;
			}
			else {

				break;
			}
		}
	}	

	// clear
	for (uint8 i = 0; i < m_blocks_info.size() - 1; ++i) {

		assert(m_sub_l_bwt_seqs[i]->is_eof() == true);

		delete m_sub_l_bwt_seqs[i]; m_sub_l_bwt_seqs[i] = nullptr;
	}

	delete pq_l; pq_l = nullptr;

#ifdef DEBUG_TEST4
	std::cerr << "finish inducing L-type substrs.\n";
#endif 

	// induce S-type substrs
	typedef TupleAscCmp3<triple_type> triple_comparator_type2;

	typedef PQS_SUB<alphabet_type, offset_type, triple_type, triple_comparator_type2> heap_type2;

	heap_type2 *pq_s = new heap_type2(MAX_MEM); // pq_l has been deleted	

	offset_vector_type *sorted_s_pos = new offset_vector_type();

	bool_vector_type *sorted_s_diff = new bool_vector_type();

	bool lml_diff_global = true; // cache values retrieved from sorted_l_diff
	
	for (uint8 i = 0; i < m_blocks_info.size() - 1; ++i) {

		m_sub_s_bwt_seqs[i]->start_read();
	}

	// read information for sorted L*-type reversely
	sorted_l_ch->start_read_reverse(); 

	sorted_l_pos->start_read_reverse();

	sorted_l_diff->start_read_reverse();

	{

		alphabet_type cur_bkt = sorted_l_ch->get_reverse(); // s contain at least one S*-type substr, thus not empty

		offset_type name_cnt = std::numeric_limits<offset_type>::max() - 1;

		while (true) {
	
			// scan S-type char bucket
			while (!pq_s->empty() && pq_s->top().first == cur_bkt) {

				bool lms_diff = true;

				// scan the rightmost in the name bucket
				Pair<alphabet_type, offset_type> cur_str = pq_s->top();

				pq_s->pop();

				// induce
				uint8 block_id = getBlockId(cur_str.second);

				if (m_blocks_info[block_id].m_end_pos == cur_str.second) { // current is S*-type

					// s[cur_str.third] is the leftmost char in a single/multi-block, no need to induce its preceding
	
					// s[cur_str.third] is the heading char of an S*-substr
					sorted_s_pos->push_back(cur_str.second);

					sorted_s_diff->push_back(lms_diff);

					lms_diff = false;
				}				
				else {

					alphabet_type pre_ch = m_sub_s_bwt_seqs[block_id]->get();

					m_sub_s_bwt_seqs[block_id]->next_remove();

					if (pre_ch <= cur_bkt) { // preceding is S-type

						pq_s->push(triple_type(pre_ch, name_cnt, cur_str.second - 1));
					}
					else { // current is S*-type
					
						sorted_s_pos->push_back(cur_str.second);

						sorted_s_diff->push_back(lms_diff);

						lms_diff = false;
					}
				}

				// scan the remaining in the same name bucket
				while (!pq_s->empty() && pq_s->top().first == cur_bkt && !pq_s->is_diff()) {

					Pair<alphabet_type, offset_type> cur_str = pq_s->top();

					pq_s->pop();

					// induce
					uint8 block_id = getBlockId(cur_str.second);

					if (m_blocks_info[block_id].m_end_pos == cur_str.second) { // leftmost in a block

						// no need to induce the preceding

						// record the information of the sorted S*-substr	
						sorted_s_pos->push_back(cur_str.second);

						sorted_s_diff->push_back(lms_diff);

						lms_diff = false;
					}
					else {

						alphabet_type pre_ch = m_sub_s_bwt_seqs[block_id]->get();

						m_sub_s_bwt_seqs[block_id]->next_remove();

						if (pre_ch <= cur_bkt) {

							pq_s->push(triple_type(pre_ch, name_cnt, cur_str.second - 1));
						}
						else {

							sorted_s_pos->push_back(cur_str.second);

							sorted_s_diff->push_back(lms_diff);

							lms_diff = false;
						}
					}
				}	

				name_cnt = name_cnt - 1;

				pq_s->flush();
			}
		
			// scan L-type char bucket
			while (!sorted_l_ch->is_eof() && sorted_l_ch->get_reverse() == cur_bkt) {

				// scan the first in the name bucket
				offset_type cur_pos = sorted_l_pos->get_reverse();

				sorted_l_ch->next_remove_reverse(), sorted_l_pos->next_remove_reverse();

				// induce the preceding substr
				uint8 block_id = getBlockId(cur_pos);

				alphabet_type pre_ch = m_sub_s_bwt_seqs[block_id]->get();

				m_sub_s_bwt_seqs[block_id]->next_remove();

				if (pre_ch < cur_bkt) {

					pq_s->push(triple_type(pre_ch, name_cnt, cur_pos - 1));
				}	

				lml_diff_global = sorted_l_diff->get_reverse();

				sorted_l_diff->next_remove_reverse();

				// scan the remaining elements in the name bucket
				while (!sorted_l_ch->is_eof() && lml_diff_global == false) {

					offset_type cur_pos = sorted_l_pos->get_reverse();

					sorted_l_ch->next_remove_reverse(), sorted_l_pos->next_remove_reverse();

					// induce the preceding substr
					uint8 block_id = getBlockId(cur_pos);
		
					alphabet_type pre_ch = m_sub_s_bwt_seqs[block_id]->get();

					m_sub_s_bwt_seqs[block_id]->next_remove();

					if (pre_ch < cur_bkt) {

						pq_s->push(triple_type(pre_ch, name_cnt, cur_pos - 1));
					}	

					lml_diff_global = sorted_l_diff->get_reverse();

					sorted_l_diff->next_remove_reverse();
				}

				name_cnt = name_cnt - 1;

				pq_s->flush();
			}

			if (!pq_s->empty()) {

				cur_bkt = pq_s->top().first;

				if (!sorted_l_ch->is_eof() && sorted_l_ch->get_reverse() > cur_bkt) {

					cur_bkt = sorted_l_ch->get_reverse();
				}
			}
			else if (!sorted_l_ch->is_eof()) {

				cur_bkt = sorted_l_ch->get_reverse();
			}
			else {

				break;
			}	
		}		
	}

#ifdef DEBUG_TEST4
	std::cerr << "finish inducing S-type substrs.\n";
#endif 

	// clear
	assert(sorted_l_ch->is_eof() == true);

	assert(sorted_l_pos->is_eof() == true);

	assert(sorted_l_diff->is_eof() == true);

	delete sorted_l_ch; sorted_l_ch = nullptr;

	delete sorted_l_pos; sorted_l_pos = nullptr;

	delete sorted_l_diff; sorted_l_diff = nullptr;

	for (uint8 i = 0; i < m_blocks_info.size() - 1; ++i) {

		assert(m_sub_s_bwt_seqs[i]->is_eof() == true);

		delete m_sub_s_bwt_seqs[i]; m_sub_s_bwt_seqs[i] = nullptr;
	}	

	delete pq_s; pq_s = nullptr;


	// generate s1 and check if duplicate names exist

#ifdef DEBUG_TEST4
	std::cerr << "start sorting substr name.\n";
#endif 

	typedef Pair<offset_type, offset_type> pair_type2; // <pos, name>

	typedef TupleAscCmp1<pair_type2> pair_comparator_type2;

	typedef MySorter<pair_type2, pair_comparator_type2> sorter_type2;

	sorter_type2 *lms_substr_sorter = new sorter_type2(MAX_MEM); // pq_s has been deleted

	bool unique = true;

	sorted_s_pos->start_read_reverse();

	sorted_s_diff->start_read_reverse();

	{
		offset_type lms_name_global = 0; // 0 is reserved for the sentinel

		bool lms_diff_global = true;
	
		while (!sorted_s_pos->is_eof()) {

			if (lms_diff_global == true) {

				lms_name_global = lms_name_global + 1;
			}
			else {

				unique = false;
			}

			lms_substr_sorter->push(pair_type2(sorted_s_pos->get_reverse(), lms_name_global));

			sorted_s_pos->next_remove_reverse();

			lms_diff_global = sorted_s_diff->get_reverse();

			sorted_s_diff->next_remove_reverse();
		}

		lms_substr_sorter->sort();
		
#ifdef DEBUG_TEST4
	std::cerr << "finish sorting substr name.\n";

	std::cerr << "start producing S1.\n";
#endif

		m_s1 = new offset_vector_type();

		while (!lms_substr_sorter->empty()) {

			m_s1->push_back((*(*lms_substr_sorter)).second);

			++(*lms_substr_sorter);
		}

		m_s1->push_back(0); // push the sentinel

		delete lms_substr_sorter; lms_substr_sorter = nullptr;
	}		

#ifdef DEBUG_TEST4
	std::cerr << "finish producing S1.\n";
#endif


#ifdef DEBUG_TEST3
	std::cerr << "s1: \n";

	m_s1->start_read();

	while (!m_s1->is_eof()) {

		std::cerr << m_s1->get() << " ";

		m_s1->next();
	}

	std::cerr << std::endl;
#endif 

	return unique;
}


/// \brief sort suffix
///
template<typename alphabet_type, typename offset_type>
void DSAComputation<alphabet_type, offset_type>::sortSuffixGlobal(offset_vector_type *& _sa1_reverse) {

	uint8 block_num = m_blocks_info.size();

	if (_sa1_reverse == nullptr) { // directly use m_s1

		m_s1->start_read_reverse();

		for (uint8 i = 0; i < block_num; ++i) {

			offset_vector_type *lms_name_seq = new offset_vector_type();

			if (i == block_num - 1) { // leftmost block, contain an S*-type char
	
				lms_name_seq->push_back(m_s1->get_reverse()), m_s1->next_remove_reverse();

				assert(m_s1->is_eof() == true);

				delete m_s1; m_s1 = nullptr;
			}
			else { // otherwise

				for (uint32 j = 0; j < m_blocks_info[i].m_lms_num; ++j, m_s1->next_remove_reverse()) {

					lms_name_seq->push_back(m_s1->get_reverse());
				}
			}

			m_lms_name_seqs.push_back(lms_name_seq);
		}
	}
	else { // m_s1 has been deleted, use m_sa1

#ifdef DEBUG_TEST4
		std::cerr << "create sorter lms suffix\n";
#endif

		typedef Pair<offset_type, offset_type> pair_type;

		typedef TupleDscCmp1<pair_type> pair_comparator_type;

		typedef MySorter<pair_type, pair_comparator_type> sorter_type;

		sorter_type *sorter_lms = new sorter_type(MAX_MEM);

		_sa1_reverse->start_read_reverse();

		for (offset_type i = 0; !_sa1_reverse->is_eof(); ++i, _sa1_reverse->next_remove_reverse()) { 

			sorter_lms->push(pair_type(_sa1_reverse->get_reverse() + 1, i)); // sa1_reader + 1 != 0 
		}

		delete _sa1_reverse; _sa1_reverse = nullptr;

		sorter_lms->sort();

#ifdef DEBUG_TEST4

		std::cerr << "sorter_lms_suffix: " << sorter_lms->size() << std::endl;
#endif

		for (uint8 i = 0; i < block_num; ++i) {

			offset_vector_type *lms_name_seq = new offset_vector_type();

			if (i == block_num - 1) { // leftmost block

				lms_name_seq->push_back((*(*sorter_lms)).second); ++(*sorter_lms);

				assert(sorter_lms->empty() == true);

				delete sorter_lms; sorter_lms = nullptr;
			}
			else {

				for (uint32 j = 0; j < m_blocks_info[i].m_lms_num; ++j, ++(*sorter_lms)) {

					lms_name_seq->push_back((*(*sorter_lms)).second);
				}
			}

			m_lms_name_seqs.push_back(lms_name_seq);
		}
	}

	// induce suffixes in each block
	m_s->start_read_reverse();

	for (uint8 i = 0; i < m_blocks_info.size(); ++i) {

		if (m_blocks_info[i].is_multi()) {

			if (sizeof(alphabet_type) <= sizeof(uint32)) { 
			
				sortSuffixMultiBlock<false>(m_blocks_info[i]); 
			}
			else { 
				
				sortSuffixMultiBlock<true>(m_blocks_info[i]); 
			}
		}
		else if (m_blocks_info[i].is_single()) {

			sortSuffixSingleBlock(m_blocks_info[i]);
		}
		else {

			sortSuffixLeftmostBlock();
		}
	}

#ifdef DEBUG_TEST4

	std::cerr << "start merge sorted suffix global\n";
#endif

	// merge blockwise results 
	mergeSortedSuffixGlobal();

	return;
}


/// \brief sort suffixes in a single block
///
/// \note perform the same process of sortSStarSingleBlock
template<typename alphabet_type, typename offset_type>
void DSAComputation<alphabet_type, offset_type>::sortSuffixSingleBlock(const BlockInfo & _block_info) {

	uint64 toread = _block_info.m_size;

	alphabet_type cur_ch, last_ch;

	// induce L	
	alphabet_vector_type *suf_l_bwt_seq = new alphabet_vector_type();

	last_ch = m_s->get_reverse(), m_s->next_reverse(), --toread; // rightmost is S*-type

	while (m_s->get_reverse() >= last_ch) { // if cur_ch is L-type

		cur_ch = m_s->get_reverse();

		suf_l_bwt_seq->push_back(cur_ch); // record the preceding of last_ch

		m_s->next_reverse(), --toread;

		last_ch = cur_ch;
	}

	suf_l_bwt_seq->push_back(m_s->get_reverse()); // push the preceding of the leftmost L-type

	// induce S
	alphabet_vector_type *suf_s_bwt_seq = new alphabet_vector_type();

	if (toread == 1) { 

		suf_s_bwt_seq->push_back(m_s->get_reverse()); // push the preceding of the leftmost L-type

		--toread; // do not perform m_s->next_reverse, because two successive blocks overlap the character
	}
	else {

		suf_s_bwt_seq->push_back(m_s->get_reverse()); // push the preceding of the leftmost L-type

		m_s->next_reverse(), --toread;

		while (toread > 1) {

			suf_s_bwt_seq->push_back(m_s->get_reverse());

			m_s->next_reverse(), --toread;
		}

		suf_s_bwt_seq->push_back(m_s->get_reverse());

		--toread; // do not perform m_s->next_reverse, because two successive blocks overlap the character
	}

	m_suf_l_bwt_seqs.push_back(suf_l_bwt_seq), m_suf_s_bwt_seqs.push_back(suf_s_bwt_seq);

	return;
}


/// \brief sort suffixes in the leftmost block containing no S*-suffixes
///
template<typename alphabet_type, typename offset_type>
void DSAComputation<alphabet_type, offset_type>::sortSuffixLeftmostBlock() {

	alphabet_vector_type *suf_l_bwt_seq = new alphabet_vector_type();

	alphabet_type cur_ch, last_ch;

	uint8 cur_t, last_t;

	// induce L-type suffixes
	last_ch = m_s->get_reverse(), last_t = S_TYPE, m_s->next_reverse(); //rightmost is S-type

	while (!m_s->is_eof()) {

		cur_ch = m_s->get_reverse();

		cur_t = (cur_ch < last_ch || (cur_ch == last_ch && last_t == S_TYPE)) ? S_TYPE : L_TYPE;

		suf_l_bwt_seq->push_back(cur_ch); // push preceding

		if (cur_t == S_TYPE) {

			break;
		}

		last_ch = cur_ch, last_t = cur_t, m_s->next_reverse();
	}

	// induce S-type suffixes
	alphabet_vector_type *suf_s_bwt_seq = new alphabet_vector_type();

	while (!m_s->is_eof()) {

		suf_s_bwt_seq->push_back(m_s->get_reverse());

		m_s->next_reverse();
	} 

	m_suf_l_bwt_seqs.push_back(suf_l_bwt_seq), m_suf_s_bwt_seqs.push_back(suf_s_bwt_seq);

	return;
}

/// \brief sort suffixes in blocks containing multiple S*-suffixes
///
template<typename alphabet_type, typename offset_type>
template<bool FORMAT>
void DSAComputation<alphabet_type, offset_type>::sortSuffixMultiBlock(const BlockInfo & _block_info) {

	// load into RAM
	uint32 block_size = _block_info.m_size;

	alphabet_type *s = new alphabet_type[block_size];

	uint32 *fs = nullptr;

	uint32 max_alpha;

	if (FORMAT == true) {

		fs = new uint32[block_size];

		formatMultiBlock(_block_info, s, fs, max_alpha);
	}
	else {

		for (uint32 i = block_size - 1; i > 0; --i, m_s->next_reverse()) {

			s[i] = m_s->get_reverse();
		}

		s[0] = m_s->get_reverse(); // two blocks overlap an S*-type char, do not perform next_reverse

		max_alpha = ALPHA_MAX;
	}

	// compute t and sort S*-suffixes
	typedef Pair<offset_type, uint32> pair_type;

	typedef TupleDscCmp1<pair_type> pair_comparator_type;

	uint8 block_id = _block_info.m_id;

	std::vector<pair_type> *container = new std::vector<pair_type>(m_lms_name_seqs[block_id]->size());
	
	char *t_buf = new char[block_size / 8 + 1]; BitWrapper t(t_buf);

	m_lms_name_seqs[block_id]->start_read();

	t.set(block_size - 1, S_TYPE);

	for (uint32 i = block_size - 2, j = 0; ; --i) {

		t.set(i, (s[i] < s[i + 1] || (s[i] == s[i + 1] && t.get(i + 1) == S_TYPE)) ? S_TYPE : L_TYPE);

		if (t.get(i) == L_TYPE && t.get(i + 1) == S_TYPE) { // s[i + 1] is S*-type
	
			(*container)[j++] = pair_type(m_lms_name_seqs[block_id]->get(), i + 1);

			 m_lms_name_seqs[block_id]->next();
		}

		if (i == 0) break;
	}

	std::sort(container->begin(), container->end(), pair_comparator_type());

	// insert sorted S*-suffixes
	uint32 * sa = new uint32[block_size];

	for (uint32 i = 0; i < block_size; ++i) {

		sa[i] = UINT32_MAX;
	}

	uint32 * bkt = new uint32[max_alpha + 1];

	if (FORMAT == true) {

		getBuckets<uint32>(fs, block_size, bkt, max_alpha + 1, true);
	}
	else {
	
		getBuckets<alphabet_type>(s, block_size, bkt, max_alpha + 1, true);
	}

	for (uint32 i = 0; i < m_lms_name_seqs[block_id]->size(); ++i) {

		if (FORMAT == true) {

			sa[bkt[fs[(*container)[i].second]]--] = (*container)[i].second;
		}
		else {

			sa[bkt[s[(*container)[i].second]]--] = (*container)[i].second;
		}
	}

	delete container; container = nullptr;

	// induce L-type suffixes
	alphabet_vector_type *suf_l_bwt_seq = new alphabet_vector_type();

	if (FORMAT == true) {

		getBuckets<uint32>(fs, block_size, bkt, max_alpha + 1, false);
	}
	else {

		getBuckets<alphabet_type>(s, block_size, bkt, max_alpha + 1, false);
	}

	for (uint32 i = 0; i < block_size; ++i) {

		if (sa[i] != UINT32_MAX) { // sa[i] != 0

			uint32 j = sa[i] - 1;

			suf_l_bwt_seq->push_back(s[j]);

			if (t.get(j) == L_TYPE) {
			
				if (FORMAT == true) {
			
					sa[bkt[fs[j]]++] = j;
				}
				else {

					sa[bkt[s[j]]++] = j;
				}
			}

			if (t.get(sa[i]) == S_TYPE) {

				sa[i] = UINT32_MAX; // reserve all L-type
			}
		}
	}

	// induce S-type suffixes
	alphabet_vector_type *suf_s_bwt_seq = new alphabet_vector_type();

	if (FORMAT == true) {

		getBuckets<uint32>(fs, block_size, bkt, max_alpha + 1, true);
	}
	else {

		getBuckets<alphabet_type>(s, block_size, bkt, max_alpha + 1, true);
	}

	for (uint32 i = block_size - 1; ; --i) {

		if (sa[i] != 0 && sa[i] != UINT32_MAX) {

			uint32 j = sa[i] - 1;

			suf_s_bwt_seq->push_back(s[j]);

			if (t.get(j) == S_TYPE) {

				if (FORMAT == true) {

					sa[bkt[fs[j]]--] = j;
				}
				else {

					sa[bkt[s[j]]--] = j;
				}
			}
		}

		if (i == 0) break;
	}

	m_suf_l_bwt_seqs.push_back(suf_l_bwt_seq), m_suf_s_bwt_seqs.push_back(suf_s_bwt_seq);

	delete [] bkt; bkt = nullptr;

	delete [] s; s = nullptr;

	delete [] fs; fs = nullptr;

	delete [] t_buf; t_buf = nullptr;

	delete [] sa; sa = nullptr;
}

/// \brief merge block-wise sorted results
///
template<typename alphabet_type, typename offset_type>
void DSAComputation<alphabet_type, offset_type>::mergeSortedSuffixGlobal() {

	// sort S*-suffixes
	typedef Triple<offset_type, alphabet_type, offset_type> triple_type; // <rank, ch, pos>
	
	typedef TupleAscCmp1<triple_type> triple_comparator_type;

	typedef MySorter<triple_type, triple_comparator_type> sorter_type;

	sorter_type *lms_suffix_sorter = new sorter_type(MAX_MEM);

	m_s->start_read_reverse();

	for (uint8 i = 0; i < m_blocks_info.size(); ++i) {

		m_lms_name_seqs[i]->start_read();

		if (i == m_blocks_info.size() - 1) {

			lms_suffix_sorter->push(triple_type(m_lms_name_seqs[i]->get() + 1, m_s->get_reverse(), m_blocks_info[i].m_end_pos));

			m_lms_name_seqs[i]->next_remove();

			assert(m_lms_name_seqs[i]->is_eof() == true);

			delete m_lms_name_seqs[i]; m_lms_name_seqs[i] = nullptr;

			while (!m_s->is_eof()) {

				m_s->next_remove_reverse();
			}

			delete m_s; m_s = nullptr;
		}
		else {

			uint64 block_size = m_blocks_info[i].m_size;

			alphabet_type cur_ch, last_ch;

			uint8 cur_t, last_t;

			offset_type last_pos;

			last_ch = m_s->get_reverse(), last_t = S_TYPE, last_pos = m_blocks_info[i].m_end_pos, m_s->next_remove_reverse();

			for (uint64 j = block_size - 2; j > 0; --j, --last_pos, m_s->next_remove_reverse()) {

				cur_ch = m_s->get_reverse();
			
				cur_t = (cur_ch < last_ch || (cur_ch == last_ch && last_t == S_TYPE)) ? S_TYPE : L_TYPE;

				if (cur_t == L_TYPE && last_t == S_TYPE) {

					lms_suffix_sorter->push(triple_type(m_lms_name_seqs[i]->get() + 1, last_ch, last_pos));
	
					m_lms_name_seqs[i]->next_remove();
				}
	
				last_ch = cur_ch, last_t = cur_t;
			}			

			assert(m_lms_name_seqs[i]->is_eof() == true);

			delete m_lms_name_seqs[i]; m_lms_name_seqs[i] = nullptr;
		}		
	}

	lms_suffix_sorter->sort();

#ifdef DEBUG_TEST4

	std::cerr << "lms_suffix_sorter: " << lms_suffix_sorter->size() << std::endl;
#endif

	// induce L-type suffixes
	typedef Triple<alphabet_type, offset_type, offset_type> triple_type2; // (ch, name, pos)

	typedef TupleDscCmp2<triple_type2> triple_comparator_type2;

	typedef PQL_SUF<alphabet_type, offset_type, triple_type2, triple_comparator_type2> heap_type;

	heap_type *pq_l = new heap_type(MAX_MEM);

	alphabet_vector_type *sorted_l_ch = new alphabet_vector_type();

	offset_vector_type *sorted_l_pos = new offset_vector_type();

	for (uint8 i = 0; i < m_blocks_info.size(); ++i) {

		m_suf_l_bwt_seqs[i]->start_read();
	}

	{
		alphabet_type cur_bkt = (*(*lms_suffix_sorter)).second; // lms_suffix_sorter contains at least the sentinel

		offset_type name_cnt = 1; // start naming from 1 to avoid <0, 0, s_len - 1>

		while (true) {

			// scan L-type char bucket
			while (!pq_l->empty() && pq_l->top().first == cur_bkt) {

				Pair<alphabet_type, offset_type> cur_str = pq_l->top();

				pq_l->pop();

				uint8 block_id = getBlockId(cur_str.second);

				if (OFFSET_MIN != cur_str.second) {

					alphabet_type pre_ch = m_suf_l_bwt_seqs[block_id]->get();

					m_suf_l_bwt_seqs[block_id]->next_remove();

					if (pre_ch >= cur_bkt) {

						pq_l->push(triple_type2(pre_ch, name_cnt, cur_str.second - 1));
					}
				}

				sorted_l_ch->push_back(cur_str.first);

				sorted_l_pos->push_back(cur_str.second);
	
				name_cnt = name_cnt + 1;
			}

			// scan S-type char bucket
			while (!lms_suffix_sorter->empty() && (*(*lms_suffix_sorter)).second == cur_bkt) {

				triple_type cur_str = *(*lms_suffix_sorter);

				++(*lms_suffix_sorter);

				uint8 block_id = getBlockId(cur_str.third);

				alphabet_type pre_ch = m_suf_l_bwt_seqs[block_id]->get();

				m_suf_l_bwt_seqs[block_id]->next_remove();

				pq_l->push(triple_type2(pre_ch, name_cnt, cur_str.third - 1));

				name_cnt = name_cnt + 1;
			}

			if (!pq_l->empty()) {

				cur_bkt = pq_l->top().first;

				if (!lms_suffix_sorter->empty() && (*(*lms_suffix_sorter)).second < cur_bkt) {

					cur_bkt = (*(*lms_suffix_sorter)).second;
				}
			}
			else if (!lms_suffix_sorter->empty()) {

				cur_bkt = (*(*lms_suffix_sorter)).second;
			}
			else {

				break;
			}	
		}
	}

	// clear
	delete lms_suffix_sorter; lms_suffix_sorter = nullptr;

	delete pq_l; pq_l = nullptr;

	for (uint8 i = 0; i < m_blocks_info.size(); ++i) {

		assert(m_suf_l_bwt_seqs[i]->is_eof() == true);

		delete m_suf_l_bwt_seqs[i]; m_suf_l_bwt_seqs[i] = nullptr;
	}

#ifdef DEBUG_TEST4
	std::cerr << "finish inducing L-type suffixes.\n";
#endif

#ifdef DEBUG_TEST3
	sorted_l_ch->start_read();

	sorted_l_pos->start_read();

	while(!sorted_l_ch->is_eof()) {

		std::cerr << "ch: " << (uint32)sorted_l_ch->get() <<" pos: " << sorted_l_pos->get() << std::endl;

		sorted_l_ch->next(), sorted_l_pos->next(); 
	}

#endif

	// induce S-type suffixes
	typedef TupleAscCmp2<triple_type2> triple_comparator_type3;
	
	typedef PQS_SUF<alphabet_type, offset_type, triple_type2, triple_comparator_type3> heap_type2;

	heap_type2 *pq_s = new heap_type2(MAX_MEM);

	std::vector<bool> is_l_star(m_blocks_info.size());

	sorted_l_ch->start_read_reverse();

	sorted_l_pos->start_read_reverse();

	for (uint8 i = 0; i < m_blocks_info.size(); ++i) {

		m_suf_s_bwt_seqs[i]->start_read();

		if (m_blocks_info[i].is_single()) { // single-block contains one L*-type char

			is_l_star[i] = true;
		}
		else if (!m_blocks_info[i].is_multi()) { // leftmost block may contain no or one L*-type char

			is_l_star[i] = (m_suf_s_bwt_seqs[i]->size() == 0) ? false : true;
		}
	}

	{
		m_sa_reverse = new offset_vector_type();

		alphabet_type cur_bkt = sorted_l_ch->get_reverse();

		offset_type name_cnt = std::numeric_limits<offset_type>::max() - 1; 
	
		while (true) {

			// scan S-type char bucket
			while (!pq_s->empty() && pq_s->top().first == cur_bkt) {

				Pair<alphabet_type, offset_type> cur_str = pq_s->top();

				pq_s->pop();

				uint8 block_id = getBlockId(cur_str.second);

				if (m_blocks_info[block_id].m_end_pos != cur_str.second && OFFSET_MIN != cur_str.second) {

					alphabet_type pre_ch = m_suf_s_bwt_seqs[block_id]->get();

					m_suf_s_bwt_seqs[block_id]->next_remove();

					if (pre_ch <= cur_bkt) {

						pq_s->push(triple_type2(pre_ch, name_cnt, cur_str.second - 1));
					}
				}

				m_sa_reverse->push_back(cur_str.second);

				name_cnt = name_cnt - 1;
			}

			// scan L-type char bucket
			while (!sorted_l_ch->is_eof() && sorted_l_ch->get_reverse() == cur_bkt) {

				offset_type cur_pos = sorted_l_pos->get_reverse();

				sorted_l_ch->next_remove_reverse(), sorted_l_pos->next_remove_reverse();

				uint8 block_id = getBlockId(cur_pos);

				if (OFFSET_MIN != cur_pos) {

					if (m_blocks_info[block_id].is_multi() || is_l_star[block_id] == true) {

						alphabet_type pre_ch = m_suf_s_bwt_seqs[block_id]->get();

						m_suf_s_bwt_seqs[block_id]->next_remove();

						if (pre_ch < cur_bkt) {
						
							pq_s->push(triple_type2(pre_ch, name_cnt, cur_pos - 1));

							is_l_star[block_id] = false;
						}
					}
				}

				m_sa_reverse->push_back(cur_pos);

				name_cnt = name_cnt - 1;
			}

			if (!pq_s->empty()) {

				cur_bkt = pq_s->top().first;

				if (!sorted_l_ch->is_eof() && sorted_l_ch->get_reverse() > cur_bkt) {

					cur_bkt = sorted_l_ch->get_reverse();
				}
			}
			else if (!sorted_l_ch->is_eof()) {

				cur_bkt = sorted_l_ch->get_reverse();
			}
			else {

				break;
			}
		}
	}

	m_sa_reverse->push_back(offset_type(m_s_len - 1));

	// clear
	for (uint8 i = 0; i < m_blocks_info.size(); ++i) {

		assert(m_suf_s_bwt_seqs[i]->is_eof() == true);

		delete m_suf_s_bwt_seqs[i]; m_suf_s_bwt_seqs[i] = nullptr;
	}

	assert(sorted_l_ch->is_eof() == true);

	assert(sorted_l_pos->is_eof() == true);

	delete sorted_l_ch; sorted_l_ch = nullptr;

	delete sorted_l_pos; sorted_l_pos = nullptr;

	delete pq_s; pq_s = nullptr;

#ifdef DEBUG_TEST4

	std::cerr << "finish inducing S-type suffixes\n";
#endif
}
#endif
