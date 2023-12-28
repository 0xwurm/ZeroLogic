#pragma once
#include <sstream>

namespace ZeroLogic::Misc{

    template <CastleType type>
    static std::string _uci_move(){
        return uci_castles[type];
    }

    template <Piece promotion_to>
    static std::string _uci_move(Square from, Square to) {
        if constexpr (promotion_to == PIECE_INVALID) return uci_squares[from] + uci_squares[to];
        else return uci_squares[from] + uci_squares[to] + uci_promotion[promotion_to - 1];
    }

    static FORCEINLINE Square Mfrom(Move enc){
        return enc & 0b111111;
    }
    static FORCEINLINE Square Mto(Move enc){
        return enc >> 10;
    }
    static FORCEINLINE u16 Mflag(Move enc){
        return (enc >> 6) & 0b1111;
    }

    static std::string uci_move(Move enc){

        Square from = Mfrom(enc);
        Square to = Mto(enc);
        u16 flag = Mflag(enc);

        if (flag & 0b111){
            flag &= 0b111;
            if (flag == QUEEN)    return _uci_move<QUEEN>(from, to);
            if (flag == ROOK)     return _uci_move<ROOK>(from, to);
            if (flag == BISHOP)   return _uci_move<BISHOP>(from, to);
            if (flag == KNIGHT)   return _uci_move<KNIGHT>(from, to);
        }

        if (flag & 0b1000) {
            if (from == 3 && to == 1)   return _uci_move<WHITE_OO>();
            if (from == 3 && to == 5)   return _uci_move<WHITE_OOO>();
            if (from == 59 && to == 57) return _uci_move<BLACK_OO>();
            if (from == 59 && to == 61) return _uci_move<BLACK_OOO>();
        }

        return _uci_move<PIECE_INVALID>(from, to);
    }

    struct boardstate{
            Boardstate::Board board;
            Boardstate::State state;
            Bit ep_target;
        };

    static boardstate noInfo_move(boardstate& bdst, u8 from, u8 to, char promotion){

        u64 hash_change{};
        if (bdst.ep_target) hash_change = ZeroLogic::TT::keys[ZeroLogic::TT::ep0 + (SquareOf(bdst.ep_target) % 8)];
        Bit fromsq = 1ull << from, tosq = 1ull << to;
        bool capture = tosq & bdst.board.Occ, nepp = false, ep = false;
        Piece promotion_to = PIECE_INVALID;

        // special pawn moves
        if (fromsq & (bdst.board.WPawn | bdst.board.BPawn)){
            if ((abs(from - to) == 7 || abs(from - to) == 9) && !capture) // ep
                ep = true;
            else if (abs(from - to) == 16){ // pp
                hash_change ^= TT::keys[TT::ep0 + (to % 8)];
                nepp = true;
            }
            else if (tosq & (EIGHTH_RANK | FIRST_RANK)){ // promotion
                if      (promotion == 'q') promotion_to = QUEEN;
                else if (promotion == 'r') promotion_to = ROOK;
                else if (promotion == 'n') promotion_to = KNIGHT;
                else if (promotion == 'b') promotion_to = BISHOP;
            }
        }

        // castles
        if (bdst.board.WKing & 0b1000ull) {
            if      (from == 3 && to == 1) return {Boardstate::castle_move<WHITE_OO, true>(bdst.board, hash_change), bdst.state.no_castles(), 0};
            else if (from == 3 && to == 5) return {Boardstate::castle_move<WHITE_OOO, true>(bdst.board, hash_change), bdst.state.no_castles(), 0};
        }
        if (bdst.board.BKing & (0b1000ull << 56)){
            if      (from == 59 && to == 57) return {Boardstate::castle_move<BLACK_OO, true>(bdst.board, hash_change), bdst.state.no_castles(), 0};
            else if (from == 59 && to == 61) return {Boardstate::castle_move<BLACK_OOO, true>(bdst.board, hash_change), bdst.state.no_castles(), 0};
        }

        if (ep) {
            if (bdst.state.white_move)   return {Boardstate::ep_move<true>(bdst.board, fromsq | tosq, bdst.ep_target), bdst.state.silent_move(), 0};
            else                    return {Boardstate::ep_move<false>(bdst.board, fromsq | tosq, bdst.ep_target), bdst.state.silent_move(), 0};
        }

        Boardstate::Board bd = Boardstate::noT_move(bdst.board, fromsq, tosq, bdst.state.white_move, capture, promotion_to, hash_change);
        if (nepp)                                           return {bd, bdst.state.new_ep_pawn(), tosq};
        if (fromsq & (bdst.board.WKing | bdst.board.BKing)) return {bd, bdst.state.no_castles(), 0};
        if (fromsq & (bdst.board.WRook | bdst.board.BKing)) {
            if ((bdst.state.white_move) ? Boardstate::State::is_rook_left<true>(fromsq) : Boardstate::State::is_rook_left<false>(fromsq))
                return {bd, bdst.state.no_ooo(), 0};
            if ((bdst.state.white_move) ? Boardstate::State::is_rook_right<true>(fromsq) : Boardstate::State::is_rook_right<false>(fromsq))
                return {bd, bdst.state.no_oo(), 0};
        }
        return {bd, bdst.state.silent_move(), 0};
    }

