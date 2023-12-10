#pragma clang diagnostic push
#pragma ide diagnostic ignored "modernize-use-nodiscard"
#pragma once
#include "variables.h"
#include "intrin.h"
#include <algorithm>

#define getNumNotation(char1, char2) (7 - (char1 - 97) + 8 * (char2 - 49))
#define SquareOf(X) _tzcnt_u64(X)
#define Bitloop(X) for(;X; X = _blsr_u64(X))

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

        template <bool white>
        constexpr inline State no_oo() const{
            if constexpr (white)    return {!white_move, false, false, white_ooo, black_oo, black_ooo};
            else                    return {!white_move, false, white_oo, white_ooo, false, black_ooo};
        }
        template <bool white>
        constexpr inline State no_ooo() const{
            if constexpr (white)    return {!white_move, false, white_oo, false, black_oo, black_ooo};
            else                    return {!white_move, false, white_oo, white_ooo, black_oo, false};
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

    template <CastleType castle_type, bool change_hash>
    FORCEINLINE static Board castle_move(const Board& old_board, const map hash_change){
        const map bp = old_board.BPawn, bn = old_board.BKnight, bb = old_board.BBishop, br = old_board.BRook, bq = old_board.BQueen, bk = old_board.BKing;
        const map wp = old_board.WPawn, wn = old_board.WKnight, wb = old_board.WBishop, wr = old_board.WRook, wq = old_board.WQueen, wk = old_board.WKing;
        u64 hash = old_board.hash ^ TT::w_key;
        if constexpr (change_hash) hash ^= hash_change;
        if constexpr        (castle_type == WHITE_OO)  return {bp, bn, bb, br, bq, bk, wp, wn, wb, wr ^ wOO_r, wq, wk ^ wOO_k, hash ^ TT::ws_key};
        else if constexpr   (castle_type == WHITE_OOO) return {bp, bn, bb, br, bq, bk, wp, wn, wb, wr ^ wOOO_r, wq, wk ^ wOOO_k, hash ^ TT::wl_key};
        else if constexpr   (castle_type == BLACK_OO)  return {bp, bn, bb, br ^ bOO_r, bq, bk ^ bOO_k, wp, wn, wb, wr, wq, wk, hash ^ TT::bs_key};
        else if constexpr   (castle_type == BLACK_OOO) return {bp, bn, bb, br ^ bOOO_r, bq, bk ^ bOOO_k, wp, wn, wb, wr, wq, wk, hash ^ TT::bl_key};
    }


    template <bool white>
    COMPILETIME int sign(int i){
        if constexpr (white)    return i;
        else                    return -i;
    }

    template <bool white_move, bool>
    FORCEINLINE static Bit fen(std::string fen){
        fen.erase(0, fen.find(' ') + 1);
        fen.erase(0, fen.find(' ') + 1);
        fen.erase(0, fen.find(' ') + 1);
        if (fen.front() == '-') return 0;
        else return 1ull << (getNumNotation(fen[0], fen[1]) + sign<white_move>(-8));
    }

    template <bool board>
    FORCEINLINE static auto fen(std::string fen){
        if constexpr (board) {
            map bp{}, bn{}, bb{}, br{}, bq{}, bk{};
            map wp{}, wn{}, wb{}, wr{}, wq{}, wk{};
            u64 hash;
            Bit index_bit = 1ull << 63;

            std::string position_fen = fen.substr(0, fen.find(' '));
            for (char t: position_fen) {
                if (t == '/') continue;
                if (t - 60 > 0) {
                    switch (t) {
                        case 'p': bp |= index_bit; hash ^= TT::get_key<PAWN, false>(SquareOf(index_bit)); break;
                        case 'P': wp |= index_bit; hash ^= TT::get_key<PAWN, true>(SquareOf(index_bit)); break;
                        case 'r': br |= index_bit; hash ^= TT::get_key<ROOK, false>(SquareOf(index_bit)); break;
                        case 'R': wr |= index_bit; hash ^= TT::get_key<ROOK, true>(SquareOf(index_bit)); break;
                        case 'b': bb |= index_bit; hash ^= TT::get_key<BISHOP, false>(SquareOf(index_bit)); break;
                        case 'B': wb |= index_bit; hash ^= TT::get_key<BISHOP, true>(SquareOf(index_bit)); break;
                        case 'n': bn |= index_bit; hash ^= TT::get_key<KNIGHT, false>(SquareOf(index_bit)); break;
                        case 'N': wn |= index_bit; hash ^= TT::get_key<KNIGHT, true>(SquareOf(index_bit)); break;
                        case 'q': bq |= index_bit; hash ^= TT::get_key<QUEEN, false>(SquareOf(index_bit)); break;
                        case 'Q': wq |= index_bit; hash ^= TT::get_key<QUEEN, true>(SquareOf(index_bit)); break;
                        case 'k': bk |= index_bit; hash ^= TT::get_key<KING, false>(SquareOf(index_bit)); break;
                        case 'K': wk |= index_bit; hash ^= TT::get_key<KING, true>(SquareOf(index_bit)); break;
                        default:
                            std::cout << "invalid fen: '" << fen << "'" << std::endl;
                            return Board{bp, bn, bb, br, bq, bk, wp, wn, wb, wr, wq, wk, hash};
                    }
                    index_bit >>= 1;
                } else index_bit >>= (t - 48);
            }

            fen.erase(0, fen.find(' ') + 1);
            if (fen.front() == 'w') hash ^= TT::w_key;

            fen.erase(0, fen.find(' ') + 1);

            if (fen.contains('K')) hash ^= TT::ws_key;
            if (fen.contains('Q')) hash ^= TT::wl_key;
            if (fen.contains('k')) hash ^= TT::bs_key;
            if (fen.contains('q')) hash ^= TT::bl_key;

            /*
            fen.erase(0, fen.find(' ') + 1);

            if (fen.front() != '-'){
                int file = getNumNotation(fen[0], fen[1]) % 8;
                hash ^= TT::keys[TT::ep0 + file];
            }
            */

            return Board{bp, bn, bb, br, bq, bk, wp, wn, wb, wr, wq, wk, hash};
        }
        else{
            bool white, wOO, wOOO, bOO, bOOO, ep;
            fen.erase(0, fen.find(' ') + 1);
            white = fen.front() == 'w';

            fen.erase(0, fen.find(' ') + 1);

            wOO = fen.contains('K');
            wOOO = fen.contains('Q');
            bOO = fen.contains('k');
            bOOO = fen.contains('q');

            fen.erase(0, fen.find(' ') + 1);

            ep = fen.front() != '-';

            return State{white, ep, wOO, wOOO, bOO, bOOO};
        }
    }

    static std::string b_to_fen(Board& board, State& state, Bit& ep_target){
        std::stringstream rev_fen;
        map occ = board.Occ;

        while (occ){
            Square sq_char = SquareOf(occ);
            Bit sq = 1ull << sq_char;
            if      (sq & board.WPawn)      rev_fen << "P";
            else if (sq & board.BPawn)      rev_fen << "p";
            else if (sq & board.WBishop)    rev_fen << "B";
            else if (sq & board.BBishop)    rev_fen << "b";
            else if (sq & board.WKnight)    rev_fen << "N";
            else if (sq & board.BKnight)    rev_fen << "n";
            else if (sq & board.WRook)      rev_fen << "R";
            else if (sq & board.BRook)      rev_fen << "r";
            else if (sq & board.WQueen)     rev_fen << "Q";
            else if (sq & board.BQueen)     rev_fen << "q";
            else if (sq & board.WKing)      rev_fen << "K";
            else if (sq & board.BKing)      rev_fen << "k";

            occ &= occ - 1;
            Square new_sq = SquareOf(occ);
            int empty_squares = new_sq - sq_char;
            empty_squares--;

            if (sq_char % 8 + empty_squares + 1 >= 8){
                if ((sq_char + 1) % 8) {
                    rev_fen << 8 - (sq_char + 1) % 8;
                    empty_squares -= 8 - sq_char % 8;
                }
                rev_fen << "/";
                for (int i = empty_squares / 8; i; i--) rev_fen << "8/";
            }
            if ((empty_squares + 1) % 8 && empty_squares % 8) rev_fen << empty_squares % 8;
        }

        std::string fen = rev_fen.str();
        fen.erase(fen.end() - 1);
        std::reverse(fen.begin(), fen.end());

        if (state.white_move)   fen.append(" w ");
        else                    fen.append(" b ");

        if (state.white_oo) fen.append("K");
        if (state.white_ooo) fen.append("Q");
        if (state.black_oo) fen.append("k");
        if (state.black_ooo) fen.append("q");
        if (!(state.can_castle<true>() || state.can_castle<false>())) fen.append("-");
        fen.append(" ");

        if (state.has_ep_pawn) {
            if (state.white_move)   fen.append(Misc::uci_squares[SquareOf(ep_target << 8)]); // shift
            else                    fen.append(Misc::uci_squares[SquareOf(ep_target >> 8)]); // shift
        }
        else fen.append("-");

        fen.append(" 0 1");

        return fen;
    }

    static u64 hash_position(Board& board, State& state, Bit& ep_target){
        u64 hash = 0;
        map occ = board.Occ;
        while (occ){
            Square sq_char = SquareOf(occ);
            Bit sq = 1ull << sq_char;
            if      (sq & board.WPawn)      hash ^= TT::get_key<PAWN, true>(sq_char);
            else if (sq & board.BPawn)      hash ^= TT::get_key<PAWN, false>(sq_char);
            else if (sq & board.WBishop)    hash ^= TT::get_key<BISHOP, true>(sq_char);
            else if (sq & board.BBishop)    hash ^= TT::get_key<BISHOP, false>(sq_char);
            else if (sq & board.WKnight)    hash ^= TT::get_key<KNIGHT, true>(sq_char);
            else if (sq & board.BKnight)    hash ^= TT::get_key<KNIGHT, false>(sq_char);
            else if (sq & board.WRook)      hash ^= TT::get_key<ROOK, true>(sq_char);
            else if (sq & board.BRook)      hash ^= TT::get_key<ROOK, false>(sq_char);
            else if (sq & board.WQueen)     hash ^= TT::get_key<QUEEN, true>(sq_char);
            else if (sq & board.BQueen)     hash ^= TT::get_key<QUEEN, false>(sq_char);
            else if (sq & board.WKing)      hash ^= TT::get_key<KING, true>(sq_char);
            else if (sq & board.BKing)      hash ^= TT::get_key<KING, false>(sq_char);

            occ &= occ - 1;
        }

        if (state.white_move) hash ^= TT::w_key;

        if (state.white_oo) hash ^= TT::ws_key;
        if (state.white_ooo) hash ^= TT::wl_key;
        if (state.black_oo) hash ^= TT::bs_key;
        if (state.black_ooo) hash ^= TT::bl_key;

        // ep

        return hash;
    }


    struct move_container{
        constexpr move_container(Boardstate::State s, Boardstate::Board b, Piece p, Piece promo, bool cap, CastleType ct, Bit from, Bit to, Bit ep_)
                : state(s), board(b), piece(p), promotion_to(promo), capture(cap), castle_type(ct), from(from), to(to), ep(ep_){}
        const Boardstate::State state;
        const Boardstate::Board board;
        const Piece piece;
        const Piece promotion_to;
        const bool capture;
        const CastleType castle_type;
        const Bit from;
        const Bit to;
        const bool ep;
    };

    FORCEINLINE Bit ep_state(const move_container& mc){
        if (mc.piece == PAWN && abs(static_cast<int>(SquareOf(mc.from)) - static_cast<int>(SquareOf(mc.to))) == 16)
            return mc.to;
        else
            return 0;
    }

    FORCEINLINE State change_state(const move_container& mc){
        if (mc.castle_type != CASTLE_INVALID) return mc.state.no_castles();
        if (mc.ep) return mc.state.silent_move();
        if (mc.piece == PAWN && abs(int(SquareOf(mc.from)) - int(SquareOf(mc.to))) == 16)
            return mc.state.new_ep_pawn();

        if (mc.state.white_move){
            if (mc.state.can_castle<false>() && mc.capture) {
                if (mc.state.can_castle<true>() && mc.piece == ROOK) {
                    if (State::is_rook_left<true>(mc.from)) {
                        if (State::is_rook_left<false>(mc.to)) return mc.state.no_ooo<true>().no_ooo<false>().silent_move();
                        return mc.state.no_ooo<true>();
                    } else if (State::is_rook_right<true>(mc.from)) {
                        if (State::is_rook_right<true>(mc.to)) return mc.state.no_oo<true>().no_oo<false>().silent_move();
                        return mc.state.no_oo<true>();
                    }
                }
                else if (State::is_rook_right<false>(mc.to)) return mc.state.no_oo<false>();
                else if (State::is_rook_left<false>(mc.to)) return mc.state.no_ooo<false>();
            }
            if (mc.state.can_castle<true>()) {
                if (mc.piece == KING) return mc.state.no_castles();
                else if (mc.piece == ROOK) {
                    if (State::is_rook_left<true>(mc.from)) return mc.state.no_ooo<true>();
                    else if (State::is_rook_right<true>(mc.from)) return mc.state.no_oo<true>();
                }
            }
        }
        else{
            if (mc.state.can_castle<true>() && mc.capture) {
                if (mc.state.can_castle<false>() && mc.piece == ROOK) {
                    if (State::is_rook_left<false>(mc.from)) {
                        if (State::is_rook_left<true>(mc.to)) return mc.state.no_ooo<false>().no_ooo<true>().silent_move();
                        return mc.state.no_ooo<false>();
                    } else if (State::is_rook_right<false>(mc.from)) {
                        if (State::is_rook_right<false>(mc.to)) return mc.state.no_oo<false>().no_oo<true>().silent_move();
                        return mc.state.no_oo<false>();
                    }
                }
                else if (State::is_rook_right<true>(mc.to)) return mc.state.no_oo<true>();
                else if (State::is_rook_left<true>(mc.to)) return mc.state.no_ooo<true>();
            }
            if (mc.state.can_castle<false>()) {
                if (mc.piece == KING) return mc.state.no_castles();
                else if (mc.piece == ROOK) {
                    if (State::is_rook_left<false>(mc.from)) return mc.state.no_ooo<false>();
                    else if (State::is_rook_right<false>(mc.from)) return mc.state.no_oo<false>();
                }
            }
        }
        return mc.state.silent_move();
    }

    FORCEINLINE static Board move(const move_container& mc){
        const map bp = mc.board.BPawn, bn = mc.board.BKnight, bb = mc.board.BBishop, br = mc.board.BRook, bq = mc.board.BQueen, bk = mc.board.BKing;
        const map wp = mc.board.WPawn, wn = mc.board.WKnight, wb = mc.board.WBishop, wr = mc.board.WRook, wq = mc.board.WQueen, wk = mc.board.WKing;

        if         (mc.castle_type == WHITE_OO)  return {bp, bn, bb, br, bq, bk, wp, wn, wb, wr ^ wOO_r, wq, wk ^ wOO_k, 0};
        else if    (mc.castle_type == WHITE_OOO) return {bp, bn, bb, br, bq, bk, wp, wn, wb, wr ^ wOOO_r, wq, wk ^ wOOO_k, 0};
        else if    (mc.castle_type == BLACK_OO)  return {bp, bn, bb, br ^ bOO_r, bq, bk ^ bOO_k, wp, wn, wb, wr, wq, wk, 0};
        else if    (mc.castle_type == BLACK_OOO) return {bp, bn, bb, br ^ bOOO_r, bq, bk ^ bOOO_k, wp, wn, wb, wr, wq, wk, 0};

        if (mc.ep){
            if (mc.state.white_move)    return {bp ^ (mc.to >> 8), bn, bb, br, bq, bk, wp ^ (mc.to | mc.from), wn, wb, wr, wq, wk, 0};
            else                        return {bp ^ (mc.to | mc.from), bn, bb, br, bq, bk, wp ^ (mc.to << 8), wn, wb, wr, wq, wk, 0};
        }

        const map mov = mc.from | mc.to;
        if  (mc.capture) {
            const map rem = ~mc.to;
            if  (mc.state.white_move) {
                if  (PAWN == mc.piece) {
                    if         (mc.promotion_to == QUEEN)     return {bp & rem, bn & rem, bb & rem, br & rem, bq & rem, bk, wp ^ mc.from, wn, wb, wr, wq ^ mc.to, wk, 0};
                    else if    (mc.promotion_to == ROOK)      return {bp & rem, bn & rem, bb & rem, br & rem, bq & rem, bk, wp ^ mc.from, wn, wb, wr ^ mc.to, wq, wk, 0};
                    else if    (mc.promotion_to == BISHOP)    return {bp & rem, bn & rem, bb & rem, br & rem, bq & rem, bk, wp ^ mc.from, wn, wb ^ mc.to, wr, wq, wk, 0};
                    else if    (mc.promotion_to == KNIGHT)    return {bp & rem, bn & rem, bb & rem, br & rem, bq & rem, bk, wp ^ mc.from, wn ^ mc.to, wb, wr, wq, wk, 0};
                    else                                      return {bp & rem, bn & rem, bb & rem, br & rem, bq & rem, bk, wp ^ mov, wn, wb, wr, wq, wk, 0};
                }
                if  (KNIGHT == mc.piece) return {bp & rem, bn & rem, bb & rem, br & rem, bq & rem, bk, wp, wn ^ mov, wb, wr, wq, wk, 0};
                if  (BISHOP == mc.piece) return {bp & rem, bn & rem, bb & rem, br & rem, bq & rem, bk, wp, wn, wb ^ mov, wr, wq, wk, 0};
                if  (ROOK == mc.piece) return {bp & rem, bn & rem, bb & rem, br & rem, bq & rem, bk, wp, wn, wb, wr ^ mov, wq, wk, 0};
                if  (QUEEN == mc.piece) return {bp & rem, bn & rem, bb & rem, br & rem, bq & rem, bk, wp, wn, wb, wr, wq ^ mov, wk, 0};
                if  (KING == mc.piece) return {bp & rem, bn & rem, bb & rem, br & rem, bq & rem, bk, wp, wn, wb, wr, wq, wk ^ mov, 0};
            }
            else {
                if  (PAWN == mc.piece) {
                    if         (mc.promotion_to == QUEEN)     return {bp ^ mc.from, bn, bb, br, bq ^ mc.to, bk, wp & rem, wn & rem, wb & rem, wr & rem, wq & rem, wk, 0};
                    else if    (mc.promotion_to == ROOK)      return {bp ^ mc.from, bn, bb, br ^ mc.to, bq, bk, wp & rem, wn & rem, wb & rem, wr & rem, wq & rem, wk, 0};
                    else if    (mc.promotion_to == BISHOP)    return {bp ^ mc.from, bn, bb ^ mc.to, br, bq, bk, wp & rem, wn & rem, wb & rem, wr & rem, wq & rem, wk, 0};
                    else if    (mc.promotion_to == KNIGHT)    return {bp ^ mc.from, bn ^ mc.to, bb, br, bq, bk, wp & rem, wn & rem, wb & rem, wr & rem, wq & rem, wk, 0};
                    else                                      return {bp ^ mov, bn, bb, br, bq, bk, wp & rem, wn & rem, wb & rem, wr & rem, wq & rem, wk, 0};
                }
                if  (KNIGHT == mc.piece) return {bp, bn ^ mov, bb, br, bq, bk, wp & rem, wn & rem, wb & rem, wr & rem, wq & rem, wk, 0};
                if  (BISHOP == mc.piece) return {bp, bn, bb ^ mov, br, bq, bk, wp & rem, wn & rem, wb & rem, wr & rem, wq & rem, wk, 0};
                if  (ROOK == mc.piece) return {bp, bn, bb, br ^ mov, bq, bk, wp & rem, wn & rem, wb & rem, wr & rem, wq & rem, wk, 0};
                if  (QUEEN == mc.piece) return {bp, bn, bb, br, bq ^ mov, bk, wp & rem, wn & rem, wb & rem, wr & rem, wq & rem, wk, 0};
                if  (KING == mc.piece) return {bp, bn, bb, br, bq, bk ^ mov, wp & rem, wn & rem, wb & rem, wr & rem, wq & rem, wk, 0};
            }
        }
        else {
            if  (mc.state.white_move) {
                if  (PAWN == mc.piece) {
                    if         (mc.promotion_to == QUEEN)     return {bp, bn, bb, br, bq, bk, wp ^ mc.from, wn, wb, wr, wq ^ mc.to, wk, 0};
                    else if    (mc.promotion_to == ROOK)      return {bp, bn, bb, br, bq, bk, wp ^ mc.from, wn, wb, wr ^ mc.to, wq, wk, 0};
                    else if    (mc.promotion_to == BISHOP)    return {bp, bn, bb, br, bq, bk, wp ^ mc.from, wn, wb ^ mc.to, wr, wq, wk, 0};
                    else if    (mc.promotion_to == KNIGHT)    return {bp, bn, bb, br, bq, bk, wp ^ mc.from, wn ^ mc.to, wb, wr, wq, wk, 0};
                    else                                      return {bp, bn, bb, br, bq, bk, wp ^ mov, wn, wb, wr, wq, wk, 0};
                }
                if  (KNIGHT == mc.piece) return {bp, bn, bb, br, bq, bk, wp, wn ^ mov, wb, wr, wq, wk, 0};
                if  (BISHOP == mc.piece) return {bp, bn, bb, br, bq, bk, wp, wn, wb ^ mov, wr, wq, wk, 0};
                if  (ROOK == mc.piece) return {bp, bn, bb, br, bq, bk, wp, wn, wb, wr ^ mov, wq, wk, 0};
                if  (QUEEN == mc.piece) return {bp, bn, bb, br, bq, bk, wp, wn, wb, wr, wq ^ mov, wk, 0};
                if  (KING == mc.piece) return {bp, bn, bb, br, bq, bk, wp, wn, wb, wr, wq, wk ^ mov, 0};
            }
            else {
                if  (PAWN == mc.piece) {
                    if         (mc.promotion_to == QUEEN)     return {bp ^ mc.from, bn, bb, br, bq ^ mc.to, bk, wp, wn, wb, wr, wq, wk, 0};
                    else if    (mc.promotion_to == ROOK)      return {bp ^ mc.from, bn, bb, br ^ mc.to, bq, bk, wp, wn, wb, wr, wq, wk, 0};
                    else if    (mc.promotion_to == BISHOP)    return {bp ^ mc.from, bn, bb ^ mc.to, br, bq, bk, wp, wn, wb, wr, wq, wk, 0};
                    else if    (mc.promotion_to == KNIGHT)    return {bp ^ mc.from, bn ^ mc.to, bb, br, bq, bk, wp, wn, wb, wr, wq, wk, 0};
                    else                                      return {bp ^ mov, bn, bb, br, bq, bk, wp, wn, wb, wr, wq, wk, 0};
                }
                if  (KNIGHT == mc.piece) return {bp, bn ^ mov, bb, br, bq, bk, wp, wn, wb, wr, wq, wk, 0};
                if  (BISHOP == mc.piece) return {bp, bn, bb ^ mov, br, bq, bk, wp, wn, wb, wr, wq, wk, 0};
                if  (ROOK == mc.piece) return {bp, bn, bb, br ^ mov, bq, bk, wp, wn, wb, wr, wq, wk, 0};
                if  (QUEEN == mc.piece) return {bp, bn, bb, br, bq ^ mov, bk, wp, wn, wb, wr, wq, wk, 0};
                if  (KING == mc.piece) return {bp, bn, bb, br, bq, bk ^ mov, wp, wn, wb, wr, wq, wk, 0};
            }
        }
        return {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    }

}

#pragma clang diagnostic pop