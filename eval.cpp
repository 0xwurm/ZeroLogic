#include "eval.h"

using namespace ZeroLogic;
using namespace Evaluation;

Files files[8] = { A, Bf, C, D, E, F, G, H };

Eval::Eval(Gamestate* g) {

	this->g = g;
	this->p = phase();

}

int Eval::pieces() {

	int val = 0;

	val += __popcnt64(g->position[wP]);
	val -= __popcnt64(g->position[bP]);
	val += 3 * __popcnt64(g->position[wB]);
	val -= 3 * __popcnt64(g->position[bB]);
	val += 3 * __popcnt64(g->position[wN]);
	val -= 3 * __popcnt64(g->position[bN]);
	val += 5 * __popcnt64(g->position[wR]);
	val -= 5 * __popcnt64(g->position[bR]);
	val += 9 * __popcnt64(g->position[wQ]);
	val -= 9 * __popcnt64(g->position[bQ]);

	val *= 100;

	return val;

}

template <>
int Eval::pawn<opening>() {

	int val = 0;

	val += 30 * __popcnt64(g->position[wP] & center);
	val -= 30 * __popcnt64(g->position[bP] & center);
	val += 10 * __popcnt64(g->position[wP] & extendedCenter);
	val -= 10 * __popcnt64(g->position[bP] & extendedCenter);

	return val;

}

template <>
int Eval::pawn<midgame>() {

	int val = 0;

	// pawnchains
	Bitboard placeholder = g->position[wP];
	for (int i = 1; i < 4 && placeholder; i++) { placeholder &= placeholder << 7; if (placeholder) val += (i * 5); }
	placeholder = g->position[wP];
	for (int i = 1; i < 4 && placeholder; i++) { placeholder &= placeholder << 9; if (placeholder) val += (i * 5); }
	placeholder = g->position[bP];
	for (int i = 1; i < 4 && placeholder; i++) { placeholder &= placeholder >> 7; if (placeholder) val -= (i * 5); }
	placeholder = g->position[bP];
	for (int i = 1; i < 4 && placeholder; i++) { placeholder &= placeholder >> 9; if (placeholder) val -= (i * 5); }

	return val;
}

template <>
int Eval::pawn<endgame>() {

	int val = 0;

	val -= 5 * __popcnt64(g->position[wP] & lower);
	val += 5 * __popcnt64(g->position[bP] & upper);

	val += 20 * __popcnt64(g->position[wP] & fifth);
	val += 55 * __popcnt64(g->position[wP] & sixth);
	val += 100 * __popcnt64(g->position[wP] & seventh);

	val -= 20 * __popcnt64(g->position[bP] & fourth);
	val -= 55 * __popcnt64(g->position[bP] & third);
	val -= 100 * __popcnt64(g->position[bP] & second);

	// passers
	for (int i = 0; i < 8; i++) {
		bool w = g->position[wP] & files[i];
		bool b = g->position[bP] & files[i];
		if (w && !b) {
			val += 10;
			bool l = (i) ? g->position[bP] & files[i - 1] : false;
			bool r = (i != 7) ? g->position[bP] & files[i + 1] : false;
			if (!(l && r)) val += 40;
			else if (!l) val += 5;
			else if (!r) val += 5;
		}
		else if (!w && b) {
			val -= 10;
			bool l = (i) ? g->position[wP] & files[i - 1] : false;
			bool r = (i != 7) ? g->position[wP] & files[i + 1] : false;
			if (!(l && r)) val -= 40;
			else if (!l) val -= 5;
			else if (!r) val -= 5;
		}

	}

	return val;

}

template <>
int Eval::king<opening>() {

	int val = 0;

	if (g->position[wK] & wsafe) val += 30;
	if (g->position[bK] & wsafe) val -= 30;

	return val;

}

