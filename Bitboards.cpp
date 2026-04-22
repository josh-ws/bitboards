#include <array>
#include <bit>
#include <byteswap.h>
#include <cassert>
#include <chrono>
#include <cstdint>
#include <format>
#include <iostream>
#include <vector>

inline constexpr uint64_t BitOf(uint8_t col, uint8_t row) { return 1ULL << (row * 8 + col); }
inline constexpr int      ColOf(int bit) { return bit & 7; }
inline constexpr int      RowOf(int bit) { return bit >> 3; }

static constexpr int      MAX_MOVES   = 218; // https://chess.stackexchange.com/questions/4490/maximum-possible-movement-in-a-turn
static constexpr int      PERFT_DEPTH = 6;
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

static std::array<uint64_t, 64>               KNIGHT_ATTACKS{};
static std::array<uint64_t, 64>               DIAG;
static std::array<uint64_t, 64>               ANTIDIAG;
static std::array<std::array<uint8_t, 64>, 8> FIRST_RANK_ATTACKS{};
static std::array<uint64_t, 8>                RANKS = {RANK_1, RANK_2, RANK_3, RANK_4, RANK_5, RANK_6, RANK_7, RANK_8};
static std::array<uint64_t, 8>                FILES = {FILE_A, FILE_B, FILE_C, FILE_D, FILE_E, FILE_F, FILE_G, FILE_H};

constexpr static bool OnBoard(int col, int row)
{
    return col >= 0 && col < 8 && row >= 0 && row < 8;
}

static void InitFirstRankAttacks()
{
    for (int col = 0; col < 8; col++)
        for (int rocc = 0; rocc < 64; rocc++) {
            const auto occ = uint8_t(rocc) << 1;
            auto       atk = 0;
            for (int c = col + 1; c < 8; c++) {
                atk |= 1u << c;
                if (occ & (1u << c))
                    break;
            }
            for (int c = col - 1; c >= 0; c--) {
                atk |= 1u << c;
                if (occ & (1u << c))
                    break;
            }
            FIRST_RANK_ATTACKS[col][rocc] = atk;
        }
}

static void InitDiag()
{
    for (int i = 0; i < 64; i++) {
        const auto col = ColOf(i), row = RowOf(i);
        auto       d = uint64_t(0);
        auto       a = uint64_t(0);
        // NE
        for (int dc = 1, dr = 1; OnBoard(col + dc, row + dr); dc++, dr++)
            d |= BitOf(col + dc, row + dr);
        for (int dc = -1, dr = -1; OnBoard(col + dc, row + dr); dc--, dr--)
            d |= BitOf(col + dc, row + dr);
        for (int dc = -1, dr = 1; OnBoard(col + dc, row + dr); dc--, dr++)
            a |= BitOf(col + dc, row + dr);
        for (int dc = 1, dr = -1; OnBoard(col + dc, row + dr); dc++, dr--)
            a |= BitOf(col + dc, row + dr);
        DIAG[i]     = d;
        ANTIDIAG[i] = a;
    }
}

static void InitKnightAttacks()
{
    for (int i = 0; i < 64; i++) {
        uint64_t b = 1ULL << i;
        uint64_t a = 0;
        a |= (b & ~FILE_H) << 17;
        a |= (b & ~FILE_A) << 15;
        a |= (b & ~(FILE_G | FILE_H)) << 10;
        a |= (b & ~(FILE_A | FILE_B)) << 6;
        a |= (b & ~(FILE_G | FILE_H)) >> 6;
        a |= (b & ~(FILE_A | FILE_B)) >> 10;
        a |= (b & ~FILE_H) >> 15;
        a |= (b & ~FILE_A) >> 17;
        KNIGHT_ATTACKS[i] = a;
    }
}

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
    NONE,
};

using Piece = uint8_t;

inline constexpr Piece     MakePiece(Color c, PieceType p) { return static_cast<Piece>((c << 3) | p); }
inline constexpr PieceType TypeOf(Piece p) { return static_cast<PieceType>(p & 7); }
inline constexpr Color     ColorOf(Piece p) { return static_cast<Color>(p >> 3); }

inline constexpr int Lsb(uint64_t b) { return std::countr_zero(b); }
inline constexpr int PopLsb(uint64_t &b)
{
    auto sq = std::countr_zero(b);
    b &= b - 1;
    return sq;
}

static uint64_t LineAttacks(uint64_t occ, uint64_t mask, uint64_t r)
{
    const auto o   = occ & mask;
    const auto fwd = (o - 2 * r);
    const auto rev = std::byteswap(std::byteswap(o) - 2 * std::byteswap(r));
    return (fwd ^ rev) & mask;
}

static uint64_t RankAttacks(uint64_t sq, uint64_t occ)
{
    const auto col  = ColOf(sq);
    const auto row  = RowOf(sq);
    const auto rocc = (occ >> (row * 8)) & 0xFF; // full 8-bit occupancy for that rank
    return (uint64_t(FIRST_RANK_ATTACKS[col][rocc]) << (row * 8));
}

static uint64_t BishopAttacks(int sq, uint64_t occ)
{
    const auto r = 1ULL << sq;
    return LineAttacks(occ, DIAG[sq], r) | LineAttacks(occ, ANTIDIAG[sq], r);
}

