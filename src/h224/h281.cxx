/*
 * h281.h
 *
 * H.281 implementation for the OpenH323 Project.
 *
 * Copyright (c) 2006 Network for Educational Technology, ETH Zurich.
 * Written by Hannes Friederich.
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
 * Contributor(s): ______________________________________.
 *
 */

#include <ptlib.h>

#ifdef __GNUC__
#pragma implementation "h281.h"
#pragma implementation "h281handler.h"
#endif

#include <h224/h281.h>

#if OPAL_HAS_H281

#include <h224/h281handler.h>
#include <h224/h224handler.h>

#define MAX_H281_DATA_SIZE

#define PTraceModule() "H.281"


static BYTE CapabilityAttributeBits[OpalH281Client::Capability::NumAttributes] = { 0x80, 0x40, 0x20, 0x10, 0x4, 0x2, 0x1 };


///////////////////////////////////////////////////////////////////////////////

const OpalMediaFormat & GetOpalFECC_RTP()
{
  static OpalMediaFormatStatic<OpalH224MediaFormat> h224_rtp(new OpalH224MediaFormat::Internal(OPAL_FECC_RTP, "Far End Camera Control (H.224 over RTP)", false));
  return h224_rtp;
};


const OpalMediaFormat & GetOpalFECC_HDLC()
{
  static OpalMediaFormatStatic<OpalH224MediaFormat> h224_hdlc(new OpalH224MediaFormat::Internal(OPAL_FECC_HDLC, "Far End Camera Control (H.224 over HDLC)", true));
  return h224_hdlc;
}


///////////////////////////////////////////////////////////////////////////////

OpalH281Client::Capability::Capability()
  : m_available(false)
{
  memset(m_attribute, 0, sizeof(m_attribute));
}


#if PTRACING
ostream & operator<<(ostream & strm, const OpalH281Client::Capability & cap)
{
  strm << '"' << cap.m_name << '"';
  if (cap.m_available) {
    bool nothing = true;
    for (PINDEX i = 0; i < PARRAYSIZE(cap.m_attribute); ++i) {
      if (cap.m_attribute[i]) {
        nothing = false;

        if (i < PVideoControlInfo::NumTypes)
          strm << ' ' << PVideoControlInfo::TypesFromInt(i);
        else {
          switch (i) {
            case OpalH281Client::Capability::MotionVideo:
              strm << " Motion";
              break;
            case OpalH281Client::Capability::NormalResolutionStillImage:
              strm << " Normal-Still";
              break;
            case OpalH281Client::Capability::DoubleResolutionStillImage:
              strm << " Double-Still";
              break;
          }
        }
      }
    }

    if (nothing)
      strm << " (no-caps)";
  }
  else
    strm << " (unavailable)";

  return strm;
}
#endif

///////////////////////////////////////////////////////////////////////////////

H281_Frame::H281_Frame()
: H224_Frame(3)
{
  SetHighPriority(true);

  BYTE *data = GetClientDataPtr();

  // Setting RequestType to StartAction
  SetRequestType(StartAction);

  // Setting Pan / Tilt / Zoom and Focus Off
  // Setting timeout to zero
  data[1] = 0x00;
  data[2] = 0x00;
}


H281_Frame::~H281_Frame()
{
}


void H281_Frame::SetRequestType(RequestType requestType)
{
  BYTE *data = GetClientDataPtr();

  data[0] = (BYTE)requestType;

  switch (requestType) {

    case StartAction:
      SetClientDataSize(3);
      break;
    default:
      SetClientDataSize(2);
      break;
  }
}


static int const ControlBitShifts[PVideoControlInfo::NumTypes] = { 6, 4, 2, 0 };
static int const ControlBitMask = 3;
static int const ControlMinus = 2;
static int const ControlPlus = 3;

