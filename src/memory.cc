#include "mirrors.hh"
#include "memory.hh"

/* -------------------------------------------------------- */
/*                                                          */
/*                         CPU BUS                          */
/*                                                          */
/* -------------------------------------------------------- */

cpu_bus::cpu_bus() {

    // Allocate memory for CPU ram
    m_ram = std::make_unique<uint8_t[]>(0x0800);

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
        m_ram[mirror_ram(addr)] = value;
    } 
    
    // IO - Address Range 0x2000 - 0x4020
    else if (addr >= 0x2000 && addr <= 0x401F) {

        // Reduce the mirrored address to a single common address
        uint16_t reduced_addr = mirror_io(addr);

        // Pull the function from the hash map and if there is a mapping
        //      jump to the io regsiter write function
        void(cpu_bus::*write_function)(uint8_t) = m_io_writes[reduced_addr];
        if (write_function != nullptr) {
            // ... This is a function call
            (this->*write_function)(value);
        }
    } 
    
    // Cart - need to read more about this region
    else if (addr >= 0x4020 && addr <= 0xFFFF) {
        // TODO:
    }

}

uint8_t cpu_bus::RB(uint16_t addr) {

    using namespace AddressMirrors::CpuBus;

    // RAM - Address Range 0x0000 - 0x2000
    /**/ if (addr >= 0x0000 && addr <= 0x1FFF) {
        return m_ram[mirror_ram(addr)];
    } 
    
    // IO - Address Range 0x2000 - 0x4020
    else if (addr >= 0x2000 && addr <= 0x401F) {
        
        // Reduce the mirrored address to a single common address
        uint16_t reduced_addr = mirror_io(addr);

        // Pull the function from the hash map and if there is a mapping 
        //      jump to the io register read function
        uint8_t(cpu_bus::*read_function)() = m_io_reads[reduced_addr];
        if (read_function != nullptr) {
            // ... This is also a function call
            (this->*read_function)();
        }

    } 
    
    // Cart - need to read more about this region
    else if (addr >= 0x4020 && addr <= 0xFFFF) {
        // TODO:
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

    // Allocate space for all memory blocks on the bus
    m_patterns   = std::make_unique<uint8_t[]>(0x2000);
    m_nametables = std::make_unique<uint8_t[]>(0x1000);
    m_palettes   = std::make_unique<uint8_t[]>(0x0020);

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
        m_patterns[addr] = value;
    }

    // Name Tables - Address Range 0x2000 - 0x3F00
    else if (addr >= 0x2000 && addr <= 0x3EFF) {
        m_nametables[mirror_nametables(addr) - 0x2000] = value;
    }

    // Palettes - Address Range 0x3F00 - 0x4000
    else if (addr >= 0x3F00 && addr <= 0x3FFF) {
        m_palettes[mirror_palettes(addr) - 0x3F00] = value;
    }

}

uint8_t ppu_bus::RB(uint16_t addr) {

    using namespace AddressMirrors::PpuBus;

    // Mirrors is just what the docs call the upper 0x4000 - 0x10000
    addr = mirror_mirrors(addr);

    // Patterns - Address Range 0x0000 - 0x2000
    /**/ if (addr >= 0x0000 && addr <= 0x1FFF) {
        return m_patterns[addr];
    }

    // Name Tables - Address Range 0x2000 - 0x3F00
    else if (addr >= 0x2000 && addr <= 0x3EFF) {
        return m_nametables[mirror_nametables(addr) - 0x2000];
    }

    // Palettes - Address Range 0x3F00 - 0x4000
    else if (addr >= 0x3F00 && addr <= 0x3FFF) {
        return m_palettes[mirror_palettes(addr) - 0x3F00];
    }

    return 0x00;
}
