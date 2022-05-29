#pragma once
#include <memory>
#include <cart/cart.hh>
#include "memory.hh"

struct cpu_bus;
struct ppu_bus;

struct Ricoh2C02 {

private:

    // A pointer to the PPU busline, to be initialized within the constructor
    std::unique_ptr<ppu_bus> m_ppu_bus;

    // A pointer to the CPU busline
    cpu_bus* m_cpu_bus;

public:

    Ricoh2C02(cpu_bus* bus_ptr) : m_cpu_bus(bus_ptr) {

        // Allocate space for the PPU's seperate bus
        m_ppu_bus = std::make_unique<ppu_bus>();

    }

    // Part of the PPU bus reads from the cartridge, so a function is needed
    //      to connect the PPU bus to the cartridge as well. This is called
    //      from cpu_bus::load_cart
    void load_cart(Cart* cart_ptr);

    // Memory access to the cpu buslines
    void WB(uint16_t addr, uint8_t value);
    uint8_t RB(uint16_t addr);

};
