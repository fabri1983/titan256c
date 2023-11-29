## Megadrive Titan 256 Colors Forever


This is a personal project using [SGDK v1.9](https://github.com/Stephane-D/SGDK) (with latest changes) 
as an attempt to simulate the great Titan 512C Forever section from Titan Overdrive mega demo v1.1, 
and learning the functioning of the megadrive and its architecture in the go.  
This version displays up to **252 colors**.  
**Work In Progress. Only tested with Blastem.**


For convenience test you can directly try the last compiled rom `titan256c_rom.bin`.


### 3 Approaches:  
**Pressing START cycles between them**  

**A**. Writing 2 u32 vars (4 colors) by CPU to CRAM every scanline. That's done 7 times inside the duration of 7 scanlines. 
Also and additional write to BG for the text color ramp effect, only done 3 times in the duration of 7 scanlines. 
Finally in the 8th scanline it writes the last colors to complete the total of 32 colors palette swap.


**B**. DMA 32 colors (2 palettes) splitted in 3 chunks + 3 BG colors writes for the text color ramp effect. 
All done inside the duration of 8 scanlines.


**C**. Same than B but calling only once the HInt with `VDP_setHIntCounter(0)` (every scanline) and disabling it using 
`VDP_setHIntCounter(255)` at the first line of the HInt.  
In this approach I manage to get only one invocation out of the two the HInt is being invoked, because changing the invocation 
counter only takes effect in the next one. Here is where I use `if GET_VCOUNTER > 0 then exit from HInt` in order to dicard 
the second unwanted HInt invocation. Ugly hack but this is what I came up so far.



**A**
![titan_cpu.jpg](screenshots/titan_cpu.jpg?raw=true "titan_cpu.jpg")


**B**
![titan_dma.jpg](screenshots/titan_dma.jpg?raw=true "titan_dma.jpg")


**C**
![titan_dma_onetime.jpg](screenshots/titan_dma_onetime.jpg?raw=true "titan_dma_onetime.jpg")


**B** and **C** show two black lines over the text color ramp. Digging around what's the issue.


### TODO:
- Approch A likely has room to update text ramp color in every scanline.
- Fix black BG color lines over text in approaches B and C.
- Add DMA command buffering as Steph does. See dma.c.
- Try to use titan256c_rgb.png as input (this need a fix in tiledpalettequant to accept an specific color as transparent).