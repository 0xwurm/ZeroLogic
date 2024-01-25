#pragma oncenamespace ZeroLogic::Movegen{    template<Color c>    COMPILETIME map last_rank() {        if constexpr (c) return RANK_EIGHT_MASK;        else return RANK_ONE_MASK;    }    template<Color c>    COMPILETIME map not_last_rank() {        if constexpr (c) return ~RANK_EIGHT_MASK;        else return ~RANK_ONE_MASK;    }    template<Color c>    COMPILETIME map third_rank() {        if constexpr (c) return RANK_THREE_MASK;        else return RANK_SIX_MASK;    }    template<Color c>    COMPILETIME map EPRank() {        if constexpr (c) return 0xffull << 32;        else return 0xffull << 24;    }    template <Color c>    inline void pin(const Square king, Square pinner, map& pinmask, Position<c>& pos){        const map path = Lookup::pin_between[64 * king + pinner];        if (path & pos.Occ)            pinmask |= path;    }    inline void slider_check(const Square king, Square attacker, Mask& masks){        if (masks.check == full)    masks.check = Lookup::pin_between[64 * king + attacker];        else                        masks.check = 0;        masks.kingban |= Lookup::check_between[64 * king + attacker];    }    template <Color c>    inline void ep_pin(Square king_square, Position<c>& pos){        if ((EPRank<c>() & pos.oKing)            && (EPRank<c>() & pos.eStraightSliders())            && (EPRank<c>() & pos.oPawns))        {            Bit lep = pos.oPawns & (pos.target & ~FILE_H) >> 1;            Bit rep = pos.oPawns & (pos.target & ~FILE_A) << 1;            if (lep)            {                map ep_occ = pos.Occ & ~(pos.target | lep);                if ((Lookup::r_atk(king_square, ep_occ) & EPRank<c>()) & pos.eStraightSliders()) pos.ep = false;            }            if (rep)            {                map ep_occ = pos.Occ & ~(pos.target | rep);                if ((Lookup::r_atk(king_square, ep_occ) & EPRank<c>()) & pos.eStraightSliders()) pos.ep = false;            }        }    }    template <Color c>    inline Mask make_masks(Position<c>& pos){        Mask masks{}; // initializes checkmask as full        const Square king_square = SquareOf(pos.oKing);        {            // checks by knights            {                map check_by_knight = Lookup::knight[king_square] & pos.eKnights;                if (check_by_knight) masks.check = check_by_knight;            }            // checks & attacks by pawns            {                const map pr = moveP<!c, RIGHT, true>(pos.ePawns);                const map pl = moveP<!c, LEFT, true>(pos.ePawns);                masks.kingban |= pr | pl;                if      (pr & pos.oKing) masks.check = moveP<c, RIGHT>(pos.oKing);                else if (pl & pos.oKing) masks.check = moveP<c, LEFT>(pos.oKing);            }            // checks & pins by sliders            if (Lookup::r_atk(king_square, 0) & pos.eStraightSliders())            {                map atk = Lookup::r_atk(king_square, pos.Occ) & pos.eStraightSliders();                Bitloop(atk)                    slider_check(king_square, SquareOf(atk), masks);                map pinning = Lookup::r_xray(king_square, pos.Occ) & pos.eStraightSliders();                Bitloop(pinning)                    pin(king_square, SquareOf(pinning), masks.rook, pos);            }            if (Lookup::b_atk(king_square, 0) & pos.eDiagonalSliders())            {                map atk = Lookup::b_atk(king_square, pos.Occ) & pos.eDiagonalSliders();                Bitloop(atk)                    slider_check(king_square, SquareOf(atk), masks);                map pinning = Lookup::b_xray(king_square, pos.Occ) & pos.eDiagonalSliders();                Bitloop(pinning)                    pin(king_square, SquareOf(pinning), masks.bishop, pos);            }            if (pos.target) ep_pin(king_square, pos);        }        masks.kingmoves = Lookup::king[king_square] & ~pos.us & ~masks.kingban;        if (masks.kingmoves){            // attacks by knights            {                map knights = pos.eKnights;                Bitloop(knights)                    masks.kingban |= Lookup::knight[SquareOf(knights)];            }            // attacks by rooks and queens            {                map rq = pos.eStraightSliders();                Bitloop(rq)                    masks.kingban |= Lookup::r_atk(SquareOf(rq), pos.Occ);            }            // attacks by bishops and queens            {                map bq = pos.eDiagonalSliders();                Bitloop(bq)                    masks.kingban |= Lookup::b_atk(SquareOf(bq), pos.Occ);            }            // attacks by king            masks.kingban |= Lookup::king[SquareOf(pos.eKing)];            masks.kingmoves &= ~masks.kingban;        }        return masks;    }    template<bool ischeck, bool arg1, bool arg2, class Callback, Color c>    inline void _enumerate(Position<c>& pos, Mask& masks, const u8& depth, typename Callback::template specific<c>& s){        const map noPin = ~(masks.rook | masks.bishop);        // compiler hints        if constexpr (ischeck)        {            masks.bishop = 0;            masks.rook = 0;        }        else            masks.check = full;        const map legalSquares = ~pos.us & masks.check;        // castling        if constexpr (!ischeck)        {            if (pos.template canCastle<SHORT>(masks))                Callback::template castlemove<SHORT, arg2, arg1>(pos, depth, s);            if (pos.template canCastle<LONG>(masks))                Callback::template castlemove<LONG, arg2, arg1>(pos, depth, s);        }        // pawns        {            const map nPawns = pos.oPawns & noPin;            const map sPawns = pos.oPawns & masks.rook;            const map dPawns = pos.oPawns & masks.bishop;            map pr = (moveP<c, RIGHT, true>(dPawns) & masks.bishop                    | moveP<c, RIGHT, true>(nPawns))                    & pos.enemy & masks.check;            map pl = (moveP<c, LEFT, true>(dPawns) & masks.bishop                    | moveP<c, LEFT, true>(nPawns))                    & pos.enemy & masks.check;            map pf = (moveP<c, FORWARD>(sPawns) & masks.rook                     | moveP<c, FORWARD>(nPawns))                     & ~pos.Occ;            map pp = moveP<c, FORWARD>(pf & third_rank<c>())                    & ~pos.Occ & masks.check;       pf &= masks.check;            map pr_promo = pr & last_rank<c>();     pr &= not_last_rank<c>();            map pl_promo = pl & last_rank<c>();     pl &= not_last_rank<c>();            map pf_promo = pf & last_rank<c>();     pf &= not_last_rank<c>();            Callback::template pawn_move<arg2, arg1>(pos, pr, pl, pr_promo, pl_promo, pf, pp, pf_promo, depth, s);            // en passant            if (pos.ep && (pos.target & ~masks.bishop))            {                Bit lep = (ischeck ? nPawns : pos.oPawns & ~masks.rook)                        & moveP<c, EAST, true>(pos.target & masks.check);                Bit rep = (ischeck ? nPawns : pos.oPawns & ~masks.rook)                        & moveP<c, WEST, true>(pos.target & masks.check);                if constexpr (!ischeck)                {                    if (rep & masks.bishop) rep = moveP<c, RIGHT>(rep) & masks.bishop;                    if (lep & masks.bishop) lep = moveP<c, LEFT>(lep) & masks.bishop;                }                Callback::template ep_move<arg2, arg1>(pos, lep, rep, depth, s);            }        }        // knights        {            map nKnights = pos.oKnights & noPin;            Bitloop(nKnights) {                const Square sq = SquareOf(nKnights);                map moves = Lookup::knight[sq] & legalSquares;                Callback::template silent_move<KNIGHT, arg2, arg1>(pos, moves, 1ull << sq, depth, s);            }        }        // diagonal sliders        {            map pinned = (pos.oQueens | pos.oBishops) & masks.bishop;            map not_pinned = (pos.oQueens | pos.oBishops) & noPin;            Bitloop(pinned){                Square sq = SquareOf(pinned);                Bit from = 1ull << sq;                map moves = Lookup::b_atk(sq, pos.Occ) & masks.bishop & legalSquares;                if (from & pos.oQueens) Callback::template silent_move<QUEEN, arg2, arg1>(pos, moves, from, depth, s);                else                    Callback::template silent_move<BISHOP, arg2, arg1>(pos, moves, from, depth, s);            }            Bitloop(not_pinned){                const Square sq = SquareOf(not_pinned);                Bit from = 1ull << sq;                map moves = Lookup::b_atk(sq, pos.Occ) & legalSquares;                if (from & pos.oQueens) Callback::template silent_move<QUEEN, arg2, arg1>(pos, moves, from, depth, s);                else                    Callback::template silent_move<BISHOP, arg2, arg1>(pos, moves, from, depth, s);            }        }        // straight sliders        {            map pinned = (pos.oQueens | pos.oRooks) & masks.rook;            map not_pinned = (pos.oQueens | pos.oRooks) & noPin;            Bitloop(pinned){                const Square sq = SquareOf(pinned);                Bit from = 1ull << sq;                map moves = Lookup::r_atk(sq, pos.Occ) & masks.rook & legalSquares;                if (from & pos.oQueens) Callback::template silent_move<QUEEN, arg2, arg1>(pos, moves, from, depth, s);                else                    Callback::template rookmove<arg2, arg1>(pos, moves, from, depth, s);            }            Bitloop(not_pinned){                const Square sq = SquareOf(not_pinned);                Bit from = 1ull << sq;                map moves = Lookup::r_atk(sq, pos.Occ) & legalSquares;                if (from & pos.oQueens) Callback::template silent_move<QUEEN, arg2, arg1>(pos, moves, from, depth, s);                else                    Callback::template rookmove<arg2, arg1>(pos, moves, from, depth, s);            }        }        Callback::template kingmove<arg2, arg1>(pos, masks.kingmoves, depth, s);    }    template <class Callback, bool arg1 = false, bool arg2 = false, Color c>    inline void enumerate(Position<c>& pos, Mask& masks, const u8& depth, typename Callback::template specific<c>& s){        if (masks.check == full)        {            _enumerate<false, arg1, arg2, Callback>(pos, masks, depth, s);        }        else if (masks.check)        {            _enumerate<true, arg1, arg2, Callback>(pos, masks, depth, s);        }        else        {            Callback::template kingmove<arg2, arg1>(pos, masks.kingmoves, depth, s);        }    }    template <class Callback, bool leaf = false, bool root = false, Color c>    inline void enumerate(Position<c>& pos, const u8& depth, typename Callback::template specific<c>& s){        Mask masks = make_masks(pos);        enumerate<Callback, leaf, root>(pos, masks, depth, s);    }    template <SearchType st, class Callback, Color c>    inline bool enumerate(Position<c>& pos, const u8 depth, typename Callback::template specific<c>& s){        Mask masks = make_masks(pos);        if (masks.check != full)        {            enumerate<Callback, true, st>(pos, masks, depth, s);            return true;        }        else        {            enumerate<Callback, false, st>(pos, masks, depth, s);            return false;        }    }}