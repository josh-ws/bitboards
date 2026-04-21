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

using Piece = uint8_t;

inline constexpr Piece     MakePiece(Color c, PieceType p) { return static_cast<Piece>((c << 3) | p); }
inline constexpr PieceType TypeOf(Piece p) { return static_cast<PieceType>(p & 7); }
inline constexpr Color     ColorOf(Piece p) { return static_cast<Color>(p >> 3); }

inline constexpr uint64_t BitOf(uint8_t col, uint8_t row) { return 1ULL << (row * 8 + col); }
inline constexpr int      ColOf(int bit) { return bit & 7; }
inline constexpr int      RowOf(int bit) { return bit >> 3; }

struct Move {
    int from;
    int to;
};

struct Position {
    uint64_t bitboards[2][6]{};
    uint64_t occupancy[2]{};
    bool     whitemove;
};

static Position CreateDefaultPosition()
{
    auto p                     = Position();
    p.bitboards[WHITE][PAWN]   = 0b00000000'00000000'00000000'00000000'00000000'00000000'11111111'00000000ULL;
    p.bitboards[WHITE][KING]   = 0b00000000'00000000'00000000'00000000'00000000'00000000'00000000'00001000ULL;
    p.bitboards[WHITE][QUEEN]  = 0b00000000'00000000'00000000'00000000'00000000'00000000'00000000'00010000ULL;
    p.bitboards[WHITE][BISHOP] = 0b00000000'00000000'00000000'00000000'00000000'00000000'00000000'00100100ULL;
    p.bitboards[WHITE][KNIGHT] = 0b00000000'00000000'00000000'00000000'00000000'00000000'00000000'01000010ULL;
    p.bitboards[WHITE][ROOK]   = 0b00000000'00000000'00000000'00000000'00000000'00000000'00000000'10000001ULL;
    p.bitboards[BLACK][PAWN]   = 0b00000000'11111111'00000000'00000000'00000000'00000000'00000000'00000000ULL;
    p.bitboards[BLACK][KING]   = 0b00001000'00000000'00000000'00000000'00000000'00000000'00000000'00000000ULL;
    p.bitboards[BLACK][QUEEN]  = 0b00010000'00000000'00000000'00000000'00000000'00000000'00000000'00000000ULL;
    p.bitboards[BLACK][BISHOP] = 0b00100100'00000000'00000000'00000000'00000000'00000000'00000000'00000000ULL;
    p.bitboards[BLACK][KNIGHT] = 0b01000010'00000000'00000000'00000000'00000000'00000000'00000000'00000000ULL;
    p.bitboards[BLACK][ROOK]   = 0b10000001'00000000'00000000'00000000'00000000'00000000'00000000'00000000ULL;
    p.occupancy[WHITE]         = p.bitboards[WHITE][PAWN] | p.bitboards[WHITE][KING] | p.bitboards[WHITE][QUEEN] | p.bitboards[WHITE][BISHOP] | p.bitboards[WHITE][KNIGHT] | p.bitboards[WHITE][ROOK];
    p.occupancy[BLACK]         = p.bitboards[BLACK][PAWN] | p.bitboards[BLACK][KING] | p.bitboards[BLACK][QUEEN] | p.bitboards[BLACK][BISHOP] | p.bitboards[BLACK][KNIGHT] | p.bitboards[BLACK][ROOK];
    return p;
}

static void MakeMove(Position &p, const Move &m)
{
}

static void UndoMove(Position &p)
{
}

static vector<Move> GenerateMoves(Position &p)
{
    return {};
}

static std::uint64_t Perft(Position &p, int depth)
{
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

int main()
{
    for (int i = 1; i <= PERFT_DEPTH; i++) {
        auto       p       = CreateDefaultPosition();
        const auto from    = std::chrono::steady_clock::now();
        const auto result  = Perft(p, i);
        const auto to      = std::chrono::steady_clock::now();
        const auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(to - from);

        const auto msg = std::format("Perft({}): {} ({})", i, result, elapsed);
        std::cout << msg << "\n";
    }
}
