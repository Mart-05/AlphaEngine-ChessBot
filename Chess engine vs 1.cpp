#include <bitset>
#include <map>
#include <iostream>
#include <chrono>
#include <string>


/**************

MACROS

***************/
//Shortcuts voor later: #define [naam] [betekenis (waar de shortcut voor staat)].
#define U64 unsigned long long
#define set_bit(bitboard, square) ((bitboard) |= (1ULL << (square)))
#define get_bit(bitboard, square) ((bitboard) & (1ULL << (square)))
#define pop_bit(bitboard, square) ((bitboard) &= ~(1ULL << (square)))

#define empty_board "8/8/8/8/8/8/8/8 w - - "
#define start_position "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1 "
#define tricky_position "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1 "
#define killer_position "rnbqkb1r/pp1p1pPp/8/2p1pP2/1P1P4/3P3P/P1P1P3/RNBQKBNR w KQkq e6 0 1"
#define dumb_ai "1rq2b1N/p1p1k1p1/7p/3Qn3/3P4/P7/1PP2PPP/R1B1R1K1 w - - 3 20 "

//Anders werkt het programma niet.
#pragma warning(disable : 4996)

/**************

BOARD ENUM

Vraag: Idee dat de enum een naam missen!?
***************/
//Rij voor namen van vakjes op het spelbord (en mogelijkheden voor kleur/stukken die met lijnen gaan??).
enum {
    a8, b8, c8, d8, e8, f8, g8, h8,
    a7, b7, c7, d7, e7, f7, g7, h7,
    a6, b6, c6, d6, e6, f6, g6, h6,
    a5, b5, c5, d5, e5, f5, g5, h5,
    a4, b4, c4, d4, e4, f4, g4, h4,
    a3, b3, c3, d3, e3, f3, g3, h3,
    a2, b2, c2, d2, e2, f2, g2, h2,
    a1, b1, c1, d1, e1, f1, g1, h1, no_sq
};

enum { white, black, both };
enum { rook, bishop };

//Kleuren en kant voor mogelijke toegestane casteling (schaakmove) aangegeven in bits. Hierbij staat w voor white, b voor black, q voor queenside en k voor kingside.
enum { wk = 1, wq = 2, bk = 4, bq = 8 };
//Alle mogelijke stukken in het spel met hoofdletters voor witte stukken.
enum { P, N, B, R, Q, K, p, n, b, r, q, k };

//Vakje naar coördinaten: voor vertaling van computertaal naar mensentaal.
const char* square_to_coordinates[] = {
    "a8", "b8", "c8", "d8", "e8", "f8", "g8", "h8",
    "a7", "b7", "c7", "d7", "e7", "f7", "g7", "h7",
    "a6", "b6", "c6", "d6", "e6", "f6", "g6", "h6",
    "a5", "b5", "c5", "d5", "e5", "f5", "g5", "h5",
    "a4", "b4", "c4", "d4", "e4", "f4", "g4", "h4",
    "a3", "b3", "c3", "d3", "e3", "f3", "g3", "h3",
    "a2", "b2", "c2", "d2", "e2", "f2", "g2", "h2",
    "a1", "b1", "c1", "d1", "e1", "f1", "g1", "h1",
};

//Ascii: American Standard Code for Information Interchange. Geen flauw idee.
char ascii_pieces[] = "PNBRQKpnbrqk";

//Van integer naar character dus van P naar 'P'. Dit is handig voor het printen en debuggen.
std::map<char, int> char_pieces = {
    {'P', P},
    {'N', N},
    {'B', B},
    {'R', R},
    {'Q', Q},
    {'K', K},
    {'p', p},
    {'n', n},
    {'b', b},
    {'r', r},
    {'q', q},
    {'k', k}
};

//Vraag: Geen idee waar dit voor is!?
U64 bitboards[12];
U64 occupancies[3];

//Game states: Kant, enpassant mogelijkheid en castling mogelijkheid.
int side;
int enpassant = no_sq;
int castle;

/**************

RANDOMS

***************/
unsigned int state = 1804289383;

unsigned int get_random_U32_number()
{
    unsigned int number = state;

    number ^= number << 13;
    number ^= number >> 17;
    number ^= number << 5;

    state = number;

    return number;
}

U64 get_random_U64_number()
{
    U64 n1, n2, n3, n4;

    n1 = (U64)(get_random_U32_number()) & 0xFFFF;
    n2 = (U64)(get_random_U32_number()) & 0xFFFF;
    n3 = (U64)(get_random_U32_number()) & 0xFFFF;
    n4 = (U64)(get_random_U32_number()) & 0xFFFF;

    return n1 | (n2 << 16) | (n3 << 32) | (n4 << 48);
}

U64 generate_magic_number()
{
    return get_random_U64_number() & get_random_U64_number() & get_random_U64_number();
}


/**************

BIT MANIP

***************/
static inline int count_bits(U64 bitboard)
{
    int count = 0;
    while (bitboard)
    {
        count++;
        bitboard &= bitboard - 1;
    }
    return count;
}

static inline int get_ls1b_index(U64 bitboard)
{
    if (bitboard)
    {
        return count_bits((bitboard & (~bitboard + 1)) - 1);
    }
    else return -1;
}


/**************

INPUT & OUTPUT

***************/
void print_bitboard(U64 bitboard)
{
    std::printf("\n");
    //Herhaald 9x voor het aantal horizontale rijen (0, 1, 2, 3, 4, 5, 6, 7, 8).
    for (int rank = 0; rank < 8; rank++)
    {
        //Herhaald 8x voor het aantal verticale rijen voor nu (0, 1, 2, 3, 4, 5, 6, 7, 8) maar eigenlijk (a, b, c, d, e, f, g, h). Dan zijn alle vakjes geweest want je hebt 8x8 + een rij onder en links om de naam van het vakje aan te geven. 
        for (int file = 0; file < 8; file++)
        {
            //Het nummer van het vakje van linksboven naar rechtsonder. Linksboven dus 1, daarna 2 en als laatste 64 rechtsonder.
            int square = rank * 8 + file;
            //Tenzij het de eerste verticale rij is (rij 0), dan print van 1 tot 8.
            if (file == 0)
            {
                std::printf("  %d ", 8 - rank);
            }
            std::printf(" %d", get_bit(bitboard, square) ? 1 : 0);
        }
        std::printf("\n");
    }
    std::printf("\n     a b c d e f g h\n\n");
    std::printf("     Bitboard: %llu\n\n", bitboard);
}

void print_board()
{
    std::printf("\n");
    for (int rank = 0; rank < 8; rank++)
    {
        for (int file = 0; file < 8; file++)
        {
            int square = rank * 8 + file;

            if (!file)
            {
                std::printf("  %d ", 8 - rank);
            }

            int piece = -1;

            for (int bitboard_piece = P; bitboard_piece <= k; bitboard_piece++)
            {
                if (get_bit(bitboards[bitboard_piece], square)) piece = bitboard_piece;
            }

            std::printf(" %c", (piece == -1) ? '.' : ascii_pieces[piece]);
        }
        std::printf("\n");
    }
    std::printf("\n     a b c d e f g h\n\n");
    std::printf("  Side:        %s\n", !side ? "White" : "Black");
    std::printf("  Enpassant:   %s\n", (enpassant != no_sq) ? square_to_coordinates[enpassant] : "No");
    std::printf("  Castling:    %c%c%c%c\n\n",
        (castle & wk) ? 'K' : '-',
        (castle & wq) ? 'Q' : '-',
        (castle & bk) ? 'k' : '-',
        (castle & bq) ? 'q' : '-');
}

