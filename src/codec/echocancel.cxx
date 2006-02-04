/*
 * echocancel.cxx
 *
 * Open Phone Abstraction Library (OPAL)
 * Formally known as the Open H323 project.
 *
 * Copyright (c) 2001 Post Increment
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
 * The Original Code is Open Phone Abstraction Library.
 *
 * The author of this code is Damien Sandras
 *
 * Contributor(s): Miguel Rodriguez Perez.
 *
 * $Log: echocancel.cxx,v $
 * Revision 1.18  2006/02/04 14:27:18  dsandras
 * Fixed leak.
 *
 * Revision 1.17  2006/02/01 09:00:46  csoutheren
 * Changes to remove dependencies in Speex code accidentally introduced
 *
 * Revision 1.16  2006/01/31 10:28:04  csoutheren
 * Added detection for variants to speex 1.11.11.1
 *
 * Revision 1.15  2006/01/31 03:28:03  csoutheren
 * Changed to compile on MSVC 6
 *
 * Revision 1.14  2006/01/23 23:01:19  dsandras
 * Protect internal speex state changes with a mutex.
 *
 * Revision 1.13  2006/01/21 23:33:36  csoutheren
 * Fixed error again :)
 *
 * Revision 1.12  2006/01/21 23:27:06  csoutheren
 * Fixed error under MSVC 6
 *
 * Revision 1.11  2006/01/07 17:37:50  dsandras
 * Updated to speex 1.1.11.2 to fix divergeance issues.
 *
 * Revision 1.10  2005/12/31 09:18:44  dsandras
 * Some fine-tuning.
 *
 * Revision 1.9  2005/12/30 17:13:34  dsandras
 * Fixed typo.
 *
 * Revision 1.8  2005/12/30 14:33:47  dsandras
 * Denoise the signal even when there is no echo to remove in it.
 *
 * Revision 1.7  2005/12/29 16:20:53  dsandras
 * Added wideband support to the echo canceller.
 *
 * Revision 1.6  2005/12/28 20:03:36  dsandras
 * Do not cancel echo when the frame is silent.
 *
 * Revision 1.5  2005/12/25 11:49:01  dsandras
 * Return if Read is unsuccessful.
 *
 * Revision 1.4  2005/11/27 20:48:05  dsandras
 * Changed ReadTimeout, WriteTimeout in case the remote is not sending data. Fixed compilation warning.
 *
 * Revision 1.3  2005/11/25 21:00:37  dsandras
 * Remove the DC or the algorithm is puzzled. Added several post-processing filters. Added missing declaration.
 *
 * Revision 1.2  2005/11/24 20:34:44  dsandras
 * Modified copyright.
 *
 * Revision 1.1  2005/11/24 20:31:54  dsandras
 * Added support for echo cancelation using Speex.
 * Added possibility to add a filter to an OpalMediaPatch for all patches of a connection.
 *
 */

#include <ptlib.h>

#ifdef __GNUC__
#pragma implementation "echocancel.h"
#endif

#include <opal/buildopts.h>

extern "C" {
#if OPAL_SYSTEM_SPEEX
#if OPAL_HAVE_SPEEX_SPEEX_H
#include <speex/speex_echo.h>
#include <speex/speex_preprocess.h>
#else
#include <speex_echo.h>
#include <speex_preprocess.h>
#endif
#else
#include "../src/codec/speex/libspeex/speex_echo.h"
#include "../src/codec/speex/libspeex/speex_preprocess.h"
#endif
};

#include <codec/echocancel.h>

///////////////////////////////////////////////////////////////////////////////

OpalEchoCanceler::OpalEchoCanceler()
#ifdef _MSC_VER
#pragma warning(disable:4355)
#endif
  : receiveHandler(PCREATE_NOTIFIER(ReceivedPacket)),
    sendHandler(PCREATE_NOTIFIER(SentPacket))
#ifdef _MSC_VER
#pragma warning(default:4355)
#endif
{
  echoState = NULL;
  preprocessState = NULL;

  e_buf = NULL;
  echo_buf = NULL;
  ref_buf = NULL;
  noise = NULL;

  echo_chan = new PQueueChannel();
  echo_chan->Open(10000);
  echo_chan->SetReadTimeout(10);
  echo_chan->SetWriteTimeout(10);

  mean = 0;
  clockRate = 8000;

  PTRACE(3, "Echo Canceler\tHandler created");
}


