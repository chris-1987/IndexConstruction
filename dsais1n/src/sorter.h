/////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Copyright (c) 2017, Sun Yat-sen University.
/// All rights reserved.
/// \file sorter.h
/// \brief A self-defined external-memory sorter designed for implementing space-efficient I/O operations.
///
/// The sorter is implemented by stxxl::vectors. 
/// It splits the received elements into blocks and sort each block separately, where each block is 
/// stored into the external memory. Afterward, it maintains a multi-way merger in RAM to 
/// sequentially retrieve the smallest among the blocks.
///
/// \author Yi Wu
/// \date 2017.5
/////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef _SORTER_H
#define _SORTER_H

#include "vector.h"

/// \brief definition of I/O operations on self-defined disk-based sorter
/// 
/// All the elements are naturally splitted into multiple block, elements in each block are sorted in RAM and forward to external memory.
/// The block-wise results are stored in self-defined disk-based vectors
/// A multi-way merger is applied to sequentially retrieving the smallest elements among all those unvisited in the blocks.
template<typename element_type, typename comparator_type>
class MySorter{

private:

	typedef std::pair<element_type, uint32> pq_element_type; ///< (element, block_idx)

public:

	/// \brief a comparator for comparing items in m_ram_pq
	///
	/// \note it is assumed that _a.first and _b.first must be different
	struct PQComparator{

		int operator()(const pq_element_type & _a, pq_element_type & _b) {

			static comparator_type cmp = comparator_type();

			return !cmp(_a.first, _b.first); ///< only compare their first components
		}
	};

private:

	std::priority_queue<pq_element_type, std::vector<pq_element_type>, PQComparator> m_ram_pq; ///< RAM merger 

private:

	typedef MyVector<element_type> element_vector_type;

	const uint64 m_vector_capacity; ////< capacity for each vector

	std::vector<element_vector_type*> m_em_vectors; ///< number of vectors 
		
	std::vector<element_type>* m_ram_vector; ///< a ram vector

	uint64 m_size; ///< number of elements to be sorted (including all the elements in m_em_vectors)

public:

	/// \brief ctor
	///
	/// \param _avail_mem RAM for a block
	MySorter(const uint64 _avail_mem) : m_vector_capacity(_avail_mem / sizeof(element_type)){

		m_em_vectors.clear();

		m_ram_vector = new std::vector<element_type>();

		m_ram_vector->clear();

		m_size = 0;
	}

	/// \brief push an element into the sorter
	///
	/// Push into the internal-memory vector first, perform sorting if the vector is full.
	/// Redirect the sorted elements to external memory
	void push(const element_type & _value) {

		if (m_ram_vector->size() == m_vector_capacity) {
		
			std::sort(m_ram_vector->begin(), m_ram_vector->end(), comparator_type());

			m_em_vectors.push_back(new element_vector_type());

			for (uint64 i = 0; i < m_ram_vector->size(); ++i) {

				m_em_vectors[m_em_vectors.size() - 1]->push_back((*m_ram_vector)[i]);
			}

			m_em_vectors[m_em_vectors.size() - 1]->finish();

			m_size += m_ram_vector->size();

			m_ram_vector->clear();
		}

		m_ram_vector->push_back(_value);
	}
	
	/// \brief prepare an internal-memory merger to compare the smallest among all the unvisited elements 
	///
	void sort() {

		// create an em vector for storing the remaing elements in the ram vector
		if (m_ram_vector->size() != 0) {

			std::sort(m_ram_vector->begin(), m_ram_vector->end(), comparator_type());

			m_em_vectors.push_back(new element_vector_type());

			for (uint64 i = 0; i < m_ram_vector->size(); ++i) {

				m_em_vectors[m_em_vectors.size() - 1]->push_back((*m_ram_vector)[i]);
			}

			m_em_vectors[m_em_vectors.size() - 1]->finish();

			m_size += m_ram_vector->size();

			m_ram_vector->clear();
		}

		// deallocate the RAM space for the ram vector	
		delete m_ram_vector; m_ram_vector = nullptr;

		// initialize the internal pq
		for (uint32 block_idx = 0; block_idx < m_em_vectors.size(); ++block_idx) {

			m_em_vectors[block_idx]->start_read();
	
			m_ram_pq.push(pq_element_type(m_em_vectors[block_idx]->get(), block_idx));

			m_em_vectors[block_idx]->next_remove();
		}	
	}

	/// \brief report statistics
	///
	void report() {

		std::cerr << "m_size: " << m_size << std::endl;

		for (uint32 i = 0; i < m_em_vectors.size(); ++i) {

			std::cerr << "logical vector: " << i << std::endl;

			m_em_vectors[i]->report();
		}
	}

	/// \brief get the top element from the ram heap
	///
	const element_type& operator*() {

		return m_ram_pq.top().first;
	}

	/// \brief pop the top element in the ram pq and push the next element belonging to the same vector 
	/// 
	void operator++() {

		uint32 block_idx = m_ram_pq.top().second;

		m_ram_pq.pop();

		if (!m_em_vectors[block_idx]->is_eof()) {

			m_ram_pq.push(pq_element_type(m_em_vectors[block_idx]->get(), block_idx));

			m_em_vectors[block_idx]->next_remove();
		}
	}

	/// \brief check if the sorter is empty 
	///
	/// If m_ramp_pq is empty, then the sorter is also empty
	bool empty() {

		return m_ram_pq.empty();
	}	


	/// \brief return the number of elements in the sorter
	///
	uint64 size() {

		return m_size; 
	}
	
};

#endif // _SORTER_H

