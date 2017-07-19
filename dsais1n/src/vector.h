//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Copyright (c) 2017, Sun Yat-sen University.
/// All rights reserved.
/// \file vector.h
/// \brief A self-defined external-memory vector designed for read/write operations I/O-efficiently. 
///
/// The vector is implemented by primitive I/O functions 
/// The vector provides interfaces for scanning elements rightward and leftward, but it doesn't support random access operations.
/// The vector supports two read modes: read-only and read-remove.
///
/// \author Yi Wu
/// \date 2017.7
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


#ifndef _VECTOR_H
#define _VECTOR_H

#include "common.h"
#include <fstream>
#include <cstdio>
#include <string>
#include "logger.h"

#define STATISTICS_COLLECTION

/// \brief Definition of a virtual vector consisting of one or multiple physical vectors.
///
template<typename element_type>
class MyVector{

private:

	uint64 m_size; ///< number of elements in the virtual vector

	uint64 m_read; ///< number of elements already read from the virtual vector

private:

	/// \brief self-defined RAM buffer
	/// 
	struct MyBuf{

	private:

		const uint32 m_capacity; ///< the capacity of the buffer, specified by VEC_BUF_RAM in common.h

		element_type *m_data; ///< handler to the payload of the buffer

		uint32 m_size;  ///< number of elements in the buffer

		uint32 m_read; ///< number of elements read from the buffer

	public:

		/// \brief ctor
		///
		MyBuf(): m_capacity(VEC_BUF_RAM / sizeof(element_type)) {

			m_data = new element_type[m_capacity]; // allocate RAM space for buffer
		}

		/// \brief dtor
		///
		~MyBuf() {

			delete[] m_data;
	
			m_data = nullptr;
		}

		/// \brief read a data block from the file
		///
		/// \param _file file handler
		/// \param _offset offset
		/// \param _num number of elements to be read
		void read_block(FILE*& _file, const long _offset, const uint32 _num) {

			fseek(_file, _offset * sizeof(element_type), SEEK_SET); // seek read position

			fread(m_data, sizeof(element_type), _num, _file); // sequentially read

#ifdef STATISTICS_COLLECTION

			Logger::addIV(_num * sizeof(element_type));
#endif

			m_size = _num;
		}

		/// \brief start read elements from the buffer
		/// 
		void start_read() {

			m_read = 0;
		}

		/// \brief check if all the elements are read already
		///
		bool is_eof() const{

			return m_size == m_read;		
		}
	
		/// \brief get an element from the buffer (forwardly)
		///
		/// \note check if eof before calling this function
		const element_type& get() const{

			return m_data[m_read];
		}

		/// \brief move forwardly
		///
		void next() {

			++m_read; // move to next element
		}

		/// \brief get an element from the buffer (reversely) 
		///
		const element_type& get_reverse() const{
 
			return m_data[m_size - 1 - m_read]; // e.g., m_read = 0, then read m_data[m_size - 1] from the buffer
		}

		/// \brief move reversely
		///
		void next_reverse() {

			++m_read; // call next()
		}

			
		/// \brief put elements into the buffer
		void start_write() {

			m_size = 0; 
		}
	
		/// \brief check if the buffer is already full 
		///
		bool full() const{

			return m_size == m_capacity;
		}
	
		/// \brief put an element to the buffer
		///
		/// \note check if full() before calling this funciton
		void put(const element_type& _elem) {

			m_data[m_size++] = _elem;
		}
		
		/// \brief write a data block to the file 
		///
		/// \note sequentially write into the block
		void write_block(FILE*& _file) {
	
			fwrite(m_data, sizeof(element_type), m_size, _file);

#ifdef STATISTICS_COLLECTION

			Logger::addPDU(m_size * sizeof(element_type));

			Logger::addOV(m_size * sizeof(element_type));
#endif
		}


		/// \brief check if empty
		///
		bool empty() const{

			return m_size == 0;
		}

		/// \brief return capacity
		///
		const uint32& capacity() const{

			return m_capacity;
		}

		const uint32& size() const  {

			return m_size;
		}
	};	

	/// \brief self-define phyiscal vector
	///
	struct MyPhiVector{

	private:

		std::string m_fname; ///< name of the file associated with the vector
	
		FILE* m_file; ///< handler

		const uint32 m_capacity; ///< capacity of the vector

		uint32 m_size; ///< number of elements in the vector

		uint32 m_read; ///< number of elements already read from the vector
		
		MyBuf*& m_buf; ///< a RAM buffer for facilitating I/O operations on the vector

	public:

		/// \brief ctor
		///
		MyPhiVector(MyBuf*&_buf) : m_capacity(PHI_VEC_EM / sizeof(element_type)), m_buf(_buf) {

			m_fname = "tmp_dsais1n_" + std::to_string(global_file_idx) + ".dat";
	
			//std::cerr <<"tmp file name: " << m_fname << std::endl;

			++global_file_idx; // plus one each time to keep unique
		}