int H281_Frame::GetDirection(PVideoControlInfo::Types type) const
{
  RequestType requestType = GetRequestType();

  if (requestType != StartAction &&
      requestType != ContinueAction &&
      requestType != StopAction) {
    // not valid
    return 0;
  }

  BYTE *data = GetClientDataPtr();

  switch ((data[1] >> ControlBitShifts[type]) & ControlBitMask) {
    case ControlMinus :
      return -1;
    case ControlPlus :
      return 1;
    default :
      return 0;
  }
}


void H281_Frame::SetDirection(PVideoControlInfo::Types type, int direction)
{
  RequestType requestType = GetRequestType();

  if (requestType != StartAction &&
      requestType != ContinueAction &&
      requestType != StopAction) {
    // not valid
    return;
  }

  BYTE *data = GetClientDataPtr();

  data[1] &= ~ControlBitMask << ControlBitShifts[type];
  if (direction < 0)
    data[1] |= ControlMinus << ControlBitShifts[type];
  else if (direction > 0)
    data[1] |= ControlPlus << ControlBitShifts[type];
}


BYTE H281_Frame::GetTimeout() const
{
  RequestType requestType = GetRequestType();

  if (requestType != StartAction) {
    return 0x00;
  }

  BYTE *data = GetClientDataPtr();

  return (data[2] & 0x0f);
}


void H281_Frame::SetTimeout(BYTE timeout)
{
  RequestType requestType = GetRequestType();

  if (requestType != StartAction) {
    return;
  }

  BYTE *data = GetClientDataPtr();

  data[2] = (timeout & 0x0f);
}


BYTE H281_Frame::GetVideoSourceNumber() const
{
  RequestType requestType = GetRequestType();

  if (requestType != SelectVideoSource &&
      requestType != VideoSourceSwitched) {
    return 0x00;
  }

  BYTE *data = GetClientDataPtr();

  return (data[1] >> 4) & 0x0f;
}


void H281_Frame::SetVideoSourceNumber(BYTE videoSourceNumber)
{
  RequestType requestType = GetRequestType();

  if (requestType != SelectVideoSource &&
      requestType != VideoSourceSwitched) {
    return;
  }

  BYTE *data = GetClientDataPtr();

  data[1] &= 0x0f;
  data[1] |= (videoSourceNumber << 4) & 0xf0;
}

H281_Frame::VideoMode H281_Frame::GetVideoMode() const
{
  RequestType requestType = GetRequestType();

  if (requestType != SelectVideoSource &&
      requestType != VideoSourceSwitched) {
    return IllegalVideoMode;
  }

  BYTE *data = GetClientDataPtr();

  return (VideoMode)(data[1] & 0x03);
}


void H281_Frame::SetVideoMode(VideoMode mode)
{
  RequestType requestType = GetRequestType();

  if (requestType != SelectVideoSource &&
      requestType != VideoSourceSwitched) {
    return;
  }

  BYTE *data = GetClientDataPtr();

  data[1] &= 0xfc;
  data[1] |= (mode & 0x03);
}


BYTE H281_Frame::GetPresetNumber() const
{
  RequestType requestType = GetRequestType();

  if (requestType != StoreAsPreset &&
      requestType != ActivatePreset) {
    return 0x00;
  }

  BYTE *data = GetClientDataPtr();

  return (data[1] >> 4) & 0x0f;
}


void H281_Frame::SetPresetNumber(BYTE presetNumber)
{
  RequestType requestType = GetRequestType();

  if (requestType != StoreAsPreset &&
      requestType != ActivatePreset) {
    return;
  }

  BYTE *data = GetClientDataPtr();

  data[1] &= 0x0f;
  data[1] |= (presetNumber << 4) & 0xf0;
}


///////////////////////////////////////////////////////////////////////////////

const PConstString & OpalH281Client::MainCamera()              { static PConstString const s("Main Camera");      return s; }
const PConstString & OpalH281Client::AuxiliaryCamera()         { static PConstString const s("Auxiliary Camera"); return s; }
const PConstString & OpalH281Client::DocumentCamera()          { static PConstString const s("Document Camera");  return s; }
const PConstString & OpalH281Client::AuxiliaryDocumentCamera() { static PConstString const s("Aux Doc Camera");   return s; }
const PConstString & OpalH281Client::VideoPlayback()           { static PConstString const s("Video Playback");   return s; }

