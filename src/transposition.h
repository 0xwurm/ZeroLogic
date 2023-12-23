#pragma once
#include <random>

namespace ZeroLogic::TT {

    static u64 keys[781]{};

    static u64 ws_key;
    static u64 wl_key;
    static u64 bs_key;
    static u64 bl_key;
    static u64 w_key;
    static int ep0 = 773; // not in use

    static void init_keys() {
        std::mt19937_64 rand{3141592653589793238ull};
        for (u64 &key: keys)
            key = rand();
        ws_key = keys[769];
        wl_key = keys[770];
        bs_key = keys[771];
        bl_key = keys[772];
        w_key = keys[780];
    }

    template<Piece p, bool white>
    static FORCEINLINE u64 get_key(Square sq) {
        if constexpr (white) {
            if constexpr (p == PAWN) return keys[sq];
            else if constexpr (p == BISHOP) return keys[64 + sq];
            else if constexpr (p == KNIGHT) return keys[128 + sq];
            else if constexpr (p == ROOK) return keys[192 + sq];
            else if constexpr (p == QUEEN) return keys[256 + sq];
            else if constexpr (p == KING) return keys[320 + sq];
        } else {
            if constexpr (p == PAWN) return keys[384 + sq];
            else if constexpr (p == BISHOP) return keys[448 + sq];
            else if constexpr (p == KNIGHT) return keys[512 + sq];
            else if constexpr (p == ROOK) return keys[576 + sq];
            else if constexpr (p == QUEEN) return keys[640 + sq];
            else if constexpr (p == KING) return keys[704 + sq];
        }
    }
}

#include "gamestate.h"

namespace ZeroLogic::Perft::TT{

    struct entry{
        u64 hash{};
        u32 nodecount{};
        u8 depth{};
    };

    static u32 key_mask = 0x3ffffff;
    static entry* table;

    static void init(u32 size){
        table = (entry*) calloc(size, sizeof(entry));
        for (u32 s = 0; s <= size; s++)
            table[s].depth = 0xff;
        key_mask = size;
    }
    static void clear(){
        free(table);
    }

}
namespace ZeroLogic::Search::TT{

    // move encoding:
    // 111111  (6) - to
    // 111111  (6) - from
    // 1111    (4) - flags

    // 1000 - castles
    // 0001 - rook promotion
    // 0010 - knight promotion
    // 0011 - bishop promotion
    // 0100 - queen promotion

    struct entry{
        u64 hash{};
        u8 depth{};
        u16 move{};
    };

    static u32 key_mask = 0x3ffffff;
    static entry* table;

    static void init(u32 size){
        table = (entry*) calloc(size, sizeof(entry));
        key_mask = size;
    }
    static void clear(){
        free(table);
    }

}
