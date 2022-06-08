#pragma once

// Uncomment to enable comparing with execution log
// #define DEBUG_2A03

#include <cstdint>
#include <iostream> // Only used for debugging
#include <fstream>  // Only used for debugging
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
            bool m_flag_c  : 1; // Carry bit
            bool m_flag_z  : 1; // Zero
            bool m_flag_i  : 1; // Disable interrupts
            bool m_flag_d  : 1; // Decimal mode
            bool m_flag_b  : 1; // Break
            bool m_unused  : 1;
            bool m_flag_v  : 1; // Overflow
            bool m_flag_n  : 1; // Negative
        };
    };
    // Program counter is 16 bits
    uint16_t m_reg_pc;

    // Memory access to the cpu buslines
    void WB(uint16_t addr, uint8_t value);
    uint8_t RB(uint16_t addr);

    /* Instructions and addressing modes ------------------ */

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
    template<AddrModes, Operations> uint8_t ins();

    /* Interrupts ----------------------------------------- */

    // Flags for interrupt checking, will be checked and reset after
    //      every instruction
    bool m_irq_requested, m_nmi_requested;

    // A function to service either an nmi or irq based on the address
    //      passed indicating where to fetch the handler address
    void do_interrupt(uint16_t addr);

public:

    Ricoh2A03();

    // Debug utilities
    #ifdef DEBUG_2A03
    void debug_print_state();
    void compare_with_log();
    #endif

    // Connect components
    void connect_bus(cpu_bus* cpu_bus_ptr);

    // External signals
    void irq(); // Maskable interrupt signal
    void nmi(); // Non-maskable interrupt signal
    void rst(); // Reset signal

    // Drives the emulation
    uint8_t step();

};
