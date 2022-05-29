#include "2A03.hh"

/* Busline connections ------------------------------------ */

void Ricoh2A03::connect_bus(cpu_bus* cpu_bus_ptr) {
    m_bus = cpu_bus_ptr;
}

/* Memory access to the cpu buslines ---------------------- */

void Ricoh2A03::WB(uint16_t addr, uint8_t value) {
    m_bus->WB(addr, value);
}

uint8_t Ricoh2A03::RB(uint16_t addr) {
    return m_bus->RB(addr);
}


/* External signals --------------------------------------- */

void Ricoh2A03::irq() {

}

void Ricoh2A03::nmi() {

}

void Ricoh2A03::rst() {

}


/* Drives the emulation ----------------------------------- */

void Ricoh2A03::step() {

}
