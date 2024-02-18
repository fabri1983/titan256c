#ifndef _BYTEKILLER_H
#define _BYTEKILLER_H

#include <types.h>

/// Bytekiller decompressor m68k implementation. Faster version made by Franck "hitchhikr" Charlet.
extern void bytekiller_depack (u8* src, u8* dest);

void bytekiller_depack_caller (u8* src, u8* dest);

#endif // _BYTEKILLER_H