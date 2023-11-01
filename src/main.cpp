#include "uci.h"
#include <chrono>

using namespace ZeroLogic;

int main(int argc, char* argv[]) {

	UCI::loop(argc, argv);

	std::cout << "God save the queen!\n";

}