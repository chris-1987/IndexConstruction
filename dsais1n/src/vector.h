//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Copyright (c) 2017, Sun Yat-sen University.
/// All rights reserved.
/// \file vector.h
/// \brief A self-defined external-memory vector designed for read/write operations I/O-efficiently. 
///
/// The vector is implemented by stxxl::vector, its elements are scatter among one or multiple stxxl::vector.
/// The vector provides interfaces for scanning elements rightward or leftward, but it does not support random access operations.
/// The vector supports two read modes: read-only and read-remove.
///
/// \author Yi Wu
/// \date 2017.5
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef _VECTOR_H
#define _VECTOR_H


#include "io.h"

/// \brief A logical vector consists of one or multiple physical vectors.
///
/// \note The capacity of a physical vector can be adjusted as required.
/// \param element_type type of the elements in the vector
template<typename element_type>
class MyVector{

private:

	typedef typename ExVector<element_type>::vector element_vector_type;

private:

	uint64 m_size; ///< size of the logical vector

	std::vector<element_vector_type*> m_vectors; ///< handlers to physical vectors 

	uint32 m_vector_write_idx; ///< indicate the vector to where put an element

	uint32 m_vector_read_idx; ///< indicate the vector from where get an element

	uint64 m_read; ///< number of read elements

	typename element_vector_type::bufwriter_type *m_buf_writer; ///< bufwriter for filling elements into the logical vector

	typename element_vector_type::bufreader_type *m_buf_reader; ///< bufreader for reading elements sequentially

	typename element_vector_type::bufreader_reverse_type *m_buf_rreader; ///< bureader for reading elements reversely


public:

	/// \brief ctor
	///
	/// \note when initializing the logical vector, create a physical vector at the same time 
	MyVector() {

		m_size = 0;

		m_vectors.clear();

		m_vectors.push_back(new element_vector_type());

		m_vectors[m_vectors.size() - 1]->resize(PHYSICAL_VECTOR_CAPACITY);

		m_buf_writer = new typename element_vector_type::bufwriter_type(*m_vectors[m_vectors.size() - 1]);

		m_vector_write_idx = 0;
	}

	/// \brief check if the logical file is empty
	///
	bool is_empty() {

		return m_size == 0;
	}
	
	/// \brief append an element to the end of the logical vector 
	/// 
	void push_back(const element_type & _value) {

		if (m_vectors[m_vector_write_idx]->size() == PHYSICAL_VECTOR_CAPACITY) {

			m_buf_writer->finish(); // close writer buffer for current phyiscal vector

			delete m_buf_writer; m_buf_writer = nullptr;

			m_vectors.push_back(new element_vector_type());

			m_vectors[m_vectors.size() - 1]->resize(PHYSICAL_VECTOR_CAPACITY);

			m_buf_writer = new typename element_vector_type::bufwriter_type(*m_vectors[m_vectors.size() - 1]);

			++m_vector_write_idx;
		}

		(*m_buf_writer) << _value;

		++m_size;
	}

	/// \brief finish pushing elements into the logical vector
	///
	void finish() {

		m_buf_writer->finish();

		delete m_buf_writer; m_buf_writer = nullptr;

		// shrink the last vector if necessary
		m_vectors[m_vectors.size() - 1]->resize(m_size - PHYSICAL_VECTOR_CAPACITY * (m_vectors.size() - 1));
	}

	/// \brief prepare for reading rightwardly
	///
	/// \note call the function before reading elements
	void start_read() {

		m_read = 0;

		m_vector_read_idx = 0;

		m_buf_reader = new typename element_vector_type::bufreader_type(*m_vectors[m_vector_read_idx]);
	}

	/// \brief prepare for reading leftwardly
	///
	/// \note call the function before reading elements reversely
	void start_read_reverse() {

		m_read = 0;

		m_vector_read_idx = m_vectors.size() - 1;

		m_buf_rreader = new typename element_vector_type::bufreader_reverse_type(*m_vectors[m_vector_read_idx]);
	}

	/// \brief has next to read
	///
	/// If the read pointer reaches the end of the logical file, return true; otherwise, return false.
	/// \note this function is applied to the situations when reading forwardly or reversely.
	bool is_eof() {

		return m_size == m_read;
	}

	/// \brief read an element from the logical vector (forwardly)
	///
	/// \note check is_eof() before calling the function
	const element_type & get() const {
		
		return **m_buf_reader;
	}	

	/// \brief read an element from the logical vector (reversely)
	///
	/// \note check is_eof() before calling the function
	const element_type& get_reverse() const {

		return **m_buf_rreader;
	}

	/// \brief forward + remove
	///
	void next_remove() {

		++m_read, ++m_buf_reader;

		if (m_buf_reader->empty()) { // current physical vector is read up

			delete m_vectors[m_vector_read_idx]; m_vectors[m_vector_read_idx] = nullptr;

			delete m_buf_reader; m_buf_reader = nullptr;

			if (!is_eof()) { // the logical vector is not read up, switch to the next physical vector

				++m_vector_read_idx;

				m_buf_reader = new typename element_vector_type::bufreader_type(*m_vectors[m_vector_read_idx]);
			}
		}
	}

	/// \brief forward
	///
	void next() {

		++m_read, ++m_buf_reader;

		if (m_buf_reader->empty()) { // current physical vector is read up

			delete m_buf_reader; m_buf_reader = nullptr;

			if (!is_eof()) { // the logical vector is not read up

				++m_vector_read_idx;

				m_buf_reader = new typename element_vector_type::bufreader_type(*m_vectors[m_vector_read_idx]);	
			}
		}
	}

	/// \brief backward + remove
	///
	void next_remove_reverse() {

		++m_read, ++m_buf_rreader;

		if (m_buf_rreader->empty()) {

			delete m_vectors[m_vector_read_idx]; m_vectors[m_vector_read_idx] = nullptr;

			delete m_buf_rreader; m_buf_rreader = nullptr;

			if (!is_eof()) {

				--m_vector_read_idx;

				m_buf_rreader = new typename element_vector_type::bufreader_reverse_type(*m_vectors[m_vector_read_idx]);
			}
		}
	}

	/// \brief backward
	///
	void next_reverse() {

		++m_read, ++m_buf_rreader;

		if (m_buf_rreader->empty()) {

			delete m_buf_rreader; m_buf_rreader = nullptr;

			if (!is_eof()) {

				--m_vector_read_idx;

				m_buf_rreader = new typename element_vector_type::bufreader_reverse_type(*m_vectors[m_vector_read_idx]);
			}
		}
	}

	/// \brief check status
	///
	void check_clear() {

		std::cerr << "vectors size: " << m_vectors.size() << std::endl;

		for (uint32 i = 0; i < m_vectors.size(); ++i) {

			if (m_vectors[i] != nullptr) std::cerr << "i is not nullptr\n";
		}
	}

	/// \brief report status
	///
	void report() {

		std::cerr << "number of physical vectors: " << m_vectors.size() << std::endl;

		for (uint32 i = 0; i < m_vectors.size(); ++i) {

			std::cerr << "elements in vector " << i << ": " << m_vectors[i]->size() << std::endl;
		}
	}

	/// \brief return size
	///
	uint64 size() const{

		return m_size;
	}
};

#endif // _IO_H
