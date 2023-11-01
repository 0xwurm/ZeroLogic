#pragma once
#include "gamestate.h"
#include "variables.h"
#include "tables.h"
#include "misc.h"
#include "eval.h"
#include <intrin.h>
#include <chrono>

#define SquareOf(X) _tzcnt_u64(X)
#define Bitloop(X) for(;X; X = _blsr_u64(X))

namespace ZeroLogic::Movegen{
    using namespace Boardstate;
    static unsigned long long partial_nodecount = 0;
    static unsigned long long overall_nodecount = 0;

    template <bool white>
    FORCEINLINE map pawn_atk_right(const map& pieces){
        if constexpr (white)    return (pieces & NOT_BORDER_EAST) << 7;
        else                    return (pieces & NOT_BORDER_WEST) >> 7;
    }
    template <bool white>
    FORCEINLINE map pawn_atk_left(const map& pieces){
        if constexpr (white)    return (pieces & NOT_BORDER_WEST) << 9;
        else                    return (pieces & NOT_BORDER_EAST) >> 9;
    }

    template <bool white>
    FORCEINLINE map pawn_forward(const map& pieces){
        if constexpr (white)    return pieces << 8;
        else                    return pieces >> 8;
    }

    template <bool white>
    COMPILETIME map last_rank(){
        if constexpr (white)    return EIGHTH_RANK;
        else                    return FIRST_RANK;
    }
    template <bool white>
    COMPILETIME map not_last_rank(){
        if constexpr (white)    return ~EIGHTH_RANK;
        else                    return ~FIRST_RANK;
    }
    template <bool white>
    COMPILETIME map third_rank(){
        if constexpr (white)    return THIRD_RANK;
        else                    return SIXTH_RANK;
    }

    template <bool white>
    FORCEINLINE void pin(Square king, Square pinner, map& pinmask, const Board& board){
        const map path = Lookup::pin_between[64 * king + pinner];
        if (path & ColorPieces<white>(board))
            pinmask |= path;
    }

    FORCEINLINE void slider_check(Square king, Square attacker, map& kingban, map& checkmask){
        if (checkmask == full){
            checkmask = Lookup::pin_between[64 * king + attacker];
        }
        else checkmask = 0;
        kingban |= Lookup::check_between[64 * king + attacker];
    }

    template <bool white>
    COMPILETIME map EPRank(){
        if constexpr (white) return 0xffull << 32;
        else                 return 0xffull << 24;
    }

    template <State state>
    FORCEINLINE void ep_pin(Square king_square, const Bit king, map enemyRQ, const Board& board, Bit& ep_target){
        constexpr bool us = state.white_move;
        const map pawns = Pawns<us>(board);

        if ((EPRank<us>() & king) && (EPRank<us>() & enemyRQ) && (EPRank<us>() & pawns)){
            Bit lep = pawns & (ep_target & ~H_FILE) >> 1;
            Bit rep = pawns & (ep_target & ~A_FILE) << 1;

            if (lep){
                map ep_occ = board.Occ & ~(ep_target | lep);
                if ((Lookup::r_atk(king_square, ep_occ) & EPRank<us>()) & enemyRQ) ep_target = 0;
            }
            if (rep){
                map ep_occ = board.Occ & ~(ep_target | rep);
                if ((Lookup::r_atk(king_square, ep_occ) & EPRank<us>()) & enemyRQ) ep_target = 0;
            }
        }
    }

