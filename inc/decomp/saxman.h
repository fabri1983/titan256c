#ifndef _SAXMAN_H
#define _SAXMAN_H

#include <types.h>

/// Saxman decompressor
extern void SaxDec (u8* src, u8* dest);

/// Saxman with length decompressor
extern void SaxDec2 (u8* src, u8* dest, u16 length);

u16 Sax2_getLength (u8* src) {
    u16 lenWord = *((u16*)src);
    // This seems to be a Little Endian conversion on a byte basis. Is what the asm m68k code does.
    return (lenWord << 8) | (lenWord & 0xFF);
}

#endif // _SAXMAN_H