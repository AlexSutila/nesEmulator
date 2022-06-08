#include "debug/debug.hh"
#include <string>

Debugger::Debugger() {
    initscr();
}

Debugger::~Debugger() {
    endwin();
}

/* Breaking ----------------------------------------------- */

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
            case 'c': m_enable = false; return;
            case 's': /*Step by instr*/ return;
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

    refresh();
}

void Debugger::set_cpu_context(CpuContext& ctx) {
    m_cpu_context = ctx;
}
