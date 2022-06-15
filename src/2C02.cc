#include <assert.h>
#include "2C02.hh"

static const unsigned int g_pal_data[64] = {
    /* Physical color palette in ARGB888 format */
    0xFF757575, 0xFF8F1B27, 0xFFAB0000, 0xFF9F0047,  
    0xFF77008F, 0xFF1300AB, 0xFF0000A7, 0xFF000B7F,  
    0xFF002F43, 0xFF004700, 0xFF005100, 0xFF173F00,  
    0xFF5F3F1B, 0xFF000000, 0xFF000000, 0xFF000000,  
    0xFFBCBCBC, 0xFFEF7300, 0xFFEF3B23, 0xFFF30083,  
    0xFFBF00BF, 0xFF5B00E7, 0xFF002BDB, 0xFF0F4FCB,  
    0xFF00738B, 0xFF009700, 0xFF00AB00, 0xFF3B9300,  
    0xFF8B8300, 0xFF000000, 0xFF000000, 0xFF000000,  
    0xFFFFFFFF, 0xFFFFBF3F, 0xFFFF975F, 0xFFFD8BA7, 
    0xFFFF7BF7, 0xFFB777FF, 0xFF6377FF, 0xFF3B9BFF, 
    0xFF3FBFF3, 0xFF13D383, 0xFF4BDF4F, 0xFF98F858, 
    0xFFDBEB00, 0xFF000000, 0xFF000000, 0xFF000000, 
    0xFFFFFFFF, 0xFFFFE7AB, 0xFFFFD7C7, 0xFFFFCBD7, 
    0xFFFFC7FF, 0xFFDBC7FF, 0xFFB3BFFF, 0xFFABDBFF, 
    0xFFA3E7FF, 0xFFA3FFE3, 0xFFBFF3AB, 0xFFCFFFB3, 
    0xFFF3FF9F, 0xFF000000, 0xFF000000, 0xFF000000, 
};

/* -------------------------------------------------------- */

Ricoh2C02::Ricoh2C02() {

    m_cycle = 0; m_scanline = -1;
    m_frameIncompete = true;
    m_curstate = prerender;

    // Create the frame buffer and clear it
    m_framebuf = std::shared_ptr<unsigned int[]>(new unsigned int[TV_W * TV_H]);
    for (int i = 0; i < TV_W * TV_H; i++) m_framebuf[i] = 0;

    m_io_db = 0x00;

    // Allocate memory for sprite attribute memory
    m_spr_ram = std::make_unique<uint8_t[]>(0x0100);

    // Initialize registers
    m_reg_ctrl1.raw  = 0x00;
    m_reg_ctrl2.raw  = 0x00;
    m_reg_status.raw = 0x00;

    // Initialize scroll latch
    m_scroll_latch.state   = m_scroll_latch.xByte;
    m_scroll_latch.scrollX = 0x00;
    m_scroll_latch.scrollY = 0x00;

    // Initialize address latch
    m_addr_latch.state = m_addr_latch.hiByte;
    m_addr_latch.addr  = 0x0000;
    m_addr_latch.ibuf  = 0x00;

}

/* Get frame buffer for rendering */

std::shared_ptr<unsigned int[]> Ricoh2C02::get_buf() {
    return m_framebuf;
};

/* Bus connections */

void Ricoh2C02::connect_bus(cpu_bus* cpu_bus_ptr) {
    m_cpu_bus = cpu_bus_ptr;
}

void Ricoh2C02::connect_bus(ppu_bus* ppu_bus_ptr) {
    m_ppu_bus = ppu_bus_ptr;
}

/* Memory access to the ppu buslines */

void Ricoh2C02::WB(uint16_t addr, uint8_t value) {
    m_ppu_bus->WB(addr, value);
}

uint8_t Ricoh2C02::RB(uint16_t addr) {
    return m_ppu_bus->RB(addr);
}

/* Step the component one cycle */

