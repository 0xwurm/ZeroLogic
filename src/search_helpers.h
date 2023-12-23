#pragma once

namespace ZeroLogic::Search {
    using namespace Boardstate;
    using namespace Movegen;

    template<bool white>
    FORCEINLINE bool is_check(const Board &board) {
        constexpr bool us = white;
        constexpr bool enemy = !us;

        const map king = King<us>(board);
        const Square king_square = SquareOf(king);

        map check_by_knight = Lookup::knight[king_square] & Knights<enemy>(board);
        if (check_by_knight) return true;

        const map pr = Movegen::pawn_atk_right<enemy>(Pawns<enemy>(board));
        const map pl = Movegen::pawn_atk_left<enemy>(Pawns<enemy>(board));
        if ((pr | pl) & king) return true;

        map atkR = Lookup::r_atk(king_square, board.Occ) & RookOrQueen<enemy>(board);
        if (atkR) return true;
        map atkB = Lookup::b_atk(king_square, board.Occ) & BishopOrQueen<enemy>(board);
        if (atkB) return true;

        return false;
    }

    // (1) color
    // (2) piece
    constexpr eval piece_val_table[2][5] = {{Bpwn, Brk, Bknght, Bbshp, Bqn},
                                            {Wpwn, Wrk, Wknght, Wbshp, Wqn}};

    template <bool white>
    FORCEINLINE eval captured_piece_val(const Board& board, const Bit& to){
        if (to & Pawns<!white>(board))     return piece_val_table[!white][PAWN];
        if (to & Bishops<!white>(board))   return piece_val_table[!white][BISHOP];
        if (to & Knights<!white>(board))   return piece_val_table[!white][KNIGHT];
        if (to & Rooks<!white>(board))     return piece_val_table[!white][ROOK];
        if (to & Queens<!white>(board))    return piece_val_table[!white][QUEEN];
        return EQUAL;
    }

    template<bool white, Piece piece>
    FORCEINLINE Board see_move(const Board& board, const Bit& to, const Bit& from){
        const map bp = board.BPawn, bn = board.BKnight, bb = board.BBishop, br = board.BRook, bq = board.BQueen, bk = board.BKing;
        const map wp = board.WPawn, wn = board.WKnight, wb = board.WBishop, wr = board.WRook, wq = board.WQueen, wk = board.WKing;

        const Bit rem = ~to;
        if constexpr(white){
            if constexpr        (piece == PAWN)     return {bp&rem, bn&rem, bb&rem, br&rem, bq&rem, bk, wp ^ (to | from), wn, wb, wr, wq, wk, 0};
            else if constexpr   (piece == KNIGHT)   return {bp&rem, bn&rem, bb&rem, br&rem, bq&rem, bk, wp, wn ^ (to | from), wb, wr, wq, wk, 0};
            else if constexpr   (piece == BISHOP)   return {bp&rem, bn&rem, bb&rem, br&rem, bq&rem, bk, wp, wn, wb ^ (to | from), wr, wq, wk, 0};
            else if constexpr   (piece == ROOK)     return {bp&rem, bn&rem, bb&rem, br&rem, bq&rem, bk, wp, wn, wb, wr ^ (to | from), wq, wk, 0};
            else if constexpr   (piece == QUEEN)    return {bp&rem, bn&rem, bb&rem, br&rem, bq&rem, bk, wp, wn, wb, wr, wq ^ (to | from), wk, 0};
        }
        else{
            if constexpr        (piece == PAWN)     return {bp ^ (to | from), bn, bb, br, bq, bk, wp&rem, wn&rem, wb&rem, wr&rem, wq&rem, wk, 0};
            else if constexpr   (piece == KNIGHT)   return {bp, bn ^ (to | from), bb, br, bq, bk, wp&rem, wn&rem, wb&rem, wr&rem, wq&rem, wk, 0};
            else if constexpr   (piece == BISHOP)   return {bp, bn, bb ^ (to | from), br, bq, bk, wp&rem, wn&rem, wb&rem, wr&rem, wq&rem, wk, 0};
            else if constexpr   (piece == ROOK)     return {bp, bn, bb, br ^ (to | from), bq, bk, wp&rem, wn&rem, wb&rem, wr&rem, wq&rem, wk, 0};
            else if constexpr   (piece == QUEEN)    return {bp, bn, bb, br, bq ^ (to | from), bk, wp&rem, wn&rem, wb&rem, wr&rem, wq&rem, wk, 0};
        }
    }

    template<bool white>
    void see(const Board& board, const Bit& atk_square, const u8& to, eval& , u8 depth);

    template<bool white, Piece piece>
    FORCEINLINE void _see(const Board& board, const Bit& atk_square, const u8& to, const Bit from, eval& value, u8 depth){
        value += captured_piece_val<white>(board, atk_square);
        const eval test_val = value;
        const Board new_board = see_move<white, piece>(board, atk_square, from);
        see<!white>(new_board, atk_square, to, value, depth + 1);

        if (depth >= 2) {
            if constexpr (white) {
                if (test_val > value)
                    value = test_val;
            } else {
                if (test_val < value)
                    value = test_val;
            }
        }
    }

    // promotion? + king?
    template<bool white>
    void see(const Board& board, const Bit& atk_square, const u8& to, eval& value, u8 depth){

        // pawns
        if (pawn_atk_left<!white>(atk_square) & Pawns<white>(board))
            _see<white, PAWN>(board, atk_square, to, 1ull << (to + pawn_shift[0][white]), value, depth);
        else if (pawn_atk_right<!white>(atk_square) & Pawns<white>(board))
            _see<white, PAWN>(board, atk_square, to, 1ull << (to + pawn_shift[1][white]), value, depth);

        // knights
        else if (Lookup::knight[to] & Knights<white>(board))
            _see<white, KNIGHT>(board, atk_square, to, 1ull << (SquareOf(Knights<white>(board) & Lookup::knight[to])), value, depth);

        // bishops
        else if (Lookup::b_atk(to, board.Occ) & Bishops<white>(board))
            _see<white, BISHOP>(board, atk_square, to, 1ull << (SquareOf(Bishops<white>(board) & Lookup::b_atk(to, board.Occ))), value, depth);

        // rooks
        else if (Lookup::r_atk(to, board.Occ) & Rooks<white>(board))
            _see<white, ROOK>(board, atk_square, to, 1ull << (SquareOf(Rooks<white>(board) & Lookup::r_atk(to, board.Occ))), value, depth);

        // queens
        else if ((Lookup::r_atk(to, board.Occ) | Lookup::b_atk(to, board.Occ)) & Queens<white>(board))
            _see<white, QUEEN>(board, atk_square, to, 1ull << (SquareOf(Queens<white>(board) & (Lookup::r_atk(to, board.Occ) | Lookup::b_atk(to, board.Occ)))), value, depth);

    }

    // static move scoring:
    // 50 - hash move
    // SEE score - captures
    // -1 - non-captures
    template <bool white, Piece piece>
    static FORCEINLINE eval static_rate(const Board& board, const u8& to, const Bit from, const Bit atk_square){
        eval val = EQUAL;
        _see<white, piece>(board, atk_square, to, from, val, 0);
        if constexpr (white)    return -val;
        else                    return val;
    }
}
