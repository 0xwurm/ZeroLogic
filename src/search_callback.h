#pragma once
#include "search_helpers.h"

namespace ZeroLogic::Search {
    using namespace Boardstate;
    using namespace Movegen;

    class new_Callback{

    private:

        static inline u32 nodecount, tt_hits;
        static inline u8 seldepth;
        static inline u8 full_depth;

        static inline std::chrono::steady_clock::time_point start_time;

        struct rated_move{
            u16 move;
            s8 static_rating;
            eval (*Mcallback)(const Board&, const Bit, const u16, const u8, eval, eval);
        };

#define ep_hash ZeroLogic::TT::keys[ZeroLogic::TT::ep0 + (SquareOf(ep_target) % 8)]

        template<State state, bool skip>
        static eval Mcallback_pp(const Board& board, const Bit ep_target, const u16 enc, const u8 depth, eval Nalpha, eval Nbeta){
            Bit from = 1ull << ((enc & 0b1111110000) >> 4);
            Bit to = 1ull << (enc >> 10);
            map ephash = ZeroLogic::TT::keys[ZeroLogic::TT::ep0 + (SquareOf(to) % 8)];
            if constexpr (state.has_ep_pawn) ephash ^= ep_hash;
            const Board new_board = move<PAWN, state.white_move, false, true, PIECE_INVALID>(board, from , to, ephash);
            return eval(-loop<state.new_ep_pawn(), skip>(new_board, to, depth + 1, Nalpha, Nbeta));
        }

        template<State state, Piece piece, bool capture, bool skip>
        static eval Mcallback_any(const Board& board, const Bit ep_target, const u16 enc, const u8 depth, eval Nalpha, eval Nbeta){
            constexpr bool us = state.white_move;
            Bit from = 1ull << ((enc & 0b1111110000) >> 4);
            Bit to = 1ull << (enc >> 10);
            const Board new_board = move<piece, us, capture, state.has_ep_pawn, PIECE_INVALID>(board, from, to, ep_hash);


            if constexpr (piece == KING)
                return eval(-loop<state.no_castles(), skip>(new_board, 0, depth + 1, Nalpha, Nbeta));

            if constexpr (piece == ROOK){
                if constexpr (state.can_oo<us>()) {
                    if (state.is_rook_right<us>(from))
                        return eval(-loop<state.no_oo(), skip>(new_board, 0, depth + 1, Nalpha, Nbeta));
                }
                else if constexpr (state.can_ooo<us>()) {
                    if (state.is_rook_left<us>(from))
                        return eval(-loop<state.no_ooo(), skip>(new_board, 0, depth + 1, Nalpha, Nbeta));
                }
            }

            return eval(-loop<state.silent_move(), skip>(new_board, 0, depth + 1, Nalpha, Nbeta));
        }

        template<State state, Piece promoted, bool capture>
        static eval Mcallback_promo(const Board& board, const Bit ep_target, const u16 enc, const u8 depth, eval Nalpha, eval Nbeta) {
            Bit from = 1ull << ((enc & 0b1111110000) >> 4);
            Bit to = 1ull << (enc >> 10);
            const Board new_board = move<PAWN, state.white_move, capture, state.has_ep_pawn, promoted>(board, from, to, ep_hash);
            return eval(-loop<state.silent_move(), true>(new_board, 0, depth + 1, Nalpha, Nbeta));
        }

        template<State state, bool skip>
        static eval Mcallback_ep(const Board& board, const Bit ep_target, const u16 enc, const u8 depth, eval Nalpha, eval Nbeta){
            Bit from = 1ull << ((enc & 0b1111110000) >> 4);
            Bit to = 1ull << (enc >> 10);
            const Board new_board = Boardstate::ep_move<state.white_move>(board, from | to, ep_target);
            return eval(-loop<state.silent_move(), skip>(new_board, 0, depth + 1, Nalpha, Nbeta));
        }

        template<State state, CastleType ct>
        static eval Mcallback_castles(const Board& board, const Bit ep_target, const u16 enc, const u8 depth, eval Nalpha, eval Nbeta){
            const Board new_board = castle_move<ct, state.has_ep_pawn>(board, ep_hash);
            return eval(-loop<state.no_castles(), false>(new_board, 0, depth + 1, Nalpha, Nbeta));
        }

#undef ep_hash

