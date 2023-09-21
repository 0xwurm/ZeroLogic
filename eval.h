#pragma once
#include "misc.h"

namespace ZeroLogic {

	using namespace Evaluation;

	class Eval {

	public:

		Eval(Gamestate* g);
		
		int get();

	private:

		Phase phase();

		int pieces();
		int pval();
		int positioning();
		template <Phase p>
		int pawn();
		template <Phase p>
		int knight();
		template <Phase p>
		int king();

		Gamestate* g{};
		Phase p{};

	};

}