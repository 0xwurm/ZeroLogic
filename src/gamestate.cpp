#include "gamestate.h"
#include "tables.h"
#include "movegen.h"
#include <cmath>
#include <map>
#include <iostream>

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
		this->zobristHash ^= Tables::ZobristKeys[769 + c];
	}
}

template <>
Piece Gamestate::get_piece<true>(Bitboard s) {
    for (int i = 0; i < 6; i++) if (this->position[i] & s) return static_cast<Piece>(i);
    return static_cast<Piece>(69);
}
template <>
Piece Gamestate::get_piece<false>(Bitboard s) {
    for (int i = 6; i < 12; i++) if (this->position[i] & s) return static_cast<Piece>(i);
    return static_cast<Piece>(69);
}

inline void Gamestate::putPiece(Piece p, Bitboard s, int sI) {
	this->position[p] ^= s;
	this->zobristHash ^= Tables::ZobristKeys[p * 64 + sI];
}

template <>
void Gamestate::moveRights<true>(Piece p, Bitboard s, Move m, Bitboard destination_mask) {
    if (p == wP) {
        this->fiftyMoveCounter = 0;
        this->lpIndex = 0;
        
        if (abs(((m & 0x3f0) >> 4) - (m >> 10)) == 16) {
            doPassantCheck = false;
            Bitboard files[8] = { 0x101010101010101, 0x202020202020202, 0x404040404040404, 0x808080808080808, 0x1010101010101010, 0x2020202020202020, 0x4040404040404040, 0x8080808080808080 };
            for (int i = 0; i < 8; ++i) {
                if (this->enPassant & files[i]) {
                    this->zobristHash ^= Tables::ZobristKeys[773 + i];
                    break;
                }
            }
            this->enPassant = destination_mask >> 8;
            for (int i = 0; i < 8; ++i) {
                if (this->enPassant & files[i]) {
                    this->zobristHash ^= Tables::ZobristKeys[773 + i];
                    break;
                }
            }
        }
    }
    else if (p == wK) {
        if (castlingRights[wk]) this->flipCastles(wk);
        if (castlingRights[wq]) this->flipCastles(wq);
    }
    else if (p == wR) {
        if ((s & wkr) && castlingRights[wk]) { this->flipCastles(wk); }
        else if ((s & wqr) && castlingRights[wq]) { this->flipCastles(wq); }
    }
}
template <>
void Gamestate::moveRights<false>(Piece p, Bitboard s, Move m, Bitboard destination_mask) {
    if (p == bP) {
        this->fiftyMoveCounter = 0;
        this->lpIndex = 0;
        
        if (abs(((m & 0x3f0) >> 4) - (m >> 10)) == 16) {
            doPassantCheck = false;
            Bitboard files[8] = { 0x101010101010101, 0x202020202020202, 0x404040404040404, 0x808080808080808, 0x1010101010101010, 0x2020202020202020, 0x4040404040404040, 0x8080808080808080 };
            for (int i = 0; i < 8; ++i) {
                if (this->enPassant & files[i]) {
                    this->zobristHash ^= Tables::ZobristKeys[773 + i];
                    break;
                }
            }
            this->enPassant = destination_mask << 8;
            for (int i = 0; i < 8; ++i) {
                if (this->enPassant & files[i]) {
                    this->zobristHash ^= Tables::ZobristKeys[773 + i];
                    break;
                }
            }
        }
    }
    else if (p == bK) {
        if (castlingRights[bk]) this->flipCastles(bk);
        if (castlingRights[bq]) this->flipCastles(bq);
    }
    else if (p == bR){
        if ((s & bkr) && castlingRights[bk]) { this->flipCastles(bk); }
        else if ((s & bqr) && castlingRights[bq]) { this->flipCastles(bq); }
    }
}

