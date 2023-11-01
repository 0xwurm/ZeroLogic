#pragma once
#include "gamestate.h"
#define BitCount(X) __popcnt64(X)

namespace ZeroLogic::Eval{
    // eval in pawns
    FORCEINLINE static u16 eval(const Boardstate::Board& board){
        u16 val =   BitCount(board.WPawn) - BitCount(board.BPawn);
            val +=  3 * (BitCount(board.WBishop | board.WKnight) - BitCount(board.BBishop | board.BKnight));
            val +=  5 * (BitCount(board.WRook) - BitCount(board.BRook));
            val +=  9 * (BitCount(board.WQueen) - BitCount(board.BQueen));
        return val;
    }
}