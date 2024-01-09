#pragma once
#include "stuff.h"

namespace ZeroLogic {
    namespace Movegen{
        struct Mask{
            map check{full};
            map rook{};
            map bishop{};
            map kingban{};
            map kingmoves{};
        };
    }

    // Castling right container
    // no(...)For methods change only specified rights and leave all others as is
    struct Castling {
        bool oOO;
        bool oOOO;
        bool eOO;
        bool eOOO;

        constexpr inline bool operator[](int ind) const{
            if (ind == SHORT)   return oOO;
            if (ind == LONG)    return oOOO;
            return false;
        }
        template<Color c>
        std::string print() const{
            std::string res;
            if (oOO || oOOO || eOO || eOOO) {
                if (oOO)    res += c == WHITE ? 'K' : 'k';
                if (oOOO)   res += c == WHITE ? 'Q': 'q';
                if (eOO)    res += c == WHITE ? 'k': 'K';
                if (eOOO)   res += c == WHITE ? 'q' : 'Q';
            }
            else res += "-";
            return res;
        }

        inline bool canCastle() const{ return oOO || oOOO; }
        inline Castling silent() const{ return {eOO, eOOO, oOO, oOOO}; }
        inline Castling noCastles() const{ return {eOO, eOOO, false, false};}
        inline Castling noOO() const{ return {eOO, eOOO, false, oOOO};}
        inline Castling noOOO() const{ return {eOO, eOOO, oOO, false};}
        COMPILETIME Castling none() {return {false, false, false, false};}
        COMPILETIME Castling all() {return {true, true, true, true};}
    };
}

#include "tables.h"

namespace ZeroLogic{

    // Class contains complete state and board
    // One instance only holds one Boardstate, when the position changes, a new Boardstate is created
    template <Color c>
    class Position {
    public:
        const Hash hash;
        const Bit target;
        const Castling cs;
        const map oPawns, oKnights, oBishops, oRooks, oQueens, oKing;
        const map ePawns, eKnights, eBishops, eRooks, eQueens, eKing;
        const map us, enemy, Occ;

        // necessary due to very nasty bug with changing the hash after ep was pruned away because of a pin
        bool ep;

        constexpr Position(
                Hash zh, Bit tg, Castling cs,
                map oP, map oN, map oB, map oR, map oQ, map oK,
                map eP, map eN, map eB, map eR, map eQ, map eK) :
                oPawns(oP), oKnights(oN), oBishops(oB), oRooks(oR), oQueens(oQ), oKing(oK),
                ePawns(eP), eKnights(eN), eBishops(eB), eRooks(eR), eQueens(eQ), eKing(eK),
                us(oP | oN | oB | oR | oQ | oK),
                enemy(eP | eN | eB | eR | eQ | eK),
                Occ(us | enemy),
                hash(zh), target(tg), cs(cs), ep(target)
        {}

        explicit constexpr Position(Position<NONE>& pos) :
                oPawns(pos.oPawns), oKnights(pos.oKnights), oBishops(pos.oBishops), oRooks(pos.oRooks), oQueens(pos.oQueens), oKing(pos.oKing),
                ePawns(pos.ePawns), eKnights(pos.eKnights), eBishops(pos.eBishops), eRooks(pos.eRooks), eQueens(pos.eQueens), eKing(pos.eKing),
                us(oPawns | oKnights | oBishops | oRooks | oQueens | oKing),
                enemy(ePawns | eKnights | eBishops | eRooks | eQueens | eKing),
                Occ(us | enemy),
                hash(pos.hash), target(pos.target), cs(pos.cs), ep(pos.target)
        {}

        // get enemy sliders of specified type
        // used to generate check masks or check for attacks
        inline map eDiagonalSliders() const{ return eBishops | eQueens; }
        inline map eStraightSliders() const{ return eRooks | eQueens; }

        template<CastleType ct>
        static inline map rm(){
            if constexpr (ct == WHITE_OO)   return 1;
            if constexpr (ct == BLACK_OO)   return 1ull << 56;
            if constexpr (ct == WHITE_OOO)  return 0x80;
            if constexpr (ct == BLACK_OOO)  return 0x80ull << 56;
            return 0;
        }
        template<CastleType ct>
        inline bool ir(){
            return oRooks & rm<ct>();
        }

