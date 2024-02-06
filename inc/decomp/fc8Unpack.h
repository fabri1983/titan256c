#ifndef _FC8_UNPACK_H
#define _FC8_UNPACK_H

#include <types.h>

#define FC8_HEADER_SIZE 8
#define FC8_DECODED_SIZE_OFFSET 4

// for FC8b block format header
#define FC8_BLOCK_HEADER_SIZE 12
#define FC8_BLOCK_SIZE_OFFSET 8

/// FC8 C decompressor by Steve Chamberlin 
u32 fc8Unpack (u8* in, u8* out, bool onlyOneBlock);

#endif // _FC8_UNPACK_H