#include <assert.h>
#include "cart/mapper.hh"
#include <iostream>

void Mapper_001::cpu_WB(uint16_t addr, uint8_t value) {

    /* Handle writes to RAM */

    if (addr >= 0x6000 && addr <= 0x7FFF) {

        m_cart->get_PRG_RAM()[addr & 0x1FFF] = value;
        return; // To avoid writing to registers

    }


    /* Handle writes to registers */

    m_shift_register.data  = (value & 0x01);
    m_shift_register.reset = (value & 0x80);

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
            m_reg_ctrl.raw = (m_shift_register.value >> 1) & 0x1F;
        }

        // CHR Bank 0
        else if (addr >= 0xA000 && addr <= 0xBFFF) {
            
            m_chr_bank0 = (m_reg_ctrl.chrBankMode == 1) ?
                (m_shift_register.value >> 1) & 0x1F:
                (m_shift_register.value >> 1) & 0x1E; // Low bit ignored

        }

        // CHR Bank 1
        else if (addr >= 0xC000 && addr <= 0xDFFF) {
            
            if (m_reg_ctrl.chrBankMode == 1)
                m_chr_bank1 = (m_shift_register.value >> 1) & 0x1F;

            // NOTE: Writes here are ignored in 8 KB mode as only one bank
            //      ends up being used

        }

        // PRG Bank
        else if (addr >= 0xE000 && addr <= 0xFFFF) {
            
            switch(m_reg_ctrl.prgBankMode) {

            case 0: case 1:
                m_prg_bank0 = (m_shift_register.value >> 1) & 0x1E; // Low bit ignored
                break;
            
            case 2:
                m_prg_bank0 = 0;
                m_prg_bank1 = (m_shift_register.value >> 1) & 0x0F;
                break;
            
            case 3:
                m_prg_bank0 = (m_shift_register.value >> 1) & 0x0F;
                m_prg_bank1 = (m_size_prg_rom / 0x4000) - 1; // Fixed at last bank
                break;

            }

        }

        // This shouldn't happen
        else assert(false);

    }

}

uint8_t Mapper_001::cpu_RB(uint16_t addr) {

    if (addr >= 0x6000 && addr <= 0x7FFF) {

        return m_cart->get_PRG_RAM()[addr & 0x1FFF];

    }

    else if (addr >= 0x8000) {

        if ((m_reg_ctrl.prgBankMode == 0) || (m_reg_ctrl.prgBankMode == 1)) {

            // Higher bank value is ignored
            return m_cart->get_PRG_ROM()[(addr & 0x7FFF) + (0x8000 * m_prg_bank0)];

        }

        else {
            
            /**/ if (addr >= 0x8000 && addr <= 0xBFFF) {
                return m_cart->get_PRG_ROM()[(addr & 0x3FFF) + (0x4000 * m_prg_bank0)];
            }

            else if (addr >= 0xC000 && addr <= 0xFFFF) {
                return m_cart->get_PRG_ROM()[(addr & 0x3FFF) + (0x4000 * m_prg_bank1)];
            }

        }

    }

    return 0x00;
}

void Mapper_001::ppu_WB(uint16_t addr, uint8_t value) {

}

uint8_t Mapper_001::ppu_RB(uint16_t addr) {
    return 0x00;
}

ntMirrors::nameTableMirrorMode Mapper_001::nt_mirror() {
    
    switch (m_reg_ctrl.mirroring) {
        case 0: return ntMirrors::singleScreenLo;
        case 1: return ntMirrors::singleScreenHi;
        case 2: return ntMirrors::vertical;
        case 3: return ntMirrors::horizontal;
    };

}

void Mapper_001::rst() {

    m_reg_ctrl.raw = 0x1C;
    m_chr_bank0 = 0x00;
    m_chr_bank1 = 0x00;
    m_prg_bank0 = 0x00;
    m_prg_bank1 = (m_size_prg_rom / 0x4000) - 1; // Init to last bank

}
