#ifndef SEGA_LOGO_H
#define SEGA_LOGO_H

#include "logo_res.h"

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

#ifdef __GNUC__
#define INTERRUPT_ATTRIBUTE HINTERRUPT_CALLBACK
#else
#define INTERRUPT_ATTRIBUTE void
#endif

void displaySegaLogo ();

#endif