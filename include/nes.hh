#pragma once
#include <SDL2/SDL.h>
#include "cart/cart.hh"
#include "2A03.hh"
#include "2C02.hh"
#include "memory.hh"

struct nes {

private:

    bool m_running;

    /* Components and Buslines ---------------------------- */

    // Both buslines
    cpu_bus m_cpu_bus;
    ppu_bus m_ppu_bus;

    // Components
    Ricoh2A03 m_cpu;
    Ricoh2C02 m_ppu;

    // Cartridge
    Cart m_cart;

    /* For rendering -------------------------------------- */

    SDL_Window   *m_window;
    SDL_Renderer *m_renderer;
    SDL_Texture  *m_texture;

public:

    nes();

    bool load_cart(const std::string& rom_path);
    void event_poll();
    void run();

};
