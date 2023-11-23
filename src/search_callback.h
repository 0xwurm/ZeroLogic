#pragma once

namespace ZeroLogic {
    using namespace Boardstate;
    using namespace Movegen;

    class Search {

    public:

        static inline u64 nodecount;
        static inline std::string bestmove;
        static inline bool better_root_move;
        static inline bool cutoff;
        static inline u8 full_depth;
        static inline std::chrono::steady_clock::time_point start_time;

        struct _vals{
            Bit from;
            Bit to;
            Bit ep_target;
        };

        struct vars{
            u8 depth;
            eval alpha;
            eval beta;
            Bit cap_square;

            template<bool root>
            FORCEINLINE void test(eval& val){
                if (val > alpha) {
                    alpha = val;
                    if (alpha >= beta)
                        cutoff = true;

                    if constexpr (root) better_root_move = true;
                }
            }

            struct move_list{
                u8 index;
                void (*functs[100])(vars&, const Board&, _vals&);
                _vals vals[100];
                void put_in(void (*funct)(vars&, const Board&, _vals&), _vals val){
                    functs[index] = funct;
                    vals[index] = val;
                    ++index;
                }
            }priority1, priority2, priority3;
        };

#define if_cutoff \
        if (cutoff) {   \
        var.alpha = -var.alpha; \
        cutoff = false; \
        return; \
        }

        static FORCEINLINE void end_routine(vars& var, const Board board){
            for (u8 i = 0; i != var.priority1.index; ++i) {
                var.priority1.functs[i](var, board, var.priority1.vals[i]);
                if_cutoff
            }
            for (u8 i = 0; i != var.priority2.index; ++i) {
                var.priority2.functs[i](var, board, var.priority2.vals[i]);
                if_cutoff
            }
            for (u8 i = 0; i != var.priority3.index; ++i) {
                var.priority3.functs[i](var, board, var.priority3.vals[i]);
                if_cutoff
            }

            var.alpha = -var.alpha;
        }

#undef if_cutoff

        static FORCEINLINE void mate(vars& var){
            var.alpha = eval(MATE_POS + (full_depth - var.depth));
        }
        static FORCEINLINE void draw(vars& var){
            var.alpha = DRAW;
        }

    private:

        static FORCEINLINE void count(const map moves){
            nodecount += BitCount(moves);
        }
        static FORCEINLINE void count_promo(const map moves){
            nodecount += 4*BitCount(moves);
        }
        static FORCEINLINE void increment(){
            ++nodecount;
        }

        template <Piece promotion_to>
        static FORCEINLINE void _bestmove(Bit& from, Bit& to){
            if (better_root_move) {
                bestmove = Misc::uci_move<promotion_to>(SquareOf(from), SquareOf(to));
                better_root_move = false;
            }
        }




        template<State new_state, bool leaf, bool root>
        static FORCEINLINE void _any_move(const Board& new_board, Bit ep_target, vars& var, Bit recapture){
            if constexpr (leaf) {
                eval val = Eval::evaluate<new_state.white_move>(new_board);
                var.test<root>(val);
            }
            else{
                vars new_var = {u8(var.depth - 1), -var.beta, -var.alpha, recapture};
                enumerate<new_state, false, Search>(new_board, new_var, ep_target);
                var.test<root>(new_var.alpha);
            }
        }

        template<State state, bool capture, bool leaf, bool root>
        static void _kingmove(vars& var, const Board& board, _vals& val) {
            const Board new_board = move<KING, state.white_move, capture>(board, val.from, val.to);
            if constexpr (capture)  _any_move<state.no_castles(), leaf, root>(new_board, 0, var, val.to);
            else                    _any_move<state.no_castles(), leaf, root>(new_board, 0, var, 0);

            if constexpr (root) _bestmove<PIECE_INVALID>(val.from, val.to);
        }

