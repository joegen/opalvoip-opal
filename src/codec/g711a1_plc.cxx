/**
* g711a1_plc.cxx
*
* packet loss concealment algorithm is based on the ITU recommendation
* G.711 Appendix I.
*
* Copyright (c) 2008 Christian Hoene, University of Tuebingen, Germany
*
* The contents of this file are subject to the Mozilla Public License
* Version 1.0 (the "License"); you may not use this file except in
* compliance with the License. You may obtain a copy of the License at
* http://www.mozilla.org/MPL/
*
* Software distributed under the License is distributed on an "AS IS"
* basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See
* the License for the specific language governing rights and limitations
* under the License.
*
* The Initial Developer of the Original Code is Christian Hoene
* based on the code given in ITU-T Recommendation G.711 Appendix I (09/99) 
* "A high quality low-complexity algorithm for packet loss concealment with G.711"
* Approved in 1999-09, www.itu.int.
*
* Contributor(s): ______________________________________.
*
* $Revision$
* $Author$
* $Date$
*/

#ifndef SBC_DISABLE_PTLIB
#include <ptlib.h>
#include <opal/buildopts.h>
#endif

#include <codec/g711a1_plc.h>

#if OPAL_G711PLC

#include <math.h>


static const int CORR_DECIMATION_RATE=4000;     /**< decimation to a rate of [Hz] */
static const int corr_len_ms=20;       /**< Correlation length [ms] */

static const int OVERLAPADD=4;             /**< length of the overlapadd windows [ms] */
static const double CORR_MINPOWER=250./80.; /**< minimum power */

static const int CONCEAL_ATTENUATION_PERIOD1=10;      /**< first 10 ms are not faded out */
static const int CONCEAL_ATTENUATION_PERIOD2=50;      /**< Then, attenuation for [ms] */

static const int CONCEAL_PERIOD1=10;     /**< the first 10 ms uses just the first channel[c].pitch period */
static const int CONCEAL_PERIOD2=50;     /**< length of the second period uses just multiple channel[c].pitch periods */

static const int TRANSITION_START=10;    /**< after 10ms of loss, the transition period will be prolonged (ITU G.711 I.2.7) [ms] */
static const double TRANSITION_RATIO=0.4;  /**< for each 10ms of loss, the transition period will be prolonged by 4ms (ITU G.711 I.2.7) */
static const int TRANSITION_MAX=10;      /**< the transition period will have a maximal length of 10ms (ITU G.711 I.2.7 [ms] */

static const double PITCH_LOW=66.6;/**< minimum allowed channel[c].pitch. default 66 Hz [Hz] */
static const double PITCH_HIGH=200;/**< maximum allowed channel[c].pitch. default 200 Hz [Hz] */


#ifdef _MSC_VER
__inline double round(const double & value)
{
  return value < 0 ? floor(value-0.5) : ceil(value+0.5);
}
#endif


/** concealment constructor.
* This function inits all state variables and print them out for verification purposes. 
* In addition it allocated memory.
* @see g711plc_destroy
* @param lc the concealment state.  
* @param rate the sampling rate. Default is 8000.
* @param channels number of channels.
* @param pitch_max_hz the highest frequency, which is considered for determing the channel[c].pitch [Hz]. Default is 200.
* @param pitch_min_hz the lowest channel[c].pitch frequency [Hz}. Default is 66.
*/

