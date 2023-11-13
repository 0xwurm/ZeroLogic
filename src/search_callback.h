#pragma once

namespace ZeroLogic {
    using namespace Boardstate;
    using namespace Movegen;

    static u64 nodecount;
    static std::string bestmove;
    static bool better_root_move;
    static bool cutoff;

    class Search {

    public:

        static bool prune(){
            return cutoff;
        }
        static void reset_prune(){
            cutoff = false;
        }

        struct vars{
            u8 depth;
            eval alpha;
            eval beta;

            template<bool root>
            FORCEINLINE void test(eval& val){
                if (val > alpha) {
                    alpha = static_cast<eval>(-val);
                    if (alpha >= beta)
                        cutoff = true;

                    if constexpr (root) better_root_move = true;
                }
            }
        };

    private:

        static void init(){
            nodecount = 0;
            start_time = std::chrono::steady_clock::now();
            bestmove = "invalid move";
            better_root_move = false;
            cutoff = false;
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

        static FORCEINLINE void display_info(u8 depth, eval evaluation) {
            auto duration = std::chrono::steady_clock::now() - start_time;
            auto duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(duration);

            std::stringstream info;
            info << "info"
                 << " depth " << int(depth)
                 << " nodes " << nodecount
                 << " time " << duration_ms
                 << " score cp " << evaluation
                 << " bestmove " << bestmove;

            std::cout << info.str() << std::endl << std::flush;
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
                eval val = Eval::evaluate(new_board);
                var.test<root>(val);
            }
            else{
                vars new_var = {u8(var.depth - 1), eval(-var.beta), eval(-var.alpha)};
                enumerate<new_state, false, Search>(new_board, new_var, ep_target);
                var.test<root>(new_var.alpha);
            }
        }

        template<State state, bool capture, bool leaf, bool root>
        static FORCEINLINE void _kingmove(const Board& board, Bit& from, Bit to, vars& var) {
            const Board new_board = move<KING, state.white_move, capture>(board, from, to);
            _any_move<state.no_castles(), leaf, root>(new_board, 0, var);

            if constexpr (root) _bestmove<PIECE_INVALID>(from, to);
        }

