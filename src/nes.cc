#include "nes.hh"

bool nes::load_cart(const std::string& rom_path) {

    if (m_cart.load_rom(rom_path)) {

        m_bus.load_cart(&m_cart);
        return true;

    } else return false;

}

void nes::run() {

}
