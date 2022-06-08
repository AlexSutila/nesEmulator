#pragma once

#include <curses.h>      // Yeaaaah this will be a terminal based debugger
#include <unordered_map> // For R,W,E breakpoints
#include <cstdint>

/* 
    I am in no way designing this debugger to be user friendly or robust. I am doing it for my own convenience
    to actually help with debugging and verification of certain things as I write this emulator. You can feel
    free to tinker with the capabilities of the debugger anyway, just bear in mind, I'm not going to focus on
    making this a solid debugger in any way what so ever. 

    It exists SOLEY for convenience
*/

enum BreakpointType {
    rd, wr, ex
};
struct Breakpoint {
    bool rd, wr, ex;
};

struct CpuContext {
    // I may add more to this, ... unsure, it all
    //      depends on what I want to observe
    uint8_t  reg_a, reg_x, reg_y, reg_s;
    union {
        uint8_t reg_p;
        struct {
            bool flag_c : 1;
            bool flag_z : 1;
            bool flag_i : 1;
            bool flag_d : 1;
            bool flag_b : 1;
            bool flag_u : 1;
            bool flag_v : 1;
            bool flag_n : 1;
        };
    };
    uint16_t reg_pc;
};

struct PpuContext {
    // This will almost certainly expand as PPU
    //      mmio is implemented
    int cycle, scanline;
};

class Debugger {

public:

    static Debugger& get() {
        static Debugger instance;
        return instance;
    }

    void do_break(uint16_t addr, BreakpointType t);
    void do_break();
    void poll();

    // For updating component information
    void set_cpu_context(CpuContext& ctx);
    void set_ppu_context(PpuContext& ctx);
    
    
private:

    // Component contexts
    CpuContext m_cpu_context;
    PpuContext m_ppu_context;

    // Read, Write, Execute breakpoints
    std::unordered_map<uint16_t, Breakpoint> m_breakpoints;

    Debugger(); ~Debugger();
    void update_display();
    bool m_enable = true;

};
