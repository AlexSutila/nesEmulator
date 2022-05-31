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

    // Enumerations for operations and addressing modes
    enum AddrModes {
        // 6502 Addressing Modes
        IMP, IMM, ZP0, ZPX, ZPY, REL, 
        ABS, ABX, ABY, IND, IZX, IZY,
    };
    enum Operations {
        // 6502 Instructions / Operations
        ADC, AND, ASL, BCC, BCS, BEQ, BIT, BMI, BNE, BPL, BRK, BVC,
        BVS, CLC, CLD, CLI, CLV, CMP, CPX, CPY, DEC, DEX, DEY, EOR,
        INC, INX, INY, JMP, JSR, LDA, LDX, LDY, LSR, NOP, ORA, PHA,
        PHP, PLA, PLP, ROL, ROR, RTI, RTS, SBC, SEC, SED, SEI, STA,
        STX, STY, TAX, TAY, TSX, TXA, TXS, TYA,
    };

    // A template for instructions, see 2A03.cc for details
    template<AddrModes, Operations> void ins();

public:

    // Connect components
    void connect_bus(cpu_bus* cpu_bus_ptr);

    // External signals
    void irq(); // Maskable interrupt signal
    void nmi(); // Non-maskable interrupt signal
    void rst(); // Reset signal

    // Drives the emulation
    void step();

    // TODO: Instructions and addressing modes

};
