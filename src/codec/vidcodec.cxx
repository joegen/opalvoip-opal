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
          m_requestTimer.Stop();
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

  m_requestTimer = m_retryTime;
}


bool OpalIntraFrameControl::RequireIntraFrame()
{
  PWaitAndSignal mutex(m_mutex);

  switch (m_state) {
    case e_Requesting:
      m_state = e_Requested;
      break;

    case e_Periodic:
      m_state = e_Idle;
      break;

    default:
      return false;
  }

  PTRACE(4, "I-Frame requested: retry=" << m_requestTimer.GetResetTime() << " this=" << this);
  return true;
}


void OpalIntraFrameControl::IntraFrameDetected()
{
  {
    PWaitAndSignal mutex(m_mutex);
    if (m_state != e_Requested)
      return;

    m_state = e_Throttled;
    PTRACE(4, "I-Frame detected: throttle=" << m_currentThrottleTime << " this=" << this);
  }

  m_requestTimer = m_currentThrottleTime; // Outside mutex
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

// This should really do the key frame detection from the codec plugin, but we cheat for now
struct OpalKeyFrameDetector
{
  template <class T> static OpalKeyFrameDetector * Create(PBYTEArray & context)
  {
#undef new  // Doing fancy "in-place" new operator so no malloc() done every time
    return new (context.GetPointer(sizeof(T))) T();
#define new PNEW
  }

  virtual ~OpalKeyFrameDetector()
  {
  }
  virtual OpalVideoFormat::VideoFrameType GetVideoFrameType(const BYTE * rtp, PINDEX size) = 0;
} *kfd = NULL;


struct OpalKeyFrameDetectorVP8 : OpalKeyFrameDetector
{
  virtual OpalVideoFormat::VideoFrameType GetVideoFrameType(const BYTE * rtp, PINDEX size)
  {
    if (size < 3)
      return OpalVideoFormat::e_NonFrameBoundary;

    PINDEX headerSize = 1;
    if ((rtp[0] & 0x80) != 0) { // Check X bit
      ++headerSize;           // Allow for X byte

      if ((rtp[1] & 0x80) != 0) { // Check I bit
        ++headerSize;           // Allow for I field
        if ((rtp[2] & 0x80) != 0) // > 7 bit picture ID
          ++headerSize;         // Allow for extra bits of I field
      }

      if ((rtp[1] & 0x40) != 0) // Check L bit
        ++headerSize;         // Allow for L byte

      if ((rtp[1] & 0x30) != 0) // Check T or K bit
        ++headerSize;         // Allow for T/K byte
    }

    if (size <= headerSize)
      return OpalVideoFormat::e_NonFrameBoundary;

    // Key frame is S bit == 1 && partition == 0 && P bit == 0
    if ((rtp[0]&0x1f) != 0x10)
      return OpalVideoFormat::e_NonFrameBoundary;

    return (rtp[headerSize] & 0x01) == 0 ? OpalVideoFormat::e_IntraFrame : OpalVideoFormat::e_InterFrame;
  }
};


struct OpalKeyFrameDetectorH264 : OpalKeyFrameDetector
{
  bool m_gotSPS;
  bool m_gotPPS;

  virtual OpalVideoFormat::VideoFrameType GetVideoFrameType(const BYTE * rtp, PINDEX size)
  {
    if (size > 2) {
      switch ((*rtp++) & 0x1f) {
        case 1: // Coded slice of a non-IDR picture
        case 2: // Coded slice data partition A
          if ((*rtp & 0x80) != 0) // High bit 1 indicates MB zero
            return OpalVideoFormat::e_InterFrame;
          break;

        case 5: // Coded slice of an IDR picture
          if (m_gotSPS && m_gotPPS && (*rtp & 0x80) != 0) // High bit 1 indicates MB zero
            return OpalVideoFormat::e_IntraFrame;
          break;

        case 7: // Sequence parameter set
          m_gotSPS = *rtp == 66 || *rtp == 77 || *rtp == 88 || *rtp == 100;
          return OpalVideoFormat::e_NonFrameBoundary;

        case 8: // Picture parameter set
          m_gotPPS = true;
          return OpalVideoFormat::e_NonFrameBoundary;

        case 28: // Fragment
          if ((*rtp & 0x80) != 0)
            return GetVideoFrameType(rtp, size - 1);
      }
    }

    return OpalVideoFormat::e_NonFrameBoundary;
  }
};


struct OpalKeyFrameDetectorMPEG4 : OpalKeyFrameDetector
{
  virtual OpalVideoFormat::VideoFrameType GetVideoFrameType(const BYTE * rtp, PINDEX size)
  {
    if (size < 4 || rtp[0] != 0 || rtp[1] != 0 || rtp[2] != 1)
      return OpalVideoFormat::e_UnknownFrameType;

    while (size > 4) {
      if (rtp[0] == 0 && rtp[1] == 0 && rtp[2] == 1) {
        if (rtp[3] == 0xb6) {
          switch ((rtp[4] & 0xC0) >> 6) {
            case 0:
              return OpalVideoFormat::e_IntraFrame;
            case 1:
              return OpalVideoFormat::e_InterFrame;
          }
        }
      }
      ++rtp;
      --size;
    }

    return OpalVideoFormat::e_NonFrameBoundary;
  }
};


struct OpalKeyFrameDetectorH263 : OpalKeyFrameDetector
{
  virtual OpalVideoFormat::VideoFrameType GetVideoFrameType(const BYTE * rtp, PINDEX size)
  {
    if (size < 8)
      return OpalVideoFormat::e_UnknownFrameType;

    if ((rtp[4] & 0x1c) != 0x1c)
      return (rtp[4] & 2) != 0 ? OpalVideoFormat::e_InterFrame : OpalVideoFormat::e_IntraFrame;

    switch (((rtp[5] & 0x80) != 0 ? (rtp[7] >> 2) : (rtp[5] >> 5)) & 3) {
      case 0:
      case 4:
        return OpalVideoFormat::e_IntraFrame;
      case 1:
      case 5:
        return OpalVideoFormat::e_InterFrame;
    }
    return OpalVideoFormat::e_NonFrameBoundary;
  }
};


struct OpalKeyFrameDetectorRFC2190 : OpalKeyFrameDetectorH263
{
  virtual OpalVideoFormat::VideoFrameType GetVideoFrameType(const BYTE * rtp, PINDEX size)
  {
    // RFC 2190 header length
    static const PINDEX ModeLen[4] = { 4, 4, 8, 12 };
    PINDEX len = ModeLen[(rtp[0] & 0xC0) >> 6];
    if (size < len + 6)
      return OpalVideoFormat::e_UnknownFrameType;

    rtp += len;
    if (rtp[0] != 0 || rtp[1] != 0 || (rtp[2] & 0xfc) != 0x80)
      return OpalVideoFormat::e_NonFrameBoundary;

    return OpalKeyFrameDetectorH263::GetVideoFrameType(rtp, size);
  }
};


struct OpalKeyFrameDetectorRFC4629 : OpalKeyFrameDetectorH263
{
  virtual OpalVideoFormat::VideoFrameType GetVideoFrameType(const BYTE * rtp, PINDEX size)
  {
    if (size < 6)
      return OpalVideoFormat::e_UnknownFrameType;

    if ((rtp[0] & 0xfd) != 4 || rtp[1] != 0 || (rtp[2] & 0xfc) != 0x80)
      return OpalVideoFormat::e_NonFrameBoundary;

    return OpalKeyFrameDetectorH263::GetVideoFrameType(rtp, size);
  }
};


OpalVideoFormat::VideoFrameType OpalVideoTranscoder::GetVideoFrameType(const PCaselessString & rtpEncodingName,
                                                                       const BYTE * payloadPtr,
                                                                       PINDEX payloadSize,
                                                                       PBYTEArray & context)
{
  if (!context.IsEmpty())
    kfd = reinterpret_cast<OpalKeyFrameDetector *>(context.GetPointer());
  else if (rtpEncodingName == "VP8")
    kfd = OpalKeyFrameDetector::Create<OpalKeyFrameDetectorVP8>(context);
  else if (rtpEncodingName == "H264")
    kfd = OpalKeyFrameDetector::Create<OpalKeyFrameDetectorH264>(context);
  else if (rtpEncodingName == "MP4V-ES")
    kfd = OpalKeyFrameDetector::Create<OpalKeyFrameDetectorMPEG4>(context);
  else if (rtpEncodingName == "H263")
    kfd = OpalKeyFrameDetector::Create<OpalKeyFrameDetectorRFC2190>(context);
  else if (rtpEncodingName == "H263-1998")
    kfd = OpalKeyFrameDetector::Create<OpalKeyFrameDetectorRFC4629>(context);

  return kfd != NULL ? kfd->GetVideoFrameType(payloadPtr, payloadSize) : OpalVideoFormat::e_UnknownFrameType;
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
