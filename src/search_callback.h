#pragma once
#include "eval.h"
#include "static.h"

namespace ZeroLogic::Search {

    template<Color c>
    struct RatedMove{
        Move move;
        Value static_rating;

        Value (*callback)(Position<c>& pos, Move, const u8, Value, Value);
    };

    class Callback{

    private:

        template<Piece piece, SearchType st, Color c>
        static Value silent_callback(Position<c>&, Move, u8, Value, Value);
        template<SearchType st, Color c>
        static Value pp_callback(Position<c>&, Move, u8, Value, Value);
        template<SearchType st, Color c>
        static Value ep_callback(Position<c>&, Move, u8, Value, Value);
        template<Piece promoted, SearchType st, Color c>
        static Value promo_callback(Position<c>&, Move, u8, Value, Value);
        template<CastleType ct, Color c>
        static Value castles_callback(Position<c>&, Move, u8, Value, Value);

        template<Piece piece, SearchType st, Color c>
        static inline void _silent_move(Position<c>&, u16, u16, RatedMove<c>*&);
        template<SearchType st, Color c>
        static inline void _pawn_push(Position<c>&, u16, u16, RatedMove<c>*&);
        template<SearchType st, Color c>
        static inline void _ep_move(Position<c>&, u16, u16, RatedMove<c>*&);
        template<SearchType st, Color c>
        static inline void _promotion_move(Position<c>&, u16, u16, RatedMove<c>*&);
        template<CastleType ct, Color c>
        static inline void _castlemove(Position<c>&, RatedMove<c>*&);

    public:

        template<Color c>
        using specific = RatedMove<c>*;

        template<Piece piece, bool st, bool check, Color c>
        static inline void silent_move(Position<c>& pos, map& moves, Bit fromBit, const u8 depth, RatedMove<c>*& movelist){
            Square from = SquareOf(fromBit);
            if constexpr (st == QSearch && !check) moves &= pos.enemy;
            Bitloop(moves)
            {
                _silent_move<piece, SearchType(st)>(pos, SquareOf(moves), from, movelist);
            }
        }

        template<bool st, bool check, Color c>
        static inline void ep_move(Position<c>& pos, Bit& lep, Bit& rep, const u8 depth, RatedMove<c>*& movelist){
            if constexpr (st == QSearch && !check) return;

            if (lep) _ep_move<SearchType(st)>(pos, SquareOf((moveP<c, LEFT>(lep))), SquareOf(lep), movelist);
            if (rep) _ep_move<SearchType(st)>(pos, SquareOf((moveP<c, RIGHT>(rep))), SquareOf(rep), movelist);
        }

