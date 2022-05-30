#pragma once
#include <cstdint>
#include "cart/cart.hh"

struct Cart;

class Mapper {

    /* Translates a given address, presented to it by the CPU or PPU, to a
       respective index into one of the cartridges memory block */

public:

    // Mapper access by CPU
    virtual void cpu_WB(uint16_t addr, uint8_t value) = 0;
    virtual uint8_t cpu_RB(uint16_t addr) = 0;

    // Mapper access by PPU
    virtual void ppu_WB(uint16_t addr, uint8_t value) = 0;
    virtual uint8_t ppu_RB(uint16_t addr) = 0;

};

/* -------------------------------------------------------- */
/*                                                          */
/*                        Mapper 000                        */
/*                                                          */
/* -------------------------------------------------------- */

class Mapper_000 : public Mapper {

private:

    int m_size_prg_rom, m_size_chr_rom, m_size_prg_ram;
    Cart* m_cart;

public:

    Mapper_000(Cart* cart_ptr, int sz_prg_rom, int sz_chr_rom, int sz_prg_ram) : 
        m_cart(cart_ptr), 
        m_size_prg_rom(sz_prg_rom),
        m_size_chr_rom(sz_chr_rom),
        m_size_prg_ram(sz_prg_ram) {}

    // Mapper access by CPU
    void cpu_WB(uint16_t addr, uint8_t value) override;
    uint8_t cpu_RB(uint16_t addr) /* ----- */ override;

    // Mapper access by PPU
    void ppu_WB(uint16_t addr, uint8_t value) override;
    uint8_t ppu_RB(uint16_t addr) /* ----- */ override;

};
