#pragma once
#include "variables.h"
#include "search.h"
#include "gamestate.h"
#include <map>

namespace ZeroLogic {

	using namespace Movegen;

	class Movegenerator {

	public:

		Movegenerator(Gamestate* externalGamestate, Move* externalMoveList, uint8_t* movenum);

		void generate();

	private:

		Gamestate* gsRef{};
		Move* movelistRef;
		uint8_t* movenum;

		Bitboard wOcc{};
		Bitboard bOcc{};
		Bitboard occ{};
		Bitboard empty{};
		Bitboard emptyorenemy{};
		Bitboard own{};
		Bitboard enemy{};

		Bitboard pawns{};
		Bitboard rooks{};
		Bitboard bishops{};
		Bitboard knights{};
		Bitboard queens{};
		Bitboard king{};

		Bitboard epawns{};
		Bitboard estraight{};
		Bitboard eknights{};
		Bitboard ediagonal{};
		Bitboard eking{};

		Bitboard checkRing{};
		Bitboard checkMask{};
		Bitboard piecesNotPinned{};
		Bitboard piecesPinnedD{};
		Bitboard piecesPinnedS{};
		Bitboard straightPin{};
		Bitboard diagonalPin{};
		Bitboard notPinned{};
		Bitboard ring{};
		Bitboard localMask{};
		bool loggedCheck{};
		bool loggedPin{};
		Bitboard noCheck{};

		Bitboard snip{};
		unsigned long mask{};

		Bitboard* nextBoardArray[3] = { &piecesNotPinned, &piecesPinnedD, &piecesPinnedS };

		Bitboard nextBoard{};
		Bitboard toBeScanned{};
		Bitboard promoBoard{};

		int blackConst{};
		int eblackConst{};
		int summand{};

		std::map<int, Borders> bordermap1 = { {1, west}, {7, northeast}, {8, north}, {9, northwest} };
		std::map<int, Borders> bordermap2 = { {1, east}, {7, southwest}, {8, south}, {9, southeast} };

		void initRings();
		void initGenerate();
		void initMasks();

		void isolator(moveflags itype, moveflags ctype, Direction direction, moveflags promocapture);

		void slider(Direction direction, PType type);
		void sliderC(Direction direction, PType type);
		void kinggen();
		void pawngen();
		void pawngenC();
		void sliderMask(Direction direction);
		void sliderCheckRing(Direction direction);
		bool attacked(Square sq);
		bool slideratk(Direction direction, int sq);
		bool passantCrossLegal();
		bool passantStraightLegal(Bitboard reversedNextBoard);

		void additBoards();

		void makeMasks();

	};
}