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
 * $Revision$
 * $Author$
 * $Date$
 */

#ifndef OPAL_CODEC_SILENCEDETECT_H
#define OPAL_CODEC_SILENCEDETECT_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif

#include <opal/buildopts.h>
#include <rtp/rtp.h>


///////////////////////////////////////////////////////////////////////////////

class OpalSilenceDetector : public PObject
{
    PCLASSINFO(OpalSilenceDetector, PObject);
  public:
    enum Mode {
      NoSilenceDetection,
      FixedSilenceDetection,
      AdaptiveSilenceDetection,
      NumModes
    };

    struct Params {
      Params(
        Mode mode = AdaptiveSilenceDetection, ///<  New silence detection mode
        unsigned threshold = 0,               ///<  Threshold value if FixedSilenceDetection
        unsigned signalDeadband = 80,         ///<  10 milliseconds of signal needed
        unsigned silenceDeadband = 3200,      ///<  400 milliseconds of silence needed
        unsigned adaptivePeriod = 4800        ///<  600 millisecond window for adaptive threshold
      )
        : m_mode(mode),
          m_threshold(threshold),
          m_signalDeadband(signalDeadband),
          m_silenceDeadband(silenceDeadband),
          m_adaptivePeriod(adaptivePeriod)
        { }

      Mode     m_mode;             /// Silence detection mode
      unsigned m_threshold;        /// Threshold value if FixedSilenceDetection
      unsigned m_signalDeadband;   /// 10 milliseconds of signal needed
      unsigned m_silenceDeadband;  /// 400 milliseconds of silence needed
      unsigned m_adaptivePeriod;   /// 600 millisecond window for adaptive threshold
    };

  /**@name Construction */
  //@{
    /**Create a new connection.
     */
    OpalSilenceDetector(
      const Params & newParam ///<  New parameters for silence detector
    );
  //@}

  /**@name Basic operations */
  //@{
    const PNotifier & GetReceiveHandler() const { return receiveHandler; }

    /**Set the silence detector parameters.
       This enables, disables the silence detector as well as adjusting its
       "agression". The deadband periods are in audio samples of 8kHz.
      */
    void SetParameters(
      const Params & newParam ///<  New parameters for silence detector
    );

    /**Get silence detection status

       The inTalkBurst value is PTrue if packet transmission is enabled and
       PFalse if it is being suppressed due to silence.

       The currentThreshold value is the value from 0 to 32767 which is used
       as the threshold value for 16 bit PCM data.
      */
    Mode GetStatus(
      PBoolean * isInTalkBurst,
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
      const BYTE * buffer,  ///<  RTP payload being detected
      PINDEX size           ///<  Size of payload buffer
    ) = 0;

  protected:
    PDECLARE_NOTIFIER(RTP_DataFrame, OpalSilenceDetector, ReceivedPacket);

    PNotifier receiveHandler;

    Params param;

    unsigned lastTimestamp;         // Last timestamp received
    unsigned receivedTime;          // Signal/Silence duration received so far.
    unsigned levelThreshold;        // Threshold level for silence/signal
    unsigned signalMinimum;         // Minimum of frames above threshold
    unsigned silenceMaximum;        // Maximum of frames below threshold
    unsigned signalReceivedTime;    // Duration of signal received
    unsigned silenceReceivedTime;   // Duration of silence received
    bool     inTalkBurst;           // Currently sending RTP data
};


class OpalPCM16SilenceDetector : public OpalSilenceDetector
{
    PCLASSINFO(OpalPCM16SilenceDetector, OpalSilenceDetector);
  public:
    /** Construct new silence detector for PCM-16
      */
    OpalPCM16SilenceDetector(
      const Params & newParam ///<  New parameters for silence detector
    ) : OpalSilenceDetector(newParam) { }

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
      const BYTE * buffer,  ///<  RTP payload being detected
      PINDEX size           ///<  Size of payload buffer
    );
  //@}
};


extern ostream & operator<<(ostream & strm, OpalSilenceDetector::Mode mode);


#endif // OPAL_CODEC_SILENCEDETECT_H


/////////////////////////////////////////////////////////////////////////////
