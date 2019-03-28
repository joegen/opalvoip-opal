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
 */

#ifndef OPAL_CODEC_SILENCEDETECT_H
#define OPAL_CODEC_SILENCEDETECT_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif

#include <opal_config.h>
#include <rtp/rtp.h>


///////////////////////////////////////////////////////////////////////////////

/**Implement silence detection of audio.
   This is the complement of Voice Activity Detection (VAD) and can be used for
   that purpose.
  */
class OpalSilenceDetector : public PObject
{
    PCLASSINFO(OpalSilenceDetector, PObject);
  public:
    P_DECLARE_STREAMABLE_ENUM(Modes,
      NoSilenceDetection,
      FixedSilenceDetection,
      AdaptiveSilenceDetection
    );
    typedef Modes Mode; // Backward compatibility

    struct Params {
      Params(
        Modes mode = AdaptiveSilenceDetection, ///<  New silence detection mode
        unsigned threshold = 0,               ///<  Threshold value if FixedSilenceDetection
        unsigned signalDeadband = 10,         ///<  10 milliseconds of signal needed
        unsigned silenceDeadband = 400,       ///<  400 milliseconds of silence needed
        unsigned adaptivePeriod = 600         ///<  600 millisecond window for adaptive threshold
      )
        : m_mode(mode),
          m_threshold(threshold),
          m_signalDeadband(signalDeadband),
          m_silenceDeadband(silenceDeadband),
          m_adaptivePeriod(adaptivePeriod)
        { }

      PString AsString() const;
      void FromString(const PString & str);

      Modes    m_mode;             /// Silence detection mode
      unsigned m_threshold;        /// Threshold value if FixedSilenceDetection
      unsigned m_signalDeadband;   /// milliseconds of signal needed
      unsigned m_silenceDeadband;  /// milliseconds of silence needed
      unsigned m_adaptivePeriod;   /// millisecond window for adaptive threshold
    };

  /**@name Construction */
  //@{
    /**Create a new detector. Default clock rate is 8000.
     */
    OpalSilenceDetector(
      const Params & newParam ///<  New parameters for silence detector
    );
  //@}

  /**@name Basic operations */
  //@{
    const PNotifier & GetReceiveHandler() const { return m_receiveHandler; }

    /**Set the silence detector parameters.
       This adjusts the silence detector "agression". The deadband and 
       adaptive periods are in ms units to work for any clock rate.
	   The clock rate value is optional: 0 leaves value unchanged.
       This may be called while audio is being transferred, but if in
       adaptive mode calling this will reset the filter.
      */
    void SetParameters(
      const Params & params,  ///< New parameters for silence detector
      const int clockRate = 0 ///< Sampling clock rate for the preprocessor
    );

    /**Get the current parameters in use.
      */
    void GetParameters(
      Params & params
    );

    /**Set the sampling clock rate for the preprocessor.
       Adusts the interpretation of time values.
       This may be called while audio is being transferred, but if in
       adaptive mode calling this will reset the filter.
     */
    void SetClockRate(
      unsigned clockRate     ///< Sampling clock rate for the preprocessor
    );

    /**Get the current sampling clock rate.
      */
    unsigned GetClockRate() const { return m_clockRate; }

    enum Result
    {
      IsSilent,
      VoiceActivated,
      VoiceActive
    };

    /**Get silence detection status

       The \p currentThreshold value for energy value as logarithmic scale
       from 0 to 255 (actually a u-Law value) which is used as the threshold
       value for audio signal.

       The \p currentLevel value is the energy as logarithmic scale from 0 to
       255 (actually a u-Law value) for last audio frame.
      */
    Result GetResult(
      unsigned * currentThreshold = NULL,
      unsigned * currentLevel = NULL
    ) const;

    /**Detemine (in context) if audio stream is currently silent.
      */
    Result Detect(
      const BYTE * audioPtr,
      PINDEX audioLen,
      unsigned timestamp
    );

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

    /**Get the average signal level in the stream.
       This is called from within the silence detection algorithm to
       calculate the average signal level of the last data frame read from
       the stream.

       The default behaviour returns UINT_MAX which indicates that the
       average signal has no meaning for the stream.
      */
    static unsigned GetAverageSignalLevelPCM16(
      const BYTE * buffer,  ///<  RTP payload being detected
      PINDEX size,          ///<  Size of payload buffer
      bool asPercentage     ///<  Return as percentage on logarithmic scale
    );

  private:
    /**Reset the adaptive filter
     */
    void AdaptiveReset();

  protected:
    PDECLARE_NOTIFIER(RTP_DataFrame, OpalSilenceDetector, ReceivedPacket);

    PNotifier m_receiveHandler;

    Mode     m_mode;
    unsigned m_signalDeadband;        // #samples of signal needed
    unsigned m_silenceDeadband;       // #samples of silence needed
    unsigned m_adaptivePeriod;        // #samples window for adaptive threshold
    unsigned m_clockRate;             // audio sampling rate

    unsigned m_lastTimestamp;         // Last timestamp received
    unsigned m_receivedTime;          // Signal/Silence duration received so far.
    unsigned m_levelThreshold;        // Threshold level for silence/signal
    unsigned m_signalMinimum;         // Minimum of frames above threshold
    unsigned m_silenceMaximum;        // Maximum of frames below threshold
    unsigned m_signalReceivedTime;    // Duration of signal received
    unsigned m_silenceReceivedTime;   // Duration of silence received
    unsigned m_lastSignalLevel;       // Energy level from last data frame
    Result   m_lastResult;            // What it says
    PDECLARE_MUTEX(m_inUse);          // Protects values to allow change while running
};


class OpalPCM16SilenceDetector : public OpalSilenceDetector
{
    PCLASSINFO(OpalPCM16SilenceDetector, OpalSilenceDetector);
  public:
    /** Construct new silence detector for PCM-16/8000
      */
    OpalPCM16SilenceDetector(
      const Params & newParam ///<  New parameters for silence detector
    ) : OpalSilenceDetector(newParam) { }

  /**@name Overrides from OpalSilenceDetector */
  //@{
    virtual unsigned GetAverageSignalLevel(
      const BYTE * buffer,  ///<  RTP payload being detected
      PINDEX size           ///<  Size of payload buffer
    );
    //@}
};


extern ostream & operator<<(ostream & strm, OpalSilenceDetector::Mode mode);


#endif // OPAL_CODEC_SILENCEDETECT_H


/////////////////////////////////////////////////////////////////////////////
