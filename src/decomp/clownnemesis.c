#include <types.h>
#include <memory.h>
#include "decomp/clownnemesis.h"
#include "compatibilities.h"

#ifndef CC_COUNT_OF
#define CC_COUNT_OF(array) (sizeof(array) / sizeof((array)[0]))
#endif

typedef struct StateCommon
{
	u8 *read_byte_user_data;
	u8 *write_byte_user_data;
} StateCommon;

static u8 ReadByte(StateCommon* state)
{
	return *state->read_byte_user_data++;
}

static void WriteByte(StateCommon* state, u8 byte)
{
    *state->write_byte_user_data++ = byte;
}

static void InitialiseCommon(StateCommon* state, u8* read_byte_user_data, u8* write_byte_user_data)
{
	state->read_byte_user_data = read_byte_user_data;
	state->write_byte_user_data = write_byte_user_data;
}

#define MAXIMUM_CODE_BITS 8

typedef struct NybbleRun
{
	u8 total_code_bits;
	u8 value;
	u8 length;
} NybbleRun;

typedef struct State
{
	StateCommon common;

	NybbleRun nybble_runs[1 << MAXIMUM_CODE_BITS];

	u32 output_buffer, previous_output_buffer;
	u8 output_buffer_nybbles_done;

	bool xor_mode_enabled;
	u16 total_tiles;

	u8 bits_available;
	u8 bits_buffer;
} State;

static u16 PopBit(State* state)
{
	state->bits_buffer <<= 1;

	if (state->bits_available == 0)
	{
		state->bits_available = 8;
		state->bits_buffer = ReadByte(&state->common);
	}

	--state->bits_available;

	return (state->bits_buffer & 0x80) != 0;
}

static u16 PopBits(State* state, u16 total_bits)
{
	u16 value;
	u16 i;

	value = 0;

	for (i = 0; i < total_bits; ++i)
	{
		value <<= 1;
		value |= PopBit(state);
	}

	return value;
}

static bool NybbleRunExists(NybbleRun* nybble_run)
{
	return nybble_run->length != 0;
}

static NybbleRun* FindCode(State* state)
{
	u16 code = 0, total_code_bits = 0;

	for (;;)
	{
		if (total_code_bits == MAXIMUM_CODE_BITS)
		{
            return NULL;
		}

		code <<= 1;
		code |= PopBit(state);
		++total_code_bits;

		/* Detect inline data. */
		if (total_code_bits == 6 && code == 0x3F)
		{
			return NULL;
		}
		else
		{
			NybbleRun* nybble_run = &state->nybble_runs[code << (8 - total_code_bits)];

			if ((NybbleRunExists(nybble_run) && nybble_run->total_code_bits == total_code_bits))
				return nybble_run;
		}
	}
}

static void OutputNybble(State* state, u16 nybble)
{
	state->output_buffer <<= 4;
	state->output_buffer |= nybble;

	if ((++state->output_buffer_nybbles_done & 7) == 0)
	{
		u32 final_output = state->output_buffer ^ (state->xor_mode_enabled ? state->previous_output_buffer : 0);

		for (u16 i = 0; i < 4; ++i)
			WriteByte(&state->common, (final_output >> (4 - 1 - i) * 8) & 0xFF);

		state->previous_output_buffer = final_output;
	}
}

static void OutputNybbles(State* state, u16 nybble, u16 total_nybbles)
{
	for (u16 i = 0; i < total_nybbles; ++i)
		OutputNybble(state, nybble);
}

static void ProcessHeader(State* state)
{
	u8 header_byte_1 = ReadByte(&state->common);
	u8 header_byte_2 = ReadByte(&state->common);
	u16 header_word = (u16)header_byte_1 << 8 | header_byte_2;

	state->xor_mode_enabled = (header_word & 0x8000) != 0;
	state->total_tiles = header_word & 0x7FFF;
}

static void ProcessCodeTable(State* state)
{
	u8 nybble_run_value = 0; /* Not necessary, but shuts up a compiler warning. */

	memset(state->nybble_runs, 0, sizeof(state->nybble_runs));

	u8 byte = ReadByte(&state->common);

	while (byte != 0xFF)
	{
		if ((byte & 0x80) != 0)
		{
			nybble_run_value = byte & 0xF;
			byte = ReadByte(&state->common);
		}
		else
		{
			u8 run_length = ((byte >> 4) & 7) + 1;
			u8 total_code_bits = byte & 0xF;
			u8 code = ReadByte(&state->common);
			u16 nybble_run_index = (u16)code << (8u - total_code_bits);
			NybbleRun* nybble_run = &state->nybble_runs[nybble_run_index];

			if (total_code_bits > 8 || total_code_bits == 0 || nybble_run_index > CC_COUNT_OF(state->nybble_runs))
			{
                return;
			}

			nybble_run->total_code_bits = total_code_bits;
			nybble_run->value = nybble_run_value;
			nybble_run->length = run_length;

			byte = ReadByte(&state->common);
		}
	}
}

static void ProcessCodes(State* state)
{
	s32 nybbles_remaining = state->total_tiles * (8 * 8);
	u16 total_runs = 0;

	while (nybbles_remaining != 0)
	{
		/* TODO: Undo this hack! */
		NybbleRun* nybble_run = FindCode(state);
		u16 run_length = nybble_run != NULL ? nybble_run->length : PopBits(state, 3) + 1;
		u16 nybble = nybble_run != NULL ? nybble_run->value : PopBits(state, 4);

		if (nybble_run != NULL)
		{
			++total_runs;
		}

		if (run_length > nybbles_remaining)
		{
            break;
		}

		OutputNybbles(state, nybble, run_length);

		nybbles_remaining -= run_length;
	}
}

u16 ClownNemesis_Decompress (u8* src, u8* dest)
{
	State state = {0};
	InitialiseCommon(&state.common, src, dest);

    ProcessHeader(&state);
    ProcessCodeTable(&state);
    ProcessCodes(&state);

    return 1; // Success return code
}

#undef MAXIMUM_CODE_BITS