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

    public:

        static void init(u8& depth){
            depth1 = depth == 1;
            if (depth == 1) depth++;
            partial_nodecount = 0;
            overall_nodecount = 0;
            start_time = std::chrono::steady_clock::now();
        }

        static FORCEINLINE void increment_nodecount(){
            ++partial_nodecount;
        }

        static FORCEINLINE void count(map moves){
            partial_nodecount += BitCount(moves);
        }

        static FORCEINLINE void count_promotion(map moves){
            partial_nodecount += 4*BitCount(moves);
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

        template<State state>
        static void go(const Board& board, u8 depth, Bit ep_target){
            init(depth);
            enumerate<state, true, Perft>(board, depth, ep_target);
            display_perft_info();
        }


        template<State state, bool capture, bool root>
        static FORCEINLINE void kingmove(const Board& board, Bit from, Bit to, u8 depth){
            const Board new_board = move<KING, state.white_move, capture>(board, from, to);
            enumerate<state.no_castles(), false, Perft>(new_board, depth - 1, 0);
            if constexpr (root) display_move_stats<PIECE_INVALID>(from, to);
        }

        template<State state, CastleType type, bool root>
        static FORCEINLINE void castlemove(const Board& board, u8 depth){
            const Board new_board = castle_move<type>(board);
            enumerate<state.no_castles(), false, Perft>(new_board, depth - 1, 0);
            if constexpr (root) display_move_stats<type>();
        }

        template<State state, bool capture, Piece piece, bool root>
        static FORCEINLINE void silent_move(const Board& board, Bit from, Bit to, u8 depth){
            const Board new_board = move<piece, state.white_move, capture>(board, from, to);
            enumerate<state.silent_move(), false, Perft>(new_board, depth - 1, 0);
            if constexpr (root) display_move_stats<PIECE_INVALID>(from, to);
        }

        template<State state, bool capture, bool root>
        static FORCEINLINE void rookmove(const Board& board, Bit from, Bit to, u8 depth){
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
        static FORCEINLINE void ep_move(const Board& board, Bit from, Bit to, Bit ep_target, u8 depth){
            const Board new_board = Boardstate::ep_move<state.white_move>(board, from | to, ep_target);
            enumerate<state.silent_move(), false, Perft>(new_board, depth - 1, 0);
            if constexpr (root) display_move_stats<PIECE_INVALID>(from, to);
        }

        template<State state, bool root>
        static FORCEINLINE void pawn_push(const Board& board, Bit from, Bit to, u8 depth){
            const Board new_board = move<PAWN, state.white_move, false>(board, from, to);
            enumerate<state.new_ep_pawn(), false, Perft>(new_board, depth - 1, to);
            if constexpr (root) display_move_stats<PIECE_INVALID>(from, to);
        }

        template<State state, Piece promotion_to, bool capture, bool root>
        static FORCEINLINE void promotion_move(const Board& board, Bit from, Bit to, u8 depth){
            const Board new_board = Boardstate::promotion_move<promotion_to, state.white_move, capture>(board, from, to);
            enumerate<state.silent_move(), false, Perft>(new_board, depth - 1, 0);
            if constexpr (root) display_move_stats<promotion_to>(from, to);
        }

    };
}