template <>
int Eval::king<midgame>() {

	int val = 0;

	if (g->position[wK] & wsafe) val += 30;
	if (g->position[bK] & bsafe) val -= 30;

	Bitboard wRing = 0;
	wRing |= ((g->position[wK] & Movegen::east) >> 1);
	wRing |= ((g->position[wK] & Movegen::west) << 1);
	wRing |= ((g->position[wK] & Movegen::north) << 8);
	wRing |= ((g->position[wK] & Movegen::south) >> 8);
	wRing |= ((g->position[wK] & Movegen::southeast) >> 9);
	wRing |= ((g->position[wK] & Movegen::northwest) << 9);
	wRing |= ((g->position[wK] & Movegen::northeast) << 7);
	wRing |= ((g->position[wK] & Movegen::southwest) >> 7);
	Bitboard bRing = 0;
	bRing |= ((g->position[bK] & Movegen::east) >> 1);
	bRing |= ((g->position[bK] & Movegen::west) << 1);
	bRing |= ((g->position[bK] & Movegen::north) << 8);
	bRing |= ((g->position[bK] & Movegen::south) >> 8);
	bRing |= ((g->position[bK] & Movegen::southeast) >> 9);
	bRing |= ((g->position[bK] & Movegen::northwest) << 9);
	bRing |= ((g->position[bK] & Movegen::northeast) << 7);
	bRing |= ((g->position[bK] & Movegen::southwest) >> 7);

	val += 10 * __popcnt64(g->position[wP] & wRing);
	val -= 10 * __popcnt64(g->position[bP] & bRing);

	// magic bitboards for checkring

	return val;

}

template <>
int Eval::king<endgame>() {

	int val = 0;

	Bitboard wRing = 0;
	wRing |= ((g->position[wK] & Movegen::east) >> 1);
	wRing |= ((g->position[wK] & Movegen::west) << 1);
	wRing |= ((g->position[wK] & Movegen::north) << 8);
	wRing |= ((g->position[wK] & Movegen::south) >> 8);
	wRing |= ((g->position[wK] & Movegen::southeast) >> 9);
	wRing |= ((g->position[wK] & Movegen::northwest) << 9);
	wRing |= ((g->position[wK] & Movegen::northeast) << 7);
	wRing |= ((g->position[wK] & Movegen::southwest) >> 7);
	Bitboard bRing = 0;
	bRing |= ((g->position[bK] & Movegen::east) >> 1);
	bRing |= ((g->position[bK] & Movegen::west) << 1);
	bRing |= ((g->position[bK] & Movegen::north) << 8);
	bRing |= ((g->position[bK] & Movegen::south) >> 8);
	bRing |= ((g->position[bK] & Movegen::southeast) >> 9);
	bRing |= ((g->position[bK] & Movegen::northwest) << 9);
	bRing |= ((g->position[bK] & Movegen::northeast) << 7);
	bRing |= ((g->position[bK] & Movegen::southwest) >> 7);

	val += 10 * __popcnt64(g->position[wP] & wRing);
	val -= 10 * __popcnt64(g->position[bP] & bRing);

	if (g->position[wK] & bigmid) val += 20;
	if (g->position[bK] & bigmid) val -= 20;

	return val;

}

int Eval::positioning() {

	int val = 0;

	if (!p) {
		val += pawn<opening>();
		// val += king<opening>();
	}
	else if (p == 1) {
		val += pawn<midgame>();
		// val += king<midgame>();
	}
	else {
		val += pawn<endgame>();
		// val += king<endgame>();
	}

	return val;

}

int Eval::pval() {

	int val = 0;

	val += __popcnt64(g->position[wP]);
	val += __popcnt64(g->position[bP]);
	val += 3 * __popcnt64(g->position[wB]);
	val += 3 * __popcnt64(g->position[bB]);
	val += 3 * __popcnt64(g->position[wN]);
	val += 3 * __popcnt64(g->position[bN]);
	val += 5 * __popcnt64(g->position[wR]);
	val += 5 * __popcnt64(g->position[bR]);
	val += 9 * __popcnt64(g->position[wQ]);
	val += 9 * __popcnt64(g->position[bQ]);

	return val;

}

Phase Eval::phase() {

	Bitboard all = 0;
	for (int i = 0; i < 12; i++) all |= g->position[i];

	if (__popcnt64(all & bigmid) >= 8 || !g->castlingRights[0] || !g->castlingRights[1] || !g->castlingRights[2] || !g->castlingRights[3] || pval() < 72) {
		if (__popcnt64(all) <= 10 || pval() < 20) return endgame;
		else return midgame;
	}
	else return opening;

}

int Eval::get() {

	int val = pieces() + positioning();

	return val;

}