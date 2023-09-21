#include "uci.h"
#include "search.h"
#include "misc.h"

// purely copy paste from stockfish...

using namespace ZeroLogic;

void UCI::position(std::istringstream& is, SearchInstance* SI) {

    Misc misc;
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

    SI->gs.fen(fen);

    while (is >> token) {
        m = misc.stringToNum(token, &SI->gs);
        SI->gs.makemove(m);
    }
}

void UCI::go(std::istringstream& is, SearchInstance* SI) {

    std::string token;

    while (is >> token)

        if (token == "movetime") {
            is >> token;
            SI->horribleTimeVar = std::stoi(token);
        }

        /* 
        if (token == "searchmoves") // Needs to be the last command on the line
            while (is >> token)
                limits.searchmoves.push_back(UCI::to_move(pos, token));

        else if (token == "wtime")     is >> limits.time[WHITE];
        else if (token == "btime")     is >> limits.time[BLACK];
        else if (token == "winc")      is >> limits.inc[WHITE];
        else if (token == "binc")      is >> limits.inc[BLACK];
        else if (token == "movestogo") is >> limits.movestogo;
        else if (token == "depth")     is >> limits.depth;
        else if (token == "nodes")     is >> limits.nodes;
        else if (token == "movetime")  is >> limits.movetime;
        else if (token == "mate")      is >> limits.mate;
        else if (token == "perft")     is >> limits.perft;
        else if (token == "infinite")  limits.infinite = 1;
        else if (token == "ponder")    ponderMode = true;*/

    SI->newsearch(BESTMOVE, 99);

}

void UCI::loop(int argc, char* argv[]) {

    Gamestate* gs = new Gamestate;
    SearchInstance SI(gs);

    SI.gs.fen(start_fen);

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
            position(is, &SI);
        }
        else if (token == "isready") {
            std::cout << "readyok\n";
        }
        else if (token == "go") {
            go(is, &SI);
        }

    } while (token != "quit");

}