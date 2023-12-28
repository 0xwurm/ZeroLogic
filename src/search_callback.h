#pragma once
#include "eval.h"
#include "static.h"

namespace ZeroLogic::Search {
    using namespace Boardstate;
    using namespace Movegen;

    struct RatedMove{
        Move move;
        Value static_rating;
        Value (*Mcallback)(const Board&, const Bit, const u16, const u8, Value, Value);
    };

    class Callback{

    private:

        template<State state, SearchType st>
        static Value Mcallback_pp(const Board&, Bit, Move, u8, Value, Value);
        template<State state, Piece piece, bool capture, SearchType st>
        static Value Mcallback_any(const Board&, Bit, Move, u8, Value, Value);
        template<State state, Piece promoted, bool capture, SearchType st>
        static Value Mcallback_promo(const Board&, Bit, Move, u8, Value, Value);
        template<State state, SearchType st>
        static Value Mcallback_ep(const Board&, Bit, Move, u8, Value, Value);
        template<State state, CastleType ct>
        static Value Mcallback_castles(const Board&, Bit, Move, u8, Value, Value);

        template<State state, Piece piece, bool capture, SearchType st>
        static FORCEINLINE void handle_move(const Board&, u16, u16, RatedMove*&);
        template<State state, SearchType st>
        static FORCEINLINE void handle_pp(const Board&, u16, u16, RatedMove*&);
        template<State state, SearchType st>
        static FORCEINLINE void handle_ep(const Board&, u16, u16, RatedMove*&);
        template<State state, bool capture, SearchType st>
        static FORCEINLINE void handle_promotion(const Board&, u16, u16, RatedMove*&);
        template<State state, CastleType ct>
        static FORCEINLINE void handle_castles(const Board&, RatedMove*&);

    public:

        using specific = RatedMove*;

        template<State state, Piece piece, bool st, bool check>
        static FORCEINLINE void silent_move(const Board& board, map& moves, map& captures, const Square& from, const Bit& ep_target, const u8& depth, RatedMove*& movelist){
            if constexpr (st == NSearch || check) {
                Bitloop(moves) handle_move<state, piece, false, SearchType(st)>(board, SquareOf(moves), from, movelist);
            }
            Bitloop(captures) handle_move<state, piece, true, SearchType(st)>(board, SquareOf(captures), from, movelist);
        }

        template<State state, bool st, bool check>
        static FORCEINLINE void ep_move(const Board& board, Bit& lep, Bit& rep, const Bit& ep_target, const u8& depth, RatedMove*& movelist){
            if (st == QSearch && !check) return;

            constexpr bool us = state.white_move;
            if (lep) handle_ep<state, SearchType(st)>(board, SquareOf(pawn_atk_left<us>(lep)), SquareOf(lep), movelist);
            if (rep) handle_ep<state, SearchType(st)>(board, SquareOf(pawn_atk_right<us>(rep)), SquareOf(rep), movelist);
        }

        template<State state, bool st, bool check>
        static FORCEINLINE void pawn_move(const Board& board, map& pr, map& pl, map& pr_promo, map& pl_promo, map& pf, map& pp, map& pf_promo, const Bit& ep_target, const u8& depth, RatedMove*& movelist){
            constexpr bool us = state.white_move;
            constexpr auto nst = SearchType(st);
            Bitloop(pr) handle_move<state, PAWN, true, nst>(board, SquareOf(pr), SquareOf(pr) + pawn_shift[0][us], movelist);
            Bitloop(pl) handle_move<state, PAWN, true, nst>(board, SquareOf(pl), SquareOf(pl) + pawn_shift[1][us], movelist);
            if constexpr (nst == NSearch || check) {
                Bitloop(pf) handle_move<state, PAWN, false, nst>(board, SquareOf(pf), SquareOf(pf) + sign<us>(-8),movelist);
                Bitloop(pp) handle_pp<state, nst>(board, SquareOf(pp), SquareOf(pp) + sign<us>(-16), movelist);
            }
            Bitloop(pr_promo) handle_promotion<state, true, nst>(board, SquareOf(pr_promo), SquareOf(pr_promo) + pawn_shift[0][us], movelist);
            Bitloop(pl_promo) handle_promotion<state, true, nst>(board, SquareOf(pl_promo), SquareOf(pl_promo) + pawn_shift[1][us], movelist);
            Bitloop(pf_promo) handle_promotion<state, false, nst>(board, SquareOf(pf_promo), SquareOf(pf_promo) + sign<us>(-8), movelist);
        }

