#ifndef _STOPWATCH_H
#define _STOPWATCH_H

#include <types.h>

const unsigned char div_100[] = {
    '0','0','0','0','0','0','0','0','0','0',
    '0','0','0','0','0','0','0','0','0','0',
    '0','0','0','0','0','0','0','0','0','0',
    '0','0','0','0','0','0','0','0','0','0',
    '0','0','0','0','0','0','0','0','0','0',
    '0','0','0','0','0','0','0','0','0','0',
    '0','0','0','0','0','0','0','0','0','0',
    '0','0','0','0','0','0','0','0','0','0',
    '0','0','0','0','0','0','0','0','0','0',
    '0','0','0','0','0','0','0','0','0','0',
    '1','1','1','1','1','1','1','1','1','1',
    '1','1','1','1','1','1','1','1','1','1',
    '1','1','1','1','1','1','1','1','1','1',
    '1','1','1','1','1','1','1','1','1','1',
    '1','1','1','1','1','1','1','1','1','1',
    '1','1','1','1','1','1','1','1','1','1',
    '1','1','1','1','1','1','1','1','1','1',
    '1','1','1','1','1','1','1','1','1','1',
    '1','1','1','1','1','1','1','1','1','1',
    '1','1','1','1','1','1','1','1','1','1',
    '2','2','2','2','2','2','2','2','2','2',
    '2','2','2','2','2','2','2','2','2','2',
    '2','2','2','2','2','2','2','2','2','2',
    '2','2','2','2','2','2','2','2','2','2',
    '2','2','2','2','2','2','2','2','2','2',
    '2','2','2','2','2','2','2','2','2','2',
    '2','2'
};

const unsigned char div_10_mod_10[262] = {
    '0','0','0','0','0','0','0','0','0','0',
    '1','1','1','1','1','1','1','1','1','1',
    '2','2','2','2','2','2','2','2','2','2',
    '3','3','3','3','3','3','3','3','3','3',
    '4','4','4','4','4','4','4','4','4','4',
    '5','5','5','5','5','5','5','5','5','5',
    '6','6','6','6','6','6','6','6','6','6',
    '7','7','7','7','7','7','7','7','7','7',
    '8','8','8','8','8','8','8','8','8','8',
    '9','9','9','9','9','9','9','9','9','9',
    '0','0','0','0','0','0','0','0','0','0',
    '1','1','1','1','1','1','1','1','1','1',
    '2','2','2','2','2','2','2','2','2','2',
    '3','3','3','3','3','3','3','3','3','3',
    '4','4','4','4','4','4','4','4','4','4',
    '5','5','5','5','5','5','5','5','5','5',
    '6','6','6','6','6','6','6','6','6','6',
    '7','7','7','7','7','7','7','7','7','7',
    '8','8','8','8','8','8','8','8','8','8',
    '9','9','9','9','9','9','9','9','9','9',
    '0','0','0','0','0','0','0','0','0','0',
    '1','1','1','1','1','1','1','1','1','1',
    '2','2','2','2','2','2','2','2','2','2',
    '3','3','3','3','3','3','3','3','3','3',
    '4','4','4','4','4','4','4','4','4','4',
    '5','5','5','5','5','5','5','5','5','5',
    '6','6'
};

const unsigned char mod_10[262] = {
    '0','1','2','3','4','5','6','7','8','9',
    '0','1','2','3','4','5','6','7','8','9',
    '0','1','2','3','4','5','6','7','8','9',
    '0','1','2','3','4','5','6','7','8','9',
    '0','1','2','3','4','5','6','7','8','9',
    '0','1','2','3','4','5','6','7','8','9',
    '0','1','2','3','4','5','6','7','8','9',
    '0','1','2','3','4','5','6','7','8','9',
    '0','1','2','3','4','5','6','7','8','9',
    '0','1','2','3','4','5','6','7','8','9',
    '0','1','2','3','4','5','6','7','8','9',
    '0','1','2','3','4','5','6','7','8','9',
    '0','1','2','3','4','5','6','7','8','9',
    '0','1','2','3','4','5','6','7','8','9',
    '0','1','2','3','4','5','6','7','8','9',
    '0','1','2','3','4','5','6','7','8','9',
    '0','1','2','3','4','5','6','7','8','9',
    '0','1','2','3','4','5','6','7','8','9',
    '0','1','2','3','4','5','6','7','8','9',
    '0','1','2','3','4','5','6','7','8','9',
    '0','1','2','3','4','5','6','7','8','9',
    '0','1','2','3','4','5','6','7','8','9',
    '0','1','2','3','4','5','6','7','8','9',
    '0','1','2','3','4','5','6','7','8','9',
    '0','1','2','3','4','5','6','7','8','9',
    '0','1','2','3','4','5','6','7','8','9',
    '0','1'
};

#define STRINGIFY(x) #x
/// At NTSC and V224 max vertical scanline is 262. Take into account your code will be interrupted by hardware VInt and HInt. 
/// HOWEVER VInt is executed at scanline 224 according to Shannon Birt. So a healthy limit for cpu utilizaiton is up to 223.
/// Alternative way by ctr001: how I do profiling: change the background color by using VDP register $87
#define STOPWATCH_START(n) \
	u8 lineStart_##n = GET_VCOUNTER;
#define STOPWATCH_STOP(n) \
	u8 lineEnd_##n = GET_VCOUNTER;\
	u8 frameTime_##n;\
	char str_##n[] = "ft__"STRINGIFY(n)"     ";\
	if (lineEnd_##n < lineStart_##n) {\
		frameTime_##n = 261 - lineStart_##n;\
		frameTime_##n += lineEnd_##n;\
		/* Add a 'w' to know this measure has wrapped around at least a display loop */ \
		*(str_##n + 2) = 'w';\
	} else {\
		frameTime_##n = lineEnd_##n - lineStart_##n;\
	}\
	{\
		*(str_##n + 8) = div_100[frameTime_##n];\
		*(str_##n + 9) = div_10_mod_10[frameTime_##n];\
		*(str_##n + 10) = mod_10[frameTime_##n];\
		*(str_##n + 11) = '\0';\
		KLog(str_##n);\
	}\

#endif // _STOPWATCH_H