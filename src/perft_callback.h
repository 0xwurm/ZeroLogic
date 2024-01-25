#pragma once

namespace ZeroLogic::Perft {
    using namespace Movegen;

    class Callback {

    public:

        static inline u32 tt_hits;
        static inline bool depth1;
        static inline bool test;
        static inline u64 overall_nodecount;
        static inline u8 full_depth;

        template<Color c>
        using specific = u64;

    private:

        static void init(){
            TT::init();
            depth1 = limits.allowed_depth == 1;
            if (depth1) limits.allowed_depth++;
            tt_hits = 0;
            full_depth = limits.allowed_depth - 1;
            overall_nodecount = 0;
            limits.start_time = Time::now();
        }

        static inline void count(map moves, u64& partial_nodecount){
            partial_nodecount += BitCount(moves);
        }
        static inline void count_promo(map moves, u64& partial_nodecount){
            partial_nodecount += 4*BitCount(moves);
        }
        static inline void increment(u64& partial_nodecount){
            ++partial_nodecount;
        }

        template<Piece promotion_to = PIECE_NONE>
        static inline void display_move_stats(Bit from, Bit to, u64& partial_nodecount){
            if (depth1) partial_nodecount = 1;
            overall_nodecount += partial_nodecount;
            if (!test) std::cout << Misc::_uci_move<promotion_to>(SquareOf(from), SquareOf(to)) << ": " << Misc::format(partial_nodecount) << std::endl;
            partial_nodecount = 0;
        }

        template<CastleType type>
        static inline void display_move_stats(u64& partial_nodecount){
            if (depth1) partial_nodecount = 1;
            overall_nodecount += partial_nodecount;
            if (!test) std::cout << Misc::_uci_move<type>() << ": " << Misc::format(partial_nodecount) << std::endl;
            partial_nodecount = 0;
        }

        static void display_info(){
            u64 duration_mys = Time::now() - limits.start_time;
            u64 duration_ms = duration_mys / 1000;
            double sPmn = double(duration_mys) / double(overall_nodecount);

            std::cout << std::endl;
            std::cout << "Overall nodes: "              << Misc::format(overall_nodecount)        << "\n";
            std::cout << "Time taken: "                 << duration_ms                               << "\n";
            std::cout << "Mn/s: "                       << overall_nodecount / duration_mys          << "\n";
            std::cout << "s/Mn: "                       << sPmn                                      << "\n";
            std::cout << "light distance equivalent: "  << 299792458 * 10e-7 * sPmn << " meters"     << "\n";
            std::cout << "tt hits: "                    << Misc::format(tt_hits)                  << "\n";
            std::cout << std::endl;
        }

        template <Color c>
        static void _any_move(Position<c>& npos, const u8& depth, u64& partial_nodecount){
            u32 key = *npos.hash;
            if (TT::table[key].hash == npos.hash && TT::table[key].depth == depth)
            {
                partial_nodecount += TT::table[key].nodecount;
                ++tt_hits;
            }
            else
            {
                u64 new_partial_nodecount{0};
                u8 new_depth{u8(depth + 1)};
                if (new_depth == full_depth)    enumerate<Callback, true>(npos, new_depth, new_partial_nodecount);
                else                            enumerate<Callback>(npos, new_depth, new_partial_nodecount);

                partial_nodecount += new_partial_nodecount;
                if (depth < TT::table[key].depth) TT::table[key] = {npos.hash, u32(new_partial_nodecount), depth};
            }
        }

        template<bool root, Color c>
        static inline void _kingmove(Position<c>& pos, Bit to, const u8& depth, u64& partial_nodecount){
            Position<!c> npos = pos.template move_silent<KING>(pos.oKing, to);
            _any_move(npos, depth, partial_nodecount);
            if constexpr (root) display_move_stats(pos.oKing, to, partial_nodecount);
        }

