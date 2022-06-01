#include "2A03.hh"

/* Busline connections ------------------------------------ */

void Ricoh2A03::connect_bus(cpu_bus* cpu_bus_ptr) {
    m_bus = cpu_bus_ptr;
}

/* Memory access to the cpu buslines ---------------------- */

void Ricoh2A03::WB(uint16_t addr, uint8_t value) {
    m_bus->WB(addr, value);
}

uint8_t Ricoh2A03::RB(uint16_t addr) {
    return m_bus->RB(addr);
}


/* External signals --------------------------------------- */

void Ricoh2A03::irq() {

}

void Ricoh2A03::nmi() {

}

void Ricoh2A03::rst() {

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
        addr_abs |= RB(m_reg_pc++) << 8;
    }
    else if constexpr (a_m == ABX) {
        uint8_t lo = RB(m_reg_pc++);
        uint8_t hi = RB(m_reg_pc++);
        addr_abs = ((hi << 8) | lo) + m_reg_x;
        // Specific instructions will check addrmode_extra_cycle to see if it needs the
        //      additional cycle to handle the page cross, other instructions just do
        //      the page cross regardless if its needed or not.
        if (addr_abs & 0xFF00 != hi << 8) 
            addrmode_extra_cycle = true;
    }
    else if constexpr (a_m == ABY) {
        uint8_t lo = RB(m_reg_pc++);
        uint8_t hi = RB(m_reg_pc++);
        addr_abs = ((hi << 8) | lo) + m_reg_y;
        // Same rational as ABX
        if (addr_abs & 0xFF00 != hi << 8)
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
        m_flag_v = (~((uint16_t)m_reg_a^(uint16_t)t16)&((uint16_t)m_reg_a^(uint16_t)t16))&0x0080;
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
        t16 = (uint16_t)(m_flag_c << 7) | (t16 >> 7);

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

        m_reg_x = m_reg_s;

    }
    else if constexpr (op == TYA) {

        m_reg_a = m_reg_y;
        m_flag_z = (m_reg_a == 0x00);
        m_flag_n = (m_reg_a & 0x80);

    }

    return extra_cycles;
}

