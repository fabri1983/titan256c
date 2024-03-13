#ifndef HV_INTERRUPTS_H
#define HV_INTERRUPTS_H

#include <types.h>
#include <sys.h>
#include "compatibilities.h"

void blackCurrentGradientPtr ();

void vertIntOnTitan256cCallback_HIntEveryN ();

void vertIntOnTitan256cCallback_HIntOneTime ();

HINTERRUPT_CALLBACK horizIntOnTitan256cCallback_CPU_EveryN_asm ();

HINTERRUPT_CALLBACK horizIntOnTitan256cCallback_CPU_EveryN ();

HINTERRUPT_CALLBACK horizIntOnTitan256cCallback_DMA_EveryN ();

HINTERRUPT_CALLBACK horizIntOnTitan256cCallback_DMA_OneTime ();

void vertIntOnDrawTextCallback ();

HINTERRUPT_CALLBACK horizIntOnDrawTextCallback ();

#endif // HV_INTERRUPTS_H