OpalG711_PLC::OpalG711_PLC(int rate, int channels, double pitch_low, double pitch_high)
{
#ifndef SBC_DISABLE_PTLIB
  PAssert(rate >= 8000 && rate <= 48000, PInvalidParameter);
#endif
  this->rate = rate;

  this->channels = channels;

  this->channel = new channel_counters[channels];
  memset(channel, 0, channels * sizeof(channel_counters));

#ifndef SBC_DISABLE_PTLIB
  PAssert(pitch_high <= 1000 && pitch_high > pitch_low, PInvalidParameter);
#endif
  pitch_min = int(rate/pitch_high);                /* minimum allowed channel[c].pitch, default 200 Hz */

#ifndef SBC_DISABLE_PTLIB
  PAssert(1/pitch_low < corr_len_ms, PInvalidParameter);
#endif
  pitch_max = int(rate/pitch_low);                /* maximum allowed channel[c].pitch, default 66 Hz */


  pitch_overlapmax = (this->pitch_max >> 2);             /* maximum channel[c].pitch OLA window */
  hist_len = (this->pitch_max * 3 + this->pitch_overlapmax);/* history buffer length*/

  pitch_buf = new double[hist_len * channels];  /* buffer for cycles of speech */
  hist_buf = new short[hist_len * channels];  /* history buffer */
  tmp_buf = new short[pitch_overlapmax * channels];


  /* correlation related variables */

  /*
  printf("sampling rate         %dHz\n",rate);
  printf("channels              %u\n",channels);
  printf("highest allowed channel[c].pitch %lfHz\t%d\n",pitch_high,pitch_min);
  printf("lowest  allowed channel[c].pitch %lfHz\t%d\n",pitch_low,pitch_max);
  printf("correlation length    %dHz\t%d\n",corr_len_ms,ms2samples(corr_len_ms));
  */

  pitch_lastq = new double[pitch_overlapmax * channels];  /* saved last quarter wavelengh */
  conceal_overlapbuf = new short[hist_len * channels];     /* store overlapping speech segments */

  transition_buf = new short[ms2samples(TRANSITION_MAX) * channels];

  memset(hist_buf, 0, hist_len * channels);
}

/** concealment destructor.
* This function frees the memory.
* @see OpalG711_PLC::construct
* @param lc the concealment state.  
*/

OpalG711_PLC::~OpalG711_PLC()
{
  //  printf("Destroy OpalG711_PLC()\n");
  delete[] transition_buf;
  delete[] conceal_overlapbuf;
  delete[] pitch_buf;
  delete[] pitch_lastq;
  delete[] hist_buf;
  delete[] tmp_buf;
  delete[] channel;
}

/** Get samples from the circular channel[c].pitch buffer. Update channel[c].pitch_offset so
* when subsequent frames are erased the signal continues.
* @param out buffer to which the channel[c].pitch period will be copied.
* @param sz size of the buffer [samples].
*/
void OpalG711_PLC::getfespeech(short *out, int c, int sz)
{
  while (sz) {
    int cnt = channel[c].pitch_blen - channel[c].pitch_offset;
    if (cnt > sz)
      cnt = sz;
    convertfs(
	pitch_buf + channels * (hist_len - channel[c].pitch_blen + channel[c].pitch_offset),
	out,
	c,
	cnt
	);
    //    for (int i=0;i<cnt;i++)
    //      printf("$ %d %d %04X\n",sz,channel[c].pitch_offset+i,out[i]);
    channel[c].pitch_offset += cnt;
    if (channel[c].pitch_offset == channel[c].pitch_blen)
      channel[c].pitch_offset = 0;
    out += cnt * channels;
    sz -= cnt;
  }
}

/** Decay concealed signal.
* The concealed signal is slowly faded out. For a CONEAL_OFFSET period the signal is
* keep high. Then, it is slowly decayed until it become pure silence.
* @param lc concealment state.
* @param inout the buffer to decay.
* @param size the size of the buffer [samples].
* 
*/
void OpalG711_PLC::scalespeech(short *inout, int c, int size, bool decay) const
{
  /** attenuation per sample */
  double attenincr = 1./ms2samples(CONCEAL_ATTENUATION_PERIOD2);  
  double g = (double)1. - (channel[c].conceal_count-ms2samples(CONCEAL_ATTENUATION_PERIOD1)) * attenincr;

  for (int i=0; i < size; i++) {
    int index = i*channels + c;
    if(g<0) {
      inout[index]=0;
    }
    else {
      if(g<1)
	inout[index] = short(round(inout[index] * g));
      if(decay)
        g -= attenincr;
    }
  }
}

/** Generate the synthetic signal.
* @param out buffer for synthetic signal
* @param size number of samples requested
*/
void OpalG711_PLC::dofe(short *out, int size)
{
  for (int c=0; c<channels; c++) {
    short *buf = out;
    int rest = size;
    do {
      int res = dofe_partly(buf, c, rest);
#ifndef SBC_DISABLE_PTLIB
      //PAssert(res > 0 && res <=size, PInvalidParameter);
#endif
      rest -= res;
      buf += res * channels;
    }while(rest>0);
  }
  hist_savespeech(out, size);
}

/** Generate the synthetic signal until a new mode is reached.
* At the beginning of an erasure determine the channel[c].pitch, and extract
* one channel[c].pitch period from the tail of the signal. Do an OLA for 1/4
* of the channel[c].pitch to smooth the signal. Then repeat the extracted signal
* for the length of the erasure. If the erasure continues for more than
* 10 msec, increase the number of periods in the pitchbuffer. At the end
* of an erasure, do an OLA with the start of the first good frame.
* The gain decays as the erasure gets longer.
* @param out buffer for synthetic signal
* @param size number of samples requested
* @return number of samples generated
*/

