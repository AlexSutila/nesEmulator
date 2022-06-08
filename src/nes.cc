#include <iostream>
#include "nes.hh"

#ifdef DEBUG
#include "debug/debug.hh"
#endif

nes::nes() {

    const char* name   = "DorcelessNESs - nes emulator"; // Window name
    const int winScale = 3;  // Feel free to ajudst this to your liking
    m_running = true;

    /* Make all necessary connections between cartridge components and buslines */

    // Connect cartridge to busline
    m_cpu_bus.connect_cart(&m_cart);
    m_ppu_bus.connect_cart(&m_cart);

    m_cpu_bus.connect_cpu(&m_cpu);
    m_cpu_bus.connect_ppu(&m_ppu);

    m_cpu.connect_bus(&m_cpu_bus);
    m_ppu.connect_bus(&m_cpu_bus);
    m_ppu.connect_bus(&m_ppu_bus);

    /* Initialize SDL2 related stuff for rendering -------- */

    m_window   = SDL_CreateWindow(name, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, TV_W * winScale, TV_H * winScale, SDL_WINDOW_RESIZABLE);
    m_renderer = SDL_CreateRenderer(m_window, -1, 0);
    m_texture  = SDL_CreateTexture(m_renderer, SDL_PIXELFORMAT_ABGR8888, SDL_TEXTUREACCESS_STREAMING, TV_W, TV_H);

}

bool nes::load_cart(const std::string& rom_path) {

   return m_cart.load_rom(rom_path);

}

void nes::event_poll() {
    for (SDL_Event event; SDL_PollEvent(&event);) {
        switch (event.type) {

            case SDL_QUIT: 
                m_running = false; 
                break;

            case SDL_KEYDOWN:
                
                #ifdef DEBUG // 'Break' stop emu, go to debugger
                const uint8_t *key_state = SDL_GetKeyboardState(nullptr);
                if (key_state[SDL_SCANCODE_B]) {
                    Debugger::get().do_break();
                }
                #endif
                
                break;
                

        }
    }
}

void nes::run() {

    // Send reset signal
    m_cpu_bus.rst();

    while (m_running) {

        while (m_ppu.m_frameIncompete) {

            // Execute a single instructoin
            uint8_t cycles = m_cpu.step();

            // Catch up remaining components
            for(cycles; cycles > 0; cycles--) 
                m_cpu_bus.step();
            
            #ifdef DEBUG
            Debugger::get().poll();
            #endif

        }

        // Render frame
        SDL_UpdateTexture(m_texture, nullptr, m_ppu.get_buf().get(), TV_W * sizeof(int));
        SDL_RenderCopy(m_renderer, m_texture, nullptr, nullptr);
        SDL_RenderPresent(m_renderer);
        m_ppu.m_frameIncompete = true;

        // Do event poll
        event_poll();

    }

}
