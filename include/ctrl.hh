#pragma once

#include <cstdint>

/* Original NES control pad */

struct Controller {

private:

    bool m_btnStates[8];
    uint8_t m_shift;

public:

    Controller();

    void update(const uint8_t *keystate);
    uint8_t r_joypad() /* --- */;
    void w_joypad(uint8_t value);

};
