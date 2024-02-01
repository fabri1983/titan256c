#include "genesis.h"

#ifdef __GNUC__
#define EXTERNALLY_VISIBLE_DEF __attribute__((externally_visible))
#elif defined(_MSC_VER)
#define EXTERNALLY_VISIBLE_DEF __declspec(dllexport)
#endif

EXTERNALLY_VISIBLE_DEF
const ROMHeader rom_header = {
#if (ENABLE_BANK_SWITCH != 0)
    "SEGA SSF        ",
#elif (MODULE_MEGAWIFI != 0)
    "SEGA MEGAWIFI   ",
#else
    "SEGA MEGA DRIVE ",
#endif
    "(C)SGDK 2024    ",
    "SAMPLE PROGRAM                                  ",
    "SAMPLE PROGRAM                                  ",
    "GM 00000000-00",
    0x000,
    "JD              ",
    0x00000000,
#if (ENABLE_BANK_SWITCH != 0)
    0x003FFFFF,
#else
    0x000FFFFF,
#endif
    0xE0FF0000,
    0xE0FFFFFF,
    "RA",
    0xF820,
    0x00200000,
    0x0020FFFF,
    "            ",
    "DEMONSTRATION PROGRAM                   ",
    "JUE             "
};