        template<CastleType ct, bool root, Color c>
        static inline void _castlemove(Position<c>& pos, const u8& depth, u64& partial_nodecount){
            Position<!c> npos = pos.template move_castles<ct>();
            _any_move(npos, depth, partial_nodecount);
            if constexpr (root) display_move_stats<(c >> ct)>(partial_nodecount);
        }

        template<Piece piece, bool root, Color c>
        static inline void _silent_move(Position<c>& pos, Bit from, Bit to, const u8& depth, u64& partial_nodecount){
            Position<!c> npos = pos.template move_silent<piece>(from, to);
            _any_move(npos, depth, partial_nodecount);
            if constexpr (root) display_move_stats(from, to, partial_nodecount);
        }

        template<CastleType ct, Color c>
        static inline bool isLoud(Position<c>& pos, Bit from)
        {
            return pos.cs[ct] && (from & pos.template rm<(c >> ct)>());
        }

        template<bool root, Color c>
        static inline void _rookmove(Position<c>& pos, Bit from, Bit to, const u8& depth, u64& partial_nodecount){

            if (isLoud<SHORT>(pos, from))
            {
                Position<!c> npos = pos.template move_loudRook<SHORT>(from, to);
                _any_move(npos, depth, partial_nodecount);
            }
            else if (isLoud<LONG>(pos, from))
            {
                Position<!c> npos = pos.template move_loudRook<LONG>(from, to);
                _any_move(npos, depth, partial_nodecount);
            }
            else
            {
                Position<!c> npos = pos.template move_silent<ROOK>(from, to);
                _any_move(npos, depth, partial_nodecount);
            }
            if constexpr (root) display_move_stats(from, to, partial_nodecount);
        }

        template<bool root, Color c>
        static inline void _ep_move(Position<c>& pos, Bit from, Bit to, const u8& depth, u64& partial_nodecount){
            Position<!c> npos = pos.move_ep(from, to);
            _any_move(npos, depth, partial_nodecount);
            if constexpr (root) display_move_stats(from, to, partial_nodecount);
        }

        template<bool root, Color c>
        static inline void _pawn_push(Position<c>& pos, Bit from, Bit to, const u8& depth, u64& partial_nodecount){
            Position<!c> npos = pos.move_pp(from, to);
            _any_move(npos, depth, partial_nodecount);
            if constexpr (root) display_move_stats(from, to, partial_nodecount);
        }

        template<bool root, Color c>
        static inline void _promotion_move(Position<c>& pos, Bit from, Bit& to, const u8& depth, u64& partial_nodecount){
            Position<!c> npos1 = pos.template move_silent<PAWN, QUEEN>(from, to);
            Position<!c> npos2 = pos.template move_silent<PAWN, KNIGHT>(from, to);
            Position<!c> npos3 = pos.template move_silent<PAWN, ROOK>(from, to);
            Position<!c> npos4 = pos.template move_silent<PAWN, BISHOP>(from, to);
            _any_move(npos1, depth, partial_nodecount);
            if constexpr (root) display_move_stats<QUEEN>(from, to, partial_nodecount);
            _any_move(npos2, depth, partial_nodecount);
            if constexpr (root) display_move_stats<KNIGHT>(from, to, partial_nodecount);
            _any_move(npos3, depth, partial_nodecount);
            if constexpr (root) display_move_stats<ROOK>(from, to, partial_nodecount);
            _any_move(npos4, depth, partial_nodecount);
            if constexpr (root) display_move_stats<BISHOP>(from, to, partial_nodecount);
        }

    public:

        template<bool istest = false, Color c>
        requires (c != NONE)
        static auto go(Position<c>& pos){
            init();
            specific<c> nodecount = 0;
            u8 start_depth = 0;
            test = istest;
            enumerate<Callback, false, true>(pos, start_depth, nodecount);
            TT::clear();
            if constexpr (istest) return overall_nodecount;
            display_info();
        }

