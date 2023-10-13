#include "movegen.h"
#include "tables.h"

using namespace ZeroLogic;

const Bitboard universe = 0xffffffffffffffff;
const Bitboard wkOcc = 0x6;
const Bitboard wqOcc = 0x30;
const Bitboard bkOcc = wkOcc << 56;
const Bitboard bqOcc = wqOcc << 56;

template <bool white>
MoveGenerator<white>::MoveGenerator(Gamestate* gs, MoveList* ml) {
    this->gs = gs;
    this->ml = ml;

    constexpr int own_const = (white) ? 0 : 6;
    constexpr int enemy_const = 6 - own_const;

    pos.pawn    = gs->position[P + own_const];
    pos.knight  = gs->position[N + own_const];
    pos.bishop  = gs->position[B + own_const];
    pos.rook    = gs->position[R + own_const];
    pos.queen   = gs->position[Q + own_const];
    pos.king    = gs->position[K + own_const];

    pos.e_pawn    = gs->position[P + enemy_const];
    pos.e_knight  = gs->position[N + enemy_const];
    pos.e_bishop  = gs->position[B + enemy_const];
    pos.e_rook    = gs->position[R + enemy_const];
    pos.e_queen   = gs->position[Q + enemy_const];
    pos.e_king    = gs->position[K + enemy_const];

    pos.own = pos.pawn | pos.knight | pos.bishop | pos.rook | pos.queen | pos.king;
    pos.enemy = pos.e_pawn | pos.e_knight | pos.e_bishop | pos.e_rook | pos.e_queen | pos.e_king;
    pos.occupied = pos.own | pos.enemy;
    pos.empty = ~pos.occupied;

    int index = std::countr_zero(pos.king);
    make_masks(index);
    make_ring(index);
}
template MoveGenerator<true>::MoveGenerator(Gamestate* gs, MoveList* ml);
template MoveGenerator<false>::MoveGenerator(Gamestate* gs, MoveList* ml);

#define passant_variables int file = e_index % 8; int rank = e_index / 8; int diagonal_up = 7 - file + rank; int diagonal_down = (7 - rank) + (7 - file);
#define passant_diagonal_pin Bitboard mask = Tables::get_attack_bishop(pos.occupied, e_index); bool pin1 = mask & (Tables::bishop_ne[diagonal_up] | Tables::bishop_sw[diagonal_up]) & (pos.e_queen | pos.e_bishop) && mask & (Tables::bishop_ne[diagonal_up] | Tables::bishop_sw[diagonal_up]) & pos.king; bool pin2 = mask & (Tables::bishop_nw[diagonal_down] | Tables::bishop_se[diagonal_down]) & (pos.e_queen | pos.e_bishop) && mask & (Tables::bishop_nw[diagonal_down] | Tables::bishop_se[diagonal_down]) & pos.king;
#define passant_straight_pin mask = Tables::get_attack_rook(pos.occupied & ~(exclude), e_index); bool pin3 = mask & (Tables::rook_left[file] | Tables::rook_right[file]) & (pos.e_queen | pos.e_rook) && mask & (Tables::rook_left[file] | Tables::rook_right[file]) & pos.king;
#define pawn_isolator_normal(moved) while (moves) { ml->list[ml->index_back] = (std::countr_zero(moves) + moved) << 10 | (std::countr_zero(moves) << 4); ml->index_back--; moves &= moves - 1; }
#define pawn_isolator_capture(moved) while (moves) { ml->list[ml->index_front] = (std::countr_zero(moves) + moved) << 10 | (std::countr_zero(moves) << 4) | 4; ml->index_front++; moves &= moves - 1; }
#define partial_promotion_macro(piece, moved, capture) ml->list[ml->index_front] = (std::countr_zero(moves) + moved) << 10 | (std::countr_zero(moves) << 4) | capture << 3 | piece; ml->index_front++;
#define promotion_macro(moved, capture) partial_promotion_macro(3, moved, capture) partial_promotion_macro(6, moved, capture) partial_promotion_macro(5, moved, capture) partial_promotion_macro(7, moved, capture)
#define pawn_isolator_promo(moved, capture) while (moves) { promotion_macro(moved, capture) moves &= moves - 1; }
#define passant_isolator(moved) ml->list[ml->index_front] = (std::countr_zero(gs->enPassant) + moved) << 10 | (std::countr_zero(gs->enPassant) << 4) | 1; ml->index_front++;
#define castles_isolator(flag) ml->list[ml->index_back] = flag | 0b10; ml->index_back--;

