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


        struct _vals{
            Bit from;
            Bit to;
            Bit ep_target;
        };

        struct vars{
            u8 depth;
            eval alpha;
            eval beta;

            template<bool root>
            FORCEINLINE void test(eval& val){
                if (val > alpha) {
                    alpha = val;
                    if (val >= beta)
                        cutoff = true;

                    if constexpr (root) better_root_move = true;
                }
            }

            u8 num_moves1;
            void (*priority1[100])(vars&, const Board&, _vals&); // cheap recapture
            _vals pr1_vals[100];
            u8 num_moves2;
            void (*priority2[100])(vars&, const Board&, _vals&); // capture
            _vals pr2_vals[100];
            u8 num_moves3;
            void (*priority3[100])(vars&, const Board&, _vals&); // other
            _vals pr3_vals[100];
        };

#define if_cutoff \
        if (cutoff) {   \
        var.alpha = -var.alpha; \
        cutoff = false; \
        return; \
        }

        static FORCEINLINE void end_routine(vars& var, const Board board){
            for (u8 i = 0; i != var.num_moves1; ++i) {
                var.priority1[i](var, board, var.pr1_vals[i]);
                if_cutoff
            }
            for (u8 i = 0; i != var.num_moves2; ++i) {
                var.priority2[i](var, board, var.pr2_vals[i]);
                if_cutoff
            }
            for (u8 i = 0; i != var.num_moves3; ++i) {
                var.priority3[i](var, board, var.pr3_vals[i]);
                if_cutoff
            }

            var.alpha = -var.alpha;
        }

