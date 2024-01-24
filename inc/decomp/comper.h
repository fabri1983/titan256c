#ifndef _COMPER_H
#define _COMPER_H

#include <types.h>

/// Comper decompressor
extern void ComperDec (u8* src, u8* dest);

/// Comper X decompressor
extern void ComperXDec (u8* src, u8* dest);

/// Modular ComperX decompressor
extern void ComperXMDec (u8* src, u8* dest);

#endif // _COMPER_H