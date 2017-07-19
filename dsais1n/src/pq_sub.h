//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Copyright (c) 2017, Sun Yat-sen university.
/// All rights reserved.
/// \file pq_sub.h
/// \brief self-defined external-memory priority queue for sorting substrings in an I/O-efficient way.
///
/// \author Yi Wu
/// \date 2017.5
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef _PQ_SUB_H
#define _PQ_SUB_H

#include "vector.h"

/// \brief a priority queue for sorting L-type substrs.
///
template<typename alphabet_type, typename offset_type, typename pq2_element_type, typename pq2_comparator_type>
class PQL_SUB{

private:

	// alias
	typedef Pair<alphabet_type, offset_type> pql_element_type; ///< (ch, pos), the return type for top() 

	typedef Pair<alphabet_type, uint32> pq1_element_type; ///< (ch, block_id), element type for the small heap

	typedef TupleDscCmp2<pq1_element_type> pq1_comparator_type; ///< comparator type for the small heap

	typedef std::priority_queue<pq1_element_type, std::vector<pq1_element_type>, pq1_comparator_type> pq1_type; ///< small heap type 

	typedef std::priority_queue<pq2_element_type, std::vector<pq2_element_type>, pq2_comparator_type> pq2_type; ///< big heap type, modified

	typedef Triple<alphabet_type, offset_type, bool> block_element_type; ///< (ch, pos, diff), element type for an EM block

	typedef MyVector<block_element_type> block_type; ///< EM block type 

private:

	// member

	pq1_type *m_heap1; ///< a small heap for organizing the elements residing on EM in their ascending order

	pq2_type *m_heap2; ///< a big heap for organizing the elements residing on RAM in their acending order

	pq2_type *m_heap3; ///< a big heap for caching suffixes induced from the name bucket being scanned

	uint64 m_pq2_capacity; ///< m_heap2 can afford at most m_pq2_capacity elements

	std::vector<block_type*> m_blocks; ///< handlers to EM blocks

	std::vector<offset_type> m_suc_name_in_block; ///< record the smallest suc name in a block

	uint32 m_cur_block_idx; ///< block idx of currently scanned L-type substr

	uint32 m_pre_block_idx; ///< block idx of previously scanned L-type substr	

	alphabet_type m_cur_ch; ///< heading char of currently scanned L-type substr

	alphabet_type m_pre_ch; ///< heading char of previously scanned L-type substr

	offset_type m_cur_suc_name; ///< succeeding name of currently scanned L-type substr

	offset_type m_pre_suc_name; ///< succeeding name of previously scanned L-type substr

	bool m_cur_diff; ///< indicate whether or not two successively scanned L-type substrs are in the same name bucket

	bool m_flag;

public:

	/// \brief ctor
	///
	/// \param _avail_mem available memory for m_heap2
	PQL_SUB(const uint64 _avail_mem) {

		m_heap1 = new pq1_type();

		m_pq2_capacity = _avail_mem / sizeof(pq2_element_type) / 2;

		m_heap2 = new pq2_type();	

		m_heap3 = new pq2_type();

		m_blocks.clear();

		m_pre_block_idx = std::numeric_limits<uint32>::max() - 1; // assumed to be max - 1

		m_pre_ch = 0; // assumed to be 0 initially, cause any L-type substr starts with a character > 0

		m_pre_suc_name = std::numeric_limits<offset_type>::max(); // assumed to be max initially

		m_flag = false;
	}

	/// \brief dtor
	///
	~PQL_SUB() {

		delete m_heap1; m_heap1 = nullptr;

		delete m_heap2; m_heap2 = nullptr;

		delete m_heap3; m_heap3 = nullptr;
	}

	/// \brief check if PQL_SUB is empty
	///
	/// check if m_heap1 and m_heap2 are both empty
	bool empty() {

		return (m_heap1->empty() && m_heap2->empty());
	}


