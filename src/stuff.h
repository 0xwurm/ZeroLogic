#pragma once
#include <iostream>

#define COMPILETIME static constexpr inline
#define getNumNotation(char1, char2) (7 - (char1 - 97) + 8 * (char2 - 49))

typedef unsigned char u8;
typedef signed char s8;
typedef signed short s16;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long long u64;

#ifdef USE_INTRIN
#include <immintrin.h>
#include <intrin.h>
#define PEXT(X, Y) _pext_u64(X, Y)
#define SquareOf(X) _tzcnt_u64(X)
#define BitOf(X) _blsi_u64(X)
#define Bitloop(X) for(;X; X = _blsr_u64(X))
#define BitCount(X) __popcnt64(X)
#else
    inline static u64 pext(u64 x, u64 mask) {
u64 res = 0;
for(u64 bb = 1; mask != 0; bb += bb) {
if(x & mask & -mask) {
res |= bb;
}
mask &= (mask - 1);
}
return res;
}
#define PEXT(X, Y) pext(X, Y)
#define SquareOf(X) std::countr_zero(X)
#define BitOf(X) 1ull << SquareOf(X)
#define Bitloop(X) for(;X; X &= X - 1)
#define BitCount(X) std::popcount(X)
#endif

inline static u64 PopBit(u64& val)
{
    u64 lsb = BitOf(val);
    val ^= lsb;
    return lsb;
}

namespace ZeroLogic{

    using map = u64;
    using Bit = u64;
    using Square = u8;
    using Move = u16;

    enum SquareWrap : Square{};

    static const char* start_fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

    enum Color{
        BLACK = false,
        WHITE = true,
        NONE
    };

    enum SearchType : bool{
        QSearch = false,
        NSearch = true
    };

    enum Piece{
        PAWN, ROOK, KNIGHT, BISHOP, QUEEN, KING,

        WHITE_PAWN = 0, WHITE_ROOK, WHITE_KNIGHT, WHITE_BISHOP, WHITE_QUEEN, WHITE_KING,
        BLACK_PAWN = 8, BLACK_ROOK, BLACK_KNIGHT, BLACK_BISHOP, BLACK_QUEEN, BLACK_KING,
        PIECE_NONE = 6,
        PIECE_INVALID = 14
    };

    enum CastleType{
        WHITE_OO, WHITE_OOO, BLACK_OO, BLACK_OOO,
        SHORT, LONG,
        CASTLE_NONE = 8,
        CASTLE_INVALID = 9
    };

    enum Value : int{
        DRAW = 0,
        ZERO = 0,
        MATE_POS = 32000,
        MATE_NEG = -32000,
        ABSOLUTE_MIN = -0x7fff,
        ABSOLUTE_MAX = 0x7fff,
        KNOWN_MATE = 31000,
        INVALID_VALUE = -32001,

        Wkgn = 30000,

        PAWN_MG = 82,       PAWN_EG = 94,
        KNIGHT_MG = 337,    KNIGHT_EG = 281,
        BISHOP_MG = 365,    BISHOP_EG = 297,
        ROOK_MG = 477,      ROOK_EG = 512,
        QUEEN_MG = 1025,    QUEEN_EG = 936,

        NON_CAPTURE = -50,
        HASHMOVE = 10000

    };

    enum File{
        FILE_H, FILE_G, FILE_F, FILE_E, FILE_D, FILE_C, FILE_B, FILE_A
    };

    enum Rank{
        RANK_ONE, RANK_TWO, RANK_THREE, RANK_FOUR, RANK_FIVE, RANK_SIX, RANK_SEVEN, RANK_EIGHT
    };

	enum BoardConstants : const map {
		FILE_A_MASK = 0x8080808080808080,    RANK_ONE_MASK = 0xff,
        FILE_B_MASK = 0x4040404040404040,    RANK_TWO_MASK = 0xff00,
        FILE_C_MASK = 0x2020202020202020,    RANK_THREE_MASK = 0xff0000,
		FILE_D_MASK = 0x1010101010101010,    RANK_FOUR_MASK = 0xff000000,
		FILE_E_MASK = 0x808080808080808,     RANK_FIVE_MASK = 0xff00000000,
		FILE_F_MASK = 0x404040404040404,     RANK_SIX_MASK = 0xff0000000000,
		FILE_G_MASK = 0x202020202020202,     RANK_SEVEN_MASK = 0xff000000000000,
		FILE_H_MASK = 0x101010101010101,     RANK_EIGHT_MASK = 0xff00000000000000,

        NOT_BORDER_WEST = ~FILE_A_MASK,
        NOT_BORDER_EAST = ~FILE_H_MASK
	};

    enum Constants : const map {
        empty = 0,
        full = 0xffffffffffffffff
    };

