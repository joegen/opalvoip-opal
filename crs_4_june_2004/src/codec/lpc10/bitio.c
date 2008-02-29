#include <stdio.h>
#include "lpc10.h"


/*

Write the bits in bits[0] through bits[len-1] to file f, in "packed"
format.

bits is expected to be an array of len integer values, where each
integer is 0 to represent a 0 bit, and any other value represents a 1
bit.  This bit string is written to the file f in the form of several
8 bit characters.  If len is not a multiple of 8, then the last
character is padded with 0 bits -- the padding is in the least
significant bits of the last byte.  The 8 bit characters are "filled"
in order from most significant bit to least significant.

*/

void
write_bits(FILE *f, INT32 *bits, int len)
{
    int             i;		/* generic loop variable */
    unsigned char   mask;	/* The next bit position within the
				   variable "data" to place the next
				   bit. */
    unsigned char   data;	/* The contents of the next byte to
				   place in the output. */

    /* Fill in the array bits.
     * The first compressed output bit will be the most significant
     * bit of the byte, so initialize mask to 0x80.  The next byte of
     * compressed data is initially 0, and the desired bits will be
     * turned on below.
     */
    mask = 0x80;
    data = 0;

    for (i = 0; i < len; i++) {
	/* Turn on the next bit of output data, if necessary. */
	if (bits[i]) {
	    data |= mask;
	}
	/*
	 * If the byte data is full, determined by mask becoming 0,
	 * then write the byte to the output file, and reinitialize
	 * data and mask for the next output byte.  Also add the byte
	 * if (i == len-1), because if len is not a multiple of 8,
	 * then mask won't yet be 0.  */
	mask >>= 1;
	if ((mask == 0) || (i == len-1)) {
	    putc(data, f);
	    data = 0;
	    mask = 0x80;
	}
    }
}



/*

Read bits from file f into bits[0] through bits[len-1], in "packed"
format.

Read ceiling(len/8) characters from file f, if that many are available
to read, otherwise read to the end of the file.  The first character's
8 bits, in order from MSB to LSB, are used to fill bits[0] through
bits[7].  The second character's bits are used to fill bits[8] through
bits[15], and so on.  If ceiling(len/8) characters are available to
read, and len is not a multiple of 8, then some of the least
significant bits of the last character read are completely ignored.
Every entry of bits[] that is modified is changed to either a 0 or a
1.

The number of bits successfully read is returned, and is always in the
range 0 to len, inclusive.  If it is less than len, it will always be
a multiple of 8.

*/

int
read_bits(FILE *f, INT32 *bits, int len)
{
    int             i;		/* generic loop variable */
    int             c;

    /* Unpack the array bits into coded_frame. */
    for (i = 0; i < len; i++) {
	if (i % 8 == 0) {
	    c = getc(f);
	    if (c == EOF) {
		return (i);
	    }
	}
	if (c & (0x80 >> (i & 7))) {
	    bits[i] = 1;
	} else {
	    bits[i] = 0;
	}
    }
    return (len);
}
