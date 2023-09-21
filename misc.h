#pragma once
#include "variables.h"
#include "gamestate.h"

namespace ZeroLogic {

	class Misc {

	public:

		std::string numToString(Move m);

		Move stringToNum(std::string m, Gamestate* gs);

		void makeZobristKeys();

	};
}