	/// \brief get the smallest 
	///
	/// \note check if non-empty before calling the function. In this case, m_heap1 or m_heap2 must be non-empty
	pql_element_type top() {

		 if (!m_heap1->empty()) {

			if (!m_heap2->empty()) {

				if (m_heap1->top().first <= m_heap2->top().first) { // m_heap1

					const pq1_element_type& cur_str = m_heap1->top();

					m_cur_block_idx = cur_str.second; // block_idx

					m_cur_ch = cur_str.first;

					m_cur_diff = m_blocks[m_cur_block_idx]->get().third; // check this field if two successively popped are both from heap1

					return pql_element_type(m_cur_ch, m_blocks[m_cur_block_idx]->get().second); // (ch ,pos)
				}
				else { // m_heap2

					const pq2_element_type& cur_str = m_heap2->top();

					m_cur_block_idx = std::numeric_limits<uint32>::max();

					m_cur_ch = cur_str.first;

					m_cur_suc_name = cur_str.second; // check this field if two successively popped are both from heap2

					return pql_element_type(m_cur_ch, cur_str.third); // (ch, pos)
				}
			}
			else { // m_heap1

				const pq1_element_type& cur_str = m_heap1->top();

				m_cur_block_idx = cur_str.second; // block_idx

				m_cur_ch = cur_str.first;

				m_cur_diff = m_blocks[m_cur_block_idx]->get().third; // check this field if two successively popped are both from heap1

				return pql_element_type(m_cur_ch, m_blocks[m_cur_block_idx]->get().second); // (ch ,pos)
			}
		}
		else { // m_heap2
				
			const pq2_element_type& cur_str = m_heap2->top();

			m_cur_block_idx = std::numeric_limits<uint32>::max();

			m_cur_ch = cur_str.first;

			m_cur_suc_name = cur_str.second;

			return pql_element_type(m_cur_ch, cur_str.third);
		}
	}


	/// \brief indicate if two successively popped substrs are in the same name bucket
	///
	bool is_diff() {

		if (m_pre_block_idx == m_cur_block_idx) { // from the same block

			if (m_cur_block_idx == std::numeric_limits<uint32>::max()) { // both from m_heap2

				return !(m_pre_suc_name == m_cur_suc_name && m_pre_ch == m_cur_ch);
			}
			else { // both from the same EM block

				return m_cur_diff;
			}
		}
		else { // from different blocks

			if (m_cur_block_idx == std::numeric_limits<uint32>::max() || m_pre_block_idx == std::numeric_limits<uint32>::max()) {

				return true;				
			}
			else {

				return !(m_suc_name_in_block[m_cur_block_idx] == m_suc_name_in_block[m_pre_block_idx] && m_pre_ch == m_cur_ch);
			}
		}

	}

	/// \brief pop the smallest element
	///
	void pop() {

		if (m_cur_block_idx == std::numeric_limits<uint32>::max()) { // pop from m_heap2

			m_pre_block_idx = m_cur_block_idx, m_pre_suc_name = m_cur_suc_name, m_pre_ch = m_cur_ch, m_heap2->pop();
		}
		else { // pop from m_heap1

			m_pre_block_idx = m_cur_block_idx, m_pre_ch = m_cur_ch, m_blocks[m_pre_block_idx]->next_remove(), m_heap1->pop();

			if (!m_blocks[m_pre_block_idx]->is_eof()) {

				m_heap1->push(pq1_element_type(m_blocks[m_pre_block_idx]->get().first, m_pre_block_idx));
			}
		}
	}

