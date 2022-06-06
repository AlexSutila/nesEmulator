#pragma once
#include <memory>
#include <cart/cart.hh>
#include "memory.hh"

struct cpu_bus;
struct ppu_bus;


// Defines screen resolution in pixels
#define TV_W 256
#define TV_H 224


struct Ricoh2C02 {

private:

    // Pointer to frame buffer containing pixel data
    std::shared_ptr<int> m_framebuf;

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
    uint8_t m_reg_ctrl1;      // Mapped to address 0x2000
    uint8_t m_reg_ctrl2;      // Mapped to address 0x2001
    uint8_t m_reg_status;     // Mapped to address 0x2002
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
