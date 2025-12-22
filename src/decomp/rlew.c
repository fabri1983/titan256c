#include <types.h>
#include <sys.h>
#include "decomp/rlew.h"

#define DUPLICATE_WORD_INTO_LONG(vword, vlong)\
    __asm __volatile__ (\
        "move.w  %0, %1\n\t"\
        "swap    %1\n\t"\
        "move.w  %0, %1"\
        : "+d" (vword), "=d" (vlong)\
        :\
        :\
    )\

#define COPY_LONG_INTO_OUT(vlong, out)\
    __asm __volatile__ (\
        "move.l  %1, (%0)+"\
        : "=a" (out)\
        : "d" (vlong)\
        : "memory"\
    )\

#define GET_LONG_AND_COPY_INTO_OUT(in, out)\
    __asm __volatile__ (\
        "move.l  (%0)+, (%1)+"\
        : "+a" (in), "=a" (out)\
        :\
        : "memory"\
    )\

#define GET_BYTE_AS_LOW_INTO_WORD_AND_COPY_INTO_OUT(in, vword, out)\
    __asm __volatile__ (\
        "move.w  %1, (%2)+\n\t" /* copy word into out buffer */\
        "move.b  (%0)+, %1" /* byte goes to low half of destination leaving high half as it is */ \
        : "+a" (in), "=d" (vword), "=a" (out)\
        :\
        : "memory"\
    )\

#define COPY_WORD_INTO_OUT(vword, out)\
    __asm __volatile__ (\
        "move.w  %1, (%0)+"\
        : "=a" (out)\
        : "d" (vword)\
        : "memory"\
    )\

