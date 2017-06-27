//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Copyright (c) 2017, Sun Yat-sen university.
/// All rights reserved.
/// \file pq_sub.h
/// \brief Self-defined external-memory priority queue designed for sorting substrs in an I/O-efficient way.
///  
/// The priority queue is implemented by MyVector.
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

	typedef Pair<alphabet_type, offset_type> pql_element_type; ///< (ch, pos)

	typedef Pair<alphabet_type, uint32> pq1_element_type; ///< (ch, block_id) 

	typedef TupleDscCmp2<pq1_element_type> pq1_comparator_type; // compare tuples by their first two elements in descending order 

	typedef	std::priority_queue<pq1_element_type, std::vector<pq1_element_type>, pq1_comparator_type> pq1_type; 

	typedef std::priority_queue<pq2_element_type, std::vector<pq2_element_type>, pq2_comparator_type> pq2_type;

	typedef Triple<alphabet_type, offset_type, bool> vec1_element_type; ///< (ch, pos, diff)

	typedef MyVector<vec1_element_type> vec1_type; ///< elements for externl-memory blocks

private:

	pq1_type *m_heap1; ///< a heap for sequentially retrieving the smallest among the blocks residing on EM

	pq2_type *m_heap2; ///< a heap for sequentially retrieving the smallest among those residing on RAM

	pq2_type *m_heap3; ///< an auxiliary heap for storing the substrs induced from currently scanned name bucket

	uint64 m_pq2_capacity;  ///< capacity for both m_heap2 and m_heap3
	
	std::vector<vec1_type*> m_blocks; ///< external-memory blocks

	std::vector<offset_type> m_blocks_start_suc_name; ///< indicate the name of the successor for the leftmost substr in the block

	uint32 m_cur_block_idx; ///< the block idx of currently scanned L-type substr

	uint32 m_pre_block_idx; ///< the block idx of previously scanned L-type substr

	alphabet_type m_cur_ch; ///< the heading char of currently scanned L-type substr

	alphabet_type m_pre_ch; ///< the heading char of previously scanned L-type substr

	offset_type m_cur_suc_name; ///< the suc-name of currently scanned L-type substr

	offset_type m_pre_suc_name; ///< the suc-name of previously scanned L-type substr

	bool m_cur_diff; ///< indicate whether or not the currently and previously scanned two L-type elements are equal or not

	bool m_heap3_was_full; ///< indicate whether or not m_heap3 is full during the scan of each name bucket

