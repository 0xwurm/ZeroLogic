#pragma once
#include <iostream>

#define FORCEINLINE inline __attribute__((__always_inline__))
#define COMPILETIME static constexpr FORCEINLINE
#define getNumNotation(char1, char2) (7 - (char1 - 97) + 8 * (char2 - 49))

#ifdef USE_INTRIN
#include <immintrin.h>
#include <intrin.h>
#define PEXT(X, Y) _pext_u64(X, Y)
#define SquareOf(X) _tzcnt_u64(X)
#define Bitloop(X) for(;X; X = _blsr_u64(X))
#define BitCount(X) __popcnt64(X)
#else
FORCEINLINE static unsigned long long pext(unsigned long long x, unsigned long long mask) {
unsigned long long res = 0;
for(unsigned long long bb = 1; mask != 0; bb += bb) {
if(x & mask & -mask) {
res |= bb;
}
mask &= (mask - 1);
}
return res;
}
#define PEXT(X, Y) pext(X, Y)
#define SquareOf(X) std::countr_zero(X)
#define Bitloop(X) for(;X; X &= X - 1)
#define BitCount(X) std::popcount(X)
#endif

namespace ZeroLogic {
    typedef unsigned char u8;
    typedef signed char s8;
    typedef signed short s16;
    typedef unsigned short u16;
    typedef unsigned int u32;
	typedef unsigned long long u64;

	using map = u64;
    using Bit = u64;
    using Square = u8;
    using Move = u16;

    static const char* start_fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

    enum Color : bool{
        BLACK = false,
        WHITE = true
    };

    enum SearchType : bool{
        QSearch = false,
        NSearch = true
    };

    enum Piece{
        PAWN, ROOK, KNIGHT, BISHOP, QUEEN, KING,
        PIECE_INVALID = 14
    };

    enum CastleType{
        WHITE_OO, WHITE_OOO, BLACK_OO, BLACK_OOO,
        CASTLE_INVALID = 5
    };

    enum Value : int{
        DRAW = 0,
        ZERO = 0,
        MATE_POS = 32000,
        MATE_NEG = -32000,
        ABSOLUTE_MIN = -0x7fff,
        ABSOLUTE_MAX = 0x7fff,
        KNOWN_MATE = 31000,

        Wkgn = 30000,

        PAWN_MG = 82,       PAWN_EG = 94,
        KNIGHT_MG = 337,    KNIGHT_EG = 281,
        BISHOP_MG = 365,    BISHOP_EG = 297,
        ROOK_MG = 477,      ROOK_EG = 512,
        QUEEN_MG = 1025,    QUEEN_EG = 936,

        NON_CAPTURE = -50,
        HASHMOVE = 10000

    };

	enum BoardConstants : const map {
		A_FILE = 0x8080808080808080,    FIRST_RANK = 0xff,
        B_FILE = 0x4040404040404040,    SECOND_RANK = 0xff00,
        C_FILE = 0x2020202020202020,    THIRD_RANK = 0xff0000,
		D_FILE = 0x1010101010101010,    FOURTH_RANK = 0xff000000,
		E_FILE = 0x808080808080808,     FIFTH_RANK = 0xff00000000,
		F_FILE = 0x404040404040404,     SIXTH_RANK = 0xff0000000000,
		G_FILE = 0x202020202020202,     SEVENTH_RANK = 0xff000000000000,
		H_FILE = 0x101010101010101,     EIGHTH_RANK = 0xff00000000000000,

        NOT_BORDER_WEST = ~A_FILE,
        NOT_BORDER_EAST = ~H_FILE
	};

    // index 1 - pr(0) pl(1)
    // index 2 - white(1) black(0)
    constexpr int pawn_shift[2][2] = {{9, -7}, {7, -9}};

    enum Constants : const map {
        empty = 0,
        full = 0xffffffffffffffff
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
                "h8", "g8", "f8", "e8", "d8", "c8", "b8", "a8",
        };
        static std::string uci_castles[4]{
                "e1g1", "e1c1", "e8g8", "e8c8"
        };
        static std::string uci_promotion[4]{
                "r", "n", "b", "q"
        };
    }
}