#pragma once
#include "gamestate.h"
#include "tables.h"

namespace ZeroLogic {

	using namespace Evaluation;

	class Eval {

	public:

		explicit Eval(Gamestate* g);
		
		int get();

	private:

		Phase phase();

		int pieces();
		int piece_value();
		int positioning();
		template <Phase p>
		int pawn();
		template <Phase p>
		int knight();
		template <Phase p>
		int bishop();
		template <Phase p>
		int king();

        Bitboard ring(bool do_white);

		Gamestate* g{};
		Phase p{};

	};

}