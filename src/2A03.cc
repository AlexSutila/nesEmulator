#include "2A03.hh"

Ricoh2A03::Ricoh2A03() {

    // Reset internal interrupt flags
    m_nmi_requested = m_irq_requested = false;

}

/* Busline connections ------------------------------------ */

void Ricoh2A03::connect_bus(cpu_bus* cpu_bus_ptr) {
    m_bus = cpu_bus_ptr;
}

/* Memory access to the cpu buslines ---------------------- */

void Ricoh2A03::WB(uint16_t addr, uint8_t value) {

    // Handle break on address write
    #ifdef DEBUG
    Debugger::get().do_break(addr, wr);
    #endif

    m_bus->WB(addr, value);
}

uint8_t Ricoh2A03::RB(uint16_t addr) {
    
    // Handle break on address read
    #ifdef DEBUG
    Debugger::get().do_break(addr, rd);
    #endif

    return m_bus->RB(addr);
}


/* External signals --------------------------------------- */

void Ricoh2A03::irq() {

    // Indicates that an irq interrupt should occur after the completion
    //      of this instruction
    m_irq_requested = true;

}

void Ricoh2A03::nmi() {

    // Indicates that an nmi interrupt should occur after the completion
    //      of this instruction
    m_nmi_requested = true;

}

void Ricoh2A03::rst() {

    // Reset internal interrupt flags
    m_nmi_requested = m_irq_requested = false;

    // Reset registers
    m_reg_a = 0x00; m_reg_x = 0x00;
    m_reg_y = 0x00; m_reg_s = 0xFD;
    m_reg_p = 0x00;

    // Initialize the PC to entry point
    m_reg_pc = RB(0xFFFC) | (RB(0xFFFD) << 8);

}

/* Interrupt handling ------------------------------------- */

void Ricoh2A03::do_interrupt(uint16_t addr) {

    // Push PC to stack
    WB(0x0100 + m_reg_s--, (m_reg_pc >> 8) & 0xFF);
    WB(0x0100 + m_reg_s--, m_reg_pc & 0xFF);

    // Push status to stack
    m_flag_b = false; m_flag_i = true;
    WB(0x0100 + m_reg_s--, m_reg_s);

    // Jump to fetched jump address
    m_reg_pc  = RB(addr++);
    m_reg_pc |= (RB(addr) << 8);

    // Do execution breakpoint, debugger will skip over any
    //      address at this point if it is not checked here
    #ifdef DEBUG
    Debugger::get().do_break(m_reg_pc, ex);
    #endif

}

/* Drives the emulation ----------------------------------- */

