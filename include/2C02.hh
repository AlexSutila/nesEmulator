#pragma once
#include <memory>
#include <cart/cart.hh>
#include "memory.hh"

struct cpu_bus;
struct ppu_bus;

struct Ricoh2C02 {

private:

    // A pointer to the PPU busline
    ppu_bus* m_ppu_bus;

    // A pointer to the CPU busline to trigger interrupts
    cpu_bus* m_cpu_bus;

public:

    // Connect components
    void connect_bus(cpu_bus* cpu_bus_ptr);
    void connect_bus(ppu_bus* ppu_bus_ptr);

    // Memory access to the cpu buslines
    void WB(uint16_t addr, uint8_t value);
    uint8_t RB(uint16_t addr);

    // Step the component one cycle
    void step();

};
