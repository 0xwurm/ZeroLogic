#pragma once
#include <iostream>

#define COMPILETIME static constexpr __forceinline
#define FORCEINLINE __forceinline

namespace ZeroLogic {
    typedef unsigned short u16;

	// Every move is stored using 16 bits.
    // 6 origin - 4 flags- 6 destination
    //
    // flags: promotion - 1000, capture - 0001, castles - 0100, en passant - 0101
    // promotion: r = 1000, n = 1001, b = 1010, q = 1011
    // castles: (LSBs) w00 = 00, w000 = 01, b00 = 10, b000 = 11
	enum Move : u16{
        MOVE_NULL, MOVE_CHECKMATE = 0x0401, MOVE_DRAW = 0x0802,

        // flags
        MOVE_FLAG = 0b1111 << 6,
        NORMAL_FLAG = MOVE_NULL, CAPTURE_FLAG = 1 << 6, PROMOTION_FLAG = 0b1000 << 6,
        CASTLES_FLAG = 0b100 << 6, EP_FLAG = 0b101 << 6,
        PROMOTION_R = PROMOTION_FLAG, PROMOTION_N = 0b1001 << 6,
        PROMOTION_B = 0b1010 << 6, PROMOTION_Q = 0b1011 << 6,
        PROMOTION_TYPE_FLAG = 3 << 6, CASTLE_TYPE_FLAG = 3,
        CASTLE_PROMO_EP_FLAG = 0b11 << 8, DESTINATION_FLAG = 0x3f
    };
	typedef unsigned long long map;
    typedef unsigned long long Bit;
    typedef unsigned char Square;

    constexpr Move operator&(Move m1, Move m2) {return Move(u16(m1) & u16(m2));}
    constexpr Move operator|(Move m1, Move m2) {return Move(u16(m1) | u16(m2));}
    constexpr Move& operator>>=(Move& m1, int shift) {return m1 = Move(int(m1) >> shift);}


    static std::string start_fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

    enum Piece {
        PAWN = 0, ROOK, KNIGHT, BISHOP, QUEEN, KING,
        PIECE_INVALID = 14
    };

    constexpr Piece operator~(Piece p){return Piece(p^8);} // flip piece color
    inline Piece& operator++(Piece& p){return p = Piece(int(p) + 1);}
    constexpr Piece operator*(Piece p){return Piece(p & 0b111);} // set piece to white

    enum CastleType{
        WHITE_OO = 0, WHITE_OOO, BLACK_OO, BLACK_OOO,
        CASTLE_INVALID = 5
    };

	enum Result {
		checkmate = 101,
		stalemate = 102
	};

    enum Values{
        mate = 32000,
        draw = 0
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
        NOT_BORDER_EAST = ~H_FILE,

        lower = FIRST_RANK | SECOND_RANK | THIRD_RANK | FOURTH_RANK,
        upper = FIFTH_RANK | SIXTH_RANK | SEVENTH_RANK | EIGHTH_RANK,
        bigmid = upper >> 16,
        flank_mid = (A_FILE | B_FILE | C_FILE | F_FILE | G_FILE | H_FILE) & (FOURTH_RANK | FIFTH_RANK),
        mid_files = C_FILE | D_FILE | E_FILE | F_FILE
	};

    namespace Misc{
        constexpr std::string uci_squares[64]{
            "h1", "g1", "f1", "e1", "d1", "c1", "b1", "a1",
            "h2", "g2", "f2", "e2", "d2", "c2", "b2", "a2",
            "h3", "g3", "f3", "e3", "d3", "c3", "b3", "a3",
            "h4", "g4", "f4", "e4", "d4", "c4", "b4", "a4",
            "h5", "g5", "f5", "e5", "d5", "c5", "b5", "a5",
            "h6", "g6", "f6", "e6", "d6", "c6", "b6", "a6",
            "h7", "g7", "f7", "e7", "d7", "c7", "b7", "a7",
            "h8", "g8", "f8", "e8", "d8", "c8", "b8", "a8",
        };
        constexpr std::string uci_castles[4]{
            "e1g1", "e1c1", "e8g8", "e8c8"
        };
        constexpr std::string uci_promotion[4]{
            "r", "n", "b", "q"
        };
    }

    template <class T, int size>
    struct fake_stack{
        T list[size]{};
        int index = 0;
        void put_in(T item) {list[index] = item; index++;}
        T get_out() {index--; return list[index];}
    };


    enum Castling : const map {
        wOO_r = 0x5,
        wOO_k = 0xa,
        wOOO_r = 0x90,
        wOOO_k = 0x28,
        bOO_r = wOO_r << 56,
        bOO_k = wOO_k << 56,
        bOOO_r = wOOO_r << 56,
        bOOO_k = wOOO_k << 56,
        w_king_start = 0x8,
        b_king_start = w_king_start << 56
    };

    enum Constants : const map {
        empty = 0,
        full = 0xffffffffffffffff
    };

	namespace Evaluation {

		enum Phase {
			opening = 0,
			midgame = 1,
			endgame = 2
		};

		enum psqt : const map {
			center = 0x3838000000,
			extendedCenter = 0x3C04043C0000,
			middleStrip = 0xFFFF000000,
			extendedStrip = 0xFFFF0000FFFF00,
			queensStarting = 0x1000000000000010
		};

		enum castleyet : const map {
			wks = 0x2,
			wqs = 0x20,
			weither = wks | wqs,
			bks = wks << 56,
			bqs = wqs << 56,
			beither = bks | bqs,

			wsafe = wks | (wks >> 1) | (wqs << 1) | (wqs << 2),
			bsafe = bks | (bks >> 1) | (bqs << 1) | (bqs << 2)
		};

	}
}