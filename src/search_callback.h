#pragma once
#include "eval.h"
#include "static.h"

namespace ZeroLogic::Search {

    template<Color c>
    struct RatedMove{
        Move move;
        Value static_rating;
    };

    class Callback{

    private:

        template<Piece p, Color c>
        static inline void _silent_move(Position<c>& pos, u16 to, u16 from, RatedMove<c>*& movelist){
            Move move = to << 10 | from;
            Value static_val = Static::rate<p>(pos, move, u8(to), 1ull << from, 1ull << to);
            *(movelist++) = {move, static_val};
        }
        template<Color c>
        static inline void _pawn_push(Position<c>& pos, u16 to, u16 from, RatedMove<c>*& movelist) {
            Move move = to << 10 | from | PP_FLAG;
            Value static_val = Static::rate<PAWN>(pos, move, u8(to), 1ull << from, 1ull << to);
            *(movelist++) = {move, static_val};
        }
        template<Color c>
        static inline void _ep_move(Position<c>& pos, u16 to, u16 from, RatedMove<c>*& movelist){
            Move move = to << 10 | from | EP_FLAG;
            Value static_val = Static::rate<PAWN>(pos, move, u8(to), 1ull << from, 1ull << to);
            *(movelist++) = {move, static_val};
        }
        template<SearchType st, Color c>
        static inline void _promotion_move(Position<c>& pos, u16 to, u16 from, RatedMove<c>*& movelist){
            Move move = to << 10 | from;
            Value static_val = Static::rate<PAWN, true>(pos, u8(to), move, 1ull << from, 1ull << to);
            *(movelist++) = {Move(move | QP_FLAG), static_val};

            if constexpr (st == NSearch)
            {
                *(movelist++) = {Move(move | NP_FLAG), NON_CAPTURE};
                *(movelist++) = {Move(move | RP_FLAG), NON_CAPTURE};
                *(movelist++) = {Move(move | BP_FLAG), NON_CAPTURE};
            }
        }
        template<CastleType ct, Color c>
        static inline void _castlemove(Position<c>& pos, RatedMove<c>*& movelist){
            Move move = castlemoves[c >> ct];
            Value static_val = Static::rate(pos, move);
            *(movelist++) = {move, static_val};
        }

    public:

        template<Color c>
        using specific = RatedMove<c>*;

        template<Piece piece, bool st, bool check, Color c>
        static inline void silent_move(Position<c>& pos, map& moves, Bit fromBit, const u8 depth, RatedMove<c>*& movelist){
            Square from = SquareOf(fromBit);
            if constexpr (st == QSearch && !check) moves &= pos.enemy;
            Bitloop(moves)
            {
                _silent_move<piece>(pos, SquareOf(moves), from, movelist);
            }
        }

        template<bool st, bool check, Color c>
        static inline void ep_move(Position<c>& pos, Bit& lep, Bit& rep, const u8 depth, RatedMove<c>*& movelist){
            if constexpr (st == QSearch && !check) return;

            if (lep) _ep_move(pos, SquareOf((moveP<c, LEFT>(lep))), SquareOf(lep), movelist);
            if (rep) _ep_move(pos, SquareOf((moveP<c, RIGHT>(rep))), SquareOf(rep), movelist);
        }

        template<bool st, bool check, Color c>
        static inline void pawn_move(
                Position<c>& pos,
                map& pr, map& pl, map& pf, map& pp,
                Depth d, RatedMove<c>*& movelist
                )
        {
            Bitloop(pr) _silent_move<PAWN>(pos, SquareOf(pr), SquareOf((moveP<!c, RIGHT>(pr))), movelist);
            Bitloop(pl) _silent_move<PAWN>(pos, SquareOf(pl), SquareOf((moveP<!c, LEFT>(pl))), movelist);

            if constexpr (st == NSearch || check)
            {
                Bitloop(pf) _silent_move<PAWN>(pos, SquareOf(pf), SquareOf((moveP<!c, FORWARD>(pf))),movelist);
                Bitloop(pp) _pawn_push(pos, SquareOf(pp), SquareOf((move<(c >> SOUTH), 2>(pp))), movelist);
            }
        }
        template<bool st, bool check, Color c>
        static inline void promotion_move(
                Position<c>& pos,
                map& pr_promo, map& pl_promo, map& pf_promo,
                Depth d, RatedMove<c>*& movelist
                )
        {
            Bitloop(pr_promo) _promotion_move<SearchType(st)>(pos, SquareOf(pr_promo), SquareOf((moveP<!c, RIGHT>(pr_promo))), movelist);
            Bitloop(pl_promo) _promotion_move<SearchType(st)>(pos, SquareOf(pl_promo), SquareOf((moveP<!c, LEFT>(pl_promo))), movelist);
            Bitloop(pf_promo) _promotion_move<SearchType(st)>(pos, SquareOf(pf_promo), SquareOf((moveP<!c, FORWARD>(pf_promo))), movelist);
        }

        template<CastleType ct, bool st, bool check, Color c>
        static inline void castlemove(Position<c>& pos, const u8& depth, RatedMove<c>*& movelist){
            if (st == QSearch) return;

            _castlemove<ct>(pos, movelist);
        }

        template<bool st, bool check, Color c>
        static inline void rookmove(Position<c>& pos, map& moves, Bit& fromBit, const u8 depth, RatedMove<c>*& movelist){
            silent_move<ROOK, st, check>(pos, moves, fromBit, depth, movelist);
        }
        template<bool st, bool check, Color c>
        static inline void kingmove(Position<c>& pos, map& moves, const u8 depth, RatedMove<c>*& movelist){
            silent_move<KING, st, check>(pos, moves, pos.oKing, depth, movelist);
        }

    };
}
