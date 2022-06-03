#include <assert.h>
#include "cart/mapper.hh"
#include <iostream>

void Mapper_001::cpu_WB(uint16_t addr, uint8_t value) {

    m_shift_register.value >>= 1; // Update shift register
    m_shift_register.value |= m_shift_register.data << 5;

    // Reset shift register if needed
    if (m_shift_register.reset) {
        // Again, the one in 0x20 acts as a notifier bit
        //      to indicate that 5 bits have been written
        m_shift_register.value = 0x20;
    }
    // If 5 bits written, write the value to the register
    else if (m_shift_register.value & 0x01) {

        // Control Register
        /**/ if (addr >= 0x8000 && addr <= 0x9FFF) {
            m_reg_ctrl = (value >> 1) & 0x1F;
        }

        // CHR Bank 0
        else if (addr >= 0xA000 && addr <= 0xBFFF) {
            m_reg_ctrl = (value >> 1) & 0x1F;
        }

        // CHR Bank 1
        else if (addr >= 0xC000 && addr <= 0xDFFF) {
            m_reg_ctrl = (value >> 1) & 0x1F;
        }

        // PRG Bank
        else if (addr >= 0xE000 && addr <= 0xFFFF) {
            m_reg_ctrl = (value >> 1) & 0x1F;
        }

        // This shouldn't happen
        else assert(false);

    }

}

uint8_t Mapper_001::cpu_RB(uint16_t addr) {

}

void Mapper_001::ppu_WB(uint16_t addr, uint8_t value) {

}

uint8_t Mapper_001::ppu_RB(uint16_t addr) {

}
