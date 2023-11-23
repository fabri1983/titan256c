#ifndef HV_INTERRUPTS_H
#define HV_INTERRUPTS_H

#include <types.h>
#include <sys.h>

void vertIntOnTitan256cCallback_HIntEveryN ();

void vertIntOnTitan256cCallback_HIntOneTime ();

HINTERRUPT_CALLBACK horizIntOnTitan256cCallback_CPU_EveryN ();

HINTERRUPT_CALLBACK horizIntOnTitan256cCallback_DMA_EveryN ();

HINTERRUPT_CALLBACK horizIntOnTitan256cCallback_DMA_OneTime ();

void beforeVBlankProcOnTitan256c_DMA_QUEUE ();

void afterVBlankProcOnTitan256c_VDP_or_DMA ();

#endif