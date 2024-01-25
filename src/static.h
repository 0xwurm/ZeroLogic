#pragma once

namespace ZeroLogic::Search {

    using namespace Movegen;

    class Static {

            static inline u8 to;
            static inline Bit exchange_square;

            static constexpr Value piece_val_table[6] = {PAWN_MG, ROOK_MG, KNIGHT_MG, BISHOP_MG, QUEEN_MG, Wkgn};
            template<Color c>
            static inline Value captured_piece_val(Position<c>& pos) {
                if (exchange_square & pos.ePawns)   return piece_val_table[PAWN];
                if (exchange_square & pos.eBishops) return piece_val_table[BISHOP];
                if (exchange_square & pos.eKnights) return piece_val_table[KNIGHT];
                if (exchange_square & pos.eRooks)   return piece_val_table[ROOK];
                if (exchange_square & pos.eQueens)  return piece_val_table[QUEEN];
                return piece_val_table[KING];
            }

            template<Piece p, bool promo = false, Color c>
            static int _see(Position<c>& pos, const Bit from) {
                Value pval = captured_piece_val(pos);
                if constexpr (promo) pval += (QUEEN_MG - PAWN_MG);
                Position<!c> npos = pos.template move_silent<p, promo ? QUEEN : PIECE_NONE, true>(from, exchange_square);
                return std::max(0, int(pval - see<!c>(npos)));
            }

            // trash
            template<Color c>
            static inline int see(Position<c>& pos) {

                // pawns
                if (moveP<!c, LEFT>(exchange_square) & pos.oPawns){
                    if (exchange_square & last_rank<c>())   return _see<PAWN, true>(pos, moveP<!c, LEFT>(exchange_square));
                    else                                    return _see<PAWN>(pos, moveP<!c, LEFT>(exchange_square));
                }
                else if (moveP<!c, RIGHT>(exchange_square) & pos.oPawns){
                    if (exchange_square & last_rank<c>())   return _see<PAWN, true>(pos, moveP<!c, RIGHT>(exchange_square));
                    else                                    return _see<PAWN>(pos, moveP<!c, RIGHT>(exchange_square));
                }


                // knights
                else if (Lookup::knight[to] & pos.oKnights)
                    return _see<KNIGHT>(pos, BitOf(pos.oKnights & Lookup::knight[to]));

                // bishops
                else if (Lookup::b_atk(to, pos.Occ) & pos.oBishops)
                    return _see<BISHOP>(pos, BitOf(pos.oBishops & Lookup::b_atk(to, pos.Occ)));

                // rooks
                else if (Lookup::r_atk(to, pos.Occ) & pos.oRooks)
                    return _see<ROOK>(pos, BitOf(pos.oRooks & Lookup::r_atk(to, pos.Occ)));

                // queens
                else if ((Lookup::r_atk(to, pos.Occ) | Lookup::b_atk(to, pos.Occ)) & pos.oQueens)
                    return _see<QUEEN>(pos, BitOf(pos.oQueens & (Lookup::r_atk(to, pos.Occ) | Lookup::b_atk(to, pos.Occ))));

                // king
                else if (Lookup::king[to] & pos.oKing)
                    return _see<KING>(pos, BitOf(pos.oKing));

                return ZERO;
            }

            template<Piece p, bool promo, Color c>
            static inline Value SEE(Position<c>& pos, const u8& tosq, const Bit& from, const Bit& exsq){
                to = tosq;
                exchange_square = exsq;
                Value pval = captured_piece_val<c>(pos);
                if constexpr (promo) pval += QUEEN_MG - PAWN_MG;
                Position<!c> npos = pos.template move_silent<p, promo ? QUEEN : PIECE_NONE>(from, exchange_square);
                return pval - see<!c>(npos);
            }

        public:

            // static move scoring:
            // 10000 - hash move
            // (qval - pval) - promotion without capture
            // SEE score - captures
            // -50 - non-captures
            template<Piece piece = PIECE_NONE, bool promotion = false, Color c>
            static inline Value rate(Position<c>& pos, const Move& move, const u8& tosq = 0, const Bit from = 0, const Bit exsq = 0) {

                const u32 key = *pos.hash;
                // if (TT::table[key].hash == pos.hash && TT::table[key].move == move) return HASHMOVE;

                // if (exsq & pos.enemy) return SEE<piece, promotion>(pos, tosq, from, exsq);
                // if constexpr (promotion) return QUEEN_MG - PAWN_MG;
                return NON_CAPTURE; // TODO: psqt diff
            }
    };
}