    template <State state>
    FORCEINLINE void make_masks(const Board& board, const map king, map& checkmask, map& kingban, map& pin_rook, map& pin_bishop, map& kingmoves, map& ep_target){
        checkmask = full;
        constexpr bool us = state.white_move;
        constexpr bool enemy = !us;
        const Square king_square = SquareOf(king);

        {
            // checks by knights
            {
                map check_by_knight = Lookup::knight[king_square] & Knights<enemy>(board);
                if (check_by_knight) checkmask = check_by_knight;
            }

            // checks & attacks by pawns
            {
                const map pr = pawn_atk_right<enemy>(Pawns<enemy>(board));
                const map pl = pawn_atk_left<enemy>(Pawns<enemy>(board));
                kingban |= pr | pl;
                if (pr & king) checkmask = pawn_atk_right<us>(king);
                else if (pl & king) checkmask = pawn_atk_left<us>(king);
            }

            // checks & pins by sliders
            if (Lookup::r_atk(king_square, 0) & RookOrQueen<enemy>(board)){
                map atk = Lookup::r_atk(king_square, board.Occ) & RookOrQueen<enemy>(board);
                Bitloop(atk)
                    slider_check(king_square, SquareOf(atk), kingban, checkmask);

                map pinning = Lookup::r_xray(king_square, board.Occ) & RookOrQueen<enemy>(board);
                Bitloop(pinning)
                    pin<us>(king_square, SquareOf(pinning), pin_rook, board);
            }
            if (Lookup::b_atk(king_square, 0) & BishopOrQueen<enemy>(board)){
                map atk = Lookup::b_atk(king_square, board.Occ) & BishopOrQueen<enemy>(board);
                Bitloop(atk)
                    slider_check(king_square, SquareOf(atk), kingban, checkmask);

                map pinning = Lookup::b_xray(king_square, board.Occ) & BishopOrQueen<enemy>(board);
                Bitloop(pinning)
                    pin<us>(king_square, SquareOf(pinning), pin_bishop, board);
            }

            if constexpr (state.has_ep_pawn) ep_pin<state>(king_square, king, RookOrQueen<enemy>(board), board, ep_target);
        }

        kingmoves = Lookup::king[king_square] & EmptyOrEnemy<us>(board) & ~kingban;
        if (kingmoves){

            // attacks by knights
            {
                map knights = Knights<enemy>(board);
                Bitloop(knights)
                    kingban |= Lookup::knight[SquareOf(knights)];
            }

            // attacks by rooks and queens
            {
                map rq = RookOrQueen<enemy>(board);
                Bitloop(rq)
                    kingban |= Lookup::r_atk(SquareOf(rq), board.Occ);
            }
            // attacks by bishops and queens
            {
                map bq = BishopOrQueen<enemy>(board);
                Bitloop(bq)
                    kingban |= Lookup::b_atk(SquareOf(bq), board.Occ);
            }

            // attacks by king
            kingban |= Lookup::king[SquareOf(King<enemy>(board))];

            kingmoves &= ~kingban;

        }

    }

    template <State state, bool root>
    void enumerate(const Board& board, int depth, Bit ep_target);

