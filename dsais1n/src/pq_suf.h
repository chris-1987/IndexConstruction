////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Copyright (c) 2017, Sun Yat-sen university.
/// All rights reserved.
/// \file pq_suf.h
/// \brief Self-defined external-memory priority queues designed for sorting suffixes I/O-efficiently.
///
/// The priority queue is implemented by MyVector.
///
/// \author Yi Wu
/// \date 2017.5
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef _PQ_SUF_H
#define _PQ_SUF_H

#include "vector.h"

/// \brief A priority queue for sorting L-type suffixes.
///
/// \note Each name bucket contains only single suffix.
template<typename alphabet_type, typename offset_type, typename pq2_element_type, typename pq2_comparator_type>
class PQL_SUF{

private:

	typedef Pair<alphabet_type, offset_type> pql_element_type; ///< (ch, pos)

	typedef Pair<alphabet_type, uint32> pq1_element_type; ///< (ch, block_id) 

	typedef TupleDscCmp2<pq1_element_type> pq1_comparator_type; 

	typedef	std::priority_queue<pq1_element_type, std::vector<pq1_element_type>, pq1_comparator_type> pq1_type; 

	typedef std::priority_queue<pq2_element_type, std::vector<pq2_element_type>, pq2_comparator_type> pq2_type;

	typedef Pair<alphabet_type, offset_type> block_element_type; ///< (ch, pos), any two must be different

	typedef MyVector<block_element_type> block_type;

private:

	pq1_type *m_heap1; ///< a heap for sequentially retrieving the smallest among all the blocks on EM

	pq2_type *m_heap2; ///< a heap for sequentially retrieving the smallest among all on RAM

	uint64 m_pq2_capacity; ///< capacity of m_heap2
	
	std::vector<block_type*> m_blocks;

	uint32 m_cur_block_idx;	

public:

	/// \brief ctor 
	///	
	PQL_SUF(const uint64 _avail_mem) {

		m_heap1 = new pq1_type();

		m_heap2 = new pq2_type();

		m_pq2_capacity = _avail_mem / sizeof(pq2_element_type);

		m_blocks.clear();
	}

	/// \brief dtor
	///
	~PQL_SUF() {

		delete m_heap1; m_heap1 = nullptr;

		delete m_heap2; m_heap2 = nullptr;
	}

	/// \brief check if PQL_SUF is empty
	///
	/// \note the currently scanned name bucket must be in m_heap1 (blocks) or m_heap2
	bool empty() {

		return (m_heap1->empty() && m_heap2->empty());
	}


	/// \brief get top element
	///
	/// \note check if non-empty before calling the function
	pql_element_type top() {

		if (!m_heap1->empty()) {

			if (!m_heap2->empty()) {

				if (m_heap1->top().first <= m_heap2->top().first) { // m_heap1

					m_cur_block_idx = m_heap1->top().second; // block_idx

					return pql_element_type(m_heap1->top().first, m_blocks[m_cur_block_idx]->get().second); // (ch, pos)
				}
				else { // m_heap2

					m_cur_block_idx = std::numeric_limits<uint32>::max();

					return pql_element_type(m_heap2->top().first, m_heap2->top().third); // (ch, pos)
				}
			}
			else { // m_heap1

				m_cur_block_idx = m_heap1->top().second;

				return pql_element_type(m_heap1->top().first, m_blocks[m_cur_block_idx]->get().second);
			}
		}
		else { // m_heap2

				m_cur_block_idx = std::numeric_limits<uint32>::max();

				return pql_element_type(m_heap2->top().first, m_heap2->top().third);
		}	
	}


	/// \brief pop the smallest element
	///
	void pop() {

		if (m_cur_block_idx == std::numeric_limits<uint32>::max()) { // pop from heap2

			m_heap2->pop();		

		}
		else { // pop from heap1

			m_blocks[m_cur_block_idx]->next_remove(), m_heap1->pop();

			if (!m_blocks[m_cur_block_idx]->is_eof()) { // if the block has more elements, push the leftmost into heap1

				m_heap1->push(pq1_element_type(m_blocks[m_cur_block_idx]->get().first, m_cur_block_idx)); 
			}
		}
	}

	/// \brief push operation
	///
	void push(const pq2_element_type & _value) {

		m_heap2->push(_value);

		if (m_heap2->size() == m_pq2_capacity) {
	
			m_blocks.push_back(new block_type());	
	
			while (!m_heap2->empty()) {

				const pq2_element_type& cur_str = m_heap2->top();

				m_blocks[m_blocks.size() - 1]->push_back(block_element_type(cur_str.first, cur_str.third));

				m_heap2->pop();
			}

			m_blocks[m_blocks.size() - 1]->start_read();

			m_heap1->push(pq1_element_type(m_blocks[m_blocks.size() - 1]->get().first, m_blocks.size() - 1));
		}	
	}
};


