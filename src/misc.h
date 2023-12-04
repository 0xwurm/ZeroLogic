#pragma once
#include "variables.h"
#include "transposition.h"
#include <sstream>
#include "gamestate.h"

namespace ZeroLogic::Misc{

    template <CastleType type>
    COMPILETIME std::string uci_move(){
        return uci_castles[type];
    }

    template <Piece promotion_to>
    FORCEINLINE static std::string uci_move(Square from, Square to){
        if constexpr (promotion_to == PIECE_INVALID)    return uci_squares[from] + uci_squares[to];
        else                                            return uci_squares[from] + uci_squares[to] + uci_promotion[promotion_to - 1];
    }

    FORCEINLINE static Boardstate::move_container engine_move(const std::string& move, const Boardstate::Board& board, const Boardstate::State& state){

        // castles
        if (board.WKing & w_king_start) {
            if (move == uci_castles[WHITE_OO])
                    return {state, board, PIECE_INVALID, PIECE_INVALID, false, WHITE_OO, 0, 0, false};
            else if (move == uci_castles[WHITE_OOO])
                    return {state, board, PIECE_INVALID, PIECE_INVALID, false, WHITE_OOO, 0, 0, false};
        }
        if (board.BKing & b_king_start){
            if (move == uci_castles[BLACK_OO])
                return {state, board, PIECE_INVALID, PIECE_INVALID, false, BLACK_OO, 0, 0, false};
            else if (move == uci_castles[BLACK_OOO])
                return {state, board, PIECE_INVALID, PIECE_INVALID, false, BLACK_OOO, 0, 0, false};
        }

        Bit from = 1ull << getNumNotation(move[0], move[1]);
        Bit to = 1ull << getNumNotation(move[2], move[3]);
        bool capture = false;
        Piece promotion_to = PIECE_INVALID;
        Piece piece;

        if (to & board.Occ) capture = true;
        if (move.size() == 5){
            switch (move[4]){
                case 'r': promotion_to = ROOK; break;
                case 'b': promotion_to = BISHOP; break;
                case 'n': promotion_to = KNIGHT; break;
                case 'q': promotion_to = QUEEN; break;
                default: std::cout << "invalid move: '" << move << "'" << std::endl; // for debugging purposes -> remove
                    return {state, board, PIECE_INVALID, PIECE_INVALID, false, CASTLE_INVALID, 0, 0, false};
            }
        }

        if      (from & (board.WPawn | board.BPawn))        piece = PAWN;
        else if (from & (board.WRook | board.BRook))        piece = ROOK;
        else if (from & (board.WBishop | board.BBishop))    piece = BISHOP;
        else if (from & (board.WKnight | board.BKnight))    piece = KNIGHT;
        else if (from & (board.WQueen | board.BQueen))      piece = QUEEN;
        else if (from & (board.WKing | board.BKing))        piece = KING;
        else {
            std::cout << "invalid move: '" << move << "'" << std::endl;
            return {state, board, PIECE_INVALID, PIECE_INVALID, false, CASTLE_INVALID, 0, 0, false};
        }

        if (piece == PAWN){
            int from_i = getNumNotation(move[0], move[1]), to_i = getNumNotation(move[2], move[3]);
            if ((abs(from_i - to_i) == 7 || abs(from_i - to_i) == 9) && !capture)
                return {state, board, PAWN, PIECE_INVALID, false, CASTLE_INVALID, from, to, true};
        }

        return {state, board, piece, promotion_to, capture, CASTLE_INVALID, from, to, false};

    }

    static std::string uci_eval(eval val){
        std::stringstream output;
        output << " score ";
        if      (val >= MATE_POS)   output << "mate "  << (val - MATE_POS)/2 + 1;
        else if (val <= MATE_NEG)   output << "mate "  << (val + MATE_POS)/2 + 1;
        else                        output << "cp "    << val;
        return output.str();
    }
}

