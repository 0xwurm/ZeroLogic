#pragma once
#include "position.h"
#include "misc.h"
#include "limit.h"
#include "movegenerator.h"
#include "search.h"
#include "perft_callback.h"
#include "tests.h"
#include "game.h"

namespace ZeroLogic::UCI {

    static const char* engine_info = "id name ZeroLogic 1\nid author wurm\n";
    static const char* options = "option name Hash type spin default 67108863 min 1 max 4294967295\n";

    static u32 hash_size = (USE_INTRIN) ? 0x3ffffff : 0x7ffff; // TODO: bad

    // gets fen for new position
    // if 'moves' are specified it initializes a Position object to use its pre-existing, 'move' methods
    // and converts the Position that arises at the end of the sequence into a fen, which is then
    // returned to this function. We later use the fen to initialize the Position we will finally search.
    static std::string position(std::istringstream& is){

        std::string token, fen;

        is >> token;

        if (token == "startpos")
        {
            fen = start_fen;
            is >> token;
        }
        else if (token == "test")
        {
            is >> token;
            fen = Test::perftTests[std::stoi(token) - 1].fen;
            is >> token;
        }
        else if (token == "fen")
            while (is >> token && token != "moves")
                fen += token + " ";
        else
            return start_fen;

        return Misc::_moves(fen, &is);
    }

    static void setoption(std::istringstream& is){
        std::string token, value, name;
        is >> token;

        while (is >> token && token != "value")
            name += (name.empty() ? "" : " ") + token;

        while (is >> token)
            value += (value.empty() ? "" : " ") + token;

        if (name == "Hash")
            hash_size = std::stoi(value);
    }

    template <Color c>
    static void go_limits(Position<c>& pos){
        limits.start_time = Time::now();

        if (limits.type == PERFT)
            Perft::Callback::go(pos);

        else if (limits.type == SINGLE)
            Search::go_single(pos);

        else
            Search::go(pos);
    }

    template <Color c>
    static void go(Position<c> pos, std::istringstream* is) {

        std::string token;

        while (*is >> token)

            if (token == "depth"){
                *is >> token;
                limits.type = DEPTH;
                limits.allowed_depth = std::stoi(token);
            }

            else if (token == "wtime"){
                *is >> token;
                limits.set_time(c, std::stoi(token));
            }else if (token == "btime"){
                *is >> token;
                limits.set_time(!c, std::stoi(token));
            }else if (token == "winc"){
                *is >> token;
                limits.set_inc(c, std::stoi(token));
            }else if (token == "binc"){
                *is >> token;
                limits.set_inc(!c, std::stoi(token));
            }else if (token == "movestogo"){
                *is >> token;
                limits.togo = std::stoi(token);
            }

            else if (token == "movetime"){
                *is >> token;
                limits.type = MOVETIME;
                limits.allowed_time = std::stoi(token);
            }

            else if (token == "perft") {
                *is >> token;
                limits.type = PERFT;
                limits.allowed_depth = std::stoi(token);
            }
            else if (token == "single"){
                *is >> token;
                limits.type = SINGLE;
                limits.allowed_depth = std::stoi(token);
            }
            else if (token == "nodes"){
                *is >> token;
                limits.type = NODES;
                limits.allowed_nodes = std::stoi(token);
            }
            else if (token == "eval"){
                std::cout << pos.evaluate() << std::endl;
                return;
            }

        go_limits(pos);

    }
    PositionToTemplate(go, void, std::istringstream*)

    template<Color c>
    static void display(Position<c> pos, std::istringstream* is){
        std::cout << pos << "Fen: " << std::string(pos) << "\n";

        char hex_hash[100];
        std::sprintf(hex_hash, "%llX", pos.hash.getVal());
        std::cout << "Key: " << hex_hash << "\n" << std::endl;
    }
    PositionToTemplate(display, void, std::istringstream*)

    static void loop(int argc, char* argv[]) {

        Lookup::fill();
        Hash::init_keys();
        Evaluation::init_tables();

        std::string token, cmd, fen = start_fen;

        for (int i = 1; i < argc; ++i)
            cmd += std::string(argv[i]) + " ";


        do {
            if (argc == 1 && !getline(std::cin, cmd))
                cmd = "quit";

            std::istringstream is(cmd);

            token.clear();
            is >> std::skipws >> token;

            if (token == "uci") {
                std::cout   << engine_info
                            << "\n"
                            << options
                            << "uciok" << std::endl;
            }
            else if (token == "position")
                fen = UCI::position(is);

            else if (token == "isready")
                std::cout << "readyok\n";

            else if (token == "setoption")
                setoption(is);

            else if (token == "d")
                _display(fen, &is);

            else if (token == "go")
                _go(fen, &is);

            else if (token == "test")
                Test::perft();

            else if (token == "game")
                Game::_go(fen, &is);

        } while (token != "quit");


    }
}