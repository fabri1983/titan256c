#include "decomp/unpack_custom.h"
#include <tools.h>
#include "titan256c_res.h"
#include "decomp/comper.h"
#include "decomp/kosinski.h"
#include "decomp/rocket.h"
#include "decomp/saxman.h"
#include "decomp/snkrle.h"
#include "decomp/uftc.h"

void unpack_custom (u16 compression, u8* src, u8* dest) {
    switch(compression) {
        case COMPRESSION_APLIB:
            aplib_unpack(src, dest);
            break;
        case COMPRESSION_LZ4W:
            lz4w_unpack(src, dest);
            break;
        case KOSINSKI:
            // KLog_U1("src       ", src);
            // KLog_U1("dest befr ", dest);
            KosDec(src, dest);
            // KLog_U1("src       ", src);
            // KLog_U1("dest aftr ", dest); // vladikomper: check if A1 points to DEST + DECOMPRESSED SIZE after decompression
            break;
        case KOSINSKI_PLUS:
            // KLog_U1("src       ", src);
            // KLog_U1("dest befr ", dest);
            KosPlusDec(src, dest);
            // KLog_U1("src       ", src);
            // KLog_U1("dest aftr ", dest); // vladikomper: check if A1 points to DEST + DECOMPRESSED SIZE after decompression
            break;
        case ROCKET:
            RocketDec(src, dest);
            break;
        case SAXMAN:
            SaxDec(src, dest);
            break;
        case SNKRLE:
            SNKDec(src, dest);
            break;
        default:
            break;
    }
}