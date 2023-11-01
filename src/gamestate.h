#pragma clang diagnostic push
#pragma ide diagnostic ignored "modernize-use-nodiscard"
#pragma once
#include "variables.h"
#include "intrin.h"

#define getNumNotation(char1, char2) (7 - (char1 - 97) + 8 * (char2 - 49))

namespace ZeroLogic::Boardstate {

    class State{
        static constexpr Bit white_rook_left = 0x80;
        static constexpr Bit white_rook_right = 1;
        static constexpr Bit black_rook_left = white_rook_left << 56;
        static constexpr Bit black_rook_right = white_rook_right << 56;
    public:
        static constexpr map wk_path = 0x6;
        static constexpr map wq_path_k = 0x30;
        static constexpr map wq_path_r = 0x70;
        static constexpr map bk_path = wk_path << 56;
        static constexpr map bq_path_k = wq_path_k << 56;
        static constexpr map bq_path_r = wq_path_r << 56;
        constexpr State(bool white, bool ep, bool wOO, bool wOOO, bool bOO, bool bOOO) :
            white_move(white), has_ep_pawn(ep), white_oo(wOO), white_ooo(wOOO), black_oo(bOO), black_ooo(bOOO)
        {}
        constexpr explicit State(char token) :
            white_move(token & 0b100000), has_ep_pawn(token & 0b10000), white_oo(token & 0b1000),
            white_ooo(token & 0b100), black_oo(token & 0b10), black_ooo(token & 1)
        {}
        const bool white_move;
        const bool has_ep_pawn;
        const bool white_oo;
        const bool white_ooo;
        const bool black_oo;
        const bool black_ooo;

        static constexpr inline State normal(){return {true, false, true, true, true, true};}
        constexpr inline State silent_move() const {return {!white_move, false, white_oo, white_ooo, black_oo, black_ooo};}
        constexpr inline State no_oo_own() const {
            if (white_move)  return {false, false, false, white_ooo, black_oo, black_ooo};
            else                return {true, false, white_oo, white_ooo, false, black_ooo};
        }
        constexpr inline State no_ooo_own() const {
            if (white_move)  return {false, false, white_oo, false, black_oo, black_ooo};
            else                return {true, false, white_oo, white_ooo, black_oo, false};
        }
        constexpr inline State no_oo_enemy() const {
            if (white_move)  return {false, false, white_oo, white_ooo, false, black_ooo};
            else                return {true, false, false, white_ooo, black_oo, black_ooo};
        }
        constexpr inline State no_ooo_enemy() const {
            if (white_move)  return {false, false, white_oo, white_ooo, black_oo, false};
            else                return {true, false, white_oo, false, black_oo, black_ooo};
        }
        constexpr inline State no_oo_all() const {return {!white_move, false, false, white_ooo, false, black_ooo};}
        constexpr inline State no_ooo_all() const {return {!white_move, false, white_oo, false, black_oo, false};}

        constexpr inline State no_castles_own() const {
            if (white_move)  return {false, false, false, false, black_oo, black_ooo};
            else             return {true, false, white_oo, white_ooo, false, false};
        }
        constexpr inline State new_ep_pawn() const{
            return {!white_move, true, white_oo, white_ooo, black_oo, black_ooo};
        }

        constexpr inline bool we_can_castle() const{
            if (white_move)  return white_oo | white_ooo;
            else                return black_oo | black_ooo;
        }
        constexpr inline bool enemy_can_castle() const{
            if (!white_move) return white_oo | white_ooo;
            else                return black_oo | black_ooo;
        }
        constexpr inline bool we_can_castle_long() const{
            if (white_move) return white_ooo;
            else            return black_ooo;
        }
        constexpr inline bool we_can_castle_short() const{
            if (white_move) return white_oo;
            else            return black_oo;
        }
        constexpr inline bool is_enemy_rook_l(Bit s) const{
            if (white_move)  return s & black_rook_left;
            else                return s & white_rook_left;
        }
        constexpr inline bool is_enemy_rook_r(Bit s) const{
            if (white_move)  return s & black_rook_right;
            else                return s & white_rook_right;
        }
        constexpr inline bool is_own_rook_l(Bit s) const{
            if (white_move)  return s & white_rook_left;
            else                return s & black_rook_left;
        }
        constexpr inline bool is_own_rook_r(Bit s) const{
            if (white_move)  return s & white_rook_right;
            else                return s & black_rook_right;
        }

    };

