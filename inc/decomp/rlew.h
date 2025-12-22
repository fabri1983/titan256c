#ifndef _RLEWMAP_H
#define _RLEWMAP_H

#include <types.h>

/// @brief Method A: lower compression ratio but faster decompression time.
/// Decompress a RLEW compressed stream of N rows and M words each row, and applies an offset in the out 
/// buffer at the end of each decompressed row only if target is a tilemap with extended width.
/// @param jumpGap Is the number of bytes to jump once the end of a row is reached on the output buffer.
/// @param in 
/// @param out 
void rlew_decomp_A (const u8 jumpGap, u8* in, u8* out);

/// @brief Method A: lower compression ratio but faster decompression time.
/// Decompress a RLEW compressed stream of N rows and M words each row, and applies an offset in the out 
/// buffer at the end of each decompressed row only if target is a tilemap with extended width.
/// @param jumpGap Is the number of bytes to jump once the end of a row is reached on the output buffer.
/// @param in 
/// @param out 
extern void rlew_decomp_A_asm (const u8 jumpGap, u8* in, u8* out);

/// @brief Method B: higher compression ratio but slower decompression time.
/// Decompress a RLEW compressed stream of N rows and M words each row, and applies an offset in the out 
/// buffer at the end of each decompressed row only if target is a tilemap with extended width.
/// @param jumpGap Is the number of bytes to jump once the end of a row is reached on the output buffer.
/// @param in 
/// @param out 
void rlew_decomp_B (const u8 jumpGap, u8* in, u8* out);

/// @brief Method B in ASM: higher compression ratio but slower decompression time.
/// Decompress a RLEW compressed stream of N rows and M words each row, and applies an offset in the out 
/// buffer at the end of each decompressed row only if target is a tilemap with extended width.
/// @param jumpGap Is the number of bytes to jump once the end of a row is reached on the output buffer.
/// @param in 
/// @param out 
extern void rlew_decomp_B_asm (const u8 jumpGap, u8* in, u8* out);

#endif // _RLEWMAP_H