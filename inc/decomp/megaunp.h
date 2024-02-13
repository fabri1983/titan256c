#ifndef _MEGAUNP_H
#define _MEGAUNP_H

#include <types.h>

extern void init_mega ();

extern void megaunp (u8* data, u8* dest);

void init_mega_caller ();

void megaunp_caller (const u8* data, u8* dest);

#endif // _MEGAUNP_H