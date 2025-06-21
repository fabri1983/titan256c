#ifndef _TWIZZLER_H
#define _TWIZZLER_H

#include <types.h>

/// Twizzler decompression.
extern void TwizDec (u8* in, u8* out);

/// Twizzler (Module) decompression.
/// vram address (I assume it must be multiple of 0x20)
extern void TwimDec (u16 vram, u8* in);

#endif // _TWIZZLER_H