		/// \brief prepare for writing
		///
		void start_write() {

			m_file = fopen(m_fname.c_str(), "wb");
		
			m_size = 0;

			m_buf->start_write();	
		}

		/// \brief check if the vector is full
		///
		bool full() const{

			return m_size == m_capacity;
		}

		/// \brief put an element into the vector
		///
		/// \note check if full before calling this function
		void push_back(const element_type& _elem) {

			if (m_buf->full()) { // buffer is full

				m_buf->write_block(m_file);

				m_buf->start_write(); // clear up the buffer
			}

			m_buf->put(_elem), ++m_size; /// put an element into the buffer
		}
		
		/// \brief ending writing process
		///
		/// \note call the function after the vector is full
		void end_write() {

			if (!m_buf->empty()) { // flush the remaining elements in the buffer

				m_buf->write_block(m_file);
			}

			fclose(m_file); // close file
		}

		/// \brief prepare for reading forwardly
		///
		void start_read() {

			m_file = fopen(m_fname.c_str(), "rb");
	
			m_read = 0;

			m_buf->read_block(m_file, m_read, std::min(m_size - m_read, m_buf->capacity()));  
			
			m_buf->start_read();
		}

		/// \brief check if no more to read
		///
		bool is_eof() {

			return m_read == m_size;
		}
		
		/// \brief prepare for reading reversely
		///
		void start_read_reverse() {

			//std::cerr << "file name to open: " << m_fname << std::endl;

			m_file = fopen(m_fname.c_str(), "rb");

			m_read = 0;

			//std::cerr << "m_size: " << m_size << " m_read: " << m_read << " m_capacity: " << m_capacity << std::endl;

			m_buf->read_block(m_file, m_size - m_read - std::min(m_size - m_read, m_buf->capacity()), std::min(m_size - m_read, m_buf->capacity()));

			//std::cerr << "m_size: " << m_size << " m_read: " << m_read << " m_capacity: " << m_capacity << std::endl;

			//std::cerr << "start_read\n";

			m_buf->start_read();
		}

		/// \brief get an element from the vector forwardly
		///
		/// \note check if eof before calling the function
		const element_type& get() const{
	
			return m_buf->get();
		}

		/// \brief get an element from the vector reversely
		///
		/// \note check if eof before calling the function
		const element_type& get_reverse() const {

			return m_buf->get_reverse();
		}


		/// \brief move to next element forwardly
		///
		void next() {

			++m_read, m_buf->next();

			if (m_buf->is_eof()) { // the elements in the buffer are already processed

				m_buf->read_block(m_file, m_read, std::min(m_size - m_read, m_buf->capacity()));

				m_buf->start_read();
			}		
		}

		/// \brief move to next element reversely
		///
		void next_reverse() {

			++m_read, m_buf->next_reverse();

			if (m_buf->is_eof()) { // the elements in the buffer are already processed

				m_buf->read_block(m_file, m_size - m_read - std::min(m_size - m_read, m_buf->capacity()), std::min(m_size - m_read, m_buf->capacity()));

				m_buf->start_read();
			}
		}

		/// \brief finish reading
		///
		/// \note call the function after is_eof() == true
		void end_read() {

			fclose(m_file);
		}

		/// \brief remove the physical file 
		///
		void remove_file() {

			std::remove(m_fname.c_str());	

#ifdef STATISTICS_COLLECTION
			Logger::delPDU(m_size * sizeof(element_type));
#endif
		}

		/// \brief get m_size
		///
		const uint32& size() const{

			return m_size;
		}

		const uint32& capacity() const {

			return m_capacity;
		}
		
	};

private:

	MyBuf* m_buf; ///< handler to RAM buffer

	std::vector<MyPhiVector*> m_phi_vectors; ///< handlers to physical vectors 

	uint32 m_phi_vector_write_idx; ///< index of the vector being written

	uint32 m_phi_vector_read_idx; ///< index of the vector being read
	
	bool m_flag; ///< set false if changing from writting to reading 

public:

	/// \brief ctor
	///
	MyVector() {

		m_buf = new MyBuf(); // create the RAM buffer

		m_phi_vectors.clear();

		start_write();

		m_flag = false;
	}	

	/// \brief dtor
	/// 
	~MyVector() {

		delete m_buf;

		//std::cerr << "herere1";

		m_buf = nullptr;

		//std::cerr << "herere1";
	}

	/// \brief start writting
	///
	void start_write() {

		m_size = 0;

		m_phi_vectors.push_back(new MyPhiVector(m_buf)); // create a physical vector at the beginning

		m_phi_vector_write_idx = 0;

		m_phi_vectors[m_phi_vector_write_idx]->start_write();
	}
	
