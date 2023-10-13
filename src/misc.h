#pragma once
#include <sstream>

namespace ZeroLogic::Misc{
    std::string numToString(Move m);
    Move stringToNum(std::string m, Gamestate* gs);
}