public:

	/// \brief ctor 
	///	
	/// \param _avail_mem available memory for m_heap2 and m_heap3
	PQL_SUB(const uint64 _avail_mem) {

		m_heap1 = new pq1_type();

		m_heap2 = new pq2_type();

		m_heap3 = new pq2_type();

		m_pq2_capacity = _avail_mem / 2 /sizeof(pq2_element_type);

		m_blocks.clear();

		m_blocks_start_suc_name.clear();		

		m_pre_block_idx = std::numeric_limits<uint32>::max() - 1; // assumed to be max - 1

		m_pre_ch = 0; // assumed to be 0 initially, cause any L-type substr starts with a character > 0

		m_pre_suc_name = std::numeric_limits<offset_type>::max(); // assumed to be max initially

		m_heap3_was_full = false; // assumed to be false initially
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
	/// When scanning an L-type name bucket, we check if empty before calling top(). A name bucket must be in heap1 or heap2.
	/// After the scan, we flush the substrs in heap1 and heap2 into a block residing on the disk.
	bool empty() {

		return (m_heap1->empty() && m_heap2->empty());
	}


	/// \brief get top element from pql
	///
	pql_element_type top() {

		if (!m_heap1->empty() && !m_heap2->empty()) { // both are non-empty

			if (m_heap1->top().first <= m_heap2->top().first) { // suc_names of those in heap1 must be smaller than those in heap2

				const pq1_element_type & cur_str = m_heap1->top();

				m_cur_block_idx = cur_str.second; // block_idx

				m_cur_ch = cur_str.first;

				m_cur_diff = m_blocks[m_cur_block_idx]->get().third; // if cur and pre are from the same block, check this field

				return pql_element_type(m_cur_ch, m_blocks[m_cur_block_idx]->get().second); // (ch, pos)
			}
			else {

				const pq2_element_type & cur_str = m_heap2->top();
				
				m_cur_block_idx = std::numeric_limits<uint32>::max();

				m_cur_ch = cur_str.first;

				m_cur_suc_name = cur_str.second; // if cur and pre are from heap2, check this field

				return pql_element_type(m_cur_ch, cur_str.third); // (ch, pos)
			}
		}
		else {

			if (!m_heap1->empty()) { // m_heap2 is empty

				const pq1_element_type & cur_str = m_heap1->top();

				m_cur_block_idx = cur_str.second;

				m_cur_ch = cur_str.first;

				m_cur_diff = m_blocks[m_cur_block_idx]->get().third;

				return pql_element_type(m_cur_ch, m_blocks[m_cur_block_idx]->get().second);
			}
			else { // m_heap1 is empty

				const pq2_element_type & cur_str = m_heap2->top();

				m_cur_block_idx = std::numeric_limits<uint32>::max();

				m_cur_ch = cur_str.first;

				m_cur_suc_name = cur_str.second;

				return pql_element_type(m_cur_ch, cur_str.third);
			}
		}
	}

	/// \brief indicate if currently and prevously popped L-type substrs are different or not
	///
	/// case (1), both from a block in m_heap1, then check m_cur_diff;
	/// case (2), both from heap2, then check suc_name and head char
	/// case (3), one from heap2 and the other from heap1, then they are different
	/// case (4), two from different blocks, then check suc_name and their head char
	bool is_diff() {

		if (m_pre_block_idx == m_cur_block_idx) { 

			if (m_cur_block_idx == std::numeric_limits<uint32>::max()) { // case (2)

				return !(m_pre_suc_name == m_cur_suc_name && m_pre_ch == m_cur_ch); 
			}
			else { // case (1)
					
				return m_cur_diff;	
			}
		}
		else { 

			if (m_cur_block_idx == std::numeric_limits<uint32>::max() || 
				m_pre_block_idx == std::numeric_limits<uint32>::max()) { // case (3)

				return true;
			}
			else { // case (4)

				if (m_blocks_start_suc_name[m_cur_block_idx] != m_blocks_start_suc_name[m_pre_block_idx]) { // different

					return true;
				}
				else { // successors' names are equal, check their heading characters

					return !(m_cur_ch == m_pre_ch); 
				}
			}
		}

	}

	/// \brief pop the smallest element
	///
	void pop() {

		if (m_cur_block_idx == std::numeric_limits<uint32>::max()) { // pop from heap2
		
			m_pre_block_idx = m_cur_block_idx, m_pre_suc_name = m_cur_suc_name, m_pre_ch = m_cur_ch, m_heap2->pop();

		}
		else { // pop from heap1

			m_pre_block_idx = m_cur_block_idx, m_pre_ch = m_cur_ch, m_blocks[m_pre_block_idx]->next_remove(), m_heap1->pop();

			if (!m_blocks[m_pre_block_idx]->is_eof()) { // if the block has more elements, push the leftmost into heap1

				m_heap1->push(pq1_element_type(m_blocks[m_pre_block_idx]->get().first, m_pre_block_idx)); 
			}
		}
	}

	/// \brief push operation
	///
	/// if the substr to be pushed is induced from lms_sorter or heap1, then insert it into heap3; otherwise insert it into heap2
	/// \note during the scan of a bucket in heap2, we pop one from the heap and push its preceding into the heap. Thus, the 
	/// heap won't exceed its capacity
	void push(const pq2_element_type & _value, const bool _induced_from_lms) {

		if (_induced_from_lms == true || m_cur_block_idx != std::numeric_limits<uint32>::max()) { // push into heap3

			if (m_heap3->size() == m_pq2_capacity) {

				m_heap3_was_full = true;

				// flush m_heap2
				if (!m_heap2->empty()) {
		
					m_blocks.push_back(new vec1_type());	

					m_blocks_start_suc_name.push_back(m_heap2->top().second);

					pq2_element_type pre_str(0, 0, 0);
	
					while (!m_heap2->empty()) {

						pq2_element_type cur_str = m_heap2->top();

						m_blocks[m_blocks.size() - 1]->push_back(vec1_element_type(cur_str.first, cur_str.third, 
							(cur_str.first == pre_str.first && cur_str.second == pre_str.second) ? false : true));

						pre_str = cur_str, m_heap2->pop();
					}

					m_blocks[m_blocks.size() - 1]->start_read();

					m_heap1->push(pq1_element_type(m_blocks[m_blocks.size() - 1]->get().first, m_blocks.size() - 1));
				}
			
				// flush m_head3
				m_blocks.push_back(new vec1_type());

				m_blocks_start_suc_name.push_back(m_heap3->top().second);

				pq2_element_type pre_str(0, 0, 0);

				while (!m_heap3->empty()) {

					pq2_element_type cur_str = m_heap3->top();

					m_blocks[m_blocks.size() - 1]->push_back(vec1_element_type(cur_str.first, cur_str.third, 
						(cur_str.first == pre_str.first) ? false : true)); // suc_names are all equal	

					pre_str = cur_str, m_heap3->pop();
				}	

				m_blocks[m_blocks.size() - 1]->start_read();

				m_heap1->push(pq1_element_type(m_blocks[m_blocks.size() - 1]->get().first, m_blocks.size() - 1));
			}

			m_heap3->push(_value);
		}		
		else { // pop from heap2, then push into heap2, the total number remains unchanged, won't exceed the capacity

			m_heap2->push(_value);
		}
	}

	/// \brief after scanning current name bucket, flush m_heap3 
	///
	void flush() {

		if (m_heap3_was_full == true) { // heap2 must be empty currently, flush heap3 into EM

			if (!m_heap3->empty()) {

				m_blocks.push_back(new vec1_type());

				m_blocks_start_suc_name.push_back(m_heap3->top().second);

				pq2_element_type pre_str(0, 0, 0);

				while (!m_heap3->empty()) {

					pq2_element_type cur_str = m_heap3->top();

					m_blocks[m_blocks.size() - 1]->push_back(vec1_element_type(cur_str.first, cur_str.third, 
						(cur_str.first == pre_str.first) ? false : true)); // suc_names are all equal	

					pre_str = cur_str, m_heap3->pop();
				}	

				m_blocks[m_blocks.size() - 1]->start_read();

				m_heap1->push(pq1_element_type(m_blocks[m_blocks.size() - 1]->get().first, m_blocks.size() - 1));
			}

			m_heap3_was_full = false; 	
		}
		else if (m_heap3->size() + m_heap2->size() <= m_pq2_capacity) { // move elements from heap3 to heap2

			while (!m_heap3->empty()) {

				m_heap2->push(m_heap3->top());

				m_heap3->pop();
			}
		}	
		else { // exceed the capacity, both contain elements, flush elements in heap2 and heap3 into EM

			m_blocks.push_back(new vec1_type());

			alphabet_type cur_bkt = m_heap2->top().first;

			m_blocks_start_suc_name.push_back(m_heap2->top().second);
				
			if (m_heap3->top().first < cur_bkt) {

				cur_bkt = m_heap3->top().first;

				m_blocks_start_suc_name[m_blocks.size() - 1] = m_heap3->top().second;
			}
	
			alphabet_type pre_ch = 0;

			offset_type pre_suc_name = 0;

			while (true) {
				
				while (!m_heap2->empty() && m_heap2->top().first == cur_bkt) {

					const pq2_element_type & cur_str = m_heap2->top();

					m_blocks[m_blocks.size() - 1]->push_back(vec1_element_type(cur_str.first, cur_str.third, 
						(pre_ch == cur_str.first && pre_suc_name == cur_str.second) ? false : true));
	
					pre_ch = cur_str.first, pre_suc_name = cur_str.second, m_heap2->pop();
				}

				while (!m_heap3->empty() && m_heap3->top().first == cur_bkt) {

					const pq2_element_type & cur_str = m_heap3->top();

					m_blocks[m_blocks.size() - 1]->push_back(vec1_element_type(cur_str.first, cur_str.third, 
						(pre_ch == cur_str.first && pre_suc_name == cur_str.second) ? false : true));							
					pre_ch = cur_str.first, pre_suc_name = cur_str.second, m_heap3->pop();
				}

				if (!m_heap2->empty()) {

					cur_bkt = m_heap2->top().first;

					if (!m_heap3->empty() && m_heap3->top().first < cur_bkt) cur_bkt = m_heap3->top().first;
				}
				else if (!m_heap3->empty()){

					cur_bkt = m_heap3->top().first;
				}
				else {
				
					break;
				}
			}
		
			m_blocks[m_blocks.size() - 1]->start_read();

			m_heap1->push(pq1_element_type(m_blocks[m_blocks.size() - 1]->get().first, m_blocks.size() - 1));
		}
	}
};