    public:

        using specific = rated_move*;

        static void check_hash_move(const Board& board, rated_move* start, const rated_move*& end, const u32& hash_key){
            if (TT::table[hash_key].hash == board.hash){
                const u16 hashmove = TT::table[hash_key].move;
                for (rated_move* head = start; head < end; ++head){
                    if (head->move == hashmove){
                        head->static_rating = 50;
                        break;
                    }
                }
            }
        }

        static void selsort(rated_move start[200], const rated_move* end){
            rated_move* best = start;
            for (rated_move* head = start + 1; head < end; ++head){
                if (head->static_rating > best->static_rating)
                    best = head;
            }
            rated_move buffer = *start;
            *start = *best;
            *best = buffer;
        }

        template <State state, bool skip>
        static eval loop(const Board& board, Bit ep_target, const u8 depth, eval alpha, eval beta){

            if constexpr (!skip) {
                if (depth >= full_depth) {
                    if (depth > seldepth) seldepth = depth;
                    ++nodecount;
                    return Eval::evaluate<state.white_move>(board);
                }
            }

            rated_move ml_start[200]{};
            rated_move* movelist = ml_start;

            {
                map rook_pin = 0, bishop_pin = 0, kingban = 0, checkmask, kingmoves;
                make_masks<state>(board, checkmask, kingban, rook_pin, bishop_pin, kingmoves, ep_target);
                if (checkmask == full) enumerate<state, false, false, new_Callback>(board, ep_target, depth, movelist, checkmask, rook_pin, bishop_pin, kingban, kingmoves);
                else enumerate<state, false, true, new_Callback>(board, ep_target, depth, movelist, checkmask, rook_pin, bishop_pin, kingban, kingmoves);

                if (ml_start == movelist) {
                    if (checkmask != full) return MATE_NEG + depth;
                    return DRAW;
                }
            }

            const rated_move *ml_end = movelist;

            // check for hash move
            const u32 hash_key = board.hash & TT::key_mask;
            check_hash_move(board, ml_start, ml_end, hash_key);

            u16 bestmove{};
            for (rated_move* head = ml_start; head < ml_end; ++head){

                selsort(head, ml_end);
                if (head->static_rating < -10) break;
                eval nodeval = head->Mcallback(board, ep_target, head->move, depth, -beta, -alpha);


                if (nodeval > alpha) {
                    alpha = nodeval;
                    bestmove = head->move;

                    if (alpha >= beta) {
                        if ((full_depth - depth) > TT::table[hash_key].depth)
                            TT::table[hash_key] = {board.hash, u8(full_depth - depth), bestmove};
                        return beta;
                    }
                }
            }

            if ((full_depth - depth) >= TT::table[hash_key].depth)
                TT::table[hash_key] = {board.hash, u8(full_depth - depth), bestmove};

            return alpha;
        }

        static void display_info(const Board& board, State state, const Bit ep_target, eval evaluation) {
            auto duration = std::chrono::steady_clock::now() - start_time;
            auto duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
            if (!duration_ms) duration_ms = 1;

            std::stringstream info;
            info    << "info"
                    << " depth "        << int(full_depth)
                    << " seldepth "     << int(seldepth)
                    << " score "        << Misc::uci_eval(evaluation)
                    << " time "         << duration_ms
                    << " nodes "        << nodecount
                    << " nps "          << 1000 * (u64(nodecount) / duration_ms)
                    << " pv"            << Misc::uci_pv(board, state, ep_target, full_depth);

            std::cout << info.str() << std::endl << std::flush;
        }

        template<State state>
        static eval go_iteration(const Board& board, const Bit& ep_target, u8 depth, eval alpha, eval beta){
            seldepth = 0;
            full_depth = depth;

            return loop<state, false>(board, ep_target, 0, alpha, beta);
        }

