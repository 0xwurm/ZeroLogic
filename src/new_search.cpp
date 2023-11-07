#pragma clang diagnostic push
#pragma ide diagnostic ignored "misc-no-recursion"

/*
namespace ZeroLogic::Time {

    static std::chrono::time_point<std::chrono::steady_clock> stop_at;
    static std::chrono::time_point<std::chrono::steady_clock> started_at;

    static void start_time(int msec) {
        stop_at = std::chrono::steady_clock::now() + std::chrono::milliseconds(msec);
        started_at = std::chrono::steady_clock::now();
    }

    bool timed_out() {
        if (std::chrono::steady_clock::now() >= stop_at) {
            return true;
        }
        else {
            return false;
        }
    }

    int getMsec() {
        return (std::chrono::steady_clock::now() - started_at).count() * 10e-7;
    }

}

namespace ZeroLogic::Search{

    static unsigned long long overall_nodes, partial_nodes;
    static int full_depth, max_depth;

    struct pv{
        Move list[50]{};
        unsigned short index = 0;
        void put_in(Move node) {list[index] = node; index++;}
        Move get_out() {index--; return list[index];}
    };

    template <bool white>
    void perft(Gamestate gs, Move m, int depth){
        if (!depth) { partial_nodes++; overall_nodes++; return; }

        MoveList ml;
        gs.makemove<!white>(m);
        MoveGenerator<white> mg(&gs, &ml);
        mg.go();

        if (!(ml.list[0] == checkmate || ml.list[0] == stalemate)) {
            for (int i = 0; i < ml.index_front; i++)
                perft<!white>(gs, ml.list[i], depth - 1);
            for (int i = ml.index_back + 1; i < 218; i++)
                perft<!white>(gs, ml.list[i], depth - 1);
        }
    }
    template void perft<true>(Gamestate gs, Move m, int depth);
    template void perft<false>(Gamestate gs, Move m, int depth);

    template <bool white>
    void start_perft(Gamestate gs, int depth){
        overall_nodes = 0; partial_nodes = 0;
        MoveList ml;
        MoveGenerator<white> mg(&gs, &ml);
        mg.go();

        if (ml.list[0] == checkmate) std::cout << "Perft aborted due to checkmate in root position.\n";
        else if (ml.list[0] == stalemate) std::cout << "Perft aborted due to stalemate in root position.\n";
        else{
            for (int i = 0; i < ml.index_front; i++){
                perft<!white>(gs, ml.list[i], depth - 1);
                std::cout << Misc::numToString(ml.list[i]) << ": " << partial_nodes << std::endl;
                partial_nodes = 0;
            }
            for (int i = ml.index_back + 1; i < 218; i++){
                perft<!white>(gs, ml.list[i], depth - 1);
                std::cout << Misc::numToString(ml.list[i]) << ": " << partial_nodes << std::endl;
                partial_nodes = 0;
            }
            std::cout << "\nOverall terminal nodes searched: " << overall_nodes << std::endl;
        }
    }
    template void start_perft<true>(Gamestate gs, int depth);
    template void start_perft<false>(Gamestate gs, int depth);

#define move_macro gs.makemove<!white>(m); if (!d) {overall_nodes++; Eval e(&gs); return ((white) ? 1 : -1) * e.get();} MoveList ml; MoveGenerator<white> mg(&gs, &ml); mg.go(); if (ml.list[0] == checkmate) return mate_in<white>(actual_depth); else if (ml.list[0] == stalemate) return draw;
#define minmax(move) if (v >= b) { return b; } if (v > a) { a = v; higher_pv.put_in(move); *lower_pv = higher_pv; }

    template<bool white>
    constexpr eval mate_in(int depth){
        eval val = mate - depth;
        if (white) return -val;
        else       return val;
    }
    constexpr int mate_in(eval val){
        return ((val > 0) ? 1 : -1) * (mate - abs(val));
    }

    template <bool white>
    eval see(Gamestate gs, Move m, pv* lower_pv, eval a, eval b, int actual_depth, int exchange_index){
        if (actual_depth > max_depth) max_depth = actual_depth;
        gs.makemove<!white>(m);
        MoveList ml; MoveGenerator<white> mg(&gs, &ml); mg.go();
        if (ml.list[0] == checkmate) return mate_in<white>(actual_depth);
        else if (ml.list[0] == stalemate) return draw;
        for (int i = 0; i < ml.index_front; ++i){
            if (Time::timed_out()) return 0;
            pv higher_pv{};
            eval v;
            if ((ml.list[i] & 0x3f0) >> 4 == exchange_index) v = -see<!white>(gs, ml.list[i], &higher_pv, -b, -a, actual_depth + 1, exchange_index);
            else v = -best_move<!white>(gs, ml.list[i], &higher_pv, -b, -a, 0, actual_depth + 1);
            minmax(ml.list[i])
        }
        for (int i = ml.index_back + 1; i < 218; ++i){
            if (Time::timed_out()) return 0;
            pv higher_pv{};
            eval v = -best_move<!white>(gs, ml.list[i], &higher_pv, -b, -a, 0, actual_depth + 1);
            minmax(ml.list[i])
        }
        return a;
    }

    template <bool white>
    eval best_move(Gamestate gs, Move m, pv* lower_pv, eval a, eval b, int d, int actual_depth){
        if (actual_depth > max_depth) max_depth = actual_depth;
        move_macro
        for (int i = 0; i < ml.index_front; ++i){
            if (Time::timed_out()) return 0;
            pv higher_pv{};
            eval v;
            if (d == 1) v = -see<!white>(gs, ml.list[i], &higher_pv, -b, -a, actual_depth + 1, (ml.list[i] & 0x3f0) >> 4);
            else v = -best_move<!white>(gs, ml.list[i], &higher_pv, -b, -a, d - 1, actual_depth + 1);
            minmax(ml.list[i])
        }
        for (int i = ml.index_back + 1; i < 218; ++i){
            if (Time::timed_out()) return 0;
            pv higher_pv{};
            eval v = -best_move<!white>(gs, ml.list[i], &higher_pv, -b, -a, d - 1, actual_depth + 1);
            minmax(ml.list[i])
        }
        return a;
    }

    template <bool white>
    eval pv_search(Gamestate gs, Move m, pv* lower_pv, pv* old_pv, eval a, eval b, int d, int actual_depth){
        gs.makemove<!white>(m); MoveList ml; MoveGenerator<white> mg(&gs, &ml); mg.go();
        if (ml.list[0] == checkmate) return mate_in<white>(actual_depth);
        else if (ml.list[0] == stalemate) return draw;
        Move pv_move = old_pv->get_out();
        {
            pv higher_pv{};
            eval v;
            if (old_pv->index) v = -pv_search <!white>(gs, pv_move, &higher_pv, old_pv, -b, -a, (d > 0) ? d - 1 : 1, actual_depth + 1);
            else               v = -best_move<!white>(gs, pv_move, &higher_pv, -b, -a, 1, actual_depth + 1);
            minmax(pv_move)
        }
        for (int i = 0; i < ml.index_front; ++i){
            if (Time::timed_out()) return 0;
            if (ml.list[i] == pv_move) continue;
            pv higher_pv{};
            eval v = -best_move<!white>(gs, ml.list[i], &higher_pv, -b, -a, (d > 0) ? d : 1, actual_depth + 1);
            minmax(ml.list[i])
        }
        for (int i = ml.index_back + 1; i < 218; ++i){
            if (Time::timed_out()) return 0;
            if (ml.list[i] == pv_move) continue;
            pv higher_pv{};
            eval v = -best_move<!white>(gs, ml.list[i], &higher_pv, -b, -a, (d > 0) ? d - 1 : 0, actual_depth + 1);
            minmax(ml.list[i])
        }
        return a;
    }

    template <bool white>
    void start_iterative_deepening(Gamestate gs, int msec){
        overall_nodes = 0;
        Time::start_time(msec);
        pv prev_pv{}; pv* lower_pv = new pv;
        Move best_move_var;
        eval low_window = 40, high_window = 40, current_eval, last_eval = 0;
        int next_depth = 1;

        while (true){
            full_depth = next_depth;
            current_eval = [&]() -> eval {
                pv curr_pv = prev_pv;
                eval a = last_eval - low_window, b = last_eval + high_window;
                MoveList ml;
                MoveGenerator<white> mg(&gs, &ml);
                mg.go();
                if (ml.list[0] == checkmate) return mate_in<white>(0);
                else if (ml.list[0] == stalemate) return draw;
                Move pv_move = 0xffff;
                if (curr_pv.index) {
                    pv_move = curr_pv.get_out();
                    pv higher_pv{};
                    eval v;
                    if (curr_pv.index) v = -pv_search<!white>(gs, pv_move, &higher_pv, &curr_pv, -b, -a,(next_depth) ? next_depth - 1 : 1, 1);
                    else v = -best_move<!white>(gs, pv_move, &higher_pv, -b, -a, (next_depth) ? next_depth - 1 : 1, 1);
                    minmax(pv_move)
                }
                for (int i = 0; i < ml.index_front; ++i) {
                    if (Time::timed_out()) return 0;
                    if (ml.list[i] == pv_move) continue;
                    pv higher_pv{};
                    eval v = -best_move<!white>(gs, ml.list[i], &higher_pv, -b, -a, next_depth, 1);
                    minmax(ml.list[i])
                }
                for (int i = ml.index_back + 1; i < 218; ++i) {
                    if (Time::timed_out()) return 0;
                    if (ml.list[i] == pv_move) continue;
                    pv higher_pv{};
                    eval v = -best_move<!white>(gs, ml.list[i], &higher_pv, -b, -a, next_depth - 1, 1);
                    minmax(ml.list[i])
                }
                return a;
            }();
            if (Time::timed_out()) break;
            max_depth = (max_depth) ? max_depth : next_depth;
            std::stringstream ss;
            ss << "info depth " << next_depth << " nodes " << overall_nodes << " seldepth " << max_depth << " time " << Time::getMsec();
            if (abs(current_eval) > 30000) ss << " score mate " << mate_in(current_eval);
            else                              ss << " score cp " << current_eval;
            if (lower_pv && lower_pv->index){
                ss << " pv";
                for (int i = lower_pv->index - 1; i > -1; i--) ss << " " << Misc::numToString(lower_pv->list[i]);
            }
            ss << std::endl;
            std::cout << ss.str() << std::flush;
            max_depth = 0;

            if      ((last_eval + high_window) <= current_eval) high_window *= 3;
            else if ((last_eval - low_window) >= current_eval) low_window *= 3;
            else if (abs(current_eval) > 30000){best_move_var = lower_pv->list[lower_pv->index - 1];}
            else{
                low_window = 40; high_window = 40;
                next_depth++;
                best_move_var = lower_pv->list[lower_pv->index - 1];
                prev_pv = *lower_pv;
                last_eval = current_eval;
            }
        }
        std::cout << "info nodes " << overall_nodes << " time " << Time::getMsec() << std::endl;
        std::cout << "bestmove " << Misc::numToString(best_move_var) << std::endl << std::flush;
    }
    template void start_iterative_deepening<true>(Gamestate gs, int msec);
    template void start_iterative_deepening<false>(Gamestate gs, int msec);
}

#pragma clang diagnostic pop*/