	void flush_heap() {

		if (!m_heap2->empty()) {

			// flush m_heap2
			pq2_element_type pre_str = m_heap2->top();

			m_suc_name_in_block.push_back(m_heap2->top().second);

			m_blocks.push_back(new block_type());
			
			m_blocks[m_blocks.size() - 1]->push_back(block_element_type(pre_str.first, pre_str.third, true));

			m_heap2->pop();

			while (!m_heap2->empty()) {

				const pq2_element_type& cur_str = m_heap2->top();

				m_blocks[m_blocks.size() - 1]->push_back(block_element_type(cur_str.first, cur_str.third, 
					(cur_str.first == pre_str.first && cur_str.second == pre_str.second) ? false : true));

				pre_str = cur_str, m_heap2->pop();
			}

			m_blocks[m_blocks.size() - 1]->start_read();

			m_heap1->push(pq1_element_type(m_blocks[m_blocks.size() - 1]->get().first, m_blocks.size() - 1));
		}
			
		if (!m_heap3->empty()){	

			// flush m_heap3
			pq2_element_type pre_str = m_heap3->top();

			m_suc_name_in_block.push_back(m_heap3->top().second);

			m_blocks.push_back(new block_type());

			m_blocks[m_blocks.size() - 1]->push_back(block_element_type(pre_str.first, pre_str.third, true));

			m_heap3->pop();

			while (!m_heap3->empty()) {

				const pq2_element_type& cur_str = m_heap3->top();

				m_blocks[m_blocks.size() - 1]->push_back(block_element_type(cur_str.first, cur_str.third,
					(cur_str.first == pre_str.first && cur_str.second == pre_str.second) ? false : true));

				pre_str = cur_str, m_heap3->pop();
			}

			m_blocks[m_blocks.size() - 1]->start_read();

			m_heap1->push(pq1_element_type(m_blocks[m_blocks.size() - 1]->get().first, m_blocks.size() - 1));	
		}
	}

	/// \brief push an element into PQ_L
	///
	void push(const pq2_element_type & _value) {

		if (m_heap3->size() == m_pq2_capacity) { // 

			flush_heap();

			m_flag = true;
		}

		m_heap3->push(_value);		
	}

	/// \brief after scanning current name bucket, check if needed to flush elements in m_heap2 into EM
	///
	void flush() {

		if (m_flag == true) {

			flush_heap();
		}
		else {

			if (m_heap2->size() + m_heap3->size() <= m_pq2_capacity) { // move elements in m_heap3 to m_heap2

				while (!m_heap3->empty()) {

					m_heap2->push(m_heap3->top());
	
					m_heap3->pop();
				}	
			}
			else { // flush m_heap2 and m_heap3 into EM
	
				flush_heap();
			}
		}

		m_flag = false;
	}
};

/// \brief a priority queue for sorting S-type substrs.
///
template<typename alphabet_type, typename offset_type, typename pq2_element_type, typename pq2_comparator_type>
class PQS_SUB{

private:

	typedef Pair<alphabet_type, offset_type> pqs_element_type; ///< (ch, pos);

	typedef Pair<alphabet_type, uint32> pq1_element_type; ///< (ch, std::numeric_limits<uint32>::max() - block_idx)

	typedef TupleAscCmp2<pq1_element_type> pq1_comparator_type; ///< comparator for sorting by the first two components in ascending order

	typedef std::priority_queue<pq1_element_type, std::vector<pq1_element_type>, pq1_comparator_type> pq1_type;

	typedef std::priority_queue<pq2_element_type, std::vector<pq2_element_type>, pq2_comparator_type> pq2_type; // modified

	typedef Triple<alphabet_type, offset_type, bool> block_element_type; ///< (ch, pos, diff)

	typedef MyVector<block_element_type> block_type;

private:

	pq1_type *m_heap1; ///< a heap for organizing elements residing on EM in descending order

	pq2_type *m_heap2; ///< a heap for organizing elements residing on RAM in descending order

	pq2_type *m_heap3; ///< a heap for caching substrings induced from the name bucket being scanned

	uint64 m_pq2_capacity; ///< capacity for m_heap2

	std::vector<block_type*> m_blocks; ///< EM blocks

	std::vector<offset_type> m_suc_name_in_block; ///< biggest suc_name in the block