void Ricoh2A03::step() {

    // For conciseness
    typedef uint8_t (Ricoh2A03::*instruction)();
    using a = Ricoh2A03;
    
    // Lookup table of function pointers, indexed by opcode to get 
    //      the instruction to execute ... 
    static const instruction lookup[0x100] = 
    {
    /* ~Dorceless~            0x-0              0x-1              0x-2              0x-3              0x-4              0x-5              0x-6              0x-7              0x-8              0x-9              0x-A              0x-B              0x-C              0x-D              0x-E              0x-F */ 
        /* 0x0- */ &a::ins<IMM,BRK>, &a::ins<IZX,ORA>, &a::ins<IMP,NOP>, &a::ins<IMP,NOP>, &a::ins<IMP,NOP>, &a::ins<ZP0,ORA>, &a::ins<ZP0,ASL>, &a::ins<IMP,NOP>, &a::ins<IMP,PHP>, &a::ins<IMM,ORA>, &a::ins<IMP,ASL>, &a::ins<IMP,NOP>, &a::ins<IMP,NOP>, &a::ins<ABS,ORA>, &a::ins<ABS,ASL>, &a::ins<IMP,NOP>,
		/* 0x1- */ &a::ins<REL,BPL>, &a::ins<IZY,ORA>, &a::ins<IMP,NOP>, &a::ins<IMP,NOP>, &a::ins<IMP,NOP>, &a::ins<ZPX,ORA>, &a::ins<ZPX,ASL>, &a::ins<IMP,NOP>, &a::ins<IMP,CLC>, &a::ins<ABY,ORA>, &a::ins<IMP,NOP>, &a::ins<IMP,NOP>, &a::ins<IMP,NOP>, &a::ins<ABX,ORA>, &a::ins<ABX,ASL>, &a::ins<IMP,NOP>,
		/* 0x2- */ &a::ins<ABS,JSR>, &a::ins<IZX,AND>, &a::ins<IMP,NOP>, &a::ins<IMP,NOP>, &a::ins<ZP0,BIT>, &a::ins<ZP0,AND>, &a::ins<ZP0,ROL>, &a::ins<IMP,NOP>, &a::ins<IMP,PLP>, &a::ins<IMM,AND>, &a::ins<IMP,ROL>, &a::ins<IMP,NOP>, &a::ins<ABS,BIT>, &a::ins<ABS,AND>, &a::ins<ABS,ROL>, &a::ins<IMP,NOP>,
		/* 0x3- */ &a::ins<REL,BMI>, &a::ins<IZY,AND>, &a::ins<IMP,NOP>, &a::ins<IMP,NOP>, &a::ins<IMP,NOP>, &a::ins<ZPX,AND>, &a::ins<ZPX,ROL>, &a::ins<IMP,NOP>, &a::ins<IMP,SEC>, &a::ins<ABY,AND>, &a::ins<IMP,NOP>, &a::ins<IMP,NOP>, &a::ins<IMP,NOP>, &a::ins<ABX,AND>, &a::ins<ABX,ROL>, &a::ins<IMP,NOP>,
		/* 0x4- */ &a::ins<IMP,RTI>, &a::ins<IZX,EOR>, &a::ins<IMP,NOP>, &a::ins<IMP,NOP>, &a::ins<IMP,NOP>, &a::ins<ZP0,EOR>, &a::ins<ZP0,LSR>, &a::ins<IMP,NOP>, &a::ins<IMP,PHA>, &a::ins<IMM,EOR>, &a::ins<IMP,LSR>, &a::ins<IMP,NOP>, &a::ins<ABS,JMP>, &a::ins<ABS,EOR>, &a::ins<ABS,LSR>, &a::ins<IMP,NOP>,
		/* 0x5- */ &a::ins<REL,BVC>, &a::ins<IZY,EOR>, &a::ins<IMP,NOP>, &a::ins<IMP,NOP>, &a::ins<IMP,NOP>, &a::ins<ZPX,EOR>, &a::ins<ZPX,LSR>, &a::ins<IMP,NOP>, &a::ins<IMP,CLI>, &a::ins<ABY,EOR>, &a::ins<IMP,NOP>, &a::ins<IMP,NOP>, &a::ins<IMP,NOP>, &a::ins<ABX,EOR>, &a::ins<ABX,LSR>, &a::ins<IMP,NOP>,
		/* 0x6- */ &a::ins<IMP,RTS>, &a::ins<IZX,ADC>, &a::ins<IMP,NOP>, &a::ins<IMP,NOP>, &a::ins<IMP,NOP>, &a::ins<ZP0,ADC>, &a::ins<ZP0,ROR>, &a::ins<IMP,NOP>, &a::ins<IMP,PLA>, &a::ins<IMM,ADC>, &a::ins<IMP,ROR>, &a::ins<IMP,NOP>, &a::ins<IND,JMP>, &a::ins<ABS,ADC>, &a::ins<ABS,ROR>, &a::ins<IMP,NOP>,
		/* 0x7- */ &a::ins<REL,BVS>, &a::ins<IZY,ADC>, &a::ins<IMP,NOP>, &a::ins<IMP,NOP>, &a::ins<IMP,NOP>, &a::ins<ZPX,ADC>, &a::ins<ZPX,ROR>, &a::ins<IMP,NOP>, &a::ins<IMP,SEI>, &a::ins<ABY,ADC>, &a::ins<IMP,NOP>, &a::ins<IMP,NOP>, &a::ins<IMP,NOP>, &a::ins<ABX,ADC>, &a::ins<ABX,ROR>, &a::ins<IMP,NOP>,
		/* 0x8- */ &a::ins<IMP,NOP>, &a::ins<IZX,STA>, &a::ins<IMP,NOP>, &a::ins<IMP,NOP>, &a::ins<ZP0,STY>, &a::ins<ZP0,STA>, &a::ins<ZP0,STX>, &a::ins<IMP,NOP>, &a::ins<IMP,DEY>, &a::ins<IMP,NOP>, &a::ins<IMP,TXA>, &a::ins<IMP,NOP>, &a::ins<ABS,STY>, &a::ins<ABS,STA>, &a::ins<ABS,STX>, &a::ins<IMP,NOP>,
		/* 0x9- */ &a::ins<REL,BCC>, &a::ins<IZY,STA>, &a::ins<IMP,NOP>, &a::ins<IMP,NOP>, &a::ins<ZPX,STY>, &a::ins<ZPX,STA>, &a::ins<ZPY,STX>, &a::ins<IMP,NOP>, &a::ins<IMP,TYA>, &a::ins<ABY,STA>, &a::ins<IMP,TXS>, &a::ins<IMP,NOP>, &a::ins<IMP,NOP>, &a::ins<ABX,STA>, &a::ins<IMP,NOP>, &a::ins<IMP,NOP>,
		/* 0xA- */ &a::ins<IMM,LDY>, &a::ins<IZX,LDA>, &a::ins<IMM,LDX>, &a::ins<IMP,NOP>, &a::ins<ZP0,LDY>, &a::ins<ZP0,LDA>, &a::ins<ZP0,LDX>, &a::ins<IMP,NOP>, &a::ins<IMP,TAY>, &a::ins<IMM,LDA>, &a::ins<IMP,TAX>, &a::ins<IMP,NOP>, &a::ins<ABS,LDY>, &a::ins<ABS,LDA>, &a::ins<ABS,LDX>, &a::ins<IMP,NOP>,
		/* 0xB- */ &a::ins<REL,BCS>, &a::ins<IZY,LDA>, &a::ins<IMP,NOP>, &a::ins<IMP,NOP>, &a::ins<ZPX,LDY>, &a::ins<ZPX,LDA>, &a::ins<ZPY,LDX>, &a::ins<IMP,NOP>, &a::ins<IMP,CLV>, &a::ins<ABY,LDA>, &a::ins<IMP,TSX>, &a::ins<IMP,NOP>, &a::ins<ABX,LDY>, &a::ins<ABX,LDA>, &a::ins<ABY,LDX>, &a::ins<IMP,NOP>,
		/* 0xC- */ &a::ins<IMM,CPY>, &a::ins<IZX,CMP>, &a::ins<IMP,NOP>, &a::ins<IMP,NOP>, &a::ins<ZP0,CPY>, &a::ins<ZP0,CMP>, &a::ins<ZP0,DEC>, &a::ins<IMP,NOP>, &a::ins<IMP,INY>, &a::ins<IMM,CMP>, &a::ins<IMP,DEX>, &a::ins<IMP,NOP>, &a::ins<ABS,CPY>, &a::ins<ABS,CMP>, &a::ins<ABS,DEC>, &a::ins<IMP,NOP>,
		/* 0xD- */ &a::ins<REL,BNE>, &a::ins<IZY,CMP>, &a::ins<IMP,NOP>, &a::ins<IMP,NOP>, &a::ins<IMP,NOP>, &a::ins<ZPX,CMP>, &a::ins<ZPX,DEC>, &a::ins<IMP,NOP>, &a::ins<IMP,CLD>, &a::ins<ABY,CMP>, &a::ins<IMP,NOP>, &a::ins<IMP,NOP>, &a::ins<IMP,NOP>, &a::ins<ABX,CMP>, &a::ins<ABX,DEC>, &a::ins<IMP,NOP>,
		/* 0xE- */ &a::ins<IMM,CPX>, &a::ins<IZX,SBC>, &a::ins<IMP,NOP>, &a::ins<IMP,NOP>, &a::ins<ZP0,CPX>, &a::ins<ZP0,SBC>, &a::ins<ZP0,INC>, &a::ins<IMP,NOP>, &a::ins<IMP,INX>, &a::ins<IMM,SBC>, &a::ins<IMP,NOP>, &a::ins<IMP,SBC>, &a::ins<ABS,CPX>, &a::ins<ABS,SBC>, &a::ins<ABS,INC>, &a::ins<IMP,NOP>,
		/* 0xF- */ &a::ins<REL,BEQ>, &a::ins<IZY,SBC>, &a::ins<IMP,NOP>, &a::ins<IMP,NOP>, &a::ins<IMP,NOP>, &a::ins<ZPX,SBC>, &a::ins<ZPX,INC>, &a::ins<IMP,NOP>, &a::ins<IMP,SED>, &a::ins<ABY,SBC>, &a::ins<IMP,NOP>, &a::ins<IMP,NOP>, &a::ins<IMP,NOP>, &a::ins<ABX,SBC>, &a::ins<ABX,INC>, &a::ins<IMP,NOP>,
    };

    uint8_t opcode = RB(m_reg_pc++);
    (this->*lookup[opcode])();
}
