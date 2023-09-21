#include "movegeneration.h"
#include <iostream>

using namespace ZeroLogic;
using namespace Movegen;

Movegenerator::Movegenerator(Gamestate* externalGamestate, Move* externalMoveList, uint8_t* movenum) {
	gsRef = externalGamestate;
	movelistRef = externalMoveList;
	this->movenum = movenum;
}
void Movegenerator::initMasks() {
	eblackConst = (gsRef->white) ? 6 : 0;

	epawns = gsRef->position[P + eblackConst];
	estraight = gsRef->position[R + eblackConst] | gsRef->position[Q + eblackConst];
	eknights = gsRef->position[N + eblackConst];
	ediagonal = gsRef->position[B + eblackConst] | gsRef->position[Q + eblackConst];
	eking = gsRef->position[K + eblackConst];

	straightPin = 0;
	diagonalPin = 0;
	checkMask = 0;
	checkRing = 0;

	initRings();

	ring &= emptyorenemy;

	loggedCheck = false;
	loggedPin = false;
}
void Movegenerator::initRings() {
	ring = 0;
	ring |= ((king & east) >> 1);
	ring |= ((king & west) << 1);
	ring |= ((king & north) << 8);
	ring |= ((king & south) >> 8);
	ring |= ((king & southeast) >> 9);
	ring |= ((king & northwest) << 9);
	ring |= ((king & northeast) << 7);
	ring |= ((king & southwest) >> 7);
}
void Movegenerator::initGenerate() {

	additBoards();

	// used for color specific position access
	blackConst = (gsRef->white) ? 0 : 6;

	pawns = gsRef->position[P + blackConst];
	rooks = gsRef->position[R + blackConst];
	knights = gsRef->position[N + blackConst];
	bishops = gsRef->position[B + blackConst];
	queens = gsRef->position[Q + blackConst];
	king = gsRef->position[K + blackConst];

	makeMasks();
}

void Movegenerator::additBoards() {
	wOcc = gsRef->position[0] | gsRef->position[1] | gsRef->position[2] | gsRef->position[3] | gsRef->position[4] | gsRef->position[5];
	bOcc = gsRef->position[6] | gsRef->position[7] | gsRef->position[8] | gsRef->position[9] | gsRef->position[10] | gsRef->position[11];
	occ = wOcc | bOcc;
	empty = ~occ;
	emptyorenemy = (gsRef->white) ? ~wOcc : ~bOcc;
	if (gsRef->white) {
		own = wOcc;
		enemy = bOcc;
	}
	else {
		own = bOcc;
		enemy = wOcc;
	}
}

void Movegenerator::isolator(moveflags itype, moveflags ctype, Direction direction, moveflags promocapture) {
	if (itype != castles) {
		do {
			_BitScanForward64(&mask, toBeScanned);
			toBeScanned &= toBeScanned - 1;
			*(movelistRef + *movenum) = ((mask - summand * direction) << 10) | (mask << 4) | promocapture | itype;
			(*movenum)++;
		} while (toBeScanned);
	}
	else {
		*(movelistRef + *movenum) = ctype | itype;
		(*movenum)++;
	}
}