int OpalG711_PLC::dofe_partly(short *out, int c, int size)
{
  //printf("dofe_partly: mode %d\tcnt %d\tsz %d\t->\t", channel[c].mode, channel[c].conceal_count, size);

  switch(channel[c].mode) {
  case NOLOSS:
  case TRANSITION:
    // first erased bytes


    convertsf(hist_buf, pitch_buf, c, hist_len); /* get history */
    channel[c].pitch = findpitch(c);                         /* find channel[c].pitch */
    channel[c].pitch_overlap = this->channel[c].pitch >> 2;           /* OLA 1/4 wavelength */
    if(channel[c].pitch_overlap > pitch_overlapmax)               /* limited algorithmic delay */
      channel[c].pitch_overlap = pitch_overlapmax;

    /* save original last channel[c].pitch_overlap samples:
     * copy current channel
     *   from	pitch_buf[hist_len - channel[c].pitch_overlap]
     *   to	pitch_lastq[0]
     *   len	channel[c].pitch_overlap */
    for (int i = 0; i < channel[c].pitch_overlap; i++)
      pitch_lastq[channels * i + c] =
	pitch_buf[channels * (hist_len - channel[c].pitch_overlap + i) + c];

    channel[c].pitch_offset = 0;                                         /* create channel[c].pitch buffer with 1 period */
    channel[c].pitch_blen = channel[c].pitch;
    overlapadd(
	pitch_lastq, 
	pitch_buf + channels * (hist_len - channel[c].pitch_blen - channel[c].pitch_overlap), 
	pitch_buf + channels * (hist_len - channel[c].pitch_overlap), 
	c,
	channel[c].pitch_overlap
	);

    /* update last 1/4 wavelength in hist_buf buffer */
    convertfs(
	pitch_buf + channels * (hist_len - channel[c].pitch_overlap),
	hist_buf  + channels * (hist_len - channel[c].pitch_overlap),
	c,
	channel[c].pitch_overlap
	);

    channel[c].conceal_count = 0;
    channel[c].mode = LOSS_PERIOD1;
    // fall thru

  case LOSS_PERIOD1:
    /* get synthesized speech */
    if(size+channel[c].conceal_count >= ms2samples(CONCEAL_PERIOD1)) {
      size = ms2samples(CONCEAL_PERIOD1)-channel[c].conceal_count;
      channel[c].mode = LOSS_PERIOD2start;
    }
    getfespeech(out, c, size);    
    break;

    // start of second period
  case LOSS_PERIOD2start: {    
    /* tail of previous channel[c].pitch estimate */
    int saveoffset = channel[c].pitch_offset;                    /* save offset for OLA */
    getfespeech(tmp_buf, c, channel[c].pitch_overlap);                  /* continue with old pitch_buf */

    /* add periods to the channel[c].pitch buffer */
    channel[c].pitch_offset = saveoffset % channel[c].pitch; // why??
    channel[c].pitch_blen += channel[c].pitch;                      /* add a period */
    overlapadd(
	pitch_lastq, 
	pitch_buf + channels * (hist_len - channel[c].pitch_blen - channel[c].pitch_overlap),
	pitch_buf + channels * (hist_len - channel[c].pitch_overlap), 
	c,
	channel[c].pitch_overlap
	);

    /* overlap add old pitchbuffer with new */
    getfespeech(conceal_overlapbuf, c, channel[c].pitch_overlap);
    overlapadds(tmp_buf, conceal_overlapbuf, conceal_overlapbuf, c, channel[c].pitch_overlap);
    scalespeech(conceal_overlapbuf, c, channel[c].pitch_overlap);
    channel[c].mode = LOSS_PERIOD2overlap;

    // fall thru
                          }
                          // still overlapping period at the beginning?
  case LOSS_PERIOD2overlap:
    if(channel[c].conceal_count + size >= ms2samples(CONCEAL_PERIOD1)+channel[c].pitch_overlap) {
      size = ms2samples(CONCEAL_PERIOD1)+channel[c].pitch_overlap-channel[c].conceal_count;
      channel[c].mode=LOSS_PERIOD2;
    }
    /* copy overlapping samples to output:
     * copy current channel
     *   from	conceal_overlapbuf[channel[c].conceal_count - ms2samples(CONCEAL_PERIOD1)]
     *   to	out[0]
     *   len	channel[c].pitch_overlap */
    for (int i = 0; i < size; i++)
      out[channels * i + c] =
	conceal_overlapbuf[channels * (channel[c].conceal_count - ms2samples(CONCEAL_PERIOD1) + i) + c];
    break;

    // no overlapping period
  case LOSS_PERIOD2:
    if(size + channel[c].conceal_count >= ms2samples(CONCEAL_PERIOD1+CONCEAL_PERIOD2)) {
      size = ms2samples(CONCEAL_PERIOD1+CONCEAL_PERIOD2)-channel[c].conceal_count;
      channel[c].mode = LOSS_PERIOD3;
    }
    getfespeech(out, c, size);
    scalespeech(out, c, size);
    break;

    // erased bytes after the second period
  case LOSS_PERIOD3:
    // zero out
    for (int i=c; i < size * channels; i += channels)
      out[i] = 0;
    break;

  default:
#ifndef SBC_DISABLE_PTLIB
    PAssertAlways(PLogicError)
#endif
    ;
  }

  channel[c].conceal_count+=size;
  //printf("mode %d\tsz %d\n", channel[c].mode, size);
  return size;
}

