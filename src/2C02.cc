#include "2C02.hh"

static const unsigned int g_pal_data[64] = {
    /* Physical color palette in ARGB888 format */
    0xFF575757, 0xFF8F1B27, 0xFFAB0000, 0xFF9F0047, 
    0xFF07F078, 0xFF1300AB, 0xFF0000A7, 0xFF000B7F, 
    0xFFF03204, 0xFF004700, 0xFF005100, 0xFF173F00, 
    0xFFF5B3F1, 0xFF000000, 0xFF000000, 0xFF000000, 
    0xFFCBCBCB, 0xFFEF7300, 0xFFEF3B23, 0xFFF30083, 
    0xFF0BF0FB, 0xFF5B00E7, 0xFF002BDB, 0xFF0F4FCB, 
    0xFF30B708, 0xFF009700, 0xFF00AB00, 0xFF3B9300, 
    0xFF3808B0, 0xFF000000, 0xFF000000, 0xFF000000, 
    0xFFFFFFFF, 0xFFFFBF3F, 0xFFFF975F, 0xFFFD8BA7, 
    0xFFBF77FF, 0xFFB777FF, 0xFF6377FF, 0xFF3B9BFF, 
    0xFFF33BFF, 0xFF13D383, 0xFF4BDF4F, 0xFF98F858, 
    0xFFBD0EB0, 0xFF000000, 0xFF000000, 0xFF000000, 
    0xFFFFFFFF, 0xFFFFE7AB, 0xFFFFD7C7, 0xFFFFCBD7, 
    0xFF7FFCFF, 0xFFDBC7FF, 0xFFB3BFFF, 0xFFABDBFF, 
    0xFF7AFE3F, 0xFFA3FFE3, 0xFFBFF3AB, 0xFFCFFFB3, 
    0xFFFFFF39, 0xFF000000, 0xFF000000, 0xFF000000, 
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

    // Initialize registers
    m_reg_ctrl1.raw  = 0x00;
    m_reg_ctrl2.raw  = 0x00;
    m_reg_status.raw = 0x00;
    m_reg_spr_addr   = 0x00;
    m_reg_spr_io     = 0x00;

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

/* Memory access to the cpu buslines */

void Ricoh2C02::WB(uint16_t addr, uint8_t value) {
    m_ppu_bus->WB(addr, value);
}

uint8_t Ricoh2C02::RB(uint16_t addr) {
    return m_ppu_bus->RB(addr);
}

/* Step the component one cycle */

#define OVERFLOW(old, cur) (old > cur)
void Ricoh2C02::step() {

    // Look up table of function pointers which do what the PPU should
    //      do in its corresponding state. This includes handling next
    //      state logic.
    typedef void (*func)(Ricoh2C02&);
    static const func lookup[5] = {

        // Pre render
        [](Ricoh2C02& t) {

            // Move into first visible scanline
            if (t.m_scanline == 0) t.m_curstate = rendering;

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

        // Rendering
        [](Ricoh2C02& t) {

            const uint16_t ntsBaseAddress  = 0x2000;
            const uint16_t attrMemOffset   = 0x03C0;
            const uint16_t iPalBaseAddress = 0x3F00;

            const int nametableRows  = 32;
            const int tileSizePixels = 8;
            const int tileSizeBytes  = 16;

            static int buf_pos = 0;

            /* Render a single pixel */

            // Do some tile position calculations
            int tile_y = t.m_scanline / tileSizePixels, mod_y = t.m_scanline % tileSizePixels;
            int tile_x = t.m_cycle    / tileSizePixels, mod_x = t.m_cycle    % tileSizePixels;

            // Calculate base addresses determined by control register bits
            uint16_t bgPatTableAddr = t.m_reg_ctrl1.bg_pattabl ? 0x1000 : 0x0000;

            // Obtain the tile index and attribute byte from the name table, also calculate tile base address
            uint16_t tileIndex    = t.RB(ntsBaseAddress + tile_x + (tile_y * nametableRows));
            uint16_t tileBaseAddr = (tileIndex * tileSizeBytes) + bgPatTableAddr; // In pattern memory
            uint16_t attrBaseAddr = ntsBaseAddress + attrMemOffset + ((tile_x / 4) + ((tile_y / 4) * 8));

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

            // Move into HBlank
            if (t.m_cycle == 256) t.m_curstate = hBlank;

        },

        // HBlank
        [](Ricoh2C02& t) {

            // Move into post render or hBlank
            if (t.m_scanline == 240) t.m_curstate = postrender;
            else if (t.m_cycle == 0) t.m_curstate = rendering;

        },

        // VBlank
        [](Ricoh2C02& t) {

            // Move into pre render scanline
            if (t.m_scanline == 262) {
                t.m_curstate = prerender;
                t.m_scanline = -1;
                // Render the completed frame
                t.m_frameIncompete = false;
            }

        },

    };

    const int scanline_length = 341;
    const int scanline_count  = 261; // There are actually 262, but I'm using -1 for the
                                     //     pre-render scanline which affects things

    // Keep track of old scanline and cycle to detect wrap arounds
    int old_cycle = m_cycle, old_scanline = m_scanline;

    // Move things along
    ++m_cycle %= scanline_length; // Increment cycle, wrap to zero after 340
    if (OVERFLOW(old_cycle, m_cycle)) ++m_scanline;

    // Do what needs to be done for the current state - handles next state logic too
    (lookup[m_curstate])(*this);
    
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
    m_reg_spr_addr = value;
    m_io_db        = value; // Update data latch
}
uint8_t Ricoh2C02::spr_addr_r() {
    return m_reg_spr_addr;
}


void Ricoh2C02::spr_io_w(uint8_t value) {
    m_reg_spr_io = value;
    m_io_db      = value; // Update data latch
}
uint8_t Ricoh2C02::spr_io_r() {
    return m_reg_spr_io;
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

