#include <stdio.h>
#include "pico/stdio.h"
#include "hardware/dma.h"
#include "hardware/irq.h"
#include "hardware/structs/systick.h"

// Custom "fast" histogram
#define HIST_COUNT 32
#include "hist.h"

// Number of channels chained together - 2 was buggy, 3 is safe
#define CHANS 3

// (Maximum) words per transfer
#define LEN 50000

// Number of runs for averaging the results
#define RUNS 1000

// 4+4 extra to allow for "random" source and destination alignment
// 4 extra to allow for a safe distance between read and write
// aligned(16) to make our prints nicer
static uint32_t __uninitialized_ram(buf)[LEN+12] __attribute__((aligned(16)));

// Everything touched during operation should be in core 0 scratch space
__scratch_x("run") static dma_channel_config c[CHANS];
__scratch_x("run") volatile bool running;
__scratch_x("run") static struct run_s {
  void *src, *dst;
  unsigned words;
  int count;
  int n;

  uint32_t first, last;

  // Overkill: Histogram
  hist_t hist;

  uint isr[CHANS];
} run;


static inline void channel_refresh(uint ch,int left) {
  channel_config_set_chain_to(c+ch,left>CHANS ? (ch+1)%CHANS : ch);
  dma_channel_configure(ch,c+ch,run.dst,run.src,run.words,false);
}

static void dma_handler(void) {
  // It's important for low jitter that time is snapped first
	uint32_t now = systick_hw->cvr;

  // Figure out which channel trigged us
  uint ch;
  for(ch=0;ch<CHANS;++ch)
    if(dma_channel_get_irq0_status(ch))
      break;
  if(ch>=CHANS)
    panic("Invalid IRQ");
  dma_channel_acknowledge_irq0(ch);
  ++run.isr[ch];

  --run.count;
  if(run.count==0)
    running = false;
  if(run.count<0)
    panic("Count underflow");
  channel_refresh(ch,run.count);

  if(run.n==0) {
    run.first = now;
  } else {
    uint32_t delta = (run.last-now) & 0xFFFFFF;
    if(run.n>=CHANS)
      hist_insert(&run.hist,delta);
  }

  run.n++;
  run.last = now;
}

static void setup_bench(int rep,size_t words,size_t srcalign,size_t dstalign) {
  run = (struct run_s){
    .src = buf+4+4+srcalign,
    .dst = buf+dstalign,
    .words = words,
    .count = rep+CHANS,
  };
  printf("%u words ..%X to ..%X: ",
         run.words,(uintptr_t)run.src&0xF,(uintptr_t)run.dst&0xF);
  for(uint ch=0;ch<CHANS;++ch)
    channel_refresh(ch,rep+ch);
}

static void print_result(void) {
  const hist_t *h = &run.hist;
  double mean = (double)h->tot/h->n;
  double cpw = mean/run.words;
  printf(" %g cycles, %g cycles/word%s\n",mean,cpw,cpw>1.01?" !":"");
//  printf("%u entries between %u and %u, mean = %g cycles, %g cycles/word\n",
//         h->n, h->min, h->max, mean, mean/run.words);
//  for(unsigned i=0;i<h->used;++i) {
//    if(h->shift)
//      printf("  %8u counts in [%u-%u]\n",h->bin[i],
//             hist_bin_lo(h,i),hist_bin_hi(h,i));
//    else
//      printf("  %8u counts at %u\n",h->bin[i],hist_bin_lo(h,i));
//  }
}



int main(void) {
  stdio_init_all();

  // Set up systick to count clock cycles with maximum period
  systick_hw->csr = 0x5;
	systick_hw->rvr = 0xFFFFFF;

  // Initialize common DMA channel properties and save the config
  for(uint ch=0;ch<CHANS;++ch) {
    dma_channel_claim(ch);
    c[ch] = dma_channel_get_default_config(ch);
    channel_config_set_read_increment(c+ch,true);
    channel_config_set_write_increment(c+ch,true);
    dma_channel_set_irq0_enabled(ch,true);
  }

  // The IRQ is important for timing - exclusive and high priority
  irq_set_exclusive_handler(DMA_IRQ_0,dma_handler);
  irq_set_priority(DMA_IRQ_0, PICO_HIGHEST_IRQ_PRIORITY);
  irq_set_enabled(DMA_IRQ_0,true);

  for(unsigned pass=1;;++pass) {
    printf("\n\n==== Pass %u ====\n\n",pass);
    for(size_t len=LEN;len>LEN-4;--len) {
      for(size_t a0=0;a0<4;++a0) {
        for(size_t a1=0;a1<4;++a1) {
          setup_bench(RUNS,len,a0,a1);
          running = true;
          dma_channel_start(0);
          // Believe it or not, this is the least intrusive wait
          while(running)
            tight_loop_contents();
          print_result();
        }
      }
    }
  }
}
