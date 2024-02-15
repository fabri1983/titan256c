#ifndef _NEMESIS_H
#define _NEMESIS_H

#include <types.h>

extern void NemDec_RAM (u8* in, u8* out);

/// A VDP command to write to the destination VRAM address must be issued before calling this routine
extern void NemDec_VDP (u8* in);

#endif // _NEMESIS_H