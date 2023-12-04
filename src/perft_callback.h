#pragma once

namespace ZeroLogic {
    using namespace Boardstate;
    using namespace Movegen;

    class Perft {

    public:

        static inline u64 overall_nodecount;
        static inline u32 tt_hits;
        static inline std::chrono::steady_clock::time_point start_time;
        static inline bool depth1;

        struct vars{
            u8 depth;
            u32 partial_nodecount;
        };

        static FORCEINLINE void mate(vars& var){}
        static FORCEINLINE void draw(vars& var){}
        static FORCEINLINE void end_routine(vars& var, const Board board){}

    private:

        static void init(u8& depth){
            depth1 = depth == 1;
            if (depth == 1) depth++;
            overall_nodecount = 0;
            tt_hits = 0;
            start_time = std::chrono::steady_clock::now();
        }

        static FORCEINLINE void count(map moves, u32& partial_nodecount){
            partial_nodecount += BitCount(moves);
        }
        static FORCEINLINE void count_promo(map moves, u32& partial_nodecount){
            partial_nodecount += 4*BitCount(moves);
        }
        static FORCEINLINE void increment(u32& partial_nodecount){
            ++partial_nodecount;
        }

        template<Piece promotion_to>
        static FORCEINLINE void display_move_stats(Bit from, Bit to, u32& partial_nodecount){
            if (depth1) partial_nodecount = 1;
            std::cout << Misc::uci_move<promotion_to>(SquareOf(from), SquareOf(to)) << ": " << partial_nodecount << std::endl;
            overall_nodecount += partial_nodecount;
        }

        template<CastleType type>
        static FORCEINLINE void display_move_stats(u32& partial_nodecount){
            if (depth1) partial_nodecount = 1;
            std::cout << Misc::uci_move<type>() << ": " << partial_nodecount << std::endl;
            overall_nodecount += partial_nodecount;
        }

        static void display_info(){
            auto duration = std::chrono::steady_clock::now() - start_time;
            auto duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(duration);
            auto duration_mys = std::chrono::duration_cast<std::chrono::microseconds>(duration).count();
            double sPmn = double(duration_mys) / double(overall_nodecount);

            std::cout << "Overall nodes: "              << overall_nodecount                         << std::endl;
            std::cout << "Time taken: "                 << duration_ms                               << std::endl;
            std::cout << "Mn/s: "                       << overall_nodecount / duration_mys          << std::endl;
            std::cout << "s/Mn: "                       << sPmn                                      << std::endl;
            std::cout << "light distance equivalent: "  << 299792458 * 10e-7 * sPmn << " meters"     << std::endl;
            std::cout << "tt hits: "                    << tt_hits                                   << std::endl;
        }

        template<State state, bool capture, bool root>
        static FORCEINLINE void _kingmove(const Board& board, Bit& from, Bit to, u8& depth, u32& partial_nodecount){
            const Board new_board = move<KING, state.white_move, capture>(board, from, to);
            u32 key = new_board.hash & TT::key_mask;
            if (TT::table[key].pos_hash == (new_board.hash) && TT::table[key].depth == depth){
                partial_nodecount += TT::table[key].nodecount;
                ++tt_hits;
            }
            else {
                vars new_var = {u8(depth - 1), 0};
                enumerate<state.no_castles(), false, Perft>(new_board, new_var, 0);
                partial_nodecount += new_var.partial_nodecount;
                if constexpr (root) display_move_stats<PIECE_INVALID>(from, to, new_var.partial_nodecount);
                if (depth > TT::table[key].depth) TT::table[key] = {(new_board.hash), new_var.partial_nodecount, depth};
            }
        }

        template<State state, CastleType type, bool root>
        static FORCEINLINE void _castlemove(const Board& board, u8& depth, u32& partial_nodecount){
            const Board new_board = castle_move<type>(board);
            u32 key = new_board.hash & TT::key_mask;
            if (TT::table[key].pos_hash == (new_board.hash) && TT::table[key].depth == depth){
                partial_nodecount += TT::table[key].nodecount;
                ++tt_hits;
            }
            else {
                vars new_var = {u8(depth - 1), 0};
                enumerate<state.no_castles(), false, Perft>(new_board, new_var, 0);
                partial_nodecount += new_var.partial_nodecount;
                if constexpr (root) display_move_stats<type>(new_var.partial_nodecount);
                if (depth > TT::table[key].depth) TT::table[key] = {(new_board.hash), new_var.partial_nodecount, depth};
            }
        }