OpalH281Client::OpalH281Client()
  : m_localSourceId(NumVideoSourceIds)
  , m_remoteSourceId(NumVideoSourceIds)
{
  // Predefined names
  m_localCapability[MainCameraId].m_name = m_remoteCapability[MainCameraId].m_name = MainCamera();
  m_localCapability[AuxiliaryCameraId].m_name = m_remoteCapability[AuxiliaryCameraId].m_name = AuxiliaryCamera();
  m_localCapability[DocumentCameraId].m_name = m_remoteCapability[DocumentCameraId].m_name = DocumentCamera();
  m_localCapability[AuxiliaryDocumentCameraId].m_name = m_remoteCapability[AuxiliaryDocumentCameraId].m_name = AuxiliaryDocumentCamera();
  m_localCapability[VideoPlaybackSourceId].m_name = m_remoteCapability[VideoPlaybackSourceId].m_name = VideoPlayback();

  m_transmitFrame.SetClient(*this);
  m_transmitFrame.SetRequestType(H281_Frame::IllegalRequest);
  m_transmitFrame.SetBS(true);
  m_transmitFrame.SetES(true);

  m_continueTimer.SetNotifier(PCREATE_NOTIFIER(ContinueAction), "H281Continue");
  m_stopTimer.SetNotifier(PCREATE_NOTIFIER(StopAction), "H281Stop");
  m_receiveTimer.SetNotifier(PCREATE_NOTIFIER(ReceiveActionTimeout), "H281Receive");
}


OpalH281Client::~OpalH281Client()
{
  m_continueTimer.Stop();
  m_receiveTimer.Stop();
}


bool OpalH281Client::Action(PVideoControlInfo::Types actionType, int direction, const PTimeInterval & duration)
{
  if (m_remoteSourceId == NumVideoSourceIds) {
    PTRACE(3, "Remote source not yet set.");
    return false;
  }

  if (!m_remoteCapability[m_remoteSourceId].m_available) {
    PTRACE(3, "Remote source not available: " << m_remoteCapability[m_remoteSourceId]);
    return false;
  }

  SendStopAction();

  if (direction == 0)
    return true;

  PTRACE(3, "Starting action for " << actionType << ", dir=" << direction);

  m_transmitFrame.SetRequestType(H281_Frame::StartAction);
  m_transmitFrame.SetDirection(actionType, direction);
  m_transmitFrame.SetTimeout(0); //800msec

  m_h224Handler->TransmitClientFrame(*this, m_transmitFrame);

  // send a ContinueAction every 400msec
  m_continueTimer.RunContinuous(400);
  m_stopTimer = duration;
  return true;
}


void OpalH281Client::SendStopAction()
{
  if (m_transmitFrame.GetRequestType() == H281_Frame::IllegalRequest)
    return;

  PTRACE(3, "Stopping action");
  m_continueTimer.Stop();
  m_transmitFrame.SetRequestType(H281_Frame::StopAction);
  for (PVideoControlInfo::Types type = PVideoControlInfo::BeginTypes; type < PVideoControlInfo::EndTypes; ++type)
    m_transmitFrame.SetDirection(type, 0);
  m_h224Handler->TransmitClientFrame(*this, m_transmitFrame);
  m_transmitFrame.SetRequestType(H281_Frame::IllegalRequest);
}