void Movegenerator::slider(Direction direction, PType type) {

	nextBoard = *nextBoardArray[type];
	summand = 0;
	do {
		nextBoard &= bordermap1[direction];
		nextBoard <<= direction;
		nextBoard &= emptyorenemy;
		if (type == diagonal) {
			nextBoard &= diagonalPin;
		}
		else if (type == straight) {
			nextBoard &= straightPin;
		}
		if (nextBoard) {
			summand++;
			if (nextBoard & empty) { toBeScanned = nextBoard & empty; isolator(normal, moveflags::none, direction, moveflags::none); }
			if (nextBoard & enemy) { toBeScanned = nextBoard & enemy; isolator(capture, moveflags::none, direction, moveflags::none); }
			nextBoard &= empty;
		}
		else { break; }
	} while (true);

	nextBoard = *nextBoardArray[type];
	summand = 0;
	do {
		nextBoard &= bordermap2[direction];
		nextBoard >>= direction;
		nextBoard &= emptyorenemy;
		if (type == diagonal) {
			nextBoard &= diagonalPin;
		}
		else if (type == straight) {
			nextBoard &= straightPin;
		}
		if (nextBoard) {
			summand--;
			if (nextBoard & empty) { toBeScanned = nextBoard & empty; isolator(normal,moveflags::none, direction, moveflags::none); }
			if (nextBoard & enemy) { toBeScanned = nextBoard & enemy; isolator(capture,moveflags::none, direction, moveflags::none); }
			nextBoard &= empty;
		}
		else { break; }
	} while (true);
}
void Movegenerator::sliderC(Direction direction, PType type) {
	nextBoard = *nextBoardArray[type];
	summand = 0;
	do {
		nextBoard &= Movegenerator::bordermap1[direction];
		nextBoard <<= direction;
		nextBoard &= emptyorenemy;
		if (nextBoard & checkMask) {
			summand++;
			if (nextBoard & empty & checkMask) { toBeScanned = nextBoard & checkMask & empty; isolator(normal, moveflags::none, direction, moveflags::none); }
			if (nextBoard & enemy & checkMask) { toBeScanned = nextBoard & checkMask & enemy; isolator(capture, moveflags::none, direction, moveflags::none); }			
			nextBoard &= empty;
		}
		else if (nextBoard) { summand++; nextBoard &= empty; }
		else { break; }
	} while (true);

	nextBoard = *nextBoardArray[type];
	summand = 0;
	do {
		nextBoard &= Movegenerator::bordermap2[direction];
		nextBoard >>= direction;
		nextBoard &= emptyorenemy;
		if (nextBoard & checkMask) {
			summand--;
			if (nextBoard & empty & checkMask) { toBeScanned = nextBoard & checkMask & empty; isolator(normal, moveflags::none, direction, moveflags::none); }
			if (nextBoard & enemy & checkMask) { toBeScanned = nextBoard & checkMask & enemy; isolator(capture, moveflags::none, direction, moveflags::none); }
			nextBoard &= empty;
		}
		else if (nextBoard) { summand--; nextBoard &= empty; }
		else { break; }
	} while (true);
}
bool Movegenerator::passantCrossLegal() {
	Bitboard thePawn = 0;
	if (gsRef->white) {
		thePawn = nextBoard >> 8;
	}
	else {
		thePawn = nextBoard << 8;
	}

	int index = _tzcnt_u64(thePawn);
	Bitboard scan{};
	if (slideratk(z, index)) {
		scan = thePawn;
		do {
			scan &= bordermap1[z];
			scan <<= z;
			if (scan & occ) {
				if (scan & king) {
					return false;
				}
				else { break; }
			}
		} while (scan);

		scan = thePawn;
		do {
			scan &= bordermap2[z];
			scan >>= z;
			if (scan & occ) {
				if (scan & king) {
					return false;
				}
				else { break; }
			}
		} while (scan);
	}
	if (slideratk(z2, index)){

		scan = thePawn;
		do {
			scan &= bordermap1[z2];
			scan <<= z2;
			if (scan & occ) {
				if (scan & king) {
					return false;
				}
				else { break; }
			}
		} while (scan);

		scan = thePawn;
		do {
			scan &= bordermap2[z2];
			scan >>= z2;
			if (scan & occ) {
				if (scan & king) {
					return false;
				}
				else { break; }
			}
		} while (scan);
	}
	return true;
}
bool Movegenerator::passantStraightLegal(Bitboard reversedNextBoard) {
	Bitboard thePawn = 0;
	if (gsRef->white) {
		thePawn = nextBoard >> 8;
	}
	else {
		thePawn = nextBoard << 8;
	}

	int index = _tzcnt_u64(thePawn);
	if (slideratk(x, index) || slideratk(x, _tzcnt_u64(reversedNextBoard))) {
		Bitboard scan{};
		scan = thePawn;
		do {
			scan &= bordermap1[x];
			scan <<= x;
			if (scan & occ) {
				if (scan & king) {
					return false;
				}
				else { break; }
			}
		} while (scan);

		scan = thePawn;
		do {
			scan &= bordermap2[x];
			scan >>= x;
			if (scan & occ) {
				if (scan & king) {
					return false;
				}
				else { break; }
			}
		} while (scan);

		scan = reversedNextBoard;
		do {
			scan &= bordermap1[x];
			scan <<= x;
			if (scan & occ) {
				if (scan & king) {
					return false;
				}
				else { break; }
			}
		} while (scan);

		scan = reversedNextBoard;
		do {
			scan &= bordermap2[x];
			scan >>= x;
			if (scan & occ) {
				if (scan & king) {
					return false;
				}
				else { break; }
			}
		} while (scan);
		return true;
	}
	else { return true; }
}
void Movegenerator::kinggen() {
	nextBoard = ((king & west) << 1) & emptyorenemy & noCheck;
	if (nextBoard & enemy) { toBeScanned = nextBoard & enemy; summand = 1; isolator(capture, moveflags::none, x, moveflags::none); }
	if (nextBoard & empty) { toBeScanned = nextBoard & empty; summand = 1; isolator(normal, moveflags::none, x, moveflags::none); }
	nextBoard = ((king & east) >> 1) & emptyorenemy & noCheck;
	if (nextBoard & enemy) { toBeScanned = nextBoard & enemy; summand = -1; isolator(capture, moveflags::none, x, moveflags::none); }
	if (nextBoard & empty) { toBeScanned = nextBoard & empty; summand = -1; isolator(normal, moveflags::none, x, moveflags::none); }
	nextBoard = ((king & north) << 8) & emptyorenemy & noCheck;
	if (nextBoard & enemy) { toBeScanned = nextBoard & enemy; summand = 8; isolator(capture, moveflags::none, x, moveflags::none); }
	if (nextBoard & empty) { toBeScanned = nextBoard & empty; summand = 8; isolator(normal, moveflags::none, x, moveflags::none); }
	nextBoard = ((king & south) >> 8) & emptyorenemy & noCheck;
	if (nextBoard & enemy) { toBeScanned = nextBoard & enemy; summand = -8; isolator(capture, moveflags::none, x, moveflags::none); }
	if (nextBoard & empty) { toBeScanned = nextBoard & empty; summand = -8; isolator(normal, moveflags::none, x, moveflags::none); }
	nextBoard = ((king & northeast) << 7) & emptyorenemy & noCheck;
	if (nextBoard & enemy) { toBeScanned = nextBoard & enemy; summand = 7; isolator(capture, moveflags::none, x, moveflags::none); }
	if (nextBoard & empty) { toBeScanned = nextBoard & empty; summand = 7; isolator(normal, moveflags::none, x, moveflags::none); }
	nextBoard = ((king & southwest) >> 7) & emptyorenemy & noCheck;
	if (nextBoard & enemy) { toBeScanned = nextBoard & enemy; summand = -7; isolator(capture, moveflags::none, x, moveflags::none); }
	if (nextBoard & empty) { toBeScanned = nextBoard & empty; summand = -7; isolator(normal, moveflags::none, x, moveflags::none); }
	nextBoard = ((king & northwest) << 9) & emptyorenemy & noCheck;
	if (nextBoard & enemy) { toBeScanned = nextBoard & enemy; summand = 9; isolator(capture, moveflags::none, x, moveflags::none); }
	if (nextBoard & empty) { toBeScanned = nextBoard & empty; summand = 9; isolator(normal, moveflags::none, x, moveflags::none); }
	nextBoard = ((king & southeast) >> 9) & emptyorenemy & noCheck;
	if (nextBoard & enemy) { toBeScanned = nextBoard & enemy; summand = -9; isolator(capture, moveflags::none, x, moveflags::none); }
	if (nextBoard & empty) { toBeScanned = nextBoard & empty; summand = -9; isolator(normal, moveflags::none, x, moveflags::none); }
}
void Movegenerator::pawngen() {

	if (gsRef->white) {

		if (piecesNotPinned) {
			nextBoard = ((piecesNotPinned << 8) & empty);

			if (nextBoard) {

				summand = 8;

				promoBoard = nextBoard & eighth;
				if (promoBoard) {
					toBeScanned = promoBoard;
					isolator(promotionb, moveflags::none, x, moveflags::none);
					toBeScanned = promoBoard;
					isolator(promotionn, moveflags::none, x, moveflags::none);
					toBeScanned = promoBoard;
					isolator(promotionq, moveflags::none, x, moveflags::none);
					toBeScanned = promoBoard;
					isolator(promotionr, moveflags::none, x, moveflags::none);
				}

				nextBoard &= (~eighth);
				if (nextBoard) {
					toBeScanned = nextBoard;
					isolator(normal, moveflags::none, x, moveflags::none);
				}

				nextBoard &= third;
				nextBoard <<= 8;
				nextBoard &= empty;
				if (nextBoard) { summand = 16; toBeScanned = nextBoard; isolator(normal, moveflags::none, x, moveflags::none); }

			}


			nextBoard = ((piecesNotPinned & northeast) << 7) & enemy;

			if (nextBoard) {

				summand = 7;

				promoBoard = nextBoard & eighth;
				if (promoBoard) {
					toBeScanned = promoBoard;
					isolator(promotionb, moveflags::none, x, promotioncapture);
					toBeScanned = promoBoard;
					isolator(promotionn, moveflags::none, x, promotioncapture);
					toBeScanned = promoBoard;
					isolator(promotionq, moveflags::none, x, promotioncapture);
					toBeScanned = promoBoard;
					isolator(promotionr, moveflags::none, x, promotioncapture);
				}

				nextBoard &= (~eighth);
				if (nextBoard) {
					toBeScanned = nextBoard;
					isolator(capture, moveflags::none, x, moveflags::none);
				}
			}

			nextBoard = ((piecesNotPinned & northwest) << 9) & enemy;

			if (nextBoard) {

				summand = 9;

				promoBoard = nextBoard & eighth;
				if (promoBoard) {
					toBeScanned = promoBoard;
					isolator(promotionb, moveflags::none, x, promotioncapture);
					toBeScanned = promoBoard;
					isolator(promotionn, moveflags::none, x, promotioncapture);
					toBeScanned = promoBoard;
					isolator(promotionq, moveflags::none, x, promotioncapture);
					toBeScanned = promoBoard;
					isolator(promotionr, moveflags::none, x, promotioncapture);
				}

				nextBoard &= (~eighth);
				if (nextBoard) {
					toBeScanned = nextBoard;
					isolator(capture, moveflags::none, x, moveflags::none);
				}
			}

			if (gsRef->enPassant) {
				nextBoard = ((piecesNotPinned & northeast) << 7) & gsRef->enPassant;
				if (nextBoard) {
					if (passantCrossLegal() && passantStraightLegal(nextBoard >> 7)){ // horrible horrible horrible
						summand = 7; toBeScanned = nextBoard; isolator(passant, moveflags::none, x, moveflags::none);
					}
				}
				nextBoard = ((piecesNotPinned & northwest) << 9) & gsRef->enPassant;
				if (nextBoard) { 
					if (passantCrossLegal() && passantStraightLegal(nextBoard >> 9)) {
						summand = 9; toBeScanned = nextBoard; isolator(passant, moveflags::none, x, moveflags::none);
					}
				}
			}
		}

		if (piecesPinnedS) {
			nextBoard = ((piecesPinnedS << 8) & empty) & straightPin;
			if (nextBoard) {

				summand = 8;

				promoBoard = nextBoard & eighth;
				if (promoBoard) {
					toBeScanned = promoBoard;
					isolator(promotionb, moveflags::none, x, moveflags::none);
					toBeScanned = promoBoard;
					isolator(promotionn, moveflags::none, x, moveflags::none);
					toBeScanned = promoBoard;
					isolator(promotionq, moveflags::none, x, moveflags::none);
					toBeScanned = promoBoard;
					isolator(promotionr, moveflags::none, x, moveflags::none);
				}

				nextBoard &= (~eighth);
				if (nextBoard) {
					toBeScanned = nextBoard;
					isolator(normal, moveflags::none, x, moveflags::none);
				}

				nextBoard &= third;
				nextBoard <<= 8;
				nextBoard &= empty;
				if (nextBoard) { summand = 16; toBeScanned = nextBoard; isolator(normal, moveflags::none, x, moveflags::none); }

			}
		}

		if (piecesPinnedD) {
			nextBoard = ((piecesPinnedD & northeast) << 7) & enemy & diagonalPin;

			if (nextBoard) {

				summand = 7;

				promoBoard = nextBoard & eighth;
				if (promoBoard) {
					toBeScanned = promoBoard;
					isolator(promotionb, moveflags::none, x, promotioncapture);
					toBeScanned = promoBoard;
					isolator(promotionn, moveflags::none, x, promotioncapture);
					toBeScanned = promoBoard;
					isolator(promotionq, moveflags::none, x, promotioncapture);
					toBeScanned = promoBoard;
					isolator(promotionr, moveflags::none, x, promotioncapture);
				}

				nextBoard &= (~eighth);
				if (nextBoard) {
					toBeScanned = nextBoard;
					isolator(capture, moveflags::none, x, moveflags::none);
				}
			}

			nextBoard = ((piecesPinnedD & northwest) << 9) & enemy & diagonalPin;

			if (nextBoard) {

				summand = 9;

				promoBoard = nextBoard & eighth;
				if (promoBoard) {
					toBeScanned = promoBoard;
					isolator(promotionb, moveflags::none, x, promotioncapture);
					toBeScanned = promoBoard;
					isolator(promotionn, moveflags::none, x, promotioncapture);
					toBeScanned = promoBoard;
					isolator(promotionq, moveflags::none, x, promotioncapture);
					toBeScanned = promoBoard;
					isolator(promotionr, moveflags::none, x, promotioncapture);
				}

				nextBoard &= (~eighth);
				if (nextBoard) {
					toBeScanned = nextBoard;
					isolator(capture, moveflags::none, x, moveflags::none);
				}
			}

			if (gsRef->enPassant) {
				nextBoard = ((piecesPinnedD & northeast) << 7) & gsRef->enPassant & diagonalPin;
				if (nextBoard) { summand = 7; toBeScanned = nextBoard; isolator(passant, moveflags::none, x, moveflags::none); }
				nextBoard = ((piecesPinnedD & northwest) << 9) & gsRef->enPassant & diagonalPin;
				if (nextBoard) { summand = 9; toBeScanned = nextBoard; isolator(passant, moveflags::none, x, moveflags::none); }
			}
		}
	}
	else {

		if (piecesNotPinned) {
			nextBoard = ((piecesNotPinned >> 8) & empty);

			if (nextBoard) {

				summand = -8;

				promoBoard = nextBoard & first;
				if (promoBoard) {
					toBeScanned = promoBoard;
					isolator(promotionb, moveflags::none, x, moveflags::none);
					toBeScanned = promoBoard;
					isolator(promotionn, moveflags::none, x, moveflags::none);
					toBeScanned = promoBoard;
					isolator(promotionq, moveflags::none, x, moveflags::none);
					toBeScanned = promoBoard;
					isolator(promotionr, moveflags::none, x, moveflags::none);
				}

				nextBoard &= (~first);
				if (nextBoard) {
					toBeScanned = nextBoard;
					isolator(normal, moveflags::none, x, moveflags::none);
				}

				nextBoard &= sixth;
				nextBoard >>= 8;
				nextBoard &= empty;
				if (nextBoard) { summand = -16; toBeScanned = nextBoard; isolator(normal, moveflags::none, x, moveflags::none); }

			}


			nextBoard = ((piecesNotPinned & southwest) >> 7) & enemy;

			if (nextBoard) {

				summand = -7;

				promoBoard = nextBoard & first;
				if (promoBoard) {
					toBeScanned = promoBoard;
					isolator(promotionb, moveflags::none, x, promotioncapture);
					toBeScanned = promoBoard;
					isolator(promotionn, moveflags::none, x, promotioncapture);
					toBeScanned = promoBoard;
					isolator(promotionq, moveflags::none, x, promotioncapture);
					toBeScanned = promoBoard;
					isolator(promotionr, moveflags::none, x, promotioncapture);
				}

				nextBoard &= (~first);
				if (nextBoard) {
					toBeScanned = nextBoard;
					isolator(capture, moveflags::none, x, moveflags::none);
				}
			}
			
			nextBoard = ((piecesNotPinned & southeast) >> 9) & enemy;

			if (nextBoard) {

				summand = -9;

				promoBoard = nextBoard & first;
				if (promoBoard) {
					toBeScanned = promoBoard;
					isolator(promotionb, moveflags::none, x, promotioncapture);
					toBeScanned = promoBoard;
					isolator(promotionn, moveflags::none, x, promotioncapture);
					toBeScanned = promoBoard;
					isolator(promotionq, moveflags::none, x, promotioncapture);
					toBeScanned = promoBoard;
					isolator(promotionr, moveflags::none, x, promotioncapture);
				}

				nextBoard &= (~first);
				if (nextBoard) {
					toBeScanned = nextBoard;
					isolator(capture, moveflags::none, x, moveflags::none);
				}
			}

			if (gsRef->enPassant) {
				nextBoard = ((piecesNotPinned & southwest) >> 7) & gsRef->enPassant;
				if (nextBoard) { 
					if (passantCrossLegal() && passantStraightLegal(nextBoard << 7)) {
						summand = -7; toBeScanned = nextBoard; isolator(passant, moveflags::none, x, moveflags::none);
					}
				}
				nextBoard = ((piecesNotPinned & southeast) >> 9) & gsRef->enPassant;
				if (nextBoard) { 
					if (passantCrossLegal() && passantStraightLegal(nextBoard << 9)) {
						summand = -9; toBeScanned = nextBoard; isolator(passant, moveflags::none, x, moveflags::none);
					}
				}
			}
		}

		if (piecesPinnedS) {
			nextBoard = ((piecesPinnedS >> 8) & empty) & straightPin;
			if (nextBoard) {

				summand = -8;

				promoBoard = nextBoard & first;
				if (promoBoard) {
					toBeScanned = promoBoard;
					isolator(promotionb, moveflags::none, x, moveflags::none);
					toBeScanned = promoBoard;
					isolator(promotionn, moveflags::none, x, moveflags::none);
					toBeScanned = promoBoard;
					isolator(promotionq, moveflags::none, x, moveflags::none);
					toBeScanned = promoBoard;
					isolator(promotionr, moveflags::none, x, moveflags::none);
				}

				nextBoard &= (~first);
				if (nextBoard) {
					toBeScanned = nextBoard;
					isolator(normal, moveflags::none, x, moveflags::none);
				}

				nextBoard &= sixth;
				nextBoard >>= 8;
				nextBoard &= empty;
				if (nextBoard) { summand = -16; toBeScanned = nextBoard; isolator(normal, moveflags::none, x, moveflags::none); }

			}
		}

		if (piecesPinnedD) {
			nextBoard = ((piecesPinnedD & southwest) >> 7) & enemy & diagonalPin;

			if (nextBoard) {

				summand = -7;

				promoBoard = nextBoard & first;
				if (promoBoard) {
					toBeScanned = promoBoard;
					isolator(promotionb, moveflags::none, x, promotioncapture);
					toBeScanned = promoBoard;
					isolator(promotionn, moveflags::none, x, promotioncapture);
					toBeScanned = promoBoard;
					isolator(promotionq, moveflags::none, x, promotioncapture);
					toBeScanned = promoBoard;
					isolator(promotionr, moveflags::none, x, promotioncapture);
				}

				nextBoard &= (~first);
				if (nextBoard) {
					toBeScanned = nextBoard;
					isolator(capture, moveflags::none, x, moveflags::none);
				}
			}

			nextBoard = ((piecesPinnedD & southeast) >> 9) & enemy & diagonalPin;

			if (nextBoard) {

				summand = -9;

				promoBoard = nextBoard & first;
				if (promoBoard) {
					toBeScanned = promoBoard;
					isolator(promotionb, moveflags::none, x, promotioncapture);
					toBeScanned = promoBoard;
					isolator(promotionn, moveflags::none, x, promotioncapture);
					toBeScanned = promoBoard;
					isolator(promotionq, moveflags::none, x, promotioncapture);
					toBeScanned = promoBoard;
					isolator(promotionr, moveflags::none, x, promotioncapture);
				}

				nextBoard &= (~first);
				if (nextBoard) {
					toBeScanned = nextBoard;
					isolator(capture, moveflags::none, x, moveflags::none);
				}
			}

			if (gsRef->enPassant) {
				nextBoard = ((piecesPinnedD & southwest) >> 7) & gsRef->enPassant & diagonalPin;
				if (nextBoard) { summand = -7; toBeScanned = nextBoard; isolator(passant, moveflags::none, x, moveflags::none); }
				nextBoard = ((piecesPinnedD & southeast) >> 9) & gsRef->enPassant & diagonalPin;
				if (nextBoard) { summand = -9; toBeScanned = nextBoard; isolator(passant, moveflags::none, x, moveflags::none); }
			}
		}

	}

}
void Movegenerator::pawngenC() {

	if (gsRef->white) {

		if (piecesNotPinned) {
			nextBoard = ((piecesNotPinned << 8) & empty);

			if (nextBoard) {

				summand = 8;

				nextBoard &= checkMask;

				if (nextBoard) {

					promoBoard = nextBoard & eighth;
					if (promoBoard) {
						toBeScanned = promoBoard;
						isolator(promotionb, moveflags::none, x, moveflags::none);
						toBeScanned = promoBoard;
						isolator(promotionn, moveflags::none, x, moveflags::none);
						toBeScanned = promoBoard;
						isolator(promotionq, moveflags::none, x, moveflags::none);
						toBeScanned = promoBoard;
						isolator(promotionr, moveflags::none, x, moveflags::none);
					}

					nextBoard &= (~eighth);
					if (nextBoard) {
						toBeScanned = nextBoard;
						isolator(normal, moveflags::none, x, moveflags::none);
					}
				}

				nextBoard = ((piecesNotPinned << 8) & empty) & third;
				nextBoard <<= 8;
				nextBoard &= empty & checkMask;
				if (nextBoard) { summand = 16; toBeScanned = nextBoard; isolator(normal, moveflags::none, x, moveflags::none); }
			}


			nextBoard = ((piecesNotPinned & northeast) << 7) & enemy;

			if (nextBoard) {

				summand = 7;

				nextBoard &= checkMask;

				if (nextBoard) {

					promoBoard = nextBoard & eighth;
					if (promoBoard) {
						toBeScanned = promoBoard;
						isolator(promotionb, moveflags::none, x, promotioncapture);
						toBeScanned = promoBoard;
						isolator(promotionn, moveflags::none, x, promotioncapture);
						toBeScanned = promoBoard;
						isolator(promotionq, moveflags::none, x, promotioncapture);
						toBeScanned = promoBoard;
						isolator(promotionr, moveflags::none, x, promotioncapture);
					}

					nextBoard &= (~eighth);
					if (nextBoard) {
						toBeScanned = nextBoard;
						isolator(capture, moveflags::none, x, moveflags::none);
					}
				}
			}

			nextBoard = ((piecesNotPinned & northwest) << 9) & enemy;

			if (nextBoard) {

				summand = 9;

				nextBoard &= checkMask;

				if (nextBoard) {

					promoBoard = nextBoard & eighth;
					if (promoBoard) {
						toBeScanned = promoBoard;
						isolator(promotionb, moveflags::none, x, promotioncapture);
						toBeScanned = promoBoard;
						isolator(promotionn, moveflags::none, x, promotioncapture);
						toBeScanned = promoBoard;
						isolator(promotionq, moveflags::none, x, promotioncapture);
						toBeScanned = promoBoard;
						isolator(promotionr, moveflags::none, x, promotioncapture);
					}

					nextBoard &= (~eighth);
					if (nextBoard) {
						toBeScanned = nextBoard;
						isolator(capture, moveflags::none, x, moveflags::none);
					}
				}
			}

			if (gsRef->enPassant) {
				nextBoard = ((piecesNotPinned & northeast) << 7) & gsRef->enPassant & (checkMask | (checkMask << 8));
				if (nextBoard) { summand = 7; toBeScanned = nextBoard; isolator(passant, moveflags::none, x, moveflags::none); }
				nextBoard = ((piecesNotPinned & northwest) << 9) & gsRef->enPassant & (checkMask | (checkMask << 8));
				if (nextBoard) { summand = 9; toBeScanned = nextBoard; isolator(passant, moveflags::none, x, moveflags::none); }
			}
		}

	}
	else {

		if (piecesNotPinned) {
			nextBoard = ((piecesNotPinned >> 8) & empty);

			if (nextBoard) {

				summand = -8;

				nextBoard &= checkMask;

				if (nextBoard) {

					promoBoard = nextBoard & first;
					if (promoBoard) {
						toBeScanned = promoBoard;
						isolator(promotionb, moveflags::none, x, moveflags::none);
						toBeScanned = promoBoard;
						isolator(promotionn, moveflags::none, x, moveflags::none);
						toBeScanned = promoBoard;
						isolator(promotionq, moveflags::none, x, moveflags::none);
						toBeScanned = promoBoard;
						isolator(promotionr, moveflags::none, x, moveflags::none);
					}

					nextBoard &= (~first);
					if (nextBoard) {
						toBeScanned = nextBoard;
						isolator(normal, moveflags::none, x, moveflags::none);
					}
				}

				nextBoard = ((piecesNotPinned >> 8) & empty) & sixth;
				nextBoard >>= 8;
				nextBoard &= empty & checkMask;
				if (nextBoard) { summand = -16; toBeScanned = nextBoard; isolator(normal, moveflags::none, x, moveflags::none); }

			}


			nextBoard = ((piecesNotPinned & southwest) >> 7) & enemy;

			if (nextBoard) {

				summand = -7;

				nextBoard &= checkMask;

				if (nextBoard) {

					promoBoard = nextBoard & first;
					if (promoBoard) {
						toBeScanned = promoBoard;
						isolator(promotionb, moveflags::none, x, promotioncapture);
						toBeScanned = promoBoard;
						isolator(promotionn, moveflags::none, x, promotioncapture);
						toBeScanned = promoBoard;
						isolator(promotionq, moveflags::none, x, promotioncapture);
						toBeScanned = promoBoard;
						isolator(promotionr, moveflags::none, x, promotioncapture);
					}

					nextBoard &= (~first);
					if (nextBoard) {
						toBeScanned = nextBoard;
						isolator(capture, moveflags::none, x, moveflags::none);
					}
				}
			}

			nextBoard = ((piecesNotPinned & southeast) >> 9) & enemy;

			if (nextBoard) {

				summand = -9;

				nextBoard &= checkMask;

				if (nextBoard) {

					promoBoard = nextBoard & first;
					if (promoBoard) {
						toBeScanned = promoBoard;
						isolator(promotionb, moveflags::none, x, promotioncapture);
						toBeScanned = promoBoard;
						isolator(promotionn, moveflags::none, x, promotioncapture);
						toBeScanned = promoBoard;
						isolator(promotionq, moveflags::none, x, promotioncapture);
						toBeScanned = promoBoard;
						isolator(promotionr, moveflags::none, x, promotioncapture);
					}

					nextBoard &= (~first);
					if (nextBoard) {
						toBeScanned = nextBoard;
						isolator(capture, moveflags::none, x, moveflags::none);
					}
				}
			}

			if (gsRef->enPassant) {
				nextBoard = ((piecesNotPinned & southwest) >> 7) & gsRef->enPassant & (checkMask | (checkMask >> 8));
				if (nextBoard) { summand = -7; toBeScanned = nextBoard; isolator(passant, moveflags::none, x, moveflags::none); }
				nextBoard = ((piecesNotPinned & southeast) >> 9) & gsRef->enPassant & (checkMask | (checkMask >> 8));
				if (nextBoard) { summand = -9; toBeScanned = nextBoard; isolator(passant, moveflags::none, x, moveflags::none); }
			}
		}

	}

}


