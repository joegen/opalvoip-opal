/*
 * vidcodec.cxx
 *
 * Uncompressed video handler
 *
 * Open Phone Abstraction Library
 *
 * Copyright (c) 2003 Equivalence Pty. Ltd.
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
#pragma implementation "vidcodec.h"
#endif

#include <opal_config.h>

#if OPAL_VIDEO

#include <codec/vidcodec.h>

#include <ptlib/videoio.h>
#include <ptlib/vconvert.h>

#define FRAME_WIDTH  PVideoDevice::MaxWidth
#define FRAME_HEIGHT PVideoDevice::MaxHeight
#define FRAME_RATE   50U

const OpalVideoFormat & GetOpalRGB24()
{
  static const OpalVideoFormat RGB24(
    OPAL_RGB24,
    RTP_DataFrame::MaxPayloadType,
    NULL,
    FRAME_WIDTH, FRAME_HEIGHT,
    FRAME_RATE,
    24U*FRAME_WIDTH*FRAME_HEIGHT*FRAME_RATE  // Bandwidth
  );
  return RGB24;
}

const OpalVideoFormat & GetOpalRGB32()
{
  static const OpalVideoFormat RGB32(
    OPAL_RGB32,
    RTP_DataFrame::MaxPayloadType,
    NULL,
    FRAME_WIDTH, FRAME_HEIGHT,
    FRAME_RATE,
    32U*FRAME_WIDTH*FRAME_HEIGHT*FRAME_RATE  // Bandwidth
  );
  return RGB32;
}

const OpalVideoFormat & GetOpalBGR24()
{
  static const OpalVideoFormat BGR24(
    OPAL_BGR24,
    RTP_DataFrame::MaxPayloadType,
    NULL,
    FRAME_WIDTH, FRAME_HEIGHT,
    FRAME_RATE,
    24*FRAME_WIDTH*FRAME_HEIGHT*FRAME_RATE  // Bandwidth
  );
  return BGR24;
}

const OpalVideoFormat & GetOpalBGR32()
{
  static const OpalVideoFormat BGR32(
    OPAL_BGR32,
    RTP_DataFrame::MaxPayloadType,
    NULL,
    FRAME_WIDTH, FRAME_HEIGHT,
    FRAME_RATE,
    32*FRAME_WIDTH*FRAME_HEIGHT*FRAME_RATE  // Bandwidth
  );
  return BGR32;
}

const OpalVideoFormat & GetOpalYUV420P()
{
  static const OpalVideoFormat YUV420P(
    OPAL_YUV420P,
    RTP_DataFrame::MaxPayloadType,
    NULL,
    FRAME_WIDTH, FRAME_HEIGHT,
    FRAME_RATE,
    12U*FRAME_WIDTH*FRAME_HEIGHT*FRAME_RATE  // Bandwidth
  );
  return YUV420P;
}


#define new PNEW
#define PTraceModule() "Media"


/////////////////////////////////////////////////////////////////////////////

OpalVideoTranscoder::OpalVideoTranscoder(const OpalMediaFormat & inputMediaFormat,
                                         const OpalMediaFormat & outputMediaFormat)
  : OpalTranscoder(inputMediaFormat, outputMediaFormat)
  , m_inDataSize(10 * 1024)
  , m_outDataSize(10 * 1024)
  , m_errorConcealment(false)
  , m_freezeTillIFrame(false)
  , m_frozenTillIFrame(false)
  , m_lastFrameWasIFrame(false)
  , m_frameDropRate(0)
  , m_frameDropBits(0)
  , m_lastTimestamp(UINT_MAX)
{
  acceptEmptyPayload = true;
}


static void SetFrameBytes(const OpalMediaFormat & fmt, const PString & widthOption, const PString & heightOption, PINDEX & size)
{
  int width  = fmt.GetOptionInteger(widthOption, PVideoFrameInfo::CIFWidth);
  int height = fmt.GetOptionInteger(heightOption, PVideoFrameInfo::CIFHeight);
  int newSize = PVideoDevice::CalculateFrameBytes(width, height, fmt);
  if (newSize > 0)
    size = sizeof(PluginCodec_Video_FrameHeader) + newSize;
}


bool OpalVideoTranscoder::UpdateMediaFormats(const OpalMediaFormat & input, const OpalMediaFormat & output)
{
  PWaitAndSignal mutex(updateMutex);

  if (!OpalTranscoder::UpdateMediaFormats(input, output))
    return false;

  SetFrameBytes(inputMediaFormat,  OpalVideoFormat::MaxRxFrameWidthOption(), OpalVideoFormat::MaxRxFrameHeightOption(), m_inDataSize);
  SetFrameBytes(outputMediaFormat, OpalVideoFormat::FrameWidthOption(),      OpalVideoFormat::FrameHeightOption(),      m_outDataSize);

  m_frameDropRate = outputMediaFormat.GetOptionBoolean(OpalVideoFormat::FrameDropOption(), true)
                            ? outputMediaFormat.GetOptionInteger(OpalMediaFormat::TargetBitRateOption()) : 0;

  m_freezeTillIFrame = inputMediaFormat.GetOptionBoolean(OpalVideoFormat::FreezeUntilIntraFrameOption()) ||
                      outputMediaFormat.GetOptionBoolean(OpalVideoFormat::FreezeUntilIntraFrameOption());

  return true;
}


PINDEX OpalVideoTranscoder::GetOptimalDataFrameSize(PBoolean input) const
{
  if (input)
    return m_inDataSize;

  if (m_outDataSize < maxOutputSize)
    return m_outDataSize;

  return maxOutputSize;
}


PBoolean OpalVideoTranscoder::ExecuteCommand(const OpalMediaCommand & command)
{
  if (PIsDescendant(&command, OpalVideoUpdatePicture))
    return HandleIFrameRequest();

  return OpalTranscoder::ExecuteCommand(command);
}


bool OpalVideoTranscoder::HandleIFrameRequest()
{
  if (outputMediaFormat == OpalYUV420P)
    return false;

  PTRACE_CONTEXT_ID_TO(m_encodingIntraFrameControl);
  m_encodingIntraFrameControl.IntraFrameRequest();
  return true;
}


PBoolean OpalVideoTranscoder::Convert(const RTP_DataFrame & /*input*/,
                                  RTP_DataFrame & /*output*/)
{
  return false;
}


