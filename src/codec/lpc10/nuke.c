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
    struct lpc10_encoder_state *st;
    FILE *infile, *outfile;
    int int32_size, int16_size;


    /* Feel free to change this code so that it changes the input and
       output files based on command line arguments.  I feel too lazy
       to do it right now.  Input is standard input, output is
       standard output! */

    infile = stdin;
    outfile = stdout;

    /* When I compiled this on a Sparc-10, it printed out 9540
       bytes. */
    fprintf(stderr, "sizeof(struct lpc10_encoder_state) = %d bytes\n",
	    sizeof(struct lpc10_encoder_state));
    int32_size = sizeof(INT32);
    int16_size = sizeof(INT16);
    fprintf(stderr, "sizeof(INT32) = %d   sizeof(INT16) = %d\n", int32_size,
	    int16_size);
    if ((int32_size != 4) || (int16_size != 2)) {
	fprintf(stderr, "The LPC-10 coder expects sizeof(INT32) = 4   sizeof(INT16) = 2.\n");
	fprintf(stderr, "Change \"typedef ??? INT16;\" and/or \"typedef ??? INT32;\" lines in lpc10.h to make this true for your compiler, and recompile the LPC-10 library and nuke, in that order.\n");
	exit(1);
    }

    st = create_lpc10_encoder_state();
    if (st == 0) {
	fprintf(stderr, "Couldn't allocate %d bytes for encoder state.\n",
		sizeof(struct lpc10_encoder_state));
	exit(1);
    }
    while (1) {
	n = read_16bit_samples(infile, speech, LPC10_SAMPLES_PER_FRAME);
	if (n != LPC10_SAMPLES_PER_FRAME) {
	    break;
	}
	lpc10_encode(speech, bits, st);
	write_bits(outfile, bits, LPC10_BITS_IN_COMPRESSED_FRAME);
    }

    exit(0);
}
