#include "spr_eng_override.h"
#include <sprite_eng.h>
#include <tools.h>
#include <mapper.h>
#include <sys.h>
#include "titan256c_consts.h"

#define VISIBILITY_ON                       0xFFFF
#define VISIBILITY_OFF                      0x0000

#define ALLOCATED                           0x8000

#define NEED_VISIBILITY_UPDATE              0x0001
#define NEED_FRAME_UPDATE                   0x0002
#define NEED_TILES_UPLOAD                   0x0004

static void setVisibility(Sprite* sprite, u16 newVisibility)
{
    // set new visibility
    sprite->visibility = newVisibility;
}

static u16 updateVisibility(Sprite* sprite, u16 status)
{
    u16 visibility;
    const SpriteDefinition* sprDef = sprite->definition;

    const u16 sw = screenWidth;
    const u16 sh = screenHeight;
    const u16 w = sprDef->w - 1;
    const u16 h = sprDef->h - 1;
    const u16 x = sprite->x - 0x80;
    const u16 y = sprite->y - 0x80;

    // fabri1983: we don't use flag SPR_FLAG_FAST_AUTO_VISIBILITY
    // fast visibility computation ?
    /*if (status & SPR_FLAG_FAST_AUTO_VISIBILITY)
    {
        // compute global visibility for sprite (use unsigned for merged <0 test)
        if (((u16)(x + w) < (u16)(sw + w)) && ((u16)(y + h) < (u16)(sh + h)))
            visibility = VISIBILITY_ON;
        else
            visibility = VISIBILITY_OFF;
    }
    else*/
    {
        // sprite is fully visible ? --> set all sprite visible (use unsigned for merged <0 test)
        if ((x < (u16)(sw - w)) && (y < (u16)(sh - h)))
        {
            visibility = VISIBILITY_ON;
        }
        // sprite is fully hidden ? --> set all sprite to hidden (use unsigned for merged <0 test)
        else if (((u16)(x + w) >= (u16)(sw + w)) || ((u16)(y + h) >= (u16)(sh + h)))
        {
            visibility = VISIBILITY_OFF;
        }
        else
        {
            const u16 bx = x + 0x1F;    // max hardware sprite size = 32
            const u16 by = y + 0x1F;
            const u16 mx = sw + 0x1F;   // max hardware sprite size = 32
            const u16 my = sh + 0x1F;

            AnimationFrame* frame = sprite->frame;
            s8 num = frame->numSprite;

            // special case of single VDP sprite with size aligned to sprite size (no offset, no flip calculation required)
            if (num < 0)
                visibility = ((bx < mx) && (by < my))?0x8000:0;
            else
            {
                FrameVDPSprite* frameSprite = frame->frameVDPSprites;
                visibility = 0;

                switch(sprite->attribut & (TILE_ATTR_HFLIP_MASK | TILE_ATTR_VFLIP_MASK))
                {
                    case 0:
                        while(num--)
                        {
                            // need to be done first
                            visibility <<= 1;

                            // compute visibility (use unsigned for merged <0 test)
                            if (((u16)(frameSprite->offsetX + bx) < mx) && ((u16)(frameSprite->offsetY + by) < my))
                                visibility |= 1;

                            // next
                            frameSprite++;
                        }
                        break;

                    case TILE_ATTR_HFLIP_MASK:
                        while(num--)
                        {
                            // need to be done first
                            visibility <<= 1;

                            // compute visibility (use unsigned for merged <0 test)
                            if (((u16)(frameSprite->offsetXFlip + bx) < mx) && ((u16)(frameSprite->offsetY + by) < my))
                                visibility |= 1;

                            // next
                            frameSprite++;
                        }
                        break;

                    case TILE_ATTR_VFLIP_MASK:
                        while(num--)
                        {
                            // need to be done first
                            visibility <<= 1;

                            // compute visibility (use unsigned for merged <0 test)
                            if (((u16)(frameSprite->offsetX + bx) < mx) && ((u16)(frameSprite->offsetYFlip + by) < my))
                                visibility |= 1;

                            // next
                            frameSprite++;
                        }
                        break;

                    case TILE_ATTR_HFLIP_MASK | TILE_ATTR_VFLIP_MASK:
                        while(num--)
                        {
                            // need to be done first
                            visibility <<= 1;

                            // compute visibility (use unsigned for merged <0 test)
                            if (((u16)(frameSprite->offsetXFlip + bx) < mx) && ((u16)(frameSprite->offsetYFlip + by) < my))
                                visibility |= 1;

                            // next
                            frameSprite++;
                        }
                        break;
                }

                // so visibility is in high bits
                visibility <<= (16 - frame->numSprite);
            }
        }
    }

    // set the new computed visibility
    setVisibility(sprite, visibility);

    // visibility update done !
    return status & ~NEED_VISIBILITY_UPDATE;
}

