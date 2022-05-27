#include "memory.hh"

/* -------------------------------------------------------- */
/*                                                          */
/*                         CPU BUS                          */
/*                                                          */
/* -------------------------------------------------------- */

cpu_bus::cpu_bus() {

    // Initialize component list
    m_cpu = std::shared_ptr<Ricoh2A03>(new Ricoh2A03(this));
    m_ppu = std::shared_ptr<Ricoh2C02>(new Ricoh2C02(this));

    // Initialize the hash map for mapped IO

}

/* Read from and write to the bus ------------------------- */

void cpu_bus::WB(uint16_t addr, uint8_t value) {

}

uint8_t cpu_bus::RB(uint16_t addr) {
    return 0x00;
}

/* External signals --------------------------------------- */

void cpu_bus::irq() {
    m_cpu->irq();
}

void cpu_bus::nmi() {
    m_cpu->nmi();
}

void cpu_bus::rst() {
    m_cpu->rst();
}

/* Step all components on the bus ------------------------- */

void cpu_bus::step(uint8_t cycles) {
    // TODO
}


/* -------------------------------------------------------- */
/*                                                          */
/*                         PPU BUS                          */
/*                                                          */
/* -------------------------------------------------------- */


