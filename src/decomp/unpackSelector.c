#include "decomp/unpackSelector.h"
#include <tools.h>

void unpackSelector (u16 compression, u8* src, u8* dest, u16 outSizeInBytes) {
    switch(compression) {
        case COMPRESSION_APLIB:
            aplib_unpack(src, dest); // SGDK
            break;
        case COMPRESSION_LZ4W:
            lz4w_unpack(src, dest); // SGDK
            break;
        case FC8:
            fc8Decode(src, dest, TRUE); // m68k version
            // fc8Unpack(src, dest, TRUE); // C version
            break;
        case KOSINSKI:
            // kprintf("src  0x%08X", src);
            // kprintf("dest 0x%08X", dest);
            KosDec(src, dest); // vladikomper: check if A1 points to DEST + DECOMPRESSED SIZE before rts and regs backup
            break;
        case KOSINSKI_PLUS:
            // kprintf("src  0x%08X", src);
            // kprintf("dest 0x%08X", dest);
            KosPlusDec(src, dest); // vladikomper: check if A1 points to DEST + DECOMPRESSED SIZE before rts and regs backup
            break;
        case LZ4:
            lz4_frame_depack(src, dest); // m68k version
            // lz4FrameUnpack(src, dest); // C version
            break;
        case LZKN:
            Kon1Dec(src, dest);
            break;
        case LZKN1:
            Kon1Dec(src, dest);
            break;
        case MEGAPACK:
            
            break;
        case RNC1:
            rnc1_Unpack(src, dest);
            break;
        case RNC2:
            rnc2_Unpack(src, dest);
            break;
        case ROCKET:
            RocketDec(src, dest);
            break;
        case SAXMAN:
            SaxDec(src, dest);
            break;
        case SBZ:
            decompress_sbz(src, dest); // faster m68k version
            // SBZ_decompress(src, dest); // simple m68k version
            break;
        case SNKRLE:
            SNKDec(src, dest);
            break;
        case UFTC:
            uftc_unpack((u16*)src, (u16*)dest, 0, outSizeInBytes/32);
            break;
        case UFTC15:
            uftc15_unpack((s16*)src, (s16*)dest, 0, (s16)outSizeInBytes/32);
            break;
        case UNAPLIB:
            unaplib(src, dest);
            break;
        default:
            break;
    }
}