void NO_INLINE rlew_decomp_A (const u8 jumpGap, u8* in, u8* out) {
    u8 rows = *in++; // get map rows

    // FIRST RUN: #rows acts a parity byte so descriptor is at odd position
    {
        u8 rleDescriptor = *in++; // read RLE descriptor byte and advance

        // if 2nd MSB == 0 then it's a basic RLE: just copy a word value N times
        if ((rleDescriptor & 0b01000000) == 0) {
            u16 value_w = *(u16*) in; // read word
            in += 2;
            u8 length = rleDescriptor & 0b00111111;
            // length is odd? then copy first word
            if ((length & 1) != 0) { // we know length >= 1
                *(u16*) out = value_w;
                out += 2;
                --length;
            }
            if (length != 0) {
                // duplicate the word value into a long value
                u32 value_l = 0;
                DUPLICATE_WORD_INTO_LONG(value_w, value_l);
                // copy the long value N/2 times
                u32* out_l = (u32*) out;
                switch (length) {
                    case 40: *out_l++ = value_l; // fall through
                    case 38: *out_l++ = value_l; // fall through
                    case 36: *out_l++ = value_l; // fall through
                    case 34: *out_l++ = value_l; // fall through
                    case 32: *out_l++ = value_l; // fall through
                    case 30: *out_l++ = value_l; // fall through
                    case 28: *out_l++ = value_l; // fall through
                    case 26: *out_l++ = value_l; // fall through
                    case 24: *out_l++ = value_l; // fall through
                    case 22: *out_l++ = value_l; // fall through
                    case 20: *out_l++ = value_l; // fall through
                    case 18: *out_l++ = value_l; // fall through
                    case 16: *out_l++ = value_l; // fall through
                    case 14: *out_l++ = value_l; // fall through
                    case 12: *out_l++ = value_l; // fall through
                    case 10: *out_l++ = value_l; // fall through
                    case  8: *out_l++ = value_l; // fall through
                    case  6: *out_l++ = value_l; // fall through
                    case  4: *out_l++ = value_l; // fall through
                    case  2: *out_l++ = value_l; // fall through
                    default: break;
                }
                out = (u8*) out_l;
            }
        }
        // 2nd MSB == 1 then we're going to copy a stream of words
        else {
            u8 length = rleDescriptor & 0b00111111;
            // length is odd? then copy first word
            if ((length & 1) != 0) { // we know length >= 2
                *(u16*) out = *(u16*) in; // copy a word
                in += 2;
                out += 2;
                --length;
            }
            // copy remaining even number of words as pairs, ie copying 2 words (1 long) at a time
            u32* in_l = (u32*) in;
            u32* out_l = (u32*) out;
            switch (length) {
                case 40: *out_l++ = *in_l++; // fall through
                case 38: *out_l++ = *in_l++; // fall through
                case 36: *out_l++ = *in_l++; // fall through
                case 34: *out_l++ = *in_l++; // fall through
                case 32: *out_l++ = *in_l++; // fall through
                case 30: *out_l++ = *in_l++; // fall through
                case 28: *out_l++ = *in_l++; // fall through
                case 26: *out_l++ = *in_l++; // fall through
                case 24: *out_l++ = *in_l++; // fall through
                case 22: *out_l++ = *in_l++; // fall through
                case 20: *out_l++ = *in_l++; // fall through
                case 18: *out_l++ = *in_l++; // fall through
                case 16: *out_l++ = *in_l++; // fall through
                case 14: *out_l++ = *in_l++; // fall through
                case 12: *out_l++ = *in_l++; // fall through
                case 10: *out_l++ = *in_l++; // fall through
                case  8: *out_l++ = *in_l++; // fall through
                case  6: *out_l++ = *in_l++; // fall through
                case  4: *out_l++ = *in_l++; // fall through
                case  2: *out_l++ = *in_l++; // fall through
                default: break;
            }
            in = (u8*) in_l;
            out = (u8*) out_l;
        }

        // is end of row bit set?
        if (rleDescriptor > 0b10000000) {
            // makes the out buffer pointer jump over the region used as expanded width of the map
            out += jumpGap;
            --rows;
        }
    }

    // REMAINING RUNS: from now on we skip parity byte before accessing any descriptor
    while (rows) {
        ++in; // skip parity byte
        u8 rleDescriptor = *in++; // read RLE descriptor byte and advance

        // if 2nd MSB == 0 then it's a basic RLE: just copy a word value N times
        if ((rleDescriptor & 0b01000000) == 0) {
            u16 value_w = *(u16*) in; // read word
            in += 2;
            u8 length = rleDescriptor & 0b00111111;
            // length is odd? then copy first word
            if ((length & 1) != 0) { // we know length >= 1
                *(u16*) out = value_w;
                out += 2;
                --length;
            }
            if (length != 0) {
                // duplicate the word value into a long value
                u32 value_l = 0;
                DUPLICATE_WORD_INTO_LONG(value_w, value_l);
                // copy the long value N/2 times
                u32* out_l = (u32*) out;
                switch (length) {
                    case 40: *out_l++ = value_l; // fall through
                    case 38: *out_l++ = value_l; // fall through
                    case 36: *out_l++ = value_l; // fall through
                    case 34: *out_l++ = value_l; // fall through
                    case 32: *out_l++ = value_l; // fall through
                    case 30: *out_l++ = value_l; // fall through
                    case 28: *out_l++ = value_l; // fall through
                    case 26: *out_l++ = value_l; // fall through
                    case 24: *out_l++ = value_l; // fall through
                    case 22: *out_l++ = value_l; // fall through
                    case 20: *out_l++ = value_l; // fall through
                    case 18: *out_l++ = value_l; // fall through
                    case 16: *out_l++ = value_l; // fall through
                    case 14: *out_l++ = value_l; // fall through
                    case 12: *out_l++ = value_l; // fall through
                    case 10: *out_l++ = value_l; // fall through
                    case  8: *out_l++ = value_l; // fall through
                    case  6: *out_l++ = value_l; // fall through
                    case  4: *out_l++ = value_l; // fall through
                    case  2: *out_l++ = value_l; // fall through
                    default: break;
                }
                out = (u8*) out_l;
            }
        }
        // 2nd MSB == 1 then we're going to copy a stream of words
        else {
            u8 length = rleDescriptor & 0b00111111;
            // length is odd? then copy first word
            if ((length & 1) != 0) { // we know length >= 2
                *(u16*) out = *(u16*) in; // copy a word
                in += 2;
                out += 2;
                --length;
            }
            // copy remaining even number of words as pairs, ie copying 2 words (1 long) at a time
            u32* in_l = (u32*) in;
            u32* out_l = (u32*) out;
            switch (length) {
                case 40: *out_l++ = *in_l++; // fall through
                case 38: *out_l++ = *in_l++; // fall through
                case 36: *out_l++ = *in_l++; // fall through
                case 34: *out_l++ = *in_l++; // fall through
                case 32: *out_l++ = *in_l++; // fall through
                case 30: *out_l++ = *in_l++; // fall through
                case 28: *out_l++ = *in_l++; // fall through
                case 26: *out_l++ = *in_l++; // fall through
                case 24: *out_l++ = *in_l++; // fall through
                case 22: *out_l++ = *in_l++; // fall through
                case 20: *out_l++ = *in_l++; // fall through
                case 18: *out_l++ = *in_l++; // fall through
                case 16: *out_l++ = *in_l++; // fall through
                case 14: *out_l++ = *in_l++; // fall through
                case 12: *out_l++ = *in_l++; // fall through
                case 10: *out_l++ = *in_l++; // fall through
                case  8: *out_l++ = *in_l++; // fall through
                case  6: *out_l++ = *in_l++; // fall through
                case  4: *out_l++ = *in_l++; // fall through
                case  2: *out_l++ = *in_l++; // fall through
                default: break;
            }
            in = (u8*) in_l;
            out = (u8*) out_l;
        }

        // is end of row bit set?
        if (rleDescriptor > 0b10000000) {
            // makes the out buffer pointer jump over the region used as expanded width of the map
            out += jumpGap;
            --rows;
        }
    }
}