	uint32 m_cur_block_idx; 

	uint32 m_pre_block_idx;

	alphabet_type m_cur_ch; ///< heading char of currently scanned S-type substr

	alphabet_type m_pre_ch; ///< heading char of previously scanned S-type substr

	offset_type m_cur_suc_name; ///< suc name of currently scanned S-type substr

	offset_type m_pre_suc_name; ///< suc name of previously scanned S-type substr

	bool m_cur_diff; ///< indicate whether or not two successively scanned S-type substrs are different

	bool m_flag;

public:

	/// \brief ctor
	///
	/// \param _avail_mem available RAM for heap2
	PQS_SUB(const uint64 _avail_mem) {

		m_heap1 = new pq1_type();

		m_heap2 = new pq2_type();

		m_heap3 = new pq2_type();

		m_pq2_capacity = _avail_mem / sizeof(pq2_element_type) / 2;

		m_blocks.clear();

		m_pre_block_idx = std::numeric_limits<uint32>::max() - 1;

		m_pre_ch = std::numeric_limits<alphabet_type>::max(); // any S-type substr must not start with max

		m_pre_suc_name = 0;					

		m_flag = false;
	}	

	/// \brief dtor
	~PQS_SUB() {

		delete m_heap1; m_heap1 = nullptr;

		delete m_heap2; m_heap2 = nullptr;

		delete m_heap3; m_heap3 = nullptr;
	}

	/// \brief check if empty
	bool empty() {

		return (m_heap1->empty() && m_heap2->empty());
	}

	/// \brief get the smallest
	///
	/// \note check if non-empty before calling the function
	pqs_element_type top() {

		if (!m_heap1->empty()) {

			if (!m_heap2->empty()) {

				if (m_heap1->top().first >= m_heap2->top().first) { // m_heap1

					const pq1_element_type& cur_str = m_heap1->top();

					m_cur_block_idx = std::numeric_limits<uint32>::max() - cur_str.second;

					m_cur_ch = cur_str.first;

					m_cur_diff = m_blocks[m_cur_block_idx]->get().third;

					return pqs_element_type(m_cur_ch, m_blocks[m_cur_block_idx]->get().second);	
				}
				else { // m_heap2

					const pq2_element_type& cur_str = m_heap2->top();

					m_cur_block_idx = std::numeric_limits<uint32>::max();

					m_cur_ch = cur_str.first;

					m_cur_suc_name = cur_str.second;
			
					return pqs_element_type(m_cur_ch, cur_str.third);				 
				}
			}
			else { // m_heap2 must be non-empty

				const pq1_element_type& cur_str = m_heap1->top();

				m_cur_block_idx = std::numeric_limits<uint32>::max() - cur_str.second;

				m_cur_ch = cur_str.first;

				m_cur_diff = m_blocks[m_cur_block_idx]->get().third;

				return pqs_element_type(m_cur_ch, m_blocks[m_cur_block_idx]->get().second);	
			}
		}
		else { // pop from m_heap2

			const pq2_element_type& cur_str = m_heap2->top();

			m_cur_block_idx = std::numeric_limits<uint32>::max();

			m_cur_ch = cur_str.first;

			m_cur_suc_name = cur_str.second;
			
			return pqs_element_type(m_cur_ch, cur_str.third);				 
		}
	}

	/// \brief indicate if two successively popped L-type substrs are different or not
	///
	bool is_diff() {

		if (m_pre_block_idx == m_cur_block_idx) { // same block or heap2

			if (m_cur_block_idx == std::numeric_limits<uint32>::max()) { // from heap2

				return !(m_pre_suc_name == m_cur_suc_name && m_pre_ch == m_cur_ch);
			}
			else { // from a block

				return m_cur_diff;
			}
		}
		else {

			if (m_cur_block_idx == std::numeric_limits<uint32>::max() || m_pre_block_idx == std::numeric_limits<uint32>::max()) {

				return true;
			}
			else {
		
				return !(m_suc_name_in_block[m_cur_block_idx] == m_suc_name_in_block[m_pre_block_idx] && m_cur_ch == m_pre_ch);
			}
		}
	}	

