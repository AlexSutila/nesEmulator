#pragma once

#include <cstdint>

/* Many readable and writable bytes within memory can be manipulated w many
   addresses. This header exists to provide functions to reduce all mirrored 
   addresses down to a single set of addresses to simplify things */

namespace AddressMirrors {

    namespace CpuBus {

        // Address range 0x0000 - 0x2000
        inline uint16_t mirror_ram(uint16_t addr) {
            return addr & 0x7FF;
        }

        // Address range 0x2000 - 0x4020
        inline uint16_t mirror_io(uint16_t addr) {
            return addr < 0x4000 ? (addr & 0x7) | 0x2000 : addr;
        }

    }

    namespace PpuBus {

        // Address range 0x2000 - 0x3F00
        inline uint16_t mirror_nametables(uint16_t addr) {
            return (addr & 0xFFF) | 0x2000;
        }

        // Address range 0x3F00 - 0x4000
        inline uint16_t mirror_palettes(uint16_t addr) {
            return (addr % 0x20) | 0x3F00;
        }

        /* The next function should be called for every address read or written
           w the ppu bus before doing any additional mirroring because the entire
           bottom quarter of the whole address range is mirrored three times */

        // Address range 0x4000 - 0x10000
        inline uint16_t mirror_mirrors(uint16_t addr) {
            return addr & 0x3FFF;
        }

    }

}