        template<State state, CastleType type, bool leaf, bool root>
        static void _castlemove(vars& var, const Board& board, _vals& val){
            const Board new_board = castle_move<type>(board);
            _any_move<state.no_castles(), leaf, root>(new_board, 0, var, 0);

            if constexpr (root){
                if (better_root_move) {
                    bestmove = Misc::uci_move<type>();
                    better_root_move = false;
                }
            }
        }

        template<State state, bool capture, Piece piece, bool leaf, bool root>
        static void _silent_move(vars& var, const Board& board, _vals& val){
            const Board new_board = move<piece, state.white_move, capture>(board, val.from, val.to);
            if constexpr (capture)  _any_move<state.silent_move(), leaf, root>(new_board, 0, var, val.to);
            else                    _any_move<state.silent_move(), leaf, root>(new_board, 0, var, 0);

            if constexpr (root) _bestmove<PIECE_INVALID>(val.from, val.to);
        }

        template<State state, bool capture, bool leaf, bool root>
        static void _rookmove(vars& var, const Board& board, _vals& val){
            constexpr bool us = state.white_move;
            const Board new_board = move<ROOK, us, capture>(board, val.from, val.to);
            
            if constexpr (leaf) {
                eval pos_val = Eval::evaluate<!state.white_move>(new_board);
                var.test<root>(pos_val);
            }
            else {
                vars new_var;
                if constexpr (capture)  new_var = {u8(var.depth - 1), -var.beta, -var.alpha, val.to};
                else                    new_var = {u8(var.depth - 1), -var.beta, -var.alpha, 0};
                [&]() {
                    if constexpr (state.can_oo<us>()) {
                        if (state.is_rook_right<us>(val.from)) {
                            enumerate<state.no_oo<us>(), false, Search>(new_board, new_var, 0);
                            return;
                        }
                    } else if constexpr (state.can_ooo<us>()) {
                        if (state.is_rook_left<us>(val.from)) {
                            enumerate<state.no_ooo<us>(), false, Search>(new_board, new_var, 0);
                            return;
                        }
                    }
                    enumerate<state.silent_move(), false, Search>(new_board, new_var, 0);
                }();
                var.test<root>(new_var.alpha);
            }

            if constexpr (root) _bestmove<PIECE_INVALID>(val.from, val.to);
        }

        template<State state, bool leaf, bool root>
        static void _ep_move(vars& var, const Board& board, _vals& val){
            const Board new_board = Boardstate::ep_move<state.white_move>(board, val.from | val.to, val.ep_target);
            _any_move<state.silent_move(), leaf, root>(new_board, 0, var, val.to);

            if constexpr (root) _bestmove<PIECE_INVALID>(val.from, val.to);
        }

        template<State state, bool leaf, bool root>
        static void _pawn_push(vars& var, const Board& board, _vals& val){
            const Board new_board = move<PAWN, state.white_move, false>(board, val.from, val.to);
            _any_move<state.new_ep_pawn(), leaf, root>(new_board, val.to, var, 0);

            if constexpr (root) _bestmove<PIECE_INVALID>(val.from, val.to);
        }

        template<State state, Piece promotion_to, bool capture, bool leaf, bool root>
        static void _promotion_move(vars& var, const Board& board, _vals& val){
            const Board new_board = promotion_move<promotion_to, state.white_move, capture>(board, val.from, val.to);
            if constexpr (capture)  _any_move<state.silent_move(), leaf, root>(new_board, 0, var, val.to);
            else                    _any_move<state.silent_move(), leaf, root>(new_board, 0, var, 0);

            if constexpr (root) _bestmove<promotion_to>(val.from, val.to);
        }


        static void init(u8 depth){
            bestmove = "invalid move";
            better_root_move = false;
            cutoff = false;
            full_depth = depth;
        }

        static void display_info(u8 depth, eval evaluation) {
            auto duration = std::chrono::steady_clock::now() - start_time;
            auto duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();

            std::stringstream info;
            info << "info"
                 << " depth "   << int(depth)
                 << " nodes "   << nodecount
                 << " time "    << duration_ms
                 << " pv "      << bestmove
                 << Misc::uci_eval(evaluation);

            std::cout << info.str() << std::endl << std::flush;
        }

