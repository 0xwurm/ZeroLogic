#pragma once
#include "movegeneration.h"
#include "eval.h"
#include "misc.h"
#include <iostream>
#include <stack>
#include <chrono>


namespace ZeroLogic {

	using namespace Search;

	class SearchInstance {

	public:

		int horribleTimeVar{};

		SearchInstance(Gamestate* externalGamestate);

		void newsearch(SType mode, int depth);

		void newperft(int depth);

		int bestmove(Move m, int depth, int alpha, int beta, std::stack<Move>* &pv, std::stack<Move>* lowerPv); // pv doesnt need to be pointer to reference?

		int see(int exchangeIndex, Move m, int depth, int alpha, int beta, std::stack<Move>* &pv, std::stack<Move>* lowerPv); // "

		Gamestate gs{};

	private:

		int initBestmove(int depth, int alpha, int beta, std::stack<Move> pv, std::stack<Move>* lowerPv);

		void initID(int depth);

		void sort(std::stack<Move>* &pv);

		void perft(Move m, int depth);

		bool stopFlag = false;

		Move movelist[218]{};
		Move sortedMovelist[218]{};
		uint8_t movenum{};

		Bitboard originmask{};
		Bitboard destinationmask{};
		int movedpiece{};
		int capturedpiece{};
		uint8_t flagged{};
		uint8_t promotioncaptureflag{};
		Move castleFlag{};

		int lastIterationEval = 0;

		int blackConst{};
		int eblackConst{};

	};
}