#ifndef _LZKN1_H
#define _LZKN1_H

#include <types.h>

/// LZ Konami Variation 1 Decompressor
extern void Kon1Dec (u8* src, u8* dest);

/// LZ Konami Variaton 1 Decompressor. Skips data size in compressed stream.
extern void Kon1Dec2 (u8* src, u8* dest);

#endif // _LZKN1_H