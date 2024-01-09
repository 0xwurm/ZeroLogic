#pragma once

namespace ZeroLogic::Evaluation{

    // eval in centipawns
    template <Color c>
    FORCEINLINE static Value evaluate(const Boardstate::Board& board){
        map brd[2][6] =
                {
                {board.pawns<c>(), board.rooks<c>(), board.knights<c>(), board.bishops<c>(), board.queens<c>(), board.king<c>()},
                {board.pawns<!c>(), board.rooks<!c>(), board.knights<!c>(), board.bishops<!c>(), board.queens<!c>(), board.king<!c>()}
                };

        Value mg_val{}, eg_val{}, gamephase{};
        for (int p = 0; p < 6; p++){
            Bitloop(brd[0][p]){
                Square sq = SquareOf(brd[0][p]);
                mg_val += mg_table[c][p][sq];
                eg_val += eg_table[c][p][sq];
                gamephase += gamephaseInc[p];
            }
            Bitloop(brd[1][p]){
                Square sq = SquareOf(brd[1][p]);
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