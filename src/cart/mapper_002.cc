#include <cassert>
#include "cart/mapper.hh"
#include "mirrors.hh"

void Mapper_002::cpu_WB(uint16_t addr, uint8_t value) {

    // Write to low bank, only 4 bits
    if (addr >= 0x8000 && addr <= 0xFFFF) {
        m_prg_bank_lo = value & 0x0F;
    }

}

uint8_t Mapper_002::cpu_RB(uint16_t addr) {

    uint8_t data = 0x00;

    // Low bank
    if (addr >= 0x8000 && addr <= 0xBFFF) {
        data = m_cart->get_PRG_ROM()[(addr & 0x3FFF) + (0x4000 * m_prg_bank_lo)];
    }

    // Fixed high bank
    else if (addr >= 0xC000 && addr <= 0xFFFF) {
        data = m_cart->get_PRG_ROM()[(addr & 0x3FFF) + (0x4000 * m_prg_bank_hi)];
    }

    return data;
}

void Mapper_002::ppu_WB(uint16_t addr, uint8_t value) {

    // CHR ROM - treated like ram when nr_chr_banks == 0
    if (m_chr_banks == 0 && addr >= 0x0000 && addr <= 0x1FFF) {
        m_cart->get_CHR_ROM()[addr] = value;
    }

}

uint8_t Mapper_002::ppu_RB(uint16_t addr) {

    uint8_t data = 0x00;

    if (addr >= 0x0000 && addr <= 0x1FFF) {
        data = m_cart->get_CHR_ROM()[addr];
    }

    return data;
}

ntMirrors::nameTableMirrorMode Mapper_002::nt_mirror() {

    return m_mirroring;

}

void Mapper_002::rst() {

    m_prg_bank_lo = 0x00;

    // This shouldn't change, update it anyway cuz why not
    m_prg_bank_hi = m_prg_banks - 1;
}

