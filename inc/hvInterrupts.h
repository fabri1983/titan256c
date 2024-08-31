#ifndef HV_INTERRUPTS_H
#define HV_INTERRUPTS_H

#include <types.h>
#include <sys.h>
#include "compatibilities.h"

void setGradientPtrToBlack ();

void setHVCallbacks (u8 titan256cHIntMode);

void setHIntScanlineStarterForBounceEffect (u16 yPos, u8 hintMode);

void vertIntOnTitan256cCallback_HIntEveryN ();

void vertIntOnTitan256cCallback_HIntOneTime ();

INTERRUPT_ATTRIBUTE horizIntOnTitan256cCallback_CPU_EveryN_asm ();

INTERRUPT_ATTRIBUTE horizIntOnTitan256cCallback_CPU_EveryN ();

INTERRUPT_ATTRIBUTE horizIntOnTitan256cCallback_DMA_EveryN_asm ();

INTERRUPT_ATTRIBUTE horizIntOnTitan256cCallback_DMA_EveryN ();

INTERRUPT_ATTRIBUTE horizIntOnTitan256cCallback_DMA_OneTime ();

void vertIntOnDrawTextCallback ();

INTERRUPT_ATTRIBUTE horizIntOnDrawTextCallback ();

#endif // HV_INTERRUPTS_H