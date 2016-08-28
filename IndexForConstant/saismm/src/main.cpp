#include "namespace.h"
#include "mycommon.h"
#include "saismm.h"
#include <string>

using namespace wtl;
int main(int argc, char ** argv) {
	std::string corpora_input(argv[1]);
	std::string corpora_output(std::string(argv[1]).append(".sa"));

	SAISComputation<uint8>(corpora_input, 255, corpora_output);

	return 0;
}


