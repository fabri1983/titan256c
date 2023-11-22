#ifndef HV_INTERRUPTS_H
#define HV_INTERRUPTS_H

#include <types.h>
#include <sys.h>

void vertIntOnTitan256cCallback_HIntEveryN ();

void vertIntOnTitan256cCallback_HIntOneTime ();

HINTERRUPT_CALLBACK horizIntOnTitan256cCallback_EveryN_CPU ();

HINTERRUPT_CALLBACK horizIntOnTitan256cCallback_EveryN_DMA ();

HINTERRUPT_CALLBACK horizIntOnTitan256cCallback_OneTime_DMA ();

void beforeVBlankProcOnTitan256c_DMA_QUEUE ();

void afterVBlankProcOnTitan256c_VDP_or_DMA ();

#endif