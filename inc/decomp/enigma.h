#ifndef _ENIGMA_H
#define _ENIGMA_H

#include <types.h>

/// Enigma decompressor ASM m68k version.
extern void EniDec (u16 mapBaseTileIndex, u8* in, u8* out);

/// Enigma decompressor ASM m68k version optimized by RealMalachi and Orion
extern void EniDec_opt (u16 mapBaseTileIndex, u8* in, u8* out);

#endif // _ENIGMA_H