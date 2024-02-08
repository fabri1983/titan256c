#include "decomp/saxman.h"

u16 Sax2_getLength (u8* src) {
    u16 lenWord = *((u16*)src);
    // This seems to be a Little Endian conversion on a byte basis. Is what the asm m68k code does.
    return (lenWord << 8) | (lenWord & 0xFF);
}