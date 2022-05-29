#include "nes.hh"

nes::nes() {

    /* Make all necessary connections between cartridge components and buslines */

    // Connect cartridge to busline
    m_cpu_bus.connect_cart(&m_cart);
    m_ppu_bus.connect_cart(&m_cart);

    m_cpu_bus.connect_cpu(&m_cpu);
    m_cpu_bus.connect_ppu(&m_ppu); // Still connected because of MMIO

    m_cpu.connect_bus(&m_cpu_bus);
    m_ppu.connect_bus(&m_cpu_bus);
    m_ppu.connect_bus(&m_ppu_bus);

}

bool nes::load_cart(const std::string& rom_path) {

   return m_cart.load_rom(rom_path);

}

void nes::run() {

}
