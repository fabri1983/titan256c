## SEGA Megadrive Titan 256 Colors Forever


This a personal project using [SGDK v1.9](https://github.com/Stephane-D/SGDK). 
It's an attempt to simulate the great Titan 512c Forever section from Titan Overdrive mega demo v1.1. 
And learning the functioning of the megadrive and its architecture in the go.  
However this version displays up to **252 colors**.  
**Work In Progress. Only tested with Blastem.**


For convenience test you can directly try the last compiled rom `titan256c_rom.bin`.


### 3 Approaches:  
**Pressing START cycles between them**  

**A**. Writing 2 u32 vars (4 colors) by CPU to CRAM every scanline. That's done 7 times inside the duration of 7 scanlines. 
Also and additional write to BG for the text color ramp effect, only done 3 times in the duration of 7 scanlines. 
And then in the 8th scanline I write the last colors to complete the total of 32 colors palette swap.


**B**. DMA 32 colors (2 palettes) splitted in 3 chunks + 3 BG colors writes for the text color ramp effect. 
That's done inside the duration of 8 scanlines.


**C**. Same than B but calling only once the HInt setting `VDP_setHIntCounter(0)` (every line) and disabling it using 
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


**B** shows two black lines over the text color ramp. Digging around what's the issue.  
**C** still WIP. I'm timing some instructions and see where is the right place to put them.


### TODO:
- Change text ramp effect by using white color instead of transparent.
- Fix black BG color lines over text in approaches B and C.
- Add colorful border as original titan demo has. Likely using shade mode?
- Add DMA command buffering as Steph does. See dma.c.