/*
 * Python bindings.
 *
 * Open Phone Abstraction Library (OPAL)
 *
 * Copyright (c) 2011 Demetrius Cassidy
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
 * The Original Code is Open Phone Abstraction Library (OPAL)
 *
 * The Initial Developer of the Original Code is Demetrius Cassidy
 *
 * Contributor(s): ______________________________________.
 *
 * $Revision$
 * $Author$
 * $Date$
 */

%ModuleHeaderCode
#include <ptlib.h>
#include <codec/silencedetect.h>
%End

class OpalSilenceDetector : PObject /Abstract/
{

  public:
    enum Mode {
      NoSilenceDetection,
      FixedSilenceDetection,
      AdaptiveSilenceDetection,
      NumModes
    };

    struct Params {
      Params(
        OpalSilenceDetector::Mode mode = OpalSilenceDetector::AdaptiveSilenceDetection, ///<  New silence detection mode
        unsigned threshold = 0,               ///<  Threshold value if FixedSilenceDetection
        unsigned signalDeadband = 80,         ///<  10 milliseconds of signal needed
        unsigned silenceDeadband = 3200,      ///<  400 milliseconds of silence needed
        unsigned adaptivePeriod = 4800        ///<  600 millisecond window for adaptive threshold
      );

      OpalSilenceDetector::Mode     m_mode;             /// Silence detection mode
      unsigned m_threshold;        /// Threshold value if FixedSilenceDetection
      unsigned m_signalDeadband;   /// 10 milliseconds of signal needed
      unsigned m_silenceDeadband;  /// 400 milliseconds of silence needed
      unsigned m_adaptivePeriod;   /// 600 millisecond window for adaptive threshold
    };

   /**@name Construction */
  //@{
    /**Create a new detector. Default clock rate is 8000.
     */
    OpalSilenceDetector(
      const OpalSilenceDetector::Params & newParam ///<  New parameters for silence detector
    );
  //@}

  /**@name Basic operations */
  //@{
    const PNotifier & GetReceiveHandler() const;

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

    /**Set the sampling clock rate for the preprocessor.
       Adusts the interpretation of time values.
       This may be called while audio is being transferred, but if in
       adaptive mode calling this will reset the filter.
     */
    void SetClockRate(
      const int clockRate     ///< Sampling clock rate for the preprocessor
    );

    /**Get the cyrrent sampling clock rate.
      */
    int GetClockRate() const;

    /**Get silence detection status

       The inTalkBurst value is true if packet transmission is enabled and
       false if it is being suppressed due to silence.

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

};


class OpalPCM16SilenceDetector : OpalSilenceDetector
{

  public:
    /** Construct new silence detector for PCM-16
      */
    OpalPCM16SilenceDetector(
      const OpalSilenceDetector::Params & newParam ///<  New parameters for silence detector
    );

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