    private:
        template<CastleType ct>
        COMPILETIME map rs(){
            if constexpr (ct == WHITE_OO)   return 0x5;
            if constexpr (ct == BLACK_OO)   return 5ull << 56;
            if constexpr (ct == WHITE_OOO)  return 0x90;
            if constexpr (ct == BLACK_OOO)  return 0x90ull << 56;
            return 0;
        }
        template<CastleType ct>
        COMPILETIME map ks(){
            if constexpr (ct == WHITE_OO)   return 0xa;
            if constexpr (ct == BLACK_OO)   return 0xaull << 56;
            if constexpr (ct == WHITE_OOO)  return 0x28;
            if constexpr (ct == BLACK_OOO)  return 0x28ull << 56;
            return 0;
        }
        template<CastleType ct>
        COMPILETIME map kp(){
            if constexpr (ct == WHITE_OO)   return 0x6;
            if constexpr (ct == BLACK_OO)   return 0x6ull << 56;
            if constexpr (ct == WHITE_OOO)  return 0x30;
            if constexpr (ct == BLACK_OOO)  return 0x30ull << 56;
            return 0;
        }
        template<CastleType ct>
        COMPILETIME map rp(){
            if constexpr (ct == WHITE_OO)   return 0x6;
            if constexpr (ct == BLACK_OO)   return 0x6ull << 56;
            if constexpr (ct == WHITE_OOO)  return 0x70;
            if constexpr (ct == BLACK_OOO)  return 0x70ull << 56;
            return 0;
        }

        // silent moves + king
        template<Piece p, Piece pr = PIECE_NONE, Piece t = PIECE_NONE>
        inline Position<!c> _move_silent(Bit from, Bit to){
            const map oP = oPawns, oN = oKnights, oB = oBishops, oR = oRooks, oQ = oQueens, oK = oKing;
            const map eP = ePawns, eN = eKnights, eB = eBishops, eR = eRooks, eQ = eQueens, eK = eKing;
            const map mov = from | to;

#define enemy eP ^ ((t == PAWN) ? to : 0), eN ^ ((t == KNIGHT) ? to : 0), eB ^ ((t == BISHOP) ? to : 0), \
                eR ^ ((t == ROOK) ? to : 0), eQ ^ ((t == QUEEN) ? to : 0), eK
#define front hash.mod<p, t, pr>(from, to, *this), 0, cs.silent(), enemy


            if constexpr (p == PAWN) {
                if constexpr (pr == PIECE_NONE)  return {front, oP ^ mov, oN, oB, oR, oQ, oK};
                if constexpr (pr == KNIGHT)      return {front, oP ^ from, oN ^ to, oB, oR, oQ, oK};
                if constexpr (pr == BISHOP)      return {front, oP ^ from, oN, oB ^ to, oR, oQ, oK};
                if constexpr (pr == ROOK)        return {front, oP ^ from, oN, oB, oR ^ to, oQ, oK};
                if constexpr (pr == QUEEN)       return {front, oP ^ from, oN, oB, oR, oQ ^ to, oK};
            }
            if constexpr (p == KNIGHT)  return { front, oP, oN ^ mov, oB, oR, oQ, oK};
            if constexpr (p == BISHOP)  return { front, oP, oN, oB ^ mov, oR, oQ, oK};
            if constexpr (p == ROOK)    return { front, oP, oN, oB, oR ^ mov, oQ, oK};
            if constexpr (p == QUEEN)   return { front, oP, oN, oB, oR, oQ ^ mov, oK};
            if constexpr (p == KING)    return { hash.modKing<t>(from, to, *this), 0, cs.noCastles(), enemy, oP, oN, oB, oR, oQ, oK ^ mov};

#undef enemy
#undef front
        }

        template<CastleType ct, Piece t = PIECE_NONE>
        inline Position<!c> _move_loudRook(Bit from, Bit to){
            return {
                    hash.modRook<t, ct>(from, to, *this), 0,
                    (ct == SHORT) ? cs.noOO() : cs.noOOO(),
                    ePawns ^ ((t == PAWN) ? to : 0), eKnights ^ ((t == KNIGHT) ? to : 0),
                    eBishops ^ ((t == BISHOP) ? to : 0), eRooks ^ ((t == ROOK) ? to : 0),
                    eQueens ^ ((t == QUEEN) ? to : 0), eKing,
                    oPawns, oKnights, oBishops, oRooks ^ (from | to), oQueens, oKing
            };
        }
    public:

