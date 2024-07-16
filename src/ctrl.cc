#include "ctrl.hh"
#include <SDL2/SDL.h>

// I'm sorry, I can't resist, cursed macros are just so much fun
#define LIST_BUTTONS(X) \
    /*name,     SDL scancode,      index*/ \
    X("up",     SDL_SCANCODE_W,         4) \
    X("left",   SDL_SCANCODE_A,         6) \
    X("down",   SDL_SCANCODE_S,         5) \
    X("right",  SDL_SCANCODE_D,         7) \
    X("start",  SDL_SCANCODE_ESCAPE,    2) \
    X("select", SDL_SCANCODE_BACKSPACE, 3) \
    X("A",      SDL_SCANCODE_L,         0) \
    X("B",      SDL_SCANCODE_J,         1)

Controller::Controller() {
    for (bool& t : m_btnStates) 
        t = false;
    m_shift = 0;
}

void Controller::update(const uint8_t *keystate) {
    #define X(name, scancode, index) \
        m_btnStates[index] = keystate[scancode];
    LIST_BUTTONS(X)
    #undef X
} 

uint8_t Controller::r_joypad() {
    
    uint8_t ret = (uint8_t)m_btnStates[m_shift];
    ++m_shift %= 8;

    return ret;
}

void Controller::w_joypad(uint8_t value) {
    // Tell controller its ready for A
    m_shift = 0;
}
