#include "gamestate.h"
#include <iostream>
#include "misc.h"

using namespace ZeroLogic;
using namespace State;

std::map<char, Piece> fenTable{ {'p', bP}, {'r', bR}, {'b', bB}, {'n', bN}, {'k', bK}, {'q', bQ}, {'P', wP}, {'R', wR}, {'B', wB}, {'N', wN}, {'K', wK}, {'Q', wQ}, {'1', Piece::nope}, {'2', Piece::nope}, {'3', Piece::nope}, {'4', Piece::nope}, {'5', Piece::nope}, {'6', Piece::nope}, {'7', Piece::nope}, {'8', Piece::nope} };
std::map<char, int> shiftTable{ {'1', 1}, {'2', 2}, {'3', 3}, {'4', 4}, {'5', 5}, {'6', 6}, {'7', 7}, {'8', 8} };
std::map<char, castleType> castleTable{ {'K', wk}, { 'Q', wq }, { 'k', bk }, { 'q', bq } }; 
std::map<char, int> strnumConversionTable1{ {'a', 7}, {'b', 6}, {'c', 5}, {'d', 4}, {'e', 3}, {'f', 2}, {'g', 1}, {'h', 0} };
std::map<char, int> strnumConversionTable2{ {'1', 0}, {'2', 8}, {'3', 16}, {'4', 24}, {'5', 32}, {'6', 40}, {'7', 48}, {'8', 56} };

void Gamestate::flipCastles(castleType c) {

	this->lpIndex = 0;
	if (this->castlingRights[c]) { 
		this->castlingRights[c] = false;
		this->zobristHash ^= ZobristKeys[769 + c]; 
	}

}

Piece Gamestate::getPiece(Bitboard* s, int startingPoint, Move m) {

	for (int i = 0; i < 6; ++i) {
		if (this->position[startingPoint + i] & *s) {
			return static_cast<Piece>(startingPoint + i);
		}
	}
	std::cout << startingPoint << std::endl;
	for (int i = 0; i < 12; i++) std::cout << i << ":\t" << std::bitset<64>(position[i]) << std::endl;
	std::cout << "s:\t" << std::bitset<64>(*s) << std::endl;
	std::cout << "m:\t" << std::bitset<16>(m) << std::endl;
	Misc* msc = new Misc;
	std::cout << "mstr:\t" << msc->numToString(m) << std::endl;
	delete msc;

}

void Gamestate::putPiece(Piece p, Bitboard* s, int sI) {

	this->position[p] ^= *s;
	this->zobristHash ^= ZobristKeys[p * 64 + sI];

}

std::vector<std::vector<double>> Gamestate::vectorify() {

	std::vector<std::vector<double>> board;
	board.reserve(8);
	for (int i = 0; i < 8; i++) {
		std::vector<double> row;
		row.reserve(8);
		for (int j = 0; j < 8; j++) {
			row.push_back(0);
		}
		board.push_back(row);
	}

	if (white) {
		for (int i = 0; i < 12; i++) {
			Bitboard container = position[i];
			int j = 0;
			while (container) {
				int tj = _tzcnt_u64(container);
				container >>= tj + 1;
				j += tj;
				board.at(floor(j / static_cast<double>(8))).at(j % 8) = (static_cast<double>(i) + 1) / 12;
				j++;
			}
		}
	}
	else {
		for (int i = 0; i < 12; i++) {
			Bitboard container = position[i];
			int j = 0;
			while (container) {
				int tj = __lzcnt64(container);
				container <<= tj + 1;
				j += tj;
				board.at(floor(j / static_cast<double>(8))).at(j % 8) = (static_cast<double>(i) + 1) / 12;
				j++;
			}
		}
	}

	return board;

}

void Gamestate::moveRights(Piece p, Bitboard* s, Move m, Bitboard* destinationmask) {

	Move toBeTested{};

	if (this->white) {
		switch (p) {
		case wP:
			this->fiftyMoveCounter	= 0;
			this->lpIndex			= 0;

			toBeTested = m ^ (m << 6);
			toBeTested >>= 10;

			if (toBeTested == 16) {
				this->enPassant = *destinationmask >> 8;
				Bitboard files[8] = { 0x101010101010101, 0x202020202020202, 0x404040404040404, 0x808080808080808, 0x1010101010101010, 0x2020202020202020, 0x4040404040404040, 0x8080808080808080 };
				for (int i = 0; i < 8; ++i) {
					if (this->enPassant & files[i]) {
						this->zobristHash ^= ZobristKeys[773 + i];
						break;
					}
				}
			}
		case wK:
			this->flipCastles(wk);
			this->flipCastles(wq);
		case wR:
			if (*s & wkr) { this->flipCastles(wk); }
			else if (*s & wqr) { this->flipCastles(wq); }
		}
	}
	else {
		switch (p) {
		case bP:
			this->fiftyMoveCounter = 0;
			this->lpIndex = 0;

			toBeTested = m ^ (m << 6);
			toBeTested >>= 10;

			if (toBeTested == 16) {
				this->enPassant = *destinationmask << 8;
				Bitboard files[8] = { 0x101010101010101, 0x202020202020202, 0x404040404040404, 0x808080808080808, 0x1010101010101010, 0x2020202020202020, 0x4040404040404040, 0x8080808080808080 };
				for (int i = 0; i < 8; ++i) {
					if (this->enPassant & files[i]) {
						this->zobristHash ^= ZobristKeys[773 + i];
						break;
					}
				}
			}
		case bK:
			this->flipCastles(bk);
			this->flipCastles(bq);
		case bR:
			if (*s & bkr) { this->flipCastles(bk); }
			else if (*s & bqr) { this->flipCastles(bq); }
		}
	}
	

}

