#pragma once
#include <memory>
#include "memory.hh"

struct cpu_bus;
struct ppu_bus;

struct Ricoh2C02 {

private:

    // A pointer to the CPU busline
    cpu_bus* m_cpu_bus;

public:

    Ricoh2C02(cpu_bus* bus_ptr) : m_cpu_bus(bus_ptr) {}

};
