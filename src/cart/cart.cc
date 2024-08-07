#include "cart/cart.hh"
#include <iostream>
#include <fstream>

// Initialize the mapper pointer with a pointer to the cartridges respective mapper. Will
//      be called within load_rom
bool Cart::init_mapper(int mapper_number) {

    switch (mapper_number) {

        // Factory design pattern - balls are sore yah
        case 0: m_mapper = std::make_unique<Mapper_000>(
            this,
            m_cart_header.size_prg_rom * 0x4000,
            m_cart_header.size_chr_rom * 0x2000,
            m_cart_header.size_prg_ram * 0x2000); 
            return true;
        case 1: m_mapper = std::make_unique<Mapper_001>(
            this,
            m_cart_header.size_prg_rom * 0x4000,
            m_cart_header.size_chr_rom * 0x2000,
            m_cart_header.size_prg_ram * 0x2000);
            return true;
        case 2: m_mapper = std::make_unique<Mapper_002>(
            this,
            m_cart_header.size_prg_rom,
            m_cart_header.size_chr_rom,
            m_cart_header.mapper_0 & 0x3);

            // When size_chr_rom is zero, chr_rom is actually replaced with RAM so the vector
            //     will need to be resized accordingly.
            if (m_cart_header.size_chr_rom == 0)
                m_chr_rom.resize(0x2000);

            return true;
    }
    
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
    if ((m_cart_header.mapper_0 & 0x4) != 0x0) 
        rom_file.seekg(512, rom_file.cur);

    // Assume one page of RAM when this is zero, docs say so 
    if (m_cart_header.size_prg_ram == 0) ++m_cart_header.size_prg_ram;

    // Resize the vectors based on the specifications in the header
    m_prg_rom.resize(0x4000 * (int)m_cart_header.size_prg_rom * sizeof(uint8_t));
    m_chr_rom.resize(0x2000 * (int)m_cart_header.size_chr_rom * sizeof(uint8_t));
    m_prg_ram.resize(0x2000 * (int)m_cart_header.size_prg_ram * sizeof(uint8_t));

    // Copy the rom to the memory blocks
    rom_file.read((char*)m_prg_rom.data(), m_prg_rom.size() * sizeof(uint8_t));
    rom_file.read((char*)m_chr_rom.data(), m_chr_rom.size() * sizeof(uint8_t));

    // Initialize the mapper pointer
    uint8_t mapper_number = (m_cart_header.mapper_1 & 0xF0) | ((m_cart_header.mapper_0 & 0xF0) >> 4);
    if (!init_mapper(mapper_number)) {
        std::cout << "Unimplemented mapper type: " << (int)mapper_number << std::endl;
        return false;
    } 

    rom_file.close();
    return true;
}


/* Memory access by CPU ----------------------------------- */

void Cart::cpu_WB(uint16_t addr, uint8_t value) {
    m_mapper->cpu_WB(addr, value);
}

uint8_t Cart::cpu_RB(uint16_t addr) {
    return m_mapper->cpu_RB(addr);
}


/* Memory access by PPU ----------------------------------- */

void Cart::ppu_WB(uint16_t addr, uint8_t value) {
    m_mapper->ppu_WB(addr, value);
}

uint8_t Cart::ppu_RB(uint16_t addr) {
    return m_mapper->ppu_RB(addr);
}


/* Getters ------------------------------------------------ */

uint8_t* Cart::get_PRG_ROM() {
    return m_prg_rom.data();
}

uint8_t* Cart::get_CHR_ROM() {
    return m_chr_rom.data();
}

uint8_t* Cart::get_PRG_RAM() {
    return m_prg_ram.data();
}


/* Return the mirroring mode being used ------------------- */

ntMirrors::nameTableMirrorMode Cart::nt_mirror() {

    // This bit being high indicates that bit zero (which normally dictates between
    //      horizontal or vertical being pyisically soldered so that its always used)
    //      should be used to determine mirroring.
    //      ...
    // Mirroring mode ignores this bit and is mapper based other wise
    if ((m_cart_header.mapper_0 & 0x08) == 0x00)
        return (m_cart_header.mapper_0 & 0x01) ?
            ntMirrors::vertical  : 
            ntMirrors::horizontal;

    else return m_mapper->nt_mirror();

}


/* Reset signal to put cartridge in initial conditions ---- */

void Cart::rst() {
    m_mapper->rst();
}
