#pragma once
#include <memory>
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

};