        template<State state, bool left, bool st, bool check>
        static FORCEINLINE void castlemove(const Board& board, const Bit& ep_target, const u8& depth, RatedMove*& movelist){
            if (st == QSearch) return;

            if constexpr (left) {
                if constexpr (state.white_move) handle_castles<state, WHITE_OOO>(board, movelist);
                else                            handle_castles<state, BLACK_OOO>(board, movelist);
            }
            else {
                if constexpr (state.white_move) handle_castles<state, WHITE_OO>(board, movelist);
                else                            handle_castles<state, BLACK_OO>(board, movelist);
            }
        }

        template<State state, bool st, bool check>
        static FORCEINLINE void rookmove(const Board& board, map& moves, map& captures, const Square& sq, const Bit& ep_target, const u8& depth, RatedMove*& movelist){
            silent_move<state, ROOK, SearchType(st), check>(board, moves, captures, sq, ep_target, depth, movelist);
        }
        template<State state, bool st, bool check>
        static FORCEINLINE void kingmove(const Board& board, map& moves, map& captures, const Bit& ep_target, const u8& depth, RatedMove*& movelist){
            silent_move<state, KING, SearchType(st), check>(board, moves, captures, SquareOf(King<state.white_move>(board)), ep_target, depth, movelist);
        }

    };
}

#include "search.h"

namespace ZeroLogic::Search{
    using namespace Boardstate;
    using namespace Movegen;

#define ep_hash ::ZeroLogic::TT::keys[::ZeroLogic::TT::ep0 + (SquareOf(ep_target) % 8)]

    template<State state, SearchType st>
    Value Callback::Mcallback_pp(const Board& board, const Bit ep_target, const u16 enc, const u8 depth, Value Nalpha, Value Nbeta){
        Bit from = 1ull << Misc::Mfrom(enc);
        Bit to = 1ull << Misc::Mto(enc);
        map ephash = ::ZeroLogic::TT::keys[::ZeroLogic::TT::ep0 + (SquareOf(to) % 8)];
        if constexpr (state.has_ep_pawn) ephash ^= ep_hash;
        const Board new_board = move<PAWN, state.white_move, false, true, PIECE_INVALID>(board, from , to, ephash);
        if constexpr (st == QSearch)    return -qsearch<state.new_ep_pawn()>(new_board, to, Nalpha, Nbeta);
        else                            return -search<state.new_ep_pawn()>(new_board, to, depth - 1, Nalpha, Nbeta);
    }

    template<State state, Piece piece, bool capture, SearchType st>
    Value Callback::Mcallback_any(const Board& board, const Bit ep_target, const u16 enc, const u8 depth, Value Nalpha, Value Nbeta){
        constexpr bool us = state.white_move;
        Bit from = 1ull << Misc::Mfrom(enc);
        Bit to = 1ull << Misc::Mto(enc);
        const Board new_board = move<piece, us, capture, state.has_ep_pawn, PIECE_INVALID>(board, from, to, ep_hash);


        if constexpr (piece == KING) {
            if constexpr (st == QSearch)    return -qsearch<state.no_castles()>(new_board, 0, Nalpha, Nbeta);
            else                            return -search<state.no_castles()>(new_board, 0, depth - 1, Nalpha, Nbeta);
        }

        if constexpr (piece == ROOK){
            if constexpr (state.can_oo<us>()) {
                if (state.is_rook_right<us>(from)) {
                    if constexpr (st == QSearch)    return -qsearch<state.no_oo()>(new_board, 0, Nalpha, Nbeta);
                    else                            return -search<state.no_oo()>(new_board, 0, depth - 1, Nalpha, Nbeta);
                }
            }
            else if constexpr (state.can_ooo<us>()) {
                if (state.is_rook_left<us>(from)) {
                    if constexpr (st == QSearch)    return -qsearch<state.no_ooo()>(new_board, 0, Nalpha, Nbeta);
                    else                            return -search<state.no_ooo()>(new_board, 0, depth - 1, Nalpha, Nbeta);
                }
            }
        }

        if constexpr (st == QSearch)    return -qsearch<state.silent_move()>(new_board, 0, Nalpha, Nbeta);
        else                            return -search<state.silent_move()>(new_board, 0, depth - 1, Nalpha, Nbeta);
    }