/// \brief a priority queue for sorting S-type substrs.
///
template<typename alphabet_type, typename offset_type, typename pq2_element_type, typename pq2_comparator_type>
class PQS_SUB{

private:

	typedef Pair<alphabet_type, offset_type> pqs_element_type; ///< (ch, pos)

	typedef Pair<alphabet_type, uint32> pq1_element_type; ///< (ch, std::numeric_limits<uint32>::max() - block_id) 

	typedef TupleAscCmp2<pq1_element_type> pq1_comparator_type; ///< comparator for sorting by the first two components in ascending order

	typedef	std::priority_queue<pq1_element_type, std::vector<pq1_element_type>, pq1_comparator_type> pq1_type; 

	typedef std::priority_queue<pq2_element_type, std::vector<pq2_element_type>, pq2_comparator_type> pq2_type;

	typedef Triple<alphabet_type, offset_type, bool> vec1_element_type; ///< (ch, pos, diff)

	typedef MyVector<vec1_element_type> vec1_type;

private:

	pq1_type *m_heap1; ///< a heap for sequentially retrieving the smallest among those in the blocks on EM

	pq2_type *m_heap2; ///< a heap for sequentially retrieving the smallest among those on RAM

	pq2_type *m_heap3; ///< a heap for storing the substrs induced from currently scanned name bucket

