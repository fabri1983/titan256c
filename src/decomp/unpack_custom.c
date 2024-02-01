#include "decomp/unpack_custom.h"
#include <tools.h>
#include "compressionTypes_res.h"
#include "decomp/comper.h"
#include "decomp/kosinski.h"
#include "decomp/lz4.h"
#include "decomp/rnc.h"
#include "decomp/rocket.h"
#include "decomp/saxman.h"
#include "decomp/snkrle.h"
#include "decomp/uftc.h"
#include "decomp/unaplib.h"

void unpack_custom (u16 compression, u8* src, u8* dest, u16 count) {
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
            // KLog_U1("dest aftr ", dest); // vladikomper: check if A1 points to DEST + DECOMPRESSED SIZE before rts and regs backup
            break;
        case KOSINSKI_PLUS:
            // KLog_U1("src       ", src);
            // KLog_U1("dest befr ", dest);
            KosPlusDec(src, dest);
            // KLog_U1("src       ", src);
            // KLog_U1("dest aftr ", dest); // vladikomper: check if A1 points to DEST + DECOMPRESSED SIZE before rts and regs backup
            break;
        case LZ4:
            lz4_frame_depack(src, dest);
            break;
        case RNC_1:
            rnc1_Unpack(src, dest);
            break;
        case RNC_2:
            rnc2_Unpack(src, dest);
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
        case UFTC:
            uftc_unpack((u16*)src, (u16*)dest, 0, count);
            break;
        case UFTC_15:
            uftc15_unpack((s16*)src, (s16*)dest, 0, (s16)count);
            break;
        case UNAPLIB:
            unaplib(src, dest);
            break;
        default:
            break;
    }
}