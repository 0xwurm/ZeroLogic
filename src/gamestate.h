#pragma once
#include "variables.h"
#include <sstream>

namespace ZeroLogic {

	using namespace State;

	class Gamestate {

	public:

        template <bool white>
        void makemove(Move m);
		bool stop();
		void fen(std::string FEN);
		void hashPosition();

		Bitboard	position[12];
		Bitboard	enPassant;
		bool		castlingRights[4];
        bool        white_to_move;

		Bitboard	zobristHash;

	private:

        template <bool white>
		Piece get_piece(Bitboard s);
		void putPiece(Piece p, Bitboard s, int sI);
		void flipCastles(castleType c);
        template <bool white>
		void moveRights(Piece p, Bitboard s, Move m, Bitboard destination_mask);

		int			lpIndex;
		Bitboard	lastPositions[100];
		int			fiftyMoveCounter;
		bool		doPassantCheck;

	};

}
