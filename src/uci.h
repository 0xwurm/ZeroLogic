#pragma once
#include "misc.h"
#include "movegenerator.h"
#include <chrono>
#include "perft_callback.h"
#include "search_callback.h"

namespace ZeroLogic::UCI {
    static const char* engine_info = "id name ZeroLogic 1\nid author wurm\n";
    static u32 hash_size = 0x3ffffff;
    static const char* options = "option name Hash type spin default 67108863 min 1 max 4294967295\n";

    static void position(std::istringstream& is, Misc::boardstate*& bdst) {

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

        // yep! IT LEAKS (maybe)
        bool wm = bdst->state.white_move;
        bdst = new Misc::boardstate({Misc::fen<true>(fen), Misc::fen<false>(fen), Misc::fen(fen, wm)});

        while (is >> token) {
            bdst = new Misc::boardstate(Misc::noInfo_move(*bdst, getNumNotation(token[0], token[1]), getNumNotation(token[2], token[3]), token[4]));
        }
    }

    static void setoption(std::istringstream& is){
        std::string token, value, name;
        is >> token;

        while (is >> token && token != "value")
            name += (name.empty() ? "" : " ") + token;

        while (is >> token)
            value += (value.empty() ? "" : " ") + token;

        if (name == "hashsize"){
            hash_size = std::stoi(value);
        }
    }

    template <Boardstate::State state>
    static void go(std::istringstream& is, const Boardstate::Board& board, Bit ep_target) {

        std::string token;

        while (is >> token)

            if (token == "depth") {
                is >> token;
                Search::TT::init(hash_size);
                Search::new_Callback::go<state>(board, ep_target, std::stoi(token));
                Search::TT::clear();
            }
            else if (token == "perft") {
                is >> token;
                Perft::TT::init(hash_size);
                // Perft::Callback::go<state>(board, std::stoi(token), ep_target);
                Perft::TT::clear();
            }
            else if (token == "single"){
                is >> token;
                Search::TT::init(hash_size);
                Search::new_Callback::go_single<state>(board, ep_target, std::stoi(token));
                Search::TT::clear();
            }

    }

