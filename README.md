## Megadrive Titan 256 Colors Forever

This is a personal project using [SGDK v2.0](https://github.com/Stephane-D/SGDK) (with latest changes) 
as an attempt to simulate the great Titan 512C Forever section from Titan Overdrive mega demo v1.1, 
and learning the functioning of the megadrive and its architecture in the go.  
This version displays up to **256 colors**.

**Work In Progress. Only tested with Blastem and Nuked-MD.**

For convenience testing you can directly try the last compiled rom [titan256c_rom.bin](titan256c_rom.bin?raw=true "titan256c_rom.bin").

## 4 Strategies  

**(pressing START cycles between them)**  

**A** and **B**.  
Strategy A is written in ASM 68k. Strategy B is written in C.  
Interweaving writes beetwen:  
- 2 u32 vars (4 colors from palette) and 1 u16 var (BG color for text ramp effect)  
and
- 4 u32 vars (8 colors from palette)
by CPU to CRAM every scanline along the duration of 8 scanlines, sending a total 2 palettes (32 colors) and 4 bg colors (ramp effect).  
This strategy has better ramp effect since the change in color happens every other scanline.

**C** and **D**.  
DMA 32 colors (2 palettes) splitted in 3 chunks + 4 BG colors writes for the text color ramp effect.  
Strategy C is written in ASM 68k. Strategy D is written in C.  
All done inside the duration of 8 scanlines.

**E**.  
Same than D but calling only once the HInt with `VDP_setHIntCounter(0)` (every scanline) and disabling it using 
`VDP_setHIntCounter(255)` at the first line of the HInt (which takes effect in the next line).

Strategy **A**:  
![titan_cpu.jpg](screenshots/titan_cpu.jpg?raw=true "titan_cpu.jpg")

Strategy **C**:  
![titan_dma.jpg](screenshots/titan_dma.jpg?raw=true "titan_dma.jpg")


### TODO:
- Fix display corruption on strategy C.
- Fix incorrect value for VDP_setHIntCounter() when on bouncing effect to avoid wrong strip <-> palette alignment.
- Fix black BG color lines over text in strategies with DMA. Check if they appear in Nuked-MD.
- Fix TITAN_256C_FADE_TO_BLACK_STRATEGY 0 and 1: wrong bitwise operations.
- Add DMA command buffering as Stef does to avoid error in some MD consoles. See dma.c.
- Try to use titan256c_rgb.png as input and do the color ramp effect over the white color.
