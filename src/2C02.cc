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

    m_buf_pos = 0;
    m_io_db = 0x00;

    // Allocate memory for sprite attribute memories
    m_spr_ram = std::make_unique<uint8_t[]>(0x0100);
    for (int i = 0; i < 8; i++) m_spr_buf[i] = std::make_shared<Sprite>();
    m_spr_buf_count = 0;

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

    const int scanline_length = 341;

    // Keep track of old scanline and cycle to detect wrap arounds
    int old_cycle = m_cycle;

    // Move things along
    ++m_cycle %= scanline_length; // Increment cycle, wrap to zero after 340
    if (OVERFLOW(old_cycle, m_cycle)) ++m_scanline;

    // Do what needs to be done for the current state - handles next state logic too

    switch (m_curstate) {
        
        case prerender: {

            // This bit is cleared at this specific dot
            if (m_cycle == 1) m_reg_status.sprite_0_hit = false;
            
            // Move into sprite prefetch to fetch sprite data for 
            //      the first scanline
            if (m_cycle == TV_W) {
                m_curstate = sprPrefetch;
                m_spr_buf_count = 0;
            }
            assert(m_scanline == -1); // Just to be safe

        } break;
        
        case postrender: {
            
            // Move into VBlank
            if (m_scanline == 241) {
                if (m_reg_ctrl1.nmi_on_vblank) m_cpu_bus->nmi(); // NMI if $2000 bit 7 set
                m_reg_status.vblank_occuring = true;               // Set bit 7 of $2002
                m_curstate = vBlank;
            }

        } break;
        
        case rendering: {

            // Render a single background pixel if its enabled, black otherwise
            m_framebuf[m_buf_pos] = m_reg_ctrl2.show_bg ? fetch_bg_pixel() : 0xFF000000;

            // Move buffer position along, wrap back around once it falls off the edge of the buffer
            ++m_buf_pos %= (TV_W * TV_H);

            // Move into sprite Prefetch to get sprite data for next scanline
            if (m_cycle == TV_W) {
                m_curstate = sprPrefetch;
            }
            
        }  break;
        
        case sprPrefetch: {

            const uint8_t attr_mask = ~0x1C; // Mask to pull unimplemented bits of attr byte low
            const uint8_t y_offset = 0, index_offset = 1, attr_offset = 2, x_offset = 3;
            if (m_cycle == 257) { // Fetching all data on this cycle for simplicity

                if (m_reg_ctrl2.show_spries)
                    // Render sprites in reverse order to ensure highest priority sprites (meaning lowest base address) are 
                    //      rendered on top of lower priority sprites (meaning higher base address)
                    for (int i = m_spr_buf_count-1; i >= 0; i--) 
                        render_sprite(*m_spr_buf[i]);
                m_spr_buf_count = 0;

                // Fetching all data on this exact cycle for simplicity
                for (uint16_t cur_sprite_addr = 0; (cur_sprite_addr < 0x100) && (m_spr_buf_count < 8); cur_sprite_addr += 4) {
                    uint8_t x = m_spr_ram[cur_sprite_addr + x_offset], y = m_spr_ram[cur_sprite_addr + y_offset];
                    if (x + 8 > 0 && x < TV_W && y + 8 > m_scanline && y <= m_scanline /* TODO: Consider variable height sprites */) {
                        
                        m_spr_buf[m_spr_buf_count].get()->y_pos = m_spr_ram[cur_sprite_addr + y_offset];
                        m_spr_buf[m_spr_buf_count].get()->index = m_spr_ram[cur_sprite_addr + index_offset];
                        // Supposedly, some unused attribute bits read zero, so I'm just masking them out entirely here
                        m_spr_buf[m_spr_buf_count].get()->attr  = m_spr_ram[cur_sprite_addr + attr_offset] & attr_mask;
                        m_spr_buf[m_spr_buf_count].get()->x_pos = m_spr_ram[cur_sprite_addr + x_offset];
                        
                        // This priority will be compared against overlapping sprites to determine which sprite should
                        //      be rendered on top of another. Lower values have a higher priority. 
                        m_spr_buf[m_spr_buf_count].get()->render_priority = cur_sprite_addr & 0xFF;
                        ++m_spr_buf_count;
                    } 
                }
                assert(m_spr_buf_count >= 0 && m_spr_buf_count <= 8);
            }
            
            // Move to HBlank, or what would normally be background prefetch with an additional
            //      five clock cycles where it basically sits idle
            if (m_cycle == 320) m_curstate = hBlank;
            
        } break;
        
        case hBlank: {

            // Move into post render or to start of another visible scanline
            if (m_scanline == 240) m_curstate = postrender;
            
            else if (m_cycle == 0) { 
                m_curstate = rendering;
            }
            
        } break;
        
        case vBlank: {

            // Move into pre render scanline
            if (m_scanline == 261) {
                m_curstate = prerender;
                m_scanline = -1;
                // Render the completed frame
                m_frameIncompete = false;
            }
            
        } break;
    }

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

/* Render a single pixel ---------------------------------- */