    static void loop(int argc, char* argv[]) {

        Movegen::init_lookup();
        TT::init_keys();
        auto* bdst = new Misc::boardstate({Misc::fen<true>(start_fen), Misc::fen<false>(start_fen), 0});

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
                std::cout   << engine_info
                            << "\n"
                            << options
                            << "uciok" << std::endl;
            }
            else if (token == "position") {
                UCI::position(is, bdst);
            }
            else if (token == "isready") {
                std::cout << "readyok\n";
            }
            else if (token == "setoption"){
                setoption(is);
            }
            else if (token == "d"){
                std::cout << "Fen: " << Misc::convert_fen(*bdst) << std::endl;
                char hex_hash[100];
                std::sprintf(hex_hash, "%llX", bdst->board.hash);
                std::cout << "Key: " << hex_hash << std::endl;
            }
            else if (token == "go") {
                // necessary for template-instatiation
                bool WM = bdst->state.white_move, EP = bdst->state.has_ep_pawn, WL = bdst->state.white_ooo;
                bool WR = bdst->state.white_oo, BL = bdst->state.black_ooo, BR = bdst->state.black_oo;
                if      ( WM &&  EP &&  WR &&  WL &&  BR &&  BL)       { constexpr Boardstate::State go_state = {true, true, true, true, true, true}; go<go_state>(is, bdst->board, bdst->ep_target); }
                else if ( WM &&  EP &&  WR &&  WL &&  BR && !BL)       { constexpr Boardstate::State go_state = {true, true, true, true, true, false}; go<go_state>(is, bdst->board, bdst->ep_target); }
                else if ( WM &&  EP &&  WR &&  WL && !BR &&  BL)       { constexpr Boardstate::State go_state = {true, true, true, true, false, true}; go<go_state>(is, bdst->board, bdst->ep_target); }
                else if ( WM &&  EP &&  WR &&  WL && !BR && !BL)       { constexpr Boardstate::State go_state = {true, true, true, true, false, false}; go<go_state>(is, bdst->board, bdst->ep_target); }
                else if ( WM &&  EP &&  WR && !WL &&  BR &&  BL)       { constexpr Boardstate::State go_state = {true, true, true, false, true, true}; go<go_state>(is, bdst->board, bdst->ep_target); }
                else if ( WM &&  EP &&  WR && !WL &&  BR && !BL)       { constexpr Boardstate::State go_state = {true, true, true, false, true, false}; go<go_state>(is, bdst->board, bdst->ep_target); }
                else if ( WM &&  EP &&  WR && !WL && !BR &&  BL)       { constexpr Boardstate::State go_state = {true, true, true, false, false, true}; go<go_state>(is, bdst->board, bdst->ep_target); }
                else if ( WM &&  EP &&  WR && !WL && !BR && !BL)       { constexpr Boardstate::State go_state = {true, true, true, false, false, false}; go<go_state>(is, bdst->board, bdst->ep_target); }
                else if ( WM &&  EP && !WR &&  WL &&  BR &&  BL)       { constexpr Boardstate::State go_state = {true, true, false, true, false, true}; go<go_state>(is, bdst->board, bdst->ep_target); }
                else if ( WM &&  EP && !WR &&  WL &&  BR && !BL)       { constexpr Boardstate::State go_state = {true, true, false, true, false, false}; go<go_state>(is, bdst->board, bdst->ep_target); }
                else if ( WM &&  EP && !WR &&  WL && !BR &&  BL)       { constexpr Boardstate::State go_state = {true, true, false, true, true, true}; go<go_state>(is, bdst->board, bdst->ep_target); }
                else if ( WM &&  EP && !WR &&  WL && !BR && !BL)       { constexpr Boardstate::State go_state = {true, true, false, true, true, false}; go<go_state>(is, bdst->board, bdst->ep_target); }
                else if ( WM &&  EP && !WR && !WL &&  BR &&  BL)       { constexpr Boardstate::State go_state = {true, true, false, false, true, true}; go<go_state>(is, bdst->board, bdst->ep_target); }
                else if ( WM &&  EP && !WR && !WL &&  BR && !BL)       { constexpr Boardstate::State go_state = {true, true, false, false, true, false}; go<go_state>(is, bdst->board, bdst->ep_target); }
                else if ( WM &&  EP && !WR && !WL && !BR &&  BL)       { constexpr Boardstate::State go_state = {true, true, false, false, false, true}; go<go_state>(is, bdst->board, bdst->ep_target); }
                else if ( WM &&  EP && !WR && !WL && !BR && !BL)       { constexpr Boardstate::State go_state = {true, true, false, false, false, false}; go<go_state>(is, bdst->board, bdst->ep_target); }
                else if ( WM && !EP &&  WR &&  WL &&  BR &&  BL)       { constexpr Boardstate::State go_state = {true, false, true, true, true, true}; go<go_state>(is, bdst->board, bdst->ep_target); }
                else if ( WM && !EP &&  WR &&  WL &&  BR && !BL)       { constexpr Boardstate::State go_state = {true, false, true, true, true, false}; go<go_state>(is, bdst->board, bdst->ep_target); }
                else if ( WM && !EP &&  WR &&  WL && !BR &&  BL)       { constexpr Boardstate::State go_state = {true, false, true, true, false, true}; go<go_state>(is, bdst->board, bdst->ep_target); }
                else if ( WM && !EP &&  WR &&  WL && !BR && !BL)       { constexpr Boardstate::State go_state = {true, false, true, true, false, false}; go<go_state>(is, bdst->board, bdst->ep_target); }
                else if ( WM && !EP &&  WR && !WL &&  BR &&  BL)       { constexpr Boardstate::State go_state = {true, false, true, false, true, true}; go<go_state>(is, bdst->board, bdst->ep_target); }
                else if ( WM && !EP &&  WR && !WL &&  BR && !BL)       { constexpr Boardstate::State go_state = {true, false, true, false, true, false}; go<go_state>(is, bdst->board, bdst->ep_target); }
                else if ( WM && !EP &&  WR && !WL && !BR &&  BL)       { constexpr Boardstate::State go_state = {true, false, true, false, false, true}; go<go_state>(is, bdst->board, bdst->ep_target); }
                else if ( WM && !EP &&  WR && !WL && !BR && !BL)       { constexpr Boardstate::State go_state = {true, false, true, false, false, false}; go<go_state>(is, bdst->board, bdst->ep_target); }
                else if ( WM && !EP && !WR &&  WL &&  BR &&  BL)       { constexpr Boardstate::State go_state = {true, false, false, true, true, true}; go<go_state>(is, bdst->board, bdst->ep_target); }
                else if ( WM && !EP && !WR &&  WL &&  BR && !BL)       { constexpr Boardstate::State go_state = {true, false, false, true, true, false}; go<go_state>(is, bdst->board, bdst->ep_target); }
                else if ( WM && !EP && !WR &&  WL && !BR &&  BL)       { constexpr Boardstate::State go_state = {true, false, false, true, false, true}; go<go_state>(is, bdst->board, bdst->ep_target); }
                else if ( WM && !EP && !WR &&  WL && !BR && !BL)       { constexpr Boardstate::State go_state = {true, false, false, true, false, false}; go<go_state>(is, bdst->board, bdst->ep_target); }
                else if ( WM && !EP && !WR && !WL &&  BR &&  BL)       { constexpr Boardstate::State go_state = {true, false, false, false, true, true}; go<go_state>(is, bdst->board, bdst->ep_target); }
                else if ( WM && !EP && !WR && !WL &&  BR && !BL)       { constexpr Boardstate::State go_state = {true, false, false, false, true, false}; go<go_state>(is, bdst->board, bdst->ep_target); }
                else if ( WM && !EP && !WR && !WL && !BR &&  BL)       { constexpr Boardstate::State go_state = {true, false, false, false, false, true}; go<go_state>(is, bdst->board, bdst->ep_target); }
                else if ( WM && !EP && !WR && !WL && !BR && !BL)       { constexpr Boardstate::State go_state = {true, false, false, false, false, false}; go<go_state>(is, bdst->board, bdst->ep_target); }
                else if (!WM &&  EP &&  WR &&  WL &&  BR &&  BL)       { constexpr Boardstate::State go_state = {false, true, true, true, true, true}; go<go_state>(is, bdst->board, bdst->ep_target); }
                else if (!WM &&  EP &&  WR &&  WL &&  BR && !BL)       { constexpr Boardstate::State go_state = {false, true, true, true, true, false}; go<go_state>(is, bdst->board, bdst->ep_target); }
                else if (!WM &&  EP &&  WR &&  WL && !BR &&  BL)       { constexpr Boardstate::State go_state = {false, true, true, true, false, true}; go<go_state>(is, bdst->board, bdst->ep_target); }
                else if (!WM &&  EP &&  WR &&  WL && !BR && !BL)       { constexpr Boardstate::State go_state = {false, true, true, true, false, false}; go<go_state>(is, bdst->board, bdst->ep_target); }
                else if (!WM &&  EP &&  WR && !WL &&  BR &&  BL)       { constexpr Boardstate::State go_state = {false, true, true, false, true, true}; go<go_state>(is, bdst->board, bdst->ep_target); }
                else if (!WM &&  EP &&  WR && !WL &&  BR && !BL)       { constexpr Boardstate::State go_state = {false, true, true, false, true, false}; go<go_state>(is, bdst->board, bdst->ep_target); }
                else if (!WM &&  EP &&  WR && !WL && !BR &&  BL)       { constexpr Boardstate::State go_state = {false, true, true, false, false, true}; go<go_state>(is, bdst->board, bdst->ep_target); }
                else if (!WM &&  EP &&  WR && !WL && !BR && !BL)       { constexpr Boardstate::State go_state = {false, true, true, false, false, false}; go<go_state>(is, bdst->board, bdst->ep_target); }
                else if (!WM &&  EP && !WR &&  WL &&  BR &&  BL)       { constexpr Boardstate::State go_state = {false, true, false, true, true, true}; go<go_state>(is, bdst->board, bdst->ep_target); }
                else if (!WM &&  EP && !WR &&  WL &&  BR && !BL)       { constexpr Boardstate::State go_state = {false, true, false, true, true, false}; go<go_state>(is, bdst->board, bdst->ep_target); }
                else if (!WM &&  EP && !WR &&  WL && !BR &&  BL)       { constexpr Boardstate::State go_state = {false, true, false, true, false, true}; go<go_state>(is, bdst->board, bdst->ep_target); }
                else if (!WM &&  EP && !WR &&  WL && !BR && !BL)       { constexpr Boardstate::State go_state = {false, true, false, true, false, false}; go<go_state>(is, bdst->board, bdst->ep_target); }
                else if (!WM &&  EP && !WR && !WL &&  BR &&  BL)       { constexpr Boardstate::State go_state = {false, true, false, false, true, true}; go<go_state>(is, bdst->board, bdst->ep_target); }
                else if (!WM &&  EP && !WR && !WL &&  BR && !BL)       { constexpr Boardstate::State go_state = {false, true, false, false, true, false}; go<go_state>(is, bdst->board, bdst->ep_target); }
                else if (!WM &&  EP && !WR && !WL && !BR &&  BL)       { constexpr Boardstate::State go_state = {false, true, false, false, false, true}; go<go_state>(is, bdst->board, bdst->ep_target); }
                else if (!WM &&  EP && !WR && !WL && !BR && !BL)       { constexpr Boardstate::State go_state = {false, true, false, false, false, false}; go<go_state>(is, bdst->board, bdst->ep_target); }
                else if (!WM && !EP &&  WR &&  WL &&  BR &&  BL)       { constexpr Boardstate::State go_state = {false, false, true, true, true, true}; go<go_state>(is, bdst->board, bdst->ep_target); }
                else if (!WM && !EP &&  WR &&  WL &&  BR && !BL)       { constexpr Boardstate::State go_state = {false, false, true, true, true, false}; go<go_state>(is, bdst->board, bdst->ep_target); }
                else if (!WM && !EP &&  WR &&  WL && !BR &&  BL)       { constexpr Boardstate::State go_state = {false, false, true, true, false, true}; go<go_state>(is, bdst->board, bdst->ep_target); }
                else if (!WM && !EP &&  WR &&  WL && !BR && !BL)       { constexpr Boardstate::State go_state = {false, false, true, true, false, false}; go<go_state>(is, bdst->board, bdst->ep_target); }
                else if (!WM && !EP &&  WR && !WL &&  BR &&  BL)       { constexpr Boardstate::State go_state = {false, false, true, false, true, true}; go<go_state>(is, bdst->board, bdst->ep_target); }
                else if (!WM && !EP &&  WR && !WL &&  BR && !BL)       { constexpr Boardstate::State go_state = {false, false, true, false, true, false}; go<go_state>(is, bdst->board, bdst->ep_target); }
                else if (!WM && !EP &&  WR && !WL && !BR &&  BL)       { constexpr Boardstate::State go_state = {false, false, true, false, false, true}; go<go_state>(is, bdst->board, bdst->ep_target); }
                else if (!WM && !EP &&  WR && !WL && !BR && !BL)       { constexpr Boardstate::State go_state = {false, false, true, false, false, false}; go<go_state>(is, bdst->board, bdst->ep_target); }
                else if (!WM && !EP && !WR &&  WL &&  BR &&  BL)       { constexpr Boardstate::State go_state = {false, false, false, true, true, true}; go<go_state>(is, bdst->board, bdst->ep_target); }
                else if (!WM && !EP && !WR &&  WL &&  BR && !BL)       { constexpr Boardstate::State go_state = {false, false, false, true, true, false}; go<go_state>(is, bdst->board, bdst->ep_target); }
                else if (!WM && !EP && !WR &&  WL && !BR &&  BL)       { constexpr Boardstate::State go_state = {false, false, false, true, false, true}; go<go_state>(is, bdst->board, bdst->ep_target); }
                else if (!WM && !EP && !WR &&  WL && !BR && !BL)       { constexpr Boardstate::State go_state = {false, false, false, true, false, false}; go<go_state>(is, bdst->board, bdst->ep_target); }
                else if (!WM && !EP && !WR && !WL &&  BR &&  BL)       { constexpr Boardstate::State go_state = {false, false, false, false, true, true}; go<go_state>(is, bdst->board, bdst->ep_target); }
                else if (!WM && !EP && !WR && !WL &&  BR && !BL)       { constexpr Boardstate::State go_state = {false, false, false, false, true, false}; go<go_state>(is, bdst->board, bdst->ep_target); }
                else if (!WM && !EP && !WR && !WL && !BR &&  BL)       { constexpr Boardstate::State go_state = {false, false, false, false, false, true}; go<go_state>(is, bdst->board, bdst->ep_target); }
                else if (!WM && !EP && !WR && !WL && !BR && !BL)       { constexpr Boardstate::State go_state = {false, false, false, false, false, false}; go<go_state>(is, bdst->board, bdst->ep_target); }
            }

        } while (token != "quit");

    }
}