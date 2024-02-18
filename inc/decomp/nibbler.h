#ifndef _NIBBLER_H
#define _NIBBLER_H

#include <types.h>

/// Nibbler decompressor m68k implementation by Henrik Erlandsson.
extern void Denibble (u8* src, u8* dest);

/// Caller for the Nibbler decompressor.
void Denibble_caller (u8* src, u8* dest);

#endif // _NIBBLER_H