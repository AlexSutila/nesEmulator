#pragma once

#include <cstdint>
#include <utility>
#include <string>
#include <map>

struct GameGenie
{

private:

    std::map<std::uint16_t, std::uint8_t> code_meta;

    std::pair<std::uint64_t, std::uint8_t> decode_6_char_code(const std::string &);
    std::pair<std::uint64_t, std::uint8_t> decode_8_char_code(const std::string &);
    std::uint8_t decode_char(char c);

public:

    std::pair<bool, std::uint8_t> RB(std::uint16_t addr);
    void add_code(const std::string &code);
    GameGenie();

};

