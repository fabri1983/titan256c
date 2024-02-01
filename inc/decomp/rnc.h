#ifndef _RNC_H
#define _RNC_H

#include <types.h>

/// RNC-1: Rob Northen method 1 decompressor
extern void rnc1_Unpack (u8* src, u8* dest);

/// RNC-2: Rob Northen method 2 decompressor
extern void rnc2_Unpack (u8* src, u8* dest);

#endif // _RNC_H