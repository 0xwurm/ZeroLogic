#pragma once
#include <vector>
#include "variables.h"

namespace ZeroLogic {

	using namespace State;

	class Gamestate {

	public:

		void makemove(Move m);
		bool stop();
		void fen(std::string FEN);
		// channel
		std::vector<std::vector<double>> vectorify();
		void hashPosition();

		Bitboard	position[12];
		Bitboard	enPassant;
		bool		castlingRights[4];
		bool		white;

		Bitboard	zobristHash;

	private:

		Piece getPiece(Bitboard* s, int startingPoint, Move m);
		void putPiece(Piece p, Bitboard* s, int sI);
		void flipCastles(castleType c);
		void moveRights(Piece p, Bitboard* s, Move m, Bitboard* destinationmask);

		int			lpIndex;
		Bitboard	lastPositions[100];
		int			fiftyMoveCounter;

	};

}
