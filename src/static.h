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


    class Static {

            static inline u8 to;
            static inline Bit exchange_square;

            static constexpr Value piece_val_table[6] = {PAWN_MG, ROOK_MG, KNIGHT_MG, BISHOP_MG, QUEEN_MG, Wkgn};
            template<bool white>
            static FORCEINLINE Value captured_piece_val(const Board &board) {
                if (exchange_square & Pawns<!white>(board))      return piece_val_table[PAWN];
                if (exchange_square & Bishops<!white>(board))    return piece_val_table[BISHOP];
                if (exchange_square & Knights<!white>(board))    return piece_val_table[KNIGHT];
                if (exchange_square & Rooks<!white>(board))      return piece_val_table[ROOK];
                if (exchange_square & Queens<!white>(board))     return piece_val_table[QUEEN];
                return piece_val_table[KING];
            }

            template<bool white, Piece piece, bool promotion>
            static FORCEINLINE Board see_move(const Board &board, const Bit &from) {
                const map bp = board.BPawn, bn = board.BKnight, bb = board.BBishop, br = board.BRook, bq = board.BQueen, bk = board.BKing;
                const map wp = board.WPawn, wn = board.WKnight, wb = board.WBishop, wr = board.WRook, wq = board.WQueen, wk = board.WKing;

                const Bit rem = ~exchange_square;
                if constexpr (white) {
                    if constexpr (piece == PAWN) {
                        if constexpr (promotion) return {bp & rem, bn & rem, bb & rem, br & rem, bq & rem, bk & rem, wp ^ from, wn, wb, wr, wq ^ exchange_square, wk, 0};
                        else                     return {bp & rem, bn & rem, bb & rem, br & rem, bq & rem, bk & rem,wp ^ (exchange_square | from), wn, wb, wr, wq, wk, 0};
                    }
                    else if constexpr (piece == KNIGHT)
                        return {bp & rem, bn & rem, bb & rem, br & rem, bq & rem, bk & rem, wp, wn ^ (exchange_square | from), wb, wr, wq, wk, 0};
                    else if constexpr (piece == BISHOP)
                        return {bp & rem, bn & rem, bb & rem, br & rem, bq & rem, bk & rem, wp, wn, wb ^ (exchange_square | from), wr, wq, wk, 0};
                    else if constexpr (piece == ROOK)
                        return {bp & rem, bn & rem, bb & rem, br & rem, bq & rem, bk & rem, wp, wn, wb, wr ^ (exchange_square | from), wq, wk, 0};
                    else if constexpr (piece == QUEEN)
                        return {bp & rem, bn & rem, bb & rem, br & rem, bq & rem, bk & rem, wp, wn, wb, wr, wq ^ (exchange_square | from), wk, 0};
                    else if constexpr (piece == KING)
                        return {bp & rem, bn & rem, bb & rem, br & rem, bq & rem, bk & rem, wp, wn, wb, wr, wq, wk ^ (exchange_square | from), 0};
                } else {
                    if constexpr (piece == PAWN){
                        if constexpr (promotion) return {bp ^ from, bn, bb, br, bq ^ exchange_square, bk, wp & rem, wn & rem, wb & rem, wr & rem, wq & rem, wk & rem, 0};
                        else                     return {bp ^ (exchange_square | from), bn, bb, br, bq, bk, wp & rem, wn & rem, wb & rem, wr & rem, wq & rem, wk & rem, 0};
                    }
                    else if constexpr (piece == KNIGHT)
                        return {bp, bn ^ (exchange_square | from), bb, br, bq, bk, wp & rem, wn & rem, wb & rem, wr & rem, wq & rem, wk & rem, 0};
                    else if constexpr (piece == BISHOP)
                        return {bp, bn, bb ^ (exchange_square | from), br, bq, bk, wp & rem, wn & rem, wb & rem, wr & rem, wq & rem, wk & rem, 0};
                    else if constexpr (piece == ROOK)
                        return {bp, bn, bb, br ^ (exchange_square | from), bq, bk, wp & rem, wn & rem, wb & rem, wr & rem, wq & rem, wk & rem, 0};
                    else if constexpr (piece == QUEEN)
                        return {bp, bn, bb, br, bq ^ (exchange_square | from), bk, wp & rem, wn & rem, wb & rem, wr & rem, wq & rem, wk & rem, 0};
                    else if constexpr (piece == KING)
                        return {bp, bn, bb, br, bq, bk ^ (exchange_square | from), wp & rem, wn & rem, wb & rem, wr & rem, wq & rem, wk & rem, 0};
                }
                return {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
            }

            template<bool white, Piece piece, bool promotion>
            static int _see(const Board& board, const Bit from) {
                Value pval = captured_piece_val<white>(board);
                if constexpr (promotion) pval += (QUEEN_MG - PAWN_MG);
                const Board new_board = see_move<white, piece, promotion>(board, from);
                return std::max(0, int(pval - see<!white>(new_board)));
            }

            template<bool white>
            static FORCEINLINE int see(const Board &board) {

                // pawns
                if (pawn_atk_left<!white>(exchange_square) & Pawns<white>(board)){
                    if (exchange_square & last_rank<white>())   return _see<white, PAWN, true>(board, 1ull << (to + pawn_shift[0][white]));
                    else                                        return _see<white, PAWN, false>(board, 1ull << (to + pawn_shift[0][white]));
                }
                else if (pawn_atk_right<!white>(exchange_square) & Pawns<white>(board)){
                    if (exchange_square & last_rank<white>())   return _see<white, PAWN, true>(board, 1ull << (to + pawn_shift[1][white]));
                    else                                        return _see<white, PAWN, false>(board, 1ull << (to + pawn_shift[1][white]));
                }


                // knights
                else if (Lookup::knight[to] & Knights<white>(board))
                    return _see<white, KNIGHT, false>(board, 1ull << (SquareOf(Knights<white>(board) & Lookup::knight[to])));

                // bishops
                else if (Lookup::b_atk(to, board.Occ) & Bishops<white>(board))
                    return _see<white, BISHOP, false>(board, 1ull << (SquareOf(Bishops<white>(board) & Lookup::b_atk(to, board.Occ))));

                // rooks
                else if (Lookup::r_atk(to, board.Occ) & Rooks<white>(board))
                    return _see<white, ROOK, false>(board, 1ull << (SquareOf(Rooks<white>(board) & Lookup::r_atk(to, board.Occ))));

                // queens
                else if ((Lookup::r_atk(to, board.Occ) | Lookup::b_atk(to, board.Occ)) & Queens<white>(board))
                    return _see<white, QUEEN, false>(board, 1ull << (SquareOf(Queens<white>(board) & (Lookup::r_atk(to, board.Occ) | Lookup::b_atk(to, board.Occ)))));

                // king
                else if (Lookup::king[to] & King<white>(board))
                    return _see<white, KING, false>(board, 1ull << SquareOf(King<white>(board)));

                return ZERO;
            }

            template<bool white, Piece piece, bool promotion>
            static FORCEINLINE Value SEE(const Board& board, const u8& tosq, const Bit& from, const Bit& exsq){
                to = tosq;
                exchange_square = exsq;
                Value pval = captured_piece_val<white>(board);
                if constexpr (promotion) pval += QUEEN_MG - PAWN_MG;
                const Board new_board = see_move<white, piece, promotion>(board, from);
                return pval - see<!white>(new_board);
            }

        public:

            // static move scoring:
            // 10000 - hash move
            // (qval - pval) - promotion without capture
            // SEE score - captures
            // -50 - non-captures
            template<bool white, Piece piece, bool capture, bool promotion>
            static FORCEINLINE Value rate(const Board &board, const u8& tosq, const Bit from, const Bit exsq, const Move& move) {

                const u32 key = TT::get_key(board);
                if (TT::table[key].hash == board.hash && TT::table[key].move == move) return HASHMOVE;

                if constexpr (capture) return SEE<white, piece, promotion>(board, tosq, from, exsq);
                if constexpr (promotion) return QUEEN_MG - PAWN_MG;
                return NON_CAPTURE; // psqt diff
            }
    };
}
