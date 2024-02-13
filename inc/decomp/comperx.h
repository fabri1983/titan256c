#ifndef _COMPERX_H
#define _COMPERX_H

#include <types.h>

/// Comper X decompressor m68k implementation
extern void ComperXDec (u8* src, u8* dest);

/// Modular ComperX decompressor m68k implementation
extern void ComperXMDec (u8* src, u8* dest);

/// Caller for the Comper X decompressor
void ComperXDec_caller (u8* src, u8* dest);

/// Caller for the Modular ComperX decompressor
void ComperXMDec_caller (u8* src, u8* dest);

#endif // _COMPERX_H