void parse_fen(const char* fen)
{
    //Reset gamestate
    memset(bitboards, 0ULL, sizeof(bitboards));
    memset(occupancies, 0ULL, sizeof(occupancies));
    side = 0;
    enpassant = no_sq;
    castle = 0;

    for (int square = 0; square < 64 && *fen && *fen != ' ';)
    {
        if ((*fen >= 'b' && *fen <= 'r') || (*fen >= 'B' && *fen <= 'R'))
        {
            int piece = char_pieces[*fen];
            set_bit(bitboards[piece], square);
            square++;
            fen++;
        }
        else if (*fen >= '1' && *fen <= '8')
        {
            int offset = *fen - '0';
            square += offset;
            fen++;
        }
        else
        {
            fen++;
        }
    }

    fen++;
    (*fen == 'w') ? (side = white) : (side = black);
    fen += 2;
    while (*fen != ' ')
    {
        switch (*fen)
        {
        case 'K': castle |= wk; break;
        case 'Q': castle |= wq; break;
        case 'k': castle |= bk; break;
        case 'q': castle |= bq; break;
        case '-': break;
        }
        fen++;
    }
    fen++;
    if (*fen != '-')
    {
        int file = fen[0] - 'a';
        int rank = 8 - (fen[1] - '0');
        enpassant = rank * 8 + file;
    }
    else
    {
        enpassant = no_sq;
    }

    for (int piece = P; piece <= K; piece++) occupancies[white] |= bitboards[piece];
    for (int piece = p; piece <= k; piece++) occupancies[black] |= bitboards[piece];
    occupancies[both] |= occupancies[white];
    occupancies[both] |= occupancies[black];
}


/**************

ATTACKS

***************/
const U64 not_a_file = 18374403900871474942ULL;
const U64 not_h_file = 9187201950435737471ULL;
const U64 not_gh_file = 4557430888798830399ULL;
const U64 not_ab_file = 18229723555195321596ULL;

const int bishop_relevant_bits[64] = {
    6, 5, 5, 5, 5, 5, 5, 6,
    5, 5, 5, 5, 5, 5, 5, 5,
    5, 5, 7, 7, 7, 7, 5, 5,
    5, 5, 7, 9, 9, 7, 5, 5,
    5, 5, 7, 9, 9, 7, 5, 5,
    5, 5, 7, 7, 7, 7, 5, 5,
    5, 5, 5, 5, 5, 5, 5, 5,
    6, 5, 5, 5, 5, 5, 5, 6
};

const int rook_relevant_bits[64] = {
    12, 11, 11, 11, 11, 11, 11, 12,
    11, 10, 10, 10, 10, 10, 10, 11,
    11, 10, 10, 10, 10, 10, 10, 11,
    11, 10, 10, 10, 10, 10, 10, 11,
    11, 10, 10, 10, 10, 10, 10, 11,
    11, 10, 10, 10, 10, 10, 10, 11,
    11, 10, 10, 10, 10, 10, 10, 11,
    12, 11, 11, 11, 11, 11, 11, 12
};

U64 rook_magic_numbers[64] = {
    0x8a80104000800020ULL,
    0x140002000100040ULL,
    0x2801880a0017001ULL,
    0x100081001000420ULL,
    0x200020010080420ULL,
    0x3001c0002010008ULL,
    0x8480008002000100ULL,
    0x2080088004402900ULL,
    0x800098204000ULL,
    0x2024401000200040ULL,
    0x100802000801000ULL,
    0x120800800801000ULL,
    0x208808088000400ULL,
    0x2802200800400ULL,
    0x2200800100020080ULL,
    0x801000060821100ULL,
    0x80044006422000ULL,
    0x100808020004000ULL,
    0x12108a0010204200ULL,
    0x140848010000802ULL,
    0x481828014002800ULL,
    0x8094004002004100ULL,
    0x4010040010010802ULL,
    0x20008806104ULL,
    0x100400080208000ULL,
    0x2040002120081000ULL,
    0x21200680100081ULL,
    0x20100080080080ULL,
    0x2000a00200410ULL,
    0x20080800400ULL,
    0x80088400100102ULL,
    0x80004600042881ULL,
    0x4040008040800020ULL,
    0x440003000200801ULL,
    0x4200011004500ULL,
    0x188020010100100ULL,
    0x14800401802800ULL,
    0x2080040080800200ULL,
    0x124080204001001ULL,
    0x200046502000484ULL,
    0x480400080088020ULL,
    0x1000422010034000ULL,
    0x30200100110040ULL,
    0x100021010009ULL,
    0x2002080100110004ULL,
    0x202008004008002ULL,
    0x20020004010100ULL,
    0x2048440040820001ULL,
    0x101002200408200ULL,
    0x40802000401080ULL,
    0x4008142004410100ULL,
    0x2060820c0120200ULL,
    0x1001004080100ULL,
    0x20c020080040080ULL,
    0x2935610830022400ULL,
    0x44440041009200ULL,
    0x280001040802101ULL,
    0x2100190040002085ULL,
    0x80c0084100102001ULL,
    0x4024081001000421ULL,
    0x20030a0244872ULL,
    0x12001008414402ULL,
    0x2006104900a0804ULL,
    0x1004081002402ULL
};

U64 bishop_magic_numbers[64] = {
    0x40040844404084ULL,
    0x2004208a004208ULL,
    0x10190041080202ULL,
    0x108060845042010ULL,
    0x581104180800210ULL,
    0x2112080446200010ULL,
    0x1080820820060210ULL,
    0x3c0808410220200ULL,
    0x4050404440404ULL,
    0x21001420088ULL,
    0x24d0080801082102ULL,
    0x1020a0a020400ULL,
    0x40308200402ULL,
    0x4011002100800ULL,
    0x401484104104005ULL,
    0x801010402020200ULL,
    0x400210c3880100ULL,
    0x404022024108200ULL,
    0x810018200204102ULL,
    0x4002801a02003ULL,
    0x85040820080400ULL,
    0x810102c808880400ULL,
    0xe900410884800ULL,
    0x8002020480840102ULL,
    0x220200865090201ULL,
    0x2010100a02021202ULL,
    0x152048408022401ULL,
    0x20080002081110ULL,
    0x4001001021004000ULL,
    0x800040400a011002ULL,
    0xe4004081011002ULL,
    0x1c004001012080ULL,
    0x8004200962a00220ULL,
    0x8422100208500202ULL,
    0x2000402200300c08ULL,
    0x8646020080080080ULL,
    0x80020a0200100808ULL,
    0x2010004880111000ULL,
    0x623000a080011400ULL,
    0x42008c0340209202ULL,
    0x209188240001000ULL,
    0x400408a884001800ULL,
    0x110400a6080400ULL,
    0x1840060a44020800ULL,
    0x90080104000041ULL,
    0x201011000808101ULL,
    0x1a2208080504f080ULL,
    0x8012020600211212ULL,
    0x500861011240000ULL,
    0x180806108200800ULL,
    0x4000020e01040044ULL,
    0x300000261044000aULL,
    0x802241102020002ULL,
    0x20906061210001ULL,
    0x5a84841004010310ULL,
    0x4010801011c04ULL,
    0xa010109502200ULL,
    0x4a02012000ULL,
    0x500201010098b028ULL,
    0x8040002811040900ULL,
    0x28000010020204ULL,
    0x6000020202d0240ULL,
    0x8918844842082200ULL,
    0x4010011029020020ULL
};

U64 pawn_attacks[2][64];
U64 knight_attacks[64];
U64 king_attacks[64];