        template<bool root, bool leaf, Color c>
        static inline void ep_move(Position<c>& pos, Bit& lep, Bit& rep, const u8 depth, u64& partial_nodecount){
            if constexpr (leaf)
            {
                if (lep) increment(partial_nodecount);
                if (rep) increment(partial_nodecount);
            }
            else
            {
                if (lep) _ep_move<root>(pos, lep, moveP<c, LEFT>(lep), depth, partial_nodecount);
                if (rep) _ep_move<root>(pos, rep, moveP<c, RIGHT>(rep), depth, partial_nodecount);
            }
        }

        template<bool root, bool leaf, Color c>
        static inline void pawn_move(
                Position<c>& pos,
                map& pr, map& pl, map& pr_promo, map& pl_promo, map& pf, map& pp, map& pf_promo,
                const u8 depth, u64& partial_nodecount
                )
        {
            if constexpr (leaf)
            {
                count(pr | pf | pp, partial_nodecount);
                count(pl, partial_nodecount);
                count_promo(pr_promo | pf_promo, partial_nodecount);
                count_promo(pl_promo, partial_nodecount);
            }
            else
            {
                while(pr){
                    Bit to = PopBit(pr);
                    _silent_move<PAWN, root>(pos, moveP<!c, RIGHT>(to), to, depth, partial_nodecount);
                }
                while(pl){
                    Bit to = PopBit(pl);
                    _silent_move<PAWN, root>(pos, moveP<!c, LEFT>(to), to, depth, partial_nodecount);
                }
                while(pr_promo){
                    Bit to = PopBit(pr_promo);
                    _promotion_move<root>(pos, moveP<!c, RIGHT>(to), to, depth, partial_nodecount);
                }
                while(pl_promo){
                    Bit to = PopBit(pl_promo);
                    _promotion_move<root>(pos, moveP<!c, LEFT>(to), to, depth, partial_nodecount);
                }
                while(pf){
                    Bit to = PopBit(pf);
                    _silent_move<PAWN, root>(pos, moveP<!c, FORWARD>(to), to, depth, partial_nodecount);
                }
                while(pf_promo){
                    Bit to = PopBit(pf_promo);
                    _promotion_move<root>(pos, moveP<!c, FORWARD>(to), to, depth, partial_nodecount);
                }
                while(pp){
                    Bit to = PopBit(pp);
                    _pawn_push<root>(pos, move<(!c >> FORWARD), 2>(to), to, depth, partial_nodecount);
                }
            }
        }

        template<Piece piece, bool root, bool leaf, Color c>
        static inline void silent_move(Position<c>& pos, map& moves, Bit from, const u8 depth, u64& partial_nodecount){
            if constexpr (leaf) count(moves, partial_nodecount);
            else
                while(moves)
                {
                    _silent_move<piece, root>(pos, from, PopBit(moves), depth, partial_nodecount);
                }
        }

        template<bool root, bool leaf, Color c>
        static inline void rookmove(Position<c>& pos, map& moves, Bit& from, const u8 depth, u64& partial_nodecount){
            if constexpr (leaf) count(moves, partial_nodecount);
            else
                while(moves)
                {
                    _rookmove<root>(pos, from, PopBit(moves), depth, partial_nodecount);
                }
        }

        template<CastleType ct, bool root, bool leaf, Color c>
        static inline void castlemove(Position<c>& pos, const u8 depth, u64& partial_nodecount){
            if constexpr (leaf) increment(partial_nodecount);
            else _castlemove<ct, root>(pos, depth, partial_nodecount);
        }

        template<bool root, bool leaf, Color c>
        static inline void kingmove(Position<c>& pos, map& moves, const u8 depth, u64& partial_nodecount){
            if constexpr (leaf) count(moves, partial_nodecount);
            else
                while(moves)
                {
                    Bit to = PopBit(moves);
                    _kingmove<root>(pos, to, depth, partial_nodecount);
                }
        }
    };
}