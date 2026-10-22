#include "input.h"

void input_from_held(KeyRepeat *r, uint32_t held, Input *out)
{
    out->held = held;
    out->down = held & ~r->prev;
    out->rep = 0;
    for (int k = 0; k < K_COUNT; k++) {
        uint32_t bit = KBIT(k);
        if (out->down & bit) {
            r->count[k] = 0;
            out->rep |= bit;
        } else if (held & bit) {
            r->count[k]++;
            if (r->count[k] >= REPEAT_DELAY + 100 * REPEAT_RATE)
                r->count[k] -= 100 * REPEAT_RATE; // keeps the phase, never overflows
            if (r->count[k] >= REPEAT_DELAY && (r->count[k] - REPEAT_DELAY) % REPEAT_RATE == 0)
                out->rep |= bit;
        }
    }
    r->prev = held;
}
