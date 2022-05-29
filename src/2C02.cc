#include "2C02.hh"

void Ricoh2C02::load_cart(Cart* cart_ptr) {
    m_ppu_bus->load_cart(cart_ptr);
}

/* Memory access to the cpu buslines */

void Ricoh2C02::WB(uint16_t addr, uint8_t value) {
    m_ppu_bus->WB(addr, value);
}

uint8_t Ricoh2C02::RB(uint16_t addr) {
    return m_ppu_bus->RB(addr);
}
