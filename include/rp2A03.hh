#pragma once

#include <cstdint>
#include "memory.hh"

struct rp2A03 {

private:

     // CPU Register Set
     uint8_t A, X, Y, S;
     union {
          // For status register
          uint8_t P;
          // Individual status flags
          struct {
               uint8_t flag_C : 1; // Carry flag
               uint8_t flag_Z : 1; // Zero flag
               uint8_t flag_D : 1; // Interrupt disable
               uint8_t flag_B : 1; // Break command
               uint8_t flag_V : 1; // Overflow flag
               uint8_t flag_N : 1; // Negative flag
          };
     };
     uint16_t PC;

public:

     rp2A03();

};

