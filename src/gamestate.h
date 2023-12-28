#pragma clang diagnostic push
#pragma ide diagnostic ignored "modernize-use-nodiscard"
#pragma once

namespace ZeroLogic::Boardstate {

    class State{
        static constexpr Bit white_rook_left = 0x80;
        static constexpr Bit white_rook_right = 1;
        static constexpr Bit black_rook_left = white_rook_left << 56;
        static constexpr Bit black_rook_right = white_rook_right << 56;
        static constexpr map wk_path = 0x6;
        static constexpr map wq_path_k = 0x30;
        static constexpr map wq_path_r = 0x70;
        static constexpr map bk_path = wk_path << 56;
        static constexpr map bq_path_k = wq_path_k << 56;
        static constexpr map bq_path_r = wq_path_r << 56;

    public:
        constexpr State(bool white, bool ep, bool wOO, bool wOOO, bool bOO, bool bOOO) :
            white_move(white), has_ep_pawn(ep), white_oo(wOO), white_ooo(wOOO), black_oo(bOO), black_ooo(bOOO)
        {}
        const bool white_move;
        const bool has_ep_pawn;
        const bool white_oo;
        const bool white_ooo;
        const bool black_oo;
        const bool black_ooo;

        static constexpr inline State normal(){return {true, false, true, true, true, true};}
        constexpr inline State silent_move() const {return {!white_move, false, white_oo, white_ooo, black_oo, black_ooo};}
        constexpr inline State new_ep_pawn() const{return {!white_move, true, white_oo, white_ooo, black_oo, black_ooo};}

        constexpr inline State no_oo() const{
            if (white_move)    return {!white_move, false, false, white_ooo, black_oo, black_ooo};
            else               return {!white_move, false, white_oo, white_ooo, false, black_ooo};
        }

        constexpr inline State no_ooo() const{
            if (white_move)    return {!white_move, false, white_oo, false, black_oo, black_ooo};
            else               return {!white_move, false, white_oo, white_ooo, black_oo, false};
        }
        constexpr inline State no_castles() const{
            if (white_move) return {false, false, false, false, black_oo, black_ooo};
            else            return {true, false, white_oo, white_ooo, false, false};
        }

        template <bool white>
        constexpr FORCEINLINE bool can_castle() const{
            if constexpr (white)    return white_oo | white_ooo;
            else                    return black_oo | black_ooo;
        }
        template <bool white>
        constexpr FORCEINLINE bool can_oo() const{
            if constexpr (white)    return white_oo;
            else                    return black_oo;
        }
        template <bool white>
        constexpr FORCEINLINE bool can_ooo() const{
            if constexpr (white)    return white_ooo;
            else                    return black_ooo;
        }

        template <bool white>
        COMPILETIME bool is_rook_left(Bit s) {
            if constexpr (white)    return s & white_rook_left;
            else                    return s & black_rook_left;
        }
        template <bool white>
        COMPILETIME bool is_rook_right(Bit s) {
            if constexpr (white)    return s & white_rook_right;
            else                    return s & black_rook_right;
        }

        // technically not fully legal: https://lichess.org/study/Vk0GyOAR/51jOOgxA
        template <bool white_move>
        static FORCEINLINE bool short_legal(const map occ, const map rook, const map kingban){
            if (white_move)   return !(wk_path & (occ | kingban)) && is_rook_right<white_move>(rook);
            else              return !(bk_path & (occ | kingban)) && is_rook_right<white_move>(rook);
        }
        template <bool white_move>
        static FORCEINLINE bool long_legal(const map occ, const map rook, const map kingban){
            if (white_move)   return !((wq_path_k & kingban) | (wq_path_r & occ)) && is_rook_left<white_move>(rook);
            else              return !((bq_path_k & kingban) | (bq_path_r & occ)) && is_rook_left<white_move>(rook);
        }

    };

	struct Board{
        const map BPawn, BKnight, BBishop, BRook, BQueen, BKing;
        const map WPawn, WKnight, WBishop, WRook, WQueen, WKing;
        const map Black, White, Occ;
        const u64 hash;

        constexpr Board(
                map bp, map bn, map bb, map br, map bq, map bk,
                map wp, map wn, map wb, map wr, map wq, map wk,
                u64 zh) :
                BPawn(bp), BKnight(bn), BBishop(bb), BRook(br), BQueen(bq), BKing(bk),
                WPawn(wp), WKnight(wn), WBishop(wb), WRook(wr), WQueen(wq), WKing(wk),
                Black(bp | bn | bb | br | bq | bk),
                White(wp | wn | wb | wr | wq | wk),
                Occ(Black | White),
                hash(zh)
        {}

    };

