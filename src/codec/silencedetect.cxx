/*
 * silencedetect.cxx
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
 * Contributor(s): ______________________________________.
 *
 */

#include <ptlib.h>

#ifdef __GNUC__
#pragma implementation "silencedetect.h"
#endif
#include <opal_config.h>

#include <codec/silencedetect.h>
#include <opal/patch.h>

#define new PNEW
#define PTraceModule() "Silence"


extern "C" {
  unsigned char linear2ulaw(int pcm_val);
  int ulaw2linear(unsigned char u_val);
};


///////////////////////////////////////////////////////////////////////////////

OpalSilenceDetector::OpalSilenceDetector(const Params & theParam)
  : m_receiveHandler(PCREATE_NOTIFIER(ReceivedPacket))
  , m_clockRate (8000)
{
  // Initialise the adaptive threshold variables.
  SetParameters(theParam);

  PTRACE(4, "Handler created");
}


void OpalSilenceDetector::AdaptiveReset()
{
  // Initialise threshold level
  m_levelThreshold = 0;

  // Initialise the adaptive threshold variables.
  m_signalMinimum = UINT_MAX;
  m_silenceMaximum = 0;
  m_signalReceivedTime = 0;
  m_silenceReceivedTime = 0;

  // Restart in silent mode, unless not detecting
  m_lastResult = m_mode == NoSilenceDetection ? VoiceActive : IsSilent;
  m_lastTimestamp = 0;
  m_receivedTime = 0;
}


void OpalSilenceDetector::SetParameters(const Params & newParam, const int rate /*= 0*/)
{
  PWaitAndSignal mutex(m_inUse);
  if (rate)
    m_clockRate = rate;
  m_mode = newParam.m_mode;
  m_signalDeadband = newParam.m_signalDeadband*m_clockRate/1000;
  m_silenceDeadband = newParam.m_silenceDeadband*m_clockRate/1000;
  m_adaptivePeriod = newParam.m_adaptivePeriod*m_clockRate/1000;
  if (m_mode == FixedSilenceDetection)
    m_levelThreshold = newParam.m_threshold;// note: this value compared to uLaw encoded signal level
  else
    AdaptiveReset();

  PTRACE(3, "Parameters set: "
            "mode=" << m_mode << ", "
            "threshold=" << m_levelThreshold << ", "
            "silencedb=" << m_silenceDeadband << " samples, "
            "signaldb=" << m_signalDeadband << " samples, "
            "period=" << m_adaptivePeriod << " samples");
}


void OpalSilenceDetector::SetClockRate(unsigned rate)
{
  PWaitAndSignal mutex(m_inUse);
  m_signalDeadband = m_signalDeadband * 1000 / m_clockRate * rate / 1000;
  m_silenceDeadband = m_silenceDeadband * 1000 / m_clockRate * rate / 1000;
  m_adaptivePeriod = m_adaptivePeriod * 1000 / m_clockRate * rate / 1000;
  m_clockRate = rate;
  if (m_mode == AdaptiveSilenceDetection)
    AdaptiveReset();
}


void OpalSilenceDetector::GetParameters(Params & params)
{
  params.m_mode = m_mode;
  params.m_threshold = m_levelThreshold*1000/m_clockRate;
  params.m_signalDeadband = m_signalDeadband*1000/m_clockRate;
  params.m_silenceDeadband = m_silenceDeadband*1000/m_clockRate;
  params.m_adaptivePeriod = m_adaptivePeriod*1000/m_clockRate;
}


PString OpalSilenceDetector::Params::AsString() const
{
  return PSTRSTRM(m_mode << ','
               << m_threshold << ','
               << m_signalDeadband << ','
               << m_silenceDeadband << ','
               << m_adaptivePeriod);
}


