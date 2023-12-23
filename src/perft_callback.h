#pragma once

namespace ZeroLogic::Perft {
    using namespace Boardstate;
    using namespace Movegen;

    class Callback {

    public:

        static inline u32 tt_hits;
        static inline std::chrono::steady_clock::time_point start_time;
        static inline bool depth1;
        static inline u64 overall_nodecount;
        static inline u8 full_depth;

        using specific = u64;

    private:

        static void init(u8& depth){
            depth1 = depth == 1;
            if (depth == 1) depth++;
            tt_hits = 0;
            full_depth = depth - 1;
            overall_nodecount = 0;
            start_time = std::chrono::steady_clock::now();
        }

        static FORCEINLINE void count(map moves, u64& partial_nodecount){
            partial_nodecount += BitCount(moves);
        }
        static FORCEINLINE void count_promo(map moves, u64& partial_nodecount){
            partial_nodecount += 4*BitCount(moves);
        }
        static FORCEINLINE void increment(u64& partial_nodecount){
            ++partial_nodecount;
        }

        template<Piece promotion_to>
        static FORCEINLINE void display_move_stats(Bit from, Bit to, u64& partial_nodecount){
            if (depth1) partial_nodecount = 1;
            overall_nodecount += partial_nodecount;
            std::cout << Misc::_uci_move<promotion_to>(SquareOf(from), SquareOf(to)) << ": " << partial_nodecount << std::endl;
            partial_nodecount = 0;
        }

        template<CastleType type>
        static FORCEINLINE void display_move_stats(u64& partial_nodecount){
            if (depth1) partial_nodecount = 1;
            overall_nodecount += partial_nodecount;
            std::cout << Misc::_uci_move<type>() << ": " << partial_nodecount << std::endl;
            partial_nodecount = 0;
        }

        static void display_info(){
            auto duration = std::chrono::steady_clock::now() - start_time;
            auto duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(duration);
            auto duration_mys = std::chrono::duration_cast<std::chrono::microseconds>(duration).count();
            double sPmn = double(duration_mys) / double(overall_nodecount);

            std::cout << std::endl;
            std::cout << "Overall nodes: "              << overall_nodecount                         << std::endl;
            std::cout << "Time taken: "                 << duration_ms                               << std::endl;
            std::cout << "Mn/s: "                       << overall_nodecount / duration_mys          << std::endl;
            std::cout << "s/Mn: "                       << sPmn                                      << std::endl;
            std::cout << "light distance equivalent: "  << 299792458 * 10e-7 * sPmn << " meters"     << std::endl;
            std::cout << "tt hits: "                    << tt_hits                                   << std::endl;
            std::cout << std::endl;
        }

#define ep_hash ZeroLogic::TT::keys[ZeroLogic::TT::ep0 + (SquareOf(ep_target) % 8)]

        template <State new_state, bool Nep_target>
        static void _any_move(const Board& new_board, const u8& depth, u64& partial_nodecount, Bit new_ep_target){
            u32 key = new_board.hash & TT::key_mask;
            if ((TT::table[key].hash == new_board.hash) && (TT::table[key].depth == depth)){
                partial_nodecount += TT::table[key].nodecount;
                ++tt_hits;
            }
            else {
                u64 new_partial_nodecount{0};
                u8 new_depth{u8(depth + 1)};
                if constexpr (Nep_target)
                    if (new_depth == full_depth)    enumerate<new_state, false, true, Callback>(new_board, new_ep_target, new_depth, new_partial_nodecount);
                    else                            enumerate<new_state, false, false, Callback>(new_board, new_ep_target, new_depth, new_partial_nodecount);
                else {
                    Bit nep = 0;
                    if (new_depth == full_depth)    enumerate<new_state, false, true, Callback>(new_board, nep, new_depth, new_partial_nodecount);
                    else                            enumerate<new_state, false, false, Callback>(new_board, nep, new_depth, new_partial_nodecount);
                }
                partial_nodecount += new_partial_nodecount;
                if (depth < TT::table[key].depth) TT::table[key] = {new_board.hash, u32(new_partial_nodecount), depth};
            }
        }