#define OVERFLOW(old, cur) (old > cur)
void Ricoh2C02::step() {

    typedef void (*func)(Ricoh2C02&); 
    
    // Look up table of function pointers which do what the PPU should
    //      do in its corresponding state. This includes handling next
    //      state logic.
    static const func lookup[6] = {

        // Pre render - Idle Period
        [](Ricoh2C02& t) {

            // Move into sprite prefetch to fetch sprite data for 
            //      the first scanline
            if (t.m_cycle == TV_W) t.m_curstate = sprPrefetch;
            assert(t.m_scanline == -1); // Just to be safe

        },

        // Post render
        [](Ricoh2C02& t) {

            // Move into VBlank
            if (t.m_scanline == 241) {
                if (t.m_reg_ctrl1.nmi_on_vblank) t.m_cpu_bus->nmi(); // NMI if $2000 bit 7 set
                t.m_reg_status.vblank_occuring = true;               // Set bit 7 of $2002
                t.m_curstate = vBlank;
            }

        },

        // Rendering - also a background tile fetch period on the real hardware, I just fetch background
        //      tiles as I need them in this implementation
        [](Ricoh2C02& t) {

            const uint16_t ntMemBaseAddress = 0x2000;
            const uint16_t attrMemOffset    = 0x03C0;
            const uint16_t iPalBaseAddress  = 0x3F00;

            const int nametableRows  = 32;
            const int tileSizePixels = 8;
            const int tileSizeBytes  = 16;

            static int buf_pos = 0;

            // The cycle variable has been incremented prior to the calling of this lambda, so use this for the x position
            //      on the screen to prevent everything from accidentally being shifted one pixel.
            int dot = t.m_cycle - 1;
            
            /* Render a single pixel */

            int nt_index_x = t.m_reg_ctrl1.nt_address & 1, nt_index_y = (t.m_reg_ctrl1.nt_address & 2) >> 1;
            int scrolled_x = dot + t.m_scroll_latch.scrollX, scrolled_y = t.m_scanline + t.m_scroll_latch.scrollY;

            // Name table corssover due to scrolling logic
            if (scrolled_x >= 256) { scrolled_x %= 256; nt_index_x ^= 1; } // Handle cross over into next nametable horizontally
            if (scrolled_y >= 240) { scrolled_y %= 240; nt_index_y ^= 1; } // Handle cross over into next nametable vertically
            // It is my intention that scrolled x and y serve as indices into the name table where as nt_index x and y determine
            //      which name table is being indexed in the current context. 
            assert((scrolled_x<256) && (scrolled_y<240) && (nt_index_x<2) && (nt_index_y<2));

            // Do some tile position calculations
            int tile_y = scrolled_y / tileSizePixels, mod_y = scrolled_y % tileSizePixels;
            int tile_x = scrolled_x / tileSizePixels, mod_x = scrolled_x % tileSizePixels;

            // Calculate base addresses determined by control register bits
            uint16_t nameTableBase   = ((nt_index_x*0x400)+(nt_index_y*0x800))+ntMemBaseAddress;
            // uint16_t sprPatTableAddr = t.m_reg_ctrl1.sprite_pattabl ? 0x1000 : 0x0000;
            uint16_t bgPatTableAddr  = t.m_reg_ctrl1.bg_pattabl     ? 0x1000 : 0x0000;

            // Obtain the tile index and attribute byte from the name table, also calculate tile base address
            uint16_t tileIndex    = t.RB(nameTableBase + tile_x + (tile_y * nametableRows));
            uint16_t tileBaseAddr = (tileIndex * tileSizeBytes) + bgPatTableAddr; // In pattern memory
            uint16_t attrBaseAddr = nameTableBase + attrMemOffset + ((tile_x / 4) + ((tile_y / 4) * 8));

            // Read the actual tile data bytes and extract the low bits of the color index
            uint8_t tileDataLo = t.RB(tileBaseAddr + mod_y + 0), tileDataHi = t.RB(tileBaseAddr + mod_y + 8);
            uint8_t colorIndex = ((tileDataLo >> (7 - mod_x)) & 1) | (((tileDataHi >> (7 - mod_x)) & 1) << 1);

            // Extract the high bits of the color index
            switch (((tile_x / 2) % 2) + (((tile_y / 2) % 2) * 2)) {
                case 0: colorIndex |= (t.RB(attrBaseAddr) & 0x03) << 2; break;
                case 1: colorIndex |= (t.RB(attrBaseAddr) & 0x0C) << 0; break;
                case 2: colorIndex |= (t.RB(attrBaseAddr) & 0x30) >> 2; break;
                case 3: colorIndex |= (t.RB(attrBaseAddr) & 0xC0) >> 4; break;
            }

            // Write the color at the color index to the frame buffer, and move the buffer index along
            t.m_framebuf[buf_pos] = g_pal_data[t.RB(iPalBaseAddress + colorIndex)];
            ++buf_pos %= (TV_W * TV_H);

            // Move into sprite Prefetch to get sprite data for next scanline
            if (t.m_cycle == TV_W) t.m_curstate = sprPrefetch;

        },

        // Sprite Pre-Fetch - always cycles 256 to 319 inclusive
        [](Ricoh2C02& t) {
            
            // Move to HBlank, or what would normally be background prefetch with an additional
            //      five clock cycles where it basically sits idle
            if (t.m_cycle == 320) t.m_curstate = hBlank;

        },

        // HBlank - This actually pre-fetches some tile data for the next scanline, two tiles
        //      to be exact. Again, my implememtation as of now just reads memory as it needs
        //      it so I'm not really going to do much here to maintain simplicity. 
        //      ...
        // These pre-fetch cycles exist to support the background scrolling to my understanding
        [](Ricoh2C02& t) {

            // Move into post render or to start of another visible scanline
            if (t.m_scanline == 240) t.m_curstate = postrender;
            else if (t.m_cycle == 0) t.m_curstate = rendering;

        },

        // VBlank
        [](Ricoh2C02& t) {

            // Move into pre render scanline
            if (t.m_scanline == 261) {
                t.m_curstate = prerender;
                t.m_scanline = -1;
                // Render the completed frame
                t.m_frameIncompete = false;
            }

        },

    };

    const int scanline_length = 341;

    // Keep track of old scanline and cycle to detect wrap arounds
    int old_cycle = m_cycle;

    // Move things along
    ++m_cycle %= scanline_length; // Increment cycle, wrap to zero after 340
    if (OVERFLOW(old_cycle, m_cycle)) ++m_scanline;

    // Do what needs to be done for the current state - handles next state logic too
    (lookup[m_curstate])(*this);

    // Update debug info
    #ifdef DEBUG
    PpuContext ctx = {
        .cycle    = m_cycle    ,
        .scanline = m_scanline ,
    };
    Debugger::get().set_ppu_context(ctx);
    // Check if a bus dump has been invoked, if so dump the ppu bus contents
    if (Debugger::get().m_dump_ppu_bus) {
        
        // Dump PPU bus
        /* This data can be visualized with some python scripts I wrote in the
                testing directory. Be wary, the code is not pretty lmao */
        std::ofstream outfile;
        outfile.open("testing/dumps/ppubusdump.txt", std::ios::out | std::ios::binary);
        for (int i = 0x0000; i <= 0x3FFF; i++) outfile << RB(i);
        outfile.close();
        
        // Dump SPR memory
        outfile.open("testing/dumps/sprramdump.txt", std::ios::out | std::ios::binary);
        for (int i = 0; i <= 0xFF; i++) outfile << m_spr_ram[i];
        outfile.close();
        
        // Set this to false so it doesn't generate upon every step
        Debugger::get().m_dump_ppu_bus = false;

    }
    #endif
    
}
#undef OVERFLOW



