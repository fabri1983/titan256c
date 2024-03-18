#ifndef _COMPATIBILITIES_H
#define _COMPATIBILITIES_H

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
#define VOID_OR_CHAR void
#elif defined(_MSC_VER)
#define VOID_OR_CHAR char
#endif

#ifdef __GNUC__
#define INTERRUPT_ATTRIBUTE HINTERRUPT_CALLBACK
#else
#define INTERRUPT_ATTRIBUTE void
#endif

#define MEMORY_BARRIER() ASM_STATEMENT volatile ("" : : : "memory")

#endif // _COMPATIBILITIES_H