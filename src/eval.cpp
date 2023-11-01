#include "eval.h"
#if defined(_WIN64)
#   include <intrin.h>
#   define piece_count(a) static_cast<int>(__popcnt64(a))
#endif

using namespace ZeroLogic;
using namespace Evaluation;
/*
Files files[8] = { A, Bf, C, D, E, F, G, H };

Eval::Eval(Gamestate* g) {

	this->g = g;
	this->p = phase();

}

int Eval::pieces() {

	int val = 0;

	val += piece_count(g->position[wP]);
	val -= piece_count(g->position[bP]);
	val += 3 * piece_count(g->position[wB]);
	val -= 3 * piece_count(g->position[bB]);
	val += 3 * piece_count(g->position[wN]);
	val -= 3 * piece_count(g->position[bN]);
	val += 5 * piece_count(g->position[wR]);
	val -= 5 * piece_count(g->position[bR]);
	val += 9 * piece_count(g->position[wQ]);
	val -= 9 * piece_count(g->position[bQ]);

	val *= 100;

	return val;

}

template<Phase p>
int Eval::pawn() {

	int val = 0;

    if (p == opening){
        val += 30 * piece_count(g->position[wP] & center);
        val -= 30 * piece_count(g->position[bP] & center);
        val += 10 * piece_count(g->position[wP] & extendedCenter);
        val -= 10 * piece_count(g->position[bP] & extendedCenter);
    }
    else if (p == midgame){
#define chain(piece, shift, factor) g->position[piece] << (shift * factor)
        // pawn chains (ignore borders)
        val += 3 * piece_count(g->position[wP] & chain(wP, 7, 1) & mid_files);
        val += 5 * piece_count(g->position[wP] & chain(wP, 7, 1) & chain(wP, 7, 2) & mid_files);
        val += 10 * piece_count(g->position[wP] & chain(wP, 7, 1) & chain(wP, 7, 2) & chain(wP, 7, 3) & mid_files);
        val += 3 * piece_count(g->position[wP] & chain(wP, 9, 1) & mid_files);
        val += 5 * piece_count(g->position[wP] & chain(wP, 9, 1) & chain(wP, 9, 2) & mid_files);
        val += 10 * piece_count(g->position[wP] & chain(wP, 9, 1) & chain(wP, 9, 2) & chain(wP, 9, 3) & mid_files);

        val -= 3 * piece_count(g->position[bP] & chain(bP, 7, 1) & mid_files);
        val -= 5 * piece_count(g->position[bP] & chain(bP, 7, 1) & chain(bP, 7, 2) & mid_files);
        val -= 10 * piece_count(g->position[bP] & chain(bP, 7, 1) & chain(bP, 7, 2) & chain(bP, 7, 3) & mid_files);
        val -= 3 * piece_count(g->position[bP] & chain(bP, 9, 1) & mid_files);
        val -= 5 * piece_count(g->position[bP] & chain(bP, 9, 1) & chain(bP, 9, 2) & mid_files);
        val -= 10 * piece_count(g->position[bP] & chain(bP, 9, 1) & chain(bP, 9, 2) & chain(bP, 9, 3) & mid_files);

        // doubled pawns
        val -= 10 * piece_count(g->position[wP] & chain(wP, 8, 1));
        val += 10 * piece_count(g->position[bP] & chain(bP, 8, 1));

        val -= 10 * piece_count(g->position[wP] & flank_mid);
        val += 10 * piece_count(g->position[bP] & flank_mid);
#undef chain
    }
    else{
        val -= 5 * piece_count(g->position[wP] & lower);
        val += 5 * piece_count(g->position[bP] & upper);

        val += 20 * piece_count(g->position[wP] & fifth);
        val += 55 * piece_count(g->position[wP] & sixth);
        val += 100 * piece_count(g->position[wP] & seventh);

        val -= 20 * piece_count(g->position[bP] & fourth);
        val -= 55 * piece_count(g->position[bP] & third);
        val -= 100 * piece_count(g->position[bP] & second);

        // passers
        for (int i = 0; i < 8; i++) {
            bool w = g->position[wP] & files[i];
            bool b = g->position[bP] & files[i];
            if (w && !b) {
                val += 10;
                bool l = i != 0 && g->position[bP] & files[i - 1];
                bool r = i != 7 && g->position[bP] & files[i + 1];
                if (!l && !r) val += 40;
                else if (!l || !r) val += 5;
            }
            else if (!w && b) {
                val -= 10;
                bool l = i != 0 && g->position[wP] & files[i - 1];
                bool r = i != 7 && g->position[wP] & files[i + 1];
                if (!l && !r) val -= 40;
                else if (!l || !r) val -= 5;
            }
        }
    }

	return val;
}

template <Phase p>
int Eval::knight() {

	int val = 0;

    if (p == opening || p == midgame) {
        val += 20 * piece_count(g->position[wN] & extendedCenter);
        val -= 20 * piece_count(g->position[bN] & extendedCenter);
    }

	return val;

}

template <Phase p>
int Eval::bishop() {

	int val = 0;

    if (p == opening || p == midgame) {
        val += 15 * piece_count(g->position[wB] & middleStrip);
        val -= 15 * piece_count(g->position[bB] & middleStrip);
        val += 8 * piece_count(g->position[wB] & extendedStrip);
        val -= 8 * piece_count(g->position[bB] & extendedStrip);
    }

	return val;

}

template <Phase p>
int Eval::king() {

	int val = 0;

    if (p == opening) {
        if (g->position[wK] & wsafe) val += 30;
        if (g->position[bK] & wsafe) val -= 30;
    }
    else if (p == midgame){
        if (g->position[wK] & wsafe) val += 30;
        if (g->position[bK] & bsafe) val -= 30;

        val -= 7 * piece_count(ring(true));
        val += 7 * piece_count(ring(false));
    }
    else{
        if (g->position[wK] & bigmid) val += 20;
        if (g->position[bK] & bigmid) val -= 20;

        val -= 10 * piece_count(ring(true));
        val += 10 * piece_count(ring(false));
    }

	return val;

}

template <Phase p>
int Eval::queen() {
    int val = 0;
    if (p == opening){
        val += 10 * piece_count(g->position[wQ] & queensStarting);
        val -= 10 * piece_count(g->position[bQ] & queensStarting);
    }
    return val;
}

Bitboard get_pawn_attacks(Bitboard origin, bool white){
    if (white)  return ((origin & ~Tables::northeast) << 7) | ((origin & ~Tables::northwest) << 9);
    else        return ((origin & ~Tables::southwest) >> 7) | ((origin & ~Tables::southeast) >> 9);
}

Bitboard Eval::ring(bool do_white){

    int own_const = (do_white) ? 0 : 6;
    int enemy_const = 6 - own_const;

    Bitboard ring = Tables::king_attack_table[std::countr_zero(g->position[K + own_const])];
    Bitboard check_ring;

    Bitboard pawn_attackers = get_pawn_attacks(ring, do_white) & g->position[P + enemy_const];
    if (!do_white)  check_ring |= ((pawn_attackers & ~Tables::northeast) << 7) | ((pawn_attackers & ~Tables::northwest) << 9);
    else         check_ring |= ((pawn_attackers & ~Tables::southwest) >> 7) | ((pawn_attackers & ~Tables::southeast) >> 9);

    ring &= ~check_ring;

    Bitboard occ_no_king;
    for (Bitboard piece : g->position) occ_no_king |= piece;

#define change_ring check_ring |= 1ull << curr_index; ring &= ring - 1; continue;

    while (ring){
        int curr_index = std::countr_zero(ring);

        Bitboard attackers = Tables::knight_attack_table[curr_index] & g->position[N + enemy_const];
        if (attackers) { change_ring }

        attackers = Tables::king_attack_table[curr_index] & g->position[K + enemy_const];
        if (attackers) { change_ring }

        attackers = Tables::get_attack_rook(occ_no_king, curr_index) & (g->position[Q + enemy_const] | g->position[R + enemy_const]);
        if (attackers) { change_ring }

        attackers = Tables::get_attack_bishop(occ_no_king, curr_index) & (g->position[Q + enemy_const] | g->position[B + enemy_const]);
        if (attackers) { change_ring }

        ring &= ring - 1;
    }

    return check_ring;
}

int Eval::positioning() {

	int val = 0;

	if (p == opening) {
		val += pawn<opening>();
		val += king<opening>();
		val += knight<opening>();
		val += bishop<opening>();
        val += queen<opening>();
	}
	else if (p == midgame) {
		val += pawn<midgame>();
		val += king<midgame>();
        val += knight<midgame>();
        val += bishop<midgame>();
	}
	else {
		val += pawn<endgame>();
		val += king<endgame>();
	}

	return val;
}

int Eval::piece_value() {

	int val = 0;

	val += piece_count(g->position[wP]);
	val += piece_count(g->position[bP]);
	val += 3 * piece_count(g->position[wB]);
	val += 3 * piece_count(g->position[bB]);
	val += 3 * piece_count(g->position[wN]);
	val += 3 * piece_count(g->position[bN]);
	val += 5 * piece_count(g->position[wR]);
	val += 5 * piece_count(g->position[bR]);
	val += 9 * piece_count(g->position[wQ]);
	val += 9 * piece_count(g->position[bQ]);

	return val;

}

Phase Eval::phase() {

	Bitboard all = 0;
	for (unsigned long long i : g->position) all |= i;

	if (piece_count(all & bigmid) >= 8 || piece_value() < 72) {
		if (piece_count(all) <= 10 || piece_value() < 20) return endgame;
		else return midgame;
	}
	else return opening;

}

int Eval::get() {

	int val = pieces() + positioning();

	return val;

}*/