#include <assert.h>
#include "cart/mapper.hh"

#include <iostream>
    
void Mapper_000::cpu_WB(uint16_t addr, uint8_t value) {

}

uint8_t Mapper_000::cpu_RB(uint16_t addr) {

    // PRG ROM
    if (addr >= 0x8000 && addr <= 0xFFFF) {

        // Two banks
        if (m_size_prg_rom == 0x8000) {
            
            int index = addr - 0x8000;
            assert(index >= 0x0000 && index <= 0x7FFF);
            return m_cart->get_PRG_ROM()[index];

        }

        // One bank - mirrored twice
        else if (m_size_prg_rom == 0x4000) {

            int index = (addr - 0x8000) & 0x3FFF;
            assert(index >= 0x0000 && index <= 0x3FFF);
            return m_cart->get_PRG_ROM()[index];

        }

        // This is either rare or does not exist
        else assert(false);

    }

    return 0x00;
}

void Mapper_000::ppu_WB(uint16_t addr, uint8_t value) {

    // CHR ROM
    if (addr >= 0x0000 && addr <= 0x1FFF) {

        // An assert really isn't needed here so
        m_cart->get_CHR_ROM()[addr] = value;
    
    }

}

uint8_t Mapper_000::ppu_RB(uint16_t addr) {

    // CHR ROM
    if (addr >= 0x0000 && addr <= 0x1FFF) {

        // An assert really isn't needed here so
        return m_cart->get_CHR_ROM()[addr];
    
    }

    return 0x00;
}

// Return name table mirroring mode
ntMirrors::nameTableMirrorMode Mapper_000::nt_mirror() {

    return ntMirrors::vertical; // This should never be used

}

void Mapper_000::rst() {
    // This doesn't really need to do anything
}
