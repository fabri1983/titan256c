## Megadrive Titan 256 Colors Forever

This is a personal project using [SGDK v2.12](https://github.com/Stephane-D/SGDK) (with custom modifications) 
as an attempt to recreate the great Titan 512C Forever section from Titan Overdrive mega demo v1.1, 
and learning the functioning of the megadrive and its architecture in the go.  
This version displays **256+ colors**.

**Work In Progress. Only tested with Blastem and Nuked-MD.**

For convenience testing you can directly try the last compiled rom [titan256c_rom.bin](titan256c_rom.bin?raw=true "titan256c_rom.bin").

## 5 Strategies  

**(Pressing START cycles between them)**  

**Strategies** **A** and **B**.  
Strategy A is written in ASM 68k. Strategy B is written in C.  
Swapping writes beetwen:  
- Two u32 values holding 4 colors from palette, and 1 u16 value as BG color for text ramp effect.  
And
- Four u32 values holding 8 colors from palette.
The writes are made by CPU to CRAM every scanline along the duration of 8 scanlines, sending a total 2 palettes (32 colors) and 4 bg colors (text color ramp effect).  

These strategies has better ramp effect since the change in color happens every other scanline.

**Strategies** **C** and **D**.  
DMA of 32 colors holding 2 palettes splitted in 3 chunks, plus 4 BG color writes (text color ramp effect).  
Strategy C is written in ASM 68k. Strategy D is written in C.  
All done inside the duration of 8 scanlines.

**Strategy** **E**.  
Same than D but calling only once the HInt with `VDP_setHIntCounter(0)` (every scanline) and disabling it using 
`VDP_setHIntCounter(255)` at the first line of the HInt (which takes effect in the next interrupt).

Strategy **A**:  
![titan_cpu.jpg](screenshots/titan_cpu.jpg?raw=true "titan_cpu.jpg")

Strategy **C**:  
![titan_dma.jpg](screenshots/titan_dma.jpg?raw=true "titan_dma.jpg")


### TODO:
- When on PAL, use VDP trick to avoid top and bottom bands:
Basically the VDP only checks where the display ends on the specific scanline it should end.
So if you switch to V30 (240px tall) before line 224, then wait until V counter is between 225 and 239, then switch back to V28 (224px tall), 
the VDP will never see the "end" and keep rendering forever.
This doesn't affect sync (as long as you always have the same V28/V30 setting by the time vsync comes it'll be safe) but it completely gets 
rid of the top and bottom borders.
Note that this also means no vblank time at all (well, unless you manually enable or disable display to make up for it).
- Fix display corruption on strategy C. Possibly due to how DMA is prepared.
- Fix incorrect value for VDP_setHIntCounter() when on bouncing effect to avoid wrong strip and palette alignment.
- Fix TITAN_256C_FADE_TO_BLACK_STRATEGY 0 and 1: wrong bitwise operations.
- Add DMA command buffering as Stef does to avoid error in some MD consoles. See dma.c.
- Try to use titan256c_rgb.png as input and do the color ramp effect over the white text color.
