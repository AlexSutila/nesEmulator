#pragma once

#include <unordered_map> 
#include <memory>
#include <cstdint>
#include "hwio.hh"

struct Busline {

private:

    // To map virtual addresses to mapped IO
    std::unordered_map<uint16_t, IORegister*> io_map;

    // Contains the contents of ram
    std::unique_ptr<uint8_t[]> ram;

public:

    // Used to map an IO register to a virtual address
    void map_io_to_virtual(IORegister* reg, uint16_t addr);

    // Perform a memory access to an address on the bus
    void write(uint16_t addr, uint8_t value);
    uint8_t read(uint16_t addr);

    // Step all components on the bus
    void step(uint8_t cycles);

};