unsigned int Ricoh2C02::fetch_bg_pixel() {

    const uint16_t ntMemBaseAddress = 0x2000;
    const uint16_t attrMemOffset    = 0x03C0;
    const uint16_t iPalBaseAddress  = 0x3F00;

    const int nametableRows  = 32;
    const int tileSizePixels = 8;
    const int tileSizeBytes  = 16;

    // When this bit is low BG within the 8 left most pixels is the BG color
    if ((!m_reg_ctrl2.clip_bg) && (m_cycle < 8)) return g_pal_data[RB(0x3F00)];

    // The cycle variable has been incremented prior to the calling of this function, so use this for the x position
    //      on the screen to prevent everything from accidentally being shifted one pixel.
    int dot = m_cycle - 1;

    int nt_index_x = m_reg_ctrl1.nt_address & 1, nt_index_y = (m_reg_ctrl1.nt_address & 2) >> 1;
    int scrolled_x = dot + m_scroll_latch.scrollX, scrolled_y = m_scanline + m_scroll_latch.scrollY;

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
    uint16_t nameTableBase  = ((nt_index_x*0x400)+(nt_index_y*0x800))+ntMemBaseAddress;
    uint16_t bgPatTableAddr = m_reg_ctrl1.bg_pattabl ? 0x1000 : 0x0000;

    // Obtain the tile index and attribute byte from the name table, also calculate tile base address
    uint16_t tileIndex    = RB(nameTableBase + tile_x + (tile_y * nametableRows));
    uint16_t tileBaseAddr = (tileIndex * tileSizeBytes) + bgPatTableAddr; // In pattern memory
    uint16_t attrBaseAddr = nameTableBase + attrMemOffset + ((tile_x / 4) + ((tile_y / 4) * 8));

    // Read the actual tile data bytes and extract the low bits of the color index
    uint8_t tileDataLo = RB(tileBaseAddr + mod_y + 0), tileDataHi = RB(tileBaseAddr + mod_y + 8);
    uint8_t colorIndex = ((tileDataLo >> (7 - mod_x)) & 1) | (((tileDataHi >> (7 - mod_x)) & 1) << 1);

    // Extract the high bits of the color index
    switch (((tile_x / 2) % 2) + (((tile_y / 2) % 2) * 2)) {
        case 0: colorIndex |= (RB(attrBaseAddr) & 0x03) << 2; break;
        case 1: colorIndex |= (RB(attrBaseAddr) & 0x0C) << 0; break;
        case 2: colorIndex |= (RB(attrBaseAddr) & 0x30) >> 2; break;
        case 3: colorIndex |= (RB(attrBaseAddr) & 0xC0) >> 4; break;
    }

    // Calculate the address at which the global color palette index is stored
    uint16_t colorAddress = iPalBaseAddress + colorIndex;
    assert(colorAddress >= 0x3F00 && colorAddress < 0x3F10);
    
    // This is my lazy ass way of keeping track of whether a background pixel is rendered or not.
    //      This will not be noticable, and its convenient for handling sprite priorities.
    unsigned int alpha_mask = ((colorAddress & 0x3) == 0x0) ?
        0xFEFFFFFF /*BG*/ : 0xFFFFFFFF /*not BG*/;
    
    return g_pal_data[RB(colorAddress)] & alpha_mask;
}

#define IS_BG(color) \
    (bool)(color & 0x01000000) // Abusing alpha bits to keep track of BG, not stupid if it works
void Ricoh2C02::render_sprite(const Sprite& spr) {

    const int tileSizePixels = 8, tileSizeBytes = 16;
    const uint16_t patternTableBase = m_reg_ctrl1.sprite_pattabl ? 0x1000 : 0x0000;
    const uint16_t sPalBaseAddress = 0x3F10;
    
    // The minus one here is because the nes stores the real y position plus one
    uint8_t offset_y = m_scanline - spr.y_pos - 1;

    // Check for vertical flip, flip vertically if bit set
    if (spr.attr & 0x80) offset_y = 7 - offset_y;

    // Fetch sprite pattern data
    uint16_t tileBaseAddr = patternTableBase + (spr.index * tileSizeBytes); // In pattern memory
    uint16_t tileDataLo = RB(tileBaseAddr + offset_y + 0), tileDataHi = RB(tileBaseAddr + offset_y + 8);

    for (int i = 0; i < tileSizePixels; i++) {

        // Don't render if sprite clipping enabled and within 8 leftmost pixels of screen
        if ((!m_reg_ctrl2.clip_sprites && spr.x_pos + i < 8)) 
            continue; // Do not render this sprite pixel

        // Calculate the two least significant bits of the color index into the global palette
        uint8_t colorIndex = (spr.attr & 0x40) ? // This specific bit determines horizontal flip
            ((tileDataLo >> i) & 1) | (((tileDataHi >> i) & 1) << 1): // Horizontal sprite flip
            ((tileDataLo >> (7 - i)) & 1) | (((tileDataHi >> (7 - i)) & 1) << 1);
        
        // Determine if the pixel being rendered to is a BG pixel or not, if so render accordingly
        //      depeinding on the sprite's render priority
        if ((spr.attr & 0x20) && IS_BG(m_framebuf[(m_scanline * TV_W) + spr.x_pos + i]))
            continue; // Do not render this sprite pixel

        // Color index zero is just ignored to my understanding. Draw nothing in this case
        if (colorIndex != 0) m_framebuf[(m_scanline * TV_W) + spr.x_pos + i] = 
            g_pal_data[RB((colorIndex | ((spr.attr & 3) << 2)) + sPalBaseAddress)];
    }
    
}
#undef IS_BG


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

