#include "decomp/fc8.h"
#include "compatibilities.h"

#include "compressionTypesTracker.h"
#ifdef USING_FC8

static u32 GetUInt32 (u8* in) {
    return ((u32)in[0]) << 24 | ((u32)in[1]) << 16 | ((u32)in[2]) << 8 | ((u32)in[3]);
}

void fc8Decode (u8* inBuf, u8* outBuf, bool onlyOneBlock) {
    // check Magick Number
    // if (inBuf[0] != 'F' || inBuf[1] != 'C' || inBuf[2] != '8' || (inBuf[3] != '_' && inBuf[3] != 'b'))
    //     return;

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
    
    for (u32 i=0; i < numBlocks; ++i) {
        u32 blockOffset = 0;
        // decompressing block format?
        if (blockSize != maxOutSize)
            blockOffset = GetUInt32(&inBuf[FC8_BLOCK_HEADER_SIZE + sizeof(u32) * i]);
        u16 success = fc8_decode_block(maxOutSize - blockSize * i, inBuf + blockOffset, outBuf + blockSize * i);
        //u16 success = fc8_decode_block_tru(inBuf + blockOffset, outBuf + blockSize * i);
        // error?
        if (success == 0)
            break; // something wrong happened
    }
}

#endif // USING_FC8