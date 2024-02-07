//***************************************************************************
// "uftc.h"
// Header file containing the prototype for the UFTC decompression function
// You should include this header when using this function
//***************************************************************************

#ifndef UFTC_H
#define UFTC_H

#include <types.h>

/// UFTC decompressor
void uftc_unpack (u16* out, u16* in, u16 start, u16 count);

/// UFTC15 decompressor
void uftc15_unpack (s16* out, s16* in, s16 start, s16 count);

#endif