    template <bool white>
    COMPILETIME map King(const Board& board){
        if constexpr (white)    return board.WKing;
        else                    return board.BKing;
    }
    template <bool white>
    COMPILETIME map Queens(const Board& board){
        if constexpr (white)    return board.WQueen;
        else                    return board.BQueen;
    }
    template <bool white>
    COMPILETIME map Bishops(const Board& board){
        if constexpr (white)    return board.WBishop;
        else                    return board.BBishop;
    }
    template <bool white>
    COMPILETIME map Rooks(const Board& board){
        if constexpr (white)    return board.WRook;
        else                    return board.BRook;
    }

    template <bool white>
    COMPILETIME map BishopOrQueen(const Board& board){
        if constexpr (white)    return board.WBishop | board.WQueen;
        else                    return board.BBishop | board.BQueen;
    }
    template <bool white>
    COMPILETIME map RookOrQueen(const Board& board){
        if constexpr (white)    return board.WRook | board.WQueen;
        else                    return board.BRook | board.BQueen;
    }

    template <bool white>
    COMPILETIME map ColorPieces(const Board& board){
        if constexpr (white)    return board.White;
        else                    return board.Black;
    }
    template <bool white>
    COMPILETIME map EmptyOrEnemy(const Board& board){
        if constexpr (white)    return ~board.White;
        else                    return ~board.Black;
    }
    template <bool white>
    COMPILETIME map Knights(const Board& board){
        if constexpr (white)    return board.WKnight;
        else                    return board.BKnight;
    }
    template <bool white>
    COMPILETIME map Pawns(const Board& board) {
        if constexpr (white) return board.WPawn;
        else return board.BPawn;
    }


    template <bool white_move>
    FORCEINLINE static Board ep_move(const Board& old_board, map mov, Bit ep_target){
        const map bp = old_board.BPawn, bn = old_board.BKnight, bb = old_board.BBishop, br = old_board.BRook, bq = old_board.BQueen, bk = old_board.BKing;
        const map wp = old_board.WPawn, wn = old_board.WKnight, wb = old_board.WBishop, wr = old_board.WRook, wq = old_board.WQueen, wk = old_board.WKing;
        u64 new_hash = old_board.hash ^ TT::keys[TT::ep0 + (SquareOf(ep_target) % 8)] ^ TT::get_key<PAWN, white_move>(SquareOf(mov)) ^ TT::get_key<PAWN, white_move>(SquareOf(mov & (mov - 1))) ^ TT::get_key<PAWN, !white_move>(SquareOf(ep_target)) ^ TT::w_key;

        if constexpr (white_move)   return {bp ^ ep_target, bn, bb, br, bq, bk, wp ^ mov, wn, wb, wr, wq, wk, new_hash};
        else                        return {bp ^ mov, bn, bb, br, bq, bk, wp ^ ep_target, wn, wb, wr, wq, wk, new_hash};
    }

    template <CastleType castle_type, bool change_hash>
    FORCEINLINE static Board castle_move(const Board& old_board, const map hash_change){
        const map bp = old_board.BPawn, bn = old_board.BKnight, bb = old_board.BBishop, br = old_board.BRook, bq = old_board.BQueen, bk = old_board.BKing;
        const map wp = old_board.WPawn, wn = old_board.WKnight, wb = old_board.WBishop, wr = old_board.WRook, wq = old_board.WQueen, wk = old_board.WKing;
        u64 hash = old_board.hash ^ TT::w_key;
        if constexpr (change_hash) hash ^= hash_change;
        if constexpr        (castle_type == WHITE_OO)  return {bp, bn, bb, br, bq, bk, wp, wn, wb, wr ^ 0x5, wq, wk ^ 0xa, hash ^ TT::ws_key};
        else if constexpr   (castle_type == WHITE_OOO) return {bp, bn, bb, br, bq, bk, wp, wn, wb, wr ^ 0x90, wq, wk ^ 0x28, hash ^ TT::wl_key};
        else if constexpr   (castle_type == BLACK_OO)  return {bp, bn, bb, br ^ (5ull << 56), bq, bk ^ (0xaull << 56), wp, wn, wb, wr, wq, wk, hash ^ TT::bs_key};
        else if constexpr   (castle_type == BLACK_OOO) return {bp, bn, bb, br ^ (0x90ull << 56), bq, bk ^ (0x28ull << 56), wp, wn, wb, wr, wq, wk, hash ^ TT::bl_key};
    }