void Movegenerator::sliderMask(Direction direction) {
	loggedPin = false;
	nextBoard = king;
	localMask = 0;
	do {
		nextBoard &= bordermap1[direction];
		nextBoard <<= direction;
		localMask |= nextBoard;
		if (nextBoard & occ) {
			if (nextBoard & own) {
				if (loggedPin) { break; }
				loggedPin = true;
			}
			else if (nextBoard & ((direction == 1 || direction == 8) ? estraight : ediagonal)) {
				if (loggedPin) { ((direction == 1 || direction == 8) ? straightPin : diagonalPin) |= localMask; break; }
				else if (loggedCheck) { checkMask = full; checkRing |= (king >> direction); break; }
				checkMask |= localMask; loggedCheck = true; 
				checkRing |= (king >> direction);
				break;
			}
			else { break; }
		}
	} while (nextBoard);

	loggedPin = false;
	nextBoard = king;
	localMask = 0;
	do {
		nextBoard &= bordermap2[direction];
		nextBoard >>= direction;
		localMask |= nextBoard;
		if (nextBoard & occ) {
			if (nextBoard & own) {
				if (loggedPin) { break; }
				loggedPin = true;
			}
			else if (nextBoard & ((direction == 1 || direction == 8) ? estraight : ediagonal)) {
				if (loggedPin) { ((direction == 1 || direction == 8) ? straightPin : diagonalPin) |= localMask; break; }
				else if (loggedCheck) { checkMask = full; checkRing |= (king << direction); break; }
				checkMask |= localMask; loggedCheck = true; 
				checkRing |= (king << direction);
				break;
			}
			else { break; }
		}
	} while (nextBoard);
}
void Movegenerator::sliderCheckRing(Direction direction) {
	nextBoard = snip;
	do {
		nextBoard &= bordermap1[direction];
		nextBoard <<= direction;
		if (nextBoard & ((direction == 1 || direction == 8) ? estraight : ediagonal)) {
			checkRing |= snip;
			return;
		}
		if (!(nextBoard & empty)) { break; }
	} while (true);

	nextBoard = snip;
	do {
		nextBoard &= bordermap2[direction];
		nextBoard >>= direction;
		if (nextBoard & ((direction == 1 || direction == 8) ? estraight : ediagonal)) {
			checkRing |= snip;
			return;
		}
		if (!(nextBoard & empty)) { break; }
	} while (true);
}
bool Movegenerator::slideratk(Direction direction, int sq) {
	Bitboard scan{};
	scan = static_cast<Bitboard>(1) << sq;
	do {
		scan &= bordermap1[direction];
		scan <<= direction;
		if (scan & occ) {
			if (scan & ((direction == 1 || direction == 8) ? estraight : ediagonal)) {
				return true;
			}
			else { break; }
		}
	} while (scan);

	scan = static_cast<Bitboard>(1) << sq;
	do {
		scan &= bordermap2[direction];
		scan >>= direction;
		if (scan & occ) {
			if (scan & ((direction == 1 || direction == 8) ? estraight : ediagonal)) {
				return true;
			}
			else { break; }
		}
	} while (scan);
	return false;
}