OpalEchoCanceler::~OpalEchoCanceler()
{
  PWaitAndSignal m(stateMutex);
  if (echoState) {
    speex_echo_state_destroy(echoState);
    echoState = NULL;
  }
  
  if (preprocessState) {
    speex_preprocess_state_destroy(preprocessState);
    preprocessState = NULL;
  }

  if (ref_buf)
    free(ref_buf);
  if (e_buf)
    free(e_buf);
  if (echo_buf)
    free(echo_buf);
  if (noise)
    free(noise);
  
  echo_chan->Close();
  delete(echo_chan);
}


void OpalEchoCanceler::SetParameters(const Params& newParam)
{
  PWaitAndSignal m(stateMutex);
  param = newParam;

  if (echoState) {
    speex_echo_state_destroy(echoState);
    echoState = NULL;
  }
  
  if (preprocessState) {
    speex_preprocess_state_destroy(preprocessState);
    preprocessState = NULL;
  }
}


void OpalEchoCanceler::SetClockRate(const int rate)
{
  clockRate = rate;
}


void OpalEchoCanceler::SentPacket(RTP_DataFrame& echo_frame, INT)
{
  if (echo_frame.GetPayloadSize() == 0)
    return;

  if (param.m_mode == NoCancelation)
    return;

  /* Write to the soundcard, and write the frame to the PQueueChannel */
  echo_chan->Write(echo_frame.GetPayloadPtr(), echo_frame.GetPayloadSize());
}


void OpalEchoCanceler::ReceivedPacket(RTP_DataFrame& input_frame, INT)
{
  int inputSize = 0;
  int i = 1;
  
  if (input_frame.GetPayloadSize() == 0)
    return;
  
  if (param.m_mode == NoCancelation)
    return;

  inputSize = input_frame.GetPayloadSize(); // Size is in bytes

  PWaitAndSignal m(stateMutex);

  if (echoState == NULL) 
    echoState = speex_echo_state_init(inputSize/sizeof(short), 32*inputSize);
  
  if (preprocessState == NULL) { 
    preprocessState = speex_preprocess_state_init(inputSize/sizeof(short), clockRate);
    speex_preprocess_ctl(preprocessState, SPEEX_PREPROCESS_SET_DENOISE, &i);
  }

  if (echo_buf == NULL)
    echo_buf = (spx_int16_t *) malloc(inputSize);
  if (noise == NULL)
#if OPAL_SPEEX_FLOAT_NOISE
    noise = malloc((inputSize/sizeof(short)+1)*sizeof(float));
#else
    noise = malloc((inputSize/sizeof(short)+1)*sizeof(spx_int32_t));
#endif
  if (e_buf == NULL)
    e_buf = (spx_int16_t *) malloc(inputSize);
  if (ref_buf == NULL)
    ref_buf = (spx_int16_t *) malloc(inputSize);

  /* Remove the DC offset */
  short *j = (short *) input_frame.GetPayloadPtr();
  for (i = 0 ; i < (int) (inputSize/sizeof(short)) ; i++) {
    mean = 0.999*mean + 0.001*j[i];
    ((spx_int16_t *)ref_buf)[i] = j[i] - (short) mean;
  }
  
  /* Read from the PQueueChannel a reference echo frame of the size
   * of the captured frame. */
  if (!echo_chan->Read((short *) echo_buf, input_frame.GetPayloadSize())) {
    
    /* Nothing to read from the speaker signal, only suppress the noise
     * and return.
     */
    speex_preprocess(preprocessState, (spx_int16_t *)ref_buf, NULL);
    memcpy(input_frame.GetPayloadPtr(), (spx_int16_t *)ref_buf, input_frame.GetPayloadSize());

    return;
  }
   
  /* Cancel the echo in this frame */
#if OPAL_SPEEX_FLOAT_NOISE
  speex_echo_cancel(echoState, (short *)ref_buf, (short *)echo_buf, (short *)e_buf, (float *)noise);
#else
  speex_echo_cancel(echoState, (short *)ref_buf, (short *)echo_buf, (short *)e_buf, (spx_int32_t *)noise);
#endif
  
  /* Suppress the noise */
#if OPAL_SPEEX_FLOAT_NOISE
  speex_preprocess(preprocessState, (spx_int16_t *)e_buf, (float *)noise);
#else
  speex_preprocess(preprocessState, (spx_int16_t *)e_buf, (spx_int32_t *)noise);
#endif

  /* Use the result of the echo cancelation as capture frame */
  memcpy(input_frame.GetPayloadPtr(), e_buf, input_frame.GetPayloadSize());
}