        template<State state, bool capture, bool root>
        static FORCEINLINE void _kingmove(const Board& board, Bit& from, Bit to, const Bit& ep_target, const u8& depth, u64& partial_nodecount){
            const Board new_board = move<KING, state.white_move, capture, state.has_ep_pawn, PIECE_INVALID>(board, from, to, ep_hash);
            _any_move<state.no_castles(), false>(new_board, depth, partial_nodecount, 0);
            if constexpr (root) display_move_stats<PIECE_INVALID>(from, to, partial_nodecount);
        }

        template<State state, CastleType type, bool root>
        static FORCEINLINE void _castlemove(const Board& board, const Bit& ep_target, const u8& depth, u64& partial_nodecount){
            const Board new_board = castle_move<type, state.has_ep_pawn>(board, ep_hash);
            _any_move<state.no_castles(), false>(new_board, depth, partial_nodecount, 0);
            if constexpr (root) display_move_stats<type>(partial_nodecount);
        }

        template<State state, bool capture, Piece piece, bool root>
        static FORCEINLINE void _silent_move(const Board& board, Bit from, Bit to, const Bit& ep_target, const u8& depth, u64& partial_nodecount){
            const Board new_board = move<piece, state.white_move, capture, state.has_ep_pawn, PIECE_INVALID>(board, from, to, ep_hash);
            _any_move<state.silent_move(), false>(new_board, depth, partial_nodecount, 0);
            if constexpr (root) display_move_stats<PIECE_INVALID>(from, to, partial_nodecount);
        }

        template<State state, bool capture, bool root>
        static FORCEINLINE void _rookmove(const Board& board, Bit from, Bit to, const Bit& ep_target, const u8& depth, u64& partial_nodecount){
            constexpr bool us = state.white_move;
            const Board new_board = move<ROOK, us, capture, state.has_ep_pawn, PIECE_INVALID>(board, from, to, ep_hash);
            [&]() {
                if constexpr (state.can_oo<us>()) {
                    if (state.is_rook_right<us>(from)) {
                        _any_move<state.no_oo(), false>(new_board, depth, partial_nodecount, 0);
                        return;
                    }
                }
                else if constexpr (state.can_ooo<us>()) {
                    if (state.is_rook_left<us>(from)) {
                        _any_move<state.no_ooo(), false>(new_board, depth, partial_nodecount, 0);
                        return;
                    }
                }
                _any_move<state.silent_move(), false>(new_board, depth, partial_nodecount, 0);
            }();
            if constexpr (root) display_move_stats<PIECE_INVALID>(from, to, partial_nodecount);
        }

        template<State state, bool root>
        static FORCEINLINE void _ep_move(const Board& board, Bit& from, Bit to, const Bit& ep_target, const u8& depth, u64& partial_nodecount){
            const Board new_board = Boardstate::ep_move<state.white_move>(board, from | to, ep_target);
            _any_move<state.silent_move(), false>(new_board, depth, partial_nodecount, 0);
            if constexpr (root) display_move_stats<PIECE_INVALID>(from, to, partial_nodecount);
        }

        template<State state, bool root>
        static FORCEINLINE void _pawn_push(const Board& board, Bit from, Bit to, const Bit& ep_target, const u8& depth, u64& partial_nodecount){
            map ephash = ZeroLogic::TT::keys[ZeroLogic::TT::ep0 + (SquareOf(to) % 8)];
            if constexpr (state.has_ep_pawn) ephash ^= ZeroLogic::TT::keys[ZeroLogic::TT::ep0 + (SquareOf(ep_target) % 8)];
            const Board new_board = move<PAWN, state.white_move, false, true, PIECE_INVALID>(board, from, to, ephash);
            _any_move<state.new_ep_pawn(), true>(new_board, depth, partial_nodecount, to);
            if constexpr (root) display_move_stats<PIECE_INVALID>(from, to, partial_nodecount);
        }

