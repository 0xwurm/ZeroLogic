#pragma once
#include "gamestate.h"
#include "tables.h"
#include "eval.h"

namespace ZeroLogic::Movegen{
    using namespace Boardstate;


    template<bool white>
    FORCEINLINE map pawn_atk_right(const map &pieces) {
        if constexpr (white) return (pieces & NOT_BORDER_EAST) << 7;
        else return (pieces & NOT_BORDER_EAST) >> 9;
    }

    template<bool white>
    FORCEINLINE map pawn_atk_left(const map &pieces) {
        if constexpr (white) return (pieces & NOT_BORDER_WEST) << 9;
        else return (pieces & NOT_BORDER_WEST) >> 7;
    }

    template<bool white>
    FORCEINLINE map pawn_forward(const map &pieces) {
        if constexpr (white) return pieces << 8;
        else return pieces >> 8;
    }

    template<bool white>
    COMPILETIME map last_rank() {
        if constexpr (white) return EIGHTH_RANK;
        else return FIRST_RANK;
    }

    template<bool white>
    COMPILETIME map not_last_rank() {
        if constexpr (white) return ~EIGHTH_RANK;
        else return ~FIRST_RANK;
    }

    template<bool white>
    COMPILETIME map third_rank() {
        if constexpr (white) return THIRD_RANK;
        else return SIXTH_RANK;
    }

    template<bool white>
    COMPILETIME map EPRank() {
        if constexpr (white) return 0xffull << 32;
        else return 0xffull << 24;
    }

    template <bool white>
    FORCEINLINE void pin(Square king, Square pinner, map& pinmask, const Board& board){
        const map path = Lookup::pin_between[64 * king + pinner];
        if (path & ColorPieces<white>(board))
            pinmask |= path;
    }

