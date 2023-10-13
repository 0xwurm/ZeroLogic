#include "uci.h"
#include "tables.h"
#include <chrono>

using namespace ZeroLogic;

int main(int argc, char* argv[]) {

    Tables::fill();
	UCI::loop(argc, argv);

	std::cout << "God save the queen!\n";

}