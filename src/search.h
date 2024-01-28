#pragma once
#include "search_callback.h"
#include <cassert>

namespace ZeroLogic::Search {

    using namespace Movegen;

    static inline u32 tt_hits;
    static inline Depth seldepth;

    template<Color c>
    struct Movelist{
    private:
        RatedMove<c> list[200];

    public:
        RatedMove<c>* start = list;
        RatedMove<c>* end = list;

        inline void sort() const{
            RatedMove<c>* best = start;
            for (RatedMove<c>* head = start + 1; head < end; ++head){
                if (head->static_rating > best->static_rating)
                    best = head;
            }
            RatedMove buffer = *start;
            *start = *best;
            *best = buffer;
        }

        inline bool contains(std::string& m) const{
            for (RatedMove<c>* head = start; head < end; ++head)
                if (Misc::uci_move(head->move) == m) return true;
            return false;
        }

        inline void print() const{
            for (RatedMove<c>* head = start; head < end; ++head)
                std::cout << Misc::uci_move(head->move) << ", ";
            std::cout << std::endl;
        }
    };

    struct Bounds{
        Value alpha;
        Value beta;
    };

    inline Bounds operator -(Bounds b)
    {
        return {-b.beta, -b.alpha};
    }

    template<SearchType st, Color c>
    inline Value not_a_callback(Position<c>& pos, Move m, Depth d, Bounds b)
    {
        Position<!c> npos = [&]() {
            Bit from    = 1ull << Misc::Mfrom(m);
            Bit to      = 1ull << Misc::Mto(m);

            if      (!(m & FLAG)) return pos.move_silent(from, to);
            else if (m & PR_FLAG)
            {
                if      (m & QP_FLAG) return pos.template move_silent<PAWN, QUEEN>(from, to);
                else if (m & RP_FLAG) return pos.template move_silent<PAWN, ROOK>(from, to);
                else if (m & BP_FLAG) return pos.template move_silent<PAWN, BISHOP>(from, to);
                else if (m & NP_FLAG) return pos.template move_silent<PAWN, KNIGHT>(from, to);
            }
            else if (m & PP_FLAG) return pos.move_pp(from, to);
            else if (m & EP_FLAG) return pos.move_ep(from, to);
            else
            {
                if (m == castlemoves[c >> SHORT])   return pos.template move_castles<SHORT>();
                else                                return pos.template move_castles<LONG>();
            }

            return pos.template move_castles<LONG>(); // error!!!
        }();

        return -search<st>(npos, d - 1, -b);
    }

    template<Color c>
    inline bool nullwindow(Position<c>& pos, Depth d, Value lowerBound, Movelist<c> ml)
    {
        Bounds b = {lowerBound, lowerBound + 1};

        for (; ml.start < ml.end; ++ml.start)
        {
            if (limits.stop()) return INVALID_VALUE;

            ml.sort();
            const Value nodeval = not_a_callback<NSearch>(pos, ml.start->move, d, b);

            if (nodeval > lowerBound)
                return true;
        }

        return false;
    }

    template <Color c>
    Value nsearch(Position<c>& pos, Depth d, Bounds b){
        ++limits.nodecount;

        Movelist<c> ml;
        bool check = enumerate<NSearch, Callback>(pos, d, ml.end);

        if (ml.start == ml.end) {
            if (check) return MATE_NEG - d;
            return DRAW;
        }

        Move bestmove{};
        for ( ; ml.start < ml.end; ++ml.start)
        {
            if (limits.stop()) return INVALID_VALUE;

            ml.sort();
            const Value nodeval = not_a_callback<NSearch>(pos, ml.start->move, d, b);

            if (nodeval > b.alpha) {
                b.alpha = nodeval;
                bestmove = ml.start->move;

                if (b.alpha >= b.beta) break;
            }
        }

        TT::replace({pos.hash, bestmove, d});
        return b.alpha;
    }