    template <State state, Piece piece, bool capture, Piece promotion_to, bool root>
    FORCEINLINE void continue_perft(const Board& board, Bit from, Bit to, int depth) { // for normal moves + promotions
        const Board new_board = move<piece, promotion_to, state.white_move, capture>(board, from, to);

        [&]()->void{
            if constexpr (state.enemy_can_castle() && capture) {
                if constexpr (state.we_can_castle() && piece == ROOK) {
                    if (state.is_own_rook_l(from)) {
                        if (state.is_enemy_rook_l(to)) {
                            constexpr State new_state = state.no_ooo_all();
                            enumerate<new_state, false>(new_board, depth - 1, 0);
                            return;
                        }
                        constexpr State new_state = state.no_ooo_own();
                        enumerate<new_state, false>(new_board, depth - 1, 0);
                        return;
                    } else if (state.is_own_rook_r(from)) {
                        if (state.is_enemy_rook_r(to)) {
                            constexpr State new_state = state.no_oo_all();
                            enumerate<new_state, false>(new_board, depth - 1, 0);
                            return;
                        }
                        constexpr State new_state = state.no_oo_own();
                        enumerate<new_state, false>(new_board, depth - 1, 0);
                        return;
                    }
                }
                else if (state.is_enemy_rook_r(to)) {
                    constexpr State new_state = state.no_oo_enemy();
                    enumerate<new_state, false>(new_board, depth - 1, 0);
                    return;
                }
                else if (state.is_enemy_rook_l(to)) {
                    constexpr State new_state = state.no_ooo_enemy();
                    enumerate<new_state, false>(new_board, depth - 1, 0);
                    return;
                }
            }
            if constexpr (state.we_can_castle()) {
                if constexpr (piece == KING) {
                    constexpr State new_state = state.no_castles_own();
                    enumerate<new_state, false>(new_board, depth - 1, 0);
                    return;
                }
                else if constexpr (piece == ROOK) {
                    if (state.is_own_rook_l(from)) {
                        constexpr State new_state = state.no_ooo_own();
                        enumerate<new_state, false>(new_board, depth - 1, 0);
                        return;
                    }
                    else if (state.is_own_rook_r(from)) {
                        constexpr State new_state = state.no_oo_own();
                        enumerate<new_state, false>(new_board, depth - 1, 0);
                        return;
                    }
                }
            }
            constexpr State new_state = state.silent_move();
            enumerate<new_state, false>(new_board, depth - 1, 0);
        }();

        if constexpr (root) {
            std::cout << Misc::uci_move<promotion_to>(SquareOf(from), SquareOf(to)) << ": " << partial_nodecount << std::endl;
            overall_nodecount += partial_nodecount;
            partial_nodecount = 0;
        }
    }

    template <State state, CastleType type, bool root>
    FORCEINLINE void continue_perft(const Board& board, int depth){ // for castles
        const Board new_board = move<type>(board);
        constexpr State new_state = change_state<state, true>();
        enumerate<new_state, false>(new_board, depth - 1, 0);
        if constexpr (root) {
            std::cout << Misc::uci_move<type>() << ": " << partial_nodecount << std::endl;
            overall_nodecount += partial_nodecount;
            partial_nodecount = 0;
        }
    }

    template <State state, bool root, bool ep>
    FORCEINLINE void continue_perft(const Board& board, Bit from, Bit to, Bit ep_target, int depth){ // for double pawn push and ep
        if constexpr (ep){
            const Board new_board = move<state.white_move>(board, from | to, ep_target);
            constexpr State new_state = state.silent_move();
            enumerate<new_state, false>(new_board, depth - 1, 0);
        }
        else {
            const Board new_board = move<PAWN, PIECE_INVALID, state.white_move, false>(board, from, to);
            constexpr State new_state = change_state<state, false>();
            enumerate<new_state, false>(new_board, depth - 1, to);
        }
        if constexpr (root) {
            std::cout << Misc::uci_move<PIECE_INVALID>(SquareOf(from), SquareOf(to)) << ": " << partial_nodecount << std::endl;
            overall_nodecount += partial_nodecount;
            partial_nodecount = 0;
        }
    }

#define both_loops(piece)\
    Bitloop(moves)  \
    continue_perft<state, piece, false, PIECE_INVALID, root>(board, 1ull << sq, 1ull << SquareOf(moves), depth);  \
    Bitloop(captures)   \
    continue_perft<state, piece, true, PIECE_INVALID, root>(board, 1ull << sq, 1ull << SquareOf(captures), depth);

