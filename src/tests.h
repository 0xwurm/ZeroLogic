#pragma once

namespace ZeroLogic::Test{

    struct PerftPos{
        std::string fen;
        u8 depth;
        u64 nodecount;
    };

    constexpr int numTests = 23;

    // positions from: https://gist.github.com/peterellisjones/8c46c28141c162d1d8a0f0badbc9cff9
    static PerftPos perftTests[numTests] =
            {
                    {"r6r/1b2k1bq/8/8/7B/8/8/R3K2R b KQ - 3 2", 1, 8},
                    {"8/8/8/2k5/2pP4/8/B7/4K3 b - d3 0 3", 1, 8},
                    {"r1bqkbnr/pppppppp/n7/8/8/P7/1PPPPPPP/RNBQKBNR w KQkq - 2 2", 1, 19},
                    {"r3k2r/p1pp1pb1/bn2Qnp1/2qPN3/1p2P3/2N5/PPPBBPPP/R3K2R b KQkq - 3 2", 1, 5},
                    {"2kr3r/p1ppqpb1/bn2Qnp1/3PN3/1p2P3/2N5/PPPBBPPP/R3K2R b KQ - 3 2", 1, 44},
                    {"rnb2k1r/pp1Pbppp/2p5/q7/2B5/8/PPPQNnPP/RNB1K2R w KQ - 3 9", 1, 39},
                    {"2r5/3pk3/8/2P5/8/2K5/8/8 w - - 5 4", 1, 9},
                    {"rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8", 3, 62379},
                    {"r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10", 3, 89890},
                    {"3k4/3p4/8/K1P4r/8/8/8/8 b - - 0 1", 6, 1134888},
                    {"8/8/4k3/8/2p5/8/B2P2K1/8 w - - 0 1", 6, 1015133},
                    {"8/8/1k6/2b5/2pP4/8/5K2/8 b - d3 0 1", 6, 1440467},
                    {"5k2/8/8/8/8/8/8/4K2R w K - 0 1", 6, 661072},
                    {"3k4/8/8/8/8/8/8/R3K3 w Q - 0 1", 6, 803711},
                    {"r3k2r/1b4bq/8/8/8/8/7B/R3K2R w KQkq - 0 1", 4, 1274206},
                    {"r3k2r/8/3Q4/8/8/5q2/8/R3K2R b KQkq - 0 1", 4, 1720476},
                    {"2K2r2/4P3/8/8/8/8/8/3k4 w - - 0 1", 6, 3821001},
                    {"8/8/1P2K3/8/2n5/1q6/8/5k2 b - - 0 1", 5, 1004658},
                    {"4k3/1P6/8/8/8/8/K7/8 w - - 0 1", 6, 217342},
                    {"8/P1k5/K7/8/8/8/8/8 w - - 0 1", 6, 92683},
                    {"K1k5/8/P7/8/8/8/8/8 w - - 0 1", 6, 2217},
                    {"8/k1P5/8/1K6/8/8/8/8 w - - 0 1", 7, 567584},
                    {"8/8/2k5/5q2/5n2/8/5K2/8 b - - 0 1", 4, 23527}
            };

    template<Color c>
    static u64 goPerft(Position<c> pos, u8 depth)
    {
        Perft::TT::init();
        u64 count = Perft::Callback::go<true>(pos, depth);
        Perft::TT::clear();
        return count;
    }
    PositionToTemplate(goPerft, u64, u8)

    static void perft()
    {
        int numPassed{}, numFailed{}, numPos{};

        printf(" %-10s| %-6s| %-12s| %-12s| %-8s\n", "Position", "Depth", "Perft", "Target", "Result");
        for (PerftPos& p : perftTests)
        {
            numPos++;
            u64 count = _goPerft(p.fen, p.depth);

            bool passed = count == p.nodecount;
            if (passed) numPassed++;
            else numFailed++;

            std::string pos = std::to_string(numPos) + "/" + std::to_string(numTests);
            printf(" %-10s| %-6d| %-12s| %-12s| %-8s\n",
                   pos.c_str(),
                   p.depth,
                   Misc::format(count).c_str(),
                   Misc::format(p.nodecount).c_str(),
                   passed ? "passed" : "failed");
        }

        std::cout << "\n" << "Passed " << numPassed << " out of " << numPos << " tests. Failed " << numFailed << std::endl;
    }

}