    static Bit fen(std::string fen, bool white_move){
        fen.erase(0, fen.find(' ') + 1);
        fen.erase(0, fen.find(' ') + 1);
        fen.erase(0, fen.find(' ') + 1);
        if (fen.front() == '-') return 0;
        else if (white_move)    return 1ull << (getNumNotation(fen[0], fen[1]) - 8);
        else                    return 1ull << (getNumNotation(fen[0], fen[1]) + 8);
    }

    template <bool board>
    static auto fen(std::string fen){
        if constexpr (board) {
            map bp{}, bn{}, bb{}, br{}, bq{}, bk{};
            map wp{}, wn{}, wb{}, wr{}, wq{}, wk{};
            u64 hash{};
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
                            return Boardstate::Board{bp, bn, bb, br, bq, bk, wp, wn, wb, wr, wq, wk, hash};
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

            fen.erase(0, fen.find(' ') + 1);

            if (fen.front() != '-'){
                int file = getNumNotation(fen[0], fen[1]) % 8;
                hash ^= TT::keys[TT::ep0 + file];
            }

            return Boardstate::Board{bp, bn, bb, br, bq, bk, wp, wn, wb, wr, wq, wk, hash};
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

            return Boardstate::State{white, ep, wOO, wOOO, bOO, bOOO};
        }
    }

    static std::string convert_fen(Misc::boardstate& bdst){
        Bit head = 1ull << 63;
        int empty = 0, sq = 0;
        std::stringstream fen;
        do{
            if (!(sq % 8)){
                if (empty){
                    fen << empty;
                    empty = 0;
                }
                if (sq != 64 && sq) fen << "/";
            }

            if ((head & bdst.board.Occ)){
                if (empty){
                    fen << empty;
                    empty = 0;
                }
                if      (head & bdst.board.WPawn)       fen << "P";
                else if (head & bdst.board.BPawn)       fen << "p";
                else if (head & bdst.board.WBishop)     fen << "B";
                else if (head & bdst.board.BBishop)     fen << "b";
                else if (head & bdst.board.WKnight)     fen << "N";
                else if (head & bdst.board.BKnight)     fen << "n";
                else if (head & bdst.board.WRook)       fen << "R";
                else if (head & bdst.board.BRook)       fen << "r";
                else if (head & bdst.board.WQueen)      fen << "Q";
                else if (head & bdst.board.BQueen)      fen << "q";
                else if (head & bdst.board.WKing)       fen << "K";
                else if (head & bdst.board.BKing)       fen << "k";
            }
            else empty++;
            sq++;
        }while (head >>= 1);
        if (empty) fen << empty;

        fen << " " << ((bdst.state.white_move) ? "w" : "b") << " ";

        if (bdst.state.can_castle<true>() || bdst.state.can_castle<false>()) {
            if (bdst.state.white_oo)    fen << "K";
            if (bdst.state.white_ooo)   fen << "Q";
            if (bdst.state.black_oo)    fen << "k";
            if (bdst.state.black_ooo)   fen << "q";
        }
        else fen << "-";
        fen << " ";

        if (bdst.state.has_ep_pawn) {
            if (bdst.state.white_move)  fen << uci_squares[SquareOf(bdst.ep_target << 8)];
            else                        fen << uci_squares[SquareOf(bdst.ep_target >> 8)];
        }
        else fen << "-";

        fen << " 0 1";
        return fen.str();
    }

    char promochar[4] = {'r', 'n', 'b', 'q'};

    static std::string uci_pv(const Boardstate::Board& board, Boardstate::State& state, const Bit ep_target, u8 full_depth){
        u64 hash = board.hash;
        u32 key = hash & Search::TT::key_mask;
        Move mov = Search::TT::table[key].move;
        std::stringstream output;
        auto* bdst = new boardstate{board, state, ep_target};
        int counter = 0;

        while (Search::TT::table[key].hash == hash && counter < (full_depth + 4) && Mfrom(mov) != Mto(mov)){
            counter++;
            output << " " << uci_move(mov);
            bdst = new boardstate{noInfo_move(*bdst, Mfrom(mov), Mto(mov), promochar[((mov >> 6) & 0b111) - 1])};
            hash = bdst->board.hash;
            key = hash & Search::TT::key_mask;
            mov = Search::TT::table[key].move;
        }
        return output.str();
    }

    static std::string uci_eval(Value val){
        std::stringstream output;
        if      (val >= 30000)   output << "mate "  << ceil(double(MATE_POS - val)/2);
        else if (val <= -30000)   output << "mate "  << ceil(double(MATE_NEG - val)/2);
        else                        output << "cp "    << val;
        return output.str();
    }
}