    template<Color c>
    Value qsearch(Position<c>& pos, Depth d, Bounds b){
        ++limits.nodecount;
        if (d < seldepth) seldepth = d;
        Movelist<c> ml;

        // we only enumerate captures and queen promotions (unless we are in check)
        bool check = enumerate<QSearch, Callback>(pos, 0, ml.end); // in case of quick pruning a lot of time is wasted on mg

        if (!check) {
            const Value stand_pat = pos.evaluate();
            if (stand_pat >= b.beta || stand_pat + 2*QUEEN_MG < b.alpha) return stand_pat; // quick pruning
            if (stand_pat > b.alpha) b.alpha = stand_pat;

            for (; ml.start < ml.end; ++ml.start)
            {
                if (limits.stop()) return INVALID_VALUE;
                ml.sort();
                if (ml.start->static_rating < 0) break;
                if (stand_pat + ml.start->static_rating + 200 < b.alpha) break; // delta pruning
                const Value nodeval = not_a_callback<QSearch>(pos, ml.start->move, d, b);

                if (nodeval > b.alpha)
                {
                    b.alpha = nodeval;
                    if (b.alpha >= b.beta) break;
                }
            }
        }
        else{
            if (ml.start == ml.end) return MATE_NEG - d;
            for (; ml.start < ml.end; ++ml.start)
            {
                if (limits.stop()) return INVALID_VALUE;
                ml.sort();
                const Value nodeval = not_a_callback<QSearch>(pos, ml.start->move, d, b);

                if (nodeval > b.alpha)
                {
                    b.alpha = nodeval;
                    if (b.alpha >= b.beta) break;
                }
            }
        }
        return b.alpha;
    }

    template<SearchType st = NSearch, Color c>
    inline Value search(Position<c>& pos, Depth d, Bounds b)
    {
        if (st == QSearch || !d)        return qsearch(pos, d, b);
        if constexpr (st == NSearch)    return nsearch(pos, d, b);
        return ZERO;
    }

    template <Color c>
    void display_info(Position<c>& pos, Value evaluation, Depth d) {
        u64 duration_ms = (Time::now() - limits.start_time) / 1000;
        if (!duration_ms) duration_ms = 1;

        std::stringstream info;
        info    << "info"
                << " depth "        << int(d)
                << " seldepth "     << int(d - seldepth)
                << " score "        << Misc::uci_eval(evaluation, d)
                << " nodes "        << limits.nodecount
                << " time "         << duration_ms
                << " nps "          << 1000 * limits.nodecount / duration_ms;

        info    << " pv"            << Misc::uci_pv(pos, d);

        std::cout << info.str() << std::endl << std::flush;
    }

    template<Color c>
    Value go_iteration(Position<c>& pos, u8 depth, Value alpha, Value beta){
        seldepth = 0;

        return nsearch(pos, depth, {alpha, beta});
    }

    template<Color c>
    std::string go(Position<c>& pos, bool display = true){
        limits.nodecount = 0;
        tt_hits = 0;
        std::string bestmove = "invalid";
        TT::init();

        int wlow = 25, whigh = 25;
        Value last_val = ZERO;

        if (limits.type == GAMETIME) limits.adjust();

        for (u8 currdepth = 1; limits.type != DEPTH || currdepth <= limits.allowed_depth; ){
            Value val = go_iteration(pos, currdepth, last_val - wlow, last_val + whigh);
            if (limits.stop()) break;

            if      (val <= last_val - wlow)    wlow *= 3;
            else if (val >= last_val + whigh)   whigh *= 3;
            else {
                last_val = val;
                wlow = 25;
                whigh = 25;
                bestmove = Misc::uci_move(TT::table[*pos.hash].move);
                if (display) display_info(pos, val, currdepth);
                currdepth++;
                if (abs(last_val) > 30000) break;
            }
        }

        std::cout << "bestmove " << bestmove << std::endl;
        TT::clear();
        return bestmove;
    }

    template<Color c>
    void go_single(Position<c>& pos){
        limits.nodecount = 0;
        tt_hits = 0;
        TT::init();

        Value val = go_iteration(pos, limits.allowed_depth, ABSOLUTE_MIN, ABSOLUTE_MAX);
        display_info(pos, val, limits.allowed_depth);
        TT::clear();
    }
}