	/// \brief pop operation
	///
	/// \note execute top() before calling the function
	void pop() {

		if (m_cur_block_idx == std::numeric_limits<uint32>::max()) {

			m_pre_block_idx = m_cur_block_idx, m_pre_suc_name = m_cur_suc_name, m_pre_ch = m_cur_ch, m_heap2->pop();
		}
		else {
	
			m_pre_block_idx = m_cur_block_idx, m_pre_ch = m_cur_ch, m_blocks[m_pre_block_idx]->next_remove(), m_heap1->pop();			

			if (!m_blocks[m_pre_block_idx]->is_eof()) { // if more exists, keep a copy in m_heap1

				m_heap1->push(pq1_element_type(m_blocks[m_pre_block_idx]->get().first, 
					std::numeric_limits<uint32>::max() - m_pre_block_idx)); 	
			}
		}
	}	

	void flush_heap() {
		
		if (!m_heap2->empty()){

			// flush m_heap2
			pq2_element_type pre_str = m_heap2->top();

			m_suc_name_in_block.push_back(m_heap2->top().second);

			m_blocks.push_back(new block_type());
			
			m_blocks[m_blocks.size() - 1]->push_back(block_element_type(pre_str.first, pre_str.third, true));

			m_heap2->pop();

			while (!m_heap2->empty()) {

				const pq2_element_type& cur_str = m_heap2->top();

				m_blocks[m_blocks.size() - 1]->push_back(block_element_type(cur_str.first, cur_str.third, 
					(cur_str.first == pre_str.first && cur_str.second == pre_str.second) ? false : true));

				pre_str = cur_str, m_heap2->pop();
			}

			m_blocks[m_blocks.size() - 1]->start_read();

			m_heap1->push(pq1_element_type(m_blocks[m_blocks.size() - 1]->get().first, std::numeric_limits<uint32>::max() - m_blocks.size() + 1));
		}
				
		if (!m_heap3->empty()){	

			// flush m_heap3
			pq2_element_type pre_str = m_heap3->top();

			m_suc_name_in_block.push_back(m_heap3->top().second);

			m_blocks.push_back(new block_type());

			m_blocks[m_blocks.size() - 1]->push_back(block_element_type(pre_str.first, pre_str.third, true));

			m_heap3->pop();

			while (!m_heap3->empty()) {

				const pq2_element_type& cur_str = m_heap3->top();

				m_blocks[m_blocks.size() - 1]->push_back(block_element_type(cur_str.first, cur_str.third,
					(cur_str.first == pre_str.first && cur_str.second == pre_str.second) ? false : true));

				pre_str = cur_str, m_heap3->pop();
			}

			m_blocks[m_blocks.size() - 1]->start_read();

			m_heap1->push(pq1_element_type(m_blocks[m_blocks.size() - 1]->get().first, std::numeric_limits<uint32>::max() - m_blocks.size() + 1));
		}
	}

	/// \brief push an element
	///
	void push(const pq2_element_type & _value) {

		if (m_heap3->size() == m_pq2_capacity) {
	
			flush_heap();	

			m_flag = true;
		}

		m_heap3->push(_value);
	}

	/// \brief after scanning current name bucket, flush m_heap2 if it reaches m_pq2_threshold
	///
	void flush() {

		if (m_flag == true) {

			flush_heap();
		}
		else {
		
			if (m_heap2->size() + m_heap3->size() <= m_pq2_capacity) {

				while (!m_heap3->empty()) {
	
					m_heap2->push(m_heap3->top());
	
					m_heap3->pop();
				}
			}
			else { 

				flush_heap();
			}
		}

		m_flag = false;
	}
};

#endif // _PQ_SUB_H
