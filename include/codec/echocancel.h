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
 * $Log: echocancel.h,v $
 * Revision 1.3  2005/11/25 21:00:38  dsandras
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

#ifndef __OPAL_ECHOCANCEL_H
#define __OPAL_ECHOCANCEL_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif

#include <rtp/rtp.h>
#include <ptclib/qchannel.h>

class SpeexEchoState;
class SpeexPreprocessState;

///////////////////////////////////////////////////////////////////////////////
class OpalEchoCanceler : public PObject
{
  PCLASSINFO(OpalEchoCanceler, PObject);
public:
  enum Mode {
    NoCancelation,
    Cancelation
  };

  struct Params {
    Params(
	   Mode mode = NoCancelation
	   ) : m_mode (mode)
    { }

    Mode m_mode;
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
       This enables, disables the silence detector as well as adjusting its
       "agression". The deadband periods are in audio samples of 8kHz.
      */
    void SetParameters(
      const Params & newParam /// New parameters for silence detector
    );

protected:
  PDECLARE_NOTIFIER(RTP_DataFrame, OpalEchoCanceler, ReceivedPacket);
  PDECLARE_NOTIFIER(RTP_DataFrame, OpalEchoCanceler, SentPacket);

  PNotifier receiveHandler;
  PNotifier sendHandler;

  Params param;

private:

  double mean;
  PQueueChannel *echo_chan;
  SpeexEchoState *echoState;
  SpeexPreprocessState *preprocessState;
  short *ref_buf;
  short *echo_buf;
  short *e_buf;
  float *noise;
};

#endif // __OPAL_ECHOCANCEL_H

/////////////////////////////////////////////////////////////////////////////