    template <bool white_move>
     FORCEINLINE bool short_legal(const map occ, const map kingban){
        if (white_move)   return !(State::wk_path & (occ | kingban));
        else              return !(State::bk_path & (occ | kingban));
    }
    template <bool white_move>
    FORCEINLINE bool long_legal(const map occ, const map kingban){
        if (white_move)   return !((State::wq_path_k & kingban) | (State::wq_path_r & occ));
        else              return !((State::bq_path_k & kingban) | (State::bq_path_r & occ));
    }

    template <State state, bool castling>
    COMPILETIME State change_state(){ // for castling and double pawn push
        if constexpr (castling) return state.no_castles_own();
        else                    return state.new_ep_pawn();
    }

	struct Board{
        const map BPawn, BKnight, BBishop, BRook, BQueen, BKing;
        const map WPawn, WKnight, WBishop, WRook, WQueen, WKing;
        const map Black, White, Occ;

        constexpr Board(
                map bp, map bn, map bb, map br, map bq, map bk,
                map wp, map wn, map wb, map wr, map wq, map wk) :
                BPawn(bp), BKnight(bn), BBishop(bb), BRook(br), BQueen(bq), BKing(bk),
                WPawn(wp), WKnight(wn), WBishop(wb), WRook(wr), WQueen(wq), WKing(wk),
                Black(bp | bn | bb | br | bq | bk),
                White(wp | wn | wb | wr | wq | wk),
                Occ(Black | White)
        {}

    };

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
            Bit index_bit = 1ull << 63;

            std::string position_fen = fen.substr(0, fen.find(' '));
            for (char t: position_fen) {
                if (t == '/') continue;
                if (t - 60 > 0) {
                    switch (t) {
                        case 'p': bp |= index_bit; break;
                        case 'P': wp |= index_bit; break;
                        case 'r': br |= index_bit; break;
                        case 'R': wr |= index_bit; break;
                        case 'b': bb |= index_bit; break;
                        case 'B': wb |= index_bit; break;
                        case 'n': bn |= index_bit; break;
                        case 'N': wn |= index_bit; break;
                        case 'q': bq |= index_bit; break;
                        case 'Q': wq |= index_bit; break;
                        case 'k': bk |= index_bit; break;
                        case 'K': wk |= index_bit; break;
                        default:
                            std::cout << "invalid fen: '" << fen << "'" << std::endl;
                            return Board{bp, bn, bb, br, bq, bk, wp, wn, wb, wr, wq, wk};
                    }
                    index_bit >>= 1;
                } else index_bit >>= (t - 48);
            }

            return Board{bp, bn, bb, br, bq, bk, wp, wn, wb, wr, wq, wk};
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

    template <bool white_move>
    FORCEINLINE static Board move(const Board& old_board, map mov, Bit ep_target){ // for ep
        const map bp = old_board.BPawn, bn = old_board.BKnight, bb = old_board.BBishop, br = old_board.BRook, bq = old_board.BQueen, bk = old_board.BKing;
        const map wp = old_board.WPawn, wn = old_board.WKnight, wb = old_board.WBishop, wr = old_board.WRook, wq = old_board.WQueen, wk = old_board.WKing;
        if constexpr (white_move)
            return {bp ^ ep_target, bn, bb, br, bq, bk, wp ^ mov, wn, wb, wr, wq, wk};
        else
            return {bp ^ mov, bn, bb, br, bq, bk, wp ^ ep_target, wn, wb, wr, wq, wk};
    }

