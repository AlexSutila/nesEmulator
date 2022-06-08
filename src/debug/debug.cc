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
        }

    }

}

/* Updating ----------------------------------------------- */

void Debugger::update_display() {
    clear(); refresh();
}
