#pragma once
#include "movegenerator.h"
#include <chrono>

namespace ZeroLogic {
    using namespace Boardstate;
    using namespace Movegen;

    static unsigned long long partial_nodecount;
    static unsigned long long overall_nodecount;
    static std::chrono::steady_clock::time_point start_time;
    static bool depth1;

    class Perft {

        static void init(u8& depth){
            depth1 = depth == 1;
            if (depth == 1) depth++;
            partial_nodecount = 0;
            overall_nodecount = 0;
            start_time = std::chrono::steady_clock::now();
        }

        static FORCEINLINE void count(map moves){
            partial_nodecount += BitCount(moves);
        }
        static FORCEINLINE void count_promo(map moves){
            partial_nodecount += 4*BitCount(moves);
        }
        static FORCEINLINE void increment(){
            ++partial_nodecount;
        }

        template<Piece promotion_to>
        static FORCEINLINE void display_move_stats(Bit from, Bit to){
            if (depth1) partial_nodecount = 1;
            std::cout << Misc::uci_move<promotion_to>(SquareOf(from), SquareOf(to)) << ": " << partial_nodecount << std::endl;
            overall_nodecount += partial_nodecount;
            partial_nodecount = 0;
        }

        template<CastleType type>
        static FORCEINLINE void display_move_stats(){
            if (depth1) partial_nodecount = 1;
            std::cout << Misc::uci_move<type>() << ": " << partial_nodecount << std::endl;
            overall_nodecount += partial_nodecount;
            partial_nodecount = 0;
        }

        static void display_perft_info(){
            auto duration = std::chrono::steady_clock::now() - start_time;
            auto duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(duration);
            auto duration_mys = std::chrono::duration_cast<std::chrono::microseconds>(duration).count();
            double sPmn = double(duration_mys) / double(overall_nodecount);

            std::cout << "Overall nodes: "              << overall_nodecount                         << std::endl;
            std::cout << "Time taken: "                 << duration_ms                               << std::endl;
            std::cout << "Mn/s: "                       << overall_nodecount / duration_mys          << std::endl;
            std::cout << "s/Mn: "                       << sPmn                                      << std::endl;
            std::cout << "light distance equivalent: "  << 299792458 * 10e-7 * sPmn << " meters"     << std::endl;
        }

        template<State state, bool capture, bool root>
        static FORCEINLINE void _kingmove(const Board& board, Bit& from, Bit to, u8& depth){
            const Board new_board = move<KING, state.white_move, capture>(board, from, to);
            enumerate<state.no_castles(), false, Perft>(new_board, depth - 1, 0);
            if constexpr (root) display_move_stats<PIECE_INVALID>(from, to);
        }

        template<State state, CastleType type, bool root>
        static FORCEINLINE void _castlemove(const Board& board, u8& depth){
            const Board new_board = castle_move<type>(board);
            enumerate<state.no_castles(), false, Perft>(new_board, depth - 1, 0);
            if constexpr (root) display_move_stats<type>();
        }

        template<State state, bool capture, Piece piece, bool root>
        static FORCEINLINE void _silent_move(const Board& board, Bit from, Bit to, u8& depth){
            const Board new_board = move<piece, state.white_move, capture>(board, from, to);
            enumerate<state.silent_move(), false, Perft>(new_board, depth - 1, 0);
            if constexpr (root) display_move_stats<PIECE_INVALID>(from, to);
        }

        template<State state, bool capture, bool root>
        static FORCEINLINE void _rookmove(const Board& board, Bit from, Bit to, u8& depth){
            constexpr bool us = state.white_move;
            const Board new_board = move<ROOK, us, capture>(board, from, to);

            [&]() {
                if constexpr (state.can_oo<us>()) {
                    if (state.is_rook_right<us>(from)) {
                        enumerate<state.no_oo<us>(), false, Perft>(new_board, depth - 1, 0);
                        return;
                    }
                } else if constexpr (state.can_ooo<us>()) {
                    if (state.is_rook_left<us>(from)) {
                        enumerate<state.no_ooo<us>(), false, Perft>(new_board, depth - 1, 0);
                        return;
                    }
                }
                enumerate<state.silent_move(), false, Perft>(new_board, depth - 1, 0);
            }();
            if constexpr (root) display_move_stats<PIECE_INVALID>(from, to);
        }

        template<State state, bool root>
        static FORCEINLINE void _ep_move(const Board& board, Bit& from, Bit to, Bit& ep_target, u8& depth){
            const Board new_board = Boardstate::ep_move<state.white_move>(board, from | to, ep_target);
            enumerate<state.silent_move(), false, Perft>(new_board, depth - 1, 0);
            if constexpr (root) display_move_stats<PIECE_INVALID>(from, to);
        }

        template<State state, bool root>
        static FORCEINLINE void _pawn_push(const Board& board, Bit from, Bit to, u8& depth){
            const Board new_board = move<PAWN, state.white_move, false>(board, from, to);
            enumerate<state.new_ep_pawn(), false, Perft>(new_board, depth - 1, to);
            if constexpr (root) display_move_stats<PIECE_INVALID>(from, to);
        }

        template<State state, Piece promotion_to, bool capture, bool root>
        static FORCEINLINE void _promotion_move(const Board& board, Bit& from, Bit& to, u8& depth){
            const Board new_board = Boardstate::promotion_move<promotion_to, state.white_move, capture>(board, from, to);
            enumerate<state.silent_move(), false, Perft>(new_board, depth - 1, 0);
            if constexpr (root) display_move_stats<promotion_to>(from, to);
        }

    public:

        template<State state>
        static void go(const Board& board, u8 depth, Bit ep_target){
            init(depth);
            enumerate<state, true, Perft>(board, depth, ep_target);
            display_perft_info();
        }