#if OPAL_STATISTICS
void OpalVideoTranscoder::GetStatistics(OpalMediaStatistics & statistics) const
{
  OpalTranscoder::GetStatistics(statistics);
  statistics.m_droppedFrames = m_framesDropped;
}
#endif

    
bool OpalVideoTranscoder::ShouldDropFrame(RTP_Timestamp ts)
{
  if (m_frameDropRate == 0)
    return false;

  if (m_lastTimestamp == UINT_MAX) {
    m_lastTimestamp = ts;
    return false;
  }

  RTP_Timestamp delta = ts - m_lastTimestamp;
  m_lastTimestamp = ts;

  // Use the expected frame time if the provided timestamps look suspicious
  if (delta == 0 || delta > OpalMediaFormat::VideoClockRate)
    delta = outputMediaFormat.GetFrameTime();

  /* This is a modified variable duty cycle algorithm. That algorithm works with the two
     parameters being constant. But neither the time nor the bits are constant in this case.
     So, we need to allow some "slop" in both directions. Effectively allowing an overspend
     of up to a frames worth of bits, so we can work with the actual bit rate being
     consistently smaller than the target bit rate at the other end. */
  int bitsForOneFrame = (int)((uint64_t)m_frameDropRate*delta/OpalMediaFormat::VideoClockRate);

  /* If encoder consistently generates slightly more bits than bitsForOneFrame then
     this subtraction will allow that frame to go out, but it will eventually build
     up in m_frameDropBits and cause a frame drop eventually.

     If the encoder generated a large I-Frame with many many bits, then m_frameDropBits
     is large for a while as the subtraction whittles away at it bitsForOneFrame at a
     time until it eventually lets a frame through. */
  m_frameDropBits -= bitsForOneFrame;

  if (m_frameDropBits < bitsForOneFrame) {
    /* If raw encoder is consistently providing fewer bits than the bit rate for every
       frame, we go negative, so we clamp to zero as everything can flow freely and 
       we don't need, or want, any "history" to remember in this mode. */
    if (m_frameDropBits < 0)
        m_frameDropBits = 0;

    // Allow frame
    return false;
  }

  // Sent too many bits, drop frame
  PTRACE(4, "Frame dropped:"
            " overrun=" << PString(PString::ScaleSI, m_frameDropBits, 3) << "bits,"
            " rate=" << PString(PString::ScaleSI, m_frameDropRate, 3) << "bps");
  ++m_framesDropped;
  return true;
}


