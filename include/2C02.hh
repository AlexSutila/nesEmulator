#pragma once
#include <memory>
#include <cart/cart.hh>
#include "memory.hh"

#ifdef DEBUG
#include "debug/debug.hh"
#endif

struct cpu_bus;
struct ppu_bus;

// Defines screen resolution in pixels
#define TV_W 256
#define TV_H 224

struct Ricoh2C02 {

private:

    // Pointer to frame buffer containing pixel data
    std::shared_ptr<unsigned int[]> m_framebuf;

    enum ppuState {

        // These are in no particular order, the lookup tables within step
        //      are depended on this ordering, however ...
        prerender  , // Single pre render scanline
        postrender , // Post visible scanline, before VBlank
        rendering  , // Any visible scanlines while rendering
        hBlank     , // Any visible scanlines while HBlanking
        vBlank     , // All VBlanking scanlines

    };
    ppuState m_curstate;

    // Memory mapped IO registers
    union /* Control register 1 */ {
        uint8_t raw;
        struct {
            uint8_t nt_address       : 2;
            bool    addr_increment   : 1;
            bool    sprite_pattabl   : 1;
            bool    bg_pattabl       : 1;
            bool    sprite_size      : 1;
            bool    master_slave_sel : 1;
            bool    nmi_on_vblank    : 1;
        };
    } m_reg_ctrl1;            // Mapped to address 0x2000
    union /* Control register 2 */ {
        uint8_t raw;
        struct {
            bool    color_or_mono : 1;
            bool    clip_bg       : 1;
            bool    clip_sprites  : 1;
            bool    show_bg       : 1;
            bool    show_spries   : 1;
            uint8_t bg_color      : 3;
        };
    } m_reg_ctrl2;
    // uint8_t m_reg_ctrl2;      // Mapped to address 0x2001
    union /* Status register */ {
        uint8_t raw;
        struct {
            uint8_t unused          : 4;
            bool    wr_vram_ignore  : 1;
            bool    gt8_sprites     : 1;
            bool    sprite_0_hit    : 1;
            bool    vblank_occuring : 1;
        };
    } m_reg_status;           // Mapped to address 0x2002
    uint8_t m_reg_spr_addr;   // Mapped to address 0x2003
    uint8_t m_reg_spr_io;     // Mapped to address 0x2004
    uint8_t m_reg_vram_addr1; // Mapped to address 0x2005
    uint8_t m_reg_vram_addr2; // Mapped to address 0x2006
    uint8_t m_reg_vram_io;    // Mapped to address 0x2007

    // Keep track of scanline and cycle
    int m_cycle, m_scanline;
    
    // A pointer to the PPU busline
    ppu_bus* m_ppu_bus;

    // A pointer to the CPU busline to trigger interrupts
    cpu_bus* m_cpu_bus;

public:

    Ricoh2C02();

    // Used to notify nes.cc to render a frame
    bool m_frameIncompete;

    // Access the frame buffer for rendering
    std::shared_ptr<unsigned int[]> get_buf();

    // Connect components
    void connect_bus(cpu_bus* cpu_bus_ptr);
    void connect_bus(ppu_bus* ppu_bus_ptr);

    // Memory access to the cpu buslines
    void WB(uint16_t addr, uint8_t value);
    uint8_t RB(uint16_t addr);

    // Step the component one cycle
    void step();

    /* MMIO functions ------------------------------------- */

    void      ctrl1_w(uint8_t value); uint8_t      ctrl1_r(); // Mapped to address 0x2000
    void      ctrl2_w(uint8_t value); uint8_t      ctrl2_r(); // Mapped to address 0x2001
    void     status_w(uint8_t value); uint8_t     status_r(); // Mapped to address 0x2002
    void   spr_addr_w(uint8_t value); uint8_t   spr_addr_r(); // Mapped to address 0x2003
    void     spr_io_w(uint8_t value); uint8_t     spr_io_r(); // Mapped to address 0x2004
    void vram_addr1_w(uint8_t value); uint8_t vram_addr1_r(); // Mapped to address 0x2005
    void vram_addr2_w(uint8_t value); uint8_t vram_addr2_r(); // Mapped to address 0x2006
    void    vram_io_w(uint8_t value); uint8_t    vram_io_r(); // Mapped to address 0x2007

};
