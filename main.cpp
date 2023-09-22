#include "uci.h"

using namespace ZeroLogic;

int main(int argc, char* argv[]) {
	UCI uci;
	uci.loop(argc, argv);

	std::cout << "God save the queen!\n";

}