//***************************************************************************
// "uftc.h"
// Header file containing the prototype for the UFTC decompression function
// You should include this header when using this function
//***************************************************************************

#ifndef UFTC_H
#define UFTC_H

#include <types.h>

#ifdef __GNUC__
#define ASM_STATEMENT __asm__
#elif defined(_MSC_VER)
#define ASM_STATEMENT __asm
#endif

/// UFTC decompressor
void decompress_uftc(u16* out, u16* in, u16 start, u16 count);

/// UFTC15 decompressor
void decompress_uftc15(s16* out, s16* in, s16 start, s16 count);

#endif