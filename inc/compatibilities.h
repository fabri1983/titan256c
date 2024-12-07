#ifndef _COMPATIBILITIES_H
#define _COMPATIBILITIES_H

/**
 *  \brief
 *      Declare function for the hint callback (generate a RTE to return from interrupt instead of RTS)
 */
#ifdef __GNUC__
#define HINTERRUPT_CALLBACK __attribute__ ((interrupt)) void
#elif defined(_MSC_VER)
#define HINTERRUPT_CALLBACK void
#endif

/**
 *  \brief
 *      To force method inlining (not sure that GCC does actually care of it)
 */
#ifdef __GNUC__
#define FORCE_INLINE inline __attribute__ ((always_inline))
#elif defined(_MSC_VER)
#define FORCE_INLINE inline __forceinline
#endif

/**
 *  \brief
 *      To force no inlining for this method
 */
#ifdef __GNUC__
#define NO_INLINE __attribute__ ((noinline))
#elif defined(_MSC_VER)
#define NO_INLINE __declspec(noinline)
#endif

#ifdef __GNUC__
#define VOID_OR_CHAR void
#elif defined(_MSC_VER)
#define VOID_OR_CHAR char
#endif

#define MEMORY_BARRIER() __asm volatile ("" : : : "memory")

#endif // _COMPATIBILITIES_H