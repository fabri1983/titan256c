#include <types.h>
#include <timer.h>
#include "utils.h"

void waitMs_ (u32 ms) {
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