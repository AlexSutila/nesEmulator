#pragma once
#include <cstdint>

namespace ntMirrors {

    enum nameTableMirrorMode {
    
        horizontal,     // Use two internal NTs, horizontal config
        vertical,       // Use two internal NTs, vertical config
        singleScreenLo, // Use lower bank from internal NT mirrored 4 times
        singleScreenHi, // Use upper bank from internal NT mirrored 4 times
        fourScreen,     // Use external RAM for four individual nametables
    
    };

}

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
            uint16_t temp = (addr % 0x20) | 0x3F00;
            // Also need to mirror the background byte a few times
            return (temp % 4 == 0) ? 0x3F00 : temp;
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
