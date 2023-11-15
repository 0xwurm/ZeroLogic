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
    FORCEINLINE void ep_pin(Square king_square, const Bit& king, map enemyRQ, const Board& board, Bit& ep_target){
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

    template<State state, bool root, bool leaf, class Callback>
    FORCEINLINE void king_moves(const Board& board, map& kingmoves, typename Callback::vars& vars, bool& found_move){
        map kingcaptures = kingmoves & ColorPieces<!state.white_move>(board);
        kingmoves &= ~board.Occ;
        if (kingmoves | kingcaptures) found_move = true;
        Callback::template kingmove<state, root, leaf>(board, kingmoves, kingcaptures, vars);
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

    // enumerator for check-positions
    template <State state, bool root, bool leaf, class Callback>
    FORCEINLINE void _enumerate_check(const Board& board, const map& checkmask, const map& pin_rook, const map& pin_bishop, typename Callback::vars& var, Bit& ep_target, bool& found_move){
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

            if (pr | pl | pr_promo | pl_promo | pf | pp | pf_promo) found_move = true;
            Callback::template pawn_move<state, root, leaf>(board, pr, pl, pr_promo, pl_promo, pf, pp, pf_promo, var);
            if (Callback::prune()) { Callback::reset_prune(); return; }

            if constexpr (state.has_ep_pawn){
                if (ep_target){
                    Bit lep = not_pinned & ~A_FILE & ((ep_target & checkmask) >> 1);
                    Bit rep = not_pinned & ~H_FILE & ((ep_target & checkmask) << 1);

                    if (rep | lep) found_move = true;

                    Callback::template ep_move<state, root, leaf>(board, lep, rep, ep_target, var);
                    if (Callback::prune()) { Callback::reset_prune(); return; }
                }
            }
        }

        // knights
        {
            map knights = Knights<us>(board) & no_pin;
            Bitloop(knights) {
                const Square sq = SquareOf(knights);
                map moves = Lookup::knight[sq] & checkmask;
                map captures = moves & ColorPieces<enemy>(board);
                moves &= ~board.Occ;
                if (moves | captures) found_move = true;
                Callback::template silent_move<state, KNIGHT, root, leaf>(board, moves, captures, sq, var);
                if (Callback::prune()) { Callback::reset_prune(); return; }
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
                if (moves | captures) found_move = true;
                if (1ull << sq & queens)    Callback::template silent_move<state, QUEEN, root, leaf>(board, moves, captures, sq, var);
                else                        Callback::template silent_move<state, BISHOP, root, leaf>(board, moves, captures, sq, var);
                if (Callback::prune()) { Callback::reset_prune(); return; }
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
                if (moves | captures) found_move = true;
                if (1ull << sq & queens)    Callback::template silent_move<state, QUEEN, root, leaf>(board, moves, captures, sq, var);
                else                        Callback::template rookmove<state, root, leaf>(board, moves, captures, sq, var);
                if (Callback::prune()) { Callback::reset_prune(); return; }
            }
        }
    }

    // enumerator for non-check-positions
    template <State state, bool root, bool leaf, class Callback>
    FORCEINLINE void _enumerate(const Board& board, const map& pin_rook, const map& pin_bishop, typename Callback::vars& var, Bit& ep_target, const map& kingban, bool& found_move){
        constexpr bool us = state.white_move;
        constexpr bool enemy = !us;
        const map no_pin = ~(pin_rook | pin_bishop);

        // castles
        if constexpr (state.can_oo<us>())
            if (State::short_legal<us>(board.Occ, Rooks<us>(board), kingban)) {
                found_move = true;
                Callback::template castlemove<state, false, root, leaf>(board, var);
                if (Callback::prune()) { Callback::reset_prune(); return; }
            }
        if constexpr (state.can_ooo<us>())
            if (State::long_legal<us>(board.Occ, Rooks<us>(board), kingban)) {
                found_move = true;
                Callback::template castlemove<state, true, root, leaf>(board, var);
                if (Callback::prune()) { Callback::reset_prune(); return; }
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

            if (pr | pl | pr_promo | pf | pf_promo | pp) found_move = true;
            Callback::template pawn_move<state, root, leaf>(board, pr, pl, pr_promo, pl_promo, pf, pp, pf_promo, var);
            if (Callback::prune()) { Callback::reset_prune(); return; }

            if constexpr (state.has_ep_pawn){
                if (ep_target & ~pin_bishop){
                    Bit lep = ~pin_rook & ~A_FILE & (ep_target >> 1) & pawns;
                    Bit rep = ~pin_rook & ~H_FILE & (ep_target << 1) & pawns;

                    if (rep & pin_bishop) rep = pawn_atk_right<us>(rep) & pin_bishop;
                    if (lep & pin_bishop) lep = pawn_atk_left<us>(lep) & pin_bishop;

                    if (rep | lep) found_move = true;

                    Callback::template ep_move<state, root, leaf>(board, lep, rep, ep_target, var);
                    if (Callback::prune()) { Callback::reset_prune(); return; }
                }
            }
        }

        // knights
        {
            map knights = Knights<us>(board) & no_pin;
            Bitloop(knights) {
                const Square sq = SquareOf(knights);
                map moves = Lookup::knight[sq];
                map captures = moves & ColorPieces<enemy>(board);
                moves &= ~board.Occ;
                if (moves | captures) found_move = true;
                Callback::template silent_move<state, KNIGHT, root, leaf>(board, moves, captures, sq, var);
                if (Callback::prune()) { Callback::reset_prune(); return; }
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
                if (moves | captures) found_move = true;
                if (1ull << sq & queens)    Callback::template silent_move<state, QUEEN, root, leaf>(board, moves, captures, sq, var);
                else                        Callback::template silent_move<state, BISHOP, root, leaf>(board, moves, captures, sq, var);
                if (Callback::prune()) { Callback::reset_prune(); return; }
            }
            Bitloop(not_pinned){
                const Square sq = SquareOf(not_pinned);
                map moves = Lookup::b_atk(sq, board.Occ);
                map captures = moves & ColorPieces<enemy>(board);
                moves &= ~board.Occ;
                if (moves | captures) found_move = true;
                if (1ull << sq & queens)    Callback::template silent_move<state, QUEEN, root, leaf>(board, moves, captures, sq, var);
                else                        Callback::template silent_move<state, BISHOP, root, leaf>(board, moves, captures, sq, var);
                if (Callback::prune()) { Callback::reset_prune(); return; }
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
                if (moves | captures) found_move = true;
                if (1ull << sq & queens)    Callback::template silent_move<state, QUEEN, root, leaf>(board, moves, captures, sq, var);
                else                        Callback::template rookmove<state, root, leaf>(board, moves, captures, sq, var);
                if (Callback::prune()) { Callback::reset_prune(); return; }
            }
            Bitloop(not_pinned){
                const Square sq = SquareOf(not_pinned);
                map moves = Lookup::r_atk(sq, board.Occ);
                map captures = moves & ColorPieces<enemy>(board);
                moves &= ~board.Occ;
                if (moves | captures) found_move = true;
                if (1ull << sq & queens)    Callback::template silent_move<state, QUEEN, root, leaf>(board, moves, captures, sq, var);
                else                        Callback::template rookmove<state, root, leaf>(board, moves, captures, sq, var);
                if (Callback::prune()) { Callback::reset_prune(); return; }
            }
        }

    }

    template <State state, bool root, class Callback>
    void enumerate(const Board& board, typename Callback::vars& var, Bit ep_target){
        const map king = King<state.white_move>(board);
        map rook_pin = 0, bishop_pin = 0, kingban = 0, checkmask, kingmoves;
        make_masks<state>(board, king, checkmask, kingban, rook_pin, bishop_pin, kingmoves, ep_target);
        bool found_move = false;

        if (var.depth == 1){
            king_moves<state, root, true, Callback>(board, kingmoves, var, found_move);
            if (Callback::prune()) { Callback::reset_prune(); return; }
            if (checkmask == full) {
                _enumerate<state, root, true, Callback>(board, rook_pin, bishop_pin, var, ep_target, kingban,found_move);
                if (!found_move) Callback::draw(var);
            }
            else if (checkmask) {
                _enumerate_check<state, root, true, Callback>(board, checkmask, rook_pin, bishop_pin, var, ep_target,found_move);
                if (!found_move) Callback::template mate<state.white_move>(var);
            }
            else if (!found_move)
                Callback::template mate<state.white_move>(var);
        }
        else {
            king_moves<state, root, false, Callback>(board, kingmoves, var, found_move);
            if (Callback::prune()) { Callback::reset_prune(); return; }
            if (checkmask == full) {
                _enumerate<state, root, false, Callback>(board, rook_pin, bishop_pin, var, ep_target, kingban, found_move);
                if (!found_move) Callback::draw(var);
            }
            else if (checkmask) {
                _enumerate_check<state, root, false, Callback>(board, checkmask, rook_pin, bishop_pin, var, ep_target, found_move);
                if (!found_move) Callback::template mate<state.white_move>(var);
            }
            else if (!found_move)
                Callback::template mate<state.white_move>(var);
        }
    }

    COMPILETIME void init_lookup(){
        Lookup::Fill();
    }

}