template<> template<>
void MoveGenerator<true>::get_moves_pawn<false>() {
    Bitboard pawns_not_pinned = pos.pawn & ~(pos.pin_mask_diagonal | pos.pin_mask_straight);
    Bitboard pawns_pinned_diagonally = pos.pawn & pos.pin_mask_diagonal;
    Bitboard forward = ((pawns_not_pinned << 8) | (((pos.pawn & pos.pin_mask_straight) << 8) & pos.pin_mask_straight)) & pos.empty;
    Bitboard moves = forward & ~eighth;             pawn_isolator_normal(-8)
    moves = ((forward & third) << 8) & pos.empty;   pawn_isolator_normal(-16)
    moves = forward & eighth;                       pawn_isolator_promo(-8, 0)

    Bitboard r = (((pawns_not_pinned & ~Tables::northeast) << 7) | (((pawns_pinned_diagonally & ~Tables::northeast) << 7) & pos.pin_mask_diagonal));
    moves = (r & pos.enemy) & ~eighth;    pawn_isolator_capture(-7)
    moves = (r & pos.enemy) & eighth;     pawn_isolator_promo(-7, 1)
    Bitboard l = (((pawns_not_pinned & ~Tables::northwest) << 9) | (((pawns_pinned_diagonally & ~Tables::northwest) << 9) & pos.pin_mask_diagonal));
    moves = (l & pos.enemy) & ~eighth;    pawn_isolator_capture(-9)
    moves = (l & pos.enemy) & eighth;     pawn_isolator_promo(-9, 1)

    if ((r | l) & gs->enPassant) {
        int e_index = std::countr_zero(gs->enPassant >> 8);
        passant_variables
                passant_diagonal_pin
        if (!pin1 && !pin2) {
            if (r & gs->enPassant && l & gs->enPassant) { passant_isolator(-7) passant_isolator(-9) }
            else {
                Bitboard exclude = gs->enPassant >> ((r & gs->enPassant) ? 7 : 9);
                passant_straight_pin
                if (!pin3) { passant_isolator(((r & gs->enPassant) ? -7 : -9)) }
            }
        }
    }

    // castles
    if (gs->castlingRights[wk] && !((pos.occupied | pos.check_ring) & wkOcc) && !attacked(1)) { castles_isolator(wkflag) }
    if (gs->castlingRights[wq] && !((pos.occupied | pos.check_ring) & wqOcc) && !attacked(5)) { castles_isolator(wqflag) }
}
template<> template<>
void MoveGenerator<false>::get_moves_pawn<false>() {
    Bitboard pawns_not_pinned = pos.pawn & ~(pos.pin_mask_diagonal | pos.pin_mask_straight);
    Bitboard pawns_pinned_diagonally = pos.pawn & pos.pin_mask_diagonal;
    Bitboard forward = ((pawns_not_pinned >> 8) | (((pos.pawn & pos.pin_mask_straight) >> 8) & pos.pin_mask_straight)) & pos.empty;
    Bitboard moves = forward & ~first;              pawn_isolator_normal(8)
    moves = ((forward & sixth) >> 8) & pos.empty;   pawn_isolator_normal(16)
    moves = forward & first;                        pawn_isolator_promo(8, 0)

    Bitboard r = (((pawns_not_pinned & ~Tables::southwest) >> 7) | (((pawns_pinned_diagonally & ~Tables::southwest) >> 7) & pos.pin_mask_diagonal));
    moves = (r & pos.enemy) & ~first;    pawn_isolator_capture(7)
    moves = (r & pos.enemy) & first;     pawn_isolator_promo(7, 1)
    Bitboard l = (((pawns_not_pinned & ~Tables::southeast) >> 9) | (((pawns_pinned_diagonally & ~Tables::southeast) >> 9) & pos.pin_mask_diagonal));
    moves = (l & pos.enemy) & ~first;    pawn_isolator_capture(9)
    moves = (l & pos.enemy) & first;     pawn_isolator_promo(9, 1)

    if ((r | l) & gs->enPassant) {
        int e_index = std::countr_zero(gs->enPassant << 8);
        passant_variables
                passant_diagonal_pin
        if (!pin1 && !pin2) {
            if (r & gs->enPassant && l & gs->enPassant) { passant_isolator(7) passant_isolator(9) }
            else {
                Bitboard exclude = gs->enPassant << ((r & gs->enPassant) ? 7 : 9);
                passant_straight_pin
                if (!pin3) { passant_isolator(((r & gs->enPassant) ? 7 : 9)) }
            }
        }
    }

    // castles
    if (gs->castlingRights[bk] && !((pos.occupied | pos.check_ring) & bkOcc) && !attacked(57)) { castles_isolator(bkflag) }
    if (gs->castlingRights[bq] && !((pos.occupied | pos.check_ring) & bqOcc) && !attacked(61)) { castles_isolator(bqflag) }
}
template<> template<>
void MoveGenerator<true>::get_moves_pawn<true>(){
    Bitboard pawns_not_pinned = pos.pawn & ~(pos.pin_mask_diagonal | pos.pin_mask_straight);
    Bitboard forward = (pawns_not_pinned << 8) & pos.empty;
    Bitboard moves = forward & ~eighth & pos.check_mask;             pawn_isolator_normal(-8)
    moves = ((forward & third) << 8) & pos.empty & pos.check_mask;   pawn_isolator_normal(-16)
    moves = forward & eighth & pos.check_mask;                       pawn_isolator_promo(-8, 0)

    Bitboard r = ((pawns_not_pinned & ~Tables::northeast) << 7) & pos.check_mask;
    moves = (r & pos.enemy) & ~eighth;    pawn_isolator_capture(-7)
    moves = (r & pos.enemy) & eighth;     pawn_isolator_promo(-7, 1)
    Bitboard l = ((pawns_not_pinned & ~Tables::northwest) << 9) & pos.check_mask;
    moves = (l & pos.enemy) & ~eighth;    pawn_isolator_capture(-9)
    moves = (l & pos.enemy) & eighth;     pawn_isolator_promo(-9, 1)

    r |= ((pawns_not_pinned & ~Tables::northeast) << 7) & (pos.check_mask << 8);
    l |= ((pawns_not_pinned & ~Tables::northwest) << 9) & (pos.check_mask << 8);
    if ((r | l) & gs->enPassant) {
        int e_index = std::countr_zero(gs->enPassant >> 8);
        passant_variables
                passant_diagonal_pin
        if (!pin1 && !pin2) {
            if (r & gs->enPassant && l & gs->enPassant) { passant_isolator(-7) passant_isolator(-9) }
            else {
                Bitboard exclude = gs->enPassant >> ((r & gs->enPassant) ? 7 : 9);
                passant_straight_pin
                if (!pin3) { passant_isolator(((r & gs->enPassant) ? -7 : -9)) }
            }
        }
    }
}
template<> template<>
void MoveGenerator<false>::get_moves_pawn<true>() {
    Bitboard pawns_not_pinned = pos.pawn & ~(pos.pin_mask_diagonal | pos.pin_mask_straight);
    Bitboard forward = (pawns_not_pinned >> 8) & pos.empty;
    Bitboard moves = forward & ~first & pos.check_mask;              pawn_isolator_normal(8)
    moves = ((forward & sixth) >> 8) & pos.empty & pos.check_mask;   pawn_isolator_normal(16)
    moves = forward & first & pos.check_mask;                        pawn_isolator_promo(8, 0)

    Bitboard r = ((pawns_not_pinned & ~Tables::southwest) >> 7) & pos.check_mask;
    moves = (r & pos.enemy) & ~first;    pawn_isolator_capture(7)
    moves = (r & pos.enemy) & first;     pawn_isolator_promo(7, 1)
    Bitboard l = ((pawns_not_pinned & ~Tables::southeast) >> 9) & pos.check_mask;
    moves = (l & pos.enemy) & ~first;    pawn_isolator_capture(9)
    moves = (l & pos.enemy) & first;     pawn_isolator_promo(9, 1)

    r |= ((pawns_not_pinned & ~Tables::southwest) >> 7) & (pos.check_mask >> 8);
    l |= ((pawns_not_pinned & ~Tables::southeast) >> 9) & (pos.check_mask >> 8);
    if ((r | l) & gs->enPassant) {
        int e_index = std::countr_zero(gs->enPassant << 8);
        passant_variables
                passant_diagonal_pin
        if (!pin1 && !pin2) {
            if (r & gs->enPassant && l & gs->enPassant) { passant_isolator(7) passant_isolator(9) }
            else {
                Bitboard exclude = gs->enPassant << ((r & gs->enPassant) ? 7 : 9);
                passant_straight_pin
                if (!pin3) { passant_isolator(((r & gs->enPassant) ? 7 : 9)) }
            }
        }
    }
}