void Movegenerator::makeMasks() {

	initMasks();


	sliderMask(y);

	sliderMask(x);

	sliderMask(z2);

	sliderMask(z);

	if (gsRef->white) {
		nextBoard = ((epawns & southwest) >> 7);
		if (nextBoard & king) {
			if (loggedCheck) { checkMask = full; }
			else { checkMask |= (king << 7); }
		}
		nextBoard = ((epawns &southeast) >> 9);
		if (nextBoard & king) {
			if (loggedCheck) { checkMask = full; }
			else { checkMask |= (king << 9); }
		}
	}
	else {
		nextBoard = ((epawns & northeast) << 7);
		if (nextBoard & king) { 
			if (loggedCheck) { checkMask = full; }
			else { checkMask |= (king >> 7); }
		}
		nextBoard = ((epawns & northwest) << 9);
		if (nextBoard & king) { 
			if (loggedCheck) { checkMask = full; }
			else { checkMask |= (king >> 9); }
		}
	}

	nextBoard = ((king & dll) >> 6);
	if (nextBoard & eknights) { checkMask |= nextBoard; checkRing |= (king << 9); if (loggedCheck) { checkMask = full; }; }
	nextBoard = ((king & urr) << 6);
	if (nextBoard & eknights) { checkMask |= nextBoard; checkRing |= (king >> 9); if (loggedCheck) { checkMask = full; }; }
	nextBoard = ((king & drr) >> 10);
	if (nextBoard & eknights) { checkMask |= nextBoard; checkRing |= (king << 7); if (loggedCheck) { checkMask = full; }; }
	nextBoard = ((king & ull) << 10);
	if (nextBoard & eknights) { checkMask |= nextBoard; checkRing |= (king >> 7); if (loggedCheck) { checkMask = full; }; }
	nextBoard = ((king & ddl) >> 15);
	if (nextBoard & eknights) { checkMask |= nextBoard; checkRing |= (king >> 9); if (loggedCheck) { checkMask = full; }; }
	nextBoard = ((king & uur) << 15);
	if (nextBoard & eknights) { checkMask |= nextBoard; checkRing |= (king << 9); if (loggedCheck) { checkMask = full; }; }
	nextBoard = ((king & ddr) >> 17);
	if (nextBoard & eknights) { checkMask |= nextBoard; checkRing |= (king >> 7); if (loggedCheck) { checkMask = full; }; }
	nextBoard = ((king & uul) << 17);
	if (nextBoard & eknights) { checkMask |= nextBoard; checkRing |= (king << 7); if (loggedCheck) { checkMask = full; }; }

	//remaining ringmask
	ring &= ~checkRing;

	while (ring) {

		_BitScanForward64(&mask, ring);

		snip = static_cast<Bitboard>(0b1) << mask;

		sliderCheckRing(x);
		sliderCheckRing(z2);
		sliderCheckRing(y);
		sliderCheckRing(z);

		if (!gsRef->white) {
			nextBoard = ((epawns & northeast) << 7);
			if (nextBoard & snip) { checkRing |= snip; ring &= ring - 1; continue; }
			nextBoard = ((epawns & northwest) << 9);
			if (nextBoard & snip) { checkRing |= snip; ring &= ring - 1; continue; }
		}
		else {
			nextBoard = ((epawns & southwest) >> 7);
			if (nextBoard & snip) { checkRing |= snip; ring &= ring - 1; continue; }
			nextBoard = ((epawns & southeast) >> 9);
			if (nextBoard & snip) { checkRing |= snip; ring &= ring - 1; continue; }
		}



		nextBoard = ((snip & dll) >> 6);
		if (nextBoard & eknights) { checkRing |= snip; ring &= ring - 1; continue; }
		nextBoard = ((snip & urr) << 6);
		if (nextBoard & eknights) { checkRing |= snip; ring &= ring - 1; continue; }
		nextBoard = ((snip & drr) >> 10);
		if (nextBoard & eknights) { checkRing |= snip; ring &= ring - 1; continue; }
		nextBoard = ((snip & ull) << 10);
		if (nextBoard & eknights) { checkRing |= snip; ring &= ring - 1; continue; }
		nextBoard = ((snip & ddl) >> 15);
		if (nextBoard & eknights) { checkRing |= snip; ring &= ring - 1; continue; }
		nextBoard = ((snip & uur) << 15);
		if (nextBoard & eknights) { checkRing |= snip; ring &= ring - 1; continue; }
		nextBoard = ((snip & ddr) >> 17);
		if (nextBoard & eknights) { checkRing |= snip; ring &= ring - 1; continue; }
		nextBoard = ((snip & uul) << 17);
		if (nextBoard & eknights) { checkRing |= snip; ring &= ring - 1; continue; }



		nextBoard = ((snip & west) << 1) & eking;
		if (nextBoard) { checkRing |= snip; ring &= ring - 1; continue; }
		nextBoard = ((snip & east) >> 1) & eking;
		if (nextBoard) { checkRing |= snip; ring &= ring - 1; continue; }
		nextBoard = ((snip & north) << 8) & eking;
		if (nextBoard) { checkRing |= snip; ring &= ring - 1; continue; }
		nextBoard = ((snip & south) >> 8) & eking;
		if (nextBoard) { checkRing |= snip; ring &= ring - 1; continue; }
		nextBoard = ((snip & northeast) << 7) & eking;
		if (nextBoard) { checkRing |= snip; ring &= ring - 1; continue; }
		nextBoard = ((snip & southwest) >> 7) & eking;
		if (nextBoard) { checkRing |= snip; ring &= ring - 1; continue; }
		nextBoard = ((snip & northwest) << 9) & eking;
		if (nextBoard) { checkRing |= snip; ring &= ring - 1; continue; }
		nextBoard = ((snip & southeast) >> 9) & eking;
		if (nextBoard) { checkRing |= snip; ring &= ring - 1; continue; }

		ring &= ring - 1;
	};

	noCheck = ~checkRing;
}
bool Movegenerator::attacked(Square sq) {
	if (slideratk(x, sq) || slideratk(z2, sq) || slideratk(y, sq) || slideratk(z, sq)) { return true; }

	Bitboard square = static_cast<Bitboard>(1) << sq;

	if (!gsRef->white) {
		nextBoard = ((epawns & northeast) << 7);
		if (nextBoard & square) { return true; }
		nextBoard = ((epawns & northwest) << 9);
		if (nextBoard & square) { return true; }
	}
	else {
		nextBoard = ((epawns & southwest) >> 7);
		if (nextBoard & square) { return true; }
		nextBoard = ((epawns & southeast) >> 9);
		if (nextBoard & square) { return true; }
	}

	nextBoard = ((square & dll) >> 6);
	if (nextBoard & eknights) { return true; }
	nextBoard = ((square & urr) << 6);
	if (nextBoard & eknights) { return true; }
	nextBoard = ((square & drr) >> 10);
	if (nextBoard & eknights) { return true; }
	nextBoard = ((square & ull) << 10);
	if (nextBoard & eknights) { return true; }
	nextBoard = ((square & ddl) >> 15);
	if (nextBoard & eknights) { return true; }
	nextBoard = ((square & uur) << 15);
	if (nextBoard & eknights) { return true; }
	nextBoard = ((square & ddr) >> 17);
	if (nextBoard & eknights) { return true; }
	nextBoard = ((square & uul) << 17);
	if (nextBoard & eknights) { return true; }

	nextBoard = ((square & west) << 1) & eking;
	if (nextBoard) { return true; }
	nextBoard = ((square & east) >> 1) & eking;
	if (nextBoard) { return true; }
	nextBoard = ((square & north) << 8) & eking;
	if (nextBoard) { return true; }
	nextBoard = ((square & south) >> 8) & eking;
	if (nextBoard) { return true; }
	nextBoard = ((square & northeast) << 7) & eking;
	if (nextBoard) { return true; }
	nextBoard = ((square & southwest) >> 7) & eking;
	if (nextBoard) { return true; }
	nextBoard = ((square & northwest) << 9) & eking;
	if (nextBoard) { return true; }
	nextBoard = ((square & southeast) >> 9) & eking;
	if (nextBoard) { return true; }

	return false;
}