        template<State state, Piece promotion_to, bool capture, bool root>
        static FORCEINLINE void _promotion_move(const Board& board, Bit& from, Bit& to, const Bit& ep_target, const u8& depth, u64& partial_nodecount){
            const Board new_board = move<PAWN, state.white_move, capture, state.has_ep_pawn, promotion_to>(board, from, to, ep_hash);
            _any_move<state.silent_move(), false>(new_board, depth, partial_nodecount, 0);
            if constexpr (root) display_move_stats<promotion_to>(from, to, partial_nodecount);
        }

#undef ep_hash

    public:

        template<State state>
        static void go(const Board& board, u8 depth, Bit ep_target){
            init(depth);
            u64 nodecount = 0;
            u8 start_depth = 0;
            enumerate<state, true, false, Callback>(board, ep_target, start_depth, nodecount);
            display_info();
        }

        template<State state, bool root, bool leaf>
        static FORCEINLINE void kingmove(const Board& board, map& moves, map& captures, const Bit& ep_target, const u8& depth, u64& partial_nodecount){
            constexpr bool us = state.white_move;
            if constexpr (leaf) count(moves | captures, partial_nodecount);
            else {
                Bit king = King<us>(board);
                Bitloop(moves)
                    _kingmove<state, false, root>(board, king, 1ull << SquareOf(moves), ep_target, depth, partial_nodecount);
                Bitloop(captures)
                    _kingmove<state, true, root>(board, king, 1ull << SquareOf(captures), ep_target, depth, partial_nodecount);
            }
        }

        template<State state, bool root, bool leaf>
        static FORCEINLINE void ep_move(const Board& board, Bit& lep, Bit& rep, const Bit& ep_target, const u8& depth, u64& partial_nodecount){
            constexpr bool us = state.white_move;
            if constexpr (leaf) {
                if (lep) increment(partial_nodecount);
                if (rep) increment(partial_nodecount);
            }
            else {
                if (lep) _ep_move<state, root>(board, lep, pawn_atk_left<us>(lep), ep_target, depth, partial_nodecount);
                if (rep) _ep_move<state, root>(board, rep, pawn_atk_right<us>(rep), ep_target, depth, partial_nodecount);
            }
        }

