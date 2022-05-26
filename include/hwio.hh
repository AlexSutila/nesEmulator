#pragma once

#include <cstdint>

class IORegister {

protected:

    // An 8-bit integer representing the value of the register
    uint8_t value;

public:

    // A callback function for reading from the register
    virtual uint8_t read() = 0;

    // A callback function for writing to the register
    virtual void write(uint8_t val) = 0;

};
