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
 * The Initial Developer of the Original Code is Equivalence Pty. Ltd.
 *
 * Contributor(s): Miguel Rodriguez Perez.
 *
 * $Log: echocancel.cxx,v $
 * Revision 1.1  2005/11/24 20:31:54  dsandras
 * Added support for echo cancelation using Speex.
 * Added possibility to add a filter to an OpalMediaPatch for all patches of a connection.
 *
 */

#include <ptlib.h>

#ifdef __GNUC__
#pragma implementation "echocancel.h"
#endif

#include <codec/echocancel.h>

extern "C" {
#if OPAL_SYSTEM_SPEEX
#include <speex_echo.h>
#include <speex_preprocess.h>
#else
#include "../src/codec/speex/libspeex/speex_echo.h"
#include "../src/codec/speex/libspeex/speex_preprocess.h"
#endif
};

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

  echo_chan = new PQueueChannel();
  echo_chan->Open(10000);
  echo_chan->SetReadTimeout(2000);
  echo_chan->SetWriteTimeout(2000);
  
  PTRACE(3, "Echo Canceler\tHandler created");
}


OpalEchoCanceler::~OpalEchoCanceler()
{
  if (echoState)
    speex_echo_state_destroy(echoState);
  if (preprocessState)
    speex_preprocess_state_destroy(preprocessState);

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


void OpalEchoCanceler::SentPacket(RTP_DataFrame& echo_frame, INT)
{
  if (param.m_mode == NoCancelation)
    return;

  /* Write to the soundcard, and write the frame to the PQueueChannel */
  echo_chan->Write(echo_frame.GetPayloadPtr(), echo_frame.GetPayloadSize());
}


void OpalEchoCanceler::ReceivedPacket(RTP_DataFrame& input_frame, INT)
{
  int inputSize = 0;
  
  if (param.m_mode == NoCancelation)
    return;

  inputSize = input_frame.GetPayloadSize(); // Size is in bytes

  if (echoState == NULL) 
    echoState = speex_echo_state_init(inputSize/sizeof(short), 4*inputSize);
  
  if (preprocessState == NULL) 
    preprocessState = speex_preprocess_state_init(inputSize/sizeof(short), 8000);

  if (echo_buf == NULL)
    echo_buf = (short *) malloc(inputSize);
  if (noise == NULL)
    noise = (float *) malloc((inputSize/sizeof(short)+1)*sizeof(float));
  if (e_buf == NULL)
    e_buf = (short *) malloc(inputSize);
  
  /* Read from the PQueueChannel a reference echo frame of the size
   * of the captured frame. */
  echo_chan->Read(echo_buf, input_frame.GetPayloadSize());

  /* Cancel the echo in this frame */
  speex_echo_cancel(echoState, (short *) input_frame.GetPayloadPtr(), echo_buf, e_buf, noise);

  /* Suppress the noise */
  speex_preprocess(preprocessState, e_buf, noise);

  /* Use the result of the echo cancelation as capture frame */
  memcpy(input_frame.GetPayloadPtr(), e_buf, input_frame.GetPayloadSize());
}
