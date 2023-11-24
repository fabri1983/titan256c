## SEGA Megadrive Titan 256 colors Forever


This a personal project using [SGDK v1.9](https://github.com/Stephane-D/SGDK). 
It's an attempt to simulate the great Titan 512c Forever demo from Titan Overdrive mega demo v1.1.  
However this version displays up to 250 colors. Still work in progress.


**3 approaches:**  
**A**. Writing 2 u32 vars (4 colors) by CPU to CRAM every scanline. That's done 7 times inside the duration of 7 scanlines. 
Also and additional write to BG for the text color ramp effect, only done 3 times in the duraiotn of 7 scanlines. 
And then in the 8th scanline I write the last colors to complete the total of 32 colors.
**B**. DMAing 32 colors (2 palettes) splitted in 3 chunks + 3 BG colors writes. That's done inside the duration of 8 scanlines.
**C**. Same than B but calling only once to the HInt and disabling it using `VDP_setHIntCounter(255)`. 
In this approach I handle the 2 times the HInt is invoked, because changing its invocation counter only takes effect in the 
next invocation. Here is where I'm using `if GET_VCOUNTER > 0 then exit from HInt` (this dicards the second unwanted HInt invocation).


*A*
![titan_cpu.jpg](screenshots/titan_cpu.jpg?raw=true "titan_cpu.jpg")


*B*
![titan_dma.jpg](screenshots/titan_dma.jpg?raw=true "titan_dma.jpg")


*C*
![titan_dma_onetime.jpg](screenshots/titan_dma_onetime.jpg?raw=true "titan_dma_onetime.jpg")


So far only **B** approach has no CRAM dots in Blastem.  
**A** has one region with some flickering in continuos dots. The captured image doesn't show that.  
**C** still wip. I'm timing some instructions and see where is the right place to put them.