static uint64_t RookAttacks(int sq, uint64_t occ)
{
    const auto r = 1ULL << sq;
    return LineAttacks(occ, FILES[ColOf(sq)], r) | RankAttacks(sq, occ);
}

static uint64_t QueenAttacks(int sq, uint64_t occ)
{
    return BishopAttacks(sq, occ) | RookAttacks(sq, occ);
}

struct Move {
    uint8_t from;
    uint8_t to;
    uint8_t piece;
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

static Position MakeMove(Position p, const Move &m)
{
    const auto mycolor    = p.whoseturn;
    const auto theircolor = Color(mycolor ^ 1);
    const auto from       = 1ULL << m.from;
    const auto to         = 1ULL << m.to;
    const auto move       = from ^ to;

    p.bitboards[mycolor][m.piece] ^= move;
    p.occupancy[mycolor] ^= move;

    if (p.occupancy[theircolor] & to) {
        for (int pt = 0; pt < 6; pt++) {
            if (p.bitboards[theircolor][pt] & to) {
                p.bitboards[theircolor][pt] ^= to;
                break;
            }
        }
        p.occupancy[theircolor] ^= to;
    }

    p.whoseturn = theircolor;
    return p;
}

static void UndoMove(Position &)
{
}

static int GenerateMoves(Position &p, std::array<Move, MAX_MOVES> &moves)
{
    const auto mycolor    = p.whoseturn;
    const auto theircolor = p.whoseturn ^ BLACK;
    const auto empty      = ~(p.occupancy[WHITE] | p.occupancy[BLACK]);
    const auto all        = p.occupancy[WHITE] | p.occupancy[BLACK];

    int index = 0;

    const auto emit = [&](int from, int to, uint8_t piece) {
        moves[index].from  = static_cast<uint8_t>(from & 0xFF);
        moves[index].to    = static_cast<uint8_t>(to & 0xFF);
        moves[index].piece = piece;
        index += 1;
    };

    const auto doTargets = [&](uint64_t targets, int add) {
        while (targets) {
            auto to = PopLsb(targets);
            emit(to + add, to, PAWN);
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
    else {
        auto single = (pawns >> 8) & empty;                             // single pawn moves
        auto dbl    = ((single & RANK_6) >> 8) & empty;                 // double pawn moves
        auto capR   = (pawns >> 9) & ~FILE_A & p.occupancy[theircolor]; // captures (left)
        auto capL   = (pawns >> 7) & ~FILE_H & p.occupancy[theircolor]; // captures (right)
        doTargets(single, 8);
        doTargets(dbl, 16);
        doTargets(capR, 15);
        doTargets(capL, 17);
    }

    auto knights = p.bitboards[mycolor][KNIGHT];
    while (knights) {
        auto from    = PopLsb(knights);
        auto targets = KNIGHT_ATTACKS[from] & ~p.occupancy[mycolor];
        while (targets)
            emit(from, PopLsb(targets), KNIGHT);
    }

    auto bishops = p.bitboards[mycolor][BISHOP];
    while (bishops) {
        auto from    = PopLsb(bishops);
        auto targets = BishopAttacks(from, all) & ~p.occupancy[mycolor];
        while (targets)
            emit(from, PopLsb(targets), BISHOP);
    }

    auto rooks = p.bitboards[mycolor][ROOK];
    while (rooks) {
        auto from    = PopLsb(rooks);
        auto targets = RookAttacks(from, all) & ~p.occupancy[mycolor];
        while (targets)
            emit(from, PopLsb(targets), ROOK);
    }

    auto queens = p.bitboards[mycolor][QUEEN];
    while (queens) {
        auto from    = PopLsb(queens);
        auto targets = QueenAttacks(from, all) & ~p.occupancy[mycolor];
        while (targets)
            emit(from, PopLsb(targets), QUEEN);
    }

    return index;
}

static uint64_t Perft(Position &p, int depth)
{
    if (depth <= 0)
        return 1;

    std::array<Move, MAX_MOVES> moves;
    int                         nmoves = GenerateMoves(p, moves);

    if (depth == 1)
        return nmoves;

    auto t = std::uint64_t(0);
    for (int i = 0; i < nmoves; i++) {
        auto newPos = MakeMove(p, moves[i]);
        t += Perft(newPos, depth - 1);
        UndoMove(p);
    }

    return t;
}

int main()
{
    InitFirstRankAttacks();
    InitKnightAttacks();
    InitDiag();

    assert(FIRST_RANK_ATTACKS[4][0] == 0b11101111);
    assert(FIRST_RANK_ATTACKS[4][0b000010] == 0b11101100);
    assert(FIRST_RANK_ATTACKS[4][0b000001] == 0b11101110);
    assert(FIRST_RANK_ATTACKS[0][0b000001] == 0b00000010);

    for (int i = 1; i <= PERFT_DEPTH; i++) {
        auto       p       = CreateDefaultPosition();
        const auto from    = std::chrono::steady_clock::now();
        const auto result  = Perft(p, i);
        const auto to      = std::chrono::steady_clock::now();
        const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(to - from);

        const auto msg = std::format("Perft({}): {} ({})", i, result, elapsed);
        std::cout << msg << "\n";
    }
}
