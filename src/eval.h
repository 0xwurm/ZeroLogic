#pragma once

namespace ZeroLogic{
    using namespace Evaluation;

    // eval in centipawns
    template <Color c>
    inline Value Position<c>::evaluate(){
        map brd[14] =
                {
                oPawns, oRooks, oKnights, oBishops, oQueens, oKing,
                0, 0,
                ePawns, eRooks, eKnights, eBishops, eQueens, eKing
                };

        Value mg_val{}, eg_val{}, gamephase{};
        for (Piece p = PAWN; p <= KING; ++p){
            Bitloop(brd[p]){
                Square sq = SquareOf(brd[p]);
                mg_val += mg_table[c][p][sq];
                eg_val += eg_table[c][p][sq];
                gamephase += gamephaseInc[p];
            }
            Bitloop(brd[!p]){
                Square sq = SquareOf(brd[!p]);
                mg_val -= mg_table[!c][p][sq];
                eg_val -= eg_table[!c][p][sq];
                gamephase += gamephaseInc[p];
            }
        }
        int mgPhase = gamephase;
        if (mgPhase > 24) mgPhase = 24;
        int egPhase = 24 - mgPhase;
        return (mg_val * mgPhase + eg_val * egPhase) / 24;
    }
}