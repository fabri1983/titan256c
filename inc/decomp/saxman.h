#ifndef _SAXMAN_H
#define _SAXMAN_H

#include <types.h>

/// Saxman decompressor
extern void SaxDec (u8* src, u8* dest);

/// Saxman with length decompressor
extern void SaxDec2 (u8* src, u8* dest, u16 length);

#endif // _SAXMAN_H