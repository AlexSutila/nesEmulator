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

}

/* Conecct controllers ------------------------------------ */

void cpu_bus::connect_ctrl(Controller* ctrl_ptr) {
    m_ctrl = ctrl_ptr;

    m_io_writes[0x4016] = [](cpu_bus& t, uint8_t value) { t.m_ctrl->w_joypad(value); };
     m_io_reads[0x4016] = [](cpu_bus& t) { return t.m_ctrl->r_joypad(); };

}


/* Connect components ------------------------------------- */

void cpu_bus::connect_cpu(Ricoh2A03* cpu_ptr) {
    m_cpu = cpu_ptr;
}

void cpu_bus::connect_ppu(Ricoh2C02* ppu_ptr) {
    m_ppu = ppu_ptr;

    /* Map MMIO registers to respective addresses */
    
    //                                                ctrl1 - Mapped to memory address 0x2000
    m_io_writes[0x2000] = [](cpu_bus& t, uint8_t value) { t.m_ppu->ctrl1_w(value); };
     m_io_reads[0x2000] = [](cpu_bus& t) { return t.m_ppu->open_bus_r(); };
    //                                                crtl2 - Mapped to memory address 0x2001
    m_io_writes[0x2001] = [](cpu_bus& t, uint8_t value) { t.m_ppu->ctrl2_w(value); };
     m_io_reads[0x2001] = [](cpu_bus& t) { return t.m_ppu->open_bus_r(); };
    //                                               status - Mapped to memory address 0x2002
    m_io_writes[0x2002] = [](cpu_bus& t, uint8_t value) { t.m_ppu->status_w(value); };
     m_io_reads[0x2002] = [](cpu_bus& t) { return t.m_ppu->status_r(); };
    //                                             spr_addr - Mapped to memory address 0x2003
    m_io_writes[0x2003] = [](cpu_bus& t, uint8_t value) { t.m_ppu->spr_addr_w(value); };
     m_io_reads[0x2003] = [](cpu_bus& t) { return t.m_ppu->open_bus_r(); };
    //                                               spr_io - Mapped to memory address 0x2004
    m_io_writes[0x2004] = [](cpu_bus& t, uint8_t value) { t.m_ppu->spr_io_w(value); };
     m_io_reads[0x2004] = [](cpu_bus& t) { return t.m_ppu->spr_io_r(); };
    //                                           vram_addr1 - Mapped to memory address 0x2005
    m_io_writes[0x2005] = [](cpu_bus& t, uint8_t value) { t.m_ppu->vram_addr1_w(value); };
     m_io_reads[0x2005] = [](cpu_bus& t) { return t.m_ppu->open_bus_r(); };
    //                                           vram_addr2 - Mapped to memory address 0x2006
    m_io_writes[0x2006] = [](cpu_bus& t, uint8_t value) { t.m_ppu->vram_addr2_w(value); };
     m_io_reads[0x2006] = [](cpu_bus& t) { return t.m_ppu->open_bus_r(); };
    //                                              vram_io - Mapped to memory address 0x2007
    m_io_writes[0x2007] = [](cpu_bus& t, uint8_t value) { t.m_ppu->vram_io_w(value); };
     m_io_reads[0x2007] = [](cpu_bus& t) { return t.m_ppu->vram_io_r(); };
    //                                              oam_dma - Mapped to memory address 0x4014
    m_io_writes[0x4014] = [](cpu_bus& t, uint8_t value) { t.m_ppu->oam_dma_w(value, t.m_elapsed_clocks); };
     m_io_reads[0x4014] = [](cpu_bus& t) { return t.m_ppu->open_bus_r(); };

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
        void(*write_function)(cpu_bus&, uint8_t) = m_io_writes[reduced_addr];
        if (write_function != nullptr) (*write_function)(*this, value);
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
        uint8_t(*read_function)(cpu_bus&) = m_io_reads[reduced_addr];
        if (read_function != nullptr) return (*read_function)(*this);

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

    /*
        NOTE: It is important that the cartridge is reset first to put the mapper in the correct
            initial conditions before the entry point is fetched from the fixed address.
    */
    m_cart->rst();
    m_cpu->rst();
}

/* Step all components on the bus ------------------------- */

void cpu_bus::step() {

    ++m_elapsed_clocks;
    
    // PPU is clocked at 3x speed
    m_ppu->step(); m_ppu->step(); m_ppu->step();

}