template<>
Bitboard MoveGenerator<true>::get_pawn_attacks(Bitboard origin) { return ((origin & ~Tables::northeast) << 7) | ((origin & ~Tables::northwest) << 9); }
template<>
Bitboard MoveGenerator<false>::get_pawn_attacks(Bitboard origin) { return ((origin & ~Tables::southwest) >> 7) | ((origin & ~Tables::southeast) >> 9); }


template <bool white>
void MoveGenerator<white>::go(){
#define isolate_normal(moves) while (moves) {ml->list[ml->index_back] = index << 10 | (std::countr_zero(moves) << 4); ml->index_back--;moves &= moves - 1;}
#define isolate_capture(moves) while (moves_capture) {ml->list[ml->index_front] = index << 10 | (std::countr_zero(moves_capture) << 4) | 4; ml->index_front++;moves_capture &= moves_capture - 1;}
#define get_moves_rook(pieces, relevant_mask) relevant_pieces = pieces; while (relevant_pieces){ int index = std::countr_zero(relevant_pieces);Bitboard legal = Tables::get_attack_rook(pos.occupied, index) & relevant_mask;Bitboard moves_capture = legal & pos.enemy; legal &= pos.empty;isolate_normal(legal) isolate_capture(moves_capture) relevant_pieces &= relevant_pieces - 1;}
#define get_moves_bishop(pieces, relevant_mask) relevant_pieces = pieces; while (relevant_pieces){ int index = std::countr_zero(relevant_pieces);Bitboard legal = Tables::get_attack_bishop(pos.occupied, index) & relevant_mask;Bitboard moves_capture = legal & pos.enemy; legal &= pos.empty;isolate_normal(legal) isolate_capture(moves_capture) relevant_pieces &= relevant_pieces - 1;}
#define get_moves_knight(relevant_mask) relevant_pieces = pos.knight & not_pinned; while (relevant_pieces){ int index = std::countr_zero(relevant_pieces);Bitboard legal = Tables::knight_attack_table[index] & relevant_mask;Bitboard moves_capture = legal & pos.enemy; legal &= pos.empty;isolate_normal(legal) isolate_capture(moves_capture) relevant_pieces &= relevant_pieces - 1;}
#define get_moves_king int index = std::countr_zero(pos.king); Bitboard legal = Tables::king_attack_table[index] & ~pos.check_ring; Bitboard moves_capture = legal & pos.enemy; legal &= pos.empty; isolate_normal(legal) isolate_capture(moves_capture)

    if (!pos.check_mask){ // no check

        Bitboard not_pinned = ~(pos.pin_mask_straight | pos.pin_mask_diagonal);
        Bitboard relevant_pieces;
        // rook
        get_moves_rook(pos.rook & not_pinned, universe)
        get_moves_rook(pos.rook & pos.pin_mask_straight, pos.pin_mask_straight)
        // bishop
        get_moves_bishop(pos.bishop & not_pinned, universe)
        get_moves_bishop(pos.bishop & pos.pin_mask_diagonal, pos.pin_mask_diagonal)
        // queen
        get_moves_rook(pos.queen & not_pinned, universe)
        get_moves_bishop(pos.queen & not_pinned, universe)
        get_moves_rook(pos.queen & pos.pin_mask_straight, pos.pin_mask_straight)
        get_moves_bishop(pos.queen & pos.pin_mask_diagonal, pos.pin_mask_diagonal)
        // knight
        get_moves_knight(universe)
        // king
        get_moves_king

        // pawn
        get_moves_pawn<false>();

        if (!ml->index_front && ml->index_front == 217) ml->list[0] = stalemate;
    }
    else if (pos.check_mask == universe){ // double check
        get_moves_king
        if (!ml->index_front && ml->index_front == 217) ml->list[0] = checkmate;
    }
    else{ // normal check

        Bitboard not_pinned = ~(pos.pin_mask_straight | pos.pin_mask_diagonal);
        Bitboard relevant_pieces;
        // rook
        get_moves_rook(pos.rook & not_pinned, pos.check_mask)
        // bishop
        get_moves_bishop(pos.bishop & not_pinned, pos.check_mask)
        // queen
        get_moves_rook(pos.queen & not_pinned, pos.check_mask)
        get_moves_bishop(pos.queen & not_pinned, pos.check_mask)
        // knight
        get_moves_knight(pos.check_mask)
        // king
        get_moves_king

        // pawn
        get_moves_pawn<true>();
        if (!ml->index_front && ml->index_front == 217) ml->list[0] = checkmate;
    }

}
template void MoveGenerator<true>::go();
template void MoveGenerator<false>::go();

