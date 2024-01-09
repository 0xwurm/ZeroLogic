#pragma once

namespace ZeroLogic::Search {
    using namespace Boardstate;
    using namespace Movegen;

    template<Color c>
    FORCEINLINE bool is_check(const TempPos &board) {
        constexpr Color us = c;
        constexpr Color enemy = !us;

        const map king = board.king<us>();
        const Square king_square = SquareOf(king);

        map check_by_knight = Lookup::knight[king_square] & board.knights<enemy>();
        if (check_by_knight) return true;

        const map pr = Movegen::pawn_atk_right<enemy>(board.pawns<enemy>());
        const map pl = Movegen::pawn_atk_left<enemy>(board.pawns<enemy>());
        if ((pr | pl) & king) return true;

        map atkR = Lookup::r_atk(king_square, board.allPieces()) & board.straightSliders<enemy>();
        if (atkR) return true;
        map atkB = Lookup::b_atk(king_square, board.allPieces()) & board.diagonalSliders<enemy>();
        if (atkB) return true;

        return false;
    }


    class Static {

            static inline u8 to;
            static inline Bit exchange_square;

            static constexpr Value piece_val_table[6] = {PAWN_MG, ROOK_MG, KNIGHT_MG, BISHOP_MG, QUEEN_MG, Wkgn};
            template<Color c>
            static FORCEINLINE Value captured_piece_val(const TempPos &board) {
                if (exchange_square & board.pawns<!c>())      return piece_val_table[PAWN];
                if (exchange_square & board.bishops<!c>())    return piece_val_table[BISHOP];
                if (exchange_square & board.knights<!c>())    return piece_val_table[KNIGHT];
                if (exchange_square & board.rooks<!c>())      return piece_val_table[ROOK];
                if (exchange_square & board.queens<!c>())     return piece_val_table[QUEEN];
                return piece_val_table[KING];
            }

            template<Color c, Piece piece, bool promotion>
            static int _see(const TempPos& board, const Bit from) {
                Value pval = captured_piece_val<c>(board);
                if constexpr (promotion) pval += (QUEEN_MG - PAWN_MG);
                const TempPos new_board = board.see_move<c, piece, promotion>(from, exchange_square);
                return std::max(0, int(pval - see<!c>(new_board)));
            }

            template<Color c>
            static FORCEINLINE int see(const TempPos &board) {

                // pawns
                if (pawn_atk_left<!c>(exchange_square) & board.pawns<c>()){
                    if (exchange_square & last_rank<c>())   return _see<c, PAWN, true>(board, 1ull << (to + pawn_shift[0][c]));
                    else                                    return _see<c, PAWN, false>(board, 1ull << (to + pawn_shift[0][c]));
                }
                else if (pawn_atk_right<!c>(exchange_square) & board.pawns<c>()){
                    if (exchange_square & last_rank<c>())   return _see<c, PAWN, true>(board, 1ull << (to + pawn_shift[1][c]));
                    else                                    return _see<c, PAWN, false>(board, 1ull << (to + pawn_shift[1][c]));
                }


                // knights
                else if (Lookup::knight[to] & board.knights<c>())
                    return _see<c, KNIGHT, false>(board, 1ull << (SquareOf(board.knights<c>() & Lookup::knight[to])));

                // bishops
                else if (Lookup::b_atk(to, board.allPieces()) & board.bishops<c>())
                    return _see<c, BISHOP, false>(board, 1ull << (SquareOf(board.bishops<c>() & Lookup::b_atk(to, board.allPieces()))));

                // rooks
                else if (Lookup::r_atk(to, board.allPieces()) & board.rooks<c>())
                    return _see<c, ROOK, false>(board, 1ull << (SquareOf(board.rooks<c>() & Lookup::r_atk(to, board.allPieces()))));

                // queens
                else if ((Lookup::r_atk(to, board.allPieces()) | Lookup::b_atk(to, board.allPieces())) & board.queens<c>())
                    return _see<c, QUEEN, false>(board, 1ull << (SquareOf(board.queens<c>() & (Lookup::r_atk(to, board.allPieces()) | Lookup::b_atk(to, board.allPieces())))));

                // king
                else if (Lookup::king[to] & board.king<c>())
                    return _see<c, KING, false>(board, 1ull << SquareOf(board.king<c>()));

                return ZERO;
            }

            template<Color c, Piece piece, bool promotion>
            static FORCEINLINE Value SEE(const TempPos& board, const u8& tosq, const Bit& from, const Bit& exsq){
                to = tosq;
                exchange_square = exsq;
                Value pval = captured_piece_val<c>(board);
                if constexpr (promotion) pval += QUEEN_MG - PAWN_MG;
                const TempPos new_board = board.see_move<c, piece, promotion>(from, exchange_square);
                return pval - see<!c>(new_board);
            }

        public:

            // static move scoring:
            // 10000 - hash move
            // (qval - pval) - promotion without capture
            // SEE score - captures
            // -50 - non-captures
            template<Color c, Piece piece, bool capture, bool promotion>
            static FORCEINLINE Value rate(const TempPos &board, const u8& tosq, const Bit from, const Bit exsq, const Move& move) {

                const u32 key = TT::get_key(board);
                if (TT::table[key].hash == board.getHash() && TT::table[key].move == move) return HASHMOVE;

                if constexpr (capture) return SEE<c, piece, promotion>(board, tosq, from, exsq);
                if constexpr (promotion) return QUEEN_MG - PAWN_MG;
                return NON_CAPTURE; // psqt diff
            }
    };
}
