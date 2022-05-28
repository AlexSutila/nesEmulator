#pragma once
#include "cart/cart.hh"
#include "memory.hh"

struct nes {

private:

    // Contains a pointer to CPU and PPU components and initializes them within
    //      the constructor. In addition, the PPU initializes it's own busline
    //      within it's constructor so the whole system sets itself up within 
    //      the constructor of this object.
    cpu_bus m_bus;

    // The cartridge, to be loaded with the contents of a rom file, given the 
    //      path to the file
    Cart m_cart;

public:

    bool load_cart(const std::string& rom_path);
    void run();

};