template <bool white>
bool MoveGenerator<white>::attacked(int index) {
    Bitboard relevant = pos.e_queen | pos.e_rook;
    Bitboard mask = Tables::get_attack_rook(pos.occupied, index);
    Bitboard attacker = mask & relevant;
    if (attacker) return true;

    relevant = pos.e_queen | pos.e_bishop;
    mask = Tables::get_attack_bishop(pos.occupied, index);
    attacker = mask & relevant;
    if (attacker) return true;

    attacker = Tables::knight_attack_table[index] & pos.e_knight;
    if (attacker) return true;
    attacker = Tables::king_attack_table[index] & pos.e_king;
    if (attacker) return true;
    attacker = get_pawn_attacks(1ull << index) & pos.e_pawn;
    if (attacker) return true;

    return false;
}
template bool MoveGenerator<true>::attacked(int index);
template bool MoveGenerator<false>::attacked(int index);

template <bool white>
void MoveGenerator<white>::make_ring(int index){
    Bitboard ring = Tables::king_attack_table[index];
    ring = ring & ~pos.own;
    if (pos.check_mask != universe) { pos.check_ring = ring & pos.check_mask & ~pos.enemy; ring &= ~(pos.check_mask & ~pos.enemy); }
    Bitboard occ_no_king = pos.occupied & ~pos.king;

    if (ring) {
        Bitboard pawn_attackers = get_pawn_attacks(ring) & pos.e_pawn;
        if (!white)  pos.check_ring |= ((pawn_attackers & ~Tables::northeast) << 7) | ((pawn_attackers & ~Tables::northwest) << 9);
        else         pos.check_ring |= ((pawn_attackers & ~Tables::southwest) >> 7) | ((pawn_attackers & ~Tables::southeast) >> 9);
    }
    else return;

    ring &= ~pos.check_ring;

#define change_ring pos.check_ring |= 1ull << curr_index; ring &= ring - 1; continue;

    while (ring){
        int curr_index = std::countr_zero(ring);

        Bitboard attackers = Tables::knight_attack_table[curr_index] & pos.e_knight;
        if (attackers) { change_ring }

        attackers = Tables::king_attack_table[curr_index] & pos.e_king;
        if (attackers) { change_ring }

        attackers = Tables::get_attack_rook(occ_no_king, curr_index) & (pos.e_queen | pos.e_rook);
        if (attackers) { change_ring }

        attackers = Tables::get_attack_bishop(occ_no_king, curr_index) & (pos.e_queen | pos.e_bishop);
        if (attackers) { change_ring }

        ring &= ring - 1;
    }
}
template <bool white>
void MoveGenerator<white>::make_masks(int index) {
    pos.check_mask = 0;
    bool logged_check = false;

    Bitboard relevant = pos.e_queen | pos.e_rook;
    Bitboard mask = Tables::get_attack_rook(pos.occupied, index);
    Bitboard attacker = mask & relevant;
    Bitboard pinned = mask & pos.own;
    if (pinned) rook_pin_routine(mask, index, attacker);
    if (attacker) { // check by queen or rook
        rook_check_routine(attacker, mask, index, &logged_check);
        if (pos.check_mask == universe) return;
    }

    relevant = pos.e_queen | pos.e_bishop;
    mask = Tables::get_attack_bishop(pos.occupied, index);
    attacker = mask & relevant;
    pinned = mask & pos.own;
    if (pinned) bishop_pin_routine(mask, index, attacker);
    if (attacker) { // check by queen or bishop
        bishop_check_routine(attacker, mask, index);
        if(logged_check) {pos.check_mask = universe; return; }
        logged_check = true;
    }

    attacker = Tables::knight_attack_table[index] & pos.e_knight;
    if (attacker){ // check by knight
        pos.check_mask |= attacker;
        if (logged_check) {pos.check_mask = universe; return;}
        logged_check = true;
    }
    attacker = get_pawn_attacks(pos.king) & pos.e_pawn;
    if (attacker){ // check by pawn
        pos.check_mask |= attacker;
        if (logged_check) {pos.check_mask = universe; return;}
    }
}

