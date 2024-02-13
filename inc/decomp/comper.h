#ifndef _COMPER_H
#define _COMPER_H

#include <types.h>

/// Comper decompressor m68k implementation
extern void ComperDec (u8* src, u8* dest);

/// Caller for the Comper decompressor
void ComperDec_caller (u8* src, u8* dest);

#endif // _COMPER_H