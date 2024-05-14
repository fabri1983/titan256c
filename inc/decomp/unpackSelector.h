#ifndef _UNPACK_SELECTOR_H
#define _UNPACK_SELECTOR_H

#include <types.h>
#include "compressionTypes_res.h"
#include "decomp/bytekiller.h"
#include "decomp/clownnemesis.h"
#include "decomp/comper.h"
#include "decomp/comperx.h"
#include "decomp/elektro.h"
#include "decomp/enigma.h"
#include "decomp/fc8.h"
#include "decomp/fc8Unpack.h"
#include "decomp/kosinski.h"
#include "decomp/kosinskiplus.h"
#include "decomp/lz4.h"
#include "decomp/lz4tiny.h"
#include "decomp/lzkn1.h"
#include "decomp/megaunp.h"
#include "decomp/nemesis.h"
#include "decomp/nibbler.h"
#include "decomp/rnc.h"
#include "decomp/rocket.h"
#include "decomp/saxman.h"
#include "decomp/sbz.h"
#include "decomp/snkrle.h"
#include "decomp/uftc.h"
#include "decomp/unaplib.h"
#include "decomp/zx0.h"

void unpackSelector (u16 compression, u8* src, u8* dest, u16 outSizeInBytes);

#endif // _UNPACK_SELECTOR_H