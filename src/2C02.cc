#include "2C02.hh"

Ricoh2C02::Ricoh2C02() {

    m_cycle = 0; m_scanline = -1;
    m_frameIncompete = true;
    m_curstate = prerender;

    // Create the frame buffer and clear it
    m_framebuf = std::shared_ptr<unsigned int[]>(new unsigned int[TV_W * TV_H]);
    for (int i = 0; i < TV_W * TV_H; i++) m_framebuf[i] = 0;

    // Initialize registers
    m_reg_ctrl1.raw  = 0x00;
    m_reg_ctrl2.raw  = 0x00;
    m_reg_status.raw = 0x00;
    m_reg_spr_addr   = 0x00;
    m_reg_spr_io     = 0x00;
    m_reg_vram_addr1 = 0x00;
    m_reg_vram_addr2 = 0x00;
    m_reg_vram_io    = 0x00;

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

/* MMIO functions - very subject to change */

void Ricoh2C02::ctrl1_w(uint8_t value) {
    m_reg_ctrl1.raw = value;
}
uint8_t Ricoh2C02::ctrl1_r() {
    return m_reg_ctrl1.raw;
}

void Ricoh2C02::ctrl2_w(uint8_t value) {
    m_reg_ctrl2.raw = value;
}
uint8_t Ricoh2C02::ctrl2_r() {
    return m_reg_ctrl2.raw;
}

void Ricoh2C02::status_w(uint8_t value) {
    // Unused bits read the values last written to them supposedly
    //      the other bits should be read only I would think
    m_reg_status.unused = value & 0x0F;
}
uint8_t Ricoh2C02::status_r() {
    uint8_t ret_val = m_reg_status.raw;
    m_reg_status.vblank_occuring = false; // Reset upon read
    return ret_val;
}

void Ricoh2C02::spr_addr_w(uint8_t value) {
    m_reg_spr_addr = value;
}
uint8_t Ricoh2C02::spr_addr_r() {
    return m_reg_spr_addr;
}

void Ricoh2C02::spr_io_w(uint8_t value) {
    m_reg_spr_io = value;
}
uint8_t Ricoh2C02::spr_io_r() {
    return m_reg_spr_io;
}

void Ricoh2C02::vram_addr1_w(uint8_t value) {
    m_reg_vram_addr1 = value;
}
uint8_t Ricoh2C02::vram_addr1_r() {
    return m_reg_vram_addr1;
}

void Ricoh2C02::vram_addr2_w(uint8_t value) {
    m_reg_vram_addr2 = value;
}
uint8_t Ricoh2C02::vram_addr2_r() {
    return m_reg_vram_addr2;
}

void Ricoh2C02::vram_io_w(uint8_t value) {
    m_reg_vram_io = value;
}
uint8_t Ricoh2C02::vram_io_r() {
    return m_reg_vram_io;
}
