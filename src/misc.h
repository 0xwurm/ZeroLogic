#pragma once
#include <sstream>
#include <algorithm>

namespace ZeroLogic::Misc{


    template<typename T>
    std::string format(T x)
    {
        std::string num = std::to_string(x);
        if (x < 1000) return num;

        int counter = 0;
        std::string res;
        for (int i = int(num.size()) - 1; i >= 0; i--)
        {
            counter++;
            if (counter == 4)
            {
                counter = 1;
                res += ",";
            }
            res += num[i];
        }
        std::reverse(res.begin(), res.end());
        return res;
    }

    template <CastleType type>
    static std::string _uci_move(){ return uci_castles[type]; }

    template <Piece promotion_to = PIECE_NONE>
    static std::string _uci_move(SquareWrap from, SquareWrap to) {
        if constexpr (promotion_to == PIECE_NONE) return uci_squares[from] + uci_squares[to];
        return uci_squares[from] + uci_squares[to] + uci_promotion[promotion_to - 1];
    }
    template <Piece promotion_to = PIECE_NONE>
    static std::string _uci_move(Square from, Square to) {
        if constexpr (promotion_to == PIECE_NONE) return uci_squares[from] + uci_squares[to];
        return uci_squares[from] + uci_squares[to] + uci_promotion[promotion_to - 1];
    }

    static SquareWrap Mfrom(Move enc){ return SquareWrap(enc & 0b111111); }
    static SquareWrap Mto(Move enc){ return SquareWrap(enc >> 10); }
    static Flags Mflag(Move enc){ return Flags(enc & FLAG); }
    static Piece getPromo(Move enc){
        u16 flag = Mflag(enc);
        if (flag & 0b111) return Piece(flag & 0b111);
        return PIECE_NONE;
    }

    static std::string uci_move(Move enc){

        SquareWrap from = Mfrom(enc);
        SquareWrap to = Mto(enc);
        Flags flag = Mflag(enc);

        if (flag == QP_FLAG) return _uci_move<QUEEN>(from, to);
        if (flag == RP_FLAG) return _uci_move<ROOK>(from, to);
        if (flag == BP_FLAG) return _uci_move<BISHOP>(from, to);
        if (flag == NP_FLAG) return _uci_move<KNIGHT>(from, to);

        if (flag & CS_FLAG) {
            if (from == "e1" && to == "g1") return _uci_move<WHITE_OO>();
            if (from == "e1" && to == "c1") return _uci_move<WHITE_OOO>();
            if (from == "e8" && to == "g8") return _uci_move<BLACK_OO>();
            if (from == "e8" && to == "c8") return _uci_move<BLACK_OOO>();
        }

        return _uci_move<PIECE_NONE>(from, to);
    }

    template<CastleType ct>
    bool isCastles(Piece m, Square f, Square t){
        File destF = ct == SHORT ? FILE_G : FILE_C;
        return ~m == KING
               && (f == RANK_ONE + FILE_E || f == RANK_EIGHT + FILE_E)
               && (t == RANK_ONE + destF || t == RANK_EIGHT + destF);
    }
    template<Color c, CastleType ct>
    bool isLoudRook(Piece m, Square f, const Castling cs) {
        Square check = ct == SHORT ?
                       (c == WHITE ? 0 : 56) :
                       (c == WHITE ? 7 : 63);
        return ~m == ROOK
               && cs[ct]
               && f == check;
    }
    bool isPP(Piece m, Square f, Square t) {
        return ~m == PAWN
               && abs(f - t) == 16;
    }
    bool isEP(Piece m, Piece c, Square f, Square t) {
        return ~m == PAWN
               && c == PIECE_NONE
               && abs(f - t) % 2;
    }

    template<Color c>
    static std::string moves(Position<c> pos, std::istringstream *is);
    template<Color c>
    static std::string uci_pv(Position<c> pos);

#define func(npos) is ? moves(npos, is) : uci_pv(npos)

