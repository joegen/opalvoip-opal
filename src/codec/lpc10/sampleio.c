#include <stdio.h>
#include <malloc.h>
#include <unistd.h>

#include "lpc10.h"



/* The return value of each of these calls is the same as that
   returned by fread/fwrite, which should be the number of samples
   successfully read/written, not the number of bytes. */

int
read_16bit_samples(FILE *f, float speech[], int n)
{
    INT16 int16samples[n];
    int samples_read;
    int i;

    samples_read = fread(int16samples, sizeof(INT16), n, f);

    /* Convert 16 bit integer samples to floating point values in the
       range [-1,+1]. */

    for (i = 0; i < samples_read; i++) {
        speech[i] = ((float) int16samples[i]) / 32768.0;
    }

    return (samples_read);
}



int
write_16bit_samples(FILE *f, float speech[], int n)
{
    INT16 int16samples[n];
    int i;
    float real_sample;
    int samples_written;

    /* Convert floating point samples in range [-1,+1] to 16 bit
       integers. */
    for (i = 0; i < n; i++) {
        real_sample = 32768.0 * speech[i];
        if (real_sample < -32768.0) {
	    int16samples[i] = -32768;
	} else if (real_sample > 32767.0) {
	    int16samples[i] = 32767;
	} else {
	    int16samples[i] = real_sample;
	}
    }

    samples_written = fwrite(int16samples, sizeof(INT16), n, f);

    return (samples_written);
}
