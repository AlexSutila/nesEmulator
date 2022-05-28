#pragma once
#include <unordered_map>
#include <memory>
#include "2A03.hh"
#include "2C02.hh"

struct Ricoh2A03;
struct Ricoh2C02;

/* CPU bus ------------------------------------------------ */

struct cpu_bus {

private:

    // Pointer to RAM memory block
    std::unique_ptr<uint8_t[]> m_ram;

    // List of pointers to connected components - will expand in time
    std::shared_ptr<Ricoh2A03> m_cpu;
    std::shared_ptr<Ricoh2C02> m_ppu;

    // Hash maps to map memory access functions for IO registers to virtual addresses
    std::unordered_map<uint16_t, void(cpu_bus::*)(uint8_t value)> m_io_writes;
    std::unordered_map<uint16_t, uint8_t(cpu_bus::*)()> m_io_reads;

    // Auxiliary functions to be mapped to virtual addresses in the hash maps above
    //      which call the corresponging IO r/w functions from the respectivecomponent

public:

    cpu_bus();

    // Memory access by CPU
    void WB(uint16_t addr, uint8_t value);
    uint8_t RB(uint16_t addr);

    // External signals
    void irq(); // Signal maskable interrupt to the cpu
    void nmi(); // Signal non-maskable interrupt to the cpu
    void rst(); // Signal reset to the cpu

    // Step all components connected to the bus by a certain number of cycles
    void step(uint8_t cycles);

};


/* PPU bus ------------------------------------------------ */

struct ppu_bus {

private:

    // Pointers to all memory blocks on the bus
    std::unique_ptr<uint8_t[]> m_patterns;
    std::unique_ptr<uint8_t[]> m_nametables;
    std::unique_ptr<uint8_t[]> m_palettes;

public:

    ppu_bus();

    // Memory access by PPU
    void WB(uint16_t addr, uint8_t value);
    uint8_t RB(uint16_t addr);

};