    template<Color c>
    static std::string standaloneSilent(Position<c> pos, Bit from, Bit to, Piece moved, Piece promo, std::istringstream* is) {
        if (~moved == PAWN) {
            if (promo == PIECE_NONE)    return func((pos.template move_silent<PAWN>(from, to)));
            if (promo == QUEEN)         return func((pos.template move_silent<PAWN, QUEEN>(from, to)));
            if (promo == ROOK)          return func((pos.template move_silent<PAWN, ROOK>(from, to)));
            if (promo == KNIGHT)        return func((pos.template move_silent<PAWN, KNIGHT>(from, to)));
            if (promo == BISHOP)        return func((pos.template move_silent<PAWN, BISHOP>(from, to)));
        }
        if (~moved == KNIGHT)   return func((pos.template move_silent<KNIGHT>(from, to)));
        if (~moved == BISHOP)   return func((pos.template move_silent<BISHOP>(from, to)));
        if (~moved == ROOK)     return func((pos.template move_silent<ROOK>(from, to)));
        if (~moved == QUEEN)    return func((pos.template move_silent<QUEEN>(from, to)));
        if (~moved == KING)     return func((pos.template move_silent<KING>(from, to)));
        return "";
    }

    template<Color c>
    static std::string standaloneMoves(Position<c> pos, Square from, Square to, Piece promo, std::istringstream* is = nullptr) {
        Bit fromBit = 1ull << from;
        Bit toBit = 1ull << to;

        Piece moved = pos.getPiece(from);
        Piece captured = pos.getPiece(to);

        if (isCastles<SHORT>(moved, from, to))
            return func(pos.template move_castles<SHORT>());
        if (isCastles<LONG>(moved, from, to))
            return func(pos.template move_castles<LONG>());

        if (isPP(moved, from, to))
            return func(pos.move_pp(fromBit, toBit));
        if (isEP(moved, captured, from, to))
            return func(pos.move_ep(fromBit, toBit));

        if (isLoudRook<c, SHORT>(moved, from, pos.cs))
            return func(pos.template move_loudRook<SHORT>(fromBit, toBit));
        if (isLoudRook<c, LONG>(moved, from, pos.cs))
            return func(pos.template move_loudRook<LONG>(fromBit, toBit));

        return standaloneSilent(pos, fromBit, toBit, moved, promo, is);
    }

#undef func

    template <Color c>
    static std::string moves(Position<c> pos, std::istringstream* is){
        std::string token;
        if (*is >> token){
            Square from = getNumNotation(token[0], token[1]);
            Square to = getNumNotation(token[2], token[3]);
            Piece promo = PIECE_NONE;
            if (token[4] == 'q') promo = QUEEN;
            if (token[4] == 'b') promo = BISHOP;
            if (token[4] == 'n') promo = KNIGHT;
            if (token[4] == 'r') promo = ROOK;

            return Misc::standaloneMoves(pos, from, to, promo, is);
        }
        return std::string(pos);
    }
    PositionToTemplate(moves, std::string, std::istringstream*)

    Depth allowed = 0;
    Depth recursion = 0;

    template<Color c>
    static std::string uci_pv(Position<c> pos){
        u32 key = *pos.hash;
        Move mov = Search::TT::table[key].move;
        if (recursion++ > allowed) return "";

        if (Search::TT::table[key].hash == pos.hash && Mfrom(mov) != Mto(mov))
            return " " + uci_move(mov) + standaloneMoves(pos, Mfrom(mov), Mto(mov), getPromo(mov));
        return "";
    }

    template<Color c>
    static std::string uci_pv(Position<c>& pos, Depth d){
        allowed = Depth(2 * d);
        recursion = 0;
        return uci_pv(pos);
    }

    static std::string uci_eval(Value val, Depth d){
        std::stringstream output;
        if      (val >= 30000)   output << "mate "  << ceil(double(d - (MATE_POS - val))/2);
        else if (val <= -30000)   output << "mate "  << ceil(double(-d - (MATE_NEG - val))/2);
        else                        output << "cp "    << val;
        return output.str();
    }
}

