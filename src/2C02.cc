#include "2C02.hh"

Ricoh2C02::Ricoh2C02() {

    // Initialize registers
    m_reg_ctrl1      = 0x00;
    m_reg_ctrl2      = 0x00;
    m_reg_status     = 0x00;
    m_reg_spr_addr   = 0x00;
    m_reg_spr_io     = 0x00;
    m_reg_vram_addr1 = 0x00;
    m_reg_vram_addr2 = 0x00;
    m_reg_vram_io    = 0x00;

}

/* Bus connections */

void Ricoh2C02::connect_bus(cpu_bus* cpu_bus_ptr) {
    m_cpu_bus = cpu_bus_ptr;
}

void Ricoh2C02::connect_bus(ppu_bus* ppu_bus_ptr) {
    m_ppu_bus = ppu_bus_ptr;
}

/* Memory access to the cpu buslines */

void Ricoh2C02::WB(uint16_t addr, uint8_t value) {
    m_ppu_bus->WB(addr, value);
}

uint8_t Ricoh2C02::RB(uint16_t addr) {
    return m_ppu_bus->RB(addr);
}

/* Step the component one cycle */

void Ricoh2C02::step() {
    // TODO
}

/* MMIO functions - very subject to change */

void Ricoh2C02::ctrl1_w(uint8_t value) {
    m_reg_ctrl1 = value;
}
uint8_t Ricoh2C02::ctrl1_r() {
    return m_reg_ctrl1;
}

void Ricoh2C02::ctrl2_w(uint8_t value) {
    m_reg_ctrl2 = value;
}
uint8_t Ricoh2C02::ctrl2_r() {
    return m_reg_ctrl2;
}

void Ricoh2C02::status_w(uint8_t value) {
    m_reg_status = value;
}
uint8_t Ricoh2C02::status_r() {
    return m_reg_status;
}

void Ricoh2C02::spr_addr_w(uint8_t value) {
    m_reg_spr_addr = value;
}
uint8_t Ricoh2C02::spr_addr_r() {
    return m_reg_spr_addr;
}

void Ricoh2C02::spr_io_w(uint8_t value) {
    m_reg_spr_io = value;
}
uint8_t Ricoh2C02::spr_io_r() {
    return m_reg_spr_io;
}

void Ricoh2C02::vram_addr1_w(uint8_t value) {
    m_reg_vram_addr1 = value;
}
uint8_t Ricoh2C02::vram_addr1_r() {
    return m_reg_vram_addr1;
}

void Ricoh2C02::vram_addr2_w(uint8_t value) {
    m_reg_vram_addr2 = value;
}
uint8_t Ricoh2C02::vram_addr2_r() {
    return m_reg_vram_addr2;
}

void Ricoh2C02::vram_io_w(uint8_t value) {
    m_reg_vram_io = value;
}
uint8_t Ricoh2C02::vram_io_r() {
    return m_reg_vram_io;
}