U64 bishop_masks[64];
U64 rook_masks[64];

U64 bishop_attacks[64][512];
U64 rook_attacks[64][4096];

U64 mask_pawn_attacks(int side, int square)
{
    U64 attacks = 0ULL;
    U64 bitboard = 0ULL;

    set_bit(bitboard, square);

    if (!side) // WHITE
    {
        if((bitboard >> 7) & not_a_file) attacks |= (bitboard >> 7);
        if ((bitboard >> 9) & not_h_file) attacks |= (bitboard >> 9);
    }
    else // BLACK
    {
        if ((bitboard << 7) & not_h_file) attacks |= (bitboard << 7);
        if ((bitboard << 9) & not_a_file) attacks |= (bitboard << 9);
    }
    return attacks;
}

U64 mask_knight_attacks(int square)
{
    U64 attacks = 0Ull;
    U64 bitboard = 0ULL;

    set_bit(bitboard, square);

    if ((bitboard >> 17) & not_h_file) attacks |= (bitboard >> 17);
    if ((bitboard >> 15) & not_a_file) attacks |= (bitboard >> 15);
    if ((bitboard >> 10) & not_gh_file) attacks |= (bitboard >> 10);
    if ((bitboard >> 6) & not_ab_file) attacks |= (bitboard >> 6);
    if ((bitboard << 17) & not_a_file) attacks |= (bitboard << 17);
    if ((bitboard << 15) & not_h_file) attacks |= (bitboard << 15);
    if ((bitboard << 10) & not_ab_file) attacks |= (bitboard << 10);
    if ((bitboard << 6) & not_gh_file) attacks |= (bitboard << 6);

    return attacks;
}

U64 mask_king_attacks(int square)
{
    U64 attacks = 0ULL;
    U64 bitboard = 0ULL;

    set_bit(bitboard, square);

    if (bitboard >> 8) attacks |= (bitboard >> 8);
    if ((bitboard >> 9) & not_h_file) attacks |= (bitboard >> 9);
    if ((bitboard >> 7) & not_a_file) attacks |= (bitboard >> 7);
    if ((bitboard >> 1) & not_h_file) attacks |= (bitboard >> 1);
    if (bitboard << 8) attacks |= (bitboard << 8);
    if ((bitboard << 9) & not_a_file) attacks |= (bitboard << 9);
    if ((bitboard << 7) & not_h_file) attacks |= (bitboard << 7);
    if ((bitboard << 1) & not_a_file) attacks |= (bitboard << 1);

    return attacks;
}

U64 mask_bishop_attacks(int square)
{
    U64 attacks = 0ULL;
    int r, f;
    int tr = square / 8;
    int tf = square % 8;

    for (r = tr + 1, f = tf + 1; r <= 6 && f <= 6; r++, f++) attacks |= (1ULL << (r * 8 + f));
    for (r = tr - 1, f = tf + 1; r >= 1 && f <= 6; r--, f++) attacks |= (1ULL << (r * 8 + f));
    for (r = tr + 1, f = tf - 1; r <= 6 && f >= 1; r++, f--) attacks |= (1ULL << (r * 8 + f));
    for (r = tr - 1, f = tf - 1; r >= 1 && f >= 1; r--, f--) attacks |= (1ULL << (r * 8 + f));

    return attacks;
}

U64 mask_rook_attacks(int square)
{
    U64 attacks = 0ULL;
    int r, f;
    int tr = square / 8;
    int tf = square % 8;

    for (r = tr + 1; r <= 6; r++) attacks |= (1ULL << (r * 8 + tf));
    for (r = tr - 1; r >= 1; r--) attacks |= (1ULL << (r * 8 + tf));
    for (f = tf + 1; f <= 6; f++) attacks |= (1ULL << (tr * 8 + f));
    for (f = tf - 1; f >= 1; f--) attacks |= (1ULL << (tr * 8 + f));

    return attacks;
}

U64 bishop_attacks_on_the_fly(int square, U64 block)
{
    U64 attacks = 0ULL;
    int r, f;
    int tr = square / 8;
    int tf = square % 8;

    for (r = tr + 1, f = tf + 1; r <= 7 && f <= 7; r++, f++)
    {
        attacks |= (1ULL << (r * 8 + f));
        if ((1ULL << (r * 8 + f)) & block) break;
    }
    for (r = tr - 1, f = tf + 1; r >= 0 && f <= 7; r--, f++)
    {
        attacks |= (1ULL << (r * 8 + f));
        if ((1ULL << (r * 8 + f)) & block) break;
    }
    for (r = tr + 1, f = tf - 1; r <= 7 && f >= 0; r++, f--)
    {
        attacks |= (1ULL << (r * 8 + f));
        if ((1ULL << (r * 8 + f)) & block) break;
    }
    for (r = tr - 1, f = tf - 1; r >= 0 && f >= 0; r--, f--)
    {
        attacks |= (1ULL << (r * 8 + f));
        if ((1ULL << (r * 8 + f)) & block) break;
    }

    return attacks;
}

U64 rook_attacks_on_the_fly(int square, U64 block)
{
    U64 attacks = 0ULL;
    int r, f;
    int tr = square / 8;
    int tf = square % 8;

    for (r = tr + 1; r <= 7; r++)
    {
        attacks |= (1ULL << (r * 8 + tf));
        if ((1ULL << (r * 8 + tf)) & block) break;
    }
    for (r = tr - 1; r >= 0; r--)
    {
        attacks |= (1ULL << (r * 8 + tf));
        if ((1ULL << (r * 8 + tf)) & block) break;
    }
    for (f = tf + 1; f <= 7; f++)
    {
        attacks |= (1ULL << (tr * 8 + f));
        if ((1ULL << (tr * 8 + f)) & block) break;
    }
    for (f = tf - 1; f >= 0; f--)
    {
        attacks |= (1ULL << (tr * 8 + f));
        if ((1ULL << (tr * 8 + f)) & block) break;
    }

    return attacks;
}

void init_leapers_attacks()
{
    for (int square = 0; square < 64; square++)
    {
        pawn_attacks[white][square] = mask_pawn_attacks(white, square);
        pawn_attacks[black][square] = mask_pawn_attacks(black, square);
        knight_attacks[square] = mask_knight_attacks(square);
        king_attacks[square] = mask_king_attacks(square);
    }
}

U64 set_occupancy(int index, int bits_in_mask, U64 attack_mask)
{
    U64 occupancy = 0ULL;

    for (int count = 0; count < bits_in_mask; count++)
    {
        int square = get_ls1b_index(attack_mask);

        pop_bit(attack_mask, square);
        if (index & (1 << count))
        {
            occupancy |= (1ULL << square);
        }
    }

    return occupancy;
}


/**************

MAGIC

***************/
U64 find_magic_number(int square, int relevant_bits, int bishop)
{
    U64 occupancies[4096];
    U64 attacks[4096];
    U64 used_attacks[4096];

    U64 attack_mask = bishop ? mask_bishop_attacks(square) : mask_rook_attacks(square);
    int occupancy_indicies = 1 << relevant_bits;

    for (int index = 0; index < occupancy_indicies; index++)
    {
        occupancies[index] = set_occupancy(index, relevant_bits, attack_mask);
        attacks[index] = bishop ? bishop_attacks_on_the_fly(square, occupancies[index]) : rook_attacks_on_the_fly(square, occupancies[index]);
    }

    for (int random_count = 0; random_count < 100000000; random_count++)
    {
        U64 magic_number = generate_magic_number();
        if (count_bits((attack_mask * magic_number) & 0xFF00000000000000) < 6) continue;

        memset(used_attacks, 0ULL, sizeof(used_attacks));

        int index, fail;

        for (index = 0, fail = 0; !fail && index < occupancy_indicies; index++)
        {
            int magic_index = (int) ((occupancies[index] * magic_number) >> (64 - relevant_bits));

            if (used_attacks[magic_index] == 0ULL)
                used_attacks[magic_index] = attacks[index];
            else if (used_attacks[magic_index] != attacks[index])
                fail = 1;
        }

        if (!fail)
            return magic_number;
    }
    std::printf("  Magic number fails!");
    return 0ULL;
}

