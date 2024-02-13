#include "decomp/unpackSelector.h"
#include "compressionTypesTracker.h"
#include <tools.h> // constants: COMPRESSION_APLIB and COMPRESSION_LZ4W

#ifdef USING_MEGAPACK
static bool megainitCalled = FALSE;
#endif

void unpackSelector (u16 compression, u8* src, u8* dest, u16 outSizeInBytes) {
    switch(compression) {
        case COMPRESSION_APLIB:
            aplib_unpack(src, dest); // SGDK
            break;
        case COMPRESSION_LZ4W:
            lz4w_unpack(src, dest); // SGDK
            break;
        #ifdef USING_COMPER
        case COMPER:
            ComperDec_caller(src, dest);
            break;
        #endif
        #ifdef USING_COMPERX
        case COMPERX:
            ComperXDec_caller(src, dest);
            break;
        #endif
        #ifdef USING_COMPERXM
        case COMPERXM:
            ComperXMDec_caller(src, dest);
            break;
        #endif
        #ifdef USING_FC8
        case FC8:
            fc8Decode(src, dest, TRUE); // m68k version
            // fc8Unpack(src, dest, TRUE); // C version
            break;
        #endif
        #ifdef USING_KOSINSKI
        case KOSINSKI:
            KosDec(src, dest);
            break;
        #endif
        #ifdef USING_KOSINSKI_PLUS
        case KOSINSKI_PLUS:
            KosPlusDec(src, dest);
            break;
        #endif
        #ifdef USING_LZ4
        case LZ4:
            lz4_fastest_caller(src, dest); // m68k version
            // lz4FrameUnpack(src, dest); // C tiny version
            break;
        #endif
        #if defined(USING_LZKN) || defined(USING_LZKN1)
        case LZKN:
        case LZKN1:
            Kon1Dec(src, dest);
            break;
        #endif
        #ifdef USING_MEGAPACK
        case MEGAPACK:
            if (megainitCalled == FALSE) {
                init_mega();
                megainitCalled = TRUE;
            }
            megaunp(src, dest);
            break;
        #endif
        #ifdef USING_RNC1
        case RNC1:
            rnc1_Unpack(src, dest);
            break;
        #endif
        #ifdef USING_RNC2
        case RNC2:
            rnc2_Unpack(src, dest);
            break;
        #endif
        #ifdef USING_ROCKET
        case ROCKET:
            RocketDec(src, dest);
            break;
        #endif
        #ifdef USING_SAXMAN
        case SAXMAN:
            SaxDec(src, dest);
            break;
        #endif
        #ifdef USING_SBZ
        case SBZ:
            // SBZ_blob_decompress(src, dest); // simple m68k version
            SBZ_decompress_caller(src, dest); // faster m68k version
            break;
        #endif
        #ifdef USING_SNKRLE
        case SNKRLE:
            SNKDec(src, dest);
            break;
        #endif
        #ifdef USING_UFTC
        case UFTC:
            uftc_unpack((u16*)src, (u16*)dest, 0, outSizeInBytes/32);
            break;
        #endif
        #ifdef USING_UFTC15
        case UFTC15:
            uftc15_unpack((s16*)src, (s16*)dest, 0, (s16)outSizeInBytes/32);
            break;
        #endif
        #ifdef USING_UNAPLIB
        case UNAPLIB:
            unaplib(src, dest);
            break;
        #endif
        default:
            break;
    }
}