        template<State state, bool capture, Piece piece, bool root>
        static FORCEINLINE void _silent_move(const Board& board, Bit from, Bit to, u8& depth, u32& partial_nodecount){
            const Board new_board = move<piece, state.white_move, capture>(board, from, to);
            u32 key = new_board.hash & TT::key_mask;
            if (TT::table[key].pos_hash == (new_board.hash) && TT::table[key].depth == depth){
                partial_nodecount += TT::table[key].nodecount;
                ++tt_hits;
                // std::cout << "\t" << Misc::uci_move<PIECE_INVALID>(SquareOf(from), SquareOf(to)) << ": " << TT::table[key].nodecount << "t" << std::endl;
            }
            else {
                vars new_var = {u8(depth - 1), 0};
                enumerate<state.silent_move(), false, Perft>(new_board, new_var, 0);
                partial_nodecount += new_var.partial_nodecount;
                if constexpr (root) display_move_stats<PIECE_INVALID>(from, to, new_var.partial_nodecount);
                // else std::cout << "\t" << Misc::uci_move<PIECE_INVALID>(SquareOf(from), SquareOf(to)) << ": " << new_var.partial_nodecount << "r" << std::endl;
                if (depth > TT::table[key].depth) TT::table[key] = {(new_board.hash), new_var.partial_nodecount, depth };
            }
        }

        template<State state, bool capture, bool root>
        static FORCEINLINE void _rookmove(const Board& board, Bit from, Bit to, u8& depth, u32& partial_nodecount){
            constexpr bool us = state.white_move;
            const Board new_board = move<ROOK, us, capture>(board, from, to);
            u32 key = new_board.hash & TT::key_mask;
            if (TT::table[key].pos_hash == (new_board.hash) && TT::table[key].depth == depth){
                partial_nodecount += TT::table[key].nodecount;
                ++tt_hits;
            }
            else {
                vars new_var = {u8(depth - 1), 0};
                [&]() {
                    if constexpr (state.can_oo<us>()) {
                        if (state.is_rook_right<us>(from)) {
                            enumerate<state.no_oo<us>(), false, Perft>(new_board, new_var, 0);
                            return;
                        }
                    } else if constexpr (state.can_ooo<us>()) {
                        if (state.is_rook_left<us>(from)) {
                            enumerate<state.no_ooo<us>(), false, Perft>(new_board, new_var, 0);
                            return;
                        }
                    }
                    enumerate<state.silent_move(), false, Perft>(new_board, new_var, 0);
                }();
                partial_nodecount += new_var.partial_nodecount;
                if constexpr (root) display_move_stats<PIECE_INVALID>(from, to, new_var.partial_nodecount);
                if (depth > TT::table[key].depth) TT::table[key] = {(new_board.hash), new_var.partial_nodecount, depth};
            }
        }

        template<State state, bool root>
        static FORCEINLINE void _ep_move(const Board& board, Bit& from, Bit to, Bit& ep_target, u8& depth, u32& partial_nodecount){
            const Board new_board = Boardstate::ep_move<state.white_move>(board, from | to, ep_target);
            u32 key = new_board.hash & TT::key_mask;
            if (TT::table[key].pos_hash == (new_board.hash) && TT::table[key].depth == depth){
                partial_nodecount += TT::table[key].nodecount;
                ++tt_hits;
            }
            else {
                vars new_var = {u8(depth - 1), 0};
                enumerate<state.silent_move(), false, Perft>(new_board, new_var, 0);
                partial_nodecount += new_var.partial_nodecount;
                if constexpr (root) display_move_stats<PIECE_INVALID>(from, to, new_var.partial_nodecount);
                if (depth > TT::table[key].depth) TT::table[key] = {(new_board.hash), new_var.partial_nodecount, depth};
            }
        }