bool OpalH281Client::SelectVideoSource(const PString & source, H281_Frame::VideoMode videoMode)
{
  PWaitAndSignal m(m_mutex);

  VideoSourceIds sourceId = MainCameraId;
  while (m_remoteCapability[sourceId].m_name != source) {
    if (++sourceId > NumVideoSourceIds) {
      PTRACE(2, "Unknown remote video source \"" << source << '"');
      return false;
    }
  }

  if (!m_remoteCapability[sourceId].m_available) {
    PTRACE(2, "Remote video source " << m_remoteCapability[sourceId]);
    return false;
  }

  switch (videoMode) {
    case H281_Frame::MotionVideo :
      if (!m_remoteCapability[sourceId].m_attribute[Capability::MotionVideo]) {
        PTRACE(2, "Cannot do MotionVideo on " << m_remoteCapability[sourceId]);
        return false;
      }
      break;

    case H281_Frame::NormalResolutionStillImage :
      if (!m_remoteCapability[sourceId].m_attribute[Capability::NormalResolutionStillImage]) {
        PTRACE(2, "Cannot do NormalResolutionStillImage on " << m_remoteCapability[sourceId]);
        return false;
      }
      break;

    case H281_Frame::DoubleResolutionStillImage :
      if (!m_remoteCapability[sourceId].m_attribute[Capability::DoubleResolutionStillImage]) {
        PTRACE(2, "Cannot do DoubleResolutionStillImage on " << m_remoteCapability[sourceId]);
        return false;
      }
      break;

    default :
      PTRACE(2, "Illegal mode for selecting remote video source \"" << source << '"');
      return false;
  }

  m_remoteSourceId = sourceId;
  PTRACE(3, "Selecting source id=" << sourceId << ' ' << m_remoteCapability[sourceId]);

  SendStopAction();

  m_transmitFrame.SetRequestType(H281_Frame::SelectVideoSource);
  m_transmitFrame.SetVideoSourceNumber((BYTE)sourceId);
  m_transmitFrame.SetVideoMode(videoMode);

  m_h224Handler->TransmitClientFrame(*this, m_transmitFrame);

  m_transmitFrame.SetRequestType(H281_Frame::IllegalRequest);
  return true;
}


void OpalH281Client::StoreAsPreset(BYTE presetNumber)
{
  PWaitAndSignal m(m_mutex);

  SendStopAction();

  m_transmitFrame.SetRequestType(H281_Frame::StoreAsPreset);
  m_transmitFrame.SetPresetNumber(presetNumber);

  m_h224Handler->TransmitClientFrame(*this, m_transmitFrame);

  m_transmitFrame.SetRequestType(H281_Frame::IllegalRequest);
}


void OpalH281Client::ActivatePreset(BYTE presetNumber)
{
  PWaitAndSignal m(m_mutex);

  SendStopAction();

  m_transmitFrame.SetRequestType(H281_Frame::ActivatePreset);
  m_transmitFrame.SetPresetNumber(presetNumber);

  m_h224Handler->TransmitClientFrame(*this, m_transmitFrame);

  m_transmitFrame.SetRequestType(H281_Frame::IllegalRequest);
}


PINDEX OpalH281Client::Capability::Encode(VideoSourceIds sourceId, BYTE * capabilities, PINDEX offset) const
{
  if (!m_available || m_name.IsEmpty())
    return offset;

  capabilities[offset] = (BYTE)(sourceId << 4);

  if (m_attribute[MotionVideo])
    capabilities[offset] |= CapabilityAttributeBits[MotionVideo];
  if (m_attribute[NormalResolutionStillImage])
    capabilities[offset] |= CapabilityAttributeBits[NormalResolutionStillImage];
  if (m_attribute[DoubleResolutionStillImage])
    capabilities[offset] |= CapabilityAttributeBits[DoubleResolutionStillImage];

  ++offset;

  if (sourceId >= UserDefinedSourceId1) {
    PINDEX len = m_name.GetLength() + 1; // Include '\0'
    memcpy(capabilities + offset, m_name.GetPointer(), len);
    offset += len;
  }

  capabilities[offset] = 0;
  for (PVideoControlInfo::Types type = PVideoControlInfo::BeginTypes; type < PVideoControlInfo::EndTypes; ++type) {
    if (m_attribute[type])
      capabilities[offset] |= CapabilityAttributeBits[type];
  }

  return offset + 1;
}