void NO_INLINE rlew_decomp_B (const u8 jumpGap, u8* in, u8* out) {
    u8 rows = *in++; // get map rows
    while (rows) {
        u8 rleDescriptor = *in++; // read RLE descriptor byte and advance

        // is descriptor the parity byte?
        if (rleDescriptor == 0b01000000) {
            rleDescriptor = *in++; // read RLE descriptor byte and advance
        }

        // is end of row byte?
        if (rleDescriptor == 0) {
            // makes the out buffer pointer jump over the region used as expanded width of the map
            out += jumpGap;
            --rows;
        }
        // if descriptor's mask matches 0b00...... then is a basic RLE: copy a word N times
        else if (rleDescriptor < 0b01000000) {
            u16 value_w = *(u16*) in; // read word
            in += 2;
            u8 length = rleDescriptor & 0b00111111;
            // length is odd? then copy first word
            if ((length & 1) != 0) { // we know length >= 1
                *(u16*) out = value_w;
                out += 2;
                --length;
                if (length == 0)
                    continue;
            }
            // duplicate the word value into a long value
            u32 value_l = 0;
            DUPLICATE_WORD_INTO_LONG(value_w, value_l);
            // copy the long value N/2 times
            switch (length) {
                case 40: COPY_LONG_INTO_OUT(value_l, out); // fall through
                case 38: COPY_LONG_INTO_OUT(value_l, out); // fall through
                case 36: COPY_LONG_INTO_OUT(value_l, out); // fall through
                case 34: COPY_LONG_INTO_OUT(value_l, out); // fall through
                case 32: COPY_LONG_INTO_OUT(value_l, out); // fall through
                case 30: COPY_LONG_INTO_OUT(value_l, out); // fall through
                case 28: COPY_LONG_INTO_OUT(value_l, out); // fall through
                case 26: COPY_LONG_INTO_OUT(value_l, out); // fall through
                case 24: COPY_LONG_INTO_OUT(value_l, out); // fall through
                case 22: COPY_LONG_INTO_OUT(value_l, out); // fall through
                case 20: COPY_LONG_INTO_OUT(value_l, out); // fall through
                case 18: COPY_LONG_INTO_OUT(value_l, out); // fall through
                case 16: COPY_LONG_INTO_OUT(value_l, out); // fall through
                case 14: COPY_LONG_INTO_OUT(value_l, out); // fall through
                case 12: COPY_LONG_INTO_OUT(value_l, out); // fall through
                case 10: COPY_LONG_INTO_OUT(value_l, out); // fall through
                case  8: COPY_LONG_INTO_OUT(value_l, out); // fall through
                case  6: COPY_LONG_INTO_OUT(value_l, out); // fall through
                case  4: COPY_LONG_INTO_OUT(value_l, out); // fall through
                case  2: COPY_LONG_INTO_OUT(value_l, out); // fall through
                default: break;
            }
        }
        // if descriptor's mask matches 0b01...... then we have an incremental RLE segment
        /*else if (rleDescriptor < 0b10000000) {
            s8 operator = *in++;
            s16 value_w = *(u16*) in; // read word
            in += 2;
            u16* out_w = (u16*) out;
            u8 length = rleDescriptor & 0b00111111;
            switch (length) { // we know length >= 2
                case 40: *out_w++ = value_w; value_w += operator; // fall through
                case 39: *out_w++ = value_w; value_w += operator; // fall through
                case 38: *out_w++ = value_w; value_w += operator; // fall through
                case 37: *out_w++ = value_w; value_w += operator; // fall through
                case 36: *out_w++ = value_w; value_w += operator; // fall through
                case 35: *out_w++ = value_w; value_w += operator; // fall through
                case 34: *out_w++ = value_w; value_w += operator; // fall through
                case 33: *out_w++ = value_w; value_w += operator; // fall through
                case 32: *out_w++ = value_w; value_w += operator; // fall through
                case 31: *out_w++ = value_w; value_w += operator; // fall through
                case 30: *out_w++ = value_w; value_w += operator; // fall through
                case 29: *out_w++ = value_w; value_w += operator; // fall through
                case 28: *out_w++ = value_w; value_w += operator; // fall through
                case 27: *out_w++ = value_w; value_w += operator; // fall through
                case 26: *out_w++ = value_w; value_w += operator; // fall through
                case 25: *out_w++ = value_w; value_w += operator; // fall through
                case 24: *out_w++ = value_w; value_w += operator; // fall through
                case 23: *out_w++ = value_w; value_w += operator; // fall through
                case 22: *out_w++ = value_w; value_w += operator; // fall through
                case 21: *out_w++ = value_w; value_w += operator; // fall through
                case 20: *out_w++ = value_w; value_w += operator; // fall through
                case 19: *out_w++ = value_w; value_w += operator; // fall through
                case 18: *out_w++ = value_w; value_w += operator; // fall through
                case 17: *out_w++ = value_w; value_w += operator; // fall through
                case 16: *out_w++ = value_w; value_w += operator; // fall through
                case 15: *out_w++ = value_w; value_w += operator; // fall through
                case 14: *out_w++ = value_w; value_w += operator; // fall through
                case 13: *out_w++ = value_w; value_w += operator; // fall through
                case 12: *out_w++ = value_w; value_w += operator; // fall through
                case 11: *out_w++ = value_w; value_w += operator; // fall through
                case 10: *out_w++ = value_w; value_w += operator; // fall through
                case  9: *out_w++ = value_w; value_w += operator; // fall through
                case  8: *out_w++ = value_w; value_w += operator; // fall through
                case  7: *out_w++ = value_w; value_w += operator; // fall through
                case  6: *out_w++ = value_w; value_w += operator; // fall through
                case  5: *out_w++ = value_w; value_w += operator; // fall through
                case  4: *out_w++ = value_w; value_w += operator; // fall through
                case  3: *out_w++ = value_w; value_w += operator; // fall through
                case  2: *out_w++ = value_w; value_w += operator; // fall through
                default: break;
            }
            *out_w++ = value_w;
            out = (u8*) out_w;
        }*/
        // if descriptor's mask matches 0b1...... then is a stream of words
        else if (rleDescriptor < 0b11000000) {
            u8 length = rleDescriptor & 0b00111111;
            // length is odd? then copy first word
            if ((length & 1) != 0) { // we know length >= 2
                *(u16*) out = *(u16*) in; // read word
                in += 2;
                out += 2;
                --length;
            }
            // copy remaining even number of words as pairs, ie copying 2 words (1 long) at a time
            switch (length) {
                case 40: GET_LONG_AND_COPY_INTO_OUT(in, out); // fall through
                case 38: GET_LONG_AND_COPY_INTO_OUT(in, out); // fall through
                case 36: GET_LONG_AND_COPY_INTO_OUT(in, out); // fall through
                case 34: GET_LONG_AND_COPY_INTO_OUT(in, out); // fall through
                case 32: GET_LONG_AND_COPY_INTO_OUT(in, out); // fall through
                case 30: GET_LONG_AND_COPY_INTO_OUT(in, out); // fall through
                case 28: GET_LONG_AND_COPY_INTO_OUT(in, out); // fall through
                case 26: GET_LONG_AND_COPY_INTO_OUT(in, out); // fall through
                case 24: GET_LONG_AND_COPY_INTO_OUT(in, out); // fall through
                case 22: GET_LONG_AND_COPY_INTO_OUT(in, out); // fall through
                case 20: GET_LONG_AND_COPY_INTO_OUT(in, out); // fall through
                case 18: GET_LONG_AND_COPY_INTO_OUT(in, out); // fall through
                case 16: GET_LONG_AND_COPY_INTO_OUT(in, out); // fall through
                case 14: GET_LONG_AND_COPY_INTO_OUT(in, out); // fall through
                case 12: GET_LONG_AND_COPY_INTO_OUT(in, out); // fall through
                case 10: GET_LONG_AND_COPY_INTO_OUT(in, out); // fall through
                case  8: GET_LONG_AND_COPY_INTO_OUT(in, out); // fall through
                case  6: GET_LONG_AND_COPY_INTO_OUT(in, out); // fall through
                case  4: GET_LONG_AND_COPY_INTO_OUT(in, out); // fall through
                case  2: GET_LONG_AND_COPY_INTO_OUT(in, out); // fall through
                default: break;
            }
        }
        // descriptor's mask matches 0b11...... then is a stream with a common high byte
        else {
            // read word with common byte in high half and valid first byte in lower half
            u16 value_w = *(u16*) in; // read word
            in += 2;
            u8 length = rleDescriptor & 0b00111111;
            switch (length) { // we know length >= 2
                case 40: GET_BYTE_AS_LOW_INTO_WORD_AND_COPY_INTO_OUT(in, value_w, out); // fall through
                case 39: GET_BYTE_AS_LOW_INTO_WORD_AND_COPY_INTO_OUT(in, value_w, out); // fall through
                case 38: GET_BYTE_AS_LOW_INTO_WORD_AND_COPY_INTO_OUT(in, value_w, out); // fall through
                case 37: GET_BYTE_AS_LOW_INTO_WORD_AND_COPY_INTO_OUT(in, value_w, out); // fall through
                case 36: GET_BYTE_AS_LOW_INTO_WORD_AND_COPY_INTO_OUT(in, value_w, out); // fall through
                case 35: GET_BYTE_AS_LOW_INTO_WORD_AND_COPY_INTO_OUT(in, value_w, out); // fall through
                case 34: GET_BYTE_AS_LOW_INTO_WORD_AND_COPY_INTO_OUT(in, value_w, out); // fall through
                case 33: GET_BYTE_AS_LOW_INTO_WORD_AND_COPY_INTO_OUT(in, value_w, out); // fall through
                case 32: GET_BYTE_AS_LOW_INTO_WORD_AND_COPY_INTO_OUT(in, value_w, out); // fall through
                case 31: GET_BYTE_AS_LOW_INTO_WORD_AND_COPY_INTO_OUT(in, value_w, out); // fall through
                case 30: GET_BYTE_AS_LOW_INTO_WORD_AND_COPY_INTO_OUT(in, value_w, out); // fall through
                case 29: GET_BYTE_AS_LOW_INTO_WORD_AND_COPY_INTO_OUT(in, value_w, out); // fall through
                case 28: GET_BYTE_AS_LOW_INTO_WORD_AND_COPY_INTO_OUT(in, value_w, out); // fall through
                case 27: GET_BYTE_AS_LOW_INTO_WORD_AND_COPY_INTO_OUT(in, value_w, out); // fall through
                case 26: GET_BYTE_AS_LOW_INTO_WORD_AND_COPY_INTO_OUT(in, value_w, out); // fall through
                case 25: GET_BYTE_AS_LOW_INTO_WORD_AND_COPY_INTO_OUT(in, value_w, out); // fall through
                case 24: GET_BYTE_AS_LOW_INTO_WORD_AND_COPY_INTO_OUT(in, value_w, out); // fall through
                case 23: GET_BYTE_AS_LOW_INTO_WORD_AND_COPY_INTO_OUT(in, value_w, out); // fall through
                case 22: GET_BYTE_AS_LOW_INTO_WORD_AND_COPY_INTO_OUT(in, value_w, out); // fall through
                case 21: GET_BYTE_AS_LOW_INTO_WORD_AND_COPY_INTO_OUT(in, value_w, out); // fall through
                case 20: GET_BYTE_AS_LOW_INTO_WORD_AND_COPY_INTO_OUT(in, value_w, out); // fall through
                case 19: GET_BYTE_AS_LOW_INTO_WORD_AND_COPY_INTO_OUT(in, value_w, out); // fall through
                case 18: GET_BYTE_AS_LOW_INTO_WORD_AND_COPY_INTO_OUT(in, value_w, out); // fall through
                case 17: GET_BYTE_AS_LOW_INTO_WORD_AND_COPY_INTO_OUT(in, value_w, out); // fall through
                case 16: GET_BYTE_AS_LOW_INTO_WORD_AND_COPY_INTO_OUT(in, value_w, out); // fall through
                case 15: GET_BYTE_AS_LOW_INTO_WORD_AND_COPY_INTO_OUT(in, value_w, out); // fall through
                case 14: GET_BYTE_AS_LOW_INTO_WORD_AND_COPY_INTO_OUT(in, value_w, out); // fall through
                case 13: GET_BYTE_AS_LOW_INTO_WORD_AND_COPY_INTO_OUT(in, value_w, out); // fall through
                case 12: GET_BYTE_AS_LOW_INTO_WORD_AND_COPY_INTO_OUT(in, value_w, out); // fall through
                case 11: GET_BYTE_AS_LOW_INTO_WORD_AND_COPY_INTO_OUT(in, value_w, out); // fall through
                case 10: GET_BYTE_AS_LOW_INTO_WORD_AND_COPY_INTO_OUT(in, value_w, out); // fall through
                case  9: GET_BYTE_AS_LOW_INTO_WORD_AND_COPY_INTO_OUT(in, value_w, out); // fall through
                case  8: GET_BYTE_AS_LOW_INTO_WORD_AND_COPY_INTO_OUT(in, value_w, out); // fall through
                case  7: GET_BYTE_AS_LOW_INTO_WORD_AND_COPY_INTO_OUT(in, value_w, out); // fall through
                case  6: GET_BYTE_AS_LOW_INTO_WORD_AND_COPY_INTO_OUT(in, value_w, out); // fall through
                case  5: GET_BYTE_AS_LOW_INTO_WORD_AND_COPY_INTO_OUT(in, value_w, out); // fall through
                case  4: GET_BYTE_AS_LOW_INTO_WORD_AND_COPY_INTO_OUT(in, value_w, out); // fall through
                case  3: GET_BYTE_AS_LOW_INTO_WORD_AND_COPY_INTO_OUT(in, value_w, out); // fall through
                case  2: GET_BYTE_AS_LOW_INTO_WORD_AND_COPY_INTO_OUT(in, value_w, out); // fall through
                default: break;
            }
            COPY_WORD_INTO_OUT(value_w, out);
        }
    }
}