        template<State state, bool root>
        static FORCEINLINE void _pawn_push(const Board& board, Bit from, Bit to, u8& depth, u32& partial_nodecount){
            const Board new_board = move<PAWN, state.white_move, false>(board, from, to);
            u32 key = new_board.hash & TT::key_mask;
            if (TT::table[key].pos_hash == (new_board.hash) && TT::table[key].depth == depth){
                partial_nodecount += TT::table[key].nodecount;
                ++tt_hits;
            }
            else {
                vars new_var = {u8(depth - 1), 0};
                enumerate<state.new_ep_pawn(), false, Perft>(new_board, new_var, to);
                partial_nodecount += new_var.partial_nodecount;
                if constexpr (root) display_move_stats<PIECE_INVALID>(from, to, new_var.partial_nodecount);
                if (depth > TT::table[key].depth) TT::table[key] = {(new_board.hash), new_var.partial_nodecount, depth};
            }
        }

        template<State state, Piece promotion_to, bool capture, bool root>
        static FORCEINLINE void _promotion_move(const Board& board, Bit& from, Bit& to, u8& depth, u32& partial_nodecount){
            const Board new_board = promotion_move<promotion_to, state.white_move, capture>(board, from, to);
            u32 key = new_board.hash & TT::key_mask;
            if (TT::table[key].pos_hash == (new_board.hash) && TT::table[key].depth == depth){
                partial_nodecount += TT::table[key].nodecount;
                ++tt_hits;
            }
            else {
                vars new_var = {u8(depth - 1), 0};
                enumerate<state.silent_move(), false, Perft>(new_board, new_var, 0);
                partial_nodecount += new_var.partial_nodecount;
                if constexpr (root) display_move_stats<promotion_to>(from, to, new_var.partial_nodecount);
                if (depth > TT::table[key].depth) TT::table[key] = {(new_board.hash), new_var.partial_nodecount, depth};
            }
        }

    public:

        template<State state>
        static void go(const Board& board, u8 depth, Bit ep_target){
            init(depth);
            vars var = {depth};
            enumerate<state, true, Perft>(board, var, ep_target);
            display_info();
        }

        template<State state, bool root, bool leaf>
        static FORCEINLINE void kingmove(const Board& board, map& moves, map& captures, vars& vars){
            constexpr bool us = state.white_move;
            if constexpr (leaf) count(moves | captures, vars.partial_nodecount);
            else {
                Bit king = King<us>(board);
                Bitloop(moves)
                    _kingmove<state, false, root>(board, king, 1ull << SquareOf(moves), vars.depth, vars.partial_nodecount);
                Bitloop(captures)
                    _kingmove<state, true, root>(board, king, 1ull << SquareOf(captures), vars.depth, vars.partial_nodecount);
            }
        }

        template<State state, bool root, bool leaf>
        static FORCEINLINE void ep_move(const Board& board, Bit& lep, Bit& rep, Bit& ep_target, vars& vars){
            constexpr bool us = state.white_move;
            if constexpr (leaf) {
                if (lep) increment(vars.partial_nodecount);
                if (rep) increment(vars.partial_nodecount);
            }
            else {
                if (lep) _ep_move<state, root>(board, lep, pawn_atk_left<us>(lep), ep_target, vars.depth, vars.partial_nodecount);
                if (rep) _ep_move<state, root>(board, rep, pawn_atk_right<us>(rep), ep_target, vars.depth, vars.partial_nodecount);
            }
        }

