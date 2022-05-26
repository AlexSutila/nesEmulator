#include "memory.hh"

void Busline::map_io_to_virtual(IORegister* reg, uint16_t addr) {
    // Insert address into the hash map
    io_map[addr] = reg;
}

/* Address calculation to conserve memory */

// To avoid polluting the hash maps with a hundred different addresses that are mapped to the same
//      io register, this can be used to translate all io register mappings so that a single address
//      represents each io register to conserve space.
inline uint16_t remap_addr(uint16_t addr) {
    return (addr < 0x4000) ? 0x2000 | (addr % 0x8) : addr;
}

/* Read and write functions */

void Busline::write(uint16_t addr, uint8_t value) {
    // RAM access
    /**/ if (addr < 0x2000) {
        ram[value & 0x7FF] = value;
    }
    // IO register access
    else if (addr < 0x4018) {
        io_map[remap_addr(addr)]->write(value);
    }
    // Cartridge access
}

uint8_t Busline::read(uint16_t addr) {
    // RAM access
    /**/ if (addr < 0x2000) {
        return ram[addr & 0x7FF];
    }
    // IO register access
    else if (addr < 0x4018) {
        return io_map[remap_addr(addr)]->read();
    }
    // Cartridge access
    else return 0x00;
}

/* Synchronize all components on the bus */

void Busline::step(uint8_t cycles) {



}
