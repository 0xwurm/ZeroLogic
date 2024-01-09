#pragma once
#include <random>

namespace ZeroLogic{

    template<Color c>
    class Position;

    class Hash{
    private:
        u64 value;
        static inline u64 keys[781];

        template<Color active, Piece p>
        static inline u64 getKey(Square sq){
            constexpr Piece cp = active >> p;
            if constexpr (cp == WHITE_PAWN)      return keys[  0 + sq];
            if constexpr (cp == WHITE_BISHOP)    return keys[ 64 + sq];
            if constexpr (cp == WHITE_KNIGHT)    return keys[128 + sq];
            if constexpr (cp == WHITE_ROOK)      return keys[192 + sq];
            if constexpr (cp == WHITE_QUEEN)     return keys[256 + sq];
            if constexpr (cp == WHITE_KING)      return keys[320 + sq];
            if constexpr (cp == BLACK_PAWN)      return keys[384 + sq];
            if constexpr (cp == BLACK_BISHOP)    return keys[448 + sq];
            if constexpr (cp == BLACK_KNIGHT)    return keys[512 + sq];
            if constexpr (cp == BLACK_ROOK)      return keys[576 + sq];
            if constexpr (cp == BLACK_QUEEN)     return keys[640 + sq];
            if constexpr (cp == BLACK_KING)      return keys[704 + sq];
            return 0;
        }
        template<Piece p>
        static inline u64 getKey(Rank r, File f){
            return getKey<WHITE, p>(f + 8*r);
        }
        template<bool both = true, CastleType ct = SHORT, Color c>
        static inline u64 getKeyCastleRights(Position<c>& pos){
            if constexpr (ct == CASTLE_INVALID) return 0;
            if constexpr (both)
                return (pos.cs.oOO ? keys[768 + (c >> SHORT)] : 0)
                        ^ (pos.cs.oOOO ? keys[768 + (c >> LONG)] : 0);
            return pos.cs[ct] ? keys[768 + (c >> ct)] : 0;
        }
        template<CastleType ct>
        static inline u64 getKeyCastles(){
            if constexpr (ct == WHITE_OO)   return getKey<WHITE, ROOK>(0) ^ getKey<WHITE, KING>(1) ^ getKey<WHITE, ROOK>(2) ^ getKey<WHITE, KING>(3);
            if constexpr (ct == WHITE_OOO)  return getKey<WHITE, ROOK>(7) ^ getKey<WHITE, KING>(5) ^ getKey<WHITE, ROOK>(4) ^ getKey<WHITE, KING>(3);
            if constexpr (ct == BLACK_OO)   return getKey<BLACK, ROOK>(56) ^ getKey<BLACK, KING>(57) ^ getKey<WHITE, ROOK>(58) ^ getKey<BLACK, KING>(59);
            if constexpr (ct == BLACK_OOO)  return getKey<BLACK, ROOK>(63) ^ getKey<BLACK, KING>(61) ^ getKey<WHITE, ROOK>(60) ^ getKey<BLACK, KING>(59);
        }
        static inline u64 getKeyEp(const Bit& target){
            if (target) return keys[772 + SquareOf(target) % 8];
            return 0;
        }

    public:
        explicit constexpr Hash(u64 zh) : value(zh){}
        u64 getVal() const{return value;}
        static void init_keys() {
            std::mt19937_64 rand{3141592653589793238ull};
            for (u64 &key: keys)
                key = rand();
        }
        friend Position<NONE> operator*(std::string fen);

        inline u32 operator *() const{return value & key_mask;}
        inline bool operator ==(Hash h2)const{return value == h2.value;}

        template<Piece p, Piece t, Piece pr, CastleType ct = CASTLE_INVALID, bool both = false, Color c>
        inline Hash mod(Bit& from, Bit& to, Position<c>& pos) const{
            return Hash{value
                        ^ getKey<c, p>(SquareOf(from))                               // remove piece from origin Square
                        ^ getKey<c, pr == PIECE_NONE ? p : pr>(SquareOf(to))        // put (promoted) piece on destination Square
                        ^ getKey<!c, t>(SquareOf(to))                              // if capture, remove captured piece
                        ^ getKeyEp(pos.target)                              // if there is an ep target, remove it
                        ^ getKeyCastleRights<both, ct>(pos)                   // if a castle right changes, flip it
                        ^ keys[780]};                                           // change color to move
        }

        template<Piece t, Color c>
        inline Hash modKing(Bit& from, Bit& to, Position<c>& pos) const{
            return mod<KING, t, PIECE_NONE, SHORT, true>(from, to, pos);
        }
        template<Piece t, CastleType ct, Color c>
        inline Hash modRook(Bit& from, Bit& to, Position<c>& pos) const{
            return mod<ROOK, t, PIECE_NONE, ct>(from, to, pos);
        }

        template<CastleType ct, Color c>
        inline Hash modCastles(Position<c>& pos) const{
            return Hash{value
                        ^ getKeyCastles<ct>()
                        ^ getKeyCastleRights(pos)
                        ^ getKeyEp(pos.target)
                        ^ keys[780]};
        }

        template<Color c>
        inline Hash modEP(Bit& from, Bit& to, Position<c>& pos) const{
            return Hash{value
                        ^ getKey<c, PAWN>(SquareOf(from))
                        ^ getKey<c, PAWN>(SquareOf(to))
                        ^ getKey<!c, PAWN>(SquareOf(pos.target))
                        ^ getKeyEp(pos.target)
                        ^ keys[780]};
        }

        template<Color c>
        inline Hash modPP(Bit& from, Bit& to, Position<c>& pos) const{
            return Hash{value
                        ^ getKey<c, PAWN>(SquareOf(from))
                        ^ getKey<c, PAWN>(SquareOf(to))
                        ^ getKeyEp(pos.target)
                        ^ getKeyEp(to)
                        ^ keys[780]};
        }
    };

}