void OpalH281Client::SendExtraCapabilities() const
{
  BYTE capabilities[NumVideoSourceIds*17];

  // The default implementation has no presets
  capabilities[0] = 0x00;

  PINDEX size = 1;

  for (VideoSourceIds sourceId = MainCameraId; sourceId < NumVideoSourceIds; sourceId++) {
    PTRACE_IF(4, m_localCapability[sourceId].m_available,
              "Sending local capability: id=" << sourceId << ' ' << m_localCapability[sourceId]);
    size = m_localCapability[sourceId].Encode(sourceId, capabilities, size);
  }

  m_h224Handler->SendExtraCapabilitiesMessage(*this, capabilities, size);
}


PINDEX OpalH281Client::Capability::Decode(VideoSourceIds sourceId, const BYTE * capabilities, PINDEX offset)
{
  m_available = true;

  m_attribute[MotionVideo]                = (capabilities[offset] & CapabilityAttributeBits[MotionVideo               ]) != 0;
  m_attribute[NormalResolutionStillImage] = (capabilities[offset] & CapabilityAttributeBits[NormalResolutionStillImage]) != 0;
  m_attribute[DoubleResolutionStillImage] = (capabilities[offset] & CapabilityAttributeBits[DoubleResolutionStillImage]) != 0;

  ++offset;

  if (sourceId >= UserDefinedSourceId1) {
    m_name.MakeEmpty();
    while (capabilities[offset] != '\0')
      m_name += (char)capabilities[offset++];
    if (m_name.IsEmpty())
      m_name.sprintf("User Camera %u", sourceId);
    ++offset;
  }

  for (PVideoControlInfo::Types type = PVideoControlInfo::BeginTypes; type < PVideoControlInfo::EndTypes; ++type)
    m_attribute[type] = (capabilities[offset] & CapabilityAttributeBits[type]) != 0;

  return offset + 1;
}


void OpalH281Client::OnReceivedExtraCapabilities(const BYTE *capabilities, PINDEX size)
{
  m_remoteNumberOfPresets = (capabilities[0] & 0x0f);

  PINDEX pos = 1;
  while (pos < size) {
    VideoSourceIds sourceId = VideoSourceIdsFromInt((capabilities[pos] >> 4) & 0x0f);
    if (sourceId == CurrentVideoSource)
      sourceId = m_remoteSourceId == NumVideoSourceIds ? MainCameraId : m_remoteSourceId; // Zero shouldn't happen, but just in case
    pos = m_remoteCapability[sourceId].Decode(sourceId, capabilities, pos);
    if (m_remoteSourceId == NumVideoSourceIds && m_remoteCapability[sourceId].m_available)
      m_remoteSourceId = sourceId;
    PTRACE(4, "Received remote camera capability: id=" << sourceId << ' ' << m_remoteCapability[sourceId]);
  }

  if (!m_remoteCapability[m_remoteSourceId].m_available) {
    for (m_remoteSourceId = MainCameraId; m_remoteSourceId < NumVideoSourceIds; ++m_remoteSourceId) {
      if (m_remoteCapability[m_remoteSourceId].m_available)
        break;
    }
  }

  OnRemoteCapabilitiesChanged();
}


