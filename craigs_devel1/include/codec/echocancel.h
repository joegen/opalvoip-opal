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
 * Revision 1.11  2006/02/01 09:00:46  csoutheren
 * Changes to remove dependencies in Speex code accidentally introduced
 *
 * Revision 1.10  2006/01/31 10:28:03  csoutheren
 * Added detection for variants to speex 1.11.11.1
 *
 * Revision 1.9  2006/01/31 08:32:34  csoutheren
 * Fixed problem with speex includes. Again
 *
 * Revision 1.8  2006/01/31 03:28:03  csoutheren
 * Changed to compile on MSVC 6
 *
 * Revision 1.7  2006/01/23 23:01:19  dsandras
 * Protect internal speex state changes with a mutex.
 *
 * Revision 1.6  2006/01/07 17:37:50  dsandras
 * Updated to speex 1.1.11.2 to fix divergeance issues.
 *
 * Revision 1.5  2006/01/05 12:02:31  rjongbloed
 * Fixed DevStudio compile errors
 *
 * Revision 1.4  2005/12/29 16:20:53  dsandras
 * Added wideband support to the echo canceller.
 *
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

private:

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

#endif // __OPAL_ECHOCANCEL_H

/////////////////////////////////////////////////////////////////////////////
