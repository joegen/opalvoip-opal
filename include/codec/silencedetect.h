/*
 * silencedetect.h
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
 * The Initial Developer of the Original Code is Equivalence Pty. Ltd.
 *
 * Contributor(s): ______________________________________.
 *
 * $Log: silencedetect.h,v $
 * Revision 1.1  2004/05/17 13:24:26  rjongbloed
 * Added silence suppression.
 *
 */

#ifndef __OPAL_SILENCEDETECT_H
#define __OPAL_SILENCEDETECT_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif


#include <rtp/rtp.h>


///////////////////////////////////////////////////////////////////////////////

class OpalSilenceDetector : public PObject
{
    PCLASSINFO(OpalSilenceDetector, PObject);
  public:
    OpalSilenceDetector();

    const PNotifier & GetReceiveHandler() const { return receiveHandler; }

    enum Mode {
      NoSilenceDetection,
      FixedSilenceDetection,
      AdaptiveSilenceDetection
    };

    /**Enable/Disable silence detection.
       The deadband periods are in audio samples of 8kHz.
      */
    void SetMode(
      Mode mode,   /// New silence detection mode
      unsigned threshold = 0,      /// Threshold value if FixedSilenceDetection
      unsigned signalDeadband = 80,    /// 10 milliseconds of signal needed
      unsigned silenceDeadband = 3200, /// 400 milliseconds of silence needed
      unsigned adaptivePeriod = 4800   /// 600 millisecond window for adaptive threshold
    );

    /**Get silence detection mode

       The inTalkBurst value is TRUE if packet transmission is enabled and
       FALSE if it is being suppressed due to silence.

       The currentThreshold value is the value from 0 to 32767 which is used
       as the threshold value for 16 bit PCM data.
      */
    Mode GetMode(
      BOOL * isInTalkBurst,
      unsigned * currentThreshold
    ) const;

    /**Get the average signal level in the stream.
       This is called from within the silence detection algorithm to
       calculate the average signal level of the last data frame read from
       the stream.

       The default behaviour returns UINT_MAX which indicates that the
       average signal has no meaning for the stream.
      */
    virtual unsigned GetAverageSignalLevel(
      const BYTE * buffer,  /// RTP payload being detected
      PINDEX size           /// Size of payload buffer
    ) = 0;

  protected:
    PDECLARE_NOTIFIER(RTP_DataFrame, OpalSilenceDetector, ReceivedPacket);

    PNotifier receiveHandler;

    Mode mode;

    unsigned signalDeadbandTime;  // Duration of signal before talk burst starts
    unsigned silenceDeadbandTime; // Duration of silence before talk burst ends
    unsigned adaptiveThresholdTime; // Duration to min/max over for adaptive threshold

    BOOL     inTalkBurst;           // Currently sending RTP data
    unsigned lastTimestamp;         // Last timestamp received
    unsigned receivedTime;          // Signal/Silence duration received so far.
    unsigned levelThreshold;        // Threshold level for silence/signal
    unsigned signalMinimum;         // Minimum of frames above threshold
    unsigned silenceMaximum;        // Maximum of frames below threshold
    unsigned signalReceivedTime;    // Duration of signal received
    unsigned silenceReceivedTime;   // Duration of silence received
};


class OpalPCM16SilenceDetector : public OpalSilenceDetector
{
    PCLASSINFO(OpalPCM16SilenceDetector, OpalSilenceDetector);
  public:
  /**@name Overrides from OpalSilenceDetector */
  //@{
    /**Get the average signal level in the stream.
       This is called from within the silence detection algorithm to
       calculate the average signal level of the last data frame read from
       the stream.

       The default behaviour returns UINT_MAX which indicates that the
       average signal has no meaning for the stream.
      */
    virtual unsigned GetAverageSignalLevel(
      const BYTE * buffer,  /// RTP payload being detected
      PINDEX size           /// Size of payload buffer
    );
  //@}
};



#endif // __OPAL_SILENCEDETECT_H


/////////////////////////////////////////////////////////////////////////////
