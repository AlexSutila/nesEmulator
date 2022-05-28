#include "mirrors.hh"
#include "memory.hh"

/* -------------------------------------------------------- */
/*                                                          */
/*                         CPU BUS                          */
/*                                                          */
/* -------------------------------------------------------- */

cpu_bus::cpu_bus() {

    // Initialize component list
    m_cpu = std::make_shared<Ricoh2A03>(this);
    m_ppu = std::make_shared<Ricoh2C02>(this);

    // Initialize the hash map for mapped IO

}

/* Read from and write to the bus ------------------------- */

void cpu_bus::WB(uint16_t addr, uint8_t value) {

    using namespace AddressMirrors::CpuBus;

    // RAM - Address Range 0x0000 - 0x2000
    /**/ if (addr >= 0x0000 && addr <= 0x1FFF) {
        
    } 
    
    // IO - Address Range 0x2000 - 0x4020
    else if (addr >= 0x2000 && addr <= 0x401F) {

    } 
    
    // Cart - need to read more about this region
    else if (addr >= 0x4020 && addr <= 0xFFFF) {

    }

}

uint8_t cpu_bus::RB(uint16_t addr) {

    using namespace AddressMirrors::CpuBus;

    // RAM - Address Range 0x0000 - 0x2000
    /**/ if (addr >= 0x0000 && addr <= 0x1FFF) {
        
    } 
    
    // IO - Address Range 0x2000 - 0x4020
    else if (addr >= 0x2000 && addr <= 0x401F) {

    } 
    
    // Cart - need to read more about this region
    else if (addr >= 0x4020 && addr <= 0xFFFF) {

    }

    return 0x00;
}

/* External signals --------------------------------------- */

void cpu_bus::irq() {
    m_cpu->irq();
}

void cpu_bus::nmi() {
    m_cpu->nmi();
}

void cpu_bus::rst() {
    m_cpu->rst();
}

/* Step all components on the bus ------------------------- */

void cpu_bus::step(uint8_t cycles) {
    // TODO
}


/* -------------------------------------------------------- */
/*                                                          */
/*                         PPU BUS                          */
/*                                                          */
/* -------------------------------------------------------- */

ppu_bus::ppu_bus() {

}

/* Read from and write to the bus ------------------------- */

/* NOTE: The bottom quarter of the address range appears to be mirrored four times so
   for simplicity, the address passed into the functions below has the necessary bits
   masked to reduce the address to an address in the lower quarter of the address range

   This reduced address is what is used for the rest of the function */

void ppu_bus::WB(uint16_t addr, uint8_t value) {

    using namespace AddressMirrors::PpuBus;

    // Mirrors is just what the docs call the upper 0x4000 - 0x10000
    addr = mirror_mirrors(addr);

    // Patterns - Address Range 0x0000 - 0x2000
    /**/ if (addr >= 0x0000 && addr <= 0x1FFF) {

    }

    // Name Tables - Address Range 0x2000 - 0x3F00
    else if (addr >= 0x2000 && addr <= 0x3EFF) {

    }

    // Palettes - Address Range 0x3F00 - 0x4000
    else if (addr >= 0x3F00 && addr <= 0x3FFF) {

    }

}

uint8_t ppu_bus::RB(uint16_t addr) {

    using namespace AddressMirrors::PpuBus;

    // Mirrors is just what the docs call the upper 0x4000 - 0x10000
    addr = mirror_mirrors(addr);

    // Patterns - Address Range 0x0000 - 0x2000
    /**/ if (addr >= 0x0000 && addr <= 0x1FFF) {

    }

    // Name Tables - Address Range 0x2000 - 0x3F00
    else if (addr >= 0x2000 && addr <= 0x3EFF) {

    }

    // Palettes - Address Range 0x3F00 - 0x4000
    else if (addr >= 0x3F00 && addr <= 0x3FFF) {

    }

    return 0x00;
}