        template<State state, CastleType type, bool leaf, bool root>
        static FORCEINLINE void _castlemove(const Board& board, vars& var){
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
        static FORCEINLINE void _silent_move(const Board& board, Bit from, Bit to, vars& var){
            const Board new_board = move<piece, state.white_move, capture>(board, from, to);
            _any_move<state.silent_move(), leaf, root>(new_board, 0, var);

            if constexpr (root) _bestmove<PIECE_INVALID>(from, to);
        }

        template<State state, bool capture, bool leaf, bool root>
        static FORCEINLINE void _rookmove(const Board& board, Bit from, Bit to, vars& var){
            constexpr bool us = state.white_move;
            const Board new_board = move<ROOK, us, capture>(board, from, to);
            
            if constexpr (leaf) {
                eval val = Eval::evaluate(new_board);
                var.test<root>(val);
            }
            else {
                vars new_var = {u8(var.depth - 1), eval(-var.beta), eval(-var.alpha)};
                [&]() {
                    if constexpr (state.can_oo<us>()) {
                        if (state.is_rook_right<us>(from)) {
                            enumerate<state.no_oo<us>(), false, Search>(new_board, new_var, 0);
                            return;
                        }
                    } else if constexpr (state.can_ooo<us>()) {
                        if (state.is_rook_left<us>(from)) {
                            enumerate<state.no_ooo<us>(), false, Search>(new_board, new_var, 0);
                            return;
                        }
                    }
                    enumerate<state.silent_move(), false, Search>(new_board, new_var, 0);
                }();
                var.test<root>(new_var.alpha);
            }

            if constexpr (root) _bestmove<PIECE_INVALID>(from, to);
        }

        template<State state, bool leaf, bool root>
        static FORCEINLINE void _ep_move(const Board& board, Bit& from, Bit to, Bit& ep_target, vars& var){
            const Board new_board = Boardstate::ep_move<state.white_move>(board, from | to, ep_target);
            _any_move<state.silent_move(), leaf, root>(new_board, 0, var);

            if constexpr (root) _bestmove<PIECE_INVALID>(from, to);
        }

        template<State state, bool leaf, bool root>
        static FORCEINLINE void _pawn_push(const Board& board, Bit from, Bit to, vars& var){
            const Board new_board = move<PAWN, state.white_move, false>(board, from, to);
            _any_move<state.new_ep_pawn(), leaf, root>(new_board, to, var);

            if constexpr (root) _bestmove<PIECE_INVALID>(from, to);
        }

        template<State state, Piece promotion_to, bool capture, bool leaf, bool root>
        static FORCEINLINE void _promotion_move(const Board& board, Bit& from, Bit& to, vars& var){
            const Board new_board = promotion_move<promotion_to, state.white_move, capture>(board, from, to);
            _any_move<state.silent_move(), leaf, root>(new_board, 0, var);

            if constexpr (root) _bestmove<promotion_to>(from, to);
        }

    public:

        template<State state>
        static void go(const Board& board, u8 depth, Bit ep_target){
            init();
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
                _kingmove<state, false, leaf, root>(board, king, 1ull << SquareOf(moves), var);
                if (prune()) return;
            }
            Bitloop(captures) {
                _kingmove<state, true, leaf, root>(board, king, 1ull << SquareOf(captures), var);
                if (prune()) return;
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
                _ep_move<state, leaf, root>(board, lep, pawn_atk_left<us>(lep), ep_target, var);
                if (prune()) return;
            }
            if (rep) _ep_move<state, leaf, root>(board, rep, pawn_atk_right<us>(rep), ep_target, var);
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
                _silent_move<state, true, PAWN, leaf, root>(board, 1ull << (sq + pawn_shift[0][us]), 1ull << sq, var);
                if (prune()) return;
            }
            Bitloop(pl){
                Square sq = SquareOf(pl);
                _silent_move<state, true, PAWN, leaf, root>(board, 1ull << (sq + pawn_shift[1][us]), 1ull << sq, var);
                if (prune()) return;
            }
            Bitloop(pr_promo){
                Square sq = SquareOf(pr_promo); Bit from = 1ull << (sq + pawn_shift[0][us]), to = 1ull << sq;
                _promotion_move<state, QUEEN, true, leaf, root>(board, from, to, var);
                if (prune()) return;
                _promotion_move<state, ROOK, true, leaf, root>(board, from, to, var);
                if (prune()) return;
                _promotion_move<state, BISHOP, true, leaf, root>(board, from, to, var);
                if (prune()) return;
                _promotion_move<state, KNIGHT, true, leaf, root>(board, from, to, var);
                if (prune()) return;
            }
            Bitloop(pl_promo){
                Square sq = SquareOf(pl_promo); Bit from = 1ull << (sq + pawn_shift[1][us]), to = 1ull << sq;
                _promotion_move<state, QUEEN, true, leaf, root>(board, from, to, var);
                if (prune()) return;
                _promotion_move<state, ROOK, true, leaf, root>(board, from, to, var);
                if (prune()) return;
                _promotion_move<state, BISHOP, true, leaf, root>(board, from, to, var);
                if (prune()) return;
                _promotion_move<state, KNIGHT, true, leaf, root>(board, from, to, var);
                if (prune()) return;
            }
            Bitloop(pf){
                Square sq = SquareOf(pf);
                _silent_move<state, false, PAWN, leaf, root>(board, 1ull << (sq + sign<us>(-8)), 1ull << sq, var);
                if (prune()) return;
            }
            Bitloop(pf_promo){
                Square sq = SquareOf(pf_promo); Bit from = 1ull << (sq + sign<us>(-8)), to = 1ull << sq;
                _promotion_move<state, QUEEN, false, leaf, root>(board, from, to, var);
                if (prune()) return;
                _promotion_move<state, ROOK, false, leaf, root>(board, from, to, var);
                if (prune()) return;
                _promotion_move<state, BISHOP, false, leaf, root>(board, from, to, var);
                if (prune()) return;
                _promotion_move<state, KNIGHT, false, leaf, root>(board, from, to, var);
                if (prune()) return;
            }
            Bitloop(pp){
                Square sq = SquareOf(pp);
                _pawn_push<state, leaf, root>(board, 1ull << (sq + sign<us>(-16)), 1ull << sq, var);
                if (prune()) return;
            }
        }

        template<State state, Piece piece, bool root, bool leaf>
        static FORCEINLINE void silent_move(const Board& board, map& moves, map& captures, const Square& sq, vars& var){
            if constexpr (leaf)
                count(moves | captures);
            Bitloop(moves) {
                _silent_move<state, false, piece, leaf, root>(board, 1ull << sq, 1ull << SquareOf(moves), var);
                if (prune()) return;
            }
            Bitloop(captures) {
                _silent_move<state, true, piece, leaf, root>(board, 1ull << sq, 1ull << SquareOf(captures), var);
                if (prune()) return;
            }
        }
        template<State state, bool root, bool leaf>
        static FORCEINLINE void rookmove(const Board& board, map& moves, map& captures, const Square& sq, vars& var){
            if constexpr (leaf)
                count(moves | captures);
            Bitloop(moves) {
                _rookmove<state, false, leaf, root>(board, 1ull << sq, 1ull << SquareOf(moves), var);
                if (prune()) return;
            }
            Bitloop(captures) {
                _rookmove<state, true, leaf, root>(board, 1ull << sq, 1ull << SquareOf(captures), var);
                if (prune()) return;
            }
        }

        template<State state, bool left, bool root, bool leaf>
        static FORCEINLINE void castlemove(const Board& board, vars& var){
            if constexpr (leaf) increment();
            if constexpr (left) {
                if constexpr (state.white_move) _castlemove<state, WHITE_OOO, leaf, root>(board, var);
                else                            _castlemove<state, BLACK_OOO, leaf, root>(board, var);
            }
            else {
                if constexpr (state.white_move) _castlemove<state, WHITE_OO, leaf, root>(board, var);
                else                            _castlemove<state, BLACK_OO, leaf, root>(board, var);
            }
        }
    };
}