template <bool white>
void MoveGenerator<white>::rook_check_routine(Bitboard attacker, Bitboard mask, int index, bool* logged_check) {
    int file = index % 8;
    int rank = index / 8;
    if (Tables::rook_right[file] & attacker) {pos.check_mask |= mask & Tables::rook_right[file]; *logged_check = true;}
    if (Tables::rook_left[file] & attacker)  {pos.check_mask |= mask & Tables::rook_left[file]; if (*logged_check) {pos.check_mask = universe; return;} *logged_check = true;}
    if (Tables::rook_down[rank] & attacker)  {pos.check_mask |= mask & Tables::rook_down[rank]; if (*logged_check) {pos.check_mask = universe; return;} *logged_check = true;}
    if (Tables::rook_up[rank] & attacker)    {pos.check_mask |= mask & Tables::rook_up[rank]; if (*logged_check) {pos.check_mask = universe; return;} *logged_check = true;}
}
template <bool white>
void MoveGenerator<white>::rook_pin_routine(Bitboard mask, int index, Bitboard old_attacker){
    pos.pin_mask_straight = 0;
    Bitboard removed_mask = Tables::get_attack_rook(pos.occupied ^ (pos.own & mask), index);
    Bitboard attackers = removed_mask & (pos.e_rook | pos.e_queen) & ~old_attacker;
    if (attackers){
        int file = index % 8;
        int rank = index / 8;
        if (Tables::rook_right[file] & attackers)   pos.pin_mask_straight |= removed_mask & Tables::rook_right[file];
        if (Tables::rook_left[file] & attackers)    pos.pin_mask_straight |= removed_mask & Tables::rook_left[file];
        if (Tables::rook_down[rank] & attackers)    pos.pin_mask_straight |= removed_mask & Tables::rook_down[rank];
        if (Tables::rook_up[rank] & attackers)      pos.pin_mask_straight |= removed_mask & Tables::rook_up[rank];
    }
}
template <bool white>
void MoveGenerator<white>::bishop_check_routine(Bitboard attacker, Bitboard mask, int index) {
    int file = index % 8;
    int rank = index / 8;
    int diagonal_up = 7 - file + rank;
    int diagonal_down = (7 - rank) + (7 - file);
    if      (Tables::bishop_nw[diagonal_down] & attacker)   pos.check_mask |= Tables::bishop_nw[diagonal_down] & mask;
    else if (Tables::bishop_ne[diagonal_up] & attacker)     pos.check_mask |= Tables::bishop_ne[diagonal_up] & mask;
    else if (Tables::bishop_se[diagonal_down] & attacker)   pos.check_mask |= Tables::bishop_se[diagonal_down] & mask;
    else                                                    pos.check_mask |= Tables::bishop_sw[diagonal_up] & mask;
}
template <bool white>
void MoveGenerator<white>::bishop_pin_routine(Bitboard mask, int index, Bitboard old_attacker){
    pos.pin_mask_diagonal = 0;
    Bitboard removed_mask = Tables::get_attack_bishop(pos.occupied ^ (pos.own & mask), index);
    Bitboard attackers = removed_mask & (pos.e_bishop | pos.e_queen) & ~old_attacker;
    if (attackers){
        int file = index % 8;
        int rank = index / 8;
        int diagonal_up = 7 - file + rank;
        int diagonal_down = (7 - rank) + (7 - file);
        if (Tables::bishop_nw[diagonal_down] & attackers)  pos.pin_mask_diagonal |= removed_mask & Tables::bishop_nw[diagonal_down];
        if (Tables::bishop_ne[diagonal_up] & attackers)    pos.pin_mask_diagonal |= removed_mask & Tables::bishop_ne[diagonal_up];
        if (Tables::bishop_se[diagonal_down] & attackers)  pos.pin_mask_diagonal |= removed_mask & Tables::bishop_se[diagonal_down];
        if (Tables::bishop_sw[diagonal_up] & attackers)    pos.pin_mask_diagonal |= removed_mask & Tables::bishop_sw[diagonal_up];
    }
}