void init_magic_numbers()
{
    for (int square = 0; square < 64; square++)
    {
        std::printf("0x%llxULL\n", find_magic_number(square, rook_relevant_bits[square], rook));
    }
    std::printf("\n\n\n");
    for (int square = 0; square < 64; square++)
    {
        std::printf("0x%llxULL\n", find_magic_number(square, bishop_relevant_bits[square], bishop));
    }
}

void init_sliders_attacks(int bishop)
{
    for (int square = 0; square < 64; square++)
    {
        bishop_masks[square] = mask_bishop_attacks(square);
        rook_masks[square] = mask_rook_attacks(square);

        U64 attack_mask = bishop ? bishop_masks[square] : rook_masks[square];

        int occupancy_indicies = (1 << count_bits(attack_mask));

        for (int index = 0; index < occupancy_indicies; index++)
        {
            if (bishop)
            {
                U64 occupancy = set_occupancy(index, bishop_relevant_bits[square], attack_mask);
                int magic_index = (occupancy * bishop_magic_numbers[square]) >> (64 - bishop_relevant_bits[square]);

                bishop_attacks[square][magic_index] = bishop_attacks_on_the_fly(square, occupancy);
            }
            else
            {
                U64 occupancy = set_occupancy(index, rook_relevant_bits[square], attack_mask);
                int magic_index = (occupancy * rook_magic_numbers[square]) >> (64 - rook_relevant_bits[square]);

                rook_attacks[square][magic_index] = rook_attacks_on_the_fly(square, occupancy);
            }
        }
    }
}

static inline U64 get_bishop_attacks(int square, U64 occupancy)
{
    occupancy &= bishop_masks[square];
    occupancy *= bishop_magic_numbers[square];
    occupancy >>= 64 - bishop_relevant_bits[square];

    return bishop_attacks[square][occupancy];
}

static inline U64 get_rook_attacks(int square, U64 occupancy)
{
    occupancy &= rook_masks[square];
    occupancy *= rook_magic_numbers[square];
    occupancy >>= 64 - rook_relevant_bits[square];

    return rook_attacks[square][occupancy];
}

static inline U64 get_queen_attacks(int square, U64 occupancy)
{
    return (get_bishop_attacks(square, occupancy) | get_rook_attacks(square, occupancy));
}


/**************

Encoding moves

***************/
#define encode_move(source, target, piece, promoted, capture, doublep, enpassant, castling) \
    (source) |          \
    (target << 6) |     \
    (piece << 12) |     \
    (promoted << 16) |  \
    (capture << 20) |   \
    (doublep << 21) |   \
    (enpassant << 22) | \
    (castling << 23)    \

#define get_move_source(move) (move & 0x3f)
#define get_move_target(move) ((move & 0xfc0) >> 6)
#define get_move_piece(move) ((move & 0xf000) >> 12)
#define get_move_promoted(move) ((move & 0xf0000) >> 16)
#define get_move_capture(move) (move & 0x100000)
#define get_move_double(move) (move & 0x200000)
#define get_move_enpassant(move) (move & 0x400000)
#define get_move_castling(move) (move & 0x800000)

typedef struct {
    int moves[256];
    int count;

} moves;

static inline void add_move(moves* move_list, int move)
{
    move_list->moves[move_list->count] = move;
    move_list->count++;
}

std::map<int, char> promoted_pieces = {
    {Q, 'q'},
    {R, 'r'},
    {B, 'b'},
    {N, 'n'},
    {q, 'q'},
    {r, 'r'},
    {b, 'b'},
    {n, 'n'}
};

void print_move(int move)
{
    if (get_move_promoted(move))
        std::printf("%s%s%c",
            square_to_coordinates[get_move_source(move)],
            square_to_coordinates[get_move_target(move)],
            promoted_pieces[get_move_promoted(move)]);
    else
        std::printf("%s%s",
            square_to_coordinates[get_move_source(move)],
            square_to_coordinates[get_move_target(move)]);
}

void print_move_list(moves* move_list)
{
    if (!move_list->count)
    {
        std::printf("\n  No moves found!\n\n");
        return;
    }
    printf("\n  move    piece   capture   double    enpass    castling\n\n");
    for (int move_count = 0; move_count < move_list->count; move_count++)
    {
        int move = move_list->moves[move_count];
        std::printf("  %s%s%c   %c       %d         %d         %d         %d\n",
            square_to_coordinates[get_move_source(move)],
            square_to_coordinates[get_move_target(move)],
            get_move_promoted(move) ? promoted_pieces[get_move_promoted(move)] : ' ',
            ascii_pieces[get_move_piece(move)],
            get_move_capture(move) ? 1 : 0,
            get_move_double(move) ? 1 : 0,
            get_move_enpassant(move) ? 1 : 0,
            get_move_castling(move) ? 1 : 0);
    }
    std::printf("\n\n  Total movecount: %d\n\n", move_list->count);
}

#define copy_board()                                                        \
    U64 bitboards_copy[12], occupancies_copy[3];                            \
    int side_copy, enpassant_copy, castle_copy;                             \
    memcpy(bitboards_copy, bitboards, 96);                                  \
    memcpy(occupancies_copy, occupancies, 24);                              \
    side_copy = side, enpassant_copy = enpassant, castle_copy = castle;     \

#define take_back()                                                         \
    memcpy(bitboards, bitboards_copy, 96);                                  \
    memcpy(occupancies, occupancies_copy, 24);                              \
    side = side_copy, enpassant = enpassant_copy, castle = castle_copy;     \


/**************

Move Gen

***************/
static inline int is_square_attacked(int square, int side)
{
    if ((side == white) && (pawn_attacks[black][square] & bitboards[P])) return 1;
    if ((side == black) && (pawn_attacks[white][square] & bitboards[p])) return 1;
    if (knight_attacks[square] & ((!side) ? bitboards[N] : bitboards[n])) return 1;
    if (get_bishop_attacks(square, occupancies[both]) & ((!side) ? bitboards[B] : bitboards[b])) return 1;
    if (get_rook_attacks(square, occupancies[both]) & ((!side) ? bitboards[R] : bitboards[r])) return 1;
    if (get_queen_attacks(square, occupancies[both]) & ((!side) ? bitboards[Q] : bitboards[q])) return 1;
    if (king_attacks[square] & ((!side) ? bitboards[K] : bitboards[k])) return 1;
    return 0;
}

void print_attacked_squares(int side)
{
    std::printf("\n");
    for (int rank = 0; rank < 8; rank++)
    {
        for (int file = 0; file < 8; file++)
        {
            int square = rank * 8 + file;
            if (!file)
            {
                std::printf("  %d  ", 8 - rank);
            }
            std::printf("%d ", is_square_attacked(square, side) ? 1 : 0);
        }
        std::printf("\n");
    }
    std::printf("\n     a b c d e f g h\n\n");
}

enum { all_moves, only_captures };

const int castling_rights[64] = {
     7, 15, 15, 15,  3, 15, 15, 11,
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15,
    13, 15, 15, 15, 12, 15, 15, 14
};