    template <Piece piece, Piece promotion_to, bool white_move, bool taking>
    FORCEINLINE static Board move(const Board& old_board, Bit from, Bit to){
        const map bp = old_board.BPawn, bn = old_board.BKnight, bb = old_board.BBishop, br = old_board.BRook, bq = old_board.BQueen, bk = old_board.BKing;
        const map wp = old_board.WPawn, wn = old_board.WKnight, wb = old_board.WBishop, wr = old_board.WRook, wq = old_board.WQueen, wk = old_board.WKing;

        const map mov = from | to;
        if constexpr (taking) {
            const map rem = ~to;
            if constexpr (white_move) {
                if constexpr (PAWN == piece) {
                    if constexpr        (promotion_to == QUEEN)     return {bp & rem, bn & rem, bb & rem, br & rem, bq & rem, bk, wp ^ from, wn, wb, wr, wq ^ to, wk};
                    else if constexpr   (promotion_to == ROOK)      return {bp & rem, bn & rem, bb & rem, br & rem, bq & rem, bk, wp ^ from, wn, wb, wr ^ to, wq, wk};
                    else if constexpr   (promotion_to == BISHOP)    return {bp & rem, bn & rem, bb & rem, br & rem, bq & rem, bk, wp ^ from, wn, wb ^ to, wr, wq, wk};
                    else if constexpr   (promotion_to == KNIGHT)    return {bp & rem, bn & rem, bb & rem, br & rem, bq & rem, bk, wp ^ from, wn ^ to, wb, wr, wq, wk};
                    else                                            return {bp & rem, bn & rem, bb & rem, br & rem, bq & rem, bk, wp ^ mov, wn, wb, wr, wq, wk};
                }
                if constexpr (KNIGHT == piece)
                    return {bp & rem, bn & rem, bb & rem, br & rem, bq & rem, bk, wp, wn ^ mov, wb, wr, wq, wk};
                if constexpr (BISHOP == piece)
                    return {bp & rem, bn & rem, bb & rem, br & rem, bq & rem, bk, wp, wn, wb ^ mov, wr, wq, wk};
                if constexpr (ROOK == piece)
                    return {bp & rem, bn & rem, bb & rem, br & rem, bq & rem, bk, wp, wn, wb, wr ^ mov, wq, wk};
                if constexpr (QUEEN == piece)
                    return {bp & rem, bn & rem, bb & rem, br & rem, bq & rem, bk, wp, wn, wb, wr, wq ^ mov, wk};
                if constexpr (KING == piece)
                    return {bp & rem, bn & rem, bb & rem, br & rem, bq & rem, bk, wp, wn, wb, wr, wq, wk ^ mov};
            }
            else {
                if constexpr (PAWN == piece) {
                    if constexpr        (promotion_to == QUEEN)     return {bp ^ from, bn, bb, br, bq ^ to, bk, wp & rem, wn & rem, wb & rem, wr & rem, wq & rem, wk};
                    else if constexpr   (promotion_to == ROOK)      return {bp ^ from, bn, bb, br ^ to, bq, bk, wp & rem, wn & rem, wb & rem, wr & rem, wq & rem, wk};
                    else if constexpr   (promotion_to == BISHOP)    return {bp ^ from, bn, bb ^ to, br, bq, bk, wp & rem, wn & rem, wb & rem, wr & rem, wq & rem, wk};
                    else if constexpr   (promotion_to == KNIGHT)    return {bp ^ from, bn ^ to, bb, br, bq, bk, wp & rem, wn & rem, wb & rem, wr & rem, wq & rem, wk};
                    else                                            return {bp ^ mov, bn, bb, br, bq, bk, wp & rem, wn & rem, wb & rem, wr & rem, wq & rem, wk};
                }
                if constexpr (KNIGHT == piece)
                    return {bp, bn ^ mov, bb, br, bq, bk, wp & rem, wn & rem, wb & rem, wr & rem, wq & rem, wk};
                if constexpr (BISHOP == piece)
                    return {bp, bn, bb ^ mov, br, bq, bk, wp & rem, wn & rem, wb & rem, wr & rem, wq & rem, wk};
                if constexpr (ROOK == piece)
                    return {bp, bn, bb, br ^ mov, bq, bk, wp & rem, wn & rem, wb & rem, wr & rem, wq & rem, wk};
                if constexpr (QUEEN == piece)
                    return {bp, bn, bb, br, bq ^ mov, bk, wp & rem, wn & rem, wb & rem, wr & rem, wq & rem, wk};
                if constexpr (KING == piece)
                return {bp, bn, bb, br, bq, bk ^ mov, wp & rem, wn & rem, wb & rem, wr & rem, wq & rem, wk};
            }
        }
        else {
            if constexpr (white_move) {
                if constexpr (PAWN == piece) {
                    if constexpr        (promotion_to == QUEEN)     return {bp, bn, bb, br, bq, bk, wp ^ from, wn, wb, wr, wq ^ to, wk};
                    else if constexpr   (promotion_to == ROOK)      return {bp, bn, bb, br, bq, bk, wp ^ from, wn, wb, wr ^ to, wq, wk};
                    else if constexpr   (promotion_to == BISHOP)    return {bp, bn, bb, br, bq, bk, wp ^ from, wn, wb ^ to, wr, wq, wk};
                    else if constexpr   (promotion_to == KNIGHT)    return {bp, bn, bb, br, bq, bk, wp ^ from, wn ^ to, wb, wr, wq, wk};
                    else                                            return {bp, bn, bb, br, bq, bk, wp ^ mov, wn, wb, wr, wq, wk};
                }
                if constexpr (KNIGHT == piece) return {bp, bn, bb, br, bq, bk, wp, wn ^ mov, wb, wr, wq, wk};
                if constexpr (BISHOP == piece) return {bp, bn, bb, br, bq, bk, wp, wn, wb ^ mov, wr, wq, wk};
                if constexpr (ROOK == piece) return {bp, bn, bb, br, bq, bk, wp, wn, wb, wr ^ mov, wq, wk};
                if constexpr (QUEEN == piece) return {bp, bn, bb, br, bq, bk, wp, wn, wb, wr, wq ^ mov, wk};
                if constexpr (KING == piece) return {bp, bn, bb, br, bq, bk, wp, wn, wb, wr, wq, wk ^ mov};
            }
            else {
                if constexpr (PAWN == piece) {
                    if constexpr        (promotion_to == QUEEN)     return {bp ^ from, bn, bb, br, bq ^ to, bk, wp, wn, wb, wr, wq, wk};
                    else if constexpr   (promotion_to == ROOK)      return {bp ^ from, bn, bb, br ^ to, bq, bk, wp, wn, wb, wr, wq, wk};
                    else if constexpr   (promotion_to == BISHOP)    return {bp ^ from, bn, bb ^ to, br, bq, bk, wp, wn, wb, wr, wq, wk};
                    else if constexpr   (promotion_to == KNIGHT)    return {bp ^ from, bn ^ to, bb, br, bq, bk, wp, wn, wb, wr, wq, wk};
                    else                                            return {bp ^ mov, bn, bb, br, bq, bk, wp, wn, wb, wr, wq, wk};
                }
                if constexpr (KNIGHT == piece) return {bp, bn ^ mov, bb, br, bq, bk, wp, wn, wb, wr, wq, wk};
                if constexpr (BISHOP == piece) return {bp, bn, bb ^ mov, br, bq, bk, wp, wn, wb, wr, wq, wk};
                if constexpr (ROOK == piece) return {bp, bn, bb, br ^ mov, bq, bk, wp, wn, wb, wr, wq, wk};
                if constexpr (QUEEN == piece) return {bp, bn, bb, br, bq ^ mov, bk, wp, wn, wb, wr, wq, wk};
                if constexpr (KING == piece) return {bp, bn, bb, br, bq, bk ^ mov, wp, wn, wb, wr, wq, wk};
            }
        }
    }


