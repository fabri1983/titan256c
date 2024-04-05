#ifndef _RNC_H
#define _RNC_H

#include <types.h>

/// RNC-1: Rob Northen method 1 decompressor
extern void rnc1c_Unpack (u8* src, u8* dest);

/// RNC-2: Rob Northen method 2 decompressor
extern u32 rnc2_Unpack (u16 key, u8 *src, u8 *dest);

/// RNC-2 compact: Rob Northen method 2 decompressor
extern void rnc2c_Unpack (u8* src, u8* dest);

#endif // _RNC_H