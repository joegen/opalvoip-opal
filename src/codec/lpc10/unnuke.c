#include <stdio.h>

#include "lpc10.h"

int read_bits(FILE *f, INT32 *bits, int len);
void write_bits(FILE *f, INT32 *bits, int len);

int read_16bit_samples(FILE *f, float speech[], int n);
int write_16bit_samples(FILE *f, float speech[], int n);



void
main(int argc, char *argv[])
{
    int n;
    float speech[LPC10_SAMPLES_PER_FRAME];
    INT32 bits[LPC10_BITS_IN_COMPRESSED_FRAME];
    struct lpc10_decoder_state *st;
    FILE *infile, *outfile;


    /* Feel free to change this code so that it changes the input and
       output files based on command line arguments.  I feel too lazy
       to do it right now.  Input is standard input, output is
       standard output! */

    infile = stdin;
    outfile = stdout;

    /* When I compiled this on a Sparc-10, it printed out 3072 bytes
       (exactly 3 KB).  */
    fprintf(stderr, "sizeof(struct lpc10_decoder_state) = %d\n",
	    sizeof(struct lpc10_decoder_state));

    st = create_lpc10_decoder_state();
    if (st == 0) {
	fprintf(stderr, "Couldn't allocate %d bytes for decoder state.\n",
		sizeof(struct lpc10_decoder_state));
	exit(1);
    }
    while (1) {
	n = read_bits(infile, bits, LPC10_BITS_IN_COMPRESSED_FRAME);
	if (n != LPC10_BITS_IN_COMPRESSED_FRAME) {
	    break;
	}
	lpc10_decode(bits, speech, st);
	n = write_16bit_samples(outfile, speech, LPC10_SAMPLES_PER_FRAME);
    }

    exit(0);
}