	uint64 m_pq2_capacity;  ///< capacity for both m_heap2 and m_heap3
	
	std::vector<vec1_type*> m_blocks; ///< external-memory blocks

	std::vector<offset_type> m_blocks_start_suc_name; ///< suc name for the leftmost substr in the blocks

	uint32 m_cur_block_idx;	///< block idx of currently scanned S-type substr

	uint32 m_pre_block_idx; ///< block idx of previously scanned S-type substr

	alphabet_type m_cur_ch; ///< heading char of currently scanned S-type substr

	alphabet_type m_pre_ch; ///< heading char of previously scanned S-type substr

	offset_type m_cur_suc_name; ///< suc name of currently scanned S-type substr

	offset_type m_pre_suc_name; ///< suc name of previously scanned S-type substr

	bool m_cur_diff; ///< indicate whether or not two successively scanned S-type substrs are different or not

	bool m_heap3_was_full; ///< indicate whether or not m_heap3 get full during the scan of each bucket

public:

	/// \brief ctor 
	///	
	/// \param _avail_mem available memory for heap2 and heap3
	PQS_SUB(const uint64 _avail_mem) {

		m_heap1 = new pq1_type();

		m_heap2 = new pq2_type();

		m_heap3 = new pq2_type();

		m_pq2_capacity = _avail_mem / 2 /sizeof(pq2_element_type);

		m_blocks.clear();

		m_blocks_start_suc_name.clear();		

		m_pre_block_idx = std::numeric_limits<uint32>::max() - 1; 

		m_pre_ch = std::numeric_limits<alphabet_type>::max(); // any S-type substr starts with a char smallert than max

		m_pre_suc_name = 0; // no S-type substr has a successor named 0 

		m_heap3_was_full = false;
	}

	/// \brief dtor
	///
	~PQS_SUB() {

		delete m_heap1; m_heap1 = nullptr;

		delete m_heap2; m_heap2 = nullptr;

		delete m_heap3; m_heap3 = nullptr;
	}

	/// \brief check if empty
	///
	bool empty() {

		return (m_heap1->empty() && m_heap2->empty());
	}


	/// \brief get top substr from pqs
	///
	pqs_element_type top() {

		if (!m_heap1->empty() && !m_heap2->empty()) { // both are non-empty

			if (m_heap1->top().first >= m_heap2->top().first) { // top in heap1

				const pq1_element_type& cur_str = m_heap1->top();

				m_cur_block_idx = std::numeric_limits<uint32>::max() - cur_str.second; // block_idx, be careful

				m_cur_ch = cur_str.first; 

				m_cur_diff = m_blocks[m_cur_block_idx]->get().third; 

				return pqs_element_type(m_cur_ch, m_blocks[m_cur_block_idx]->get().second);
			}
			else { // top in heap2

				const pq2_element_type& cur_str = m_heap2->top();

				m_cur_block_idx = std::numeric_limits<uint32>::max();

				m_cur_ch = cur_str.first;

				m_cur_suc_name = cur_str.second;

				return pqs_element_type(m_cur_ch, cur_str.third);
			}
		}
		else { // only one is non-empty

			if (!m_heap1->empty()) { // m_heap2 is empty

				const pq1_element_type& cur_str = m_heap1->top();

				m_cur_block_idx = std::numeric_limits<uint32>::max() - cur_str.second;

				m_cur_ch = cur_str.first;

				m_cur_diff = m_blocks[m_cur_block_idx]->get().third;

				return pqs_element_type(m_cur_ch, m_blocks[m_cur_block_idx]->get().second);
			}
			else { // m_heap1 is empty

				const pq2_element_type& cur_str = m_heap2->top();

				m_cur_block_idx = std::numeric_limits<uint32>::max();

				m_cur_ch = cur_str.first;

				m_cur_suc_name = cur_str.second;

				return pqs_element_type(m_cur_ch, cur_str.third);
			}
		}
	}

