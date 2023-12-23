#pragma once

namespace ZeroLogic::Eval{

    // ++ nodecount

    // eval in centipawns
    template <bool white>
    static eval evaluate(const Boardstate::Board& board){
        u64 val =   BitCount(board.WPawn) - BitCount(board.BPawn);
            val +=  3 * (BitCount(board.WBishop | board.WKnight) - BitCount(board.BBishop | board.BKnight));
            val +=  5 * (BitCount(board.WRook) - BitCount(board.BRook));
            val +=  9 * (BitCount(board.WQueen) - BitCount(board.BQueen));
        if constexpr (!white) val *= -1;
        return static_cast<eval>(100 * val);
    }
}