// Will construct a function for a specific instruction given its
//      base cycle count (do not include additional clock cycles),
//      it's addressing mode, and it's operation.
//      ...
// Will be used to construct a jump table that is indexed by the 
//      opcode for CPU instruction execution
// ---------------------------------------------------------------
//      a_m -> addressing mode - from Ricoh2A03 member enum
//      op  -> operation       - from Ricoh2A03 member enum
//  Returns -> uint8_t         - any additional cycles used
// ---------------------------------------------------------------
// I give lots of credit to https://github.com/OneLoneCoder as I 
//      referenced his code often to make this template
template<Ricoh2A03::AddrModes a_m, Ricoh2A03::Operations op>
uint8_t Ricoh2A03::ins() {

    // A boolean flag to determine if an additional cycle
    //      will be added only in some instructions
    bool addrmode_extra_cycle = false;

    // Temporary variables to ease instruction execution
    uint16_t addr_abs = 0x0000, addr_rel = 0x0000, t16 = 0x0000;
    uint8_t t8 = 0, extra_cycles = 0;

    // Do addressing mode
    /**/ if constexpr (a_m == IMP) {
        t8 = m_reg_a;
    }
    else if constexpr (a_m == IMM) {
        addr_abs = m_reg_pc++;
    }
    else if constexpr (a_m == ZP0) {
        addr_abs = RB(m_reg_pc++) & 0x00FF;
    }
    else if constexpr (a_m == ZPX) {
        addr_abs = (RB(m_reg_pc++) + m_reg_x) & 0x00FF;
    }
    else if constexpr (a_m == ZPY) {
        addr_abs = (RB(m_reg_pc++) + m_reg_y) & 0x00FF;
    }
    else if constexpr (a_m == REL) {
        addr_rel = RB(m_reg_pc++);
        if (addr_rel & 0x80) addr_rel |= 0xFF00;
    }
    else if constexpr (a_m == ABS) {
        addr_abs  = RB(m_reg_pc++);
        addr_abs |= (RB(m_reg_pc++) << 8);
    }
    else if constexpr (a_m == ABX) {
        uint8_t lo = RB(m_reg_pc++);
        uint8_t hi = RB(m_reg_pc++);
        addr_abs = ((hi << 8) | lo) + m_reg_x;
        // Specific instructions will check addrmode_extra_cycle to see if it needs the
        //      additional cycle to handle the page cross, other instructions just do
        //      the page cross regardless if its needed or not.
        if ((addr_abs & 0xFF00) != (hi << 8)) 
            addrmode_extra_cycle = true;
    }
    else if constexpr (a_m == ABY) {
        uint8_t lo = RB(m_reg_pc++);
        uint8_t hi = RB(m_reg_pc++);
        addr_abs = ((hi << 8) | lo) + m_reg_y;
        // Same rational as ABX
        if ((addr_abs & 0xFF00) != (hi << 8))
            addrmode_extra_cycle = true;
    }
    else if constexpr (a_m == IND) {
        uint16_t rd_addr = RB(m_reg_pc++);
        rd_addr |=  (RB(m_reg_pc++) << 8);
        addr_abs = (rd_addr & 0x00FF) == 0xFF ?
            (RB(rd_addr & 0xFF00) << 8) | RB(rd_addr): // Hardware bug on page boundaries
            (RB(rd_addr + 0x0001) << 8) | RB(rd_addr); // Regular behavior
    }
    else if constexpr (a_m == IZX) {
        uint16_t rd_addr = RB(m_reg_pc++);
        addr_abs  = RB((uint16_t)(rd_addr + (uint16_t)m_reg_x    ) & 0x00FF);
        addr_abs |= RB((uint16_t)(rd_addr + (uint16_t)m_reg_x + 1) & 0x00FF) << 8;
    }
    else if constexpr (a_m == IZY) {
        uint16_t rd_addr = RB(m_reg_pc++);
        uint8_t  lo = RB( rd_addr      & 0x00FF);
        uint8_t  hi = RB((rd_addr + 1) & 0x00FF);
        addr_abs = ((hi << 8) | lo) + m_reg_y;
        // Page change potential to add cycle like before
        if ((addr_abs & 0xFF00) != (hi << 8))
            addrmode_extra_cycle = true;
    }

    // Do operation
    /**/ if constexpr (op == ADC) {

        if constexpr (a_m != IMP) t8 = RB(addr_abs);
        t16 = (uint16_t)m_reg_a + (uint16_t)t8 + (uint16_t)m_flag_c;

        m_flag_c = (t16 > 0xFF);
        m_flag_z = (t16 & 0xFF) == 0;
        m_flag_v = (~((uint16_t)m_reg_a^(uint16_t)t8)&((uint16_t)m_reg_a^(uint16_t)t16))&0x0080;
        m_flag_n = t16 & 0x80;

        m_reg_a = t16 & 0xFF;
        if (addrmode_extra_cycle) ++extra_cycles;
    }
    else if constexpr (op == AND) {

        if constexpr (a_m != IMP) t8 = RB(addr_abs);
        m_reg_a &= t8;

        m_flag_z = (m_reg_a == 0);
        m_flag_n = (m_reg_a & 0x80);

        if (addrmode_extra_cycle) ++extra_cycles;

    }
    else if constexpr (op == ASL) {

        if constexpr (a_m != IMP) t8 = RB(addr_abs);
        t16 = (uint16_t)t8 << 1;
        
        m_flag_c = (t16 & 0xFF00) > 0;
        m_flag_z = (t16 & 0x00FF) == 0;
        m_flag_n = (t16 & 0x0080);

        if constexpr (a_m == IMP)
            m_reg_a = t16 & 0x00FF;

        else WB(addr_abs, t16 & 0x00FF);

    }
    else if constexpr (op == BCC) {

        if (!m_flag_c) {
            ++extra_cycles;
            addr_abs = addr_rel + m_reg_pc;

            if ((addr_abs & 0xFF00) != (m_reg_pc & 0xFF00))
                ++extra_cycles;

            m_reg_pc = addr_abs;
        }

    }
    else if constexpr (op == BCS) {

        if (m_flag_c) {
            ++extra_cycles;
            addr_abs = addr_rel + m_reg_pc;

            if ((addr_abs & 0xFF00) != (m_reg_pc & 0xFF00))
                ++extra_cycles;

            m_reg_pc = addr_abs;
        }

    }
    else if constexpr (op == BEQ) {

        if (m_flag_z) {
            ++extra_cycles;
            addr_abs = addr_rel + m_reg_pc;

            if ((addr_abs & 0xFF00) != (m_reg_pc & 0xFF00))
                ++extra_cycles;

            m_reg_pc = addr_abs;
        }

    }
    else if constexpr (op == BIT) {

        if constexpr (a_m != IMP) t8 = RB(addr_abs);
        t16 = m_reg_a & t8;

        m_flag_z = (t16 & 0xFF) == 0;
        m_flag_n = t8 & 0x80;
        m_flag_v = t8 & 0x40;

    }
    else if constexpr (op == BMI) { // I can reduce the code reuse for these branches
                                    //     will likely come back to this

        if (m_flag_n) {
            ++extra_cycles;
            addr_abs = addr_rel + m_reg_pc;

            if ((addr_abs & 0xFF00) != (m_reg_pc & 0xFF00))
                ++extra_cycles;

            m_reg_pc = addr_abs;
        }

    }
    else if constexpr (op == BNE) {

        if (!m_flag_z) {
            ++extra_cycles;
            addr_abs = addr_rel + m_reg_pc;

            if ((addr_abs & 0xFF00) != (m_reg_pc & 0xFF00))
                ++extra_cycles;
            
            m_reg_pc = addr_abs;
        }

    }
    else if constexpr (op == BPL) {

        if (!m_flag_n) {
            ++extra_cycles;
            addr_abs = addr_rel + m_reg_pc;

            if ((addr_abs & 0xFF00) != (m_reg_pc & 0xFF00))
                ++extra_cycles;
            
            m_reg_pc = addr_abs;
        }

    }
    else if constexpr (op == BRK) {

        WB(0x0100 + m_reg_s--, (m_reg_pc >> 8) & 0xFF);
        WB(0x0100 + m_reg_s--, m_reg_pc & 0xFF);
        
        WB(0x100 + m_reg_s--, m_reg_p | 0x30);
        m_flag_b = false; m_flag_i = true;

        m_reg_pc = (uint16_t)RB(0xFFFE) | ((uint16_t)RB(0xFFFF) << 8);

    }
    else if constexpr (op == BVC) {

        if (!m_flag_v) {
            ++extra_cycles;
            addr_abs = addr_rel + m_reg_pc;

            if ((addr_abs & 0xFF00) != (m_reg_pc & 0xFF00))
                ++extra_cycles;
            
            m_reg_pc = addr_abs;
        }

    }
    else if constexpr (op == BVS) {

        if (m_flag_v) {
            ++extra_cycles;
            addr_abs = addr_rel + m_reg_pc;

            if ((addr_abs & 0xFF00) != (m_reg_pc & 0xFF00))
                ++extra_cycles;
            
            m_reg_pc = addr_abs;
        }

    }
    else if constexpr (op == CLC) {

        m_flag_c = false;

    }
    else if constexpr (op == CLD) {

        m_flag_d = false;

    }
    else if constexpr (op == CLI) {

        m_flag_i = false;

    }
    else if constexpr (op == CLV) {

        m_flag_v = false;

    }
    else if constexpr (op == CMP) {

        if constexpr (a_m != IMP) t8 = RB(addr_abs);
        t16 = (uint16_t)m_reg_a - (uint16_t)t8;

        m_flag_c = m_reg_a >= t8;
        m_flag_z = (t16 & 0x00FF) == 0;
        m_flag_n = t16 & 0x0080;

        if (addrmode_extra_cycle) ++extra_cycles;

    }
    else if constexpr (op == CPX) {

        if constexpr (a_m != IMP) t8 = RB(addr_abs);
        t16 = (uint16_t)m_reg_x - (uint16_t)t8;

        m_flag_c = (m_reg_x >= t8);
        m_flag_z = (t16 & 0x00FF) == 0;
        m_flag_n = (t16 & 0x0080);

    }
    else if constexpr (op == CPY) {

        if constexpr (a_m != IMP) t8 = RB(addr_abs);
        t16 = (uint16_t)m_reg_y - (uint16_t)t8;

        m_flag_c = (m_reg_y >= t8);
        m_flag_z = (t16 & 0x00FF) == 0;
        m_flag_n = (t16 & 0x0080);

    }
    else if constexpr (op == DEC) {

        if constexpr (a_m != IMP) t8 = RB(addr_abs);
        t16 = t8 - 1;

        WB(addr_abs, t16 & 0x00FF);
        m_flag_z = (t16 & 0xFF) == 0;
        m_flag_n = t16 & 0x80;

    }
    else if constexpr (op == DEX) {

        m_reg_x = m_reg_x - 1;
        m_flag_z = (m_reg_x == 0x00);
        m_flag_n = (m_reg_x & 0x80);

    }
    else if constexpr (op == DEY) {

        m_reg_y = m_reg_y - 1;
        m_flag_z = (m_reg_y == 0x00);
        m_flag_n = (m_reg_y & 0x80);

    }
    else if constexpr (op == EOR) {

        if constexpr (a_m != IMP) t8 = RB(addr_abs);
        m_reg_a ^= t8;

        m_flag_z = (m_reg_a == 0x00);
        m_flag_n = (m_reg_a & 0x80);

        if (addrmode_extra_cycle) ++extra_cycles;

    }
    else if constexpr (op == INC) {

        if constexpr (a_m != IMP) t8 = RB(addr_abs);
        t16 = t8 + 1;

        WB(addr_abs, t16 & 0x00FF);
        m_flag_z = (t16 & 0xFF) == 0;
        m_flag_n = t16 & 0x80;

    }
    else if constexpr (op == INX) {

        m_reg_x++;
        m_flag_z = (m_reg_x == 0x00);
        m_flag_n = (m_reg_x & 0x80);

    }
    else if constexpr (op == INY) {

        m_reg_y++;
        m_flag_z = (m_reg_y == 0x00);
        m_flag_n = (m_reg_y & 0x80);

    }
    else if constexpr (op == JMP) {

        m_reg_pc = addr_abs;

    }
    else if constexpr (op == JSR) {

        --m_reg_pc;
        WB(0x0100 + m_reg_s--, (m_reg_pc >> 8) & 0xFF);
        WB(0x0100 + m_reg_s--,  m_reg_pc       & 0xFF);
        m_reg_pc = addr_abs;

    }
    else if constexpr (op == LDA) {

        if constexpr (a_m != IMP) t8 = RB(addr_abs);
        m_reg_a = t8;

        m_flag_z = (m_reg_a == 0x00);
        m_flag_n = (m_reg_a & 0x80);

        if (addrmode_extra_cycle) ++extra_cycles;

    }
    else if constexpr (op == LDX) {

        if constexpr (a_m != IMP) t8 = RB(addr_abs);
        m_reg_x = t8;

        m_flag_z = (m_reg_x == 0x00);
        m_flag_n = (m_reg_x & 0x80);

        if (addrmode_extra_cycle) ++extra_cycles;

    }
    else if constexpr (op == LDY) {

        if constexpr (a_m != IMP) t8 = RB(addr_abs);
        m_reg_y = t8;

        m_flag_z = (m_reg_y == 0x00);
        m_flag_n = (m_reg_y & 0x80);

        if (addrmode_extra_cycle) ++extra_cycles;

    }
    else if constexpr (op == LSR) {

        if constexpr (a_m != IMP) t8 = RB(addr_abs);
        t16 = t8 >> 1;
        
        m_flag_c = t8 & 0x01;
        m_flag_z = (t16 & 0x00FF) == 0;
        m_flag_n = (t16 & 0x0080);

        if constexpr (a_m == IMP)
            m_reg_a = t16 & 0x00FF;

        else WB(addr_abs, t16 & 0x00FF);

    }
    else if constexpr (op == NOP) {
        // Do nothing for now - NOP (when considering illegal opcodes)
        //      can have weird timing and I'm not sure how to handle it
        //      quite yet ...
    }
    else if constexpr (op == ORA) {

        if constexpr (a_m != IMP) t8 = RB(addr_abs);
        m_reg_a |= t8;

        m_flag_z = (m_reg_a == 0x00);
        m_flag_n = (m_reg_a & 0x80);

        if (addrmode_extra_cycle) ++extra_cycles;

    }
    else if constexpr (op == PHA) {

        WB(0x0100 + m_reg_s--, m_reg_a);

    }
    else if constexpr (op == PHP) {

        WB(0x0100 + m_reg_s--, m_reg_p | 0x30);
        m_reg_p &= 0xCF;

    }
    else if constexpr (op == PLA) {

        m_reg_a = RB(++m_reg_s + 0x0100);
        m_flag_z = m_reg_a == 0x00;
        m_flag_n = m_reg_a & 0x80;

    }
    else if constexpr (op == PLP) {

        m_reg_p = RB(++m_reg_s + 0x0100) | 0x20;

    }
    else if constexpr (op == ROL) {

        if constexpr (a_m != IMP) t8 = RB(addr_abs);
        t16 = (uint16_t)(t8 << 1) | (uint16_t)m_flag_c;

        m_flag_c = (t16 & 0xFF00);
        m_flag_z = ((t16 & 0x00FF) == 0);
        m_flag_n = (t16 & 0x0080);

        if constexpr (a_m == IMP)
            m_reg_a = t16 & 0x00FF;
        
        else WB(addr_abs, t16 & 0x00FF);
    
    }
    else if constexpr (op == ROR) {

        if constexpr (a_m != IMP) t8 = RB(addr_abs);
        t16 = (uint16_t)(m_flag_c << 7) | (t8 >> 1);

        m_flag_c = t8 & 0x01;
        m_flag_z = (t16 & 0xFF) == 0;
        m_flag_n = t16 & 0x80;

        if constexpr (a_m == IMP)
            m_reg_a = t16 & 0x00FF;

        else WB(addr_abs, t16 & 0x00FF); 

    }
    else if constexpr (op == RTI) {

        m_reg_p = RB(++m_reg_s + 0x0100) & 0xCF;
        m_reg_pc  = (uint16_t)RB(++m_reg_s + 0x0100);
        m_reg_pc |= (uint16_t)RB(++m_reg_s + 0x0100) << 8;

    }
    else if constexpr (op == RTS) {

        m_reg_pc  = (uint16_t)RB(++m_reg_s + 0x0100);
        m_reg_pc |= (uint16_t)RB(++m_reg_s + 0x0100) << 8;
        ++m_reg_pc;

    }
    else if constexpr (op == SBC) {

        if (a_m != IMP) t8 = RB(addr_abs);
        uint16_t val = ((uint16_t)t8) ^ 0x00FF;

        t16 = (uint16_t)m_reg_a + val + (uint16_t)m_flag_c;
        m_flag_c = (t16 & 0xFF00);
        m_flag_z = (t16 & 0x00FF) == 0;
        m_flag_v = (t16 ^ (uint16_t)m_reg_a) & (t16 ^ val) & 0x80;
        m_flag_n = (t16 & 0x0080);
        m_reg_a = t16 & 0xFF;

        if (addrmode_extra_cycle) ++extra_cycles;

    }
    else if constexpr (op == SEC) {

        m_flag_c = true;

    }
    else if constexpr (op == SED) {

        m_flag_d = true;

    }
    else if constexpr (op == SEI) {

        m_flag_i = true;

    }
    else if constexpr (op == STA) {

        WB(addr_abs, m_reg_a);

    }
    else if constexpr (op == STX) {

        WB(addr_abs, m_reg_x);

    }
    else if constexpr (op == STY) {

        WB(addr_abs, m_reg_y);

    }
    else if constexpr (op == TAX) {
        
        m_reg_x = m_reg_a;
        m_flag_z = (m_reg_x == 0x00);
        m_flag_n = (m_reg_x & 0x80);

    }
    else if constexpr (op == TAY) {

        m_reg_y = m_reg_a;
        m_flag_z = (m_reg_y == 0x00);
        m_flag_n = (m_reg_y & 0x80);

    }
    else if constexpr (op == TSX) {

        m_reg_x = m_reg_s;
        m_flag_z = (m_reg_x == 0x00);
        m_flag_n = (m_reg_x & 0x80);

    }
    else if constexpr (op == TXA) {

        m_reg_a = m_reg_x;
        m_flag_z = (m_reg_a == 0x00);
        m_flag_n = (m_reg_a & 0x80);

    }
    else if constexpr (op == TXS) {

        m_reg_s = m_reg_x;

    }
    else if constexpr (op == TYA) {

        m_reg_a = m_reg_y;
        m_flag_z = (m_reg_a == 0x00);
        m_flag_n = (m_reg_a & 0x80);

    }

    return extra_cycles;
}