void Gamestate::makemove(Move m) {

	Move flagged			= m & flagmask;
	Move promocaptureflag	= m & promotioncapture;

	this->fiftyMoveCounter++;

	if (flagged != castles) {

		Move from = m >> 10;
		Bitboard originmask = static_cast<Bitboard>(1) << from;

		Move to = m << 6;
		to >>= 10;
		Bitboard destinationmask = static_cast<Bitboard>(1) << to;

		int blackConst = (this->white) ? 0 : 6;
		int eblackConst = (this->white) ? 6 : 0;



		Piece movedpiece = this->getPiece(&originmask, blackConst, m);
		this->putPiece(movedpiece, &originmask, from);
		this->moveRights(movedpiece, &originmask, m, &destinationmask);

		switch (flagged) {
		case promotionq: this->putPiece(static_cast<Piece>(4 + blackConst), &destinationmask, to);
		case promotionb: this->putPiece(static_cast<Piece>(3 + blackConst), &destinationmask, to);
		case promotionn: this->putPiece(static_cast<Piece>(2 + blackConst), &destinationmask, to);
		case promotionr: this->putPiece(static_cast<Piece>(1 + blackConst), &destinationmask, to);
		default:
			this->putPiece(movedpiece, &destinationmask, to);

			if (flagged == passant) {
				Bitboard passantMask = (this->white) ? (destinationmask >> 8) : (destinationmask << 8);
				this->putPiece(static_cast<Piece>(eblackConst), &passantMask, (this->white) ? (to - 8) : (to + 8));
			}
		}

		if (flagged == capture || promocaptureflag) {

			this->fiftyMoveCounter = 0;
			this->lpIndex = 0;

			Piece capturedpiece = getPiece(&destinationmask, eblackConst, m);
			this->putPiece(capturedpiece, &destinationmask, to);
			if (capturedpiece == R) {
				if (this->white) {
					if (destinationmask & bkr) {
						this->flipCastles(bk);
					}
					else if (destinationmask & bqr) {
						this->flipCastles(bq);
					}
				}
				else {
					if (destinationmask & wkr) {
						this->flipCastles(wk);
					}
					else if (destinationmask & wqr) {
						this->flipCastles(wq);
					}
				}
			}
		}

	}
	else {

		this->fiftyMoveCounter = 0;
		this->lpIndex = 0;

		Move castleFlag = m & wkflag;

		if (castleFlag == bkflag) {
			this->flipCastles(bk);
			this->flipCastles(bq);
			this->zobristHash ^= ZobristKeys[bR * 64 + 56];
			this->zobristHash ^= ZobristKeys[bR * 64 + 58];
			this->zobristHash ^= ZobristKeys[bK * 64 + 57];
			this->zobristHash ^= ZobristKeys[bK * 64 + 59];
			this->position[bK] ^= bkfullk;
			this->position[bR] ^= bkfullr;
		}
		else if (castleFlag == bqflag) {
			this->flipCastles(bk);
			this->flipCastles(bq);
			this->zobristHash ^= ZobristKeys[bR * 64 + 63];
			this->zobristHash ^= ZobristKeys[bR * 64 + 60];
			this->zobristHash ^= ZobristKeys[bK * 64 + 61];
			this->zobristHash ^= ZobristKeys[bK * 64 + 59];
			this->position[bK] ^= bqfullk;
			this->position[bR] ^= bqfullr;
		}
		else if (castleFlag == wkflag) {
			this->flipCastles(wk);
			this->flipCastles(wq);
			this->zobristHash ^= ZobristKeys[wR * 64];
			this->zobristHash ^= ZobristKeys[wR * 64 + 2];
			this->zobristHash ^= ZobristKeys[wK * 64 + 1];
			this->zobristHash ^= ZobristKeys[wK * 64 + 3];
			this->position[wK] ^= wkfullk;
			this->position[wR] ^= wkfullr;
		}
		else {
			this->flipCastles(wk);
			this->flipCastles(wq);
			this->zobristHash ^= ZobristKeys[wR * 64 + 7];
			this->zobristHash ^= ZobristKeys[wR * 64 + 4];
			this->zobristHash ^= ZobristKeys[wK * 64 + 5];
			this->zobristHash ^= ZobristKeys[wK * 64 + 3];
			this->position[wK] ^= wqfullk;
			this->position[wR] ^= wqfullr;
		}
	}

	if (this->enPassant) {
		Bitboard files[8] = { 0x101010101010101, 0x202020202020202, 0x404040404040404, 0x808080808080808, 0x1010101010101010, 0x2020202020202020, 0x4040404040404040, 0x8080808080808080 };
		for (int i = 0; i < 8; ++i) {
			if (this->enPassant & files[i]) {
				this->zobristHash ^= ZobristKeys[773 + i];
				break;
			}
		}
		this->enPassant = 0;
	}

	this->white = (this->white) ? false : true;
	this->zobristHash ^= ZobristKeys[768];

}