static inline int make_move(int move, int move_flag)
{
    if (move_flag == all_moves)
    {
        copy_board();

        int source_square = get_move_source(move);
        int target_square = get_move_target(move);
        int piece = get_move_piece(move);
        int promoted = get_move_promoted(move);
        int capture = get_move_capture(move);
        int double_push = get_move_double(move);
        int enpass = get_move_enpassant(move);
        int castling = get_move_castling(move);

        pop_bit(bitboards[piece], source_square);
        set_bit(bitboards[piece], target_square);

        if (capture)
        {
            int start_piece, end_piece;
            if (side == white)
            {
                start_piece = p;
                end_piece = k;
            }
            else
            {
                start_piece = P;
                end_piece = K;
            }
            for (int bb_piece = start_piece; bb_piece <= end_piece; bb_piece++)
            {
                if (get_bit(bitboards[bb_piece], target_square))
                {
                    pop_bit(bitboards[bb_piece], target_square);
                    break;
                }
            }
        }
        if (promoted)
        {
            pop_bit(bitboards[(side == white) ? P : p], target_square);
            set_bit(bitboards[promoted], target_square);
        }
        if (enpass)
        {
            (side == white) ?
                pop_bit(bitboards[p], target_square + 8) :
                pop_bit(bitboards[P], target_square - 8);
        }

        enpassant = no_sq;

        if (double_push)
        {
            (side == white) ?
                enpassant = target_square + 8 :
                enpassant = target_square - 8;
        }
        if (castling)
        {
            switch (target_square)
            {
            case (g1):
                pop_bit(bitboards[R], h1);
                set_bit(bitboards[R], f1);
                break;
            case (c1):
                pop_bit(bitboards[R], a1);
                set_bit(bitboards[R], d1);
                break;
            case (g8):
                pop_bit(bitboards[r], h8);
                set_bit(bitboards[r], f8);
                break;
            case (c8):
                pop_bit(bitboards[r], a8);
                set_bit(bitboards[r], d8);
                break;
            }
        }

        castle &= castling_rights[source_square];
        castle &= castling_rights[target_square];

        memset(occupancies, 0ULL, 24);
        for (int bb_piece = P; bb_piece <= K; bb_piece++) occupancies[white] |= bitboards[bb_piece];
        for (int bb_piece = p; bb_piece <= k; bb_piece++) occupancies[black] |= bitboards[bb_piece];
        occupancies[both] |= occupancies[white];
        occupancies[both] |= occupancies[black];

        side ^= 1;

        if (is_square_attacked((side == white) ? get_ls1b_index(bitboards[k]) : get_ls1b_index(bitboards[K]), side))
        {
            take_back();
            return 0;
        }
        else return 1;
    }
    else
    {
        if (get_move_capture(move)) make_move(move, all_moves);
        else return 0;
    }
}