	/// \brief indicate if currently and prevously popped L-type substrs are different or not
	///
	/// case (1), both from heap2, check their suc names and head chars
	/// case (2), both from a block, check m_cur_diff
	/// case (3), one from heap2 and the other from a block, must be different
	/// case (4), fom different blocks, check their sucnames and head chars
	bool is_diff() {

		if (m_pre_block_idx == m_cur_block_idx) {

			if (m_cur_block_idx == std::numeric_limits<uint32>::max()) { // case (1)

				return !(m_pre_suc_name == m_cur_suc_name && m_pre_ch == m_cur_ch); 
			}
			else { // case (2)
					
				return m_cur_diff;
			}
		}
		else { // case (3)

			if (m_cur_block_idx == std::numeric_limits<uint32>::max() || 
				m_pre_block_idx == std::numeric_limits<uint32>::max()) { // case (2), must be different

				return true;
			}
			else { // case (4)

				if (m_blocks_start_suc_name[m_cur_block_idx] != m_blocks_start_suc_name[m_pre_block_idx]) { // different

					return true;
				}
				else { // suc names are equal, check heading characters

					return !(m_cur_ch == m_pre_ch);
				}
			}
		}
	}

	/// \brief pop operation
	///
	/// \note call top() before executing the function
	void pop() {

		if (m_cur_block_idx == std::numeric_limits<uint32>::max()) { // pop from heap2
		
			m_pre_block_idx = m_cur_block_idx, m_pre_suc_name = m_cur_suc_name, m_pre_ch = m_cur_ch, m_heap2->pop();
		}
		else { // pop from heap1

			m_pre_block_idx = m_cur_block_idx, m_pre_ch = m_cur_ch, m_blocks[m_pre_block_idx]->next_remove(), m_heap1->pop();

			if (!m_blocks[m_pre_block_idx]->is_eof()) { // if more exists, keep a copy in m_heap1

				m_heap1->push(pq1_element_type(m_blocks[m_pre_block_idx]->get().first, 
					std::numeric_limits<uint32>::max() - m_pre_block_idx)); 
			}
		}
	}

	/// \brief push operation
	///
	void push(const pq2_element_type & _value, const bool _induced_from_lms) {

		if (_induced_from_lms == true || m_cur_block_idx != std::numeric_limits<uint32>::max()) { // push into heap3

			if (m_heap3->size() == m_pq2_capacity) {

				m_heap3_was_full = true;

				// flush m_heap2
				if (!m_heap2->empty()) {
		
					m_blocks.push_back(new vec1_type());	

					pq2_element_type pre_str(std::numeric_limits<alphabet_type>::max(), 0, 0);

					m_blocks_start_suc_name.push_back(m_heap2->top().second);
	
					while (!m_heap2->empty()) {

						const pq2_element_type& cur_str = m_heap2->top();

						m_blocks[m_blocks.size() - 1]->push_back(vec1_element_type(cur_str.first, cur_str.third, 
							(cur_str.first == pre_str.first && cur_str.second == pre_str.second) ? false : true));

						pre_str = cur_str, m_heap2->pop();
					}

					m_blocks[m_blocks.size() - 1]->start_read();

					m_heap1->push(pq1_element_type(m_blocks[m_blocks.size() - 1]->get().first, 
						std::numeric_limits<uint32>::max() - m_blocks.size() + 1));
				}

				// flush m_head3
				m_blocks.push_back(new vec1_type());

				pq2_element_type pre_str(std::numeric_limits<alphabet_type>::max(), 0, 0);

				m_blocks_start_suc_name.push_back(m_heap3->top().second);

				while (!m_heap3->empty()) {

					const pq2_element_type& cur_str = m_heap3->top();

					m_blocks[m_blocks.size() - 1]->push_back(vec1_element_type(cur_str.first, cur_str.third, 
						(cur_str.first == pre_str.first) ? false : true)); // suc names are equal	

					pre_str = cur_str, m_heap3->pop();
				}	

				m_blocks[m_blocks.size() - 1]->start_read();

				m_heap1->push(pq1_element_type(m_blocks[m_blocks.size() - 1]->get().first, 
					std::numeric_limits<uint32>::max() - m_blocks.size() + 1));
			}

			m_heap3->push(_value);
		}		
		else { // pop one and push one, the total number remains unchanged

			m_heap2->push(_value);
		}
	}

