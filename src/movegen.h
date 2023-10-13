#pragma once
#include "gamestate.h"
#include "variables.h"

namespace ZeroLogic{

    struct Position{
        Bitboard king;
        Bitboard queen;
        Bitboard rook;
        Bitboard bishop;
        Bitboard knight;
        Bitboard pawn;

        Bitboard e_king;
        Bitboard e_queen;
        Bitboard e_rook;
        Bitboard e_bishop;
        Bitboard e_knight;
        Bitboard e_pawn;

        Bitboard own;
        Bitboard enemy;
        Bitboard occupied;
        Bitboard empty;
        Bitboard check_mask;
        Bitboard pin_mask_straight;
        Bitboard pin_mask_diagonal;
        Bitboard check_ring;
    };
    struct MoveList{
        Move list[218]{};
        unsigned short index_front = 0;
        unsigned short index_back = 217;
    };

    template <bool white>
    class MoveGenerator{
    public:
        MoveGenerator(Gamestate* gs, MoveList* ml);
        void go();
        bool attacked(int index);
    private:
        Gamestate* gs{};
        MoveList* ml{};

        Position pos{};

        void make_ring(int index);

        void make_masks(int index);
        void rook_check_routine(Bitboard attacker, Bitboard mask, int index, bool* logged_check);
        void rook_pin_routine(Bitboard mask, int index, Bitboard old_attacker);
        void bishop_check_routine(Bitboard attacker, Bitboard mask, int index);
        void bishop_pin_routine(Bitboard mask, int index, Bitboard old_attacker);

        template <bool check>
        void get_moves_pawn(); // no-check variant includes castles
        Bitboard get_pawn_attacks(Bitboard origin);
    };

}