bool Gamestate::stop() {

	if (this->fiftyMoveCounter == 100) { return true; }

	this->lastPositions[this->lpIndex] = this->zobristHash;

	int oddConst = (this->lpIndex % 2) ? 1 : 0;
	bool foundMatch = false;
	for (int i = 0; i < floor(this->lpIndex / 2); ++i) {
		if (this->lastPositions[i * 2 + oddConst] == this->lastPositions[this->lpIndex]) {
			if (foundMatch) {
				this->lpIndex++;
				return true;
			}
			foundMatch = true;
		}
	}

	return false;

}

void Gamestate::fen(std::string FEN) {

	char current = FEN.front();
	Bitboard currentIndex = static_cast<Bitboard>(1) << 63;
	Piece p = nope;

	for (int i = 0; i < 12; ++i) { this->position[i] = 0; }
	this->enPassant = 0;

	while (current != ' ') {
		if (current != '/') {
			p = fenTable[current];

			if (p != nope) {
				this->position[p] ^= currentIndex;
				currentIndex >>= 1;
			}
			else {
				currentIndex >>= shiftTable[current];
			}
		}

		FEN.erase(FEN.begin());
		current = FEN.front();
	}

	FEN.erase(FEN.begin());
	this->white = (FEN.front() == 'w') ? true : false;
	FEN.erase(FEN.begin());
	FEN.erase(FEN.begin());
	current = FEN.front();

	for (int i = 0; i < 4; i++) { this->castlingRights[i] = false; }
	if (current != '-') {
		while (current != ' ') {
			this->castlingRights[castleTable[current]] = true;
			FEN.erase(FEN.begin());
			current = FEN.front();
		}
	}
	else { FEN.erase(FEN.begin()); }

	FEN.erase(FEN.begin());
	if (FEN.front() != '-') {
		int shift = strnumConversionTable1[FEN.front()];
		FEN.erase(FEN.begin());
		shift += strnumConversionTable2[FEN.front()];
		this->enPassant = static_cast<Bitboard>(1) << shift;
	}
	else {
		this->enPassant = 0;
	}
	FEN.erase(FEN.begin());
	FEN.erase(FEN.begin());

	if (FEN.size() == 2) {
		this->fiftyMoveCounter = (FEN.front() - 48) * 10;
		FEN.erase(FEN.begin());
	}
	this->fiftyMoveCounter += FEN.front() - 48;

	this->hashPosition();

}

void Gamestate::hashPosition() {

	Bitboard h{};

	// hash key -> colorPiece * 64 + square
	// 768: color to move (1 == black)
	// 769 - 772: wk, wq, bk, bq
	// 773 - 781: h - a en passant files

	Bitboard wOcc = this->position[0] | this->position[1] | this->position[2] | this->position[3] | this->position[4] | this->position[5];
	Bitboard bOcc = this->position[6] | this->position[7] | this->position[8] | this->position[9] | this->position[10] | this->position[11];
	Bitboard occ = wOcc | bOcc;
	Bitboard mask{};

	for (int i = 0; i < 64; ++i) {

		mask = static_cast<Bitboard>(1) << i;
		if (occ & mask) {

			for (int j = 0; j < 12; ++j) {
				if (this->position[j] & mask) {
					h ^= ZobristKeys[j * 64 + i];
					break;
				}
			}

		}


	}

	if (!this->white) { h ^= ZobristKeys[768]; }

	for (int i = 0; i < 4; ++i) {
		if (this->castlingRights[i]) {
			h ^= ZobristKeys[769 + i];
		}
	}

	Bitboard files[8] = { 0x101010101010101, 0x202020202020202, 0x404040404040404, 0x808080808080808, 0x1010101010101010, 0x2020202020202020, 0x4040404040404040, 0x8080808080808080 };
	if (this->enPassant) {
		for (int i = 0; i < 8; ++i) {
			if (this->enPassant & files[i]) {
				h ^= ZobristKeys[773 + i];
				break;
			}
		}
	}

	this->zobristHash = h;

	this->lastPositions[0] = h;
	this->lpIndex = 1;

}