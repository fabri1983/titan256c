#ifndef _UNPACK_SELECTOR_H
#define _UNPACK_SELECTOR_H

#include <types.h>

/**
 * additionalArg is useful to send whatever extra parameter the decompresison algorithm needs, 
 * like decompressed size or an adjustment value.
 */
void unpackSelector (u16 compression, u8* src, u8* dest, u16 additionalArg);

#endif // _UNPACK_SELECTOR_H