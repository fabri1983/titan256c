#ifndef _KOSINSKI_PLUS_H
#define _KOSINSKI_PLUS_H

#include <types.h>

#ifdef __GNUC__
#define FORCE_INLINE            inline __attribute__ ((always_inline))
#elif defined(_MSC_VER)
#define FORCE_INLINE            inline __forceinline
#endif

#ifdef __GNUC__
#define NO_INLINE               __attribute__ ((noinline))
#elif defined(_MSC_VER)
#define NO_INLINE               __declspec(noinline)
#endif

#ifdef __GNUC__
#define ASM_STATEMENT __asm__
#elif defined(_MSC_VER)
#define ASM_STATEMENT __asm
#endif

/// Kosinski decompressor
extern void KosDec (u8* src, u8* dest);

/// Kosinski Plus decompressor
extern void KosPlusDec (u8* src, u8* dest);

#endif // _KOSINSKI_PLUS_H