    template<State state, Piece promoted, bool capture, SearchType st>
    Value Callback::Mcallback_promo(const Board& board, const Bit ep_target, const u16 enc, const u8 depth, Value Nalpha, Value Nbeta) {
        Bit from = 1ull << Misc::Mfrom(enc);
        Bit to = 1ull << Misc::Mto(enc);
        const Board new_board = move<PAWN, state.white_move, capture, state.has_ep_pawn, promoted>(board, from, to, ep_hash);
        if constexpr (st == QSearch)    return -qsearch<state.silent_move()>(new_board, 0, Nalpha, Nbeta);
        else                            return -search<state.silent_move()>(new_board, 0, 0, Nalpha, Nbeta);
    }

    template<State state, SearchType st>
    Value Callback::Mcallback_ep(const Board& board, const Bit ep_target, const u16 enc, const u8 depth, Value Nalpha, Value Nbeta){
        Bit from = 1ull << Misc::Mfrom(enc);
        Bit to = 1ull << Misc::Mto(enc);
        const Board new_board = Boardstate::ep_move<state.white_move>(board, from | to, ep_target);
        if constexpr (st == QSearch)    return -qsearch<state.silent_move()>(new_board, 0, Nalpha, Nbeta);
        else                            return -search<state.silent_move()>(new_board, 0, depth - 1, Nalpha, Nbeta);
    }

    template<State state, CastleType ct>
    Value Callback::Mcallback_castles(const Board& board, const Bit ep_target, const u16 enc, const u8 depth, Value Nalpha, Value Nbeta){
        const Board new_board = castle_move<ct, state.has_ep_pawn>(board, ep_hash);
        return -search<state.no_castles()>(new_board, 0, depth - 1, Nalpha, Nbeta);
    }

#undef ep_hash


    template<State state, Piece piece, bool capture, SearchType st>
    FORCEINLINE void Callback::handle_move(const Board& board, const u16 to, const u16 from, RatedMove*& movelist){
        const Move move = to << 10 | from;
        const Value static_val = Static::rate<state.white_move, piece, capture, false>(board, u8(to), 1ull << from, 1ull << to, move);
        *(movelist++) = {move, static_val, Mcallback_any<state, piece, capture, st>};
    }
    template<State state, SearchType st>
    FORCEINLINE void Callback::handle_pp(const Board& board, const u16 to, const u16 from, RatedMove*& movelist){
        const Move move = to << 10 | from;
        const Value static_val = Static::rate<state.white_move, PAWN, false, false>(board, u8(to), 1ull << from, 1ull << to, move);
        *(movelist++) = {move, static_val, Mcallback_pp<state, st>};
    }
    template<State state, SearchType st>
    FORCEINLINE void Callback::handle_ep(const Board& board, const u16 to, const u16 from, RatedMove*& movelist){
        const Move move = to << 10 | from;
        const Value static_val = Static::rate<state.white_move, PAWN, false, false>(board, u8(to), 1ull << from, 1ull << to, move);
        *(movelist++) = {move, static_val, Mcallback_ep<state, st>};
    }

    template<State state, bool capture, SearchType st>
    FORCEINLINE void Callback::handle_promotion(const Board& board, const u16 to, const u16 from, RatedMove*& movelist){
        const Move move = to << 10 | from;
        const Value static_val = Static::rate<state.white_move, PAWN, capture, true>(board, u8(to), 1ull << from, 1ull << to, move);
        *(movelist++) = {Move(move | (0b0100 << 6)), static_val, Mcallback_promo<state, QUEEN, capture, st>};
        if constexpr (st == NSearch) {
            *(movelist++) = {Move(move | (0b0010 << 6)), Value(-50), Mcallback_promo<state, KNIGHT, capture, NSearch>};
            *(movelist++) = {Move(move | (0b0001 << 6)), Value(-50), Mcallback_promo<state, ROOK, capture, NSearch>};
            *(movelist++) = {Move(move | (0b0011 << 6)), Value(-50), Mcallback_promo<state, BISHOP, capture, NSearch>};
        }
    }

    static constexpr Move castle_encodings[4] = {(1 << 10) | (0b1000 << 6) | 3, (5 << 10) | (0b1000 << 6) | 3, (57 << 10) | (0b1000 << 6) | 59,(61 << 10) | (0b1000 << 6) | 59};
    template<State state, CastleType ct>
    FORCEINLINE void Callback::handle_castles(const Board& board, RatedMove*& movelist){
        const Move move = castle_encodings[ct];
        const Value static_val = Static::rate<state.white_move, PIECE_INVALID, false, false>(board, 0, 0, 0, move);
        *(movelist++) = {move, static_val, Mcallback_castles<state, ct>};
    }

}