        template<State state>
        static void start_iteration(const Board& board, vars& var, Bit ep_target){
            init(var.depth);
            enumerate<state, true, Search>(board, var, ep_target);
        }

        constexpr static const eval bounds[4] = {eval(10), eval(100), eval(300), ABSOLUTE_MAX};

        template<State state>
        static void start_id(const Board& board, u8 depth, Bit ep_target){
            eval last_eval{}, lower_bound{10}, upper_bound{10};
            u8 curr_depth = 1;
            nodecount = 0;
            start_time = std::chrono::steady_clock::now();

            int fail_high_count = 0;
            int fail_low_count = 0;
            while (curr_depth != depth + 1){
                // std::cout << "iteration\n";
                vars curr_var = {curr_depth, last_eval - lower_bound, last_eval + upper_bound, 0};
                start_iteration<state>(board, curr_var, ep_target);
                curr_var.alpha *= -1;


                if      (curr_var.alpha >= (last_eval + upper_bound)) {fail_high_count++; upper_bound = bounds[fail_high_count];}
                else if (curr_var.alpha <= (last_eval - lower_bound)) {fail_low_count++; lower_bound = bounds[fail_low_count];}
                else{
                    display_info(curr_depth, curr_var.alpha);
                    last_eval = curr_var.alpha;
                    lower_bound = eval(10);
                    upper_bound = eval(10);
                    curr_depth++;
                    fail_high_count = 0;
                    fail_low_count = 0;
                }
            }
        }

    public:

        static FORCEINLINE void check_priority(void (*_func)(vars&, const Board&, _vals&), _vals _val, vars& var){
            if (var.cap_square & _val.to)   var.priority1.put_in(_func, _val);
            else                            var.priority2.put_in(_func, _val);
        }

        template<State state, bool capture, bool leaf, bool root>
        static FORCEINLINE void promotion_sort(_vals _val, vars& var){
            var.priority1.put_in(_promotion_move<state, QUEEN, capture, leaf, root>, _val);

            var.priority3.put_in(_promotion_move<state, ROOK, capture, leaf, root>, _val);
            var.priority3.put_in(_promotion_move<state, KNIGHT, capture, leaf, root>, _val);
            var.priority3.put_in(_promotion_move<state, BISHOP, capture, leaf, root>, _val);
        }

        template<State state, bool root, bool leaf>
        static FORCEINLINE void kingmove(const Board& board, map& moves, map& captures, vars& var){
            constexpr bool us = state.white_move;
            if constexpr (leaf) count(moves | captures);
            Bit king = King<us>(board);
            Bitloop(moves)
                var.priority3.put_in(_kingmove<state, false, leaf, root>, {king, 1ull << SquareOf(moves), 0});
            Bitloop(captures)
                check_priority(_kingmove<state, true, leaf, root>, {king, 1ull << SquareOf(captures), 0}, var);
        }

        template<State state, bool root, bool leaf>
        static FORCEINLINE void ep_move(const Board& board, Bit& lep, Bit& rep, Bit& ep_target, vars& var){
            constexpr bool us = state.white_move;
            if constexpr (leaf) {
                if (lep) increment();
                if (rep) increment();
            }
            if (lep) var.priority2.put_in(_ep_move<state, leaf, root>, {lep, pawn_atk_left<us>(lep), ep_target});
            if (rep) var.priority2.put_in(_ep_move<state, leaf, root>, {rep, pawn_atk_right<us>(rep), ep_target});
        }

