#include "debug/debug.hh"
#include <string>

Debugger::Debugger() {
    initscr();
}

Debugger::~Debugger() {
    endwin();
}

/* Breaking ----------------------------------------------- */

void Debugger::do_break(uint16_t addr, BreakpointType t) {

    // Return if the entry exists in the hash map
    if (m_breakpoints.find(addr) == m_breakpoints.end()) return;

    // Check bit at hash map entry for address
    switch (t) {
        case rd: if (m_breakpoints.at(addr).rd) m_enable = true; break;
        case wr: if (m_breakpoints.at(addr).wr) m_enable = true; break;
        case ex: if (m_breakpoints.at(addr).ex) m_enable = true; break;
    }

}

void Debugger::do_break() {
    m_enable = true;
}

void Debugger::poll() {

    while (m_enable) {

        update_display();
        std::string in;
        in.reserve(100);

        getnstr(in.data(), 100);
        switch(in[0]) {
            case 'c': m_enable = false; return; // Continue emulation
            case 's': /* ----------- */ return; // Step 1 instruction
            // Break points will consist of read write and execute and I will utilize a
            //      number 0-7 to denote which I want to set or unset
            // EX: "b3 1234" -> R/W break point at hex address 1234, can be removed by 
            //      typing the following: "b0 1234", as it sets each flag to false
            case 'b': {
                    uint16_t addr = std::strtol(&in.data()[2], nullptr, 16);
                    Breakpoint brk = {
                        .rd = false, // bit 0
                        .wr = false, // bit 1
                        .ex = false, // bit 2
                    };
                    if (in[1] & 0x1) brk.rd = true;
                    if (in[1] & 0x2) brk.wr = true;
                    if (in[1] & 0x4) brk.ex = true;
                    m_breakpoints[addr] = brk;
                } break;
        }

    }

}

/* Updating ----------------------------------------------- */

void Debugger::update_display() {

    clear(); 

    printw("CART ------------------------------------------------------------------------------------------------\n");
    
    printw("CPU -------------------------------------------------------------------------------------------------\n");

    // Print CPU registers
    printw(" A: 0x%02X X: 0x%02X" , m_cpu_context.reg_a, m_cpu_context.reg_x );
    printw(" S: 0x%02X Y: 0x%02X" , m_cpu_context.reg_s, m_cpu_context.reg_y );
    printw(" P: 0x%02X PC: 0x%04X" , m_cpu_context.reg_p, m_cpu_context.reg_pc);

    // Print CPU flags
    #define to_bin(flag) flag ? 1 : 0
    printw(" - Flags - C: %d Z: %d I: %d D: %d B: %d -: %d V: %d N: %d\n",
        to_bin(m_cpu_context.flag_c), to_bin(m_cpu_context.flag_z),
        to_bin(m_cpu_context.flag_i), to_bin(m_cpu_context.flag_d),
        to_bin(m_cpu_context.flag_b), to_bin(m_cpu_context.flag_u),
        to_bin(m_cpu_context.flag_v), to_bin(m_cpu_context.flag_n));
    #undef to_bin

    printw("PPU -------------------------------------------------------------------------------------------------\n");

    printw(" Cycle: %d\t Scanline: %d\n", m_ppu_context.cycle, m_ppu_context.scanline);

    refresh();
}

void Debugger::set_cpu_context(CpuContext& ctx) {
    m_cpu_context = ctx;
}

void Debugger::set_ppu_context(PpuContext& ctx) {
    m_ppu_context = ctx;
}
