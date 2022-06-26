#pragma once
#include <memory>
#include <array>
#include <cart/cart.hh>
#include "memory.hh"

#ifdef DEBUG
#include "debug/debug.hh"
#include <fstream>
#endif

struct cpu_bus;
struct ppu_bus;

// Defines screen resolution in pixels
#define TV_W 256
#define TV_H 240

struct Ricoh2C02 {

private:

    // Pointer to frame buffer containing pixel data
    std::shared_ptr<unsigned int[]> m_framebuf;
    int m_buf_pos;    // Current position in frame buffer during a frame

    enum ppuState {

        // These are in no particular order, the lookup tables within step
        //      are depended on this ordering, however ...
        prerender   , // Single pre render scanline
        postrender  , // Post visible scanline, before VBlank
        rendering   , // Any visible scanlines while rendering
        sprPrefetch , // Prefetch sprite data for next scanln
        // On a real NES, the hardware does bgPrefetch during this period I'm refering to as 
        //      hBlank. I'm not going to worry about this for the sake of simplicity, plus
        //      as of right now my implementation really just doesn't need it to work. 
        hBlank      , // Any visible scanlines while HBlanking
        vBlank      , // All VBlanking scanlines

    };
    ppuState m_curstate;

    // Representation of internal data bus used for CPU communication, writes fill this "latch"
    //      and bits read also fill the latch with the bits read. Reading write only bits will
    //      read the corresponding latch bits, how ever these latch bits do decay over time.
    //      ...
    // I will not emulate this decay, I don't expect games will rely on this behavior
    uint8_t m_io_db;

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
    } m_reg_ctrl2;            // Mapped to address 0x2001
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
    struct {                  // Helper struct for MMIO 0x2003 and 0x2004
        // Not sure if this needs anything else fancy yet, it looks like this
        //      sorta address only needs to be eight bits
        uint8_t addr;
    } m_oam_latch;
    struct {                  // Helper struct for MMIO 0x2005
        enum LatchState {
            xByte, yByte
        };
        LatchState state;
        uint8_t scrollX, scrollY;
    } m_scroll_latch;
    struct {                  // Helper struct for MMIO 0x2006 and 0x2007
        enum LatchState {
            loByte, hiByte
        };
        LatchState state;
        uint16_t   addr;
        uint8_t    ibuf; // Shorthand for internal buffer
    } m_addr_latch;

    // Keep track of scanline and cycle
    int m_cycle, m_scanline;

    // A representation of the sprites in object attribute memory
    typedef struct {
        uint8_t y_pos;
        uint8_t index;
        union {
            uint8_t attr;
            struct {
                uint8_t color      : 2;
                uint8_t unused     : 3;
                bool    priority   : 1;
                bool    flip_horiz : 1;
                bool    flip_vet   : 1;
            };
        };
        uint8_t x_pos;
        // Dictates whether one sprite is rendered over another, this is an
        //      assistive variable rather than a real memory byte
        uint8_t render_priority;
    } Sprite;

    void render_sprite(const Sprite& spr);
    unsigned int fetch_bg_pixel();

    // Pointer to sprite attribute memory plus buffer for prefetch
    //      data during visible scanlines
    std::unique_ptr<uint8_t[]> m_spr_ram;
    // Due to hardware limitations, this buffer can only contain 
    //      eight sprites
    std::array<std::shared_ptr<Sprite>, 8> m_spr_buf;
    int m_spr_buf_count;
    
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

    uint8_t open_bus_r(); // Some registers are wr_only, and reading from them results in
                          //    reading from open bus. This function cuts code repetition
                          //    because this happens for multiple addresses ... see below

    void      ctrl1_w(uint8_t value); /* Open bus behavior */ // Mapped to address 0x2000
    void      ctrl2_w(uint8_t value); /* Open bus behavior */ // Mapped to address 0x2001
    void     status_w(uint8_t value); uint8_t     status_r(); // Mapped to address 0x2002
    void   spr_addr_w(uint8_t value); /* Open bus behavior */ // Mapped to address 0x2003
    void     spr_io_w(uint8_t value); uint8_t     spr_io_r(); // Mapped to address 0x2004
    void vram_addr1_w(uint8_t value); /* Open bus behavior */ // Mapped to address 0x2005
    void vram_addr2_w(uint8_t value); /* Open bus behavior */ // Mapped to address 0x2006
    void    vram_io_w(uint8_t value); uint8_t    vram_io_r(); // Mapped to address 0x2007
    // Includes cyc parameter for current cpu bus cycle number to determine if its odd or even
    //      as this slightly affects the timing of the CPU suspension
    void    oam_dma_w(uint8_t value, unsigned long long cyc); // Mapped to address 0x4014

};
