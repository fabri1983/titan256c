
#include "decomp/fc8Unpack.h"

/// LUT for decoding the copy length parameter
static const u16 _FC8_LENGTH_DECODE_LUT[32] = {
    3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,35,48,72,128,256
};

static u32 GetUInt32 (u8* in) {
    return ((u32)in[0]) << 24 | ((u32)in[1]) << 16 | ((u32)in[2]) << 8 | ((u32)in[3]);
}

static u32 Decode (u8* in, u8* out, u32 outsize) {
    /* Check magic number */
    // if (in[0] != 'F' || in[1] != 'C' || in[2] != '8' || in[3] != '_')
    //     return 0;

    /* Get & check output buffer size */
    // if (outsize < GetUInt32(&in[FC8_DECODED_SIZE_OFFSET]))
    //     return 0;

    /* Initialize the byte streams */
    u8* src = in;
    u8* dst = out;
    
    /* Skip header information */
    src += FC8_HEADER_SIZE;
	
    /* Main decompression loop */
    for(;;) {
        /* Get the next symbol */
        u8 symbol = *src++;

        u8 symbolType = symbol >> 6;
        switch (symbolType) {
            case 0: {
                // LIT = 00aaaaaa  next aaaaaa+1 bytes are literals
                u16 length = symbol+1;
                for (u16 i=length; i--;)
                    *dst++ = *src++;
                break;
            }
            case 1: {
                // BR0 = 01baaaaa  backref offset aaaaa, length b+3
                u16 length = 3 + ((symbol >> 5) & 0x01);
                u32 offset = symbol & 0x1F;
                if (offset == 0)
                    return dst - out;
                for (u16 i=length; i--;) {
                    u8 v = *(dst - offset);
                    *dst++ = v;
                }
                break;
            }
            case 2: {
                // BR1 = 10bbbaaa'aaaaaaaa   backref offset aaa'aaaaaaaa, length bbb+3
                u16 length = 3 + ((symbol >> 3) & 0x07);
                u32  offset = (((u32)(symbol & 0x07)) << 8) | src[0];
                src++;
                for (u16 i=length; i--;) {
                    u8 v = *(dst - offset);
                    *dst++ = v;
                }
                break;
            }
            case 3: {
                // BR2 = 11bbbbba'aaaaaaaa'aaaaaaaa   backref offset a'aaaaaaaa'aaaaaaaa, length lookup_table[bbbbb]
                u16 length = _FC8_LENGTH_DECODE_LUT[(symbol >> 1) & 0x1f];
                u32 offset = (((u32)(symbol & 0x01)) << 16) | (((u32)src[0]) << 8) | src[1];
                src += 2;
                for (u16 i=length; i--;) {
                    u8 v = *(dst - offset);
                    *dst++ = v;
                }
                break;
            }
        }
    }

    return dst - out;
}

u32 fc8Unpack (u8* inBuf, u8* outBuf, bool onlyOneBlock) {
    // check Magick Number
    // if (inBuf[0] != 'F' || inBuf[1] != 'C' || inBuf[2] != '8' || (inBuf[3] != '_' && inBuf[3] != 'b'))
    //     return 0;

    // determine blockSize and numBlocks
    u32 maxOutSize = GetUInt32(&inBuf[FC8_DECODED_SIZE_OFFSET]);
    u32 blockSize = maxOutSize;
    u32 numBlocks = 1;

    if (onlyOneBlock == FALSE) {
        if (inBuf[3] == 'b') {
            blockSize = GetUInt32(&inBuf[FC8_BLOCK_SIZE_OFFSET]);
            numBlocks = (maxOutSize + blockSize - 1) / blockSize;
        }
    }

    u32 outSize = 0;
    for (u32 i=0; i < numBlocks; ++i) {
        u32 blockOffset = 0;
        // decompressing block format?
        if (blockSize != maxOutSize)
            blockOffset = GetUInt32(&inBuf[FC8_BLOCK_HEADER_SIZE + sizeof(u32) * i]);
        u32 processedBlockSize = Decode(inBuf + blockOffset, outBuf + blockSize * i, maxOutSize - blockSize * i);
        // error?
        if (processedBlockSize != blockSize) {
            outSize = 0;
            break;
        }
        outSize += processedBlockSize;
    }

    return outSize;
}