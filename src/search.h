#pragma once

namespace ZeroLogic::Search {

    using namespace Movegen;

    static inline u32 nodecount, tt_hits;
    static inline u8 seldepth;
    static inline u8 full_depth;

    static inline std::time_t start_time;

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
    };

    template<Color c>
    static Value qsearch(Position<c>& pos, Value alpha, Value beta);

    template <Color c>
    static Value nsearch(Position<c>& pos, const u8 depth, Value alpha, Value beta){
        ++nodecount;

        Movelist<c> ml;
        bool check = enumerate<NSearch, Callback>(pos, depth, ml.end);

        if (ml.start == ml.end) {
            if (check) return MATE_NEG - depth;
            return DRAW;
        }

        Move bestmove{};
        for ( ; ml.start < ml.end; ++ml.start){

            ml.sort();
            const Value nodeval = ml.start->callback(pos, ml.start->move, depth, -beta, -alpha);

            if (nodeval > alpha) {
                alpha = nodeval;
                bestmove = ml.start->move;

                if (alpha >= beta) break;
            }
        }

        TT::replace({pos.hash, bestmove, depth});
        return alpha;
    }

    template<Color c>
    static Value qsearch(Position<c>& pos, Value alpha, Value beta){
        ++nodecount;
        Movelist<c> ml;

        // we only enumerate captures and queen promotions (unless we are in check)
        bool check = enumerate<QSearch, Callback>(pos, 0, ml.end); // in case of quick pruning a lot of time is wasted on mg

        if (!check) {
            const Value stand_pat = pos.evaluate();
            if (stand_pat >= beta || stand_pat + 2*QUEEN_MG < alpha) return stand_pat; // quick pruning
            if (stand_pat > alpha) alpha = stand_pat;

            for (; ml.start < ml.end; ++ml.start)
            {
                ml.sort();
                if (ml.start->static_rating < 0) break;
                if (stand_pat + ml.start->static_rating + 200 < alpha) break; // delta pruning
                const Value nodeval = ml.start->callback(pos, ml.start->move, 0, -beta, -alpha);

                if (nodeval > alpha)
                {
                    alpha = nodeval;
                    if (alpha >= beta) break;
                }
            }
        }
        else{
            if (ml.start == ml.end) return -KNOWN_MATE;
            for (; ml.start < ml.end; ++ml.start)
            {
                ml.sort();
                const Value nodeval = ml.start->callback(pos, ml.start->move, 0, -beta, -alpha);

                if (nodeval > alpha)
                {
                    alpha = nodeval;
                    if (alpha >= beta) break;
                }
            }
        }
        return alpha;
    }

    template<SearchType st = NSearch, Color c>
    static inline Value search(Position<c>& pos, Value alpha, Value beta, u8 depth = 0)
    {
        if (st == QSearch || !depth) return qsearch(pos, alpha, beta);
        if constexpr (st == NSearch) return nsearch(pos, depth, alpha, beta);
        return ZERO;
    }

    template <Color c>
    static void display_info(Position<c>& pos, Value evaluation) {
#ifdef USE_INTRIN
        auto duration = std::time(nullptr) - start_time;
        auto duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
        if (!duration_ms) duration_ms = 1;
#endif

        std::stringstream info;
        info    << "info"
                << " depth "        << int(full_depth)
                << " seldepth "     << int(seldepth)
                << " score "        << Misc::uci_eval(evaluation)
                << " nodes "        << nodecount;

#ifdef USE_INTRIN
        info    << " time "         << duration_ms
                << " nps "          << 1000 * (u64(nodecount) / duration_ms);
#endif

        info    << " pv"            << Misc::uci_pv(pos);

        std::cout << info.str() << std::endl << std::flush;
    }

    template<Color c>
    static Value go_iteration(Position<c>& pos, u8 depth, Value alpha, Value beta){
        seldepth = 0;
        full_depth = depth;

        return search(pos, alpha, beta, depth);
    }

    template<Color c>
    static void go(Position<c>& pos, u8 depth){
        nodecount = 0;
        tt_hits = 0;
        start_time = std::time(nullptr);

        int wlow = 25, whigh = 25;
        Value last_val = ZERO;

        for (u8 currdepth = 1; currdepth <= depth; ){
            Value val = go_iteration(pos, currdepth, last_val - wlow, last_val + whigh);

            if      (val <= last_val - wlow)    wlow *= 3;
            else if (val >= last_val + whigh)   whigh *= 3;
            else {
                last_val = val;
                wlow = 25;
                whigh = 25;
                currdepth++;
                display_info(pos, val);
                if (abs(last_val) > 30000) break;
            }
        }

        std::cout << "bestmove " << Misc::uci_move(TT::table[*pos.hash].move) << std::endl;
    }

    template<Color c>
    static void go_single(Position<c>& pos, u8 depth){
        nodecount = 0;
        tt_hits = 0;
        start_time = std::time(nullptr);

        Value val = go_iteration(pos, depth, ABSOLUTE_MIN, ABSOLUTE_MAX);
        display_info(pos, val);
    }
}