    template <State state, bool root>
    FORCEINLINE void pawn_loops(const Board& board, int depth, map pr, map pl, map pr_promo, map pl_promo, map pf, map pf_promo, map pp){
        constexpr bool us = state.white_move;
        Bitloop(pr){
            Square sq = SquareOf(pr);
            continue_perft<state, PAWN, true, PIECE_INVALID, root>(board, 1ull << (sq + sign<us>(-7)), 1ull << sq, depth);
        }
        Bitloop(pl){
            Square sq = SquareOf(pl);
            continue_perft<state, PAWN, true, PIECE_INVALID, root>(board, 1ull << (sq + sign<us>(-9)), 1ull << sq, depth);
        }
        Bitloop(pr_promo){
            Square sq = SquareOf(pr_promo); Bit from = 1ull << (sq + sign<us>(-7)), to = 1ull << sq;
            continue_perft<state, PAWN, true, QUEEN, root>(board, from, to, depth);
            continue_perft<state, PAWN, true, ROOK, root>(board, from, to, depth);
            continue_perft<state, PAWN, true, BISHOP, root>(board, from, to, depth);
            continue_perft<state, PAWN, true, KNIGHT, root>(board, from, to, depth);
        }
        Bitloop(pl_promo){
            Square sq = SquareOf(pl_promo); Bit from = 1ull << (sq + sign<us>(-9)), to = 1ull << sq;
            continue_perft<state, PAWN, true, QUEEN, root>(board, from, to, depth);
            continue_perft<state, PAWN, true, ROOK, root>(board, from, to, depth);
            continue_perft<state, PAWN, true, BISHOP, root>(board, from, to, depth);
            continue_perft<state, PAWN, true, KNIGHT, root>(board, from, to, depth);
        }
        Bitloop(pf){
            Square sq = SquareOf(pf);
            continue_perft<state, PAWN, false, PIECE_INVALID, root>(board, 1ull << (sq + sign<us>(-8)), 1ull << sq, depth);
        }
        Bitloop(pf_promo){
            Square sq = SquareOf(pf_promo); Bit from = 1ull << (sq + sign<us>(-8)), to = 1ull << sq;
            continue_perft<state, PAWN, false, QUEEN, root>(board, from, to, depth);
            continue_perft<state, PAWN, false, ROOK, root>(board, from, to, depth);
            continue_perft<state, PAWN, false, BISHOP, root>(board, from, to, depth);
            continue_perft<state, PAWN, false, KNIGHT, root>(board, from, to, depth);
        }
        Bitloop(pp){
            Square sq = SquareOf(pp);
            continue_perft<state, root, false>(board, 1ull << (sq + sign<us>(-16)), 1ull << sq, 0, depth);
        }
    }

    template <State state, bool root>
    FORCEINLINE void king_moves(const Board& board, int depth, map& kingmoves){
        constexpr bool us = state.white_move;
        constexpr bool enemy = !us;
        map king_captures = kingmoves & ColorPieces<enemy>(board);
        kingmoves &= ~board.Occ;
        Bitloop(kingmoves)
            continue_perft<state, KING, false, PIECE_INVALID, root>(board, King<us>(board), 1ull << SquareOf(kingmoves), depth);
        Bitloop(king_captures)
            continue_perft<state, KING, true, PIECE_INVALID, root>(board, King<us>(board), 1ull << SquareOf(king_captures), depth);
    }