/** Saves samples in the history buffer.
* Save a frames worth of new speech in the history buffer.
* Return the output speech delayed by this->pitch_overlapmax
* @param inout buffer which contains the input speech and to which the delay speech is written
* @param size size of this buffer [samples]
*/
void OpalG711_PLC::hist_savespeech(short *inout, int size)
{
  if(size < hist_len-pitch_overlapmax) {
    //    printf(".");
    /* make room for new signal */
    copy(hist_buf + channels*size, hist_buf, hist_len - size);
    /* copy in the new frame */
    copy(inout, hist_buf + channels * (hist_len - size), size);
    /* copy history signal to inout buffer*/
    copy(hist_buf + channels * (hist_len - size - pitch_overlapmax), inout, size);
  }
  else if(size <= hist_len) {
    //    printf(";");
    /* store old signal tail */
    copy(hist_buf + channels * (hist_len - pitch_overlapmax), tmp_buf, pitch_overlapmax);
    /* make room for new signal */
    copy(hist_buf + channels * size, hist_buf, hist_len - size);
    /* copy in the new frame */
    copy(inout, hist_buf + channels * (hist_len - size), size);
    /* move data to delay frame */
    copy(inout, inout + channels * pitch_overlapmax, size - pitch_overlapmax);
    /* copy history signal */
    copy(tmp_buf, inout, pitch_overlapmax);
  }
  else {
    //    printf(": %d %d %d\n",size,hist_len,pitch_overlapmax);
    /* store old signal tail */
    copy(hist_buf + channels * (hist_len - pitch_overlapmax), tmp_buf, pitch_overlapmax);
    /* copy in the new frame */
    copy(inout + channels * (size - hist_len), hist_buf, hist_len);
    /* move data to delay frame */
    copy(inout, inout + channels * pitch_overlapmax, size - pitch_overlapmax);
    /* copy history signal */
    copy(tmp_buf, inout, pitch_overlapmax);
  }
}

