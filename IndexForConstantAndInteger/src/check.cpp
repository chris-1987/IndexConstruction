#include "common.h"
#include "checker.h"

#include <iostream>
#include <chrono>
#include <fstream>

int main(int argc, char** argv){

	// mount stxxl disk	
	stxxl::config *cfg = stxxl::config::get_instance();

	stxxl::disk_config disk("stxxl.tmp", 1024 * 1024 * 1024, "syscall autogrow unlink");

	disk.direct = stxxl::disk_config::DIRECT_ON;

	cfg->add_disk(disk);

	// statistcs collection
	stxxl::stats *Stats = stxxl::stats::get_instance();

	stxxl::stats_data stats_begin(*Stats);

	stxxl::block_manager * bm = stxxl::block_manager::get_instance();
	
	// check if input params are legal
	if (argc != 3) {

		std::cerr << "two param required: input_path and output_path.\n";

		exit(-1);
	}	

	// retrieve file name for input string
	std::string s_fname(argv[1]); 
	
	// retrieve file name for output SA
	std::string sa_fname(argv[2]);

	// compute input string's size
	std::fstream s_stream(s_fname, std::fstream::in);

	s_stream.seekg(0, std::ios_base::end);

	uint64 s_size = s_stream.tellg();
	
	// check the SA for the given string
	Checker<uint8, uint40> checker(s_fname, sa_fname);	

	std::cerr << (checker.run() ? "right" : "wrong")<< std::endl;

	// output report
	std::cerr << (stxxl::stats_data(*Stats) - stats_begin);

	uint64 pdu = bm->get_maximum_allocation();

	std::cerr << "pdu\t total: " << pdu << "\t per char: " << (double)pdu / s_size << std::endl;

	uint64 io_volume = Stats->get_written_volume() + Stats->get_read_volume();

	std::cerr << "io\t total: " << io_volume << "\t per char: " << (double)io_volume / s_size << std::endl;


}