        template<State state, bool root, bool leaf>
        static FORCEINLINE void pawn_move(const Board& board, map& pr, map& pl, map& pr_promo, map& pl_promo, map& pf, map& pp, map& pf_promo, vars& var){
            if constexpr (leaf){
                count(pr);
                count(pl);
                count_promo(pr_promo);
                count_promo(pl_promo);
                count(pf | pp);
                count_promo(pf_promo);
            }
            constexpr bool us = state.white_move;
            Bitloop(pr){
                Square sq = SquareOf(pr);
                check_priority(_silent_move<state, true, PAWN, leaf, root>, {1ull << (sq + pawn_shift[0][us]), 1ull << sq, 0}, var);
            }
            Bitloop(pl){
                Square sq = SquareOf(pl);
                check_priority(_silent_move<state, true, PAWN, leaf, root>, {1ull << (sq + pawn_shift[1][us]), 1ull << sq, 0}, var);
            }
            Bitloop(pr_promo){
                Square sq = SquareOf(pr_promo); Bit from = 1ull << (sq + pawn_shift[0][us]), to = 1ull << sq;
                promotion_sort<state, true, leaf, root>({from, to, 0}, var);
            }
            Bitloop(pl_promo){
                Square sq = SquareOf(pl_promo); Bit from = 1ull << (sq + pawn_shift[1][us]), to = 1ull << sq;
                promotion_sort<state, true, leaf, root>({from, to, 0}, var);
            }
            Bitloop(pf){
                Square sq = SquareOf(pf);
                var.priority3.put_in(_silent_move<state, false, PAWN, leaf, root>, {1ull << (sq + sign<us>(-8)), 1ull << sq, 0});
            }
            Bitloop(pf_promo){
                Square sq = SquareOf(pf_promo); Bit from = 1ull << (sq + sign<us>(-8)), to = 1ull << sq;
                promotion_sort<state, false, leaf, root>({from, to, 0}, var);
            }
            Bitloop(pp){
                Square sq = SquareOf(pp);
                var.priority3.put_in(_pawn_push<state, leaf, root>, {1ull << (sq + sign<us>(-16)), 1ull << sq, 0});
            }
        }

        template<State state, Piece piece, bool root, bool leaf>
        static FORCEINLINE void silent_move(const Board& board, map& moves, map& captures, const Square& sq, vars& var){
            if constexpr (leaf)
                count(moves | captures);
            Bitloop(moves)
                var.priority3.put_in(_silent_move<state, false, piece, leaf, root>, {1ull << sq, 1ull << SquareOf(moves), 0});
            Bitloop(captures)
                check_priority(_silent_move<state, true, piece, leaf, root>, {1ull << sq, 1ull << SquareOf(captures), 0}, var);
        }

        template<State state, bool root, bool leaf>
        static FORCEINLINE void rookmove(const Board& board, map& moves, map& captures, const Square& sq, vars& var){
            if constexpr (leaf)
                count(moves | captures);
            Bitloop(moves)
                var.priority3.put_in(_rookmove<state, false, leaf, root>, {1ull << sq, 1ull << SquareOf(moves), 0});
            Bitloop(captures)
                check_priority(_rookmove<state, true, leaf, root>, {1ull << sq, 1ull << SquareOf(captures), 0}, var);
        }

        template<State state, bool left, bool root, bool leaf>
        static FORCEINLINE void castlemove(const Board& board, vars& var){
            if constexpr (leaf) increment();
            if constexpr (left) {
                if constexpr (state.white_move) var.priority3.put_in(_castlemove<state, WHITE_OOO, leaf, root>, {});
                else                            var.priority3.put_in(_castlemove<state, BLACK_OOO, leaf, root>, {});
            }
            else {
                if constexpr (state.white_move) var.priority3.put_in(_castlemove<state, WHITE_OO, leaf, root>, {});
                else                            var.priority3.put_in(_castlemove<state, BLACK_OO, leaf, root>, {});
            }
        }

        template<State state>
        static void go(const Board& board, u8 depth, Bit ep_target){
            start_id<state>(board, depth, ep_target);
            std::cout << "bestmove " << bestmove << std::endl;
        }
        template<State state>
        static void go_single(const Board& board, u8 depth, Bit ep_target){
            nodecount = 0;
            start_time = std::chrono::steady_clock::now();
            vars var = {depth, ABSOLUTE_MIN, ABSOLUTE_MAX, 0};
            start_iteration<state>(board, var, ep_target);
            display_info(depth, -var.alpha);
            std::cout << "bestmove " << bestmove << std::endl;
        }
    };
}