    FORCEINLINE void slider_check(Square king, Square attacker, map& kingban, map& checkmask){
        if (checkmask == full)  checkmask = Lookup::pin_between[64 * king + attacker];
        else                    checkmask = 0;
        kingban |= Lookup::check_between[64 * king + attacker];
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
                if (pr & king) checkmask = pawn_atk_left<us>(king);
                else if (pl & king) checkmask = pawn_atk_right<us>(king);
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

#define silent_loops(piece)\
    Bitloop(moves)  \
    Callback::template silent_move<state, false, piece, root>(board, 1ull << sq, 1ull << SquareOf(moves), depth);  \
    Bitloop(captures)   \
    Callback::template silent_move<state, true, piece, root>(board, 1ull << sq, 1ull << SquareOf(captures), depth);

    template <State state, bool root, class Callback>
    FORCEINLINE void pawn_loops(const Board& board, u8 depth, map pr, map pl, map pr_promo, map pl_promo, map pf, map pf_promo, map pp){
        constexpr bool us = state.white_move;
        Bitloop(pr){
            Square sq = SquareOf(pr);
            Callback::template silent_move<state, true, PAWN, root>(board, 1ull << (sq + pawn_shift[0][us]), 1ull << sq, depth);
        }
        Bitloop(pl){
            Square sq = SquareOf(pl);
            Callback::template silent_move<state, true, PAWN, root>(board, 1ull << (sq + pawn_shift[1][us]), 1ull << sq, depth);
        }
        Bitloop(pr_promo){
            Square sq = SquareOf(pr_promo); Bit from = 1ull << (sq + pawn_shift[0][us]), to = 1ull << sq;
            Callback::template promotion_move<state, QUEEN, true, root>(board, from, to, depth);
            Callback::template promotion_move<state, ROOK, true, root>(board, from, to, depth);
            Callback::template promotion_move<state, BISHOP, true, root>(board, from, to, depth);
            Callback::template promotion_move<state, KNIGHT, true, root>(board, from, to, depth);
        }
        Bitloop(pl_promo){
            Square sq = SquareOf(pl_promo); Bit from = 1ull << (sq + pawn_shift[1][us]), to = 1ull << sq;
            Callback::template promotion_move<state, QUEEN, true, root>(board, from, to, depth);
            Callback::template promotion_move<state, ROOK, true, root>(board, from, to, depth);
            Callback::template promotion_move<state, BISHOP, true, root>(board, from, to, depth);
            Callback::template promotion_move<state, KNIGHT, true, root>(board, from, to, depth);
        }
        Bitloop(pf){
            Square sq = SquareOf(pf);
            Callback::template silent_move<state, false, PAWN, root>(board, 1ull << (sq + sign<us>(-8)), 1ull << sq, depth);
        }
        Bitloop(pf_promo){
            Square sq = SquareOf(pf_promo); Bit from = 1ull << (sq + sign<us>(-8)), to = 1ull << sq;
            Callback::template promotion_move<state, QUEEN, false, root>(board, from, to, depth);
            Callback::template promotion_move<state, ROOK, false, root>(board, from, to, depth);
            Callback::template promotion_move<state, BISHOP, false, root>(board, from, to, depth);
            Callback::template promotion_move<state, KNIGHT, false, root>(board, from, to, depth);
        }
        Bitloop(pp){
            Square sq = SquareOf(pp);
            Callback::template pawn_push<state, root>(board, 1ull << (sq + sign<us>(-16)), 1ull << sq, depth);
        }
    }

    template <State state, bool root, class Callback>
    FORCEINLINE void king_moves(const Board& board, u8 depth, map& kingmoves) {
        constexpr bool us = state.white_move;
        constexpr bool enemy = !us;
        map king_captures = kingmoves & ColorPieces<enemy>(board);
        kingmoves &= ~board.Occ;
        Bitloop(kingmoves)
            Callback::template kingmove<state, false, root>(board, King<us>(board), 1ull << SquareOf(kingmoves), depth);
        Bitloop(king_captures)
            Callback::template kingmove<state, true, root>(board, King<us>(board), 1ull << SquareOf(king_captures), depth);
    }

    // enumerator for check-positions
    template <State state, bool root, bool count, class Callback>
    FORCEINLINE void _enumerate(const Board& board, const map& checkmask, const map& pin_rook, const map& pin_bishop, u8 depth, const map& ep_target){
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
            map pp = pawn_forward<us>(pf & third_rank<us>()) & ~board.Occ & checkmask; pf &= checkmask;
            map pf_promo = pf & last_rank<us>(); pf &= not_last_rank<us>();

            if constexpr (state.has_ep_pawn){
                if (ep_target){
                    Bit lep = not_pinned & ~A_FILE & ((ep_target & checkmask) >> 1);
                    Bit rep = not_pinned & ~H_FILE & ((ep_target & checkmask) << 1);

                    if constexpr (count) {
                        if (lep) Callback::increment_nodecount();
                        if (rep) Callback::increment_nodecount();
                    }
                    else {
                        if (lep) Callback::template ep_move<state, root>(board, lep, pawn_atk_left<us>(lep), ep_target, depth);
                        if (rep) Callback::template ep_move<state, root>(board, rep, pawn_atk_right<us>(rep), ep_target, depth);
                    }
                }
            }

            if constexpr (count){
                Callback::count(pr);
                Callback::count(pl);
                Callback::count_promotion(pr_promo);
                Callback::count_promotion(pl_promo);
                Callback::count(pf | pp);
                Callback::count_promotion(pf_promo);
            }
            else pawn_loops<state, root, Callback>(board, depth, pr, pl, pr_promo, pl_promo, pf, pf_promo, pp);
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
                    Callback::count(moves | captures);
                }
                else { silent_loops(KNIGHT) }
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
                    Callback::count(moves | captures);
                }
                else {
                    if (1ull << sq & queens)    { silent_loops(QUEEN) }
                    else                        { silent_loops(BISHOP) }
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
                    Callback::count(moves | captures);
                }
                else {
                    if (1ull << sq & queens){ silent_loops(QUEEN) }
                    else{
                        Bitloop(moves)
                            Callback::template rookmove<state, false, root>(board, 1ull << sq, 1ull << SquareOf(moves), depth);
                        Bitloop(captures)
                            Callback::template rookmove<state, true, root>(board, 1ull << sq, 1ull << SquareOf(captures), depth);
                    }
                }
            }
        }
    }

    // enumerator for non-check-positions
    template <State state, bool root, bool count, class Callback>
    FORCEINLINE void _enumerate(const Board& board, const map& pin_rook, const map& pin_bishop, u8 depth, const map& ep_target, const map kingban){
        constexpr bool us = state.white_move;
        constexpr bool enemy = !us;
        const map no_pin = ~(pin_rook | pin_bishop);

        // castles
        if constexpr (state.can_oo<us>()){
            if (State::short_legal<us>(board.Occ, Rooks<us>(board), kingban)) {
                if constexpr (count) Callback::increment_nodecount();
                else {
                    if constexpr (state.white_move) Callback::template castlemove<state, WHITE_OO, root>(board, depth);
                    else                            Callback::template castlemove<state, BLACK_OO, root>(board, depth);
                }
            }
        }
        if constexpr (state.can_ooo<us>()){
            if (State::long_legal<us>(board.Occ, Rooks<us>(board), kingban)) {
                if constexpr (count) Callback::increment_nodecount();
                else {
                    if constexpr (state.white_move) Callback::template castlemove<state, WHITE_OOO, root>(board, depth);
                    else                            Callback::template castlemove<state, BLACK_OOO, root>(board, depth);
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

                    if (rep & pin_bishop) rep = pawn_atk_right<us>(rep) & pin_bishop;
                    if (lep & pin_bishop) lep = pawn_atk_left<us>(lep) & pin_bishop;

                    if constexpr (count) {
                        if (lep) Callback::increment_nodecount();
                        if (rep) Callback::increment_nodecount();
                    }
                    else {
                        if (lep) Callback::template ep_move<state, root>(board, lep, pawn_atk_left<us>(lep), ep_target, depth);
                        if (rep) Callback::template ep_move<state, root>(board, rep, pawn_atk_right<us>(rep), ep_target, depth);
                    }
                }
            }

            if constexpr (count){
                Callback::count(pr);
                Callback::count(pl);
                Callback::count_promotion(pr_promo);
                Callback::count_promotion(pl_promo);
                Callback::count(pf | pp);
                Callback::count_promotion(pf_promo);
            }
            else pawn_loops<state, root, Callback>(board, depth, pr, pl, pr_promo, pl_promo, pf, pf_promo, pp);
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
                    Callback::count(moves | captures);
                }
                else { silent_loops(KNIGHT) }
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
                    Callback::count(moves | captures);
                }
                else {
                    if (1ull << sq & queens)    { silent_loops(QUEEN) }
                    else                        { silent_loops(BISHOP) }
                }
            }
            Bitloop(not_pinned){
                const Square sq = SquareOf(not_pinned);
                map moves = Lookup::b_atk(sq, board.Occ);
                map captures = moves & ColorPieces<enemy>(board);
                moves &= ~board.Occ;
                if constexpr (count){
                    Callback::count(moves | captures);
                }
                else {
                    if (1ull << sq & queens)    { silent_loops(QUEEN) }
                    else                        { silent_loops(BISHOP) }
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
                    Callback::count(moves | captures);
                }
                else {
                    if (1ull << sq & queens){ silent_loops(QUEEN) }
                    else{
                        Bitloop(moves)
                            Callback::template rookmove<state, false, root>(board, 1ull << sq, 1ull << SquareOf(moves), depth);
                        Bitloop(captures)
                            Callback::template rookmove<state, true, root>(board, 1ull << sq, 1ull << SquareOf(captures), depth);
                        }
                }
            }
            Bitloop(not_pinned){
                const Square sq = SquareOf(not_pinned);
                map moves = Lookup::r_atk(sq, board.Occ);
                map captures = moves & ColorPieces<enemy>(board);
                moves &= ~board.Occ;
                if constexpr (count){
                    Callback::count(moves | captures);
                }
                else {
                    if (1ull << sq & queens){ silent_loops(QUEEN) }
                    else{
                        Bitloop(moves)
                            Callback::template rookmove<state, false, root>(board, 1ull << sq, 1ull << SquareOf(moves), depth);
                        Bitloop(captures)
                            Callback::template rookmove<state, true, root>(board, 1ull << sq, 1ull << SquareOf(captures), depth);
                    }
                }
            }
        }

    }

    template <State state, bool root, class Callback>
    void enumerate(const Board& board, u8 depth, Bit ep_target){
        const map king = King<state.white_move>(board);
        map rook_pin = 0, bishop_pin = 0, kingban = 0, checkmask, kingmoves;
        make_masks<state>(board, king, checkmask, kingban, rook_pin, bishop_pin, kingmoves, ep_target);
        if (depth == 1){
            Callback::count(kingmoves);
            if (checkmask == full)
                _enumerate<state, root, true, Callback>(board, rook_pin, bishop_pin, depth, ep_target, kingban);
            else if (checkmask)
                _enumerate<state, root, true, Callback>(board, checkmask, rook_pin, bishop_pin, depth, ep_target);
            // else double check
        }
        else {
            king_moves<state, root, Callback>(board, depth, kingmoves);
            if (checkmask == full)
                _enumerate<state, root, false, Callback>(board, rook_pin, bishop_pin, depth, ep_target, kingban);
            else if (checkmask)
                _enumerate<state, root, false, Callback>(board, checkmask, rook_pin, bishop_pin, depth, ep_target);
            // else double check
        }
    }

    COMPILETIME void init_lookup(){
        Lookup::Fill();
    }

}