#pragma once

#include <curses.h> // Yeaaaah this will be a terminal based debugger

/* 
    I am in no way designing this debugger to be user friendly or robust. I am doing it for my own convenience
    to actually help with debugging and verification of certain things as I write this emulator. You can feel
    free to tinker with the capabilities of the debugger anyway, just bear in mind, I'm not going to focus on
    making this a solid debugger in any way what so ever. 

    It exists SOLEY for convenience
*/

class Debugger {

public:

    static Debugger& get() {
        static Debugger instance;
        return instance;
    }

    void do_break();
    void poll();
    
private:

    Debugger(); ~Debugger();
    void update_display();
    bool m_enable = false;

};