#undef if_cutoff

        template<bool white>
        static FORCEINLINE void mate(vars& var){
            if constexpr (white)    var.alpha = eval(MATE_NEG - (full_depth - var.depth));
            else                    var.alpha = eval(MATE_POS + (full_depth - var.depth));
        }
        static FORCEINLINE void draw(vars& var){
            var.alpha = DRAW;
        }

    private:

        static void init(u8 depth){
            nodecount = 0;
            start_time = std::chrono::steady_clock::now();
            bestmove = "invalid move";
            better_root_move = false;
            cutoff = false;
            full_depth = depth;
        }

        static FORCEINLINE void count(const map moves){
            nodecount += BitCount(moves);
        }
        static FORCEINLINE void count_promo(const map moves){
            nodecount += 4*BitCount(moves);
        }
        static FORCEINLINE void increment(){
            ++nodecount;
        }

        static void display_info(u8 depth, eval evaluation) {
            auto duration = std::chrono::steady_clock::now() - start_time;
            auto duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();

            std::stringstream info;
            info << "info"
                 << " depth "   << int(depth)
                 << " nodes "   << nodecount
                 << " time "    << duration_ms
                 << Misc::uci_eval(eval(-evaluation));

            std::cout << info.str()                << std::endl << std::flush;
            std::cout << "bestmove " << bestmove   << std::endl << std::flush;
        }

        template <Piece promotion_to>
        static FORCEINLINE void _bestmove(Bit& from, Bit& to){
            if (better_root_move) {
                bestmove = Misc::uci_move<promotion_to>(SquareOf(from), SquareOf(to));
                better_root_move = false;
            }
        }

        template<State new_state, bool leaf, bool root>
        static FORCEINLINE void _any_move(const Board& new_board, Bit ep_target, vars& var){
            if constexpr (leaf) {
                eval val = Eval::evaluate<new_state.white_move>(new_board);
                var.test<root>(val);
            }
            else{
                vars new_var = {u8(var.depth - 1), -var.beta, -var.alpha};
                enumerate<new_state, false, Search>(new_board, new_var, ep_target);
                var.test<root>(new_var.alpha);
            }
        }

        template<State state, bool capture, bool leaf, bool root>
        static void _kingmove(vars& var, const Board& board, _vals& val) {
            const Board new_board = move<KING, state.white_move, capture>(board, val.from, val.to);
            _any_move<state.no_castles(), leaf, root>(new_board, 0, var);

            if constexpr (root) _bestmove<PIECE_INVALID>(val.from, val.to);
        }

        template<State state, CastleType type, bool leaf, bool root>
        static void _castlemove(vars& var, const Board& board, _vals& val){
            const Board new_board = castle_move<type>(board);
            _any_move<state.no_castles(), leaf, root>(new_board, 0, var);

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
            _any_move<state.silent_move(), leaf, root>(new_board, 0, var);

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
                vars new_var = {u8(var.depth - 1), -var.beta, -var.alpha};
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
            _any_move<state.silent_move(), leaf, root>(new_board, 0, var);

            if constexpr (root) _bestmove<PIECE_INVALID>(val.from, val.to);
        }

        template<State state, bool leaf, bool root>
        static void _pawn_push(vars& var, const Board& board, _vals& val){
            const Board new_board = move<PAWN, state.white_move, false>(board, val.from, val.to);
            _any_move<state.new_ep_pawn(), leaf, root>(new_board, val.to, var);

            if constexpr (root) _bestmove<PIECE_INVALID>(val.from, val.to);
        }

        template<State state, Piece promotion_to, bool capture, bool leaf, bool root>
        static void _promotion_move(vars& var, const Board& board, _vals& val){
            const Board new_board = promotion_move<promotion_to, state.white_move, capture>(board, val.from, val.to);
            _any_move<state.silent_move(), leaf, root>(new_board, 0, var);

            if constexpr (root) _bestmove<promotion_to>(val.from, val.to);
        }

    public:

        template<State state>
        static void go(const Board& board, u8 depth, Bit ep_target){
            init(depth);
            vars var = {depth, ABSOLUTE_MIN, ABSOLUTE_MAX};
            enumerate<state, true, Search>(board, var, ep_target);
            display_info(depth, var.alpha);
        }

        template<State state, bool root, bool leaf>
        static FORCEINLINE void kingmove(const Board& board, map& moves, map& captures, vars& var){
            constexpr bool us = state.white_move;
            if constexpr (leaf) count(moves | captures);
            Bit king = King<us>(board);
            Bitloop(moves) {
                var.priority3[var.num_moves3] = _kingmove<state, false, leaf, root>;
                var.pr3_vals[var.num_moves3] = {king, 1ull << SquareOf(moves), 0};
                ++var.num_moves3;
            }
            Bitloop(captures) {
                var.priority2[var.num_moves2] = _kingmove<state, true, leaf, root>;
                var.pr2_vals[var.num_moves2] = {king, 1ull << SquareOf(captures), 0};
                ++var.num_moves2;
            }
        }

        template<State state, bool root, bool leaf>
        static FORCEINLINE void ep_move(const Board& board, Bit& lep, Bit& rep, Bit& ep_target, vars& var){
            constexpr bool us = state.white_move;
            if constexpr (leaf) {
                if (lep) increment();
                if (rep) increment();
            }
            if (lep) {
                var.priority2[var.num_moves2] =  _ep_move<state, leaf, root>;
                var.pr2_vals[var.num_moves2] = {lep, pawn_atk_left<us>(lep), ep_target};
                ++var.num_moves2;
            }
            if (rep) {
                var.priority2[var.num_moves2] =  _ep_move<state, leaf, root>;
                var.pr2_vals[var.num_moves2] = {rep, pawn_atk_right<us>(rep), ep_target};
                ++var.num_moves2;
            }
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
                var.priority2[var.num_moves2] =  _silent_move<state, true, PAWN, leaf, root>;
                var.pr2_vals[var.num_moves2] = {1ull << (sq + pawn_shift[0][us]), 1ull << sq, 0};
                ++var.num_moves2;
            }
            Bitloop(pl){
                Square sq = SquareOf(pl);
                var.priority2[var.num_moves2] =  _silent_move<state, true, PAWN, leaf, root>;
                var.pr2_vals[var.num_moves2] = {1ull << (sq + pawn_shift[1][us]), 1ull << sq, 0};
                ++var.num_moves2;
            }
            Bitloop(pr_promo){
                Square sq = SquareOf(pr_promo); Bit from = 1ull << (sq + pawn_shift[0][us]), to = 1ull << sq;
                var.priority1[var.num_moves1] = _promotion_move<state, QUEEN, true, leaf, root>;
                var.pr1_vals[var.num_moves1] = {from, to, 0};
                ++var.num_moves1;

                var.priority3[var.num_moves3] =  _promotion_move<state, ROOK, true, leaf, root>;
                var.pr3_vals[var.num_moves3] = {from, to, 0};
                ++var.num_moves3;
                var.priority3[var.num_moves3] =  _promotion_move<state, BISHOP, true, leaf, root>;
                var.pr3_vals[var.num_moves3] = {from, to, 0};
                ++var.num_moves3;
                var.priority3[var.num_moves3] =  _promotion_move<state, KNIGHT, true, leaf, root>;
                var.pr3_vals[var.num_moves3] = {from, to, 0};
                ++var.num_moves3;
            }
            Bitloop(pl_promo){
                Square sq = SquareOf(pl_promo); Bit from = 1ull << (sq + pawn_shift[1][us]), to = 1ull << sq;
                var.priority1[var.num_moves1] = _promotion_move<state, QUEEN, true, leaf, root>;
                var.pr1_vals[var.num_moves1] = {from, to, 0};
                ++var.num_moves1;

                var.priority3[var.num_moves3] =  _promotion_move<state, ROOK, true, leaf, root>;
                var.pr3_vals[var.num_moves3] = {from, to, 0};
                ++var.num_moves3;
                var.priority3[var.num_moves3] =  _promotion_move<state, BISHOP, true, leaf, root>;
                var.pr3_vals[var.num_moves3] = {from, to, 0};
                ++var.num_moves3;
                var.priority3[var.num_moves3] =  _promotion_move<state, KNIGHT, true, leaf, root>;
                var.pr3_vals[var.num_moves3] = {from, to, 0};
                ++var.num_moves3;
            }
            Bitloop(pf){
                Square sq = SquareOf(pf);
                var.priority3[var.num_moves3] =  _silent_move<state, false, PAWN, leaf, root>;
                var.pr3_vals[var.num_moves3] = {1ull << (sq + sign<us>(-8)), 1ull << sq, 0};
                ++var.num_moves3;
            }
            Bitloop(pf_promo){
                Square sq = SquareOf(pf_promo); Bit from = 1ull << (sq + sign<us>(-8)), to = 1ull << sq;
                var.priority1[var.num_moves1] = _promotion_move<state, QUEEN, false, leaf, root>;
                var.pr1_vals[var.num_moves1] = {from, to, 0};
                ++var.num_moves1;

                var.priority3[var.num_moves3] =  _promotion_move<state, ROOK, false, leaf, root>;
                var.pr3_vals[var.num_moves3] = {from, to, 0};
                ++var.num_moves3;
                var.priority3[var.num_moves3] =  _promotion_move<state, BISHOP, false, leaf, root>;
                var.pr3_vals[var.num_moves3] = {from, to, 0};
                ++var.num_moves3;
                var.priority3[var.num_moves3] =  _promotion_move<state, KNIGHT, false, leaf, root>;
                var.pr3_vals[var.num_moves3] = {from, to, 0};
                ++var.num_moves3;
            }
            Bitloop(pp){
                Square sq = SquareOf(pp);
                var.priority3[var.num_moves3] =  _pawn_push<state, leaf, root>;
                var.pr3_vals[var.num_moves3] = {1ull << (sq + sign<us>(-16)), 1ull << sq, 0};
                ++var.num_moves3;
            }
        }

        template<State state, Piece piece, bool root, bool leaf>
        static FORCEINLINE void silent_move(const Board& board, map& moves, map& captures, const Square& sq, vars& var){
            if constexpr (leaf)
                count(moves | captures);
            Bitloop(moves) {
                var.priority3[var.num_moves3] = _silent_move<state, false, piece, leaf, root>;
                var.pr3_vals[var.num_moves3] = {1ull << sq, 1ull << SquareOf(moves), 0};
                ++var.num_moves3;
            }
            Bitloop(captures) {
                var.priority2[var.num_moves2] = _silent_move<state, true, piece, leaf, root>;
                var.pr2_vals[var.num_moves2] = {1ull << sq, 1ull << SquareOf(captures), 0};
                ++var.num_moves2;
            }
        }
        template<State state, bool root, bool leaf>
        static FORCEINLINE void rookmove(const Board& board, map& moves, map& captures, const Square& sq, vars& var){
            if constexpr (leaf)
                count(moves | captures);
            Bitloop(moves) {
                var.priority3[var.num_moves3] = _rookmove<state, false, leaf, root>;
                var.pr3_vals[var.num_moves3] = {1ull << sq, 1ull << SquareOf(moves), 0};
                ++var.num_moves3;
            }
            Bitloop(captures) {
                var.priority2[var.num_moves2] = _rookmove<state, true, leaf, root>;
                var.pr2_vals[var.num_moves2] = {1ull << sq, 1ull << SquareOf(captures), 0};
                ++var.num_moves2;
            }
        }

        template<State state, bool left, bool root, bool leaf>
        static FORCEINLINE void castlemove(const Board& board, vars& var){
            if constexpr (leaf) increment();
            if constexpr (left) {
                if constexpr (state.white_move) var.priority3[var.num_moves3] = _castlemove<state, WHITE_OOO, leaf, root>;
                else                            var.priority3[var.num_moves3] = _castlemove<state, BLACK_OOO, leaf, root>;
            }
            else {
                if constexpr (state.white_move) var.priority3[var.num_moves3] = _castlemove<state, WHITE_OO, leaf, root>;
                else                            var.priority3[var.num_moves3] = _castlemove<state, BLACK_OO, leaf, root>;
            }
            ++var.num_moves3;
        }
    };
}