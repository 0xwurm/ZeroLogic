#include "gamestate.h"
#include "misc.h"
#include <cmath>

namespace ZeroLogic::Misc {

    const std::string raw_to_string_table[64] = { "h1", "g1", "f1", "e1", "d1", "c1", "b1", "a1", "h2", "g2", "f2", "e2", "d2", "c2", "b2", "a2", "h3", "g3", "f3", "e3", "d3", "c3", "b3", "a3", "h4", "g4", "f4", "e4", "d4", "c4", "b4", "a4", "h5", "g5", "f5", "e5", "d5", "c5", "b5", "a5", "h6", "g6", "f6", "e6", "d6", "c6", "b6", "a6", "h7", "g7", "f7", "e7", "d7", "c7", "b7", "a7", "h8", "g8", "f8", "e8", "d8", "c8", "b8", "a8" };

    // index = char - 97
    const unsigned short char_to_raw_table_files[8] = { 7, 6, 5, 4, 3, 2, 1, 0 };
    // index = char - 49
    const unsigned short char_to_raw_table_ranks[8] = { 0, 8, 16, 24, 32, 40, 48, 56 };


    std::string numToString(Move m) {
        Move flagged = m & flagmask;

        if (flagged != castles) {

            Move origin = m >> 10;
            m <<= 6;
            Move destination = m >> 10;

            if (flagged == promotionq) {
                return raw_to_string_table[origin] + raw_to_string_table[destination] + "q";
            }
            else if (flagged == promotionb) {
                return raw_to_string_table[origin] + raw_to_string_table[destination] + "b";
            }
            else if (flagged == promotionr) {
                return raw_to_string_table[origin] + raw_to_string_table[destination] + "r";
            }
            else if (flagged == promotionn) {
                return raw_to_string_table[origin] + raw_to_string_table[destination] + "n";
            }
            else {
                return raw_to_string_table[origin] + raw_to_string_table[destination];
            }
        }
        else {
            Move castleFlag = m & wkflag;

            if (castleFlag == wkflag) {
                return "e1g1";
            }
            else if (castleFlag == bkflag) {
                return "e8g8";
            }
            else if (castleFlag == wqflag){
                return "e1c1";
            }
            else {
                return "e8c8";
            }
        }
    }

    Move stringToNum(std::string m, Gamestate* gs) {

        int eblackConst = (gs->white_to_move) ? 6 : 0;

        if (m == "e1g1") {
            if (gs->position[wK] & Movegen::wkhere) { return wkflag | castles; }
        }
        else if (m == "e1c1") {
            if (gs->position[wK] & Movegen::wkhere) { return wqflag | castles; }
        }
        else if (m == "e8g8") {
            if (gs->position[bK] & Movegen::bkhere) { return bkflag | castles; }
        }
        else if (m == "e8c8") {
            if (gs->position[bK] & Movegen::bkhere) { return bqflag | castles; }
        }
        Move destination = char_to_raw_table_files[m[2] - 97] + char_to_raw_table_ranks[m[3] - 49];
        Move origin = char_to_raw_table_files[m[0] - 97] + char_to_raw_table_ranks[m[1] - 49];
        Bitboard destinationmask = 1ull << destination;
        Bitboard originmask = 1ull << origin;

        bool captureb = false;
        for (int i = 0; i < 6; i++) { if (gs->position[i + eblackConst] & destinationmask) { captureb = true; break; } }

        if (!captureb) {
            if (m.size() == 5) { // meaning if move is a promotion
                if (m.back() == 'q') { return (origin << 10) | (destination << 4) | promotionq; }
                if (m.back() == 'b') { return (origin << 10) | (destination << 4) | promotionb; }
                if (m.back() == 'r') { return (origin << 10) | (destination << 4) | promotionr; }
                if (m.back() == 'n') { return (origin << 10) | (destination << 4) | promotionn; }
            }
            else {
                int blackConst = (gs->white_to_move) ? 0 : 6;
                int movedpiece = moveflags::none;
                for (int i = 0; i < 6; i++) { if (gs->position[i + blackConst] & originmask) { movedpiece = i; break; } }

                if (movedpiece == P && gs->enPassant & destinationmask) {
                    return (origin << 10) | (destination << 4) | passant;
                }
                else {
                    return (origin << 10) | (destination << 4) | normal;
                }
            }
        }
        else {
            if (m.size() == 5) {
                if (m.back() == 'q') { return (origin << 10) | (destination << 4) | promotioncapture | promotionq; }
                if (m.back() == 'b') { return (origin << 10) | (destination << 4) | promotioncapture | promotionb; }
                if (m.back() == 'r') { return (origin << 10) | (destination << 4) | promotioncapture | promotionr; }
                if (m.back() == 'n') { return (origin << 10) | (destination << 4) | promotioncapture | promotionn; }
            }
            else {
                return (origin << 10) | (destination << 4) | capture;
            }

        }
    }

}
