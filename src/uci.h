#pragma once
#include "misc.h"
#include "movegenerator.h"
#include <chrono>
#include "perft_callback.h"
#include "search_callback.h"

namespace ZeroLogic::UCI {
    static std::string engine_info = "id name ZeroLogic 1\nid author wurm\n";
    static u32 hash_size = 0x3ffffff;
    static std::string options = "option name Hash type spin default 67108863 min 1 max 4294967295\n";

    static void position(std::istringstream& is, Boardstate::Board*& board, Boardstate::State*& state, Bit& ep_target) {

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

        // leakage
        board = new Boardstate::Board(Boardstate::fen<true>(fen));
        state = new Boardstate::State(Boardstate::fen<false>(fen));
        if (state->white_move)  ep_target = Boardstate::fen<true, true>(fen);
        else                    ep_target = Boardstate::fen<false, true>(fen);

        while (is >> token) {
            Boardstate::move_container move = Misc::engine_move(token, *board, *state);
            board = new Boardstate::Board(Boardstate::move(move));
            state = new Boardstate::State(Boardstate::change_state(move));
            ep_target = Boardstate::ep_state(move);
        }
        board = new Boardstate::Board(board->BPawn, board->BKnight, board->BBishop, board->BRook, board->BQueen, board->BKing,
                                      board->WPawn, board->WKnight, board->WBishop, board->WRook, board->WQueen, board->WKing,
                                      Boardstate::hash_position(*board, *state, ep_target));
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
                Search::Callback::go<state>(board, std::stoi(token), ep_target, hash_size);
                Search::TT::clear();
            }
            else if (token == "perft") {
                is >> token;
                Perft::TT::init(hash_size);
                Perft::Callback::go<state>(board, std::stoi(token), ep_target);
                Perft::TT::clear();
            }
            else if (token == "single"){
                is >> token;
                // Search::go_single<state>(board, std::stoi(token), ep_target);
            }

    }

    static void loop(int argc, char* argv[]) {

        Movegen::init_lookup();
        TT::init_keys();
        Boardstate::State* state;
        Boardstate::Board* board;
        Boardstate::State start_state = Boardstate::State::normal();
        state = &start_state;
        Boardstate::Board start_board = Boardstate::fen<true>(start_fen);
        board = &start_board;
        Bit ep_target = 0;

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
                UCI::position(is, board, state, ep_target);
            }
            else if (token == "isready") {
                std::cout << "readyok\n";
            }
            else if (token == "setoption"){
                setoption(is);
            }
            else if (token == "d"){
                std::cout << "Fen: " << b_to_fen(*board, *state, ep_target) << std::endl;
                char hex_hash[100];
                std::sprintf(hex_hash, "%llX", hash_position(*board, *state, ep_target));
                std::cout << "Key: " << hex_hash << std::endl;
            }
            else if (token == "go") {
                // template instantiation hell
                bool WM = state->white_move, EP = state->has_ep_pawn, WL = state->white_ooo;
                bool WR = state->white_oo, BL = state->black_ooo, BR = state->black_oo;
                if      ( WM &&  EP &&  WR &&  WL &&  BR &&  BL)       { constexpr Boardstate::State go_state = {true, true, true, true, true, true}; go<go_state>(is, *board, ep_target); }
                else if ( WM &&  EP &&  WR &&  WL &&  BR && !BL)       { constexpr Boardstate::State go_state = {true, true, true, true, true, false}; go<go_state>(is, *board, ep_target); }
                else if ( WM &&  EP &&  WR &&  WL && !BR &&  BL)       { constexpr Boardstate::State go_state = {true, true, true, true, false, true}; go<go_state>(is, *board, ep_target); }
                else if ( WM &&  EP &&  WR &&  WL && !BR && !BL)       { constexpr Boardstate::State go_state = {true, true, true, true, false, false}; go<go_state>(is, *board, ep_target); }
                else if ( WM &&  EP &&  WR && !WL &&  BR &&  BL)       { constexpr Boardstate::State go_state = {true, true, true, false, true, true}; go<go_state>(is, *board, ep_target); }
                else if ( WM &&  EP &&  WR && !WL &&  BR && !BL)       { constexpr Boardstate::State go_state = {true, true, true, false, true, false}; go<go_state>(is, *board, ep_target); }
                else if ( WM &&  EP &&  WR && !WL && !BR &&  BL)       { constexpr Boardstate::State go_state = {true, true, true, false, false, true}; go<go_state>(is, *board, ep_target); }
                else if ( WM &&  EP &&  WR && !WL && !BR && !BL)       { constexpr Boardstate::State go_state = {true, true, true, false, false, false}; go<go_state>(is, *board, ep_target); }
                else if ( WM &&  EP && !WR &&  WL &&  BR &&  BL)       { constexpr Boardstate::State go_state = {true, true, false, true, false, true}; go<go_state>(is, *board, ep_target); }
                else if ( WM &&  EP && !WR &&  WL &&  BR && !BL)       { constexpr Boardstate::State go_state = {true, true, false, true, false, false}; go<go_state>(is, *board, ep_target); }
                else if ( WM &&  EP && !WR &&  WL && !BR &&  BL)       { constexpr Boardstate::State go_state = {true, true, false, true, true, true}; go<go_state>(is, *board, ep_target); }
                else if ( WM &&  EP && !WR &&  WL && !BR && !BL)       { constexpr Boardstate::State go_state = {true, true, false, true, true, false}; go<go_state>(is, *board, ep_target); }
                else if ( WM &&  EP && !WR && !WL &&  BR &&  BL)       { constexpr Boardstate::State go_state = {true, true, false, false, true, true}; go<go_state>(is, *board, ep_target); }
                else if ( WM &&  EP && !WR && !WL &&  BR && !BL)       { constexpr Boardstate::State go_state = {true, true, false, false, true, false}; go<go_state>(is, *board, ep_target); }
                else if ( WM &&  EP && !WR && !WL && !BR &&  BL)       { constexpr Boardstate::State go_state = {true, true, false, false, false, true}; go<go_state>(is, *board, ep_target); }
                else if ( WM &&  EP && !WR && !WL && !BR && !BL)       { constexpr Boardstate::State go_state = {true, true, false, false, false, false}; go<go_state>(is, *board, ep_target); }
                else if ( WM && !EP &&  WR &&  WL &&  BR &&  BL)       { constexpr Boardstate::State go_state = {true, false, true, true, true, true}; go<go_state>(is, *board, ep_target); }
                else if ( WM && !EP &&  WR &&  WL &&  BR && !BL)       { constexpr Boardstate::State go_state = {true, false, true, true, true, false}; go<go_state>(is, *board, ep_target); }
                else if ( WM && !EP &&  WR &&  WL && !BR &&  BL)       { constexpr Boardstate::State go_state = {true, false, true, true, false, true}; go<go_state>(is, *board, ep_target); }
                else if ( WM && !EP &&  WR &&  WL && !BR && !BL)       { constexpr Boardstate::State go_state = {true, false, true, true, false, false}; go<go_state>(is, *board, ep_target); }
                else if ( WM && !EP &&  WR && !WL &&  BR &&  BL)       { constexpr Boardstate::State go_state = {true, false, true, false, true, true}; go<go_state>(is, *board, ep_target); }
                else if ( WM && !EP &&  WR && !WL &&  BR && !BL)       { constexpr Boardstate::State go_state = {true, false, true, false, true, false}; go<go_state>(is, *board, ep_target); }
                else if ( WM && !EP &&  WR && !WL && !BR &&  BL)       { constexpr Boardstate::State go_state = {true, false, true, false, false, true}; go<go_state>(is, *board, ep_target); }
                else if ( WM && !EP &&  WR && !WL && !BR && !BL)       { constexpr Boardstate::State go_state = {true, false, true, false, false, false}; go<go_state>(is, *board, ep_target); }
                else if ( WM && !EP && !WR &&  WL &&  BR &&  BL)       { constexpr Boardstate::State go_state = {true, false, false, true, true, true}; go<go_state>(is, *board, ep_target); }
                else if ( WM && !EP && !WR &&  WL &&  BR && !BL)       { constexpr Boardstate::State go_state = {true, false, false, true, true, false}; go<go_state>(is, *board, ep_target); }
                else if ( WM && !EP && !WR &&  WL && !BR &&  BL)       { constexpr Boardstate::State go_state = {true, false, false, true, false, true}; go<go_state>(is, *board, ep_target); }
                else if ( WM && !EP && !WR &&  WL && !BR && !BL)       { constexpr Boardstate::State go_state = {true, false, false, true, false, false}; go<go_state>(is, *board, ep_target); }
                else if ( WM && !EP && !WR && !WL &&  BR &&  BL)       { constexpr Boardstate::State go_state = {true, false, false, false, true, true}; go<go_state>(is, *board, ep_target); }
                else if ( WM && !EP && !WR && !WL &&  BR && !BL)       { constexpr Boardstate::State go_state = {true, false, false, false, true, false}; go<go_state>(is, *board, ep_target); }
                else if ( WM && !EP && !WR && !WL && !BR &&  BL)       { constexpr Boardstate::State go_state = {true, false, false, false, false, true}; go<go_state>(is, *board, ep_target); }
                else if ( WM && !EP && !WR && !WL && !BR && !BL)       { constexpr Boardstate::State go_state = {true, false, false, false, false, false}; go<go_state>(is, *board, ep_target); }
                else if (!WM &&  EP &&  WR &&  WL &&  BR &&  BL)       { constexpr Boardstate::State go_state = {false, true, true, true, true, true}; go<go_state>(is, *board, ep_target); }
                else if (!WM &&  EP &&  WR &&  WL &&  BR && !BL)       { constexpr Boardstate::State go_state = {false, true, true, true, true, false}; go<go_state>(is, *board, ep_target); }
                else if (!WM &&  EP &&  WR &&  WL && !BR &&  BL)       { constexpr Boardstate::State go_state = {false, true, true, true, false, true}; go<go_state>(is, *board, ep_target); }
                else if (!WM &&  EP &&  WR &&  WL && !BR && !BL)       { constexpr Boardstate::State go_state = {false, true, true, true, false, false}; go<go_state>(is, *board, ep_target); }
                else if (!WM &&  EP &&  WR && !WL &&  BR &&  BL)       { constexpr Boardstate::State go_state = {false, true, true, false, true, true}; go<go_state>(is, *board, ep_target); }
                else if (!WM &&  EP &&  WR && !WL &&  BR && !BL)       { constexpr Boardstate::State go_state = {false, true, true, false, true, false}; go<go_state>(is, *board, ep_target); }
                else if (!WM &&  EP &&  WR && !WL && !BR &&  BL)       { constexpr Boardstate::State go_state = {false, true, true, false, false, true}; go<go_state>(is, *board, ep_target); }
                else if (!WM &&  EP &&  WR && !WL && !BR && !BL)       { constexpr Boardstate::State go_state = {false, true, true, false, false, false}; go<go_state>(is, *board, ep_target); }
                else if (!WM &&  EP && !WR &&  WL &&  BR &&  BL)       { constexpr Boardstate::State go_state = {false, true, false, true, true, true}; go<go_state>(is, *board, ep_target); }
                else if (!WM &&  EP && !WR &&  WL &&  BR && !BL)       { constexpr Boardstate::State go_state = {false, true, false, true, true, false}; go<go_state>(is, *board, ep_target); }
                else if (!WM &&  EP && !WR &&  WL && !BR &&  BL)       { constexpr Boardstate::State go_state = {false, true, false, true, false, true}; go<go_state>(is, *board, ep_target); }
                else if (!WM &&  EP && !WR &&  WL && !BR && !BL)       { constexpr Boardstate::State go_state = {false, true, false, true, false, false}; go<go_state>(is, *board, ep_target); }
                else if (!WM &&  EP && !WR && !WL &&  BR &&  BL)       { constexpr Boardstate::State go_state = {false, true, false, false, true, true}; go<go_state>(is, *board, ep_target); }
                else if (!WM &&  EP && !WR && !WL &&  BR && !BL)       { constexpr Boardstate::State go_state = {false, true, false, false, true, false}; go<go_state>(is, *board, ep_target); }
                else if (!WM &&  EP && !WR && !WL && !BR &&  BL)       { constexpr Boardstate::State go_state = {false, true, false, false, false, true}; go<go_state>(is, *board, ep_target); }
                else if (!WM &&  EP && !WR && !WL && !BR && !BL)       { constexpr Boardstate::State go_state = {false, true, false, false, false, false}; go<go_state>(is, *board, ep_target); }
                else if (!WM && !EP &&  WR &&  WL &&  BR &&  BL)       { constexpr Boardstate::State go_state = {false, false, true, true, true, true}; go<go_state>(is, *board, ep_target); }
                else if (!WM && !EP &&  WR &&  WL &&  BR && !BL)       { constexpr Boardstate::State go_state = {false, false, true, true, true, false}; go<go_state>(is, *board, ep_target); }
                else if (!WM && !EP &&  WR &&  WL && !BR &&  BL)       { constexpr Boardstate::State go_state = {false, false, true, true, false, true}; go<go_state>(is, *board, ep_target); }
                else if (!WM && !EP &&  WR &&  WL && !BR && !BL)       { constexpr Boardstate::State go_state = {false, false, true, true, false, false}; go<go_state>(is, *board, ep_target); }
                else if (!WM && !EP &&  WR && !WL &&  BR &&  BL)       { constexpr Boardstate::State go_state = {false, false, true, false, true, true}; go<go_state>(is, *board, ep_target); }
                else if (!WM && !EP &&  WR && !WL &&  BR && !BL)       { constexpr Boardstate::State go_state = {false, false, true, false, true, false}; go<go_state>(is, *board, ep_target); }
                else if (!WM && !EP &&  WR && !WL && !BR &&  BL)       { constexpr Boardstate::State go_state = {false, false, true, false, false, true}; go<go_state>(is, *board, ep_target); }
                else if (!WM && !EP &&  WR && !WL && !BR && !BL)       { constexpr Boardstate::State go_state = {false, false, true, false, false, false}; go<go_state>(is, *board, ep_target); }
                else if (!WM && !EP && !WR &&  WL &&  BR &&  BL)       { constexpr Boardstate::State go_state = {false, false, false, true, true, true}; go<go_state>(is, *board, ep_target); }
                else if (!WM && !EP && !WR &&  WL &&  BR && !BL)       { constexpr Boardstate::State go_state = {false, false, false, true, true, false}; go<go_state>(is, *board, ep_target); }
                else if (!WM && !EP && !WR &&  WL && !BR &&  BL)       { constexpr Boardstate::State go_state = {false, false, false, true, false, true}; go<go_state>(is, *board, ep_target); }
                else if (!WM && !EP && !WR &&  WL && !BR && !BL)       { constexpr Boardstate::State go_state = {false, false, false, true, false, false}; go<go_state>(is, *board, ep_target); }
                else if (!WM && !EP && !WR && !WL &&  BR &&  BL)       { constexpr Boardstate::State go_state = {false, false, false, false, true, true}; go<go_state>(is, *board, ep_target); }
                else if (!WM && !EP && !WR && !WL &&  BR && !BL)       { constexpr Boardstate::State go_state = {false, false, false, false, true, false}; go<go_state>(is, *board, ep_target); }
                else if (!WM && !EP && !WR && !WL && !BR &&  BL)       { constexpr Boardstate::State go_state = {false, false, false, false, false, true}; go<go_state>(is, *board, ep_target); }
                else if (!WM && !EP && !WR && !WL && !BR && !BL)       { constexpr Boardstate::State go_state = {false, false, false, false, false, false}; go<go_state>(is, *board, ep_target); }
            }

        } while (token != "quit");

    }
}