void OpalH281Client::OnReceivedMessage(const H224_Frame & h224Frame)
{
  const H281_Frame & message = (const H281_Frame &)h224Frame;
  H281_Frame::RequestType requestType = message.GetRequestType();
  PTRACE(4, "Received message: type=" << requestType);

  switch (requestType) {
    case H281_Frame::StartAction :
      int directions[PVideoControlInfo::NumTypes];
      for (PVideoControlInfo::Types type = PVideoControlInfo::BeginTypes; type < PVideoControlInfo::EndTypes; ++type)
        directions[type] = message.GetDirection(type);
      if (m_onAction.IsNULL())
        OnStartAction(directions);
      else
        m_onAction(*this, (P_INT_PTR)directions);
      // Do continue action case, restart timer;

    case H281_Frame::ContinueAction :
      m_receiveTimer = 800; // timeout is always 800 msec
      break;

    case H281_Frame::StopAction :
      m_receiveTimer.Stop();
      if (m_onAction.IsNULL())
        OnStopAction();
      else
        m_onAction(*this, 0);
      break;

    case H281_Frame::SelectVideoSource :
      if (message.GetVideoSourceNumber() > 0)
        m_localSourceId = VideoSourceIdsFromInt(message.GetVideoSourceNumber());
      OnSelectVideoSource(m_localCapability[m_localSourceId].m_name, message.GetVideoMode());
      break;

    case H281_Frame::StoreAsPreset :
      OnStoreAsPreset(message.GetPresetNumber());
      break;

    case H281_Frame::ActivatePreset :
      OnActivatePreset(message.GetPresetNumber());
      break;

    default :
      PTRACE(2, "Received unknown tequest: " << requestType);
  }
}


void OpalH281Client::OnRemoteCapabilitiesChanged()
{
  if (!m_capabilityChanged.IsNULL())
    m_capabilityChanged(*this, m_remoteSourceId);
}


void OpalH281Client::SetCapabilityChangedNotifier(const PNotifier & notifier)
{
  m_capabilityChanged = notifier;
  OnRemoteCapabilitiesChanged();
}


void OpalH281Client::SetOnActionNotifier(const PNotifier & notifier)
{
  m_onAction = notifier;
}


void OpalH281Client::OnSelectVideoSource(const PString & /*source*/, H281_Frame::VideoMode /*videoMode*/)
{
  // not handled
}


void OpalH281Client::OnStoreAsPreset(BYTE /*presetNumber*/)
{
  // not handled
}


void OpalH281Client::OnActivatePreset(BYTE /*presetNumber*/)
{
  // not handled
}


void OpalH281Client::ContinueAction(PTimer &, P_INT_PTR)
{
  PWaitAndSignal m(m_mutex);

  PTRACE(4, "Continue action");
  m_transmitFrame.SetRequestType(H281_Frame::ContinueAction);
  m_h224Handler->TransmitClientFrame(*this, m_transmitFrame);
}


void OpalH281Client::StopAction(PTimer &, P_INT_PTR)
{
  SendStopAction();
}


void OpalH281Client::ReceiveActionTimeout(PTimer &, P_INT_PTR)
{
  PTRACE(4, "Action timeout");

  // Never got explicit stop action, so timeout does it
  if (m_onAction.IsNULL())
    OnStopAction();
  else
    m_onAction(*this, 0);
}


///////////////////////////////////////////////////////////////////////////////

OpalFarEndCameraControl::OpalFarEndCameraControl()
  : m_stepRate(100)
{
  memset(m_videoInputDevices, 0, sizeof(m_videoInputDevices));
  memset(m_step, 0, sizeof(m_step));

  m_stepTimer.SetNotifier(PCREATE_NOTIFIER(StepCamera), "FECCStep");
}


void OpalFarEndCameraControl::Attach(PVideoInputDevice * device, const PString & source)
{
  VideoSourceIds sourceId = MainCameraId;
  while (sourceId < NumVideoSourceIds && m_localCapability[sourceId].m_name != source)
    ++sourceId;

  if (sourceId < NumVideoSourceIds && m_videoInputDevices[sourceId] == device)
    return;

  if (sourceId >= NumVideoSourceIds) {
    sourceId = UserDefinedSourceId1;
    while (!m_localCapability[sourceId].m_name.IsEmpty()) {
      if (++sourceId >= NumVideoSourceIds) {
        PTRACE(2, "No more video sources available");
        return;
      }
    }
    m_localCapability[sourceId].m_name = source;
  }

  m_videoInputDevices[sourceId] = device;
  m_localCapability[sourceId].m_available = device != NULL;

  if (device != NULL) {
    PTRACE(3, "Attaching " << device->GetDeviceName());

    if (m_localSourceId == NumVideoSourceIds)
      m_localSourceId = sourceId;

    PVideoInputDevice::Capabilities caps;
    if (m_videoInputDevices[sourceId]->GetDeviceCapabilities(&caps)) {
      m_localCapability[sourceId].m_attribute[Capability::MotionVideo] = true;
      for (std::list<PVideoControlInfo>::iterator it = caps.m_controls.begin(); it != caps.m_controls.end(); ++it)
        m_localCapability[sourceId].m_attribute[it->GetType()] = it->IsValid();
    }
  }

  SendExtraCapabilities();
}


