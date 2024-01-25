#pragma once

namespace ZeroLogic::Game{

    using namespace Search;

    Limit game_limits{};
    bool display{};

    template<Color c>
    void ply(Position<c> pos, bool player, bool Nselfplay = true)
    {
        std::cout << pos << std::endl;

        limits.type = NODES;
        limits.allowed_nodes = 0;
        TT::init();
        Movelist<c> ml;
        bool check = enumerate<NSearch, Callback>(pos, 10, ml.end);
        TT::clear();

        if (ml.start == ml.end) {
            if (check)  std::cout << "Checkmate! " << (player ? "Calculator" : "Player") << " wins!" << std::endl;
            else        std::cout << "Draw by stalemate!" << std::endl;
            return;
        }

        limits = game_limits;
        limits.start_time = Time::now();
        instanceStart:
        std::string move{};
        if (player)
        {
            std::cout << "Your move: ";
            std::cin >> move;
            if (move == "exit")
            {
                std::cout << "Left the game." << std::endl;
                return;
            }
            else if (move == "legal")
                ml.print();
            /*
            if (limits.stop())
            {
                std::cout << "Timeout! Calculator wins!" << std::endl;
                return;
            }
            */
        }
        else move = go(pos, display);

        if (!(ml.contains(move)))
        {
            std::cout << "Illegal Move." << std::endl;
            goto instanceStart;
        }

        std::istringstream is(move);
        std::string fen = Misc::moves(pos, &is);

        Position<NONE> npos = *fen;
        ply(Position<!c>(npos), !player && Nselfplay, Nselfplay);
    }

    template<Color c>
    void go(Position<c> pos, std::istringstream* is)
    {
        std::string token;
        *is >> token;

        std::string movetime;
        std::cout << "Movetime: ";
        std::cin >> movetime;
        game_limits.type = MOVETIME;
        game_limits.allowed_time = std::stoi(movetime);

        dispSel:
        std::string disp;
        std::cout << "Show engine info (y/n): ";
        std::cin >> disp;
        if (disp != "y" && disp != "n") goto dispSel;
        display = disp == "y";

        if (token == "selfplay")
            ply(pos, false, false);
        else
        {
            colSel:
            std::string col;
            std::cout << "Your color (w/b): ";
            std::cin >> col;
            if (col != "w" && col != "b") goto colSel;

            if ((c && col == "w") || (!c && col == "b"))    ply(pos, true);
            else                                            ply(pos, false);
        }
    }
    PositionToTemplate(go, void, std::istringstream*)

}
