## Raspberry Pi Pico DMA copy buffer alignment

I wanted to copy a a buffer in striped RAM on the Raspberry Pi Pico to another
buffer, and wondered if the relative alignment between the buffers mattered. It
should.

A question posted at the Raspberry Pi forum gave a few interesting answers:
https://forums.raspberrypi.com/viewtopic.php?t=365987

This is a piece of test code that carefully measures the number of cycles per
word that a DMA copy can sustain, given all different combinations of
alignments and lengths.

#### Theory

It sets up a chain of three separate DMA channel on the same buffer:

    DMA0 -> DMA1 -> DMA2 -> DMA0 -> ...

To maximize the size of the buffer, it copies to itself with a few words
padding distance. The systick timer is set up to count CPU cycles, and the
timer is captured in the DMA interrupt which is triggered at exactly the same
time between the buffer copies since the DMA channels trigger each other
automatically.

A histogram is used to verify that we have minimal jitter: There is never more
than two different cycle counts, and most often there's only one. This is
because every buffer the CPU touches is forced to reside in X scratch space, so
the DMA should be the only thing touching the normal SRAM banks. The code runs
from Flash cache.

(Amusingly, SDK code declared as `__time_critical_func` slowed things down and
added jitter to the result since it forces the function to relocate to normal
SRAM instead of the Flash cache. This started competing with the DMA. I avoided
all such code)


#### Results

We can see that for a 49999 and 50000 count buffer it takes 1 cycle per word
for everything except if there is a 2 word distance (8 bytes) when it takes 1.5
cycles per word.

Oddly enough, for a 49997 and 49998 cuont buffer it take 1 cycle per word for
all offsets.


````
50000 words ..0 to ..0:  50003.0 cycles, 1.00006 cycles/word
50000 words ..0 to ..4:  50003.0 cycles, 1.00006 cycles/word
50000 words ..0 to ..8:  75002.0 cycles, 1.50004 cycles/word !
50000 words ..0 to ..C:  50004.0 cycles, 1.00008 cycles/word
50000 words ..4 to ..0:  50003.0 cycles, 1.00006 cycles/word
50000 words ..4 to ..4:  50003.0 cycles, 1.00006 cycles/word
50000 words ..4 to ..8:  50004.0 cycles, 1.00008 cycles/word
50000 words ..4 to ..C:  75003.0 cycles, 1.50006 cycles/word !
50000 words ..8 to ..0:  75002.0 cycles, 1.50004 cycles/word !
50000 words ..8 to ..4:  50003.0 cycles, 1.00006 cycles/word
50000 words ..8 to ..8:  50003.0 cycles, 1.00006 cycles/word
50000 words ..8 to ..C:  50004.0 cycles, 1.00008 cycles/word
50000 words ..C to ..0:  50003.0 cycles, 1.00006 cycles/word
50000 words ..C to ..4:  75002.0 cycles, 1.50004 cycles/word !
50000 words ..C to ..8:  50003.0 cycles, 1.00006 cycles/word
50000 words ..C to ..C:  50004.0 cycles, 1.00008 cycles/word
49999 words ..0 to ..0:  50002.0 cycles, 1.00006 cycles/word
49999 words ..0 to ..4:  50002.0 cycles, 1.00006 cycles/word
49999 words ..0 to ..8:  75001.0 cycles, 1.50005 cycles/word !
49999 words ..0 to ..C:  50003.0 cycles, 1.00008 cycles/word
49999 words ..4 to ..0:  50002.0 cycles, 1.00006 cycles/word
49999 words ..4 to ..4:  50002.0 cycles, 1.00006 cycles/word
49999 words ..4 to ..8:  50003.0 cycles, 1.00008 cycles/word
49999 words ..4 to ..C:  75002.0 cycles, 1.50007 cycles/word !
49999 words ..8 to ..0:  75001.0 cycles, 1.50005 cycles/word !
49999 words ..8 to ..4:  50002.0 cycles, 1.00006 cycles/word
49999 words ..8 to ..8:  50002.0 cycles, 1.00006 cycles/word
49999 words ..8 to ..C:  50003.0 cycles, 1.00008 cycles/word
49999 words ..C to ..0:  50002.0 cycles, 1.00006 cycles/word
49999 words ..C to ..4:  75001.0 cycles, 1.50005 cycles/word !
49999 words ..C to ..8:  50002.0 cycles, 1.00006 cycles/word
49999 words ..C to ..C:  50003.0 cycles, 1.00008 cycles/word
49998 words ..0 to ..0:  50001.0 cycles, 1.00006 cycles/word
49998 words ..0 to ..4:  50001.0 cycles, 1.00006 cycles/word
49998 words ..0 to ..8:  50002.0 cycles, 1.00008 cycles/word
49998 words ..0 to ..C:  50002.0 cycles, 1.00008 cycles/word
49998 words ..4 to ..0:  50001.0 cycles, 1.00006 cycles/word
49998 words ..4 to ..4:  50001.0 cycles, 1.00006 cycles/word
49998 words ..4 to ..8:  50002.0 cycles, 1.00008 cycles/word
49998 words ..4 to ..C:  50003.0 cycles, 1.00010 cycles/word
49998 words ..8 to ..0:  50003.0 cycles, 1.00010 cycles/word
49998 words ..8 to ..4:  50001.0 cycles, 1.00006 cycles/word
49998 words ..8 to ..8:  50001.0 cycles, 1.00006 cycles/word
49998 words ..8 to ..C:  50002.0 cycles, 1.00008 cycles/word
49998 words ..C to ..0:  50001.0 cycles, 1.00006 cycles/word
49998 words ..C to ..4:  50002.0 cycles, 1.00008 cycles/word
49998 words ..C to ..8:  50001.0 cycles, 1.00006 cycles/word
49998 words ..C to ..C:  50002.0 cycles, 1.00008 cycles/word
49997 words ..0 to ..0:  50000.0 cycles, 1.00006 cycles/word
49997 words ..0 to ..4:  50000.0 cycles, 1.00006 cycles/word
49997 words ..0 to ..8:  50001.0 cycles, 1.00008 cycles/word
49997 words ..0 to ..C:  50001.0 cycles, 1.00008 cycles/word
49997 words ..4 to ..0:  50000.0 cycles, 1.00006 cycles/word
49997 words ..4 to ..4:  50000.0 cycles, 1.00006 cycles/word
49997 words ..4 to ..8:  50001.0 cycles, 1.00008 cycles/word
49997 words ..4 to ..C:  50002.0 cycles, 1.00010 cycles/word
49997 words ..8 to ..0:  50002.0 cycles, 1.00010 cycles/word
49997 words ..8 to ..4:  50000.0 cycles, 1.00006 cycles/word
49997 words ..8 to ..8:  50000.0 cycles, 1.00006 cycles/word
49997 words ..8 to ..C:  50001.0 cycles, 1.00008 cycles/word
49997 words ..C to ..0:  50000.0 cycles, 1.00006 cycles/word
49997 words ..C to ..4:  50001.0 cycles, 1.00008 cycles/word
49997 words ..C to ..8:  50000.0 cycles, 1.00006 cycles/word
49997 words ..C to ..C:  50001.0 cycles, 1.00008 cycles/word
````

