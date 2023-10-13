#pragma once

namespace ZeroLogic {

	// Every move is stored using 16 bits.
    // 6-6-1-3
	// 6 (origin), 6 (destination), 1 (if promotion, capture), 3 (10 castles, 1 en passant, 11 promotion q, 101 promotion r, 110 promotion b, 111 promotion n, 000 normal move no capture, 100 normal move capture)
	// if castles: first 2 bits (11 wk, 10 wq, 01 bk, 00 bq)
	typedef unsigned short Move;
	typedef unsigned long long Bitboard;

	enum Result {
		checkmate = 101,
		stalemate = 102
	};

    enum Values{
        mate = 32000,
        draw = 0
    };

	enum Piece {
		P = 0,
		R = 1,
		N = 2,
		B = 3,
		Q = 4,
		K = 5,

		wP = 0,
		wR = 1,
		wN = 2,
		wB = 3,
		wQ = 4,
		wK = 5,
		bP = 6,
		bR = 7,
		bN = 8,
		bB = 9,
		bQ = 10,
		bK = 11,

		nope = 12
	};

	enum Square {
		h1, g1, f1, e1, d1, c1, b1, a1,
		h2, g2, f2, e2, d2, c2, b2, a2,
		h3, g3, f3, e3, d3, c3, b3, a3,
		h4, g4, f4, e4, d4, c4, b4, a4,
		h5, g5, f5, e5, d5, c5, b5, a5,
		h6, g6, f6, e6, d6, c6, b6, a6,
		h7, g7, f7, e7, d7, c7, b7, a7,
		h8, g8, f8, e8, d8, c8, b8, a8
	};

	enum Ranks : const Bitboard {
		first = 0xFF,
		second = 0xFF00,
		third = 0xFF0000,
		fourth = 0xFF000000,
		fifth = 0xFF00000000,
		sixth = 0xFF0000000000,
		seventh = 0xFF000000000000,
		eighth = 0xFF00000000000000,
		lower = first | second | third | fourth,
		upper = fifth | sixth | seventh | eighth,
		bigmid = upper >> 16
	};

	enum Files : const Bitboard {
		A = 0x8080808080808080,
		Bf = 0x4040404040404040,
		C = 0x2020202020202020,
		D = 0x1010101010101010,
		E = 0x808080808080808,
		F = 0x404040404040404,
		G = 0x202020202020202,
		H = 0x101010101010101
	};

	namespace Movegen {

		enum Borders : const Bitboard {
			north = 0xffffffffffffff,
			south = 0xffffffffffffff00,
			east = 0xfefefefefefefefe,
			west = 0x7f7f7f7f7f7f7f7f,
			northeast = 0xfefefefefefefe,
			northwest = 0x7f7f7f7f7f7f7f,
			southeast = 0xfefefefefefefe00,
			southwest = 0x7f7f7f7f7f7f7f00,


			// knight borders: up(u)/down(d)/left(l)/right(r)

			urr = 0xfcfcfcfcfcfcfc,
			uur = 0xfefefefefefe,
			drr = 0xfcfcfcfcfcfcfc00,
			ddr = 0xfefefefefefe0000,
			ull = 0x3f3f3f3f3f3f3f,
			uul = 0x7f7f7f7f7f7f,
			dll = 0x3f3f3f3f3f3f3f00,
			ddl = 0x3f3f3f3f3f3f3f00
		};

		enum Castling : const Bitboard {
			wkCheck = 0xE,
			wqCheck = 0x38,
			bkCheck = 0xE00000000000000,
			bqCheck = 0x3800000000000000,
			wkOcc = 0x6,
			wqOcc = 0x70,
			bkOcc = 0x600000000000000,
			bqOcc = 0x7000000000000000,
			wkhere = 0x8,
			bkhere = 0x800000000000000
		};

		enum Constants : const Bitboard {
			empty = 0,
			full = 0xFFFFFFFFFFFFFFFF
		};

	}

	namespace State {

		enum move : const Bitboard {
			wkr = 1,
			wqr = 0x80,
			bkr = 0x100000000000000,
			bqr = 0x8000000000000000,

			wkfullk = 0b1010,
			wkfullr = 0b101,
			wqfullk = 0b101000,
			wqfullr = 0b10010000,

			bkfullk = 0xA00000000000000,
			bkfullr = 0x500000000000000,
			bqfullk = 0x2800000000000000,
			bqfullr = 0x9000000000000000
		};

		enum moveflags {
			flagmask = 0b111,

			normal = 0,
			capture = 0b100,
			passant = 0b1,
			promotionq = 0b11,
			promotionr = 0b101,
			promotionb = 0b110,
			promotionn = 0b111,
			castles = 0b10,
			promotioncapture = 0b1000,

			wkflag = (3 << 14),
			wqflag = (2 << 14),
			bkflag = (1 << 14),
			bqflag = 0,

			none = 0

		};

		enum castleType {
			wk = 0,
			wq = 1,
			bk = 2,
			bq = 3,
		};

	}

	namespace Evaluation {

		enum Phase {
			opening = 0,
			midgame = 1,
			endgame = 2
		};

		enum psqt : const Bitboard {
			center = 0x3838000000,
			extendedCenter = 0x3C04043C0000,
			middleStrip = 0xFFFF000000,
			extendedStrip = 0xFFFF0000FFFF00,
			queensStarting = 0x1000000000000010
		};

		enum castleyet : const Bitboard {
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