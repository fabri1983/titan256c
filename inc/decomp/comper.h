#ifndef _COMPER_H
#define _COMPER_H

#include <types.h>

/// Comper decompressor m68k implementation
extern void ComperDec (u8* src, u8* dest);

/// Comper X decompressor m68k implementation
extern void ComperXDec (u8* src, u8* dest);

/// Modular ComperX decompressor m68k implementation
extern void ComperXMDec (u8* src, u8* dest);

/// Comper X decompressor caller
void comperx_dec (u8* src, u8* dest);

/// Modular ComperX decompressor caller
void comperxm_dec (u8* src, u8* dest);

#endif // _COMPER_H