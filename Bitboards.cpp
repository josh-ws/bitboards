#include <chrono>
#include <cstdint>
#include <format>
#include <iostream>
#include <vector>

using std::vector;

static constexpr int PERFT_DEPTH = 7;

enum Color : uint8_t {
    WHITE,
    BLACK,
};

enum PieceType : uint8_t {
    PAWN,
    KING,
    QUEEN,
    BISHOP,
    KNIGHT,
    ROOK,
};

struct Move {
    int from;
    int to;
};

struct Position {
    uint64_t bitboards[2][6]{};
    uint64_t occupancy[2]{};
};

static Position CreateDefaultPosition() {
    return Position{};
}

static void MakeMove(Position &p, const Move &m) {
}

static void UndoMove(Position &p) {
}

static vector<Move> GenerateMoves(Position &p) {
    return {};
}

static std::uint64_t Perft(Position &p, int depth) {
    if (depth <= 0)
        return 1;

    const auto moves = GenerateMoves(p);
    if (depth == 1)
        return moves.size();

    auto t = std::uint64_t(0);
    for (const auto &move : moves) {
        MakeMove(p, move);
        t += Perft(p, depth - 1);
        UndoMove(p);
    }

    return t;
}

int main() {
    for (int i = 1; i <= PERFT_DEPTH; i++) {
        auto p = CreateDefaultPosition();
        const auto from = std::chrono::steady_clock::now();
        const auto result = Perft(p, i);
        const auto to = std::chrono::steady_clock::now();
        const auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(to - from);

        const auto msg = std::format("Perft({}): {} ({})", i, result, elapsed);
        std::cout << msg << "\n";
    }
}