        template<Piece p, Piece promo = PIECE_NONE>
        inline Position<!c> move_silent(Bit from, Bit to){
            if (to & Occ){
                if (to & ePawns)    return _move_silent<p, promo, PAWN>(from, to);
                if (to & eKnights)  return _move_silent<p, promo, KNIGHT>(from, to);
                if (to & eBishops)  return _move_silent<p, promo, BISHOP>(from, to);
                if (to & eRooks)    return _move_silent<p, promo, ROOK>(from, to);
                if (to & eQueens)   return _move_silent<p, promo, QUEEN>(from, to);
            }
            return _move_silent<p, promo>(from, to);
        }
        template<CastleType ct>
        inline Position<!c> move_loudRook(Bit from, Bit to){
            if (to & Occ){
                if (to & ePawns)    return _move_loudRook<ct, PAWN>(from, to);
                if (to & eKnights)  return _move_loudRook<ct, KNIGHT>(from, to);
                if (to & eBishops)  return _move_loudRook<ct, BISHOP>(from, to);
                if (to & eRooks)    return _move_loudRook<ct, ROOK>(from, to);
                if (to & eQueens)   return _move_loudRook<ct, QUEEN>(from, to);
            }
            return _move_loudRook<ct>(from, to);
        }

        inline Position<!c> move_pp(Bit from, Bit to){
            return {
                hash.modPP<c>(from, to, *this), to, cs.silent(),
                ePawns, eKnights, eBishops, eRooks, eQueens, eKing,
                oPawns ^ (from | to), oKnights, oBishops, oRooks, oQueens, oKing
            };
        }

        inline Position<!c> move_ep(Bit from, Bit to){
            return {
                hash.modEP<c>(from, to, *this), 0, cs.silent(),
                ePawns ^ target, eKnights, eBishops, eRooks, eQueens, eKing,
                oPawns ^ (from | to), oKnights, oBishops, oRooks, oQueens, oKing
            };
        }

        template<CastleType ct>
        inline Position<!c> move_castles(){
            constexpr CastleType cct = c >> ct;
            return {
                hash.modCastles<cct>(*this), 0, cs.noCastles(),
                ePawns, eKnights, eBishops, eRooks, eQueens, eKing,
                oPawns, oKnights, oBishops, oRooks ^ rs<cct>(), oQueens, oKing ^ ks<cct>()
            };
        }

        template<CastleType ct>
        inline bool canCastle(Movegen::Mask& masks){
            constexpr CastleType cct = c >> ct;
            return cs[ct] && !((kp<cct>() & masks.kingban) | (rp<cct>() & Occ)) && ir<cct>();
        }

        // does what you think it does
        Piece getPiece(Square sq) const{
            Bit is = 1ull << sq;
            if (is & Occ){
                if (is & oPawns)    return  c >> PAWN;
                if (is & ePawns)    return !c >> PAWN;
                if (is & oBishops)  return  c >> BISHOP;
                if (is & eBishops)  return !c >> BISHOP;
                if (is & oKnights)  return  c >> KNIGHT;
                if (is & eKnights)  return !c >> KNIGHT;
                if (is & oRooks)    return  c >> ROOK;
                if (is & eRooks)    return !c >> ROOK;
                if (is & oQueens)   return  c >> QUEEN;
                if (is & eQueens)   return !c >> QUEEN;
                if (is & oKing)     return  c >> KING;
                if (is & eKing)     return !c >> KING;
            }
            return PIECE_NONE;
        }
        Piece getPiece(Rank r, File f)const{return getPiece(f+8*r);}

        // convert Boardstate to fen string
        explicit operator std::string(){
            std::string res;
            int empty_squares = 0;
            for (Rank r = RANK_EIGHT; r >= RANK_ONE; --r){
                for (File f = FILE_A; f >= FILE_H; --f){
                    char pChar = *getPiece(r, f);
                    if (pChar != ' ') {
                        if (empty_squares) {
                            res += std::to_string(empty_squares);
                            empty_squares = 0;
                        }
                        res += pChar;
                    }
                    else empty_squares++;
                }
                res += ((empty_squares) ? std::to_string(empty_squares) : "")
                        + ((r) ? "/" : "");
                empty_squares = 0;
            }

            return res += ((c == WHITE) ? " w " : " b ")
                        + cs.print<c>() + " "
                        + ((target) ? Misc::uci_squares[SquareOf((moveP<c, SOUTH>(target)))] : "-")
                        + " 0 1";
        }

    };