void OpalVideoTranscoder::UpdateFrameDrop(const RTP_DataFrameList & encoded)
{
  for (RTP_DataFrameList::const_iterator it = encoded.begin(); it != encoded.end(); ++it)
    m_frameDropBits += it->GetPayloadSize()*8;
}


void OpalVideoTranscoder::SendIFrameRequest(unsigned sequenceNumber, unsigned timestamp)
{
  m_frozenTillIFrame = m_freezeTillIFrame;

  PTRACE_CONTEXT_ID_TO(m_decodingIntraFrameControl);
  m_decodingIntraFrameControl.IntraFrameRequest();

  if (!m_decodingIntraFrameControl.RequireIntraFrame())
    return;

  if (sequenceNumber == 0 && timestamp == 0)
    NotifyCommand(OpalVideoUpdatePicture());
  else
    NotifyCommand(OpalVideoPictureLoss(sequenceNumber, timestamp));
}


///////////////////////////////////////////////////////////////////////////////

PTimeInterval const OpalIntraFrameControl::DefaultMinThrottle(500);
PTimeInterval const OpalIntraFrameControl::DefaultMaxThrottle(0,4);
PTimeInterval const OpalIntraFrameControl::DefaultPeriodic(0,0,1);

OpalIntraFrameControl::OpalIntraFrameControl(const PTimeInterval & minThrottle,
                                             const PTimeInterval & maxThrottle,
                                             const PTimeInterval & periodic)
  : m_minThrottleTime(minThrottle)
  , m_maxThrottleTime(maxThrottle)
  , m_periodicTime(periodic)
  , m_retryTime(500)
  , m_currentThrottleTime(minThrottle)
  , m_state(e_Idle)
  , m_stuckCount(0)
  , m_lastRequest(0)
{
  m_requestTimer.SetNotifier(PCREATE_NOTIFIER(OnTimedRequest), "OpalIntraCtrl");
  PTRACE(4, "Constructed I-Frame request control: this=" << this);
}


void OpalIntraFrameControl::SetTimes(const PTimeInterval & minThrottle,
                                     const PTimeInterval & maxThrottle,
                                     const PTimeInterval & periodic,
                                     const PTimeInterval & retry)
{
  PWaitAndSignal mutex(m_mutex);
  m_minThrottleTime = minThrottle;
  m_maxThrottleTime = maxThrottle;
  m_periodicTime = periodic;
  m_retryTime = retry;
}


void OpalIntraFrameControl::IntraFrameRequest()
{
  {
    PWaitAndSignal mutex(m_mutex);

    switch (m_state) {
      case e_Idle :
        if (m_maxThrottleTime == 0 || m_retryTime == 0) {
          m_requestTimer.Stop(false);
          return;
        }
        break;

      case e_Throttled :
        // Back to requested state so when timer fires, it will do a retry for new request
        m_state = e_Requested;
        PTRACE(3, "Queuing I-Frame request as throttled: delay=" << m_currentThrottleTime << " this=" << this);
        return;

      default :
        // Don't touch timer, this request will be serviced by existing I-Frame on it's way
        PTRACE(4, "Ignoring I-Frame request as already in progress: state=" << m_state << " this=" << this);
        return;
    }

    PTime now;
    PTimeInterval timeSinceLast = now - m_lastRequest;
    m_lastRequest = now;

    if (timeSinceLast < m_minThrottleTime) {
      m_currentThrottleTime = m_currentThrottleTime * 2;
      if (m_currentThrottleTime > m_maxThrottleTime)
        m_currentThrottleTime = m_maxThrottleTime;
    }
    else if (timeSinceLast > m_maxThrottleTime) {
      m_currentThrottleTime = m_currentThrottleTime / 2;
      if (m_currentThrottleTime < m_minThrottleTime)
        m_currentThrottleTime = m_minThrottleTime;
    }

    m_state = e_Requesting;
    m_stuckCount = 0;
    PTRACE(3, "Immediate I-Frame request: retry=" << m_retryTime << " this=" << this);
  }

  m_requestTimer = m_retryTime; // Outside mutex
}


