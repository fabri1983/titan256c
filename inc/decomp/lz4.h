#ifndef _LZ4_H
#define _LZ4_H

#include <types.h>

/// LZ4-68k: Arnaud Carré's depacker
extern void lz4_frame_depack (u8* src, u8* dest);

#endif // _LZ4_H