#include <iostream>
#include <string>
#include "nes.hh"

int main(int argc, char** argv) {

    nes emulator;
    if (argv[1] != nullptr && emulator.load_cart(argv[1]))
    {
        for (int i = 2; i < argc; i++)
            emulator.add_cheat_code(std::string(argv[i]));
        emulator.run();
    }
    else std::cout << "Failed to load rom" << std::endl;

    return 0;
}
