#ifndef _HIVERLE_H
#define _HIVERLE_H

#include <types.h>

/// Decompress HiveRLE archive to RAM.
extern void HiveDec (u8* in, u8* out);

/// Decompress HiveRLE archive to VRAM.
/// It assumes VDP Auto Inc is already 2.
/// vram address already preconverted to VDP instruction. Eg: VDP_WRITE_VRAM_ADDR(1) indicates VRAM tile 1
extern void HiveDecVRAM (u32 vram, u8* in, u8* buf);

#endif // _HIVERLE_H