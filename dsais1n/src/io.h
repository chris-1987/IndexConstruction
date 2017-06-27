#ifndef _IO_H
#define _IO_H

#include "common.h"

/// \brief stxxl vector generator
///
template<typename element_type>
struct ExVector{

	typedef typename stxxl::VECTOR_GENERATOR<element_type, 1, 2, K_512 * sizeof(element_type)>::result vector;
};


/// \brief stxxl sorter generator
///
template<typename tuple_type, typename comparator_type>
struct ExSorter{

	typedef typename stxxl::sorter<tuple_type, comparator_type> sorter;
};

#endif // _IO_H
