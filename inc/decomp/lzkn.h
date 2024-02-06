#ifndef _LZKN_H
#define _LZKN_H

#include <types.h>

/// LZ Konami Variation 1 Decompressor
extern void Kon1Dec (u8* src, u8* dest);

/// LZ Konami Variaiton 1 Decompressor. Skips data size in compressed stream.
extern void Kon1Dec2 (u8* src, u8* dest);

#endif // _LZKN_H