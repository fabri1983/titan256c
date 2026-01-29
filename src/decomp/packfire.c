#include <types.h>
#include <sys.h>
#include <memory.h>
#include "decomp/packfire.h"

#include "compressionTypesTracker.h"
#if defined(USING_PACKFIRE_TINY) || defined(USING_PACKFIRE_LARGE)

void depacker_large_caller (u8* in, u8* out)
{
    u8* buf = MEM_alloc(15980);
    depacker_large(in, out, buf);
    MEM_free(buf);
}

#endif // USING_NIBBLER