    // enumerator for check-positions
    template <State state, bool root, bool count>
    FORCEINLINE void _enumerate(const Board& board, const map& checkmask, const map& pin_rook, const map& pin_bishop, int depth, const map& ep_target){
        constexpr bool us = state.white_move;
        constexpr bool enemy = !us;
        const map no_pin = ~(pin_rook | pin_bishop);

        // pawns
        {
            const map not_pinned = Pawns<us>(board) & no_pin;

            map pr = pawn_atk_right<us>(not_pinned) & ColorPieces<enemy>(board) & checkmask;
            map pl = pawn_atk_left<us>(not_pinned) & ColorPieces<enemy>(board) & checkmask;
            map pr_promo = pr & last_rank<us>() & checkmask; pr &= not_last_rank<us>();
            map pl_promo = pl & last_rank<us>() & checkmask; pl &= not_last_rank<us>();
            map pf = pawn_forward<us>(not_pinned) & ~board.Occ;
            map pf_promo = pf & last_rank<us>() & checkmask; pf &= not_last_rank<us>();
            map pp = pawn_forward<us>(pf & third_rank<us>()) & ~board.Occ & checkmask; pf &= checkmask;

            if constexpr (state.has_ep_pawn){
                if (ep_target){
                    Bit lep = not_pinned & ~A_FILE & ((ep_target & checkmask) >> 1);
                    Bit rep = not_pinned & ~H_FILE & ((ep_target & checkmask) << 1);

                    if constexpr (count) {if (lep) partial_nodecount++; if (rep) partial_nodecount++;}
                    else {
                        if constexpr (state.white_move) {
                            if (lep) continue_perft<state, root, true>(board, lep, pawn_atk_left<us>(lep), ep_target, depth);
                            if (rep) continue_perft<state, root, true>(board, rep, pawn_atk_right<us>(rep), ep_target, depth);
                        }
                        else{
                            if (lep) continue_perft<state, root, true>(board, lep, pawn_atk_right<us>(lep), ep_target, depth);
                            if (rep) continue_perft<state, root, true>(board, rep, pawn_atk_left<us>(rep), ep_target, depth);
                        }
                    }
                }
            }

            if constexpr (count){
                partial_nodecount += BitCount(pr);
                partial_nodecount += BitCount(pl);
                partial_nodecount += 4*BitCount(pr_promo);
                partial_nodecount += 4*BitCount(pl_promo);
                partial_nodecount += BitCount(pf | pp);
                partial_nodecount += 4*BitCount(pf_promo);
            }
            else pawn_loops<state, root>(board, depth, pr, pl, pr_promo, pl_promo, pf, pf_promo, pp);
        }

        // knights
        {
            map knights = Knights<us>(board) & no_pin;
            Bitloop(knights) {
                const Square sq = SquareOf(knights);
                map moves = Lookup::knight[sq] & checkmask;
                map captures = moves & ColorPieces<enemy>(board);
                moves &= ~board.Occ;
                if constexpr (count){
                    partial_nodecount += BitCount(moves | captures);
                }
                else { both_loops(KNIGHT) }
            }
        }

        const map queens = Queens<us>(board);
        // bishops & queens
        {
            map not_pinned = (queens | Bishops<us>(board)) & no_pin;

            Bitloop(not_pinned){
                const Square sq = SquareOf(not_pinned);
                map moves = Lookup::b_atk(sq, board.Occ) & checkmask;
                map captures = moves & ColorPieces<enemy>(board);
                moves &= ~board.Occ;
                if constexpr (count){
                    partial_nodecount += BitCount(moves | captures);
                }
                else {
                    if (1ull << sq & queens)    { both_loops(QUEEN) }
                    else                        { both_loops(BISHOP) }
                }
            }
        }
        // rooks & queens
        {
            map not_pinned = (queens | Rooks<us>(board)) & no_pin;

            Bitloop(not_pinned){
                const Square sq = SquareOf(not_pinned);
                map moves = Lookup::r_atk(sq, board.Occ) & checkmask;
                map captures = moves & ColorPieces<enemy>(board);
                moves &= ~board.Occ;
                if constexpr (count){
                    partial_nodecount += BitCount(moves | captures);
                }
                else {
                    if (1ull << sq & queens)    { both_loops(QUEEN) }
                    else                        { both_loops(ROOK) }
                }
            }
        }
    }

