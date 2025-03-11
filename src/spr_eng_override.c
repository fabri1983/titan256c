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

#define STATE_ANIMATION_DONE                0x0010

static void setVisibility(Sprite* sprite, u16 newVisibility)
{
    // set new visibility
    sprite->visibility = newVisibility;
}

static u16 updateVisibility(Sprite* sprite, u16 status)
{
    // fabri1983: sprites are always visible, they are unloaded when not used.
    // set the new computed visibility
    setVisibility(sprite, VISIBILITY_ON);
    // visibility update done !
    return status & ~NEED_VISIBILITY_UPDATE;


    u16 visibility;
    const SpriteDefinition* sprDef = sprite->definition;

    // we use 'unsigned' on purpose here to get merged <0 test
    // fabri1983: we know the screenWidth at compilation time
    const u16 sw = 320; // screenWidth;
    // fabri1983: we know the screenHeight at compilation time
    const u16 sh = 224; // screenHeight;
    const u16 w = sprDef->w - 1;
    const u16 h = sprDef->h - 1;
    const u16 x = sprite->x - 0x80;
    const u16 y = sprite->y - 0x80;

    // fabri1983: we don't use flag SPR_FLAG_FAST_AUTO_VISIBILITY
    // fast visibility computation ?
    /*if (status & SPR_FLAG_FAST_AUTO_VISIBILITY)
    {
        // compute global visibility for sprite ('unsigned' allow merged <0 test)
        if (((x + w) < (sw + w)) && ((y + h) < (sh + h)))
            visibility = VISIBILITY_ON;
        else
            visibility = VISIBILITY_OFF;
    }
    else*/
    {
        // sprite is fully visible ? --> set all sprite visible ('unsigned' allow merged <0 test)
        if ((x < (sw - w)) && (y < (sh - h)))
        {
            visibility = VISIBILITY_ON;
        }
        // sprite is fully hidden ? --> set all sprite to hidden ('unsigned' allow merged <0 test)
        else if (((x + w) >= (sw + w)) || ((y + h) >= (sh + h)))
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
                #if SPR_ENG_ALLOW_MULTI_PALS
                FrameVDPSpriteWithPal* frameSprite = (FrameVDPSpriteWithPal*)frame->frameVDPSprites;
                #else
                FrameVDPSprite* frameSprite = frame->frameVDPSprites;
                #endif
                visibility = 0;

                switch(sprite->attribut & (TILE_ATTR_HFLIP_MASK | TILE_ATTR_VFLIP_MASK))
                {
                    case 0:
                        while(num--)
                        {
                            // need to be done first
                            visibility <<= 1;

                            // compute visibility ('unsigned' allow merged <0 test)
                            if (((frameSprite->offsetX + bx) < mx) && ((frameSprite->offsetY + by) < my))
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

                            // compute visibility ('unsigned' allow merged <0 test)
                            if (((frameSprite->offsetXFlip + bx) < mx) && ((frameSprite->offsetY + by) < my))
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

                            // compute visibility ('unsigned' allow merged <0 test)
                            if (((frameSprite->offsetX + bx) < mx) && ((frameSprite->offsetYFlip + by) < my))
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

                            // compute visibility ('unsigned' allow merged <0 test)
                            if (((frameSprite->offsetXFlip + bx) < mx) && ((frameSprite->offsetYFlip + by) < my))
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
    // init timer for this frame *before* frame change callback so it can modify change it if needed.
    if (SPR_getAutoAnimation(sprite))
        sprite->timer = frame->timer;

    // fabri1983: we don't need it
    // frame change event handler defined ? --> call it
    /*if (sprite->onFrameChange)
    {
        // important to preserve status value which may be modified externally here
        sprite->status = status;
        sprite->onFrameChange(sprite);
        status = sprite->status;
    }*/

    // require tile data upload
    if (status & SPR_FLAG_AUTO_TILE_UPLOAD)
        status |= NEED_TILES_UPLOAD;
    // fabri1983: we don't use flag SPR_FLAG_AUTO_VISIBILITY
    // require visibility update
    /*if (status & SPR_FLAG_AUTO_VISIBILITY)
        status |= NEED_VISIBILITY_UPDATE;*/

    // frame update done, also clear ANIMATION_DONE state
    status &= ~(NEED_FRAME_UPDATE | STATE_ANIMATION_DONE);

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
