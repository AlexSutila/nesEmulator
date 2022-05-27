#pragma once
#include <cstdint>
#include <memory>
#include "memory.hh"

struct cpu_bus;
struct ppu_bus;

struct Ricoh2A03 {

private:

    // A pointer to the CPU busline
    cpu_bus* m_bus;

    // CPU Registers - 8 bits
    uint8_t m_reg_a, m_reg_x, m_reg_y, m_reg_s;
    union {
        // Processor status
        uint8_t m_reg_p;
        struct {
            // Individual bits of processor status
            uint8_t m_flag_c  : 1;
            uint8_t m_flag_z  : 1;
            uint8_t m_flag_i  : 1;
            uint8_t m_flag_b  : 1;
            uint8_t m_unused1 : 1;
            uint8_t m_unused2 : 1;
            uint8_t m_flag_v  : 1;
            uint8_t m_flag_n  : 1;
        };
    };
    // Program counter is 16 bits
    uint16_t m_reg_pc;

    // Memory access to the cpu buslines
    void WB(uint16_t addr, uint8_t value);
    uint8_t RB(uint16_t addr);

public:

    Ricoh2A03(cpu_bus* bus_ptr) : m_bus(bus_ptr) {}

    // External signals
    void irq(); // Maskable interrupt signal
    void nmi(); // Non-maskable interrupt signal
    void rst(); // Reset signal

    // Drives the emulation
    void step();

    // TODO: Instructions and addressing modes

};
