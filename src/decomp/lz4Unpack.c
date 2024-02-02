/*---------------------------------------------------------
;
;	LZ4 block tiny C depacker
;	Written by Arnaud Carré ( @leonard_coder )
;	https://github.com/arnaud-carre/lz4-68k
;
;	LZ4 technology by Yann Collet ( https://lz4.github.io/lz4/ )
;
;---------------------------------------------------------*/
#include "decomp/lz4Unpack.h"

static u32 lz4_rlen (u32 size, u8* ppRead)
{
	if (15 == size)
	{
		u8 v;
		do
		{
			v = *ppRead++;
			size += v;
		}
		while (0xff == v);
	}
	return size;
}

s32	lz4BlockUnpack (u8* src, u8* out, u32 srcSize)
{
	const u8* outStart = out;
	const u8* lz4BlockEnd = src + srcSize;
	for (;;)
	{
		const u8 token = *src++;
		u32 count = (u32)(token >> 4);
		if (count > 0)
		{
			count = lz4_rlen(count, src);
			for (u32 i = 0; i < count; i++)
				*out++ = *src++;

			if (src == lz4BlockEnd)
				break;
		}

		// match len
		u32 offset = src[0] | ((u32)(src[1]) << 8);	// valid on both little or big endian machine
		src += 2;
		count = lz4_rlen(token & 15, src) + 4;
		u8* r = out - offset;
		for (u32 i = 0; i < count; i++)
			*out++ = *r++;
	}
	return (s32)(out - outStart);
}

static u32 readU32LittleEndian (u8* src)
{
	return ( ((u32)(src[3]) << 24) | ((u32)(src[2]) << 16) | ((u32)(src[1]) << 8) | ((u32)(src[0]) << 0) );
}

s32 lz4FrameUnpack (u8* src, u8* dst)
{
	if (readU32LittleEndian(src) != 0x184D2204)
		return -1;

	if (0x40 != (src[4] & 0xc9))	// check version, no depacked size, and no DictID
		return -1;

	const u32 packedSize = readU32LittleEndian(src + 7);
	return lz4BlockUnpack(src+7+4, dst, packedSize);
}