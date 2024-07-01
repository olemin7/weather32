#pragma once

namespace blink {
enum class led_state_e {
    OFF,
    ON,
    SLOW,
    FAST,
};

void init();
void set(led_state_e);
}; // namespace blink