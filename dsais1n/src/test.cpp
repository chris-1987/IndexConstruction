#include "test.h"


//#define TEST_DEBUG1

int main() {


//	test_my_sorter();

//	exit(0);


	uint64 size = 1 * K_1024 * 100;

#ifdef TEST_DEBUG1

	stxxl::config *cfg = stxxl::config::get_instance();

	stxxl::disk_config disk("stxxl.tmp", 1024 * 1024 * 1024, "syscall autogrow unlink");

	disk.direct = stxxl::disk_config::DIRECT_ON;

	cfg->add_disk(disk);

	stxxl::stats *Stats = stxxl::stats::get_instance();

	stxxl::stats_data stats_begin(*Stats);

	stxxl::block_manager *bm = stxxl::block_manager::get_instance();

	//
	typedef typename ExVector<uint40>::vector uint40_vector_type;

	uint40_vector_type *vec1 = new uint40_vector_type();

	uint40_vector_type *vec2 = new uint40_vector_type();
	
	for (uint64 i = 0; i < size; ++i) {

		vec1->push_back(i);	
	}	

	for (uint64 i = 0; i < size; ++i) {

		vec2->push_back((*vec1)[i]);
	} 

	delete vec1; vec1 = nullptr;

	delete vec2; vec2 = nullptr;

	std::cerr << (stxxl::stats_data(*Stats) - stats_begin);

	uint64 pdu = bm->get_maximum_allocation();

	std::cerr << "pdu\t total: " << pdu << "\t per char: " << (double)pdu / size << std::endl;

	uint64 io_volume = Stats->get_written_volume() + Stats->get_read_volume();

	std::cerr << "io\t total: " << io_volume << "\t per char " << (double)io_volume / size;

#else

	stxxl::config *cfg = stxxl::config::get_instance();

	stxxl::disk_config disk("stxxl.tmp", 1024 * 1024 * 1024, "syscall autogrow unlink");

	disk.direct = stxxl::disk_config::DIRECT_ON;

	cfg->add_disk(disk);

	stxxl::stats *Stats = stxxl::stats::get_instance();

	stxxl::stats_data stats_begin(*Stats);

	stxxl::block_manager *bm = stxxl::block_manager::get_instance();

	typedef	MyVector<uint40> my_uint40_vector_type;

	my_uint40_vector_type *vec3 = new my_uint40_vector_type();

	my_uint40_vector_type *vec4 = new my_uint40_vector_type();

	for (uint64 i = 0; i < size; ++i) {

		vec3->push_back(i);	
	}	

	vec3->finish();

	std::cerr << "report vec3:\n";

	vec3->report();

	vec3->start_read();

	for (uint64 i =0; i < size; ++i, vec3->next_remove()) {

		vec4->push_back(vec3->get());
	} 

	vec4->finish();

	delete vec3; vec3 = nullptr;

	std::cerr << "report vec4:\n";

	vec4->start_read_reverse();

	for (uint64 i = 0; i < size; ++i, vec4->next_remove_reverse()) {

		vec4->get_reverse();
	}

	delete vec4; vec4 = nullptr;

	std::cerr << (stxxl::stats_data(*Stats) - stats_begin);

	uint64 pdu = bm->get_maximum_allocation();

	std::cerr << "pdu\t total: " << pdu << "\t per char: " << (double)pdu / size << std::endl;

	uint64 io_volume = Stats->get_written_volume() + Stats->get_read_volume();

	std::cerr << "io\t total: " << io_volume << "\t per char " << (double)io_volume / size;
#endif

}