    // enumerator for non-check-positions
    template <State state, bool root, bool count>
    FORCEINLINE void _enumerate(const Board& board, const map& pin_rook, const map& pin_bishop, int depth, const map& ep_target, const map kingban){
        constexpr bool us = state.white_move;
        constexpr bool enemy = !us;
        const map no_pin = ~(pin_rook | pin_bishop);

        // castles
        if constexpr (state.we_can_castle_short()){
            if (short_legal<us>(board.Occ, kingban)) {
                if constexpr (count) partial_nodecount++;
                else {
                    if constexpr (state.white_move) continue_perft<state, WHITE_OO, root>(board, depth);
                    else                            continue_perft<state, BLACK_OO, root>(board, depth);
                }
            }
        }
        if constexpr (state.we_can_castle_long()){
            if (long_legal<us>(board.Occ, kingban)) {
                if constexpr (count) partial_nodecount++;
                else {
                    if constexpr (state.white_move) continue_perft<state, WHITE_OOO, root>(board, depth);
                    else                            continue_perft<state, BLACK_OOO, root>(board, depth);
                }
            }
        }

        // pawns
        {
            const map pawns = Pawns<us>(board);
            const map not_pinned = pawns & no_pin;
            const map pinned_straight = pawns & pin_rook;
            const map pinned_diag = pawns & pin_bishop;

            map pr = (pawn_atk_right<us>(pinned_diag) & pin_bishop | pawn_atk_right<us>(not_pinned)) & ColorPieces<enemy>(board);
            map pl = (pawn_atk_left<us>(pinned_diag) & pin_bishop | pawn_atk_left<us>(not_pinned)) & ColorPieces<enemy>(board);
            map pr_promo = pr & last_rank<us>(); pr &= not_last_rank<us>();
            map pl_promo = pl & last_rank<us>(); pl &= not_last_rank<us>();
            map pf = (pawn_forward<us>(pinned_straight) & pin_rook | pawn_forward<us>(not_pinned)) & ~board.Occ;
            map pf_promo = pf & last_rank<us>(); pf &= not_last_rank<us>();
            map pp = pawn_forward<us>(pf & third_rank<us>()) & ~board.Occ;

            if constexpr (state.has_ep_pawn){
                if (ep_target & ~pin_bishop){
                    Bit lep = ~pin_rook & ~A_FILE & (ep_target >> 1) & pawns;
                    Bit rep = ~pin_rook & ~H_FILE & (ep_target << 1) & pawns;

                    if constexpr (state.white_move) {
                        if (rep & pin_bishop) rep = pawn_atk_right<us>(rep) & pin_bishop;
                        if (lep & pin_bishop) lep = pawn_atk_left<us>(lep) & pin_bishop;
                    }
                    else {
                        if (rep & pin_bishop) rep = pawn_atk_left<us>(rep) & pin_bishop;
                        if (lep & pin_bishop) lep = pawn_atk_right<us>(lep) & pin_bishop;
                    }
                    if constexpr (count) {
                        if (lep) partial_nodecount++;
                        if (rep) partial_nodecount++;
                    }
                    else {
                        if constexpr (state.white_move) {
                            if (lep) continue_perft<state, root, true>(board, lep, pawn_atk_left<us>(lep), ep_target, depth);
                            if (rep) continue_perft<state, root, true>(board, rep, pawn_atk_right<us>(rep), ep_target, depth);
                        }
                        else{
                            if (lep) continue_perft<state, root, true>(board, lep, pawn_atk_right<us>(lep), ep_target, depth);
                            if (rep) continue_perft<state, root, true>(board, rep, pawn_atk_left<us>(rep), ep_target, depth);
                        }
                    }
                }
            }

            if constexpr (count){
                partial_nodecount += BitCount(pr);
                partial_nodecount += BitCount(pl);
                partial_nodecount += 4*BitCount(pr_promo);
                partial_nodecount += 4*BitCount(pl_promo);
                partial_nodecount += BitCount(pf | pp);
                partial_nodecount += 4*BitCount(pf_promo);
            }
            else pawn_loops<state, root>(board, depth, pr, pl, pr_promo, pl_promo, pf, pf_promo, pp);
        }

        // knights
        {
            map knights = Knights<us>(board) & no_pin;
            Bitloop(knights) {
                const Square sq = SquareOf(knights);
                map moves = Lookup::knight[sq];
                map captures = moves & ColorPieces<enemy>(board);
                moves &= ~board.Occ;
                if constexpr (count){
                    partial_nodecount += BitCount(moves | captures);
                }
                else { both_loops(KNIGHT) }
            }
        }

        const map queens = Queens<us>(board);
        // bishops & queens
        {
            const map bishops = Bishops<us>(board);
            map pinned = (queens | bishops) & pin_bishop;
            map not_pinned = (queens | bishops) & no_pin;

            Bitloop(pinned){
                const Square sq = SquareOf(pinned);
                map moves = Lookup::b_atk(sq, board.Occ) & pin_bishop;
                map captures = moves & ColorPieces<enemy>(board);
                moves &= ~board.Occ;
                if constexpr (count){
                    partial_nodecount += BitCount(moves | captures);
                }
                else {
                    if (1ull << sq & queens)    { both_loops(QUEEN) }
                    else                        { both_loops(BISHOP) }
                }
            }
            Bitloop(not_pinned){
                const Square sq = SquareOf(not_pinned);
                map moves = Lookup::b_atk(sq, board.Occ);
                map captures = moves & ColorPieces<enemy>(board);
                moves &= ~board.Occ;
                if constexpr (count){
                    partial_nodecount += BitCount(moves | captures);
                }
                else {
                    if (1ull << sq & queens)    { both_loops(QUEEN) }
                    else                        { both_loops(BISHOP) }
                }
            }
        }
        // rooks & queens
        {
            const map rooks = Rooks<us>(board);
            map pinned = (queens | rooks) & pin_rook;
            map not_pinned = (queens | rooks) & no_pin;

            Bitloop(pinned){
                const Square sq = SquareOf(pinned);
                map moves = Lookup::r_atk(sq, board.Occ) & pin_rook;
                map captures = moves & ColorPieces<enemy>(board);
                moves &= ~board.Occ;
                if constexpr (count){
                    partial_nodecount += BitCount(moves | captures);
                }
                else {
                    if (1ull << sq & queens)    { both_loops(QUEEN) }
                    else                        { both_loops(ROOK) }
                }
            }
            Bitloop(not_pinned){
                const Square sq = SquareOf(not_pinned);
                map moves = Lookup::r_atk(sq, board.Occ);
                map captures = moves & ColorPieces<enemy>(board);
                moves &= ~board.Occ;
                if constexpr (count){
                    partial_nodecount += BitCount(moves | captures);
                }
                else {
                    if (1ull << sq & queens)    { both_loops(QUEEN) }
                    else                        { both_loops(ROOK) }
                }
            }
        }

    }

