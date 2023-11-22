#ifndef UTILS_H
#define UTILS_H

#include <types.h>
#include <timer.h>

#define CLAMP(x, minimum, maximum) ( min(max((x),(minimum)),(maximum)) )

#define SIGN(x) ( (x > 0) - (x < 0) )

/// @brief Waits for a certain amount of millisecond (~3.33 ms based timer when wait is >= 100ms). 
/// Lightweight implementation without calling SYS_doVBlankProcess().
/// This method CAN NOT be called from V-Int callback or when V-Int is disabled.
/// @param ms >= 100ms
void waitMillis (u32 ms) {
	u32 tick = (ms * TICKPERSECOND) / 1000;
	const u32 start = getTick();
    u32 max = start + tick;
    u32 current;

    // need to check for overflow
    if (max < start) max = 0xFFFFFFFF;

    // wait until we reached subtick
    do {
        current = getTick();
    }
    while (current < max);
}

#endif