#include <array>
#include <chrono>
#include <cstdint>
#include <format>
#include <iostream>
#include <vector>

using std::vector;

static constexpr int      MAX_MOVES   = 218; // https://chess.stackexchange.com/questions/4490/maximum-possible-movement-in-a-turn
static constexpr int      PERFT_DEPTH = 7;
static constexpr uint64_t RANK_1      = 0x00000000000000FFULL;
static constexpr uint64_t RANK_2      = 0x000000000000FF00ULL;
static constexpr uint64_t RANK_3      = 0x0000000000FF0000ULL;
static constexpr uint64_t RANK_4      = 0x00000000FF000000ULL;
static constexpr uint64_t RANK_5      = 0x000000FF00000000ULL;
static constexpr uint64_t RANK_6      = 0x0000FF0000000000ULL;
static constexpr uint64_t RANK_7      = 0x00FF000000000000ULL;
static constexpr uint64_t RANK_8      = 0xFF00000000000000ULL;
static constexpr uint64_t FILE_A      = 0x0101010101010101ULL;
static constexpr uint64_t FILE_B      = 0x0202020202020202ULL;
static constexpr uint64_t FILE_C      = 0x0404040404040404ULL;
static constexpr uint64_t FILE_D      = 0x0808080808080808ULL;
static constexpr uint64_t FILE_E      = 0x1010101010101010ULL;
static constexpr uint64_t FILE_F      = 0x2020202020202020ULL;
static constexpr uint64_t FILE_G      = 0x4040404040404040ULL;
static constexpr uint64_t FILE_H      = 0x8080808080808080ULL;

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

inline constexpr int Lsb(uint64_t b) { return std::countr_zero(b); }
inline constexpr int PopLsb(uint64_t &b)
{
    auto sq = std::countr_zero(b);
    b &= b - 1;
    return sq;
}

struct Move {
    uint8_t from;
    uint8_t to;
};

struct Position {
    uint64_t bitboards[2][6]{};
    uint64_t occupancy[2]{};
    Color    whoseturn;
};

static Position CreateDefaultPosition()
{
    auto p                     = Position();
    p.bitboards[WHITE][PAWN]   = 0b00000000'00000000'00000000'00000000'00000000'00000000'11111111'00000000ULL;
    p.bitboards[WHITE][KING]   = 0b00000000'00000000'00000000'00000000'00000000'00000000'00000000'00010000ULL;
    p.bitboards[WHITE][QUEEN]  = 0b00000000'00000000'00000000'00000000'00000000'00000000'00000000'00001000ULL;
    p.bitboards[WHITE][BISHOP] = 0b00000000'00000000'00000000'00000000'00000000'00000000'00000000'00100100ULL;
    p.bitboards[WHITE][KNIGHT] = 0b00000000'00000000'00000000'00000000'00000000'00000000'00000000'01000010ULL;
    p.bitboards[WHITE][ROOK]   = 0b00000000'00000000'00000000'00000000'00000000'00000000'00000000'10000001ULL;
    p.bitboards[BLACK][PAWN]   = 0b00000000'11111111'00000000'00000000'00000000'00000000'00000000'00000000ULL;
    p.bitboards[BLACK][KING]   = 0b00010000'00000000'00000000'00000000'00000000'00000000'00000000'00000000ULL;
    p.bitboards[BLACK][QUEEN]  = 0b00001000'00000000'00000000'00000000'00000000'00000000'00000000'00000000ULL;
    p.bitboards[BLACK][BISHOP] = 0b00100100'00000000'00000000'00000000'00000000'00000000'00000000'00000000ULL;
    p.bitboards[BLACK][KNIGHT] = 0b01000010'00000000'00000000'00000000'00000000'00000000'00000000'00000000ULL;
    p.bitboards[BLACK][ROOK]   = 0b10000001'00000000'00000000'00000000'00000000'00000000'00000000'00000000ULL;
    p.occupancy[WHITE]         = p.bitboards[WHITE][PAWN] | p.bitboards[WHITE][KING] | p.bitboards[WHITE][QUEEN] | p.bitboards[WHITE][BISHOP] | p.bitboards[WHITE][KNIGHT] | p.bitboards[WHITE][ROOK];
    p.occupancy[BLACK]         = p.bitboards[BLACK][PAWN] | p.bitboards[BLACK][KING] | p.bitboards[BLACK][QUEEN] | p.bitboards[BLACK][BISHOP] | p.bitboards[BLACK][KNIGHT] | p.bitboards[BLACK][ROOK];
    return p;
}

static void MakeMove(Position &, const Move &)
{
}

static void UndoMove(Position &)
{
}

static int GenerateMoves(Position &p, std::array<Move, MAX_MOVES> &moves)
{
    const auto mycolor    = p.whoseturn;
    const auto theircolor = p.whoseturn ^ BLACK;
    const auto empty      = ~(p.occupancy[WHITE] | p.occupancy[BLACK]);

    int index = 0;

    const auto emit = [&](int from, int to) {
        moves[index] = Move{static_cast<uint8_t>(from & 0xFF), static_cast<uint8_t>(to & 0xFF)};
        index += 1;
    };

    const auto doTargets = [&](uint64_t targets, int add) {
        while (targets) {
            auto to = PopLsb(targets);
            emit(to + add, to);
        }
    };

    const auto pawns = p.bitboards[mycolor][PAWN];
    if (mycolor == WHITE) {
        auto single = (pawns << 8) & empty;                             // single pawn moves
        auto dbl    = ((single & RANK_3) << 8) & empty;                 // double pawn moves
        auto capR   = (pawns << 9) & ~FILE_A & p.occupancy[theircolor]; // captures (left)
        auto capL   = (pawns << 7) & ~FILE_H & p.occupancy[theircolor]; // captures (right)
        doTargets(single, -8);
        doTargets(dbl, -16);
        doTargets(capR, -15);
        doTargets(capL, -17);
    }

    return index;
}

static uint64_t Perft(Position &p, std::array<Move, MAX_MOVES> &moves, int depth)
{
    if (depth <= 0)
        return 1;

    int nmoves = GenerateMoves(p, moves);
    if (depth == 1)
        return nmoves;

    auto t = std::uint64_t(0);
    for (int i = 0; i < nmoves; i++) {
        MakeMove(p, moves[i]);
        t += Perft(p, moves, depth - 1);
        UndoMove(p);
    }

    return t;
}

int main()
{
    auto position = CreateDefaultPosition();
    auto moves    = std::array<Move, MAX_MOVES>();
    auto numMoves = GenerateMoves(position, moves);
    for (int i = 0; i < numMoves; ++i) {
        const auto files   = "abcdefgh";
        const auto rank    = "12345678";
        auto       fromCol = moves[i].from % 8;
        auto       fromRow = moves[i].from / 8;
        auto       toCol   = moves[i].to % 8;
        auto       toRow   = moves[i].to / 8;
        const auto from    = std::format("{}{}", files[fromCol], rank[fromRow]);
        const auto to      = std::format("{}{}", files[toCol], rank[toRow]);
        std::cout << std::format("moves[{}]: {}{}\n", i, from, to);
    }
}
