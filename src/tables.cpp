#include "tables.h"
#include <map>

namespace ZeroLogic::Tables{
    
    std::map<int, long long> borders_1 = { {1, west}, {7, northeast}, {8, north}, {9, northwest} };
    std::map<int, long long> borders_2 = { {1, east}, {7, southwest}, {8, south}, {9, southeast} };

    unsigned long long partial_mask(int shift, Square s, Bitboard blockers, bool mask) {
        unsigned long long partMoves = 1ull << s;
        while (!(partMoves & borders_1[shift])) {
            partMoves |= (partMoves << shift);
            if (partMoves & blockers) break;
        }
        unsigned long long partMoves1 = 1ull << s;
        while (!(partMoves1 & borders_2[shift])) {
            partMoves1 |= (partMoves1 >> shift);
            if (partMoves1 & blockers) break;
        }
        if (mask) return (partMoves | partMoves1) & ~(1ull << s) & ~borders_1[shift] & ~borders_2[shift];
        else return (partMoves | partMoves1) & ~(1ull << s);
    }

    long long get_mask(bool do_rook, Bitboard blockers, Square s, bool mask) {
        unsigned long long moves = 0;
        if (do_rook) {
            moves |= partial_mask(1, s, blockers, mask);
            moves |= partial_mask(8, s, blockers, mask);
        }
        else {
            moves |= partial_mask(7, s, blockers, mask);
            moves |= partial_mask(9, s, blockers, mask);
        }
        return static_cast<long long>(moves);
    }

    Bitboard get_index(Bitboard x, long long mask) {
        Bitboard res = 0;
        for (Bitboard bb = 1; mask != 0; bb += bb) {
            if (x & mask & -mask) {
                res |= bb;
            }
            mask &= (mask - 1);
        }
        return res;
    }

    void fill(){
        int index = 0;
        for (int i = 0; i < 64; i++) {
            long long rook_mask = get_mask(true, 0, static_cast<Square>(i), true);
            long long bishop_mask = get_mask(false, 0, static_cast<Square>(i), true);

            rook[i].mask = rook_mask;
            bishop[i].mask = bishop_mask;

            rook[i].ptr = &slider_attack_table[index];

            long long subset = 0;
            do {
                subset = (subset - rook_mask) & rook_mask;
                Bitboard attacks = get_mask(true, subset, static_cast<Square>(i), false);
                *(rook[i].ptr + get_index(subset, rook_mask)) = attacks;
                index++;
            } while (subset);

            bishop[i].ptr = &slider_attack_table[index];

            do {
                subset = (subset - bishop_mask) & bishop_mask;
                Bitboard attacks = get_mask(false, subset, static_cast<Square>(i), false);
                *(bishop[i].ptr + get_index(subset, bishop_mask)) = attacks;
                index++;
            } while (subset);
        }
    }
    
    Bitboard get_attack_rook(Bitboard occupied, int index){
        return *(rook[index].ptr + get_index(occupied, rook[index].mask));
    }
    Bitboard get_attack_bishop(Bitboard occupied, int index){
        return *(bishop[index].ptr + get_index(occupied, bishop[index].mask));
    }

}