        template<State state, bool root, bool leaf>
        static FORCEINLINE void pawn_move(const Board& board, map& pr, map& pl, map& pr_promo, map& pl_promo, map& pf, map& pp, map& pf_promo, const Bit& ep_target, const u8& depth, u64& partial_nodecount){
            if constexpr (leaf){
                count(pr, partial_nodecount);
                count(pl, partial_nodecount);
                count_promo(pr_promo, partial_nodecount);
                count_promo(pl_promo, partial_nodecount);
                count(pf | pp, partial_nodecount);
                count_promo(pf_promo, partial_nodecount);
            }
            else{
                constexpr bool us = state.white_move;
                Bitloop(pr){
                    Square sq = SquareOf(pr);
                    _silent_move<state, true, PAWN, root>(board, 1ull << (sq + pawn_shift[0][us]), 1ull << sq, ep_target, depth, partial_nodecount);
                }
                Bitloop(pl){
                    Square sq = SquareOf(pl);
                    _silent_move<state, true, PAWN, root>(board, 1ull << (sq + pawn_shift[1][us]), 1ull << sq, ep_target, depth, partial_nodecount);
                }
                Bitloop(pr_promo){
                    Square sq = SquareOf(pr_promo); Bit from = 1ull << (sq + pawn_shift[0][us]), to = 1ull << sq;
                    _promotion_move<state, QUEEN, true, root>(board, from, to, ep_target, depth, partial_nodecount);
                    _promotion_move<state, ROOK, true, root>(board, from, to, ep_target, depth, partial_nodecount);
                    _promotion_move<state, BISHOP, true, root>(board, from, to, ep_target, depth, partial_nodecount);
                    _promotion_move<state, KNIGHT, true, root>(board, from, to, ep_target, depth, partial_nodecount);
                }
                Bitloop(pl_promo){
                    Square sq = SquareOf(pl_promo); Bit from = 1ull << (sq + pawn_shift[1][us]), to = 1ull << sq;
                    _promotion_move<state, QUEEN, true, root>(board, from, to, ep_target, depth, partial_nodecount);
                    _promotion_move<state, ROOK, true, root>(board, from, to, ep_target, depth, partial_nodecount);
                    _promotion_move<state, BISHOP, true, root>(board, from, to, ep_target, depth, partial_nodecount);
                    _promotion_move<state, KNIGHT, true, root>(board, from, to, ep_target, depth, partial_nodecount);
                }
                Bitloop(pf){
                    Square sq = SquareOf(pf);
                    _silent_move<state, false, PAWN, root>(board, 1ull << (sq + sign<us>(-8)), 1ull << sq, ep_target, depth, partial_nodecount);
                }
                Bitloop(pf_promo){
                    Square sq = SquareOf(pf_promo); Bit from = 1ull << (sq + sign<us>(-8)), to = 1ull << sq;
                    _promotion_move<state, QUEEN, false, root>(board, from, to, ep_target, depth, partial_nodecount);
                    _promotion_move<state, ROOK, false, root>(board, from, to, ep_target, depth, partial_nodecount);
                    _promotion_move<state, BISHOP, false, root>(board, from, to, ep_target, depth, partial_nodecount);
                    _promotion_move<state, KNIGHT, false, root>(board, from, to, ep_target, depth, partial_nodecount);
                }
                Bitloop(pp){
                    Square sq = SquareOf(pp);
                    _pawn_push<state, root>(board, 1ull << (sq + sign<us>(-16)), 1ull << sq, ep_target, depth, partial_nodecount);
                }
            }
        }

        template<State state, Piece piece, bool root, bool leaf>
        static FORCEINLINE void silent_move(const Board& board, map& moves, map& captures, const Square& sq, const Bit& ep_target, const u8& depth, u64& partial_nodecount){
            if constexpr (leaf)
                count(moves | captures, partial_nodecount);
            else{
                Bitloop(moves)
                    _silent_move<state, false, piece, root>(board, 1ull << sq, 1ull << SquareOf(moves), ep_target, depth, partial_nodecount);
                Bitloop(captures)
                    _silent_move<state, true, piece, root>(board, 1ull << sq, 1ull << SquareOf(captures), ep_target, depth, partial_nodecount);

            }
        }
        template<State state, bool root, bool leaf>
        static FORCEINLINE void rookmove(const Board& board, map& moves, map& captures, const Square& sq, const Bit& ep_target, const u8& depth, u64& partial_nodecount){
            if constexpr (leaf)
                count(moves | captures, partial_nodecount);
            else{
                Bitloop(moves)
                    _rookmove<state, false, root>(board, 1ull << sq, 1ull << SquareOf(moves), ep_target, depth, partial_nodecount);
                Bitloop(captures)
                    _rookmove<state, true, root>(board, 1ull << sq, 1ull << SquareOf(captures), ep_target, depth, partial_nodecount);

            }
        }

        template<State state, bool left, bool root, bool leaf>
        static FORCEINLINE void castlemove(const Board& board, const Bit& ep_target, const u8& depth, u64& partial_nodecount){
            if constexpr (leaf) increment(partial_nodecount);
            else {
                if constexpr (left) {
                    if constexpr (state.white_move) _castlemove<state, WHITE_OOO, root>(board, ep_target, depth, partial_nodecount);
                    else                            _castlemove<state, BLACK_OOO, root>(board, ep_target, depth, partial_nodecount);
                }
                else {
                    if constexpr (state.white_move) _castlemove<state, WHITE_OO, root>(board, ep_target, depth, partial_nodecount);
                    else                            _castlemove<state, BLACK_OO, root>(board, ep_target, depth, partial_nodecount);
                }
            }
        }
    };
}