/** receiving good samples.
* A good frame was received and decoded.
* If right after an erasure, do an overlap add with the synthetic signal.
* Add the frame to history buffer.
* @param s pointer to samples
* @param size number of samples
*/
void OpalG711_PLC::addtohistory(short *s, int size)
{
  for (int c=0; c < channels; c++) {
    switch(channel[c].mode) {
      case NOLOSS:
	break;
      case LOSS_PERIOD1:
      case LOSS_PERIOD2:
      case LOSS_PERIOD2start:
      case LOSS_PERIOD2overlap:
      case LOSS_PERIOD3:
	channel[c].mode = TRANSITION;
	/** I.2.7   First good frame after an erasure.
	  At the first good frame after an erasure, a smooth transition is needed between the synthesized
	  erasure speech and the real signal. To do this, the synthesized speech from the channel[c].pitch buffer is
	  continued beyond the end of the erasure, and then mixed with the real signal using an OLA. The
	  length of the OLA depends on both the channel[c].pitch period and the length of the erasure. For short, 10 ms
	  erasures, a 1/4 wavelength window is used. For longer erasures the window is increased by 4 ms per
	  10 ms of erasure, up to a maximum of the frame size, 10 ms.
	  */
	channel[c].transition_len = channel[c].pitch_overlap;
	if(channel[c].conceal_count > ms2samples(TRANSITION_START))
	  channel[c].transition_len += int(round((channel[c].conceal_count - ms2samples(TRANSITION_START))*TRANSITION_RATIO));
	if(channel[c].transition_len > ms2samples(TRANSITION_MAX))
	  channel[c].transition_len = ms2samples(TRANSITION_MAX);
	getfespeech(transition_buf, c, channel[c].transition_len);
	scalespeech(transition_buf, c, channel[c].transition_len,false);
	channel[c].transition_count=0;

      case TRANSITION:
	int end = channel[c].transition_count+size;
	if(end >= channel[c].transition_len) {
	  end = channel[c].transition_len;
	  channel[c].mode = NOLOSS;
	}
	//    printf("addtohistory: s %d\tlen %d\te %d\tcnt %d\tadel %d\n",channel[c].transition_count,channel[c].transition_len, end, channel[c].conceal_count, getAlgDelay());
	overlapaddatend(s, transition_buf + channels * channel[c].transition_count, c, channel[c].transition_count, end, channel[c].transition_len);
	channel[c].transition_count = end;
    }
  }
  hist_savespeech(s, size);
}

/** the previous frame is dropped intentionally to speed up the play out
* @param s pointer to samples
* @param size number of samples
*/
void OpalG711_PLC::drop(short *s, int size)
{
  dofe(transition_buf,ms2samples(TRANSITION_MAX));
  for (int c=0; c < channels; c++) {
    channel[c].transition_len = channel[c].pitch_overlap;
    channel[c].transition_count = 0;
    channel[c].mode = TRANSITION;
  }
  addtohistory(s, size);
}

/** 
* Overlapp add the end of the erasure with the start of the first good frame.
* @param s pointer to good signal
* @param f pointer to concealed signal
* @param start first sample to consider
* @param end last sample to consider (not including)
* @param count length of decay period [samples]
*/
void OpalG711_PLC::overlapaddatend(short *s, short *f, int c, int start, int end, int count) const
{
#ifndef SBC_DISABLE_PTLIB
  PAssert(start<=end, PInvalidParameter);
  PAssert(end<=count, PInvalidParameter);
  PAssert(start >= 0 && count < 32767, PInvalidParameter);
#endif

  int size=end-start;
  start++; /* XXX ?? */
  for (int i=0; i < size; i++) {
    int index = i*channels + c;
    int t = ((count-start) * f[index] + start * s[index]) / count;
    //    printf("%d %d %d %d -> %d\n",count-start,f[index],start,s[index], t);
    //    printf("%04X %04X -> %08X (%d %d)\n",f[index]&0xffff,s[index]&0xffff,t,start,count);
    if (t > 32767)
      t = 32767;
    else if (t < -32768)
      t = -32768;
    s[index] = (short)t;
#ifndef SBC_DISABLE_PTLIB
    PAssert(end >=0 && end<=count && start >=0 && start <= count, PInvalidParameter);
#endif
    start++;
  }
}

/** Overlapp add left and right sides.
* @param l   input buffer (gets silent)
* @param r   input buffer (gets loud)
* @param o   output buffer
* @param cnt size of buffers
*/
void OpalG711_PLC::overlapadd(double *l, double *r, double *o, int c, int cnt) const
{
  double  incr, lw, rw, t;

  if (cnt == 0)
    return;
  incr = (double)1. / cnt;
  lw = (double)1. - incr;
  rw = incr;
  for (int i=0; i < cnt; i++) {
    int index = i*channels + c;
    t = lw * l[index] + rw * r[index];
    if (t > (double)32767.)
      t = (double)32767.;
    else if (t < (double)-32768.)
      t = (double)-32768.;
    o[index] = t;
    lw -= incr;
    rw += incr;
  }
}

/** Overlapp add left and right sides.
* @param l  input buffer (gets silent).
* @param r  input buffer (gets lough).
* @param o output buffer.
* @param cnt size of buffers.
*/
void OpalG711_PLC::overlapadds(short *l, short *r, short *o, int c, int cnt) const
{
  double  incr, lw, rw, t;

  if (cnt == 0)
    return;
  incr = (double)1. / cnt;
  lw = (double)1. - incr;
  rw = incr;
  for (int i=0; i < cnt; i++) {
    int index = i*channels + c;
    t = lw * l[index] + rw * r[index];
    if (t > (double)32767.)
      t = (double)32767.;
    else if (t < (double)-32768.)
      t = (double)-32768.;
    o[index] = (short)t;
    lw -= incr;
    rw += incr;
  }
}

