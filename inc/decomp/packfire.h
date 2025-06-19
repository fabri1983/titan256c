#ifndef _PACKFIRE_H
#define _PACKFIRE_H

#include <types.h>

/// PackFire 1.2k - (tiny depacker)
extern void depacker_tiny (u8* in, u8* out);

/// PackFire 1.2k - (large depacker)
/// buf must be 15980 bytes.
extern void depacker_large (u8* in, u8* out, u8* buf);

/// PackFire 1.2k - (large depacker)
/// It uses a temporal buffer of 15980 bytes.
void depacker_large_caller (u8* in, u8* out);

#endif // _PACKFIRE_H