/* MMIO functions ----------------------------------------- */

// For write only registers
uint8_t Ricoh2C02::open_bus_r() {

    // This is specificly for write only registers, attempting to read will read
    //      the current bits stored in the latch ...
    // Again, I am not about to emulate data degredation so assume no bit decay
    return m_io_db;

}


void Ricoh2C02::ctrl1_w(uint8_t value) {
    m_reg_ctrl1.raw = value;
    m_io_db         = value; // Update data latch
}
/* This register is write only - call open bus for read */


void Ricoh2C02::ctrl2_w(uint8_t value) {
    m_reg_ctrl2.raw = value;
    m_io_db         = value; // Update data latch
}
/* This register is write only - call open bus for read */


void Ricoh2C02::status_w(uint8_t value) {
    // This register is read only, but the bits written
    //      are still filled in the latch
    m_io_db = value;
}
uint8_t Ricoh2C02::status_r() {
    // Here, only a few bits return stale bus data, docs slightly
    //      conflict about bit 4 ------------- TODO: confirmation
    m_io_db = (m_reg_status.raw & 0xF0) | (m_io_db & 0x0F);
    m_reg_status.vblank_occuring = false; // Reset upon read
    return m_io_db;
}


void Ricoh2C02::spr_addr_w(uint8_t value) {
    m_oam_latch.addr = value;
    m_io_db = value; // Update data latch
}
/* This register is write only - call open bus for read */