    template <Piece piece, bool white_move, Piece taken, Piece promotion>
    FORCEINLINE static Board _move(const Board& old_board, const Bit& from, const Bit& to, u64& hash){
        map bp = old_board.BPawn, bn = old_board.BKnight, bb = old_board.BBishop, br = old_board.BRook, bq = old_board.BQueen, bk = old_board.BKing;
        map wp = old_board.WPawn, wn = old_board.WKnight, wb = old_board.WBishop, wr = old_board.WRook, wq = old_board.WQueen, wk = old_board.WKing;

        if constexpr (taken != PIECE_INVALID) {
            hash ^= TT::get_key<taken, !white_move>(SquareOf(to));
            if constexpr (white_move) {
                if constexpr        (taken == PAWN)   bp ^= to;
                else if constexpr   (taken == KNIGHT) bn ^= to;
                else if constexpr   (taken == BISHOP) bb ^= to;
                else if constexpr   (taken == ROOK)   br ^= to;
                else if constexpr   (taken == QUEEN)  bq ^= to;
            }
            else {
                if constexpr        (taken == PAWN)   wp ^= to;
                else if constexpr   (taken == KNIGHT) wn ^= to;
                else if constexpr   (taken == BISHOP) wb ^= to;
                else if constexpr   (taken == ROOK)   wr ^= to;
                else if constexpr   (taken == QUEEN)  wq ^= to;
            }
        }

        if constexpr (promotion != PIECE_INVALID){
            hash ^= TT::get_key<PAWN, white_move>(SquareOf(from)) ^ TT::get_key<promotion, white_move>(SquareOf(to));
            if constexpr (white_move){
                if      constexpr (promotion == QUEEN)   return {bp, bn, bb, br, bq, bk, wp ^ from, wn, wb, wr, wq ^ to, wk, hash};
                else if constexpr (promotion == ROOK)    return {bp, bn, bb, br, bq, bk, wp ^ from, wn, wb, wr ^ to, wq, wk, hash};
                else if constexpr (promotion == BISHOP)  return {bp, bn, bb, br, bq, bk, wp ^ from, wn, wb ^ to, wr, wq, wk, hash};
                else if constexpr (promotion == KNIGHT)  return {bp, bn, bb, br, bq, bk, wp ^ from, wn ^ to, wb, wr, wq, wk, hash};
            }
            else{
                if      constexpr (promotion == QUEEN)   return {bp ^ from, bn, bb, br, bq ^ to, bk, wp, wn, wb, wr, wq, wk, hash};
                else if constexpr (promotion == ROOK)    return {bp ^ from, bn, bb, br ^ to, bq, bk, wp, wn, wb, wr, wq, wk, hash};
                else if constexpr (promotion == BISHOP)  return {bp ^ from, bn, bb ^ to, br, bq, bk, wp, wn, wb, wr, wq, wk, hash};
                else if constexpr (promotion == KNIGHT)  return {bp ^ from, bn ^ to, bb, br, bq, bk, wp, wn, wb, wr, wq, wk, hash};
            }
        }
        else {
            const map mov = from | to;
            hash ^= TT::get_key<piece, white_move>(SquareOf(from)) ^ TT::get_key<piece, white_move>(SquareOf(to));
            if constexpr (white_move) {
                if constexpr (PAWN == piece)    return {bp, bn, bb, br, bq, bk, wp ^ mov, wn, wb, wr, wq, wk, hash};
                if constexpr (KNIGHT == piece)  return {bp, bn, bb, br, bq, bk, wp, wn ^ mov, wb, wr, wq, wk, hash};
                if constexpr (BISHOP == piece)  return {bp, bn, bb, br, bq, bk, wp, wn, wb ^ mov, wr, wq, wk, hash};
                if constexpr (ROOK == piece)    return {bp, bn, bb, br, bq, bk, wp, wn, wb, wr ^ mov, wq, wk, hash};
                if constexpr (QUEEN == piece)   return {bp, bn, bb, br, bq, bk, wp, wn, wb, wr, wq ^ mov, wk, hash};
                if constexpr (KING == piece)    return {bp, bn, bb, br, bq, bk, wp, wn, wb, wr, wq, wk ^ mov, hash};
            } else {
                if constexpr (PAWN == piece)    return {bp ^ mov, bn, bb, br, bq, bk, wp, wn, wb, wr, wq, wk, hash};
                if constexpr (KNIGHT == piece)  return {bp, bn ^ mov, bb, br, bq, bk, wp, wn, wb, wr, wq, wk, hash};
                if constexpr (BISHOP == piece)  return {bp, bn, bb ^ mov, br, bq, bk, wp, wn, wb, wr, wq, wk, hash};
                if constexpr (ROOK == piece)    return {bp, bn, bb, br ^ mov, bq, bk, wp, wn, wb, wr, wq, wk, hash};
                if constexpr (QUEEN == piece)   return {bp, bn, bb, br, bq ^ mov, bk, wp, wn, wb, wr, wq, wk, hash};
                if constexpr (KING == piece)    return {bp, bn, bb, br, bq, bk ^ mov, wp, wn, wb, wr, wq, wk, hash};
            }
        }
    }