/// \brief a priority queue for sorting S-type suffixes.
///
template<typename alphabet_type, typename offset_type, typename pq2_element_type, typename pq2_comparator_type>
class PQS_SUF{

private:

	typedef Pair<alphabet_type, offset_type> pqs_element_type; ///< (ch, pos)

	typedef Pair<alphabet_type, uint32> pq1_element_type; ///< (ch, max - block_id) 

	typedef TupleAscCmp2<pq1_element_type> pq1_comparator_type; ///< comparator for pq1

	typedef	std::priority_queue<pq1_element_type, std::vector<pq1_element_type>, pq1_comparator_type> pq1_type; 

	typedef std::priority_queue<pq2_element_type, std::vector<pq2_element_type>, pq2_comparator_type> pq2_type;

	typedef Pair<alphabet_type, offset_type> block_element_type; ///< (ch, pos)

	typedef MyVector<block_element_type> block_type;

private:

	pq1_type *m_heap1; ///< a heap for sequentially retrieving the smallest from the blocks on EM

	pq2_type *m_heap2; ///< a heap for sequentially retrieving the smallest from those on RAM

	uint64 m_pq2_capacity;  ///< capacity for m_heap2 
	
	std::vector<block_type*> m_blocks; ///< store the substrs induced from the previously scanned name bucket (except for those on RAM)

	uint32 m_cur_block_idx;	

public:

	/// \brief ctor 
	///	
	PQS_SUF(const uint64 _avail_mem) {

		m_heap1 = new pq1_type();

		m_heap2 = new pq2_type();

		m_pq2_capacity = _avail_mem / sizeof(pq2_element_type);

		m_blocks.clear();
	}

	/// \brief dtor
	///
	~PQS_SUF() {

		delete m_heap1; m_heap1 = nullptr;

		delete m_heap2; m_heap2 = nullptr;
	}

	/// \brief check if empty
	///
	bool empty() {

		return (m_heap1->empty() && m_heap2->empty());
	}


	/// \brief get top operation
	///
	/// \note check if non-empty before calling the function
	pqs_element_type top() {

		if (!m_heap1->empty()) {

			if (!m_heap2->empty()) {

				if (m_heap1->top().first >= m_heap2->top().first) {

					m_cur_block_idx = std::numeric_limits<uint32>::max() - m_heap1->top().second; // block_idx

					return pqs_element_type(m_heap1->top().first, m_blocks[m_cur_block_idx]->get().second);
				}
				else {
	
					m_cur_block_idx = std::numeric_limits<uint32>::max();

					return pqs_element_type(m_heap2->top().first, m_heap2->top().third);
				}	
			}
			else {

				m_cur_block_idx = std::numeric_limits<uint32>::max() - m_heap1->top().second;

				return pqs_element_type(m_heap1->top().first, m_blocks[m_cur_block_idx]->get().second);
			}
		}
		else {

			m_cur_block_idx = std::numeric_limits<uint32>::max();

			return pqs_element_type(m_heap2->top().first, m_heap2->top().third);
		}
	}

	/// \brief pop operation
	///
	/// \note call top() before executing the function
	void pop() {

		if (m_cur_block_idx == std::numeric_limits<uint32>::max()) { // pop from heap2
		
			m_heap2->pop();
		}
		else { // pop from heap1

			m_blocks[m_cur_block_idx]->next_remove(), m_heap1->pop();

			if (!m_blocks[m_cur_block_idx]->is_eof()) { // if the block has more elements, push the leftmost into heap1

				m_heap1->push(pq1_element_type(m_blocks[m_cur_block_idx]->get().first, 
					std::numeric_limits<uint32>::max() - m_cur_block_idx)); 
			}
		}
	}

	/// \brief push operation
	///
	void push(const pq2_element_type & _value) {

		m_heap2->push(_value);

		if (m_heap2->size() == m_pq2_capacity) {

			m_blocks.push_back(new block_type());	
	
			while (!m_heap2->empty()) {

				pq2_element_type cur_str = m_heap2->top();

				m_blocks[m_blocks.size() - 1]->push_back(block_element_type(cur_str.first, cur_str.third));

				m_heap2->pop();
			}

			m_blocks[m_blocks.size() - 1]->start_read();

			m_heap1->push(pq1_element_type(m_blocks[m_blocks.size() - 1]->get().first, std::numeric_limits<uint32>::max() - m_blocks.size() + 1));
		}
	}
};

#endif // _PQ_SUB_H
