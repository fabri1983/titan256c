#ifndef _FC8_H
#define _FC8_H

#include <types.h>

#define FC8_HEADER_SIZE 8
#define FC8_DECODED_SIZE_OFFSET 4

// for FC8b block format header
#define FC8_BLOCK_HEADER_SIZE 12
#define FC8_BLOCK_SIZE_OFFSET 8

/// FC8 decompressor. Uses asm m68k version of the block decoder.
void fc8Decode (u8* inBuf, u8* outBuf, bool onlyOneBlock);

/// FC8 m68k decompressor by Steve Chamberlin.
/// Returns 1 if decompression was successful, or 0 upon failure.
extern u16 fc8_decode_block (u8* in, u8* out, u32 outsize);

/// FC8 m68k decompressor from https://github.com/leuat/TRSE.
/// Returns 1 if decompression was successful, or 0 upon failure.
extern u16 fc8_decode_block_tru (u8* in, u8* out);

#endif // _FC8_H