        template<State state>
        static void go(const Board& board, const Bit& ep_target, u8 depth){
            nodecount = 0;
            tt_hits = 0;
            start_time = std::chrono::steady_clock::now();
            eval wlow = eval(25), whigh = eval(25), last_val = EQUAL;

            for (u8 currdepth = 1; currdepth <= depth; ){
                eval val = go_iteration<state>(board, ep_target, currdepth, last_val - wlow, last_val + whigh);

                if      (val <= last_val - wlow)    wlow *= 5;
                else if (val >= last_val + whigh)   whigh *= 5;
                else {
                    last_val = val;
                    wlow = eval(25);
                    whigh = eval(25);
                    currdepth++;
                    display_info(board, state, ep_target, val);
                    if (abs(last_val) > 30000) break;
                }
            }

            std::cout << "bestmove " << Misc::uci_move(TT::table[board.hash & TT::key_mask].move) << std::endl;
        }

        template<State state>
        static void go_single(const Board& board, const Bit& ep_target, u8 depth){
            nodecount = 0;
            tt_hits = 0;
            start_time = std::chrono::steady_clock::now();

            eval val = go_iteration<state>(board, ep_target, depth, ABSOLUTE_MIN, ABSOLUTE_MAX);
            display_info(board, state, ep_target, val);
        }

        template<State state, bool root, bool check>
        static FORCEINLINE void kingmove(const Board& board, map& moves, map& captures, const Bit& ep_target, const u8& depth, rated_move*& movelist){
            const u16 king_from = SquareOf(King<state.white_move>(board)) << 4;
            Bitloop(moves)
                *(movelist++) = {u16((SquareOf(moves) << 10) | king_from), -1, Mcallback_any<state, KING, false, check>};
            Bitloop(captures) {
                const Bit to = 1ull << SquareOf(captures);
                const s8 sval = captured_piece_val<state.white_move>(board, to);
                if constexpr (state.white_move) *(movelist++) = {u16((SquareOf(captures) << 10) | king_from), s8(-sval), (sval) ? Mcallback_any<state, KING, true, true> : Mcallback_any<state, KING, true, check>};
                else                            *(movelist++) = {u16((SquareOf(captures) << 10) | king_from), sval, (sval) ? Mcallback_any<state, KING, true, true> : Mcallback_any<state, KING, true, check>};
            }
        }

        template<State state, bool root, bool check>
        static FORCEINLINE void ep_move(const Board& board, Bit& lep, Bit& rep, const Bit& ep_target, const u8& depth, rated_move*& movelist){
            constexpr bool us = state.white_move;
            if (lep) *(movelist++) = {u16((SquareOf(pawn_atk_left<us>(lep)) << 10) | (SquareOf(lep) << 4)), 1, Mcallback_ep<state, check>};
            if (rep) *(movelist++) = {u16((SquareOf(pawn_atk_right<us>(rep)) << 10) | (SquareOf(rep) << 4)), 1, Mcallback_ep<state, check>};
        }