void Movegenerator::generate() {

	initGenerate();

	if (!checkMask) { // no check

		notPinned = ~(straightPin | diagonalPin);

		// p
		piecesNotPinned = pawns & notPinned;
		piecesPinnedD = pawns & diagonalPin;
		piecesPinnedS = pawns & straightPin;

		pawngen();

		// n
		piecesNotPinned = knights & notPinned;

		nextBoard = ((piecesNotPinned & dll) >> 6) & emptyorenemy;
		if (nextBoard & enemy) { toBeScanned = nextBoard & enemy; summand = -6; isolator(capture, moveflags::none, x, moveflags::none); }
		if (nextBoard & empty) { toBeScanned = nextBoard & empty; summand = -6; isolator(normal, moveflags::none, x, moveflags::none); }
		nextBoard = ((piecesNotPinned & urr) << 6) & emptyorenemy;
		if (nextBoard & enemy) { toBeScanned = nextBoard & enemy; summand = 6; isolator(capture, moveflags::none, x, moveflags::none); }
		if (nextBoard & empty) { toBeScanned = nextBoard & empty; summand = 6; isolator(normal, moveflags::none, x, moveflags::none); }
		nextBoard = ((piecesNotPinned & drr) >> 10) & emptyorenemy;
		if (nextBoard & enemy) { toBeScanned = nextBoard & enemy; summand = -10; isolator(capture, moveflags::none, x, moveflags::none); }
		if (nextBoard & empty) { toBeScanned = nextBoard & empty; summand = -10; isolator(normal, moveflags::none, x, moveflags::none); }
		nextBoard = ((piecesNotPinned & ull) << 10) & emptyorenemy;
		if (nextBoard & enemy) { toBeScanned = nextBoard & enemy; summand = 10; isolator(capture, moveflags::none, x, moveflags::none); }
		if (nextBoard & empty) { toBeScanned = nextBoard & empty; summand = 10; isolator(normal, moveflags::none, x, moveflags::none); }
		nextBoard = ((piecesNotPinned & ddl) >> 15) & emptyorenemy;
		if (nextBoard & enemy) { toBeScanned = nextBoard & enemy; summand = -15; isolator(capture, moveflags::none, x, moveflags::none); }
		if (nextBoard & empty) { toBeScanned = nextBoard & empty; summand = -15; isolator(normal, moveflags::none, x, moveflags::none); }
		nextBoard = ((piecesNotPinned & uur) << 15) & emptyorenemy;
		if (nextBoard & enemy) { toBeScanned = nextBoard & enemy; summand = 15; isolator(capture, moveflags::none, x, moveflags::none); }
		if (nextBoard & empty) { toBeScanned = nextBoard & empty; summand = 15; isolator(normal, moveflags::none, x, moveflags::none); }
		nextBoard = ((piecesNotPinned & uul) << 17) & emptyorenemy;
		if (nextBoard & enemy) { toBeScanned = nextBoard & enemy; summand = 17; isolator(capture, moveflags::none, x, moveflags::none); }
		if (nextBoard & empty) { toBeScanned = nextBoard & empty; summand = 17; isolator(normal, moveflags::none, x, moveflags::none); }
		nextBoard = ((piecesNotPinned & ddr) >> 17) & emptyorenemy;
		if (nextBoard & enemy) { toBeScanned = nextBoard & enemy; summand = -17; isolator(capture, moveflags::none, x, moveflags::none); }
		if (nextBoard & empty) { toBeScanned = nextBoard & empty; summand = -17; isolator(normal, moveflags::none, x, moveflags::none); }

		// q
		piecesNotPinned = queens & notPinned;
		piecesPinnedD = queens & diagonalPin;
		piecesPinnedS = queens & straightPin;

		if (piecesNotPinned) {
			slider(x, PType::none);
			slider(z2, PType::none);
			slider(y, PType::none);
			slider(z, PType::none);
		}
		if (piecesPinnedD) {
			slider(z2, diagonal);
			slider(z, diagonal);
		}
		if (piecesPinnedS) {
			slider(x, straight);
			slider(y, straight);
		}

		// r
		piecesNotPinned = rooks & notPinned;
		piecesPinnedS = rooks & straightPin;

		if (piecesNotPinned) {
			slider(x, PType::none);
			slider(y, PType::none);
		}
		if (piecesPinnedS) {
			slider(x, straight);
			slider(y, straight);
		}

		// b
		piecesNotPinned = bishops & notPinned;
		piecesPinnedD = bishops & diagonalPin;

		if (piecesNotPinned) {
			slider(z2, PType::none);
			slider(z, PType::none);
		}
		if (piecesPinnedD) {
			slider(z2, diagonal);
			slider(z, diagonal);
		}

		// k
		kinggen();

		// castling
		if (gsRef->white) {
			if (gsRef->castlingRights[wk] && !(wkOcc & occ) && !(wkCheck & checkRing) && !attacked(g1)) { isolator(castles, wkflag, null, moveflags::none); };
			if (gsRef->castlingRights[wq] && !(wqOcc & occ) && !(wqCheck & checkRing) && !attacked(c1)) { isolator(castles, wqflag, null, moveflags::none); };
		}
		else {
			if (gsRef->castlingRights[bk] && !(bkOcc & occ) && !(bkCheck & checkRing) && !attacked(g8)) { isolator(castles, bkflag, null, moveflags::none); };
			if (gsRef->castlingRights[bq] &&	!(bqOcc & occ) && !(bqCheck & checkRing) && !attacked(c8)) { isolator(castles, bqflag, null, moveflags::none); };
		}

		if (*movenum == 0) { 
			*movelistRef = stalemate; 
		}
	}
	else if (checkMask == full) { // double-check

		kinggen();

		if (*movenum == 0) { *movelistRef = checkmate; } // checkmate
	}
	else { // normal check

		notPinned = ~(straightPin | diagonalPin);

		// p
		piecesNotPinned = pawns & notPinned;

		pawngenC();

		// n
		piecesNotPinned = knights & notPinned;

		nextBoard = ((piecesNotPinned & dll) >> 6) & emptyorenemy & checkMask;
		if (nextBoard & enemy) { toBeScanned = nextBoard & enemy; summand = -6; isolator(capture, moveflags::none, x, moveflags::none); }
		if (nextBoard & empty) { toBeScanned = nextBoard & empty; summand = -6; isolator(normal, moveflags::none, x, moveflags::none); }
		nextBoard = ((piecesNotPinned & urr) << 6) & emptyorenemy & checkMask;
		if (nextBoard & enemy) { toBeScanned = nextBoard & enemy; summand = 6; isolator(capture, moveflags::none, x, moveflags::none); }
		if (nextBoard & empty) { toBeScanned = nextBoard & empty; summand = 6; isolator(normal, moveflags::none, x, moveflags::none); }
		nextBoard = ((piecesNotPinned & drr) >> 10) & emptyorenemy & checkMask;
		if (nextBoard & enemy) { toBeScanned = nextBoard & enemy; summand = -10; isolator(capture, moveflags::none, x, moveflags::none); }
		if (nextBoard & empty) { toBeScanned = nextBoard & empty; summand = -10; isolator(normal, moveflags::none, x, moveflags::none); }
		nextBoard = ((piecesNotPinned & ull) << 10) & emptyorenemy & checkMask;
		if (nextBoard & enemy) { toBeScanned = nextBoard & enemy; summand = 10; isolator(capture, moveflags::none, x, moveflags::none); }
		if (nextBoard & empty) { toBeScanned = nextBoard & empty; summand = 10; isolator(normal, moveflags::none, x, moveflags::none); }
		nextBoard = ((piecesNotPinned & ddl) >> 15) & emptyorenemy & checkMask;
		if (nextBoard & enemy) { toBeScanned = nextBoard & enemy; summand = -15; isolator(capture, moveflags::none, x, moveflags::none); }
		if (nextBoard & empty) { toBeScanned = nextBoard & empty; summand = -15; isolator(normal, moveflags::none, x, moveflags::none); }
		nextBoard = ((piecesNotPinned & uur) << 15) & emptyorenemy & checkMask;
		if (nextBoard & enemy) { toBeScanned = nextBoard & enemy; summand = 15; isolator(capture, moveflags::none, x, moveflags::none); }
		if (nextBoard & empty) { toBeScanned = nextBoard & empty; summand = 15; isolator(normal, moveflags::none, x, moveflags::none); }
		nextBoard = ((piecesNotPinned & uul) << 17) & emptyorenemy & checkMask;
		if (nextBoard & enemy) { toBeScanned = nextBoard & enemy; summand = 17; isolator(capture, moveflags::none, x, moveflags::none); }
		if (nextBoard & empty) { toBeScanned = nextBoard & empty; summand = 17; isolator(normal, moveflags::none, x, moveflags::none); }
		nextBoard = ((piecesNotPinned & ddr) >> 17) & emptyorenemy & checkMask;
		if (nextBoard & enemy) { toBeScanned = nextBoard & enemy; summand = -17; isolator(capture, moveflags::none, x, moveflags::none); }
		if (nextBoard & empty) { toBeScanned = nextBoard & empty; summand = -17; isolator(normal, moveflags::none, x, moveflags::none); }

		// q
		piecesNotPinned = queens & notPinned;

		if (piecesNotPinned) {
			sliderC(x, PType::none);
			sliderC(z2, PType::none);
			sliderC(y, PType::none);
			sliderC(z, PType::none);
		}

		// r
		piecesNotPinned = rooks & notPinned;

		if (piecesNotPinned) {
			sliderC(x, PType::none);
			sliderC(y, PType::none);
		}

		// b
		piecesNotPinned = bishops & notPinned;

		if (piecesNotPinned) {
			sliderC(z2, PType::none);
			sliderC(z, PType::none);
		}

		// k
		kinggen();

		if (*movenum == 0) { *movelistRef = checkmate; } // checkmate
	}
}