	/// \brief after scannin a name bucket, flush the internal memory if a new 
	///
	void flush() {

		if (m_heap3_was_full == true) {

			if (!m_heap3->empty()) {

				m_blocks.push_back(new vec1_type());

				pq2_element_type pre_str(std::numeric_limits<alphabet_type>::max(), 0, 0);

				m_blocks_start_suc_name.push_back(m_heap3->top().second);

				while (!m_heap3->empty()) {

					const pq2_element_type& cur_str = m_heap3->top();

					m_blocks[m_blocks.size() - 1]->push_back(vec1_element_type(cur_str.first, cur_str.third, 
						(cur_str.first == pre_str.first) ? false : true)); // suc names are equal	

					pre_str = cur_str, m_heap3->pop();
				}	

				m_blocks[m_blocks.size() - 1]->start_read();

				m_heap1->push(pq1_element_type(m_blocks[m_blocks.size() - 1]->get().first, 
					std::numeric_limits<uint32>::max() - m_blocks.size() + 1));
			}

			m_heap3_was_full = false; 	
		}
		else if (m_heap3->size() + m_heap2->size() <= m_pq2_capacity) { // move elements from heap3 to heap2

			while (!m_heap3->empty()) {

				m_heap2->push(m_heap3->top());

				m_heap3->pop();
			}
		}	
		else { // both cotain elements, flush elements in heap2 and heap3 into EM

			m_blocks.push_back(new vec1_type());

			alphabet_type cur_bkt;

			cur_bkt = m_heap2->top().first;

			m_blocks_start_suc_name.push_back(m_heap2->top().second);
				
			if (m_heap3->top().first > cur_bkt) {

				cur_bkt = m_heap3->top().first;
				
				m_blocks_start_suc_name[m_blocks.size() - 1] = m_heap3->top().second;
			}
	
			alphabet_type pre_ch = std::numeric_limits<alphabet_type>::max();

			offset_type pre_suc_name = 0;

			while (true) {
				
				while (!m_heap2->empty() && m_heap2->top().first == cur_bkt) {

					const pq2_element_type & cur_str = m_heap2->top();

					m_blocks[m_blocks.size() - 1]->push_back(vec1_element_type(cur_str.first, cur_str.third, 
						(pre_ch == cur_str.first && pre_suc_name == cur_str.second) ? false : true));		
	
					pre_ch = cur_str.first, pre_suc_name = cur_str.second, m_heap2->pop();
				}

				while (!m_heap3->empty() && m_heap3->top().first == cur_bkt) {

					const pq2_element_type & cur_str = m_heap3->top();

					m_blocks[m_blocks.size() - 1]->push_back(vec1_element_type(cur_str.first, cur_str.third, 
						(pre_ch == cur_str.first && pre_suc_name == cur_str.second) ? false : true));							
					pre_ch = cur_str.first, pre_suc_name = cur_str.second, m_heap3->pop();
				}

				if (!m_heap2->empty()) {

					cur_bkt = m_heap2->top().first;

					if (!m_heap3->empty() && m_heap3->top().first > cur_bkt) cur_bkt = m_heap3->top().first;
				}
				else if (!m_heap3->empty()){

					cur_bkt = m_heap3->top().first;
				}
				else {
				
					break;
				}
			}

			m_blocks[m_blocks.size() - 1]->start_read();

			m_heap1->push(pq1_element_type(m_blocks[m_blocks.size() - 1]->get().first, 
				std::numeric_limits<uint32>::max() - m_blocks.size() + 1));
		}
	}
};

#endif // _PQ_SUB_H