void OpalFarEndCameraControl::Detach(PVideoInputDevice * device)
{
  if (device == NULL)
    return;

  PTRACE(3, "Detaching " << device->GetDeviceName());

  for (VideoSourceIds sourceId = MainCameraId; sourceId < NumVideoSourceIds; ++sourceId) {
    if (m_videoInputDevices[sourceId] == device) {
      m_videoInputDevices[sourceId] = NULL;
      m_localCapability[sourceId].m_available = false;
      SendExtraCapabilities();

      for (m_localSourceId = MainCameraId; m_localSourceId < NumVideoSourceIds; ++m_localSourceId) {
        if (m_videoInputDevices[m_localSourceId] != NULL && m_localCapability[m_localSourceId].m_available) {
          PTRACE(3, "Switched to source id=" << m_localSourceId << ' ' << m_localCapability[m_localSourceId]);
          break;
        }
      }
    }
  }
}


bool OpalFarEndCameraControl::SelectVideoDevice(PVideoInputDevice * device, H281_Frame::VideoMode mode)
{
  PWaitAndSignal m(m_mutex);

  for (VideoSourceIds sourceId = MainCameraId; sourceId < NumVideoSourceIds; ++sourceId) {
    if (m_videoInputDevices[sourceId] == device)
      return SelectVideoSource(m_remoteCapability[sourceId].m_name, mode);
  }

  PTRACE(3, "Video device " << device->GetDeviceName() << " not attached.");
  return false;
}


void OpalFarEndCameraControl::OnStartAction(int directions[PVideoControlInfo::NumTypes])
{
  PWaitAndSignal m(m_mutex);

  if (m_videoInputDevices[m_localSourceId] == NULL) {
    PTRACE(3, "Stepping unattached source " << m_localSourceId);
    return;
  }

  for (PVideoControlInfo::Types type = PVideoControlInfo::BeginTypes; type < PVideoControlInfo::EndTypes; ++type)
    m_step[type] = directions[type] * m_videoInputDevices[m_localSourceId]->GetControlInfo(type).GetStep();

#if PTRACING
  if (PTrace::CanTrace(3)) {
    ostream & trace = PTRACE_BEGIN(3);
    trace << "Starting camera timer:";
    for (PVideoControlInfo::Types type = PVideoControlInfo::BeginTypes; type < PVideoControlInfo::EndTypes; ++type)
      trace << ' ' << type << '=' << m_step[type];
    trace << PTrace::End;
  }
#endif

  m_stepTimer.RunContinuous(m_stepRate);
}


void OpalFarEndCameraControl::OnStopAction()
{
  PTRACE(3, "Stopping");
  PWaitAndSignal m(m_mutex);

  m_stepTimer.Stop();
  memset(m_step, 0, sizeof(m_step));
}


void OpalFarEndCameraControl::StepCamera(PTimer &, P_INT_PTR)
{
  PWaitAndSignal m(m_mutex);

  if (m_videoInputDevices[m_localSourceId] == NULL) {
    PTRACE(3, "Stepping unattached source " << m_localSourceId);
    return;
  }

  for (PVideoControlInfo::Types type = PVideoControlInfo::BeginTypes; type < PVideoControlInfo::EndTypes; ++type) {
    if (m_step[type] != 0)
      m_videoInputDevices[m_localSourceId]->SetControl(type, m_step[type], PVideoInputDevice::RelativeControl);
  }
}


#endif // OPAL_HAS_H281