void OpalSilenceDetector::Params::FromString(const PString & str)
{
  PStringArray params = str.Tokenise(',');
  switch (params.GetSize()) {
    default :
    case 5 :
      m_adaptivePeriod = params[4].AsUnsigned();
    case 4 :
      m_silenceDeadband = params[3].AsUnsigned();
    case 3 :
      m_signalDeadband = params[2].AsUnsigned();
    case 2 :
      m_threshold = params[1].AsUnsigned();
    case 1 :
      m_mode = params[0].As<OpalSilenceDetector::Modes>(m_mode);
    case 0 :
      break;
  }
}


OpalSilenceDetector::Result OpalSilenceDetector::GetResult(unsigned * currentThreshold, unsigned * currentLevel) const
{
  PWaitAndSignal mutex(m_inUse);

  if (currentThreshold != NULL)
    *currentThreshold = m_levelThreshold;

  if (currentLevel != NULL)
    *currentLevel = m_lastSignalLevel;

  return m_lastResult;
}


void OpalSilenceDetector::ReceivedPacket(RTP_DataFrame & frame, P_INT_PTR)
{
  switch (Detect(frame.GetPayloadPtr(), frame.GetPayloadSize(), frame.GetTimestamp())) {
    case IsSilent :
      frame.SetPayloadSize(0); // Not in talk burst so silence the frame
      break;

    case VoiceActivated :
      frame.SetMarker(true); // If we just have moved to sending a talk burst, set the RTP marker
    default :
      break;
  }
}


