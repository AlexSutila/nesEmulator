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

    // Initialize the hash map for mapped IO

}

/* Connect components ------------------------------------- */

void cpu_bus::connect_cpu(Ricoh2A03* cpu_ptr) {
    m_cpu = cpu_ptr;
}

void cpu_bus::connect_ppu(Ricoh2C02* ppu_ptr) {
    m_ppu = ppu_ptr;
}

void cpu_bus::connect_cart(Cart* cart_ptr) {
    m_cart = cart_ptr;
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
    
    // Cart - Address Range 0x4020 - 0xFFFF
    else if (addr >= 0x4020 && addr <= 0xFFFF) {
        m_cart->cpu_WB(addr, value);
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
    
    // Cart - Address Range 0x4020 - 0xFFFF
    else if (addr >= 0x4020 && addr <= 0xFFFF) {
        return m_cart->cpu_RB(addr);
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

void cpu_bus::step() {
    
    // PPU is clocked at 3x speed
    m_ppu->step(); m_ppu->step(); m_ppu->step();

}


/* -------------------------------------------------------- */
/*                                                          */
/*                         PPU BUS                          */
/*                                                          */
/* -------------------------------------------------------- */

ppu_bus::ppu_bus() {

}

/* Connect components ------------------------------------- */

void ppu_bus::connect_cart(Cart* cart_ptr) {
    m_cart = cart_ptr;
}

/* Read from and write to the bus ------------------------- */

/* NOTE: The bottom quarter of the address range appears to be mirrored four times so
   for simplicity, the address passed into the functions below has the necessary bits
   masked to reduce the address to an address in the lower quarter of the address range

   This reduced address is what is used for the rest of the function */

void ppu_bus::WB(uint16_t addr, uint8_t value) {

    using namespace AddressMirrors::PpuBus;

    // Reduce address to lower quarter of address range
    addr = mirror_mirrors(addr);

    // Pattern Tables - Address Range 0x0000 - 0x2000
    /**/ if (addr >= 0x0000 && addr <= 0x1FFF) {
        m_cart->ppu_WB(addr, value);
    }

    // TODO: Rest of PPU memory map

}

uint8_t ppu_bus::RB(uint16_t addr) {

    using namespace AddressMirrors::PpuBus;

    // Reduce address to lower quarter of address range
    addr = mirror_mirrors(addr);

    // Pattern Tables - Address Range 0x0000 - 0x2000
    /**/ if (addr >= 0x0000 && addr <= 0x1FFF) {
        return m_cart->ppu_RB(addr);
    }

    // TODO: Rest of PPU memory map
    return 0x00;
}