static u16 updateFrame(Sprite* sprite, u16 status)
{
    AnimationFrame* frame = sprite->animation->frames[sprite->frameInd];

    // fabri1983: we don't delay frame update
    // we need to transfert tiles data for this sprite and frame delay is not disabled ?
    /*if ((status & (SPR_FLAG_AUTO_TILE_UPLOAD | SPR_FLAG_DISABLE_DELAYED_FRAME_UPDATE)) == SPR_FLAG_AUTO_TILE_UPLOAD)
    {
        // not enough DMA capacity to transfer sprite tile data ?
        const u16 dmaCapacity = DMA_getMaxTransferSize();

        if (dmaCapacity && ((DMA_getQueueTransferSize() + (frame->tileset->numTile * 32)) > dmaCapacity))
        {
            // initial frame update ? --> better to set frame at least
            if (sprite->frame == NULL)
                sprite->frame = frame;

            // delay frame update (when we will have enough DMA capacity to do it)
            return status;
        }
    }*/

    // set frame
    sprite->frame = frame;

    // fabri1983: we don't need it
    // frame change event handler defined ? --> call it
    /*if (sprite->onFrameChange)
    {
        // important to preserve status value which may be modified externally here
        sprite->status = status;
        sprite->onFrameChange(sprite);
        status = sprite->status;

        // init timer for this frame *after* callback call and only if auto animation is enabled and timer was not manually changed in callback
        if (sprite->timer == 0)
            sprite->timer = frame->timer;
    }
    else*/
    {
        // init timer for this frame *after* callback call to allow SPR_isAnimationDone(..) to correctly report TRUE when animation is done in the callbacnk
        if (SPR_getAutoAnimation(sprite))
            sprite->timer = frame->timer;
    }

    // require tile data upload
    if (status & SPR_FLAG_AUTO_TILE_UPLOAD)
        status |= NEED_TILES_UPLOAD;
    // fabri1983: we don't use flag SPR_FLAG_AUTO_VISIBILITY
    // require visibility update
    /*if (status & SPR_FLAG_AUTO_VISIBILITY)
        status |= NEED_VISIBILITY_UPDATE;*/

    // frame update done
    status &= ~NEED_FRAME_UPDATE;

    return status;
}

static FORCE_INLINE void loadTiles (Sprite* sprite)
{
    TileSet* tileset = sprite->frame->tileset;

    // need to test for empty tileset (blank frame)
    if (tileset->numTile)
    {
        u16 lenInWord = (tileset->numTile * 32) / 2;

        // TODO: separate tileset per VDP sprite and only unpack/upload visible VDP sprite (using visibility) to VRAM

        // need unpacking ?
        #if TITAN_SPHERE_TEXT_COMPRESSED
        u16 compression = tileset->compression;
        if (compression != COMPRESSION_NONE)
        {
            // get buffer
            u8* buf = DMA_allocateTemp(lenInWord);

            // unpack in temp buffer obtained from DMA queue
            //if (buf)
            {
                unpackSelector(compression, (u8*) FAR_SAFE(tileset->tiles, lenInWord * 2), buf);
                DMA_queueDmaFast(DMA_VRAM, buf, (sprite->attribut & TILE_INDEX_MASK) * 32, lenInWord, 2);
                DMA_releaseTemp(lenInWord);
            }
        }
        else
        #endif
        {
            // just DMA operation to transfer tileset data to VRAM
            DMA_queueDma(DMA_VRAM, FAR_SAFE(tileset->tiles, lenInWord * 2), (sprite->attribut & TILE_INDEX_MASK) * 32, lenInWord, 2);
        }
    }
}

