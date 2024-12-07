#include "decomp/unpackSelector.h"
#include "compressionTypesTracker.h"
#include <tools.h> // constants: COMPRESSION_APLIB and COMPRESSION_LZ4W
#include "compatibilities.h"

#ifdef USING_MEGAPACK
static bool megainitCalled = FALSE;
#endif

FORCE_INLINE void unpackSelector (u16 compression, u8* src, u8* dest, u16 outSizeInBytes) {
    switch (compression) {
        #ifdef USING_APLIB
        case COMPRESSION_APLIB:
            aplib_unpack(src, dest); // SGDK
            break;
        #endif
        #ifdef USING_LZ4W
        case COMPRESSION_LZ4W:
            lz4w_unpack(src, dest); // SGDK
            break;
        #endif
        #ifdef USING_BYTEKILLER
        case BYTEKILLER:
            bytekiller_depack_caller(src, dest);
            break;
        #endif
        #ifdef USING_CLOWNNEMESIS
        case CLOWNNEMESIS:
            ClownNemesis_Decompress(src, dest);
            break;
        #endif
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
        #ifdef USING_ELEKTRO
        case ELEKTRO:
            elektro_unpack_caller(src, dest);
            break;
        #endif
        #ifdef USING_ENIGMA
        case ENIGMA:
            //EniDec(0, src, dest); // 0 acts as a mapBaseTileIndex, which we don't use here
            EniDec_opt(0, src, dest); // 0 acts as a mapBaseTileIndex, which we don't use here
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
        #if defined(USING_LZKN1_MDCOMP) || defined(USING_LZKN1_R57SHELL) || defined(USING_LZKN1_VLADIKCOMPER)
        case LZKN1_MDCOMP:
        case LZKN1_R57SHELL:
        case LZKN1_VLADIKCOMPER:
            Kon1Dec(src, dest);
            break;
        #endif
        #ifdef USING_MEGAPACK
        case MEGAPACK:
            // TODO: need to test if this needs to be called once or everytime
            if (megainitCalled == FALSE) {
                init_mega();
                megainitCalled = TRUE;
            }
            megaunp(src, dest);
            break;
        #endif
        #ifdef USING_NEMESIS
        case NEMESIS:
            NemDec_RAM(src, dest);
            break;
        #endif
        #ifdef USING_NIBBLER
        case NIBBLER:
            Denibble(src, dest);
            break;
        #endif
        #ifdef USING_RNC1
        case RNC1:
            rnc1c_Unpack(src, dest);
            break;
        #endif
        #ifdef USING_RNC2
        case RNC2:
            // rnc2c_Unpack(src, dest);
            rnc2_Unpack(0, src, dest);
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
        #ifdef USING_ZX0
        case ZX0:
            zx0Dec(src, dest);
            break;
        #endif
        default:
            break;
    }
}