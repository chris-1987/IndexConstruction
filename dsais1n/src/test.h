////////////////////////////////////////////////////////////
/// Copyright (c) 2017, Sun Yat-sen University,
/// All rights reserved
/// \file test.h
/// \brief test functions
///
/// Test the program.
///
/// \author Yi Wu
/// \date 2017.5
///////////////////////////////////////////////////////////

#ifndef __TEST_H
#define __TEST_H

#include "vector.h"
#include "sorter.h"
#include "tuple.h"
#include "tuple_sorter.h"

/// \brief test_my_sorter
///
void test_my_sorter() {

	typedef Pair<uint40, uint32> pair_type;

	typedef TupleAscCmp1<pair_type> pair_comparator_type;

	MySorter<pair_type, pair_comparator_type> my_sorter(500);

	std::cerr << "create random data:\n";

	for (uint32 i = 0; i < 100; ++i) {

		uint40 first = i;

		uint32 second = i;

		my_sorter.push(pair_type(first, second));

		std::cerr << i <<"\t";

		std::cin.get();
	}
	
	std::cerr << "start sorting\n";

	my_sorter.sort();

	std::cerr << "report statistics\n";

	my_sorter.report();


	std::cerr << "output sorted data:\n";

	while (!my_sorter.empty()) {

		pair_type cur = *my_sorter;

		std::cerr << cur.first << " " << cur.second << std::endl;

		++my_sorter;
	}
}

#endif

