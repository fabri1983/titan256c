#ifndef _SAXMAN_H
#define _SAXMAN_H

#include <types.h>

/// Saxman decompressor
extern void SaxDec (u8* src, u8* dest);

/// Saxman decompressor with length as parameter
extern void SaxDec2 (u8* src, u8* dest, u16 length);

/// Extracts the length of the data out of from the src
u16 Sax2_getLength (u8* src);

#endif // _SAXMAN_H