        template<State state, bool root, bool check>
        static FORCEINLINE void pawn_move(const Board& board, map& pr, map& pl, map& pr_promo, map& pl_promo, map& pf, map& pp, map& pf_promo, const Bit& ep_target, const u8& depth, rated_move*& movelist){
            constexpr bool us = state.white_move;
            Bitloop(pr){
                const u16 to = SquareOf(pr), from = to + pawn_shift[0][us];
                s8 sval = static_rate<us, PAWN>(board, u8(to), 1ull << from, 1ull << to);
                *(movelist++) = {u16((to << 10) | (from << 4)), sval, (sval) ? Mcallback_any<state, PAWN, true, true> : Mcallback_any<state, PAWN, true, check>};
            }
            Bitloop(pl){
                const u16 to = SquareOf(pl), from = to + pawn_shift[1][us];
                s8 sval = static_rate<us, PAWN>(board, to, 1ull << from, 1ull << to);
                *(movelist++) = {u16((to << 10) | (from << 4)), sval, (sval) ? Mcallback_any<state, PAWN, true, true> : Mcallback_any<state, PAWN, true, check>};
            }
            Bitloop(pr_promo){
                const u16 to = SquareOf(pr_promo), from = to + pawn_shift[0][us], move = (to << 10) | (from << 4);
                const eval see_val = static_rate<us, PAWN>(board, to, 1ull << from, 1ull << to);
                *(movelist++) = {u16(move | 0b0100), s8(see_val + Wqn), Mcallback_promo<state, QUEEN, true>};
                *(movelist++) = {u16(move | 0b0010), s8(see_val + Wknght), Mcallback_promo<state, KNIGHT, true>};
                *(movelist++) = {u16(move | 0b0001), s8(see_val), Mcallback_promo<state, ROOK, true>};
                *(movelist++) = {u16(move | 0b0011), s8(see_val), Mcallback_promo<state, BISHOP, true>};
            }
            Bitloop(pl_promo){
                const u16 to = SquareOf(pl_promo), from = to + pawn_shift[1][us], move = (to << 10) | (from << 4);
                const eval see_val = static_rate<us, PAWN>(board, to, 1ull << from, 1ull << to);
                *(movelist++) = {u16(move | 0b0100), s8(see_val + Wqn), Mcallback_promo<state, QUEEN, true>};
                *(movelist++) = {u16(move | 0b0010), s8(see_val + Wknght), Mcallback_promo<state, KNIGHT, true>};
                *(movelist++) = {u16(move | 0b0001), s8(see_val), Mcallback_promo<state, ROOK, true>};
                *(movelist++) = {u16(move | 0b0011), s8(see_val), Mcallback_promo<state, BISHOP, true>};
            }
            Bitloop(pf){
                const u16 to = SquareOf(pf);
                *(movelist++) = {u16((to << 10) | ((to + sign<us>(-8)) << 4)), -1, Mcallback_any<state, PAWN, false, check>};
            }
            Bitloop(pf_promo){
                const u16 to = SquareOf(pf_promo), from = to + sign<us>(-8), move = (to << 10) | (from << 4);
                *(movelist++) = {u16(move | 0b0100), Wqn, Mcallback_promo<state, QUEEN, false>};
                *(movelist++) = {u16(move | 0b0010), Wknght, Mcallback_promo<state, KNIGHT, false>};
                *(movelist++) = {u16(move | 0b0001), EQUAL, Mcallback_promo<state, ROOK, false>};
                *(movelist++) = {u16(move | 0b0011), EQUAL, Mcallback_promo<state, BISHOP, false>};
            }
            Bitloop(pp){
                const u8 to = SquareOf(pp);
                *(movelist++) = {u16((to << 10) | ((to + sign<us>(-16)) << 4)), -1, Mcallback_pp<state, check>};
            }
        }

        template<State state, Piece piece, bool root, bool check>
        static FORCEINLINE void silent_move(const Board& board, map& moves, map& captures, const Square& from, const Bit& ep_target, const u8& depth, rated_move*& movelist){
            u16 fromshift = u16(from) << 4;
            Bitloop(moves)
                *(movelist++) = {u16((SquareOf(moves) << 10) | fromshift), -1, Mcallback_any<state, piece, false, check>};
            Bitloop(captures){
                const u16 to = SquareOf(captures);
                s8 sval = static_rate<state.white_move, piece>(board, u8(to), 1ull << from, 1ull << to);
                *(movelist++) = {u16((to << 10) | fromshift), sval, (sval) ? Mcallback_any<state, piece, true, true> : Mcallback_any<state, piece, true, check>};
            }
        }

        template<State state, bool root, bool check>
        static FORCEINLINE void rookmove(const Board& board, map& moves, map& captures, const Square& sq, const Bit& ep_target, const u8& depth, rated_move*& movelist){
            silent_move<state, ROOK, root, check>(board, moves, captures, sq, ep_target, depth, movelist);
        }

        template<State state, bool left, bool root, bool leaf>
        static FORCEINLINE void castlemove(const Board& board, const Bit& ep_target, const u8& depth, rated_move*& movelist){
            if constexpr (left) {
                if constexpr (state.white_move) *(movelist++) = {(5 << 10) | (3 << 4) | 0b1000, -1, Mcallback_castles<state, WHITE_OOO>};
                else                            *(movelist++) = {(61 << 10) | (59 << 4) | 0b1000, -1, Mcallback_castles<state, BLACK_OOO>};
            }
            else {
                if constexpr (state.white_move) *(movelist++) = {(1 << 10) | (3 << 4) | 0b1000, -1, Mcallback_castles<state, WHITE_OO>};
                else                            *(movelist++) = {(57 << 10) | (59 << 4) | 0b1000, -1, Mcallback_castles<state, BLACK_OO>};
            }
        }

    };

}
