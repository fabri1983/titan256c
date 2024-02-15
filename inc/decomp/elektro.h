#ifndef _ELEKTRO_H
#define _ELEKTRO_H

#include <types.h>

/// Elektro decompressor m68k implementation made by r57shell
extern void elektro_unpack (u8* src, u8* dest);

void elektro_unpack_caller (u8* src, u8* dest);

#endif // _ELEKTRO_H