	/// \brief check if the vector is empty	
	///
	bool is_empty() {
	
		return m_size == 0;
	}

	/// \brief append an element to the end of the virtual vector
	///
	/// \note 		
	void push_back(const element_type & _value) {
	
		if (m_phi_vectors[m_phi_vector_write_idx]->full()) {

			m_phi_vectors[m_phi_vector_write_idx]->end_write();

			m_phi_vectors.push_back(new MyPhiVector(m_buf));

			++m_phi_vector_write_idx;

			m_phi_vectors[m_phi_vector_write_idx]->start_write();
		}
	
		m_phi_vectors[m_phi_vector_write_idx]->push_back(_value), ++m_size;

	//	std::cerr << "phi idx: " << m_phi_vector_write_idx << " m_size: " << m_size << std::endl;
	}

	/// \brief finish writing the virtual vector
	///
	void end_write() {

		m_phi_vectors[m_phi_vector_write_idx]->end_write(); // finish writing the last physical vector
	}

	/// \brief start reading the virtual vector forwardly
	///
	void start_read() {

		if (m_flag == false) {

			end_write(); // finish writing the final vector

			m_flag = true;
		}

		m_read = 0;

		m_phi_vector_read_idx = 0;

		m_phi_vectors[m_phi_vector_read_idx]->start_read();
	}

	/// \brief start reading the virtual vector reversely 
	///
	void start_read_reverse() {

		if (m_flag == false) {

			end_write();

			m_flag = true;
		}

		m_read = 0;

		m_phi_vector_read_idx = m_phi_vectors.size() - 1;

		m_phi_vectors[m_phi_vector_read_idx]->start_read_reverse();
	}

	/// \brief check if all the elements are already read
	///
	bool is_eof() {

		return m_size == m_read;
	}

	/// \brief get an element from the virtual vector forwardly
	///
	const element_type& get() const{

		return m_phi_vectors[m_phi_vector_read_idx]->get();
	}

	/// \brief get an element from the virtual vector reversely
	///
	const element_type& get_reverse() const {

		return m_phi_vectors[m_phi_vector_read_idx]->get_reverse();
	}

	/// \brief move to next + remove
	///
	void next_remove() {

		++m_read, m_phi_vectors[m_phi_vector_read_idx]->next();

		if (m_phi_vectors[m_phi_vector_read_idx]->is_eof()) {

			m_phi_vectors[m_phi_vector_read_idx]->end_read();

			m_phi_vectors[m_phi_vector_read_idx]->remove_file();

			delete m_phi_vectors[m_phi_vector_read_idx];

			m_phi_vectors[m_phi_vector_read_idx] = nullptr;

			if (!is_eof()) {

				++m_phi_vector_read_idx;
			
				m_phi_vectors[m_phi_vector_read_idx]->start_read();
			}
		}
	}

	/// \brief move to next element
	///
	void next() {

		++m_read, m_phi_vectors[m_phi_vector_read_idx]->next();

		if (m_phi_vectors[m_phi_vector_read_idx]->is_eof()) {

			m_phi_vectors[m_phi_vector_read_idx]->end_read();

			if (!is_eof()) {

				++m_phi_vector_read_idx;
			
				m_phi_vectors[m_phi_vector_read_idx]->start_read();
			}
		}
	}

	/// \brief move to next + remove +reverse
	///
	void next_remove_reverse() {

		++m_read, m_phi_vectors[m_phi_vector_read_idx]->next_reverse();
	
		if (m_phi_vectors[m_phi_vector_read_idx]->is_eof()) {

			m_phi_vectors[m_phi_vector_read_idx]->end_read();

			m_phi_vectors[m_phi_vector_read_idx]->remove_file();

			delete m_phi_vectors[m_phi_vector_read_idx];

			m_phi_vectors[m_phi_vector_read_idx] = nullptr;

			if (!is_eof()) {

				--m_phi_vector_read_idx;
			
				m_phi_vectors[m_phi_vector_read_idx]->start_read_reverse();
			}
		}
	}

	/// \brief next + reverse
	///
	void next_reverse() {

		++m_read, m_phi_vectors[m_phi_vector_read_idx]->next_reverse();
	
		if (m_phi_vectors[m_phi_vector_read_idx]->is_eof()) {

			m_phi_vectors[m_phi_vector_read_idx]->end_read();

			if (!is_eof()) {

				--m_phi_vector_read_idx;
			
				m_phi_vectors[m_phi_vector_read_idx]->start_read_reverse();
			}
		}
	}

	/// \brief report status
	///
	void report() {

		std::cerr << "number of phi vecs: " << m_phi_vectors.size() << std::endl;
	}

	/// \brief return size
	///
	uint64 size() const {

		return m_size;
	}
};

#endif