        template<bool st, bool check, Color c>
        static inline void pawn_move(
                Position<c>& pos,
                map& pr, map& pl, map& pr_promo, map& pl_promo, map& pf, map& pp, map& pf_promo,
                const u8 depth, RatedMove<c>*& movelist
                )
        {
            constexpr auto nst = SearchType(st);
            Bitloop(pr) _silent_move<PAWN, nst>(pos, SquareOf(pr), SquareOf((moveP<!c, RIGHT>(pr))), movelist);
            Bitloop(pl) _silent_move<PAWN, nst>(pos, SquareOf(pl), SquareOf((moveP<!c, LEFT>(pl))), movelist);

            if constexpr (nst == NSearch || check)
            {
                Bitloop(pf) _silent_move<PAWN, nst>(pos, SquareOf(pf), SquareOf((moveP<!c, FORWARD>(pf))),movelist);
                Bitloop(pp) _pawn_push<nst>(pos, SquareOf(pp), SquareOf((move<(c >> SOUTH), 2>(pp))), movelist);
            }

            Bitloop(pr_promo) _promotion_move<nst>(pos, SquareOf(pr_promo), SquareOf((moveP<!c, RIGHT>(pr_promo))), movelist);
            Bitloop(pl_promo) _promotion_move<nst>(pos, SquareOf(pl_promo), SquareOf((moveP<!c, LEFT>(pl_promo))), movelist);
            Bitloop(pf_promo) _promotion_move<nst>(pos, SquareOf(pf_promo), SquareOf((moveP<!c, FORWARD>(pf_promo))), movelist);
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

#include "search.h"

namespace ZeroLogic::Search{

    template<SearchType st, Color c>
    Value Callback::pp_callback(Position<c>& pos, Move enc, const u8 depth, Value Nalpha, Value Nbeta){
        Bit from = 1ull << Misc::Mfrom(enc);
        Bit to = 1ull << Misc::Mto(enc);
        Position<!c> npos = pos.move_pp(from, to);
        return -search<st>(npos, Nalpha, Nbeta, depth - 1);
    }

    template<Piece p, SearchType st, Color c>
    Value Callback::silent_callback(Position<c>& pos, Move enc, const u8 depth, Value Nalpha, Value Nbeta){
        Bit from = 1ull << Misc::Mfrom(enc);
        Bit to = 1ull << Misc::Mto(enc);
        Position<!c> npos = pos.template move_silent<p>(from, to);
        return -search<st>(npos, Nalpha, Nbeta, depth - 1);
    }

    template<Piece promo, SearchType st, Color c>
    Value Callback::promo_callback(Position<c>& pos, Move enc, const u8 depth, Value Nalpha, Value Nbeta) {
        Bit from = 1ull << Misc::Mfrom(enc);
        Bit to = 1ull << Misc::Mto(enc);
        Position<!c> npos = pos.template move_silent<PAWN, promo>(from, to);
        return -search<st>(npos, Nalpha, Nbeta, depth - 1);
    }

    template<SearchType st, Color c>
    Value Callback::ep_callback(Position<c>& pos, Move enc, const u8 depth, Value Nalpha, Value Nbeta){
        Bit from = 1ull << Misc::Mfrom(enc);
        Bit to = 1ull << Misc::Mto(enc);
        Position<!c> npos = pos.move_ep(from, to);
        return -search<st>(npos, Nalpha, Nbeta, depth - 1);
    }

    template<CastleType ct, Color c>
    Value Callback::castles_callback(Position<c>& pos, Move enc, const u8 depth, Value Nalpha, Value Nbeta){
        Position<!c> npos = pos.template move_castles<ct>();
        return -search(npos, Nalpha, Nbeta, depth - 1);
    }


    template<Piece p, SearchType st, Color c>
    inline void Callback::_silent_move(Position<c>& pos, u16 to, u16 from, RatedMove<c>*& movelist){
        Move move = to << 10 | from;
        Value static_val = Static::rate<p>(pos, move, u8(to), 1ull << from, 1ull << to);
        *(movelist++) = {move, static_val, silent_callback<p, st>};
    }
    template<SearchType st, Color c>
    inline void Callback::_pawn_push(Position<c>& pos, u16 to, u16 from, RatedMove<c>*& movelist) {
        Move move = to << 10 | from;
        Value static_val = Static::rate<PAWN>(pos, move, u8(to), 1ull << from, 1ull << to);
        *(movelist++) = {move, static_val, pp_callback<st>};
    }
    template<SearchType st, Color c>
    inline void Callback::_ep_move(Position<c>& pos, u16 to, u16 from, RatedMove<c>*& movelist){
        Move move = to << 10 | from;
        Value static_val = Static::rate<PAWN>(pos, move, u8(to), 1ull << from, 1ull << to);
        *(movelist++) = {move, static_val, ep_callback<st>};
    }

    template<SearchType st, Color c>
    inline void Callback::_promotion_move(Position<c>& pos, u16 to, u16 from, RatedMove<c>*& movelist){
        Move move = to << 10 | from;
        Value static_val = Static::rate<PAWN, true>(pos, u8(to), move, 1ull << from, 1ull << to);
        *(movelist++) = {QUEEN >> move, static_val, promo_callback<QUEEN, st>};

        if constexpr (st == NSearch)
        {
            *(movelist++) = {KNIGHT >> move, NON_CAPTURE, promo_callback<KNIGHT, st>};
            *(movelist++) = {ROOK >> move, NON_CAPTURE, promo_callback<ROOK, st>};
            *(movelist++) = {BISHOP >> move, NON_CAPTURE, promo_callback<BISHOP, st>};
        }
    }

    // wtf
    static constexpr Move castle_encodings[4] = {(1 << 10) | (0b1000 << 6) | 3, (5 << 10) | (0b1000 << 6) | 3, (57 << 10) | (0b1000 << 6) | 59,(61 << 10) | (0b1000 << 6) | 59};
    template<CastleType ct, Color c>
    inline void Callback::_castlemove(Position<c>& pos, RatedMove<c>*& movelist){
        Move move = castle_encodings[ct];
        Value static_val = Static::rate(pos, move);
        *(movelist++) = {move, static_val, castles_callback<ct>};
    }

}
