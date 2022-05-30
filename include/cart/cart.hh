#pragma once
#include "cart/mapper.hh"
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

class Mapper;

struct Cart {

private:

    // The NES cartridge header - iNES file format based
    //      https://www.nesdev.org/wiki/INES
    struct CartHeader {
        // Conatant - $43 $45 $53 $1A ("NES" followed by MS-DOS end-of-file)
        uint8_t constants[4]; 
        // Size of PRG ROM in 16 KB units
        uint8_t size_prg_rom;
        // Size of CHR ROM in 8 KB units
        uint8_t size_chr_rom;

        /* The mapper number can be extracted from the upper four bits
           of the next two mapper_x entries in this header */
        
        // Mapper, mirroring, battery, trained
        uint8_t mapper_0;
        // Mapper, VS/Playchoice
        uint8_t mapper_1;
        
        // PRG_RAM size
        uint8_t size_prg_ram;
        // TV system
        uint8_t tv_system_0;
        // TV system - again
        uint8_t tv_system_1;
        // Usually filled with zeros, doesn't really matter
        uint8_t padding[5];
    } m_cart_header;

    // Vectors containing the contents of program and character roms as well as
    //      program ram, each will be resized based on cartridge specs
    std::vector<uint8_t> m_prg_rom;
    std::vector<uint8_t> m_chr_rom;
    std::vector<uint8_t> m_prg_ram;

    // A pointer to the cartridge's mapper and function to initialize said
    //      mapper pointer given the respective mapper number
    bool init_mapper(int mapper_number);
    std::unique_ptr<Mapper> m_mapper;

public:

    // Load a rom into the cartridge
    bool load_rom(const std::string& rom_path);

    // Memory access by CPU
    void cpu_WB(uint16_t addr, uint8_t value);
    uint8_t cpu_RB(uint16_t addr);

    // Memory access by PPU
    void ppu_WB(uint16_t addr, uint8_t value);
    uint8_t ppu_RB(uint16_t addr);

    // To allow mapper to access the memory read from the ROM
    uint8_t* get_PRG_ROM();
    uint8_t* get_CHR_ROM();
    uint8_t* get_PRG_RAM();

};
