#ifndef HIST_H
#define HIST_H

#include <assert.h>
#include <string.h>

#ifndef HIST_BINS
#define HIST_BINS 64
#endif

// TODO: Make it work with signed values
typedef unsigned hist_val_t;

typedef struct hist_s {
  unsigned n;

  // Raw counts
  hist_val_t min, max;
  long long tot;

  // Values are shifted down this many bits before counting
  unsigned shift;

  // Shifted value of index 0
  hist_val_t lo;

  // Each bin spans (i+lo)<<shift to ((i+1+lo)<<shift)-1

  // Number of bins used: count[used...] = 0
  unsigned used;

  unsigned bin[HIST_BINS];
} hist_t;

static inline void hist_clear(hist_t *h) { *h = (hist_t){0}; }

static inline hist_val_t hist_bin_lo(const hist_t *h,unsigned i) {
  return (h->lo+i)<<h->shift;
}
static inline hist_val_t hist_bin_hi(const hist_t *h,unsigned i) {
  return ((h->lo+i+1)<<h->shift)-1;
}

// Rescale to a higher shift value, better done multiple times
static inline void hist_scale(hist_t *h) {
  unsigned *w = h->bin;
  unsigned n = h->used;
  if(h->lo&1) {
    ++w;
    --n;
  }
  unsigned *r = w;
  for(;n>=2;n-=2) {
    unsigned t = *r++;
    *w++ = t+*r++;
  }
  if(n)
    *w++ = *r++;
  h->used = w - h->bin;
  h->lo>>=1;
  ++h->shift;
}

// Add space at the bottom for more entries
static inline void hist_shift(hist_t *h,unsigned n) {
  memmove(h->bin+n, h->bin, sizeof(*h->bin)*h->used);
  memset(h->bin,0,sizeof(*h->bin)*n);
  h->used += n;
  h->lo -= n;
}

// Make sure that x fits; maybe nothing is necessary
static inline void hist_rescale(hist_t *h,hist_val_t x) {
  hist_val_t min = x<h->min ? x : h->min;
  hist_val_t max = x>h->max ? x : h->max;
  
  // First determine the shift size
  unsigned shift = h->shift;
  for(;;) {
    if((max>>shift)-(min>>shift)<HIST_BINS)
      break;
    ++shift;
  }

  while(h->shift<shift)
    hist_scale(h);

  unsigned lo = min>>shift;

  if(lo!=h->lo)
    hist_shift(h,h->lo-lo);
}

static inline void hist_insert(hist_t *h,hist_val_t x) {

  if(h->n==0) {
    h->tot = x;
    h->lo = x;
    h->min = h->max = x;
    h->bin[0] = 1;
    h->used = 1;
    h->n = 1;
    return;
  }

  // We may need a scale and shift; compare with the current limits
  if( x<h->min || x>h->max )
    hist_rescale(h,x);

  hist_val_t scaled = x >> h->shift;
  assert( scaled >= h->lo );

  hist_val_t shifted = scaled - h->lo;
  assert( shifted < HIST_BINS );

  ++h->n;
  h->tot += x;
  if(x<h->min) h->min = x;
  if(x>h->max) h->max = x;

  // Clear any intermediate values
  while(h->used<=shifted)
    h->bin[h->used++] = 0;

  ++h->bin[shifted];
}



#endif /* HIST_H */
