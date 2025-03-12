#ifndef _SPHERE_TEXT_ANIMATION_H
#define _SPHERE_TEXT_ANIMATION_H

#include <sprite_eng.h>

#define SPRITE_ANIM_MANUAL_EFFECT_DELAY_FRAMES 1

u16 setupSphereTextAnimations (u16 currTileIndex);

void sphereTextAnimationFreeFrameIndexes ();

void sphereTextAnimationsPosition (u16 posX, u16 posY);

void toggleSphereTextAnimations ();

void updateSphereTextAnimFrameOnJoyInput (u16* buttonBitsChange, u16* buttonBitsState);

#endif // _SPHERE_TEXT_ANIMATION_H