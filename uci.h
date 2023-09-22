#pragma once
#include "variables.h"
#include "search.h"
#include <sstream>

namespace ZeroLogic {

	class UCI {

	public:

		void loop(int argc, char* argv[]);

	private:

		std::string engine_info = "id name ZeroLogic v3.0.1\nid author wurm\n";
		std::string start_fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

		void position(std::istringstream& is, SearchInstance* SI);

		void go(std::istringstream& is, SearchInstance* SI);

	};

}