#pragma once
#include <unordered_map>
#include <memory>
#include "2A03.hh"
#include "2C02.hh"
#include "cart/cart.hh"

struct Ricoh2A03;
struct Ricoh2C02;

/* CPU bus ------------------------------------------------ */

struct cpu_bus {

private:

    // Pointer to RAM memory block
    std::unique_ptr<uint8_t[]> m_ram;

    // List of pointers to connected components - will expand in time
    Ricoh2A03* m_cpu;
    Ricoh2C02* m_ppu;

    // A pointer to the cartridge as the CPU will need to access PRGROM
    Cart* m_cart;

    // Hash maps to map memory access functions for IO registers to virtual addresses
    std::unordered_map<uint16_t, void(*)(cpu_bus& t, uint8_t value)> m_io_writes;
    std::unordered_map<uint16_t, uint8_t(*)(cpu_bus& t)> m_io_reads;

public:

    cpu_bus();

    // Just something to keep track of elapsed cycles
    unsigned long long m_elapsed_clocks = 0;

    // Connect components
    void connect_cpu(Ricoh2A03* cpu_ptr);
    void connect_ppu(Ricoh2C02* ppu_ptr);
    void connect_cart(Cart* cart_ptr);

    // Memory access by CPU
    void WB(uint16_t addr, uint8_t value);
    uint8_t RB(uint16_t addr);

    // External signals
    void irq(); // Signal maskable interrupt to the cpu
    void nmi(); // Signal non-maskable interrupt to the cpu
    void rst(); // Signal reset to the cpu

    // Step all components connected to the bus by a certain number of cycles
    void step();

};


/* PPU bus ------------------------------------------------ */

struct ppu_bus {

private:

    // Pointers to PPU memory blocks
    std::unique_ptr<uint8_t[][0x0400]> m_vram; // Two internal 1KiB name tables
    std::unique_ptr<uint8_t[]> m_pal; // -------- Two palettes, image and sprite 

    // A pointer to the cartridge as the PPU will need to read CHRROM
    Cart* m_cart;

public:

    ppu_bus();

    // Functions to connect components
    void connect_cart(Cart* cart_ptr);

    // Memory access by PPU
    void WB(uint16_t addr, uint8_t value);
    uint8_t RB(uint16_t addr);

};
