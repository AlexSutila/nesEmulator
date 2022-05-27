#pragma once
#include "memory.hh"

struct nes {

private:

    cpu_bus m_bus;

public:

    void run();

};
