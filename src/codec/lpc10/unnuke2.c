#include <stdio.h>

#include "lpc10.h"

int read_bits(FILE *f, INT32 *bits, int len);
void write_bits(FILE *f, INT32 *bits, int len);

int read_16bit_samples(FILE *f, float speech[], int n);
int write_16bit_samples(FILE *f, float speech[], int n);


/*

This is just like unnuke.c, except that we read in two compressed
audio files, interleaving the reading and decoding of frames from one
file with frames from the other file.  Each file has its own
lpc10_decoder_state structure, so if things are implemented correctly,
this should work the same way as running unnuke on each file one at a
time.

*/



void
main(int argc, char *argv[])
{
    int n;
    float speech[LPC10_SAMPLES_PER_FRAME];
    INT32 bits[LPC10_BITS_IN_COMPRESSED_FRAME];
    struct lpc10_decoder_state *st1;
    struct lpc10_decoder_state *st2;
    FILE *infile1, *outfile1;
    FILE *infile2, *outfile2;
    int done_with_file_1;
    int done_with_file_2;

    if (argc != 5) {
	fprintf(stderr, "Usage: %s infile1 outfile1 infile2 outfile2\n",
		argv[0]);
	exit(1);
    }

    infile1 = fopen(argv[1], "r");
    if (infile1 == NULL) {
	fprintf(stderr, "%s: Could not open file '%s' for reading.\n",
		argv[0], argv[1]);
	exit(1);
    }
    outfile1 = fopen(argv[2], "w");
    if (outfile1 == NULL) {
	fprintf(stderr, "%s: Could not open file '%s' for writing.\n",
		argv[0], argv[2]);
	exit(1);
    }
    infile2 = fopen(argv[3], "r");
    if (infile2 == NULL) {
	fprintf(stderr, "%s: Could not open file '%s' for reading.\n",
		argv[0], argv[3]);
	exit(1);
    }
    outfile2 = fopen(argv[4], "w");
    if (outfile2 == NULL) {
	fprintf(stderr, "%s: Could not open file '%s' for writing.\n",
		argv[0], argv[4]);
	exit(1);
    }

    st1 = create_lpc10_decoder_state();
    st2 = create_lpc10_decoder_state();
    if (st1 == 0 || st2 == 0) {
	fprintf(stderr,
		"Couldn't allocate %d bytes for two sets of decoder state.\n",
		2 * sizeof(struct lpc10_decoder_state));
	exit(1);
    }
    done_with_file_1 = 0;
    done_with_file_2 = 0;
    do {
	/* Read and process one frame from file 1. */
	n = read_bits(infile1, bits, LPC10_BITS_IN_COMPRESSED_FRAME);
	if (n != LPC10_BITS_IN_COMPRESSED_FRAME) {
	    done_with_file_1 = 1;
	}
	if (!done_with_file_1) {
	    lpc10_decode(bits, speech, st1);
	    n = write_16bit_samples(outfile1, speech, LPC10_SAMPLES_PER_FRAME);
	}

	/* Now do the same thing for file 2. */
	n = read_bits(infile2, bits, LPC10_BITS_IN_COMPRESSED_FRAME);
	if (n != LPC10_BITS_IN_COMPRESSED_FRAME) {
	    done_with_file_2 = 1;
	}
	if (!done_with_file_2) {
	    lpc10_decode(bits, speech, st2);
	    n = write_16bit_samples(outfile2, speech, LPC10_SAMPLES_PER_FRAME);
	}
    } while (!(done_with_file_1 && done_with_file_2));

    exit(0);
}