OpalSilenceDetector::Result OpalSilenceDetector::Detect(const BYTE * audioPtr, PINDEX audioLen, unsigned timestamp)
{
  // Already silent
  if (audioLen == 0)
    return m_lastResult = IsSilent;

  PWaitAndSignal mutex(m_inUse);

  // Can never have silence if NoSilenceDetection
  if (m_mode == NoSilenceDetection)
    return m_lastResult;

  if (m_lastTimestamp == 0) {
    m_lastTimestamp = timestamp;
    return m_lastResult;
  }

  unsigned timeSinceLastFrame = timestamp - m_lastTimestamp;
  m_lastTimestamp = timestamp;

  // Average is absolute value up to 32767
  unsigned rawSignalLevel = GetAverageSignalLevel(audioPtr, audioLen);

  // Can never have average signal level that high, this indicates that the
  // GetAverageSignalLevel (possibly hardware) cannot do energy calculation.
  if (m_lastSignalLevel == UINT_MAX)
    return m_lastResult = VoiceActive;

  // Convert to a logarithmic scale - use uLaw which is complemented
  m_lastSignalLevel = linear2ulaw(rawSignalLevel) ^ 0xff;

  // Now if signal level above threshold we are "talking"
  bool haveSignal = m_lastSignalLevel > m_levelThreshold;

  // If no change ie still talking or still silent, reset frame counter
  if ((m_lastResult != IsSilent) == haveSignal) {
    m_receivedTime = 0;
    if (m_lastResult == VoiceActivated)
      m_lastResult = VoiceActive;
  }
  else {
    m_receivedTime += timeSinceLastFrame;
    // If have had enough consecutive frames talking/silent, swap modes.
    if (m_receivedTime >= (m_lastResult != IsSilent ? m_silenceDeadband : m_signalDeadband)) {
      m_lastResult = m_lastResult != IsSilent ? IsSilent : VoiceActivated;
      PTRACE(4, "Detector transition: "
             << (m_lastResult != IsSilent ? "Talk" : "Silent")
             << " level=" << m_lastSignalLevel << " threshold=" << m_levelThreshold);

      // If we had talk/silence transition restart adaptive threshold measurements
      m_signalMinimum = UINT_MAX;
      m_silenceMaximum = 0;
      m_signalReceivedTime = 0;
      m_silenceReceivedTime = 0;
    }
    else {
      if (m_lastResult == VoiceActivated)
        m_lastResult = VoiceActive;
    }
  }

  if (m_mode == FixedSilenceDetection)
    return m_lastResult;

  // Adaptive silence detection
  if (m_levelThreshold == 0) {
    if (m_lastSignalLevel > 1) {
      // Bootstrap condition, use first frame level as silence level
      m_levelThreshold = m_lastSignalLevel/2;
      PTRACE(4, "Threshold initialised to: " << m_levelThreshold);
    }
    return m_lastResult;
  }

  // Count the number of silent and signal frames and calculate min/max
  if (haveSignal) {
    if (m_lastSignalLevel < m_signalMinimum)
      m_signalMinimum = m_lastSignalLevel;
    m_signalReceivedTime = m_signalReceivedTime + timeSinceLastFrame;
  }
  else {
    if (m_lastSignalLevel > m_silenceMaximum)
      m_silenceMaximum = m_lastSignalLevel;
    m_silenceReceivedTime = m_silenceReceivedTime + timeSinceLastFrame;
  }

  // See if we have had enough frames to look at proportions of silence/signal
  if ((m_signalReceivedTime + m_silenceReceivedTime) > m_adaptivePeriod) {

    /* Now we have had a period of time to look at some average values we can
       make some adjustments to the threshold. There are four cases:
     */
    if (m_signalReceivedTime >= m_adaptivePeriod) {
      /* If every frame was noisy, move threshold up. Don't want to move too
         fast so only go a quarter of the way to minimum signal value over the
         period. This avoids oscillations, and time will continue to make the
         level go up if there really is a lot of background noise.
       */
      int delta = (m_signalMinimum - m_levelThreshold)/4;
      if (delta != 0) {
        m_levelThreshold += delta;
        PTRACE(4, "Threshold increased to: " << m_levelThreshold);
      }
    }
    else if (m_silenceReceivedTime >= m_adaptivePeriod) {
      /* If every frame was silent, move threshold down. Again do not want to
         move too quickly, but we do want it to move faster down than up, so
         move to halfway to maximum value of the quiet period. As a rule the
         lower the threshold the better as it would improve response time to
         the start of a talk burst.
       */
      unsigned newThreshold = (m_levelThreshold + m_silenceMaximum)/2 + 1;
      if (m_levelThreshold != newThreshold) {
        m_levelThreshold = newThreshold;
        PTRACE(4, "Threshold decreased to: " << m_levelThreshold);
      }
    }
    else if (m_signalReceivedTime > m_silenceReceivedTime) {
      /* We haven't got a definitive silent or signal period, but if we are
         constantly hovering at the threshold and have more signal than
         silence we should creep up a bit.
       */
      m_levelThreshold++;
      PTRACE(4, "Threshold incremented to: " << m_levelThreshold
             << " signal=" << m_signalReceivedTime << ' ' << m_signalMinimum
             << " silence=" << m_silenceReceivedTime << ' ' << m_silenceMaximum);
    }

    m_signalMinimum = UINT_MAX;
    m_silenceMaximum = 0;
    m_signalReceivedTime = 0;
    m_silenceReceivedTime = 0;
  }

  return m_lastResult;
}


unsigned OpalSilenceDetector::GetAverageSignalLevelPCM16(const BYTE * buffer, PINDEX size, bool asPercentage)
{
  // Calculate the average signal level of this frame
  int sum = 0;
  PINDEX samples = size/2;
  const short * pcm = (const short *)buffer;
  const short * end = pcm + samples;
  while (pcm != end) {
    if (*pcm < 0)
      sum -= *pcm++;
    else
      sum += *pcm++;
  }

  unsigned average = sum / samples;
  return asPercentage ? ((linear2ulaw(average) ^ 0xff) * 100 / 127) : average;
}


/////////////////////////////////////////////////////////////////////////////

unsigned OpalPCM16SilenceDetector::GetAverageSignalLevel(const BYTE * buffer, PINDEX size)
{
  return GetAverageSignalLevelPCM16(buffer, size, false);
}


/////////////////////////////////////////////////////////////////////////////