        template<State state, bool root, bool leaf>
        static FORCEINLINE void pawn_move(const Board& board, map& pr, map& pl, map& pr_promo, map& pl_promo, map& pf, map& pp, map& pf_promo, vars& vars){
            if constexpr (leaf){
                count(pr, vars.partial_nodecount);
                count(pl, vars.partial_nodecount);
                count_promo(pr_promo, vars.partial_nodecount);
                count_promo(pl_promo, vars.partial_nodecount);
                count(pf | pp, vars.partial_nodecount);
                count_promo(pf_promo, vars.partial_nodecount);
            }
            else{
                constexpr bool us = state.white_move;
                Bitloop(pr){
                    Square sq = SquareOf(pr);
                    _silent_move<state, true, PAWN, root>(board, 1ull << (sq + pawn_shift[0][us]), 1ull << sq, vars.depth, vars.partial_nodecount);
                }
                Bitloop(pl){
                    Square sq = SquareOf(pl);
                    _silent_move<state, true, PAWN, root>(board, 1ull << (sq + pawn_shift[1][us]), 1ull << sq, vars.depth, vars.partial_nodecount);
                }
                Bitloop(pr_promo){
                    Square sq = SquareOf(pr_promo); Bit from = 1ull << (sq + pawn_shift[0][us]), to = 1ull << sq;
                    _promotion_move<state, QUEEN, true, root>(board, from, to, vars.depth, vars.partial_nodecount);
                    _promotion_move<state, ROOK, true, root>(board, from, to, vars.depth, vars.partial_nodecount);
                    _promotion_move<state, BISHOP, true, root>(board, from, to, vars.depth, vars.partial_nodecount);
                    _promotion_move<state, KNIGHT, true, root>(board, from, to, vars.depth, vars.partial_nodecount);
                }
                Bitloop(pl_promo){
                    Square sq = SquareOf(pl_promo); Bit from = 1ull << (sq + pawn_shift[1][us]), to = 1ull << sq;
                    _promotion_move<state, QUEEN, true, root>(board, from, to, vars.depth, vars.partial_nodecount);
                    _promotion_move<state, ROOK, true, root>(board, from, to, vars.depth, vars.partial_nodecount);
                    _promotion_move<state, BISHOP, true, root>(board, from, to, vars.depth, vars.partial_nodecount);
                    _promotion_move<state, KNIGHT, true, root>(board, from, to, vars.depth, vars.partial_nodecount);
                }
                Bitloop(pf){
                    Square sq = SquareOf(pf);
                    _silent_move<state, false, PAWN, root>(board, 1ull << (sq + sign<us>(-8)), 1ull << sq, vars.depth, vars.partial_nodecount);
                }
                Bitloop(pf_promo){
                    Square sq = SquareOf(pf_promo); Bit from = 1ull << (sq + sign<us>(-8)), to = 1ull << sq;
                    _promotion_move<state, QUEEN, false, root>(board, from, to, vars.depth, vars.partial_nodecount);
                    _promotion_move<state, ROOK, false, root>(board, from, to, vars.depth, vars.partial_nodecount);
                    _promotion_move<state, BISHOP, false, root>(board, from, to, vars.depth, vars.partial_nodecount);
                    _promotion_move<state, KNIGHT, false, root>(board, from, to, vars.depth, vars.partial_nodecount);
                }
                Bitloop(pp){
                    Square sq = SquareOf(pp);
                    _pawn_push<state, root>(board, 1ull << (sq + sign<us>(-16)), 1ull << sq, vars.depth, vars.partial_nodecount);
                }
            }
        }

        template<State state, Piece piece, bool root, bool leaf>
        static FORCEINLINE void silent_move(const Board& board, map& moves, map& captures, const Square& sq, vars& vars){
            if constexpr (leaf)
                count(moves | captures, vars.partial_nodecount);
            else{
                Bitloop(moves)
                    _silent_move<state, false, piece, root>(board, 1ull << sq, 1ull << SquareOf(moves), vars.depth, vars.partial_nodecount);
                Bitloop(captures)
                    _silent_move<state, true, piece, root>(board, 1ull << sq, 1ull << SquareOf(captures), vars.depth, vars.partial_nodecount);

            }
        }
        template<State state, bool root, bool leaf>
        static FORCEINLINE void rookmove(const Board& board, map& moves, map& captures, const Square& sq, vars& vars){
            if constexpr (leaf)
                count(moves | captures, vars.partial_nodecount);
            else{
                Bitloop(moves)
                    _rookmove<state, false, root>(board, 1ull << sq, 1ull << SquareOf(moves), vars.depth, vars.partial_nodecount);
                Bitloop(captures)
                    _rookmove<state, true, root>(board, 1ull << sq, 1ull << SquareOf(captures), vars.depth, vars.partial_nodecount);

            }
        }

        template<State state, bool left, bool root, bool leaf>
        static FORCEINLINE void castlemove(const Board& board, vars& vars){
            if constexpr (leaf) increment(vars.partial_nodecount);
            else {
                if constexpr (left) {
                    if constexpr (state.white_move) _castlemove<state, WHITE_OOO, root>(board, vars.depth, vars.partial_nodecount);
                    else                            _castlemove<state, BLACK_OOO, root>(board, vars.depth, vars.partial_nodecount);
                }
                else {
                    if constexpr (state.white_move) _castlemove<state, WHITE_OO, root>(board, vars.depth, vars.partial_nodecount);
                    else                            _castlemove<state, BLACK_OO, root>(board, vars.depth, vars.partial_nodecount);
                }
            }
        }
    };
}