    template <Piece piece, bool white_move, bool taking, bool change_hash, Piece promotion>
    FORCEINLINE static Board move(const Board& old_board, const Bit& from, const Bit& to, const map hash_change){
        u64 hash = old_board.hash ^ TT::w_key;
        if constexpr (change_hash) hash ^= hash_change;

        if constexpr (taking) {
            if (to & Pawns<!white_move>(old_board))     return _move<piece, white_move, PAWN, promotion>(old_board, from, to, hash);
            if (to & Bishops<!white_move>(old_board))   return _move<piece, white_move, BISHOP, promotion>(old_board, from, to, hash);
            if (to & Knights<!white_move>(old_board))   return _move<piece, white_move, KNIGHT, promotion>(old_board, from, to, hash);
            if (to & Rooks<!white_move>(old_board))     return _move<piece, white_move, ROOK, promotion>(old_board, from, to, hash);
            if (to & Queens<!white_move>(old_board))    return _move<piece, white_move, QUEEN, promotion>(old_board, from, to, hash);
        }
        return _move<piece, white_move, PIECE_INVALID, promotion>(old_board, from, to, hash);
    }

    template <bool wm, bool taking>
    static Board pT_move(const Board& old_board, const Bit& from, const Bit& to, const Piece promo, u64 hash_change){
        if (promo == QUEEN)     return move<PAWN, wm, taking, true, QUEEN>(old_board, from, to, hash_change);
        if (promo == KNIGHT)    return move<PAWN, wm, taking, true, KNIGHT>(old_board, from, to, hash_change);
        if (promo == BISHOP)    return move<PAWN, wm, taking, true, BISHOP>(old_board, from, to, hash_change);
        if (promo == ROOK)      return move<PAWN, wm, taking, true, ROOK>(old_board, from, to, hash_change);

        if (from & Pawns<wm>(old_board))        return move<PAWN, wm, taking, true, PIECE_INVALID>(old_board, from, to, hash_change);
        if (from & Bishops<wm>(old_board))      return move<BISHOP, wm, taking, true, PIECE_INVALID>(old_board, from, to, hash_change);
        if (from & Knights<wm>(old_board))      return move<KNIGHT, wm, taking, true, PIECE_INVALID>(old_board, from, to, hash_change);
        if (from & Rooks<wm>(old_board))        return move<ROOK, wm, taking, true, PIECE_INVALID>(old_board, from, to, hash_change);
        if (from & Queens<wm>(old_board))       return move<QUEEN, wm, taking, true, PIECE_INVALID>(old_board, from, to, hash_change);
        if (from & King<wm>(old_board))         return move<KING, wm, taking, true, PIECE_INVALID>(old_board, from, to, hash_change);
        return {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    }

    static Board noT_move(const Board& old_board, const Bit& from, const Bit& to, const bool wm, const bool capture, const Piece promo, u64 hash_change) {
        if ( wm &&  capture) return pT_move<true, true>(old_board, from, to, promo, hash_change);
        if (!wm &&  capture) return pT_move<false, true>(old_board, from, to, promo, hash_change);
        if (!wm && !capture) return pT_move<false, false>(old_board, from, to, promo, hash_change);
        if ( wm && !capture) return pT_move<true, false>(old_board, from, to, promo, hash_change);
        return {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    }

    template <bool white>
    COMPILETIME int sign(int i){
        if constexpr (white)    return i;
        else                    return -i;
    }

}

#pragma clang diagnostic pop