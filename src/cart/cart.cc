#include "cart/cart.hh"
#include <iostream>
#include <iterator>
#include <fstream>

// Initialize the mapper pointer with a pointer to the cartridges respective mapper. Will
//      be called within load_rom
bool Cart::init_mapper(int mapper_number) {

    // TODO
    
    return false;
}

// Initialize the memory blocks and fill them with the respective memory from the ROM file
bool Cart::load_rom(const std::string& rom_path) {

    // Open rom and read the cartridge header
    std::ifstream rom_file = std::ifstream(rom_path, std::ios::binary);
    if (rom_file.is_open()) {
        rom_file.read((char*)&m_cart_header, sizeof(CartHeader));
    }
    else {
        std::cout << "ROM file could not be opened" << std::endl;
        return false;
    }

    // Ignore the 512 byte trainer, if the file contains it
    if (m_cart_header.mapper_0 & 0x4 != 0x0) 
        rom_file.seekg(512, rom_file.cur);

    // Resize the vectors based on the specifications in the header
    m_prg_rom.resize(0x4000 * (int)m_cart_header.size_prg_rom * sizeof(uint8_t));
    m_chr_rom.resize(0x2000 * (int)m_cart_header.size_chr_rom * sizeof(uint8_t));

    // Copy the rom to the memory blocks
    rom_file.read((char*)m_prg_rom.data(), m_prg_rom.size() * sizeof(uint8_t));
    rom_file.read((char*)m_chr_rom.data(), m_chr_rom.size() * sizeof(uint8_t));

    // Initialize the mapper pointer
    uint8_t mapper_number = (m_cart_header.mapper_1 & 0xF0) & ((m_cart_header.mapper_0 & 0x0F) >> 4);
    if (!init_mapper(mapper_number)) {
        std::cout << "Unimplemented mapper type" << std::endl;
    } 

    rom_file.close();
    return true;
}


/* Memory access by CPU ----------------------------------- */

void cpu_WB(uint16_t addr, uint8_t value) {

}

uint8_t cpu_RB(uint16_t addr) {
    return 0x00;
}


/* Memory access by PPU ----------------------------------- */

void ppu_WB(uint16_t addr, uint8_t value) {

}

uint8_t ppu_RB(uint16_t addr) {
    return 0x00;
}
