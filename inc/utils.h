#ifndef _UTILS_H
#define _UTILS_H

#include <types.h>

#define MEMORY_BARRIER() __asm volatile ("" : : : "memory")

#define CLAMP(x, minimum, maximum) ( min(max((x),(minimum)),(maximum)) )

#define SIGN(x) ( (x > 0) - (x < 0) )

/// @brief Waits for a certain amount of millisecond (~3.33 ms based timer when wait is >= 100ms). 
/// Lightweight implementation without calling SYS_doVBlankProcess().
/// This method CAN NOT be called from V-Int callback or when V-Int is disabled.
/// @param ms >= 100ms, otherwise use waitMs() from timer.h
void waitMs_ (u32 ms);

#endif // _UTILS_H