/** Estimate the channel[c].pitch.
*/
int OpalG711_PLC::findpitch(int c)
{
  /** decimation for correlation. default is 2:1 at 8000 Hz */
  int corr_ndec = rate/CORR_DECIMATION_RATE;   
  /** minimum power */
  double corr_minpower = CORR_MINPOWER * ms2samples(corr_len_ms) / corr_ndec;

  int  i, j, k;
  int  bestmatch;
  double  bestcorr;
  double  corr;    /**< correlation */
  double  energy;    /**< running energy */
  double  scale;    /**< scale correlation by average power */
  double  *rp;    /**< segment to match */
  /** l - pointer to first sample in last 20 msec of speech. */
  double  *l = pitch_buf + channels * (hist_len - ms2samples(corr_len_ms));
  /** r - points to the sample pitch_max before l. */
  double  *r = pitch_buf + channels * (hist_len - ms2samples(corr_len_ms) - pitch_max);

  /* coarse search */
  rp = r;
  energy = (double)0.;
  corr = (double)0.;
  for (i = 0; i < ms2samples(corr_len_ms); i += corr_ndec) {
    int index = i*channels + c;
    energy += rp[index] * rp[index];
    corr += rp[index] * l[index];
  }
  scale = energy;
  if (scale < corr_minpower)
    scale = corr_minpower;
  corr = corr / (double)sqrt(scale);
  bestcorr = corr;
  bestmatch = 0;
  for (j = corr_ndec; j <= (pitch_max-pitch_min); j += corr_ndec) {
    int index = channels * ms2samples(corr_len_ms) + c;
    energy -= rp[0] * rp[0];
    energy += rp[index] * rp[index];
    rp += corr_ndec * channels;
    corr = 0.f;
    for (i = 0; i < ms2samples(corr_len_ms); i += corr_ndec) {
      int index = channels * i + c;
      corr += rp[index] * l[index];
    }
    scale = energy;
    if (scale < corr_minpower)
      scale = corr_minpower;
    corr /= (double)sqrt(scale);
    if (corr >= bestcorr) {
      bestcorr = corr;
      bestmatch = j;
    }
  }

  /* fine search */
  j = bestmatch - (corr_ndec - 1);
  if (j < 0)
    j = 0;
  k = bestmatch + (corr_ndec - 1);
  if (k > (pitch_max-pitch_min))
    k = (pitch_max-pitch_min);
  rp = &r[channels * j];
  energy = 0.f;
  corr = 0.f;
  for (i = 0; i < ms2samples(corr_len_ms); i++) {
    int index = i*channels + c;
    energy += rp[index] * rp[index];
    corr += rp[index] * l[index];
  }
  scale = energy;
  if (scale < corr_minpower)
    scale = corr_minpower;
  corr = corr / (double)sqrt(scale);
  bestcorr = corr;
  bestmatch = j;
  for (j++; j <= k; j++) {
    int index = channels * ms2samples(corr_len_ms) + c;
    energy -= rp[0] * rp[0];
    energy += rp[index] * rp[index];
    rp += channels;
    corr = 0.f;
    for (i = 0; i < ms2samples(corr_len_ms); i++) {
      int index = i*channels + c;
      corr += rp[index] * l[index];
    }
    scale = energy;
    if (scale < corr_minpower)
      scale = corr_minpower;
    corr = corr / (double)sqrt(scale);
    if (corr > bestcorr) {
      bestcorr = corr;
      bestmatch = j;
    }
  }
  return pitch_max - bestmatch;
}

/** converts short into double values.
* @param f input buffer of type short.
* @param t output buffer of type double.
* @param cnt size of buffers.
*/
void OpalG711_PLC::convertsf(short *f, double *t, int c, int cnt) const
{
  for (int i=c; i < channels*cnt; i += channels)
    t[i] = (double)f[i];
}

/** converts double into short values.
* @param f input buffer of type double.
* @param t output buffer of type short.
* @param cnt size of buffers.
*/
void OpalG711_PLC::convertfs(double *f, short *t, int c, int cnt) const
{
  for (int i=c; i < channels*cnt; i += channels)
    t[i] = (short)f[i];
}

#endif // OPAL_G711PLC
