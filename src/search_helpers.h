#pragma once

namespace ZeroLogic {

    template<bool white>
    FORCEINLINE bool is_check(const Board &board) {
        constexpr bool us = white;
        constexpr bool enemy = !us;

        const map king = King<us>(board);
        const Square king_square = SquareOf(king);

        map check_by_knight = Lookup::knight[king_square] & Knights<enemy>(board);
        if (check_by_knight) return true;

        const map pr = pawn_atk_right<enemy>(Pawns<enemy>(board));
        const map pl = pawn_atk_left<enemy>(Pawns<enemy>(board));
        if ((pr | pl) & king) return true;

        map atkR = Lookup::r_atk(king_square, board.Occ) & RookOrQueen<enemy>(board);
        if (atkR) return true;
        map atkB = Lookup::b_atk(king_square, board.Occ) & BishopOrQueen<enemy>(board);
        if (atkB) return true;

        return false;
    }
}
