#ifndef _SBZ_H
#define _SBZ_H

#include <types.h>

/*! 
 *  \brief Simple implementation. Ian Karlsson SBZ Decompressor. Decompress data using SBZ algorithm.
 *  \param   in   Pointer to input data. Must be word aligned!
 *  \param   out  Pointer to destination memory address.
 *  \return       Pointer to the end of decompressed data.
 */
u8* SBZ_decompress(const u8* in, u8* out);

/// @brief ASM m68k implementation. Ian Karlsson SBZ Decompressor.
extern void decompress_sbz (u8* in, u8* out);

#endif