template <bool white>
void Gamestate::makemove(Move m) {
    Move flagged			= m & flagmask;
    Move promotion_capture	= m & promotioncapture;

    this->fiftyMoveCounter++;
    doPassantCheck = true;

    if (flagged != castles) {

        Move from = m >> 10;
        Bitboard origin_mask = 1ull << from;

        Move to = m << 6;
        to >>= 10;
        Bitboard destination_mask = 1ull << to;


        Piece moved_piece = this->get_piece<white>(origin_mask);
        this->putPiece(moved_piece, origin_mask, from);
        this->moveRights<white>(moved_piece, origin_mask, m, destination_mask);

        if (flagged == promotionq) this->putPiece((white) ? wQ : bQ, destination_mask, to);
        else if (flagged == promotionb) this->putPiece((white) ? wB : bB, destination_mask, to);
        else if (flagged == promotionn) this->putPiece((white) ? wN : bN, destination_mask, to);
        else if (flagged == promotionr) this->putPiece((white) ? wR : bR, destination_mask, to);
        else {
            this->putPiece(moved_piece, destination_mask, to);

            if (flagged == passant) {
                Bitboard passantMask = (white) ? (destination_mask >> 8) : (destination_mask << 8);
                this->putPiece((white) ? bP : wP, passantMask, (white) ? (to - 8) : (to + 8));
            }
        }

        if (flagged == capture || promotion_capture) {

            this->fiftyMoveCounter = 0;
            this->lpIndex = 0;

            Piece captured_piece = get_piece<!white>(destination_mask);
            this->putPiece(captured_piece, destination_mask, to);
            if (captured_piece == bR) {
                if ((destination_mask & bkr) && castlingRights[bk]) {
                    this->flipCastles(bk);
                }
                else if ((destination_mask & bqr) && castlingRights[bq]) {
                    this->flipCastles(bq);
                }
            }
            else if (captured_piece == wR) {
                if ((destination_mask & wkr) && castlingRights[wk]) {
                    this->flipCastles(wk);
                }
                else if ((destination_mask & wqr) && castlingRights[wq]) {
                    this->flipCastles(wq);
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
            this->zobristHash ^= Tables::ZobristKeys[bR * 64 + 56];
            this->zobristHash ^= Tables::ZobristKeys[bR * 64 + 58];
            this->zobristHash ^= Tables::ZobristKeys[bK * 64 + 57];
            this->zobristHash ^= Tables::ZobristKeys[bK * 64 + 59];
            this->position[bK] ^= bkfullk;
            this->position[bR] ^= bkfullr;
        }
        else if (castleFlag == bqflag) {
            this->flipCastles(bk);
            this->flipCastles(bq);
            this->zobristHash ^= Tables::ZobristKeys[bR * 64 + 63];
            this->zobristHash ^= Tables::ZobristKeys[bR * 64 + 60];
            this->zobristHash ^= Tables::ZobristKeys[bK * 64 + 61];
            this->zobristHash ^= Tables::ZobristKeys[bK * 64 + 59];
            this->position[bK] ^= bqfullk;
            this->position[bR] ^= bqfullr;
        }
        else if (castleFlag == wkflag) {
            this->flipCastles(wk);
            this->flipCastles(wq);
            this->zobristHash ^= Tables::ZobristKeys[wR * 64];
            this->zobristHash ^= Tables::ZobristKeys[wR * 64 + 2];
            this->zobristHash ^= Tables::ZobristKeys[wK * 64 + 1];
            this->zobristHash ^= Tables::ZobristKeys[wK * 64 + 3];
            this->position[wK] ^= wkfullk;
            this->position[wR] ^= wkfullr;
        }
        else {
            this->flipCastles(wk);
            this->flipCastles(wq);
            this->zobristHash ^= Tables::ZobristKeys[wR * 64 + 7];
            this->zobristHash ^= Tables::ZobristKeys[wR * 64 + 4];
            this->zobristHash ^= Tables::ZobristKeys[wK * 64 + 5];
            this->zobristHash ^= Tables::ZobristKeys[wK * 64 + 3];
            this->position[wK] ^= wqfullk;
            this->position[wR] ^= wqfullr;
        }
    }

    if (this->enPassant && doPassantCheck) {
        Bitboard files[8] = { 0x101010101010101, 0x202020202020202, 0x404040404040404, 0x808080808080808, 0x1010101010101010, 0x2020202020202020, 0x4040404040404040, 0x8080808080808080 };
        for (int i = 0; i < 8; ++i) {
            if (this->enPassant & files[i]) {
                this->zobristHash ^= Tables::ZobristKeys[773 + i];
                break;
            }
        }
        this->enPassant = 0;
    }

    this->zobristHash ^= Tables::ZobristKeys[768];
    MoveList ml;
    MoveGenerator<white> mg(this, &ml);
    if (mg.attacked(std::countr_zero(position[K + ((white) ? 0 : 6)]))){
        std::cout << "very bad" << std::endl << std::flush;
    }
    white_to_move = !white_to_move;

}
template void Gamestate::makemove<true>(Move m);
template void Gamestate::makemove<false>(Move m);

bool Gamestate::stop() {

	if (this->fiftyMoveCounter == 100) { return true; }

	this->lastPositions[this->lpIndex] = this->zobristHash;

	int oddConst = (this->lpIndex % 2) ? 1 : 0;
	bool foundMatch = false;
	for (int i = 0; i < this->lpIndex / 2; ++i) {
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
	Piece p;

	for (unsigned long long & i : this->position) { i = 0; }
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
    this->white_to_move = (FEN.front() == 'w');
	FEN.erase(FEN.begin());
	FEN.erase(FEN.begin());
	current = FEN.front();

	for (bool & castlingRight : this->castlingRights) { castlingRight = false; }
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
	Bitboard mask;

	for (int i = 0; i < 64; ++i) {

		mask = static_cast<Bitboard>(1) << i;
		if (occ & mask) {

			for (int j = 0; j < 12; ++j) {
				if (this->position[j] & mask) {
					h ^= Tables::ZobristKeys[j * 64 + i];
					break;
				}
			}

		}


	}

	if (!white_to_move) { h ^= Tables::ZobristKeys[768]; }

	for (int i = 0; i < 4; ++i) {
		if (this->castlingRights[i]) {
			h ^= Tables::ZobristKeys[769 + i];
		}
	}

	Bitboard files[8] = { 0x101010101010101, 0x202020202020202, 0x404040404040404, 0x808080808080808, 0x1010101010101010, 0x2020202020202020, 0x4040404040404040, 0x8080808080808080 };
	if (this->enPassant) {
		for (int i = 0; i < 8; ++i) {
			if (this->enPassant & files[i]) {
				h ^= Tables::ZobristKeys[773 + i];
				break;
			}
		}
	}

	this->zobristHash = h;

	this->lastPositions[0] = h;
	this->lpIndex = 1;

}