static inline void generate_moves(moves *move_list)
{
    move_list->count = 0;
    int source_square, target_square;
    U64 bitboard, attacks;

    for (int piece = P; piece <= k; piece++)
    {
        bitboard = bitboards[piece];
        if (side == white)
        {
            if (piece == P)
            {
                while (bitboard)
                {
                    source_square = get_ls1b_index(bitboard);
                    target_square = source_square - 8;

                    if (!(target_square < a8) && !get_bit(occupancies[both], target_square))
                    {
                        if (source_square >= a7 && source_square <= h7) // If promotion
                        {
                            add_move(move_list, encode_move(source_square, target_square, piece, Q, 0, 0, 0, 0));
                            add_move(move_list, encode_move(source_square, target_square, piece, R, 0, 0, 0, 0));
                            add_move(move_list, encode_move(source_square, target_square, piece, B, 0, 0, 0, 0));
                            add_move(move_list, encode_move(source_square, target_square, piece, N, 0, 0, 0, 0));
                        }

                        else
                        {
                            add_move(move_list, encode_move(source_square, target_square, piece, 0, 0, 0, 0, 0));

                            if ((source_square >= a2 && source_square <= h2) && !get_bit(occupancies[both], target_square - 8))
                                add_move(move_list, encode_move(source_square, target_square - 8, piece, 0, 0, 1, 0, 0));
                        }
                    }
                    attacks = pawn_attacks[side][source_square] & occupancies[black];

                    while (attacks)
                    {
                        target_square = get_ls1b_index(attacks);

                        if (source_square >= a7 && source_square <= h7) // If promotion
                        {
                            add_move(move_list, encode_move(source_square, target_square, piece, Q, 1, 0, 0, 0));
                            add_move(move_list, encode_move(source_square, target_square, piece, R, 1, 0, 0, 0));
                            add_move(move_list, encode_move(source_square, target_square, piece, B, 1, 0, 0, 0));
                            add_move(move_list, encode_move(source_square, target_square, piece, N, 1, 0, 0, 0));
                        }
                        else
                        {
                            add_move(move_list, encode_move(source_square, target_square, piece, 0, 1, 0, 0, 0));
                        }
                        pop_bit(attacks, target_square);
                    }

                    if (enpassant != no_sq)
                    {
                        U64 enpassant_attacks = pawn_attacks[side][source_square] & (1ULL << enpassant);
                        if (enpassant_attacks)
                        {
                            int target_enpassant = get_ls1b_index(enpassant_attacks);
                            add_move(move_list, encode_move(source_square, target_enpassant, piece, 0, 1, 0, 1, 0));
                        }
                    }
                    pop_bit(bitboard, source_square);
                }
            }

            if (piece == K)
            {
                if (castle & wk)
                {
                    if (!get_bit(occupancies[both], f1) && !get_bit(occupancies[both], g1))
                    {
                        if (!is_square_attacked(e1, black) && !is_square_attacked(f1, black))
                        {
                            add_move(move_list, encode_move(e1, g1, piece, 0, 0, 0, 0, 1));
                        }
                    }
                }
                if (castle & wq)
                {
                    if (!get_bit(occupancies[both], b1) && !get_bit(occupancies[both], c1) && !get_bit(occupancies[both], d1))
                    {
                        if (!is_square_attacked(d1, black) && !is_square_attacked(e1, black))
                        {
                            add_move(move_list, encode_move(e1, c1, piece, 0, 0, 0, 0, 1));
                        }
                    }
                }
            }
        }
        else
        {
            if (piece == p)
            {
                while (bitboard)
                {
                    source_square = get_ls1b_index(bitboard);
                    target_square = source_square + 8;

                    if (!(target_square > h1) && !get_bit(occupancies[both], target_square))
                    {
                        if (source_square >= a2 && source_square <= h2) // If promotion
                        {
                            add_move(move_list, encode_move(source_square, target_square, piece, q, 0, 0, 0, 0));
                            add_move(move_list, encode_move(source_square, target_square, piece, r, 0, 0, 0, 0));
                            add_move(move_list, encode_move(source_square, target_square, piece, b, 0, 0, 0, 0));
                            add_move(move_list, encode_move(source_square, target_square, piece, n, 0, 0, 0, 0));
                        }
                        else
                        {
                            add_move(move_list, encode_move(source_square, target_square, piece, 0, 0, 0, 0, 0));

                            if ((source_square >= a7 && source_square <= h7) && !get_bit(occupancies[both], target_square + 8))
                                add_move(move_list, encode_move(source_square, target_square + 8, piece, 0, 0, 1, 0, 0));
                        }
                    }
                    attacks = pawn_attacks[side][source_square] & occupancies[white];

                    while (attacks)
                    {
                        target_square = get_ls1b_index(attacks);

                        if (source_square >= a2 && source_square <= h2) // If promotion
                        {
                            add_move(move_list, encode_move(source_square, target_square, piece, q, 1, 0, 0, 0));
                            add_move(move_list, encode_move(source_square, target_square, piece, r, 1, 0, 0, 0));
                            add_move(move_list, encode_move(source_square, target_square, piece, b, 1, 0, 0, 0));
                            add_move(move_list, encode_move(source_square, target_square, piece, n, 1, 0, 0, 0));
                        }
                        else
                        {
                            add_move(move_list, encode_move(source_square, target_square, piece, 0, 1, 0, 0, 0));
                        }

                        pop_bit(attacks, target_square);
                    }

                    if (enpassant != no_sq)
                    {
                        U64 enpassant_attacks = pawn_attacks[side][source_square] & (1ULL << enpassant);
                        if (enpassant_attacks)
                        {
                            int target_enpassant = get_ls1b_index(enpassant_attacks);
                            add_move(move_list, encode_move(source_square, target_enpassant, piece, 0, 1, 0, 1, 0));
                        }
                    }
                    pop_bit(bitboard, source_square);
                }
            }

            if (piece == k)
            {
                if (castle & bk)
                {
                    if (!get_bit(occupancies[both], f8) && !get_bit(occupancies[both], g8))
                    {
                        if (!is_square_attacked(e8, white) && !is_square_attacked(f8, white))
                        {
                            add_move(move_list, encode_move(e8, g8, piece, 0, 0, 0, 0, 1));
                        }
                    }
                }
                if (castle & bq)
                {
                    if (!get_bit(occupancies[both], b8) && !get_bit(occupancies[both], c8) && !get_bit(occupancies[both], d8))
                    {
                        if (!is_square_attacked(d8, white) && !is_square_attacked(e8, white))
                        {
                            add_move(move_list, encode_move(e8, c8, piece, 0, 0, 0, 0, 1));
                        }
                    }
                }
            }
        }

        if ((side == white) ? piece == N : piece == n)
        {
            while (bitboard)
            {
                source_square = get_ls1b_index(bitboard);
                attacks = knight_attacks[source_square] & ~occupancies[side];

                while (attacks)
                {
                    target_square = get_ls1b_index(attacks);
                    if (!get_bit(((side == white) ? occupancies[black] : occupancies[white]), target_square))
                    {
                        add_move(move_list, encode_move(source_square, target_square, piece, 0, 0, 0, 0, 0));
                    } else add_move(move_list, encode_move(source_square, target_square, piece, 0, 1, 0, 0, 0));
                    pop_bit(attacks, target_square);
                }
                pop_bit(bitboard, source_square);
            }
        }
        if ((side == white) ? piece == B : piece == b)
        {
            while (bitboard)
            {
                source_square = get_ls1b_index(bitboard);
                attacks = get_bishop_attacks(source_square, occupancies[both]) & ~occupancies[side];

                while (attacks)
                {
                    target_square = get_ls1b_index(attacks);
                    if (!get_bit(((side == white) ? occupancies[black] : occupancies[white]), target_square))
                    {
                        add_move(move_list, encode_move(source_square, target_square, piece, 0, 0, 0, 0, 0));
                    }
                    else add_move(move_list, encode_move(source_square, target_square, piece, 0, 1, 0, 0, 0));
                    pop_bit(attacks, target_square);
                }
                pop_bit(bitboard, source_square);
            }
        }
        if ((side == white) ? piece == R : piece == r)
        {
            while (bitboard)
            {
                source_square = get_ls1b_index(bitboard);
                attacks = get_rook_attacks(source_square, occupancies[both]) & ~occupancies[side];

                while (attacks)
                {
                    target_square = get_ls1b_index(attacks);
                    if (!get_bit(((side == white) ? occupancies[black] : occupancies[white]), target_square))
                    {
                        add_move(move_list, encode_move(source_square, target_square, piece, 0, 0, 0, 0, 0));
                    }
                    else add_move(move_list, encode_move(source_square, target_square, piece, 0, 1, 0, 0, 0));
                    pop_bit(attacks, target_square);
                }
                pop_bit(bitboard, source_square);
            }
        }
        if ((side == white) ? piece == Q : piece == q)
        {
            while (bitboard)
            {
                source_square = get_ls1b_index(bitboard);
                attacks = get_queen_attacks(source_square, occupancies[both]) & ~occupancies[side];

                while (attacks)
                {
                    target_square = get_ls1b_index(attacks);
                    if (!get_bit(((side == white) ? occupancies[black] : occupancies[white]), target_square))
                    {
                        add_move(move_list, encode_move(source_square, target_square, piece, 0, 0, 0, 0, 0));
                    }
                    else add_move(move_list, encode_move(source_square, target_square, piece, 0, 1, 0, 0, 0));
                    pop_bit(attacks, target_square);
                }
                pop_bit(bitboard, source_square);
            }
        }
        if ((side == white) ? piece == K : piece == k)
        {
            while (bitboard)
            {
                source_square = get_ls1b_index(bitboard);
                attacks = king_attacks[source_square] & ~occupancies[side];

                while (attacks)
                {
                    target_square = get_ls1b_index(attacks);
                    if (!get_bit(((side == white) ? occupancies[black] : occupancies[white]), target_square))
                    {
                        add_move(move_list, encode_move(source_square, target_square, piece, 0, 0, 0, 0, 0));
                    }
                    else add_move(move_list, encode_move(source_square, target_square, piece, 0, 1, 0, 0, 0));
                    pop_bit(attacks, target_square);
                }
                pop_bit(bitboard, source_square);
            }
        }
    }
}


/**************

SEARCH

***************/
int material_score[12] = {
    100,      // white pawn score
    300,      // white knight scrore
    350,      // white bishop score
    500,      // white rook score
   1000,      // white queen score
  10000,      // white king score
   -100,      // black pawn score
   -300,      // black knight scrore
   -350,      // black bishop score
   -500,      // black rook score
  -1000,      // black queen score
 -10000,      // black king score
};

// pawn positional score
const int pawn_score[64] =
{
    90,  90,  90,  90,  90,  90,  90,  90,
    30,  30,  30,  40,  40,  30,  30,  30,
    20,  20,  20,  30,  30,  30,  20,  20,
    10,  10,  10,  20,  20,  10,  10,  10,
     5,   5,  10,  20,  20,   5,   5,   5,
     0,   0,   0,   5,   5,   0,   0,   0,
     0,   0,   0, -10, -10,   0,   0,   0,
     0,   0,   0,   0,   0,   0,   0,   0
};

// knight positional score
const int knight_score[64] =
{
    -5,   0,   0,   0,   0,   0,   0,  -5,
    -5,   0,   0,  10,  10,   0,   0,  -5,
    -5,   5,  20,  20,  20,  20,   5,  -5,
    -5,  10,  20,  30,  30,  20,  10,  -5,
    -5,  10,  20,  30,  30,  20,  10,  -5,
    -5,   5,  20,  10,  10,  20,   5,  -5,
    -5,   0,   0,   0,   0,   0,   0,  -5,
    -5, -10,   0,   0,   0,   0, -10,  -5
};

// bishop positional score
const int bishop_score[64] =
{
     0,   0,   0,   0,   0,   0,   0,   0,
     0,   0,   0,   0,   0,   0,   0,   0,
     0,   0,   0,  10,  10,   0,   0,   0,
     0,   0,  10,  20,  20,  10,   0,   0,
     0,   0,  10,  20,  20,  10,   0,   0,
     0,  10,   0,   0,   0,   0,  10,   0,
     0,  30,   0,   0,   0,   0,  30,   0,
     0,   0, -10,   0,   0, -10,   0,   0

};

