#ifndef _UNPACK_SELECTOR_H
#define _UNPACK_SELECTOR_H

#include <types.h>
#include "compressionTypes_res.h"
#include "decomp/comper.h"
#include "decomp/fc8.h"
#include "decomp/fc8Unpack.h"
#include "decomp/kosinski.h"
#include "decomp/lz4.h"
#include "decomp/lz4Unpack.h"
#include "decomp/lzkn.h"
#include "decomp/rnc.h"
#include "decomp/rocket.h"
#include "decomp/saxman.h"
#include "decomp/snkrle.h"
#include "decomp/uftc.h"
#include "decomp/unaplib.h"

void unpackSelector (u16 compression, u8* src, u8* dest, u16 outSizeInBytes);

#endif // _UNPACK_SELECTOR_H