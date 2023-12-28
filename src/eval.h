#pragma once

namespace ZeroLogic::Evaluation{

    // eval in centipawns
    template <bool white>
    FORCEINLINE static Value evaluate(const Boardstate::Board& board){
        map brd[2][6] =
                {
                {board.BPawn, board.BRook, board.BKnight, board.BBishop, board.BQueen, board.BKing},
                {board.WPawn, board.WRook, board.WKnight, board.WBishop, board.WQueen, board.WKing}
                };

        Value mg_val{}, eg_val{}, gamephase{};
        for (int p = 0; p < 6; p++){
            Bitloop(brd[white][p]){
                Square sq = SquareOf(brd[white][p]);
                mg_val += mg_table[white][p][sq];
                eg_val += eg_table[white][p][sq];
                gamephase += gamephaseInc[p];
            }
            Bitloop(brd[!white][p]){
                Square sq = SquareOf(brd[!white][p]);
                mg_val -= mg_table[!white][p][sq];
                eg_val -= eg_table[!white][p][sq];
                gamephase += gamephaseInc[p];
            }
        }
        int mgPhase = gamephase;
        if (mgPhase > 24) mgPhase = 24;
        int egPhase = 24 - mgPhase;
        return (mg_val * mgPhase + eg_val * egPhase) / 24;
    }
}