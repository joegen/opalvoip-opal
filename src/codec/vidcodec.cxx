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
 * $Revision$
 * $Author$
 * $Date$
 */

#include <ptlib.h>

#ifdef __GNUC__
#pragma implementation "vidcodec.h"
#endif

#include <opal/buildopts.h>

#if OPAL_VIDEO

#include <codec/vidcodec.h>

#include <ptlib/videoio.h>
#include <ptlib/vconvert.h>

#define FRAME_WIDTH  PVideoDevice::MaxWidth
#define FRAME_HEIGHT PVideoDevice::MaxHeight
#define FRAME_RATE   30 // NTSC

const OpalVideoFormat & GetOpalRGB24()
{
  static const OpalVideoFormat RGB24(
    OPAL_RGB24,
    RTP_DataFrame::MaxPayloadType,
    NULL,
    FRAME_WIDTH, FRAME_HEIGHT,
    FRAME_RATE,
    24*FRAME_WIDTH*FRAME_HEIGHT*FRAME_RATE  // Bandwidth
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

const OpalVideoFormat & GetOpalYUV420P()
{
  static const OpalVideoFormat YUV420P(
    OPAL_YUV420P,
    RTP_DataFrame::MaxPayloadType,
    NULL,
    FRAME_WIDTH, FRAME_HEIGHT,
    FRAME_RATE,
    12*FRAME_WIDTH*FRAME_HEIGHT*FRAME_RATE  // Bandwidth
  );
  return YUV420P;
}


#define new PNEW

/////////////////////////////////////////////////////////////////////////////

OpalVideoTranscoder::OpalVideoTranscoder(const OpalMediaFormat & inputMediaFormat,
                                         const OpalMediaFormat & outputMediaFormat)
  : OpalTranscoder(inputMediaFormat, outputMediaFormat)
  , m_inDataSize(10*1024)
  , m_outDataSize(10*1024)
  , m_errorConcealment(false)
  , m_freezeTillIFrame(false)
  , m_frozenTillIFrame(false)
  , m_forceIFrame(false)
  , m_lastFrameWasIFrame(false)
  , m_totalFrames(0)
  , m_keyFrames(0)
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
    return PFalse;

  SetFrameBytes(inputMediaFormat,  OpalVideoFormat::MaxRxFrameWidthOption(), OpalVideoFormat::MaxRxFrameHeightOption(), m_inDataSize);
  SetFrameBytes(outputMediaFormat, OpalVideoFormat::FrameWidthOption(),      OpalVideoFormat::FrameHeightOption(),      m_outDataSize);

  if (outputMediaFormat.GetOptionInteger(OpalMediaFormat::MaxTxPacketSizeOption()) > maxOutputSize) {
    PTRACE(4, "Media\tReducing \"" << OpalMediaFormat::MaxTxPacketSizeOption() << "\" to " << maxOutputSize);
    outputMediaFormat.SetOptionInteger(OpalMediaFormat::MaxTxPacketSizeOption(), maxOutputSize);
  }

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

  if (m_forceIFrame) {
    PTRACE(5, "Media\tIgnoring forced I-Frame as already in progress for " << *this);
    return true;
  }

  if (m_throttleSendIFrameTimer.IsRunning()) {
    PTRACE(4, "Media\tIgnoring forced I-Frame request due to throttling for " << *this);
    return true;
  }

  PTimeInterval now = PTimer::Tick();
  PTimeInterval timeSinceLast = now - m_lastSentIFrame;
  m_lastSentIFrame = now;

  static PTimeInterval const MinThrottle(500);
  static PTimeInterval const MaxThrottle(0, 4);

  if (timeSinceLast < MinThrottle && m_throttleSendIFrameTimer < MaxThrottle)
    m_throttleSendIFrameTimer = m_throttleSendIFrameTimer * 2;
  else if (timeSinceLast > MaxThrottle && m_throttleSendIFrameTimer > MinThrottle)
    m_throttleSendIFrameTimer = m_throttleSendIFrameTimer / 2;
  else if (m_throttleSendIFrameTimer > MinThrottle)
    m_throttleSendIFrameTimer = m_throttleSendIFrameTimer;
  else
    m_throttleSendIFrameTimer = MinThrottle;

  PTRACE(3, "Media\tI-Frame forced (throttle " << m_throttleSendIFrameTimer << ") in video stream " << *this);
  m_forceIFrame = true; // Reset when I-Frame is sent
  return true;
}


PBoolean OpalVideoTranscoder::Convert(const RTP_DataFrame & /*input*/,
                                  RTP_DataFrame & /*output*/)
{
  return PFalse;
}


#if OPAL_STATISTICS
void OpalVideoTranscoder::GetStatistics(OpalMediaStatistics & statistics) const
{
  statistics.m_totalFrames = m_totalFrames;
  statistics.m_keyFrames   = m_keyFrames;
}
#endif


void OpalVideoTranscoder::SendIFrameRequest(unsigned sequenceNumber, unsigned timestamp)
{
  m_frozenTillIFrame = m_freezeTillIFrame;

  if (m_throttleRequestIFrameTimer.IsRunning()) {
    PTRACE(4, "Media\tI-Frame requested, but not sent due to throttling.");
    return;
  }

  if (sequenceNumber == 0 && timestamp == 0)
    NotifyCommand(OpalVideoUpdatePicture());
  else
    NotifyCommand(OpalVideoPictureLoss(sequenceNumber, timestamp));
  m_throttleRequestIFrameTimer.SetInterval(0, 1);
}


///////////////////////////////////////////////////////////////////////////////

PObject * OpalVideoUpdatePicture::Clone() const
{
  return new OpalVideoUpdatePicture(*this);
}


PString OpalVideoUpdatePicture::GetName() const
{
  return "Update Picture";
}


OpalVideoPictureLoss::OpalVideoPictureLoss(unsigned sequenceNumber, unsigned timestamp)
  : m_sequenceNumber(sequenceNumber)
  , m_timestamp(timestamp)
{
}


PObject * OpalVideoPictureLoss::Clone() const
{
  return new OpalVideoPictureLoss(*this);
}


PString OpalVideoPictureLoss::GetName() const
{
  return "Picture Loss";
}


PString OpalTemporalSpatialTradeOff::GetName() const
{
  return "Temporal Spatial Trade Off";
}


#endif // OPAL_VIDEO

/////////////////////////////////////////////////////////////////////////////
