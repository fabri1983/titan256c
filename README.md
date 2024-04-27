## Megadrive Titan 256 Colors Forever


This is a personal project using [SGDK v2.0](https://github.com/Stephane-D/SGDK) (with latest changes) 
as an attempt to simulate the great Titan 512C Forever section from Titan Overdrive mega demo v1.1, 
and learning the functioning of the megadrive and its architecture in the go.  
This version displays up to **256 colors**.


**Work In Progress. Only tested with Blastem and Nuked-MD.**


For convenience test you can directly try the last compiled rom `titan256c_rom.bin`.


### 4 Approaches:  
**(pressing START cycles between them)**  

**A** and **B**. Interweaving writes beetwen:  
A is written in ASM 68k, B is written in C.  
- 2 u32 vars (4 colors from palette) and 1 u16 var (BG color for text ramp effect)  
and
- 4 u32 vars (8 colors from palette)
by CPU to CRAM every scanline along the duration of 8 scanlines, sending a total 2 palettes (32 colors) and 4 bg colors (ramp effect).  
This approach has better ramp effect since the change in color happens every other scanline.


**C**. DMA 32 colors (2 palettes) splitted in 3 chunks + 3 BG colors writes for the text color ramp effect.  
Written in C.  
All done inside the duration of 8 scanlines.


**D**. Same than C but calling only once the HInt with `VDP_setHIntCounter(0)` (every scanline) and disabling it using 
`VDP_setHIntCounter(255)` at the first line of the HInt (which takes effect in the next line).


**A** and **B**:  
![titan_cpu.jpg](screenshots/titan_cpu.jpg?raw=true "titan_cpu.jpg")


**C**:  
![titan_dma.jpg](screenshots/titan_dma.jpg?raw=true "titan_dma.jpg")


**D**:  
![titan_dma_onetime.jpg](screenshots/titan_dma_onetime.jpg?raw=true "titan_dma_onetime.jpg")


### TODO:
- Fix incorrect set of black palette below the image when on bouncing effect.
- Fix black BG color lines over text in approaches using DMA. Check if they appear in Nuked-MD.
- Add DMA command buffering as Stef does. See dma.c.
- Try to use titan256c_rgb.png as input and do the color ramp effect over the white color.
- Add a new version of the algorithm in which it splits the image in 4 rows of pixels, hence forcing the HInt color swap to be executed every 4 scanlines.
