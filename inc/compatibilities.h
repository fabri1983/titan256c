#ifndef _COMPATIBILITIES_H
#define _COMPATIBILITIES_H

#ifdef __GNUC__
#define HINTERRUPT_CALLBACK __attribute__((interrupt)) void
#elif defined(_MSC_VER)
// Declare function for the hint callback (generate a RTE to return from interrupt instead of RTS)
#define HINTERRUPT_CALLBACK void
#endif

#ifdef __GNUC__
#define FORCE_INLINE inline __attribute__((always_inline))
#elif defined(_MSC_VER)
// To force method inlining (not sure that GCC does actually care of it)
#define FORCE_INLINE inline __forceinline
#endif

#ifdef __GNUC__
#define NO_INLINE __attribute__((noinline))
#elif defined(_MSC_VER)
// To force no inlining for this method
#define NO_INLINE __declspec(noinline)
#endif

#ifdef __GNUC__
#define VOID_OR_CHAR void
#elif defined(_MSC_VER)
#define VOID_OR_CHAR char
#endif

#define MEMORY_BARRIER() __asm volatile ("" : : : "memory")

#endif // _COMPATIBILITIES_H