        template<State state, bool root, bool leaf>
        static FORCEINLINE void kingmove(const Board& board, map& moves, map& captures, u8& depth){
            constexpr bool us = state.white_move;
            if constexpr (leaf) count(moves | captures);
            else {
                Bit king = King<us>(board);
                Bitloop(moves)
                    _kingmove<state, false, root>(board, king, 1ull << SquareOf(moves), depth);
                Bitloop(captures)
                    _kingmove<state, true, root>(board, king, 1ull << SquareOf(captures), depth);
            }
        }

        template<State state, bool root, bool leaf>
        static FORCEINLINE void ep_move(const Board& board, Bit& lep, Bit& rep, Bit& ep_target, u8& depth){
            constexpr bool us = state.white_move;
            if constexpr (leaf) {
                if (lep) increment();
                if (rep) increment();
            }
            else {
                if (lep) _ep_move<state, root>(board, lep, pawn_atk_left<us>(lep), ep_target, depth);
                if (rep) _ep_move<state, root>(board, rep, pawn_atk_right<us>(rep), ep_target, depth);
            }
        }

        template<State state, bool root, bool leaf>
        static FORCEINLINE void pawn_move(const Board& board, map& pr, map& pl, map& pr_promo, map& pl_promo, map& pf, map& pp, map& pf_promo, u8& depth){
            if constexpr (leaf){
                count(pr);
                count(pl);
                count_promo(pr_promo);
                count_promo(pl_promo);
                count(pf | pp);
                count_promo(pf_promo);
            }
            else{
                constexpr bool us = state.white_move;
                Bitloop(pr){
                    Square sq = SquareOf(pr);
                    _silent_move<state, true, PAWN, root>(board, 1ull << (sq + pawn_shift[0][us]), 1ull << sq, depth);
                }
                Bitloop(pl){
                    Square sq = SquareOf(pl);
                    _silent_move<state, true, PAWN, root>(board, 1ull << (sq + pawn_shift[1][us]), 1ull << sq, depth);
                }
                Bitloop(pr_promo){
                    Square sq = SquareOf(pr_promo); Bit from = 1ull << (sq + pawn_shift[0][us]), to = 1ull << sq;
                    _promotion_move<state, QUEEN, true, root>(board, from, to, depth);
                    _promotion_move<state, ROOK, true, root>(board, from, to, depth);
                    _promotion_move<state, BISHOP, true, root>(board, from, to, depth);
                    _promotion_move<state, KNIGHT, true, root>(board, from, to, depth);
                }
                Bitloop(pl_promo){
                    Square sq = SquareOf(pl_promo); Bit from = 1ull << (sq + pawn_shift[1][us]), to = 1ull << sq;
                    _promotion_move<state, QUEEN, true, root>(board, from, to, depth);
                    _promotion_move<state, ROOK, true, root>(board, from, to, depth);
                    _promotion_move<state, BISHOP, true, root>(board, from, to, depth);
                    _promotion_move<state, KNIGHT, true, root>(board, from, to, depth);
                }
                Bitloop(pf){
                    Square sq = SquareOf(pf);
                    _silent_move<state, false, PAWN, root>(board, 1ull << (sq + sign<us>(-8)), 1ull << sq, depth);
                }
                Bitloop(pf_promo){
                    Square sq = SquareOf(pf_promo); Bit from = 1ull << (sq + sign<us>(-8)), to = 1ull << sq;
                    _promotion_move<state, QUEEN, false, root>(board, from, to, depth);
                    _promotion_move<state, ROOK, false, root>(board, from, to, depth);
                    _promotion_move<state, BISHOP, false, root>(board, from, to, depth);
                    _promotion_move<state, KNIGHT, false, root>(board, from, to, depth);
                }
                Bitloop(pp){
                    Square sq = SquareOf(pp);
                    _pawn_push<state, root>(board, 1ull << (sq + sign<us>(-16)), 1ull << sq, depth);
                }
            }
        }

        template<State state, Piece piece, bool root, bool leaf>
        static FORCEINLINE void silent_move(const Board& board, map& moves, map& captures, const Square& sq, u8& depth){
            if constexpr (leaf)
                count(moves | captures);
            else{
                Bitloop(moves)
                    _silent_move<state, false, piece, root>(board, 1ull << sq, 1ull << SquareOf(moves), depth);
                Bitloop(captures)
                    _silent_move<state, true, piece, root>(board, 1ull << sq, 1ull << SquareOf(captures), depth);

            }
        }
        template<State state, bool root, bool leaf>
        static FORCEINLINE void rookmove(const Board& board, map& moves, map& captures, const Square& sq, u8& depth){
            if constexpr (leaf)
                count(moves | captures);
            else{
                Bitloop(moves)
                    _rookmove<state, false, root>(board, 1ull << sq, 1ull << SquareOf(moves), depth);
                Bitloop(captures)
                    _rookmove<state, true, root>(board, 1ull << sq, 1ull << SquareOf(captures), depth);

            }
        }

        template<State state, bool left, bool root, bool leaf>
        static FORCEINLINE void castlemove(const Board& board, u8& depth){
            if constexpr (leaf) increment();
            else {
                if constexpr (left) {
                    if constexpr (state.white_move) _castlemove<state, WHITE_OOO, root>(board, depth);
                    else                            _castlemove<state, BLACK_OOO, root>(board, depth);
                }
                else {
                    if constexpr (state.white_move) _castlemove<state, WHITE_OO, root>(board, depth);
                    else                            _castlemove<state, BLACK_OO, root>(board, depth);
                }
            }
        }
    };
}