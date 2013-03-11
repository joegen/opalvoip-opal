/*
 * echocancel.h
 *
 * Open Phone Abstraction Library (OPAL)
 * Formally known as the Open H323 project.
 *
 * Copyright (c) 2004 Post Increment
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
 * Contributor(s): Miguel Rodriguez Perez
 *
 * $Revision$
 * $Author$
 * $Date$
 */

#ifndef OPAL_CODEC_ECHOCANCEL_H
#define OPAL_CODEC_ECHOCANCEL_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif

#include <opal/buildopts.h>

#include <rtp/rtp.h>
#include <ptclib/qchannel.h>

#ifndef SPEEX_ECHO_H
struct SpeexEchoState;
#endif

#ifndef SPEEX_PREPROCESS_H
struct SpeexPreprocessState;
#endif 

///////////////////////////////////////////////////////////////////////////////
class OpalEchoCanceler : public PObject
{
  PCLASSINFO(OpalEchoCanceler, PObject);
public:
  struct Params {
    Params()
      : m_enabled(false)
      , m_duration(2000) // 250ms at 8kHz
    { }

    bool    m_enabled;    ///< Echo cancellation will be performed
    unsigned m_duration;  ///< Number of samples of echo to cancel

  };

  /**@name Construction */
  //@{
  /**Create a new canceler.
   */
  OpalEchoCanceler();
  ~OpalEchoCanceler();
  //@}


  /**@@name Basic operations */
  //@{
    const PNotifier & GetReceiveHandler() const { return receiveHandler; }
    const PNotifier & GetSendHandler() const {return sendHandler; }

    
    /**Set the silence detector parameters.
      */
    void SetParameters(
      const Params & newParam ///> New parameters for silence detector
    );


    /**Set the clock rate for the preprocessor
     */
    void SetClockRate(
      const int clockRate     ///> Clock Rate for the preprocessor
    );

protected:
  PDECLARE_NOTIFIER(RTP_DataFrame, OpalEchoCanceler, ReceivedPacket);
  PDECLARE_NOTIFIER(RTP_DataFrame, OpalEchoCanceler, SentPacket);

  PNotifier receiveHandler;
  PNotifier sendHandler;

  Params param;

  double mean;
  int clockRate;
  PQueueChannel *echo_chan;
  PMutex stateMutex;
  SpeexEchoState *echoState;
  SpeexPreprocessState *preprocessState;

  // the following types are all void * to avoid including Speex header files
  void * ref_buf;
  void * echo_buf;
  void * e_buf;
  void * noise;
};


#endif // OPAL_CODEC_ECHOCANCEL_H


/////////////////////////////////////////////////////////////////////////////