bool OpalIntraFrameControl::RequireIntraFrame()
{
  PTimeInterval newTimeout;

  {
    PWaitAndSignal mutex(m_mutex);

    switch (m_state) {
      case e_Requesting:
        m_state = e_Requested;
        break;

      case e_Periodic:
        m_state = e_Idle;
        break;

      case e_Idle :
        if (m_maxThrottleTime > 0 && m_retryTime > 0 && m_periodicTime > 0 && !m_requestTimer.IsRunning()) {
          newTimeout = m_periodicTime;
          PTRACE(4, "Initial Periodic I-Frame request: next=" << m_periodicTime << " this=" << this);
          break;
        }
        // Do default case

      default:
        return false;
    }
  }

  if (newTimeout > 0) {
    m_requestTimer = newTimeout;
    return false;
  }

  PTRACE(4, "I-Frame requested: retry=" << m_requestTimer.GetResetTime() << " this=" << this);
  return true;
}


void OpalIntraFrameControl::IntraFrameDetected()
{
  PTimeInterval newTimeout;

  {
    PWaitAndSignal mutex(m_mutex);

    switch (m_state) {
      case e_Idle :
        newTimeout = m_periodicTime;
        break;

      case e_Requested:
        m_state = e_Throttled;
        newTimeout = m_currentThrottleTime;
        PTRACE(4, "I-Frame detected: throttle=" << m_currentThrottleTime << " this=" << this);
        break;

      default :
        break;
    }
  }

  if (newTimeout > 0)
    m_requestTimer = newTimeout; // Outside mutex
}


void OpalIntraFrameControl::OnTimedRequest(PTimer &, P_INT_PTR)
{
  PWaitAndSignal mutex(m_mutex);

  // If idle, then this is a periodic request, restart the timer for another
  switch (m_state) {
    case e_Idle :
      if (m_periodicTime > 0) {
        m_state = e_Periodic;
        m_requestTimer = m_periodicTime;
        PTRACE(4, "Periodic I-Frame request: next=" << m_periodicTime << " this=" << this);
      }
      break;

    case e_Throttled :
      m_state = e_Idle;
      if (m_periodicTime > 0) {
        m_requestTimer = m_periodicTime;
        PTRACE(4, "End throttled I-Frames: next=" << m_periodicTime << " this=" << this);
      }
      break;

    case e_Requested :
      m_state = e_Requesting;
      m_stuckCount = 0;
      m_requestTimer = m_retryTime;
      PTRACE(4, "Retry I-Frame request: retry=" << m_retryTime << " this=" << this);
      break;

    default :
      if (++m_stuckCount < 10) {
        m_requestTimer = m_requestTimer.GetResetTime();
        PTRACE(m_stuckCount == 1 ? 2 : 3, "Encoder slow, still requesting I-Frame: "
                                          "retry=" << m_requestTimer.GetResetTime() << " this=" << this);
      }
      else
        PTRACE(2, "Encoder stopped, not requesting I-Frames any more: this=" << this);
      break;
  }
}


///////////////////////////////////////////////////////////////////////////////

OpalVideoUpdatePicture::OpalVideoUpdatePicture(unsigned sessionID, unsigned ssrc)
  : OpalMediaCommand(OpalMediaType::Video(), sessionID, ssrc)
{
}


PString OpalVideoUpdatePicture::GetName() const
{
  return "Update Picture";
}


OpalVideoPictureLoss::OpalVideoPictureLoss(unsigned sequenceNumber, unsigned timestamp, unsigned sessionID, unsigned ssrc)
  : OpalVideoUpdatePicture(sessionID, ssrc)
  , m_sequenceNumber(sequenceNumber)
  , m_timestamp(timestamp)
{
}


PString OpalVideoPictureLoss::GetName() const
{
  return "Picture Loss";
}


OpalTemporalSpatialTradeOff::OpalTemporalSpatialTradeOff(unsigned tradeoff, unsigned sessionID, unsigned ssrc)
  : OpalMediaCommand(OpalMediaType::Video(), sessionID, ssrc)
  , m_tradeOff(tradeoff)
{
}


PString OpalTemporalSpatialTradeOff::GetName() const
{
  return "Temporal Spatial Trade Off";
}


#endif // OPAL_VIDEO

/////////////////////////////////////////////////////////////////////////////