// rook positional score
const int rook_score[64] =
{
    50,  50,  50,  50,  50,  50,  50,  50,
    50,  50,  50,  50,  50,  50,  50,  50,
     0,   0,  10,  20,  20,  10,   0,   0,
     0,   0,  10,  20,  20,  10,   0,   0,
     0,   0,  10,  20,  20,  10,   0,   0,
     0,   0,  10,  20,  20,  10,   0,   0,
     0,   0,  10,  20,  20,  10,   0,   0,
     0,   0,   0,  20,  20,   0,   0,   0

};

// king positional score
const int king_score[64] =
{
     0,   0,   0,   0,   0,   0,   0,   0,
     0,   0,   5,   5,   5,   5,   0,   0,
     0,   5,   5,  10,  10,   5,   5,   0,
     0,   5,  10,  20,  20,  10,   5,   0,
     0,   5,  10,  20,  20,  10,   5,   0,
     0,   0,   5,  10,  10,   5,   0,   0,
     0,   5,   5,  -5,  -5,   0,   5,   0,
     0,   0,   5,   0, -15,   0,  10,   0
};

const int mirror_score[128] =
{
    a1, b1, c1, d1, e1, f1, g1, h1,
    a2, b2, c2, d2, e2, f2, g2, h2,
    a3, b3, c3, d3, e3, f3, g3, h3,
    a4, b4, c4, d4, e4, f4, g4, h4,
    a5, b5, c5, d5, e5, f5, g5, h5,
    a6, b6, c6, d6, e6, f6, g6, h6,
    a7, b7, c7, d7, e7, f7, g7, h7,
    a8, b8, c8, d8, e8, f8, g8, h8
};

static inline int evaluate()
{
    int score = 0;
    U64 bitboard;
    int piece, square;

    for (int bb_piece = P; bb_piece <= k; bb_piece++)
    {
        bitboard = bitboards[bb_piece];
        while (bitboard)
        {
            piece = bb_piece;
            square = get_ls1b_index(bitboard);

            score += material_score[piece];

            switch (piece)
            {
            case P: score += pawn_score[square]; break;
            case N: score += knight_score[square]; break;
            case B: score += bishop_score[square]; break;
            case R: score += rook_score[square]; break;
            //case Q: score += queen_score[square]; break;
            case K: score += king_score[square]; break;

            case p: score -= pawn_score[mirror_score[square]]; break;
            case n: score -= knight_score[mirror_score[square]]; break;
            case b: score -= bishop_score[mirror_score[square]]; break;
            case r: score -= rook_score[mirror_score[square]]; break;
            //case q: score -= queen_score[mirror_score[square]]; break;
            case k: score -= king_score[mirror_score[square]]; break;
            }

            pop_bit(bitboard, square);
        }
    }
    return (side == white) ? score : -score;
}

static int mvv_lva[12][12] = {
    105, 205, 305, 405, 505, 605,  105, 205, 305, 405, 505, 605,
    104, 204, 304, 404, 504, 604,  104, 204, 304, 404, 504, 604,
    103, 203, 303, 403, 503, 603,  103, 203, 303, 403, 503, 603,
    102, 202, 302, 402, 502, 602,  102, 202, 302, 402, 502, 602,
    101, 201, 301, 401, 501, 601,  101, 201, 301, 401, 501, 601,
    100, 200, 300, 400, 500, 600,  100, 200, 300, 400, 500, 600,

    105, 205, 305, 405, 505, 605,  105, 205, 305, 405, 505, 605,
    104, 204, 304, 404, 504, 604,  104, 204, 304, 404, 504, 604,
    103, 203, 303, 403, 503, 603,  103, 203, 303, 403, 503, 603,
    102, 202, 302, 402, 502, 602,  102, 202, 302, 402, 502, 602,
    101, 201, 301, 401, 501, 601,  101, 201, 301, 401, 501, 601,
    100, 200, 300, 400, 500, 600,  100, 200, 300, 400, 500, 600
};

const int MAX_PLY = 64;

int killer_moves[2][MAX_PLY];
int history_moves[12][64];

int pv_length[MAX_PLY];
int pv_table[MAX_PLY][MAX_PLY];

int follow_pv, score_pv;

long nodes;

int ply;

static inline void enable_pv_scoring(moves* move_list)
{
    follow_pv = 0;
    for (int count = 0; count < move_list->count; count++)
    {
        if (pv_table[0][ply] == move_list->moves[count])
        {
            score_pv = 1;
            follow_pv = 1;
        }
    }
}

static inline int score_move(int move)
{
    int piece_score = material_score[get_move_piece(move)];

    if (score_pv)
    {
        if (pv_table[0][ply] == move)
        {
            score_pv = 0;
            return 20000;
        }
    }

    if (get_move_capture(move))
    {
        int target_piece = P;

        int start_piece, end_piece;

        if (side == white) { start_piece = p; end_piece = k; }
        else { start_piece = P; end_piece = K; }

        for (int bb_piece = start_piece; bb_piece <= end_piece; bb_piece++)
        {
            if (get_bit(bitboards[bb_piece], get_move_target(move)))
            {
                target_piece = bb_piece;
                break;
            }
        }

        return mvv_lva[get_move_piece(move)][target_piece] + 10000;
    }
    else
    {
        if (killer_moves[0][ply] == move)
            return 9000;
        else if (killer_moves[1][ply] == move)
            return 8000;
        else
            return history_moves[get_move_piece(move)][get_move_target(move)] + piece_score;
    }
    return 0;
}

static inline int sort_moves(moves* move_list)
{
    int *move_scores = new int[move_list->count];

    for (int count = 0; count < move_list->count; count++)
        move_scores[count] = score_move(move_list->moves[count]);

    for (int current = 0; current < move_list->count; current++)
    {
        for (int next = current + 1; next < move_list->count; next++)
        {
            if (move_scores[current] < move_scores[next])
            {
                int temp_score = move_scores[current];
                move_scores[current] = move_scores[next];
                move_scores[next] = temp_score;

                int temp_move = move_list->moves[current];
                move_list->moves[current] = move_list->moves[next];
                move_list->moves[next] = temp_move;
            }
        }
    }
    return 0;
}


void print_move_scores(moves* move_list)
{
    std::printf("     Move scores:\n\n");

    for (int count = 0; count < move_list->count; count++)
    {
        std::printf("     move: ");
        print_move(move_list->moves[count]);
        std::printf(" score: %d\n", score_move(move_list->moves[count]));
    }
}

static inline int quiescence(int alpha, int beta)
{
    nodes++;

    int eval = evaluate();

    if (eval >= beta)
    {
        return beta;
    }

    if (eval > alpha)
    {
        alpha = eval;
    }

    moves move_list[1];
    generate_moves(move_list);

    sort_moves(move_list);

    for (int count = 0; count < move_list->count; count++)
    {
        copy_board();
        ply++;

        if (make_move(move_list->moves[count], only_captures) == 0)
        {
            ply--;
            continue;
        }

        int score = -quiescence(-beta, -alpha);

        ply--;
        take_back();

        if (score >= beta)
        {
            return beta;
        }

        if (score > alpha)
        {
            alpha = score;
        }
    }
    return alpha;
}

