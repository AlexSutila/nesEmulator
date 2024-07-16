#pragma once
#include <SDL2/SDL.h>
#include <chrono>
#include <string>
#include "cart/cart.hh"
#include "gamegenie.hh"
#include "2A03.hh"
#include "2C02.hh"
#include "ctrl.hh"
#include "memory.hh"

struct nes {

private:

    bool m_running;

    /* Components and Buslines ---------------------------- */

    // For cheat codes >:)
    GameGenie game_genie;

    // Both buslines
    cpu_bus m_cpu_bus;
    ppu_bus m_ppu_bus;

    // Components
    Ricoh2A03 m_cpu;
    Ricoh2C02 m_ppu;

    // Cartridge
    Cart m_cart;

    /* Controllers, subject to change --------------------- */

    Controller m_ctrl1;

    /* For rendering and timing --------------------------- */

    std::chrono::time_point<std::chrono::system_clock> m_time;
    SDL_Window   *m_window;
    SDL_Renderer *m_renderer;
    SDL_Texture  *m_texture;

public:

    nes();

    void add_cheat_code(const std::string& code);
    bool load_cart(const std::string& rom_path);
    void event_poll();
    void run();

};