    static std::chrono::steady_clock::time_point start_time;

    template <State state, bool root>
    void enumerate(const Board& board, int depth, Bit ep_target){
        if constexpr (root) start_time = std::chrono::steady_clock::now();
        const map king = King<state.white_move>(board);
        map rook_pin = 0, bishop_pin = 0, kingban = 0, checkmask, kingmoves;
        make_masks<state>(board, king, checkmask, kingban, rook_pin, bishop_pin, kingmoves, ep_target);
        if (depth == 1){
            partial_nodecount += BitCount(kingmoves);
            if (checkmask == full)
                _enumerate<state, root, true>(board, rook_pin, bishop_pin, depth, ep_target, kingban);
            else if (checkmask)
                _enumerate<state, root, true>(board, checkmask, rook_pin, bishop_pin, depth, ep_target);
            // else double check
        }
        else {
            king_moves<state, root>(board, depth, kingmoves);
            if (checkmask == full)
                _enumerate<state, root, false>(board, rook_pin, bishop_pin, depth, ep_target, kingban);
            else if (checkmask)
                _enumerate<state, root, false>(board, checkmask, rook_pin, bishop_pin, depth, ep_target);
            // else double check
        }

        if constexpr (root){
            std::cout << "Overall nodes: " << overall_nodecount << std::endl;
            std::cout << "Time taken: " << std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start_time) << std::endl;
            std::cout << "Mn/s: " << overall_nodecount / (std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - start_time).count()) << std::endl;
            double sPmn = static_cast<double>(std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - start_time).count()) / static_cast<double>(overall_nodecount);
            std::cout << "s/Mn: " << sPmn << std::endl;
            std::cout << "light distance equivalent: " << 299792458 * 10e-7 * sPmn << " meters" << std::endl;
            overall_nodecount = 0;
        }

    }

    COMPILETIME void init_lookup(){
        Lookup::Fill();
    }

}