static inline int negamax(int alpha, int beta, int depth)
{
    int found_pv = 0;

    pv_length[ply] = ply;

    if (depth == 0)
        return quiescence(alpha, beta);

    if (ply > MAX_PLY - 1)
        return evaluate();

    nodes++;

    int in_check = is_square_attacked((side == white) ? get_ls1b_index(bitboards[K]) :
        get_ls1b_index(bitboards[k]), side ^ 1);

    if (in_check) depth++;

    int legal_moves = 0;

    moves move_list[1];
    generate_moves(move_list);

    if (follow_pv)
        enable_pv_scoring(move_list);

    sort_moves(move_list);

    for (int count = 0; count < move_list->count; count++)
    {
        copy_board();
        ply++;

        if (make_move(move_list->moves[count], all_moves) == 0)
        {
            ply--;
            continue;
        }
        legal_moves++;

        int score;

        if (found_pv)
        {
            score = -negamax(-alpha - 1, -alpha, depth - 1);
            if ((score > alpha) && (score < beta))
                score = -negamax(-beta, -alpha, depth - 1);
        } else
            score = -negamax(-beta, -alpha, depth - 1);

        ply--;
        take_back();

        if (score >= beta)
        {
            if (get_move_capture(move_list->moves[count]) == 0)
            {
                killer_moves[1][ply] = killer_moves[0][ply];
                killer_moves[0][ply] = move_list->moves[count];
            }
            return beta;
        }

        if (score > alpha)
        {
            if (get_move_capture(move_list->moves[count]) == 0)
                history_moves[get_move_piece(move_list->moves[count])][get_move_target(move_list->moves[count])] += depth;

            alpha = score;

            found_pv = 1;

            pv_table[ply][ply] = move_list->moves[count];
            for (int next = ply + 1; next < pv_length[ply + 1]; next++)
                pv_table[ply][next] = pv_table[ply + 1][next];

            pv_length[ply] = pv_length[ply + 1];
        }
    }
    if (legal_moves == 0)
    {
        if (in_check)
            return -49000 + ply;
        else
            return 0;
    }
    return alpha;
}


void search_position(int depth)
{
    nodes = 0;
    follow_pv = 0;
    score_pv = 0;
    memset(killer_moves, 0, sizeof(killer_moves));
    memset(history_moves, 0, sizeof(history_moves));
    memset(pv_table, 0, sizeof(pv_table));
    memset(pv_length, 0, sizeof(pv_length));

    for (int current = 1; current <= depth; current++)
    {
        follow_pv = 1;
        int score = negamax(-50000, 50000, current);

        std::printf("info score cp %d depth %d nodes %ld pv ", score, current, nodes);
        for (int count = 0; count < pv_length[0]; count++)
        {
            print_move(pv_table[0][count]);
            std::printf(" ");
        }
        std::printf("\n");
    }
    std::printf("bestmove ");
    print_move(pv_table[0][0]);
    std::printf("\n");
}


/**************

UCI

***************/
int parse_move(std::string move_string)
{
    moves move_list[1];
    generate_moves(move_list);

    int source_square = (move_string[0] - 'a') + (8 - (move_string[1] - '0')) * 8;
    int target_square = (move_string[2] - 'a') + (8 - (move_string[3] - '0')) * 8;

    for (int move_count = 0; move_count < move_list->count; move_count++)
    {
        int move = move_list->moves[move_count];
        if (source_square == get_move_source(move) && target_square == get_move_target(move))
        {
            int promoted_piece = get_move_promoted(move);
            if (promoted_piece)
            {
                if (move_string[4] == 'q' || move_string[4] == 'r' || move_string[4] == 'b' || move_string[4] == 'n')
                    return move;
                return 0;
            }
            return move;
        }
    }
    return 0;
}

void parse_position(const char* command)
{
    command += 9;
    const char* current_char = command;

    if (strncmp(command, "startpos", 8) == 0)
        parse_fen(start_position);
    else
    {
        current_char = strstr(command, "fen");

        if (current_char == NULL)
            parse_fen(start_position);
        else
        {
            current_char += 4;
            parse_fen(current_char);
        }
    }
    current_char = strstr(command, "moves");

    if (current_char != NULL)
    {
        current_char += 6;
        while (*current_char)
        {
            int move = parse_move(current_char);
            if (move == 0)
                break;

            make_move(move, all_moves);
            while (*current_char && *current_char != ' ') current_char++;
            current_char++;
        }
    }
}

void parse_go(const char* command)
{
    command += 3;
    int depth = -1;
    const char* current_depth = command;

    if (strncmp(command, "depth", 5) == 0) {
        current_depth += 6;
        depth = atoi(current_depth);
    }
    else
        depth = 5;

    search_position(depth);
}

void uci_loop()
{
    setbuf(stdin, NULL);
    setbuf(stdout, NULL);

    char input[2000];

    std::printf("id name CPPChess\n");
    std::printf("id author DevRudolf\n");
    std::printf("uciok\n");

    while (1)
    {
        memset(input, 0, sizeof(input));
        fflush(stdout);

        if (!fgets(input, 2000, stdin))
            continue;
        else if (input[0] == '\n')
            continue;
        else if (strncmp(input, "isready", 7) == 0)
        {
            std::printf("readyok\n");
            continue;
        }
        else if (strncmp(input, "position", 8) == 0)
            parse_position(input);
        else if (strncmp(input, "ucinewgame", 10) == 0)
            parse_position("position startpos");
        else if (strncmp(input, "go", 2) == 0)
            parse_go(input);
        else if (strncmp(input, "quit", 4) == 0)
            break;
        else if (strncmp(input, "uci", 3) == 0)
        {
            std::printf("id name CPPChess\n");
            std::printf("id author DevRudolf\n");
            std::printf("uciok\n");
        }
    }
}


/**************

MAIN DRIVER

***************/
void init_all()
{
    init_leapers_attacks();
    init_sliders_attacks(bishop);
    init_sliders_attacks(rook);
}

auto get_time_ms()
{
    auto ms_since_epoch = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
        );
    return ms_since_epoch;
}

static inline void perft_driver(int depth)
{
    if (depth == 0)
    {
        nodes++;
        return;
    }

    moves move_list[1];
    generate_moves(move_list);

    for (int move_count = 0; move_count < move_list->count; move_count++)
    {
        copy_board();

        if (!make_move(move_list->moves[move_count], all_moves)) continue;
        perft_driver(depth - 1);

        take_back();
    }
}

void perft(int depth)
{
    std::printf("\n     Performance test\n\n");
    moves move_list[1];
    generate_moves(move_list);
    auto start_time = get_time_ms();
    for (int move_count = 0; move_count < move_list->count; move_count++)
    {
        copy_board();
        if (!make_move(move_list->moves[move_count], all_moves)) continue;
        long all_nodes = nodes;
        perft_driver(depth - 1);
        long old_nodes = nodes - all_nodes;
        take_back();
        std::printf("     move: %s%s%c  nodes: %ld\n", square_to_coordinates[get_move_source(move_list->moves[move_count])],
            square_to_coordinates[get_move_target(move_list->moves[move_count])],
            get_move_promoted(move_list->moves[move_count]) ? promoted_pieces[get_move_promoted(move_list->moves[move_count])] : ' ',
            old_nodes);
    }
    std::printf("\n\n    Depth:   %d\n", depth);
    std::printf("    Nodes:   %ld\n", nodes);
    std::printf("    Time:    %ld\n", get_time_ms() - start_time);
}

int main()
{
    init_all();

    int debug = 0;

    if (debug)
    {
        parse_fen(start_position);
        print_board();
        for (int i = 1; i < 7; i++) {
            perft(i);
        }
        /*moves move_list[1];
        generate_moves(move_list);
        print_move_scores(move_list);
        sort_moves(move_list);
        print_move_scores(move_list);*/
    } else uci_loop();

    return 0;
}
