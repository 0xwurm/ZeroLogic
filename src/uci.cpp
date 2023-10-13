#include "uci.h"
#include "misc.h"
#include "new_search.h"

namespace ZeroLogic::UCI{

    std::string engine_info = "id name ZeroLogic v4\nid author wurm\n";
    std::string start_fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

    void position(std::istringstream& is, Gamestate* gs) {

        Move m;
        std::string token, fen;

        is >> token;

        if (token == "startpos")
        {
            fen = start_fen;
            is >> token;
        }
        else if (token == "fen")
            while (is >> token && token != "moves")
                fen += token + " ";
        else
            return;

        gs->fen(fen);

        while (is >> token) {
            m = Misc::stringToNum(token, gs);
            if (gs->white_to_move)  gs->makemove<true>(m);
            else                    gs->makemove<false>(m);
        }
    }

    void go(std::istringstream& is, Gamestate* gs) {

        std::string token;

        while (is >> token)

            if (token == "movetime") {
                is >> token;
                if (gs->white_to_move) Search::start_iterative_deepening<true>(*gs, std::stoi(token));
                else                   Search::start_iterative_deepening<false>(*gs, std::stoi(token));
            }
            else if (token == "perft") {
                is >> token;
                if (gs->white_to_move)  Search::start_perft<true>(*gs, std::stoi(token));
                else                    Search::start_perft<false>(*gs, std::stoi(token));
            }

    }

    void loop(int argc, char* argv[]) {

        Gamestate gs{};
        gs.fen(start_fen);

        std::string token, cmd;

        for (int i = 1; i < argc; ++i)
            cmd += std::string(argv[i]) + " ";


        do {
            if (argc == 1 && !getline(std::cin, cmd))
                cmd = "quit";

            std::istringstream is(cmd);

            token.clear();
            is >> std::skipws >> token;

            if (token == "uci") {
                std::cout << engine_info << "uciok\n";
            }
            else if (token == "position") {
                UCI::position(is, &gs);
            }
            else if (token == "isready") {
                std::cout << "readyok\n";
            }
            else if (token == "go") {
                go(is, &gs);
            }

        } while (token != "quit");

    }
}