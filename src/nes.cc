#include "nes.hh"

bool nes::load_cart(const std::string& rom_path) {

    return m_cart.load_rom(rom_path);

}

void nes::run() {

}
