#include "gamegenie.hh"
#include <cassert>
#include <vector>

GameGenie::GameGenie()
{
    code_meta = { };
}

/*
 * Returns true, alongside the byte read. The boolean is true when the Game
 * Genie hijacks the CPU bus and returns the byte from the cheat code instead.
 */
std::pair<bool, std::uint8_t>
GameGenie::RB(std::uint16_t addr)
{
    if (auto search = code_meta.find(addr); search != code_meta.end())
        return { true, code_meta.at(addr) };

    return { false, 0xFF };
}

void
GameGenie::add_code(const std::string &code)
{
    std::uint16_t addr = 0;
    std::uint8_t data = 0;

    if (code.size() == 6)
        std::tie(addr, data) = decode_6_char_code(code);

    else if (code.size() == 8)
        std::tie(addr, data) = decode_8_char_code(code);

    else
        assert(false);

    code_meta[addr] = data;
}

std::pair<std::uint64_t, std::uint8_t>
GameGenie::decode_6_char_code(const std::string &code)
{
    std::vector <std::uint8_t> digits = { };
    std::uint16_t addr = 0;
    std::uint8_t data = 0;

    assert(code.size() == 6);
    for (auto c : code)
        digits.push_back(decode_char(c));

    addr = 0x8000 |
        ((digits[3] & 7) << 12) |
        ((digits[5] & 7) << 8)  | ((digits[4] & 8) << 8) |
        ((digits[2] & 7) << 4)  | ((digits[1] & 8) << 4) |
        ((digits[4] & 7) << 0)  | ((digits[3] & 8) << 0);

    data =
        ((digits[1] & 7) << 4) | ((digits[0] & 8) << 4) |
        ((digits[0] & 7) << 0) | ((digits[5] & 8) << 0);

    return { addr, data };
}

std::pair<std::uint64_t, std::uint8_t>
GameGenie::decode_8_char_code(const std::string &code)
{
    std::vector <std::uint8_t> digits = { };
    std::uint16_t addr = 0;
    std::uint8_t data = 0;
    /* TODO */
    return { addr, data };
}

std::uint8_t GameGenie::decode_char(char c)
{
    switch (c) {
        case 'A':
            return 0x0;
        case 'P':
            return 0x1;
        case 'Z':
            return 0x2;
        case 'L':
            return 0x3;
        case 'G':
            return 0x4;
        case 'I':
            return 0x5;
        case 'T':
            return 0x6;
        case 'Y':
            return 0x7;
        case 'E':
            return 0x8;
        case 'O':
            return 0x9;
        case 'X':
            return 0xA;
        case 'U':
            return 0xB;
        case 'K':
            return 0xC;
        case 'S':
            return 0xD;
        case 'V':
            return 0xE;
        case 'N':
            return 0xF;
        default: assert(false);
    }
}

