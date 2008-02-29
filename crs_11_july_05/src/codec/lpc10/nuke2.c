#include <stdio.h>

#include "lpc10.h"

int read_bits(FILE *f, INT32 *bits, int len);
void write_bits(FILE *f, INT32 *bits, int len);

int read_16bit_samples(FILE *f, float speech[], int n);
int write_16bit_samples(FILE *f, float speech[], int n);


/*

This is just like nuke.c, except that we read in two audio files,
interleaving the reading and encoding of frames from one file with
frames from the other file.  Each file has its own lpc10_encoder_state
structure, so if things are implemented correctly, this should work
the same way as running nuke on each file one at a time.

*/


void
main(int argc, char *argv[])
{
    int n;
    float speech[LPC10_SAMPLES_PER_FRAME];
    INT32 bits[LPC10_BITS_IN_COMPRESSED_FRAME];
    struct lpc10_encoder_state *st1;
    struct lpc10_encoder_state *st2;
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

    st1 = create_lpc10_encoder_state();
    st2 = create_lpc10_encoder_state();
    if (st1 == 0 || st2 == 0) {
	fprintf(stderr,
		"Couldn't allocate %d bytes for two sets of encoder state.\n",
		2 * sizeof(struct lpc10_encoder_state));
	exit(1);
    }
    done_with_file_1 = 0;
    done_with_file_2 = 0;
    do {
	/* Read and process one frame from file 1. */
	n = read_16bit_samples(infile1, speech, LPC10_SAMPLES_PER_FRAME);
	if (n != LPC10_SAMPLES_PER_FRAME) {
	    done_with_file_1 = 1;
	}
	if (!done_with_file_1) {
	    lpc10_encode(speech, bits, st1);
	    write_bits(outfile1, bits, LPC10_BITS_IN_COMPRESSED_FRAME);
	}

	/* Now do the same thing for file 2. */
	n = read_16bit_samples(infile2, speech, LPC10_SAMPLES_PER_FRAME);
	if (n != LPC10_SAMPLES_PER_FRAME) {
	    done_with_file_2 = 1;
	}
	if (!done_with_file_2) {
	    lpc10_encode(speech, bits, st2);
	    write_bits(outfile2, bits, LPC10_BITS_IN_COMPRESSED_FRAME);
	}
    } while (!(done_with_file_1 && done_with_file_2));

    exit(0);
}