void Ricoh2C02::spr_io_w(uint8_t value) {
    m_spr_ram[(m_addr_latch.addr)++] = value;
    m_io_db = value; // Update data latch
}
uint8_t Ricoh2C02::spr_io_r() {
    uint8_t data = m_spr_ram[m_addr_latch.addr];

    // I think this is sufficient but am not certain
    if ((m_curstate != vBlank) && (m_reg_ctrl2.show_bg && m_reg_ctrl2.show_spries)) 
        m_addr_latch.addr++; 

    m_io_db = data; // Update data latch
    return data;
}


void Ricoh2C02::vram_addr1_w(uint8_t value) {

    if (m_scroll_latch.state == m_scroll_latch.xByte) {
        m_scroll_latch.scrollX = value;
        m_scroll_latch.state = m_scroll_latch.yByte;

    } else /* m_scroll_latch.state == yByte */ {
        m_scroll_latch.scrollY = value;
        m_scroll_latch.state = m_scroll_latch.xByte;
    }
    
    m_io_db = value; // Update data latch
}
/* This register is write only - call open bus for read */


void Ricoh2C02::vram_addr2_w(uint8_t value) {

    if (m_addr_latch.state == m_addr_latch.hiByte) {
        m_addr_latch.addr  = (m_addr_latch.addr & 0x00FF) | (value << 8);
        m_addr_latch.state = m_addr_latch.loByte;
    
    } else /* m_addr_latch.state == loByte */ {
        m_addr_latch.addr  = (m_addr_latch.addr & 0xFF00) | value;
        m_addr_latch.state = m_addr_latch.hiByte;
    }

    m_io_db = value; // Update data latch
}
/* This register is write only - call open bus for read */


void Ricoh2C02::vram_io_w(uint8_t value) {

    uint16_t inc = m_reg_ctrl1.addr_increment ? 32 : 1 , 
        old_addr = m_addr_latch.addr;
    m_addr_latch.addr += inc; // Writes increment this address
    
    // Set data at address pre-increment
    WB(old_addr, value);

    m_io_db = value; // Update data latch
}
uint8_t Ricoh2C02::vram_io_r() {

    uint16_t inc = m_reg_ctrl1.addr_increment ? 32 : 1 ,
        old_addr = m_addr_latch.addr;
    m_addr_latch.addr += inc; // Reads also increment this address

    if (old_addr <= 0x3EFF) /* Internal buffer delayed */ {
        uint8_t data = m_addr_latch.ibuf;
        m_addr_latch.ibuf = RB(old_addr);
        return data;
    }
    
    else /* No buffering but buffer is still updated */ {
        // TODO: Verify I'm reading the new internal buffer value from
        //      the correct address. Docs say it comes from the 'nametable 
        //      underneath', and this should read nametable data, what I'm 
        //      doing here is entirely guess work sooooooooo. 
        //      ...
        // Rational - clearing bit 8 places address in nametable region
        m_addr_latch.ibuf = RB(old_addr & 0x3EFF);
        // Desired data is placed on the bus immediately, no read priming
        return RB(old_addr);
    }

}


// Note, DMA is handled entirely within this function including CPU suspension as the
//      bus is clocked as bytes are transfered. This does not include stepping the CPU
void Ricoh2C02::oam_dma_w(uint8_t value, unsigned long long cyc) {

    m_io_db = value; // Update data latch

    // Value written makes up the upper byte of the source address
    uint16_t src_addr = value << 8;
    // Initial wait state cycle to wait for write to complete
    m_cpu_bus->step();
    // Additional cycle before transfer if on an odd CPU cycle
    if ((cyc & 1) != 0) m_cpu_bus->step();
    // Begin transfer, 2 clocks per byte, one for the read and one
    //      for the write - transfer an entire page
    for (uint16_t offset = 0; offset <= 0xFF; offset++) {
        uint8_t data = m_cpu_bus->RB(src_addr + offset); m_cpu_bus->step();
        m_spr_ram[offset] = data;                        m_cpu_bus->step();
    }

}
/* This register is write only - call open bus for read */

