#include <types.h>
#include <joy.h>
#include <memory.h>
#include "sphereTextAnimation.h"
#include "titan256c_consts.h"
#include "titan256c.h"
#include "titan_sphere_res.h"

static Sprite* titanSphereText_1_AnimSpr;
static Sprite* titanSphereText_2_AnimSpr;

void setupSphereTextAnimations ()
{
    setSphereTextColorsIntoTitanPalettes(sprDefTitanSphereText_1_Anim.palette);

    SPR_initEx(sprDefTitanSphereText_1_Anim.maxNumTile + sprDefTitanSphereText_2_Anim.maxNumTile); // Eg: 117 + 124 tiles

    u16 flags = SPR_FLAG_AUTO_TILE_UPLOAD | SPR_FLAG_AUTO_VRAM_ALLOC;

    titanSphereText_1_AnimSpr = SPR_addSpriteExSafe(&sprDefTitanSphereText_1_Anim, 
        TITAN_SPHERE_TILEMAP_START_X_POS * 8, TITAN_SPHERE_TILEMAP_START_Y_POS * 8, 
        TILE_ATTR(PAL0, 0, FALSE, FALSE), flags);
    // Starts visible
    SPR_setVisibility(titanSphereText_1_AnimSpr, VISIBLE);

    titanSphereText_2_AnimSpr = SPR_addSpriteExSafe(&sprDefTitanSphereText_2_Anim, 
        TITAN_SPHERE_TILEMAP_START_X_POS * 8, TITAN_SPHERE_TILEMAP_START_Y_POS * 8, 
        TILE_ATTR(PAL0, 0, FALSE, FALSE), flags);
    // Starts hidden
    SPR_setVisibility(titanSphereText_2_AnimSpr, HIDDEN);
}

void sphereTextAnimationsPosition (u16 posX, u16 posY)
{
    SPR_setPosition(titanSphereText_1_AnimSpr, posX, posY);
    SPR_setPosition(titanSphereText_2_AnimSpr, posX, posY);
}

void toggleSphereTextAnimations () {
    // We check directly against VISIBLE because sprites settings are only VISIBLE or HIDDEN since their creation
    if (SPR_getVisibility(titanSphereText_1_AnimSpr) == VISIBLE) {
        if (SPR_isAnimationDone(titanSphereText_1_AnimSpr)) {
            SPR_setVisibility(titanSphereText_1_AnimSpr, HIDDEN);
            SPR_setVisibility(titanSphereText_2_AnimSpr, VISIBLE);
            SPR_setFrame(titanSphereText_2_AnimSpr, 0); // reset animation to first frame
            //titanSphereText_2_AnimSpr->status &= ~0x0010; // set NOT STATE_ANIMATION_DONE from sprite_eng.c
        }
    }
    else if (SPR_getVisibility(titanSphereText_2_AnimSpr) == VISIBLE) {
        if (SPR_isAnimationDone(titanSphereText_2_AnimSpr)) {
            SPR_setVisibility(titanSphereText_2_AnimSpr, HIDDEN);
            SPR_setVisibility(titanSphereText_1_AnimSpr, VISIBLE);
            SPR_setFrame(titanSphereText_1_AnimSpr, 0); // reset animation to first frame
            //titanSphereText_1_AnimSpr->status &= ~0x0010; // set NOT STATE_ANIMATION_DONE from sprite_eng.c
        }
    }
}

static u8 spriteAnimManualEffectDelay = 0;
static s8 animOffset = 0; // 0: no direction by default

void updateSphereTextAnimFrameOnJoyInput (u16* buttonBitsChange, u16* buttonBitsState)
{
    if (*buttonBitsChange & BUTTON_RIGHT) {
        if (*buttonBitsState & BUTTON_RIGHT) {
            animOffset = -1; // move the animation backwards
        }
        else {
            if (animOffset < 0) // check if the other direction is not being used
                animOffset = 0;
            *buttonBitsChange &= ~BUTTON_RIGHT; // released
        }
    }
    if (*buttonBitsChange & BUTTON_LEFT) {
        if (*buttonBitsState & BUTTON_LEFT) {
            animOffset = 1; // move the animation fordwards
        }
        else {
            if (animOffset > 0) // check if the other direction is not being used
                animOffset = 0;
            *buttonBitsChange &= ~BUTTON_LEFT; // released
        }
    }

    if (spriteAnimManualEffectDelay == 0 && animOffset != 0) {
        // We check directly against VISIBLE because sprites settings are only VISIBLE or HIDDEN since their creation
        if (SPR_getVisibility(titanSphereText_1_AnimSpr) == VISIBLE) {
            s16 nextFrameInd = titanSphereText_1_AnimSpr->frameInd + animOffset;
            // if we moved back all the animation then we switch them
            if (nextFrameInd < 0) {
                nextFrameInd = titanSphereText_1_AnimSpr->animation->numFrame - 1;
                SPR_setVisibility(titanSphereText_1_AnimSpr, FALSE);
                SPR_setVisibility(titanSphereText_2_AnimSpr, TRUE);
            }
            // if we exceeded last frame animation then we switch them
            else if (nextFrameInd >= titanSphereText_1_AnimSpr->animation->numFrame) {
                nextFrameInd = 0;
                SPR_setVisibility(titanSphereText_1_AnimSpr, FALSE);
                SPR_setVisibility(titanSphereText_2_AnimSpr, TRUE);
            }
            SPR_setFrame(titanSphereText_1_AnimSpr, nextFrameInd);
        }
        else if (SPR_getVisibility(titanSphereText_2_AnimSpr) == VISIBLE) {
            s16 nextFrameInd = titanSphereText_2_AnimSpr->frameInd + animOffset;
            // if we moved back all the animation then we switch them
            if (nextFrameInd < 0) {
                nextFrameInd = titanSphereText_2_AnimSpr->animation->numFrame - 1;
                SPR_setVisibility(titanSphereText_2_AnimSpr, FALSE);
                SPR_setVisibility(titanSphereText_1_AnimSpr, TRUE);
            }
            // if we exceeded last frame animation then we switch them
            else if (nextFrameInd >= titanSphereText_2_AnimSpr->animation->numFrame) {
                nextFrameInd = 0;
                SPR_setVisibility(titanSphereText_2_AnimSpr, FALSE);
                SPR_setVisibility(titanSphereText_1_AnimSpr, TRUE);
            }
            SPR_setFrame(titanSphereText_2_AnimSpr, nextFrameInd);
        }
        spriteAnimManualEffectDelay = SPRITE_ANIM_MANUAL_EFFECT_DELAY_FRAMES;
    }

    if (spriteAnimManualEffectDelay > 0)
        --spriteAnimManualEffectDelay;
}
