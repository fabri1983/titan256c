#ifndef _CLOWNNEMESIS_H
#define _CLOWNNEMESIS_H

#include <types.h>

/// Clownnemeses decompressor.
/// Returns 0 on error, 1 on success.
u16 ClownNemesis_Decompress (u8* src, u8* dest);

#endif // _CLOWNNEMESIS_H