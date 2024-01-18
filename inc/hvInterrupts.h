#ifndef HV_INTERRUPTS_H
#define HV_INTERRUPTS_H

#include <types.h>
#include <sys.h>

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

void blackCurrentGradientPtr ();

void vertIntOnTitan256cCallback_HIntEveryN ();

void vertIntOnTitan256cCallback_HIntOneTime ();

HINTERRUPT_CALLBACK horizIntOnTitan256cCallback_CPU_EveryN ();

HINTERRUPT_CALLBACK horizIntOnTitan256cCallback_DMA_EveryN ();

HINTERRUPT_CALLBACK horizIntOnTitan256cCallback_DMA_OneTime ();

#endif