/* -------------------------------------------------------- */
/*                                                          */
/*                         PPU BUS                          */
/*                                                          */
/* -------------------------------------------------------- */

ppu_bus::ppu_bus() {

    // Allocate memory for name tables and palettes
    m_vram = std::make_unique<uint8_t[][0x0400]>(2);
    m_pal  = std::make_unique<uint8_t[]>(0x20);

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

    // Name Tables - Address Range 0x2000 - 0x3F00
    else if (addr >= 0x2000 && addr <= 0x3EFF) {

        ntMirrors::nameTableMirrorMode mode = m_cart->nt_mirror();
        uint16_t reduced_addr = mirror_nametables(addr);

        switch (mode) {

            case ntMirrors::fourScreen: 
                // TODO
                break;
            
            case ntMirrors::horizontal:
                
                if ((reduced_addr & 0xFC00) == 0x2000 || (reduced_addr & 0xFC00) == 0x2400) {
                    m_vram[0][reduced_addr & 0x3FF] = value;
                } else /* reduced_addr & 0xFC00 is 0x2800 or 0x2C00 */ {
                    m_vram[1][reduced_addr & 0x3FF] = value;
                } break;
            
            case ntMirrors::vertical: 
                
                if ((reduced_addr & 0xFC00) == 0x2000 || (reduced_addr & 0xFC00) == 0x2800) {
                    m_vram[0][reduced_addr & 0x3FF] = value;
                } else /* reduced_addr & 0xFC00 is 0x2400 or 0x2C00 */ {
                    m_vram[1][reduced_addr & 0x3FF] = value;
                } break;
            
            case ntMirrors::singleScreenHi: 
            
                m_vram[1][reduced_addr & 0x3FF] = value; 
                break;
            
            case ntMirrors::singleScreenLo: 
                
                m_vram[0][reduced_addr & 0x3FF] = value; 
                break;

        }

    }

    // Palettes - Address range 0x3F00 - 0x4000
    else if (addr >= 0x3F00 && addr <= 0x3FFF) {

        uint16_t reduced_addr = mirror_palettes(addr);
        m_pal[reduced_addr] = value;

    }

}

uint8_t ppu_bus::RB(uint16_t addr) {

    using namespace AddressMirrors::PpuBus;

    // Reduce address to lower quarter of address range
    addr = mirror_mirrors(addr);

    // Pattern Tables - Address Range 0x0000 - 0x2000
    /**/ if (addr >= 0x0000 && addr <= 0x1FFF) {
        return m_cart->ppu_RB(addr);
    }

    // Name Tables - Address Range 0x2000 - 0x3F00
    else if (addr >= 0x2000 && addr <= 0x3EFF) {

        ntMirrors::nameTableMirrorMode mode = m_cart->nt_mirror();
        uint16_t reduced_addr = mirror_nametables(addr);

        switch (mode) {

            case ntMirrors::fourScreen: 
                return 0x00; break; // TODO
            
            case ntMirrors::horizontal:
                
                if ((reduced_addr & 0xFC00) == 0x2000 || (reduced_addr & 0xFC00) == 0x2400) {
                    return m_vram[0][reduced_addr & 0x3FF];
                } else /* reduced_addr & 0xFC00 is 0x2800 or 0x2C00 */ {
                    return m_vram[1][reduced_addr & 0x3FF];
                } break;
            
            case ntMirrors::vertical: 
                
                if ((reduced_addr & 0xFC00) == 0x2000 || (reduced_addr & 0xFC00) == 0x2800) {
                    return m_vram[0][reduced_addr & 0x3FF];
                } else /* reduced_addr & 0xFC00 is 0x2400 or 0x2C00 */ {
                    return m_vram[1][reduced_addr & 0x3FF];
                } break;
            
            case ntMirrors::singleScreenHi: 
            
                return m_vram[1][reduced_addr & 0x3FF]; break;
            
            case ntMirrors::singleScreenLo: 
                
                return m_vram[0][reduced_addr & 0x3FF]; break;

        }

    }

    // Palettes - Address range 0x3F00 - 0x4000
    else if (addr >= 0x3F00 && addr <= 0x3FFF) {

        uint16_t reduced_addr = mirror_palettes(addr);
        return m_pal[reduced_addr];

    }

    return 0x00;
}
