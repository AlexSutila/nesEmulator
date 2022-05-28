#include <iostream>
#include "nes.hh"

int main(int argc, char** argv) {

     nes emulator;
     if (argv[1] != nullptr && emulator.load_cart(argv[1])) {
          emulator.run();
     }
     else std::cout << "Failed to load rom" << std::endl;

     return 0;
}