    enum Direction{
        NORTH = 8, EAST = -1, SOUTH = -8, WEST = 1,
        NORTHWEST = NORTH + WEST, SOUTHWEST = SOUTH + WEST,
        NORTHEAST = NORTH + EAST, SOUTHEAST = SOUTH + EAST,
        LEFT = NORTHWEST, RIGHT = NORTHEAST, FORWARD = NORTH
    };


#define ENABLE_BASE_OPERATORS_ON(T)                                \
constexpr T operator+(T d1, int d2) { return T(int(d1) + d2); }    \
constexpr T operator-(T d1, int d2) { return T(int(d1) - d2); }    \
constexpr T operator-(T d) { return T(-int(d)); }                  \
inline T& operator+=(T& d1, int d2) { return d1 = d1 + d2; }       \
inline T& operator-=(T& d1, int d2) { return d1 = d1 - d2; }

#define ENABLE_INCR_OPERATORS_ON(T)                                \
inline T& operator++(T& d) { return d = T(int(d) + 1); }           \
inline T& operator--(T& d) { return d = T(int(d) - 1); }

#define ENABLE_FULL_OPERATORS_ON(T)                                \
ENABLE_BASE_OPERATORS_ON(T)                                        \
constexpr T operator*(int i, T d) { return T(i * int(d)); }        \
constexpr T operator*(T d, int i) { return T(int(d) * i); }        \
constexpr T operator/(T d, int i) { return T(int(d) / i); }        \
constexpr int operator/(T d1, T d2) { return int(d1) / int(d2); }  \
inline T& operator*=(T& d, int i) { return d = T(int(d) * i); }    \
inline T& operator/=(T& d, int i) { return d = T(int(d) / i); }

ENABLE_FULL_OPERATORS_ON(Value)
ENABLE_FULL_OPERATORS_ON(Rank)
ENABLE_INCR_OPERATORS_ON(Rank)
ENABLE_FULL_OPERATORS_ON(File)
ENABLE_INCR_OPERATORS_ON(File)
ENABLE_BASE_OPERATORS_ON(Direction)
ENABLE_INCR_OPERATORS_ON(Piece)
constexpr Color operator!(Color c) {return Color(!bool(c)); }

constexpr Piece operator!(Piece p) {return Piece(p^8); } // flip piece color
constexpr Piece operator>>(Color c, Piece p) {
    if (c == WHITE) return p;
    else            return !p;
}
constexpr Piece operator~(Piece p) {return Piece(p&0b111);} // normalize piece
constexpr char operator *(Piece p) {return "PRNBQK  prnbqk "[p];}

constexpr CastleType operator !(CastleType ct) {return CastleType(ct ^ 1);} // get other right
constexpr CastleType operator >>(Color c, CastleType t){
    if (c == WHITE) return CastleType(t ^ 4);
    return CastleType((t ^ 4) + 2);
}
constexpr Square operator+(Rank r, File f) { return Square(f + 8*r); }

constexpr bool operator!(Direction d){return int(d) < 0;}
constexpr Direction operator >>(Color c, Direction d)
{
    if (c == WHITE) return d;
    return Direction(-int(d));
}

constexpr Move operator>>(Piece p, Move m){m |= (p << 6); return m;}

constexpr bool operator==(SquareWrap sq, const char* str)
{
    return sq == getNumNotation(str[0], str[1]);
}

#undef ENABLE_FULL_OPERATORS_ON
#undef ENABLE_INCR_OPERATORS_ON
#undef ENABLE_BASE_OPERATORS_ON


    namespace Misc{
        static std::string uci_squares[64]{
                "h1", "g1", "f1", "e1", "d1", "c1", "b1", "a1",
                "h2", "g2", "f2", "e2", "d2", "c2", "b2", "a2",
                "h3", "g3", "f3", "e3", "d3", "c3", "b3", "a3",
                "h4", "g4", "f4", "e4", "d4", "c4", "b4", "a4",
                "h5", "g5", "f5", "e5", "d5", "c5", "b5", "a5",
                "h6", "g6", "f6", "e6", "d6", "c6", "b6", "a6",
                "h7", "g7", "f7", "e7", "d7", "c7", "b7", "a7",
                "h8", "g8", "f8", "e8", "d8", "c8", "b8", "a8"
        };
        static std::string uci_castles[4]{
                "e1g1", "e1c1", "e8g8", "e8c8"
        };
        static std::string uci_promotion[4]{
                "r", "n", "b", "q"
        };
    }

    template<Direction d, int i = 1>
    inline map move(const map& X)
    {
        if constexpr (!d)   return X >> (-d * i);
        else                return X << (d * i);
    }

    template<Direction d>
    COMPILETIME map borderP()
    {
        if constexpr (d == NORTHWEST || d == SOUTHWEST || d == WEST) return NOT_BORDER_WEST;
        if constexpr (d == NORTHEAST || d == SOUTHEAST || d == EAST) return NOT_BORDER_EAST;
        return 0;
    }

    template<Color c, Direction d, bool borders = false>
    inline map moveP(const map& X)
    {
        constexpr Direction cd = c >> d;
        if constexpr (!borders) return move<cd>(X);
        return move<cd>(X & borderP<cd>());
    }

#define PositionToTemplate(func, T1, T2) \
    static T1 _##func(std::string fen, T2 param) { \
        Position<NONE> pos = *fen;\
        if (fen.contains('w')) return func(Position<WHITE>(pos), param);     \
        return func(Position<BLACK>(pos), param);    \
    }
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
}