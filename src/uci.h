#pragma once
#include "position.h"
#include "misc.h"
#include "movegenerator.h"
#ifdef USE_INTRIN
#include <chrono>
#endif
// #include "search_callback.h"
#include "perft_callback.h"
#include "tests.h"

namespace ZeroLogic::UCI {

    static const char* engine_info = "id name ZeroLogic 1\nid author wurm\n";
    static const char* options = "option name Hash type spin default 67108863 min 1 max 4294967295\n";

#ifdef USE_INTRIN
    static u32 hash_size = 0x3ffffff;
#else
    static u32 hash_size = 0x7ffff;
#endif

    // gets fen for new position
    // if 'moves' are specified it initializes a Position object to use its pre-existing, 'move' methods
    // and converts the Position that arises at the end of the sequence into a fen, which is then
    // returned to this function. We later use the fen to initialize the Position we will finally search.
    // As usual this is due to our great friend, the template.
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

        if (name == "hashsize")
            hash_size = std::stoi(value);
    }

    template <Color c>
    static void go(Position<c> pos, std::istringstream* is) {

        std::string token;

        while (*is >> token)

            if (token == "depth") {
                *is >> token;
                Search::TT::init(hash_size);
                // Search::go<state>(board, ep_target, std::stoi(token));
                Search::TT::clear();
            }
            else if (token == "perft") {
                *is >> token;
                Perft::TT::init(hash_size);
                Perft::Callback::go(pos, std::stoi(token));
                Perft::TT::clear();
            }
            else if (token == "single"){
                *is >> token;
                Search::TT::init(hash_size);
                // Search::go_single<state>(board, ep_target, std::stoi(token));
                Search::TT::clear();
            }
            else if (token == "eval"){
                // std::cout << Evaluation::evaluate<state.active_color>(board) << std::endl;
            }

    }
    PositionToTemplate(go, void, std::istringstream*)

    template<Color c>
    static void display(Position<c> pos, std::istringstream* is){ std::cout << pos; }
    PositionToTemplate(display, void, std::istringstream*)

    static void loop(int argc, char* argv[]) {

        Movegen::init_lookup();
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

        } while (token != "quit");

    }
}