    // print epic board, stolen from stockfish
    template <Color c>
    std::ostream& operator <<(std::ostream& stream, Position<c>& pos){
        stream << "\n +---+---+---+---+---+---+---+---+\n";

        for (Rank r = RANK_EIGHT; r >= RANK_ONE; --r)
        {
            for (File f = FILE_A; f >= FILE_H; --f)
                stream << " | " << *pos.getPiece(r, f);

            stream << " | " << (r + 1) << "\n +---+---+---+---+---+---+---+---+\n";
        }
        stream << "   a   b   c   d   e   f   g   h\n";

        stream << "Fen: " << std::string(pos) << "\n";
        char hex_hash[100];
        std::sprintf(hex_hash, "%llX", pos.hash.getVal());
        stream << "Key: " << hex_hash << "\n";

        return stream << std::endl;
    }

    // convert fen string to position for further processing
    Position<NONE> operator *(std::string fen){
        u64 hash{};
        Bit tg{};
        Castling cs{};
        map brd[14]{};
        
        File f = FILE_A;
        Rank r = RANK_EIGHT;

        auto putPiece = [&]<Piece p>(){
            brd[p] |= 1ull << (f + 8 * r);
            hash ^= Hash::getKey<p>(r, f);
            --f;
        };

        for (char c : fen){
            if (c == ' ') break;
            else if (c == '/') {f = FILE_A; --r;}
            else if (c == 'P') putPiece.operator()<WHITE_PAWN>();
            else if (c == 'p') putPiece.operator()<BLACK_PAWN>();
            else if (c == 'B') putPiece.operator()<WHITE_BISHOP>();
            else if (c == 'b') putPiece.operator()<BLACK_BISHOP>();
            else if (c == 'N') putPiece.operator()<WHITE_KNIGHT>();
            else if (c == 'n') putPiece.operator()<BLACK_KNIGHT>();
            else if (c == 'R') putPiece.operator()<WHITE_ROOK>();
            else if (c == 'r') putPiece.operator()<BLACK_ROOK>();
            else if (c == 'Q') putPiece.operator()<WHITE_QUEEN>();
            else if (c == 'q') putPiece.operator()<BLACK_QUEEN>();
            else if (c == 'K') putPiece.operator()<WHITE_KING>();
            else if (c == 'k') putPiece.operator()<BLACK_KING>();
            else f -= c - 48;
        }
        fen.erase(0, fen.find(' ') + 1);
        Color c = fen.front() == 'w' ? WHITE : BLACK;
        fen.erase(0, fen.find(' ') + 1);
        cs.oOO = fen.contains('K');
        cs.oOOO = fen.contains('Q');
        cs.eOO = fen.contains('k');
        cs.eOOO = fen.contains('q');
        fen.erase(0, fen.find(' ') + 1);
        if (fen.front() != '-') {
            tg = 1ull << (getNumNotation(fen[0], fen[1]) + (c == WHITE ? -8 : 8));
            hash ^= Hash::getKeyEp(tg);
        }

        if (c) hash ^= Hash::keys[780];
        if (cs.oOO)     hash ^= Hash::keys[768 + WHITE_OO];
        if (cs.oOOO)    hash ^= Hash::keys[768 + WHITE_OOO];
        if (cs.eOO)     hash ^= Hash::keys[768 + BLACK_OO];
        if (cs.eOOO)    hash ^= Hash::keys[768 + BLACK_OOO];

        if (c == BLACK) cs = {cs.eOO, cs.eOOO, cs.oOO, cs.oOOO};

        return {Hash{hash}, tg, cs,
                brd[c>>PAWN], brd[c>>KNIGHT], brd[c>>BISHOP], brd[c>>ROOK], brd[c>>QUEEN], brd[c>>KING],
                brd[!c>>PAWN], brd[!c>>KNIGHT], brd[!c>>BISHOP], brd[!c>>ROOK], brd[!c>>QUEEN], brd[!c>>KING]};
    }

}