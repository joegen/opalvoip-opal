/*
 * lfsr.c
 *
 */


#include <stdio.h>
#include "datatypes.h"

uint32_t 
parity(uint32_t x) {

  x ^= (x >> 16);
  x ^= (x >> 8);
  x ^= (x >> 4);
  x ^= (x >> 2);
  x ^= (x >> 1);

  return x & 1;
}


/* typedef struct { */
/*   uint32_t register[8]; */
/* } lfsr_t; */

void
compute_period(uint32_t feedback_polynomial) {
  int i;
  v32_t lfsr;
  v32_t mask;

  mask.value = feedback_polynomial;
  lfsr.value = 1;

  printf("polynomial: %s\t", v32_bit_string(mask));

  for (i=0; i < 256; i++) {
/*     printf("%s\n", v32_bit_string(lfsr)); */
    if (parity(mask.value & lfsr.value))
      lfsr.value = ((lfsr.value << 1) | 1) & 0xff;
    else
      lfsr.value = (lfsr.value << 1) & 0xff;
    
    /* now halt if we're back at the initial state */
    if (lfsr.value == 1) {
      printf("period: %d\n", i);
      break;
    }
  }
}

uint32_t poly0 = 223;


uint32_t polynomials[39] = {
31, 
47,
55,
59,
61,
79,
87,
91,
103,
107,
109,
115,
117,
121,
143,
151,
157,
167,
171,
173,
179,
181,
185,
199,
203,
205,
211,
213,
227,
229,
233, 
241,
127,
191,
223, 
239,
247,
251,
253 
};

char binary_string[32];

char *
u32_bit_string(uint32_t x, unsigned int length) {
  unsigned int mask;
  int index;
 
  mask = 1 << length;
  index = 0;
  for (; mask > 0; mask >>= 1)
    if ((x & mask) == 0)
      binary_string[index++] = '0';
    else
      binary_string[index++] = '1';

  binary_string[index++] = 0;  /* NULL terminate string */
  return binary_string;
}

extern int octet_weight[256];

unsigned int 
weight(uint32_t poly) {
  int wt = 0;

  /* note: endian-ness makes no difference */
  wt += octet_weight[poly        & 0xff]; 
  wt += octet_weight[(poly >> 8) & 0xff];
  wt += octet_weight[(poly >> 16) & 0xff];
  wt += octet_weight[(poly >> 24)];

  return wt;
}

#define MAX_PERIOD 65535

#define debug_print 0

int
period(uint32_t poly) {
  int i;
  uint32_t x;


  /* set lfsr to 1 */
  x = 1;
#if debug_print
  printf("%d:\t%s\n", 0, u32_bit_string(x,8));
#endif
  for (i=1; i < MAX_PERIOD; i++) {
    if (x & 1) 
      x = (x >> 1) ^ poly;
    else
      x = (x >> 1);

#if debug_print
    /* print for a sanity check */
    printf("%d:\t%s\n", i, u32_bit_string(x,8));
#endif

    /* check for return to original value */
    if (x == 1)
      return i;
  }
  return i;
}

/*
 * weight distribution computes the weight distribution of the
 * code generated by the polynomial poly
 */

#define MAX_LEN    8
#define MAX_WEIGHT (1 << MAX_LEN)

int A[MAX_WEIGHT+1];

void
weight_distribution2(uint32_t poly, int *A) {
  int i;
  uint32_t x;

  /* zeroize array */
  for (i=0; i < MAX_WEIGHT+1; i++)
    A[i] = 0;

  /* loop over all input sequences */
  
  
  /* set lfsr to 1 */
  x = 1;
#if debug_print
  printf("%d:\t%s\n", 0, u32_bit_string(x,8));
#endif
  for (i=1; i < MAX_PERIOD; i++) {
    if (x & 1) 
      x = (x >> 1) ^ poly;
    else
      x = (x >> 1);

#if debug_print
    /* print for a sanity check */
    printf("%d:\t%s\n", i, u32_bit_string(x,8));
#endif
    
    /* increment weight */
    wt += (x & 1);

    /* check for return to original value */
    if (x == 1)
      break;
  }

  /* set zero */
  A[0] = 0;
}


void
weight_distribution(uint32_t poly, int *A) {
  int i;
  uint32_t x;

  /* zeroize array */
  for (i=0; i < MAX_WEIGHT+1; i++)
    A[i] = 0;

  /* set lfsr to 1 */
  x = 1;
#if debug_print
  printf("%d:\t%s\n", 0, u32_bit_string(x,8));
#endif
  for (i=1; i < MAX_PERIOD; i++) {
    if (x & 1) 
      x = (x >> 1) ^ poly;
    else
      x = (x >> 1);

#if debug_print
    /* print for a sanity check */
    printf("%d:\t%s\n", i, u32_bit_string(x,8));
#endif

    /* compute weight, increment proper element */
    A[weight(x)]++;

    /* check for return to original value */
    if (x == 1)
      break;
  }

  /* set zero */
  A[0] = 0;
}




int
main () {

  int i,j;
  v32_t x;
  v32_t p;

  /* originally 0xaf */
  p.value = 0x9;

  printf("polynomial: %s\tperiod: %d\n",  
 	   u32_bit_string(p.value,8), period(p.value)); 
 
   /* compute weight distribution */
  weight_distribution(p.value, A);
  
  /* print weight distribution */
  for (i=0; i <= 8; i++) {
    printf("A[%d]: %d\n", i, A[i]);
  }
  
#if 0
  for (i=0; i < 39; i++) {
     printf("polynomial: %s\tperiod: %d\n",  
 	   u32_bit_string(polynomials[i],8), period(polynomials[i])); 
   
     /* compute weight distribution */
     weight_distribution(p.value, A);
     
     /* print weight distribution */
     for (j=0; j <= 8; j++) {
       printf("A[%d]: %d\n", j, A[j]);
     }   
  }
#endif

  { 
    int bits = 8;
    uint32_t y;
    for (y=0; y < (1 << bits); y++) {
      printf("polynomial: %s\tweight: %d\tperiod: %d\n", 
	     u32_bit_string(y,bits), weight(y), period(y));
      
      /* compute weight distribution */
      weight_distribution(y, A);
      
      /* print weight distribution */
      for (j=0; j <= 8; j++) {
	printf("A[%d]: %d\n", j, A[j]);
      }     
    }
  }

  return 0;
}