    // uci state changers
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
        if (mc.piece == PAWN && abs(static_cast<int>(_tzcnt_u64(mc.from)) - static_cast<int>(_tzcnt_u64(mc.to))) == 16)
            return mc.to;
        else
            return 0;
    }

    FORCEINLINE State change_state(const move_container& mc){
        if (mc.castle_type != CASTLE_INVALID) return mc.state.no_castles_own();
        if (mc.ep) return mc.state.silent_move();
        if (mc.piece == PAWN && abs(static_cast<int>(_tzcnt_u64(mc.from)) - static_cast<int>(_tzcnt_u64(mc.to))) == 16)
            return mc.state.new_ep_pawn();
        
        if (mc.state.enemy_can_castle() && mc.capture) {
            if (mc.state.we_can_castle() && mc.piece == ROOK) {
                if (mc.state.is_own_rook_l(mc.from)) {
                    if (mc.state.is_enemy_rook_l(mc.to)) return mc.state.no_ooo_all();
                    return mc.state.no_ooo_own();
                } else if (mc.state.is_own_rook_r(mc.from)) {
                    if (mc.state.is_enemy_rook_r(mc.to)) return mc.state.no_oo_all();
                    return mc.state.no_oo_own();
                }
            }
            else if (mc.state.is_enemy_rook_r(mc.to)) return mc.state.no_oo_enemy();
            else if (mc.state.is_enemy_rook_l(mc.to)) return mc.state.no_ooo_enemy();
        }
        if (mc.state.we_can_castle()) {
            if (mc.piece == KING) return mc.state.no_castles_own();
            else if (mc.piece == ROOK) {
                if (mc.state.is_own_rook_l(mc.from)) return mc.state.no_ooo_own();
                else if (mc.state.is_own_rook_r(mc.from)) return mc.state.no_oo_own();
            }
        }
        return mc.state.silent_move();
    }

    FORCEINLINE static Board move(const move_container& mc){
        const map bp = mc.board.BPawn, bn = mc.board.BKnight, bb = mc.board.BBishop, br = mc.board.BRook, bq = mc.board.BQueen, bk = mc.board.BKing;
        const map wp = mc.board.WPawn, wn = mc.board.WKnight, wb = mc.board.WBishop, wr = mc.board.WRook, wq = mc.board.WQueen, wk = mc.board.WKing;

        if         (mc.castle_type == WHITE_OO)  return {bp, bn, bb, br, bq, bk, wp, wn, wb, wr ^ wOO_r, wq, wk ^ wOO_k};
        else if    (mc.castle_type == WHITE_OOO) return {bp, bn, bb, br, bq, bk, wp, wn, wb, wr ^ wOOO_r, wq, wk ^ wOOO_k};
        else if    (mc.castle_type == BLACK_OO)  return {bp, bn, bb, br ^ bOO_r, bq, bk ^ bOO_k, wp, wn, wb, wr, wq, wk};
        else if    (mc.castle_type == BLACK_OOO) return {bp, bn, bb, br ^ bOOO_r, bq, bk ^ bOOO_k, wp, wn, wb, wr, wq, wk};

        if (mc.ep){
            if (mc.state.white_move)
                return {bp ^ (mc.to >> 8), bn, bb, br, bq, bk, wp ^ (mc.to | mc.from), wn, wb, wr, wq, wk};
            else
                return {bp ^ (mc.to | mc.from), bn, bb, br, bq, bk, wp ^ (mc.to << 8), wn, wb, wr, wq, wk};
        }
        
        const map mov = mc.from | mc.to;
        if  (mc.capture) {
            const map rem = ~mc.to;
            if  (mc.state.white_move) {
                if  (PAWN == mc.piece) {
                    if         (mc.promotion_to == QUEEN)     return {bp & rem, bn & rem, bb & rem, br & rem, bq & rem, bk, wp ^ mc.from, wn, wb, wr, wq ^ mc.to, wk};
                    else if    (mc.promotion_to == ROOK)      return {bp & rem, bn & rem, bb & rem, br & rem, bq & rem, bk, wp ^ mc.from, wn, wb, wr ^ mc.to, wq, wk};
                    else if    (mc.promotion_to == BISHOP)    return {bp & rem, bn & rem, bb & rem, br & rem, bq & rem, bk, wp ^ mc.from, wn, wb ^ mc.to, wr, wq, wk};
                    else if    (mc.promotion_to == KNIGHT)    return {bp & rem, bn & rem, bb & rem, br & rem, bq & rem, bk, wp ^ mc.from, wn ^ mc.to, wb, wr, wq, wk};
                    else                                      return {bp & rem, bn & rem, bb & rem, br & rem, bq & rem, bk, wp ^ mov, wn, wb, wr, wq, wk};
                }
                if  (KNIGHT == mc.piece)
                    return {bp & rem, bn & rem, bb & rem, br & rem, bq & rem, bk, wp, wn ^ mov, wb, wr, wq, wk};
                if  (BISHOP == mc.piece)
                    return {bp & rem, bn & rem, bb & rem, br & rem, bq & rem, bk, wp, wn, wb ^ mov, wr, wq, wk};
                if  (ROOK == mc.piece)
                    return {bp & rem, bn & rem, bb & rem, br & rem, bq & rem, bk, wp, wn, wb, wr ^ mov, wq, wk};
                if  (QUEEN == mc.piece)
                    return {bp & rem, bn & rem, bb & rem, br & rem, bq & rem, bk, wp, wn, wb, wr, wq ^ mov, wk};
                if  (KING == mc.piece)
                    return {bp & rem, bn & rem, bb & rem, br & rem, bq & rem, bk, wp, wn, wb, wr, wq, wk ^ mov};
            }
            else {
                if  (PAWN == mc.piece) {
                    if         (mc.promotion_to == QUEEN)     return {bp ^ mc.from, bn, bb, br, bq ^ mc.to, bk, wp & rem, wn & rem, wb & rem, wr & rem, wq & rem, wk};
                    else if    (mc.promotion_to == ROOK)      return {bp ^ mc.from, bn, bb, br ^ mc.to, bq, bk, wp & rem, wn & rem, wb & rem, wr & rem, wq & rem, wk};
                    else if    (mc.promotion_to == BISHOP)    return {bp ^ mc.from, bn, bb ^ mc.to, br, bq, bk, wp & rem, wn & rem, wb & rem, wr & rem, wq & rem, wk};
                    else if    (mc.promotion_to == KNIGHT)    return {bp ^ mc.from, bn ^ mc.to, bb, br, bq, bk, wp & rem, wn & rem, wb & rem, wr & rem, wq & rem, wk};
                    else                                      return {bp ^ mov, bn, bb, br, bq, bk, wp & rem, wn & rem, wb & rem, wr & rem, wq & rem, wk};
                }
                if  (KNIGHT == mc.piece)
                    return {bp, bn ^ mov, bb, br, bq, bk, wp & rem, wn & rem, wb & rem, wr & rem, wq & rem, wk};
                if  (BISHOP == mc.piece)
                    return {bp, bn, bb ^ mov, br, bq, bk, wp & rem, wn & rem, wb & rem, wr & rem, wq & rem, wk};
                if  (ROOK == mc.piece)
                    return {bp, bn, bb, br ^ mov, bq, bk, wp & rem, wn & rem, wb & rem, wr & rem, wq & rem, wk};
                if  (QUEEN == mc.piece)
                    return {bp, bn, bb, br, bq ^ mov, bk, wp & rem, wn & rem, wb & rem, wr & rem, wq & rem, wk};
                if  (KING == mc.piece)
                return {bp, bn, bb, br, bq, bk ^ mov, wp & rem, wn & rem, wb & rem, wr & rem, wq & rem, wk};
            }
        }
        else {
            if  (mc.state.white_move) {
                if  (PAWN == mc.piece) {
                    if         (mc.promotion_to == QUEEN)     return {bp, bn, bb, br, bq, bk, wp ^ mc.from, wn, wb, wr, wq ^ mc.to, wk};
                    else if    (mc.promotion_to == ROOK)      return {bp, bn, bb, br, bq, bk, wp ^ mc.from, wn, wb, wr ^ mc.to, wq, wk};
                    else if    (mc.promotion_to == BISHOP)    return {bp, bn, bb, br, bq, bk, wp ^ mc.from, wn, wb ^ mc.to, wr, wq, wk};
                    else if    (mc.promotion_to == KNIGHT)    return {bp, bn, bb, br, bq, bk, wp ^ mc.from, wn ^ mc.to, wb, wr, wq, wk};
                    else                                      return {bp, bn, bb, br, bq, bk, wp ^ mov, wn, wb, wr, wq, wk};
                }
                if  (KNIGHT == mc.piece) return {bp, bn, bb, br, bq, bk, wp, wn ^ mov, wb, wr, wq, wk};
                if  (BISHOP == mc.piece) return {bp, bn, bb, br, bq, bk, wp, wn, wb ^ mov, wr, wq, wk};
                if  (ROOK == mc.piece) return {bp, bn, bb, br, bq, bk, wp, wn, wb, wr ^ mov, wq, wk};
                if  (QUEEN == mc.piece) return {bp, bn, bb, br, bq, bk, wp, wn, wb, wr, wq ^ mov, wk};
                if  (KING == mc.piece) return {bp, bn, bb, br, bq, bk, wp, wn, wb, wr, wq, wk ^ mov};
            }
            else {
                if  (PAWN == mc.piece) {
                    if         (mc.promotion_to == QUEEN)     return {bp ^ mc.from, bn, bb, br, bq ^ mc.to, bk, wp, wn, wb, wr, wq, wk};
                    else if    (mc.promotion_to == ROOK)      return {bp ^ mc.from, bn, bb, br ^ mc.to, bq, bk, wp, wn, wb, wr, wq, wk};
                    else if    (mc.promotion_to == BISHOP)    return {bp ^ mc.from, bn, bb ^ mc.to, br, bq, bk, wp, wn, wb, wr, wq, wk};
                    else if    (mc.promotion_to == KNIGHT)    return {bp ^ mc.from, bn ^ mc.to, bb, br, bq, bk, wp, wn, wb, wr, wq, wk};
                    else                                      return {bp ^ mov, bn, bb, br, bq, bk, wp, wn, wb, wr, wq, wk};
                }
                if  (KNIGHT == mc.piece) return {bp, bn ^ mov, bb, br, bq, bk, wp, wn, wb, wr, wq, wk};
                if  (BISHOP == mc.piece) return {bp, bn, bb ^ mov, br, bq, bk, wp, wn, wb, wr, wq, wk};
                if  (ROOK == mc.piece) return {bp, bn, bb, br ^ mov, bq, bk, wp, wn, wb, wr, wq, wk};
                if  (QUEEN == mc.piece) return {bp, bn, bb, br, bq ^ mov, bk, wp, wn, wb, wr, wq, wk};
                if  (KING == mc.piece) return {bp, bn, bb, br, bq, bk ^ mov, wp, wn, wb, wr, wq, wk};
            }
        }
        return {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    }
    // end of uci state changers

    template <CastleType castle_type>
    FORCEINLINE static Board move(const Board& old_board){
        const map bp = old_board.BPawn, bn = old_board.BKnight, bb = old_board.BBishop, br = old_board.BRook, bq = old_board.BQueen, bk = old_board.BKing;
        const map wp = old_board.WPawn, wn = old_board.WKnight, wb = old_board.WBishop, wr = old_board.WRook, wq = old_board.WQueen, wk = old_board.WKing;
        if constexpr        (castle_type == WHITE_OO)  return {bp, bn, bb, br, bq, bk, wp, wn, wb, wr ^ wOO_r, wq, wk ^ wOO_k};
        else if constexpr   (castle_type == WHITE_OOO) return {bp, bn, bb, br, bq, bk, wp, wn, wb, wr ^ wOOO_r, wq, wk ^ wOOO_k};
        else if constexpr   (castle_type == BLACK_OO)  return {bp, bn, bb, br ^ bOO_r, bq, bk ^ bOO_k, wp, wn, wb, wr, wq, wk};
        else if constexpr   (castle_type == BLACK_OOO) return {bp, bn, bb, br ^ bOOO_r, bq, bk ^ bOOO_k, wp, wn, wb, wr, wq, wk};
    }

    // helper functions
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
    COMPILETIME map Pawns(const Board& board){
        if constexpr (white)    return board.WPawn;
        else                    return board.BPawn;
    }
}

#pragma clang diagnostic pop