void NO_INLINE spr_eng_update()
{
    Sprite* sprite = firstSprite;
    // SAT pointer
    VDPSprite* vdpSprite = vdpSpriteCache;
    // VDP sprite index (for link field)
    u8 vdpSpriteInd = 1;

    // fabri1983: we don't use this method for showing frame load
    // first sprite used by CPU load monitor
    /*if (SYS_getShowFrameLoad())
    {
        // goes to next VDP sprite then
        vdpSprite->link = vdpSpriteInd++;
        vdpSprite++;
    }*/

    // iterate over all sprites
    while(sprite)
    {
        s16 timer = sprite->timer;

        // handle frame animation
        if (timer > 0)
        {
            // timer elapsed --> next frame
            if (--timer == 0) SPR_nextFrame(sprite);
            // just update remaining timer
            else sprite->timer = timer;
        }

        u16 status = sprite->status;

        // order is important: updateFrame first then updateVisibility
        if (status & NEED_FRAME_UPDATE)
            status = updateFrame(sprite, status);
        if (status & NEED_VISIBILITY_UPDATE)
            status = updateVisibility(sprite, status);

        // sprite visible and still allocated (can be released during updateFrame(..) with the frame change callback) with enough entry in SAT ?
        if (sprite->visibility && (status & ALLOCATED) && (vdpSpriteInd <= SAT_MAX_SIZE))
        {
            if (status & NEED_TILES_UPLOAD)
            {
                loadTiles(sprite);
                // tiles upload and sprite table done
                status &= ~NEED_TILES_UPLOAD;
            }

            // update SAT now
            AnimationFrame* frame = sprite->frame;
            FrameVDPSprite* frameSprite = frame->frameVDPSprites;
            u16 attr = sprite->attribut;
            s8 numSprite = frame->numSprite;

            // special case of single VDP sprite with size aligned to sprite size (no offset, no flip calculation required)
            if (numSprite < 0)
            {
                vdpSprite->y = sprite->y;
                vdpSprite->size = frameSprite->size;
                vdpSprite->link = vdpSpriteInd++;
                vdpSprite->attribut = attr;
                vdpSprite->x = sprite->x;
                vdpSprite++;
            }
            else
            {
                static const u16 visibilityMask[17] =
                {
                    0x0000, 0x8000, 0xC000, 0xE000, 0xF000, 0xF800, 0xFC00, 0xFE00,
                    0xFF00, 0xFF80, 0xFFC0, 0xFFE0, 0xFFF0, 0xFFF8, 0xFFFC, 0xFFFE,
                    0xFFFF
                };

                // so visibility also allow to get the number of sprite
                s16 visibility = (s16)(sprite->visibility & visibilityMask[(u8) numSprite]);

                switch(attr & (TILE_ATTR_VFLIP_MASK | TILE_ATTR_HFLIP_MASK))
                {
                    case 0:
                        while(visibility)
                        {
                            // current sprite visibility bit is in high bit
                            if (visibility < 0)
                            {
                                vdpSprite->y = sprite->y + frameSprite->offsetY;
                                vdpSprite->size = frameSprite->size;
                                vdpSprite->link = vdpSpriteInd++;
                                vdpSprite->attribut = attr;
                                vdpSprite->x = sprite->x + frameSprite->offsetX;
                                vdpSprite++;
                            }

                            // increment tile index in attribut field
                            attr += frameSprite->numTile;
                            // next
                            frameSprite++;
                            // next VDP sprite
                            visibility <<= 1;
                        }
                        break;

                    case TILE_ATTR_HFLIP_MASK:
                        while(visibility)
                        {
                            // current sprite visibility bit is in high bit
                            if (visibility < 0)
                            {
                                vdpSprite->y = sprite->y + frameSprite->offsetY;
                                vdpSprite->size = frameSprite->size;
                                vdpSprite->link = vdpSpriteInd++;
                                vdpSprite->attribut = attr;
                                vdpSprite->x = sprite->x + frameSprite->offsetXFlip;
                                vdpSprite++;
                            }

                            // increment tile index in attribut field
                            attr += frameSprite->numTile;
                            // next
                            frameSprite++;
                            // next VDP sprite
                            visibility <<= 1;
                        }
                        break;

                    case TILE_ATTR_VFLIP_MASK:
                        while(visibility)
                        {
                            // current sprite visibility bit is in high bit
                            if (visibility < 0)
                            {
                                vdpSprite->y = sprite->y + frameSprite->offsetYFlip;
                                vdpSprite->size = frameSprite->size;
                                vdpSprite->link = vdpSpriteInd++;
                                vdpSprite->attribut = attr;
                                vdpSprite->x = sprite->x + frameSprite->offsetX;
                                vdpSprite++;
                            }

                            // increment tile index in attribut field
                            attr += frameSprite->numTile;
                            // next
                            frameSprite++;
                            // next VDP sprite
                            visibility <<= 1;
                        }
                        break;

                    case (TILE_ATTR_VFLIP_MASK | TILE_ATTR_HFLIP_MASK):
                        while(visibility)
                        {
                            // current sprite visibility bit is in high bit
                            if (visibility < 0)
                            {
                                vdpSprite->y = sprite->y + frameSprite->offsetYFlip;
                                vdpSprite->size = frameSprite->size;
                                vdpSprite->link = vdpSpriteInd++;
                                vdpSprite->attribut = attr;
                                vdpSprite->x = sprite->x + frameSprite->offsetXFlip;
                                vdpSprite++;
                            }

                            // increment tile index in attribut field
                            attr += frameSprite->numTile;
                            // next
                            frameSprite++;
                            // next VDP sprite
                            visibility <<= 1;
                        }
                        break;
                }
            }
        }

        // processes done
        sprite->status = status;
        // next sprite
        sprite = sprite->next;
    }

    // remove 1 to get number of hard sprite used
    vdpSpriteInd--;

    // something to display ?
    if (vdpSpriteInd > 0)
    {
        // get back to last sprite
        vdpSprite--;
        // mark as end
        vdpSprite->link = 0;
        // send sprites to VRAM
        DMA_queueDmaFast(DMA_VRAM, vdpSpriteCache, VDP_SPRITE_TABLE, vdpSpriteInd * (sizeof(VDPSprite) / 2), 2);
    }
    // no sprite to display
    else
    {
        // set 1st sprite off screen and mark as end
        vdpSprite->y = 0;
        vdpSprite->link = 0;
        // send sprites to VRAM
        DMA_queueDmaFast(DMA_VRAM, vdpSpriteCache, VDP_SPRITE_TABLE, 1 * (sizeof(VDPSprite) / 2), 2);
    }
}
