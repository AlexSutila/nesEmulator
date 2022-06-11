#include "2C02.hh"

static const unsigned int g_pal_data[64] = {
    /* Physical color palette in ARGB888 format */
    0xFF757575, 0xFF271B8F, 0xFF0000AB, 0xFF47009F, 
    0xFF8F0077, 0xFFAB0013, 0xFFA70000, 0xFF7F0B00, 
    0xFF432F00, 0xFF004700, 0xFF005100, 0xFF003F17, 
    0xFF1B3F5F, 0xFF000000, 0xFF000000, 0xFF000000, 
    0xFFBCBCBC, 0xFF0073EF, 0xFF233BEF, 0xFF8300F3, 
    0xFFBF00BF, 0xFFE7005B, 0xFFDB2B00, 0xFFCB4F0F, 
    0xFF8B7300, 0xFF009700, 0xFF00AB00, 0xFF00933B, 
    0xFF00838B, 0xFF000000, 0xFF000000, 0xFF000000, 
    0xFFFFFFFF, 0xFF3FBFFF, 0xFF5F97FF, 0xFFA78BFD, 
    0xFFF77BFF, 0xFFFF77B7, 0xFFFF7763, 0xFFFF9B3B, 
    0xFFF3BF3F, 0xFF83D313, 0xFF4FDF4B, 0xFF58F898, 
    0xFF00EBDB, 0xFF000000, 0xFF000000, 0xFF000000, 
    0xFFFFFFFF, 0xFFABE7FF, 0xFFC7D7FF, 0xFFD7CBFF, 
    0xFFFFC7FF, 0xFFFFC7DB, 0xFFFFBFB3, 0xFFFFDBAB, 
    0xFFFFE7A3, 0xFFE3FFA3, 0xFFABF3BF, 0xFFB3FFCF, 
    0xFF9FFFF3, 0xFF000000, 0xFF000000, 0xFF000000, 
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

            static int buf_pos = 0;

            // Generate static at current screen position, obviously this will be replaced
            //      with actual pixel data when I get to that point so
            t.m_framebuf[buf_pos] = rand() | 0xFF000000;
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

