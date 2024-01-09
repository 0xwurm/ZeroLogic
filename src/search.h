#pragma once

namespace ZeroLogic::Search {

    using namespace Boardstate;
    using namespace Movegen;

    static inline u32 nodecount, tt_hits;
    static inline u8 seldepth;
    static inline u8 full_depth;

#ifdef USE_INTRIN
    static inline std::chrono::steady_clock::time_point start_time;
#endif

    struct Movelist{
    private:
        RatedMove list[200];

    public:
        RatedMove* start = list;
        RatedMove* end = list;

        FORCEINLINE void sort() const{
            RatedMove* best = start;
            for (RatedMove* head = start + 1; head < end; ++head){
                if (head->static_rating > best->static_rating)
                    best = head;
            }
            RatedMove buffer = *start;
            *start = *best;
            *best = buffer;
        }
    };

    template<State state>
    static Value qsearch(const TempPos& board, Bit ep_target, Value alpha, Value beta);

    template <State state>
    static Value search(const TempPos& board, Bit ep_target, const u8 depth, Value alpha, Value beta){
        if (!depth)
            return qsearch<state>(board, ep_target, alpha, beta);
        ++nodecount;

        Movelist ml;
        bool check = enumerate<state, NSearch, Callback>(board, ep_target, depth, ml.end);

        if (ml.start == ml.end) {
            if (check) return MATE_NEG - depth;
            return DRAW;
        }

        Move bestmove{};
        for ( ; ml.start < ml.end; ++ml.start){

            ml.sort();
            const Value nodeval = ml.start->Mcallback(board, ep_target, ml.start->move, depth, -beta, -alpha);

            if (nodeval > alpha) {
                alpha = nodeval;
                bestmove = ml.start->move;

                if (alpha >= beta) break;
            }
        }

        TT::replace({board.getHash(), bestmove, depth});
        return alpha;
    }

    template<State state>
    static Value qsearch(const TempPos& board, Bit ep_target, Value alpha, Value beta){
        ++nodecount;
        Movelist ml;
        // we only enumerate captures and queen promotions (unless we are in check)
        bool check = enumerate<state, QSearch, Callback>(board, ep_target, 0, ml.end); // in case of quick pruning a lot of time is wasted on mg

        if (!check) {
            const Value stand_pat = Evaluation::evaluate<state.active_color>(board);
            if (stand_pat >= beta || stand_pat + 2*QUEEN_MG < alpha) return stand_pat; // quick pruning
            if (stand_pat > alpha) alpha = stand_pat;

            for (; ml.start < ml.end; ++ml.start) {
                ml.sort();
                if (ml.start->static_rating < 0) break;
                if (stand_pat + ml.start->static_rating + 200 < alpha) break; // delta pruning
                const Value nodeval = ml.start->Mcallback(board, ep_target, ml.start->move, 0, -beta, -alpha);

                if (nodeval > alpha) {
                    alpha = nodeval;
                    if (alpha >= beta) break;
                }
            }
        }
        else{
            if (ml.start == ml.end) return -KNOWN_MATE;
            for (; ml.start < ml.end; ++ml.start) {
                ml.sort();
                const Value nodeval = ml.start->Mcallback(board, ep_target, ml.start->move, 0, -beta, -alpha);
                if (nodeval > alpha) {
                    alpha = nodeval;
                    if (alpha >= beta) break;
                }
            }
        }
        return alpha;
    }

    static void display_info(const TempPos& board, State state, const Bit ep_target, Value evaluation) {
#ifdef USE_INTRIN
        auto duration = std::chrono::steady_clock::now() - start_time;
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

        info    << " pv"            << Misc::uci_pv(board, state, ep_target, full_depth);

        std::cout << info.str() << std::endl << std::flush;
    }

    template<State state>
    static Value go_iteration(const TempPos& board, const Bit& ep_target, u8 depth, Value alpha, Value beta){
        seldepth = 0;
        full_depth = depth;

        return search<state>(board, ep_target, depth, alpha, beta);
    }

    template<State state>
    static void go(const TempPos& board, const Bit& ep_target, u8 depth){
        nodecount = 0;
        tt_hits = 0;
#ifdef USE_INTRIN
        start_time = std::chrono::steady_clock::now();
#endif
        int wlow = 25, whigh = 25;
        Value last_val = ZERO;

        for (u8 currdepth = 1; currdepth <= depth; ){
            Value val = go_iteration<state>(board, ep_target, currdepth, last_val - wlow, last_val + whigh);

            if      (val <= last_val - wlow)    wlow *= 3;
            else if (val >= last_val + whigh)   whigh *= 3;
            else {
                last_val = val;
                wlow = 25;
                whigh = 25;
                currdepth++;
                display_info(board, state, ep_target, val);
                if (abs(last_val) > 30000) break;
            }
        }

        std::cout << "bestmove " << Misc::uci_move(TT::table[board.getHash() & TT::key_mask].move) << std::endl;
    }

    template<State state>
    static void go_single(const TempPos& board, const Bit& ep_target, u8 depth){
        nodecount = 0;
        tt_hits = 0;
#ifdef USE_INTRIN
        start_time = std::chrono::steady_clock::now();
#endif

        Value val = go_iteration<state>(board, ep_target, depth, ABSOLUTE_MIN, ABSOLUTE_MAX);
        display_info(board, state, ep_target, val);
    }
}