uint8_t Ricoh2A03::step() {

    typedef struct { 
        uint8_t (Ricoh2A03::*fn)(); // Instruction function function pointer
        uint8_t len;                // Base length of instruction in cycles
    } instruction;
    
    #define a(a_m, op, cyc) \
        { &Ricoh2A03::ins<a_m,op>, cyc }
    // Lookup table of function pointers, indexed by opcode to get 
    //      the instruction to execute ... 
    static const instruction lookup[0x100] = 
    {
    /* ~Dorceless~        0x-0          0x-1          0x-2          0x-3          0x-4          0x-5          0x-6          0x-7          0x-8          0x-9          0x-A          0x-B          0x-C          0x-D          0x-E          0x-F */ 
        /* 0x0- */ a(IMM,BRK,7), a(IZX,ORA,6), a(IMP,NOP,2), a(IMP,NOP,8), a(IMP,NOP,3), a(ZP0,ORA,3), a(ZP0,ASL,5), a(IMP,NOP,5), a(IMP,PHP,3), a(IMM,ORA,2), a(IMP,ASL,2), a(IMP,NOP,2), a(IMP,NOP,4), a(ABS,ORA,4), a(ABS,ASL,6), a(IMP,NOP,6),
        /* 0x1- */ a(REL,BPL,2), a(IZY,ORA,5), a(IMP,NOP,2), a(IMP,NOP,8), a(IMP,NOP,4), a(ZPX,ORA,4), a(ZPX,ASL,6), a(IMP,NOP,6), a(IMP,CLC,2), a(ABY,ORA,4), a(IMP,NOP,2), a(IMP,NOP,7), a(IMP,NOP,4), a(ABX,ORA,4), a(ABX,ASL,7), a(IMP,NOP,7),
        /* 0x2- */ a(ABS,JSR,6), a(IZX,AND,6), a(IMP,NOP,2), a(IMP,NOP,8), a(ZP0,BIT,3), a(ZP0,AND,3), a(ZP0,ROL,5), a(IMP,NOP,5), a(IMP,PLP,4), a(IMM,AND,2), a(IMP,ROL,2), a(IMP,NOP,2), a(ABS,BIT,4), a(ABS,AND,4), a(ABS,ROL,6), a(IMP,NOP,6),
        /* 0x3- */ a(REL,BMI,2), a(IZY,AND,5), a(IMP,NOP,2), a(IMP,NOP,8), a(IMP,NOP,4), a(ZPX,AND,4), a(ZPX,ROL,6), a(IMP,NOP,6), a(IMP,SEC,2), a(ABY,AND,4), a(IMP,NOP,2), a(IMP,NOP,7), a(IMP,NOP,4), a(ABX,AND,4), a(ABX,ROL,7), a(IMP,NOP,7),
        /* 0x4- */ a(IMP,RTI,6), a(IZX,EOR,6), a(IMP,NOP,2), a(IMP,NOP,8), a(IMP,NOP,3), a(ZP0,EOR,3), a(ZP0,LSR,5), a(IMP,NOP,5), a(IMP,PHA,3), a(IMM,EOR,2), a(IMP,LSR,2), a(IMP,NOP,2), a(ABS,JMP,3), a(ABS,EOR,4), a(ABS,LSR,6), a(IMP,NOP,6),
        /* 0x5- */ a(REL,BVC,2), a(IZY,EOR,5), a(IMP,NOP,2), a(IMP,NOP,8), a(IMP,NOP,4), a(ZPX,EOR,4), a(ZPX,LSR,6), a(IMP,NOP,6), a(IMP,CLI,2), a(ABY,EOR,4), a(IMP,NOP,2), a(IMP,NOP,7), a(IMP,NOP,4), a(ABX,EOR,4), a(ABX,LSR,7), a(IMP,NOP,7),
        /* 0x6- */ a(IMP,RTS,6), a(IZX,ADC,6), a(IMP,NOP,2), a(IMP,NOP,8), a(IMP,NOP,3), a(ZP0,ADC,3), a(ZP0,ROR,5), a(IMP,NOP,5), a(IMP,PLA,4), a(IMM,ADC,2), a(IMP,ROR,2), a(IMP,NOP,2), a(IND,JMP,5), a(ABS,ADC,4), a(ABS,ROR,6), a(IMP,NOP,6),
        /* 0x7- */ a(REL,BVS,2), a(IZY,ADC,5), a(IMP,NOP,2), a(IMP,NOP,8), a(IMP,NOP,4), a(ZPX,ADC,4), a(ZPX,ROR,6), a(IMP,NOP,6), a(IMP,SEI,2), a(ABY,ADC,4), a(IMP,NOP,2), a(IMP,NOP,7), a(IMP,NOP,4), a(ABX,ADC,4), a(ABX,ROR,7), a(IMP,NOP,7),
        /* 0x8- */ a(IMP,NOP,2), a(IZX,STA,6), a(IMP,NOP,2), a(IMP,NOP,6), a(ZP0,STY,3), a(ZP0,STA,3), a(ZP0,STX,3), a(IMP,NOP,3), a(IMP,DEY,2), a(IMP,NOP,2), a(IMP,TXA,2), a(IMP,NOP,2), a(ABS,STY,4), a(ABS,STA,4), a(ABS,STX,4), a(IMP,NOP,4),
        /* 0x9- */ a(REL,BCC,2), a(IZY,STA,6), a(IMP,NOP,2), a(IMP,NOP,6), a(ZPX,STY,4), a(ZPX,STA,4), a(ZPY,STX,4), a(IMP,NOP,4), a(IMP,TYA,2), a(ABY,STA,5), a(IMP,TXS,2), a(IMP,NOP,5), a(IMP,NOP,5), a(ABX,STA,5), a(IMP,NOP,5), a(IMP,NOP,5),
        /* 0xA- */ a(IMM,LDY,2), a(IZX,LDA,6), a(IMM,LDX,2), a(IMP,NOP,6), a(ZP0,LDY,3), a(ZP0,LDA,3), a(ZP0,LDX,3), a(IMP,NOP,3), a(IMP,TAY,2), a(IMM,LDA,2), a(IMP,TAX,2), a(IMP,NOP,2), a(ABS,LDY,4), a(ABS,LDA,4), a(ABS,LDX,4), a(IMP,NOP,4),
        /* 0xB- */ a(REL,BCS,2), a(IZY,LDA,5), a(IMP,NOP,2), a(IMP,NOP,5), a(ZPX,LDY,4), a(ZPX,LDA,4), a(ZPY,LDX,4), a(IMP,NOP,4), a(IMP,CLV,2), a(ABY,LDA,4), a(IMP,TSX,2), a(IMP,NOP,4), a(ABX,LDY,4), a(ABX,LDA,4), a(ABY,LDX,4), a(IMP,NOP,4),
        /* 0xC- */ a(IMM,CPY,2), a(IZX,CMP,6), a(IMP,NOP,2), a(IMP,NOP,8), a(ZP0,CPY,3), a(ZP0,CMP,3), a(ZP0,DEC,5), a(IMP,NOP,5), a(IMP,INY,2), a(IMM,CMP,2), a(IMP,DEX,2), a(IMP,NOP,2), a(ABS,CPY,4), a(ABS,CMP,4), a(ABS,DEC,6), a(IMP,NOP,6),
        /* 0xD- */ a(REL,BNE,2), a(IZY,CMP,5), a(IMP,NOP,2), a(IMP,NOP,8), a(IMP,NOP,4), a(ZPX,CMP,4), a(ZPX,DEC,6), a(IMP,NOP,6), a(IMP,CLD,2), a(ABY,CMP,4), a(IMP,NOP,2), a(IMP,NOP,7), a(IMP,NOP,4), a(ABX,CMP,4), a(ABX,DEC,7), a(IMP,NOP,7),
        /* 0xE- */ a(IMM,CPX,2), a(IZX,SBC,6), a(IMP,NOP,2), a(IMP,NOP,8), a(ZP0,CPX,3), a(ZP0,SBC,3), a(ZP0,INC,5), a(IMP,NOP,5), a(IMP,INX,2), a(IMM,SBC,2), a(IMP,NOP,2), a(IMP,SBC,2), a(ABS,CPX,4), a(ABS,SBC,4), a(ABS,INC,6), a(IMP,NOP,6),
        /* 0xF- */ a(REL,BEQ,2), a(IZY,SBC,5), a(IMP,NOP,2), a(IMP,NOP,8), a(IMP,NOP,4), a(ZPX,SBC,4), a(ZPX,INC,6), a(IMP,NOP,6), a(IMP,SED,2), a(ABY,SBC,4), a(IMP,NOP,2), a(IMP,NOP,7), a(IMP,NOP,4), a(ABX,SBC,4), a(ABX,INC,7), a(IMP,NOP,7),
    };
    #undef a

    uint8_t extra_cycles = 0;

    // Check if an interrupt was request was made during the last sync period
    //      and add any additional cycles used to service it
    if (m_nmi_requested) {
        
        do_interrupt(0xFFFA); 

        // assuming both irq and nmi are pending after an instruction, nmi
        //      takes priority and irq is forgotten
        m_nmi_requested = m_irq_requested = false;

        extra_cycles += 7;
    }
    if (m_irq_requested) {

        do_interrupt(0xFFFE); 

        m_irq_requested = false;
        extra_cycles += 7;
    }

    // Read an opcode and execute the corresponding instruction, add
    //      any extra cycles consumed during instruction execution
    const instruction& i = lookup[RB(m_reg_pc++)];
    extra_cycles += (this->*i.fn)();

    // Update debug info
    #ifdef DEBUG
    CpuContext ctx = {
        .reg_a  = m_reg_a,
        .reg_x  = m_reg_x,
        .reg_y  = m_reg_y,
        .reg_s  = m_reg_s,
        .reg_p  = m_reg_p,
        .reg_pc = m_reg_pc
    };
    Debugger::get().set_cpu_context(ctx); 
    // Handle execution breakpoints
    Debugger::get().do_break(m_reg_pc, ex);
    #endif

    // Return the total number of cycles used
    return extra_cycles + i.len;
}
