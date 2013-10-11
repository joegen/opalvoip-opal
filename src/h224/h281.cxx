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
 * $Revision$
 * $Author$
 * $Date$
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


///////////////////////////////////////////////////////////////////////////////

const OpalMediaFormat & GetOpalFECC_RTP()
{
  static OpalH224MediaFormat h224_rtp(new OpalH224MediaFormatInternal(OPAL_FECC_RTP, "Far End Camera Control (H.224 over RTP)", false));
  return h224_rtp;
};


const OpalMediaFormat & GetOpalFECC_HDLC()
{
  static OpalH224MediaFormat h224_hdlc(new OpalH224MediaFormatInternal(OPAL_FECC_HDLC, "Far End Camera Control (H.224 over HDLC)", true));
  return h224_hdlc;
}


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

OpalH281Client::OpalH281Client()
  : m_localSource(NumVideoSources)
  , m_localNumberOfPresets(0)
  , m_remoteSource(MainCamera)
  , m_remoteNumberOfPresets(0)
{
  memset(m_localCapability, 0, sizeof(m_remoteCapability));
  memset(m_remoteCapability, 0, sizeof(m_remoteCapability));

  m_transmitFrame.SetClient(*this);
  m_transmitFrame.SetRequestType(H281_Frame::IllegalRequest);
  m_transmitFrame.SetBS(true);
  m_transmitFrame.SetES(true);

  m_continueTimer.SetNotifier(PCREATE_NOTIFIER(ContinueAction));
  m_receiveTimer.SetNotifier(PCREATE_NOTIFIER(StopActionLocally));
}


OpalH281Client::~OpalH281Client()
{
  m_continueTimer.Stop();
  m_receiveTimer.Stop();
}


bool OpalH281Client::Action(PVideoControlInfo::Types actionType, int direction)
{
  SendStopAction();

  m_transmitFrame.SetRequestType(H281_Frame::StartAction);
  m_transmitFrame.SetDirection(actionType, direction);

  for (PVideoControlInfo::Types type = PVideoControlInfo::BeginTypes; type < PVideoControlInfo::EndTypes; ++type) {
    if (m_transmitFrame.GetDirection(type) != 0) {
      PTRACE(3, "Starting action for " << type << ", dir=" << direction);

      m_transmitFrame.SetTimeout(0); //800msec

      m_h224Handler->TransmitClientFrame(*this, m_transmitFrame);

      // send a ContinueAction every 400msec
      m_continueTimer.RunContinuous(400);
      break;
    }
  }

  m_transmitFrame.SetRequestType(H281_Frame::IllegalRequest);
  return true;
}


void OpalH281Client::SendStopAction()
{
  if (m_transmitFrame.GetRequestType() == H281_Frame::IllegalRequest)
    return;

  PTRACE(3, "Stopping action");
  m_transmitFrame.SetRequestType(H281_Frame::StopAction);
  m_h224Handler->TransmitClientFrame(*this, m_transmitFrame);
  m_transmitFrame.SetRequestType(H281_Frame::IllegalRequest);
  m_continueTimer.Stop();
}


bool OpalH281Client::SelectVideoSource(VideoSources source, H281_Frame::VideoMode videoMode)
{
  PWaitAndSignal m(m_mutex);

  switch (videoMode) {
    case H281_Frame::MotionVideo :
      if ((m_remoteCapability[m_remoteSource]&H281_Frame::CanMotionVideo) == 0) {
        PTRACE(3, "Cannot do MotionVideo on remote");
        return false;
      }
      break;

    case H281_Frame::NormalResolutionStillImage :
      if ((m_remoteCapability[m_remoteSource]&H281_Frame::CanNormalResolutionStillImage) == 0) {
        PTRACE(3, "Cannot do NormalResolutionStillImage on remote");
        return false;
      }
      break;

    case H281_Frame::DoubleResolutionStillImage :
      if ((m_remoteCapability[m_remoteSource]&H281_Frame::CanDoubleResolutionStillImage) == 0) {
        PTRACE(3, "Cannot do DoubleResolutionStillImage on remote");
        return false;
      }
      break;

    default :
      PTRACE(2, "Illegal mode for selecting remote source");
      return false;
  }

  PTRACE(3, "Selected source " << source);

  SendStopAction();

  m_transmitFrame.SetRequestType(H281_Frame::SelectVideoSource);
  m_transmitFrame.SetVideoSourceNumber((BYTE)source);
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


void OpalH281Client::SendExtraCapabilities() const
{
  BYTE capabilities[11];

  // The default implementation has no presets
  capabilities[0] = 0x00;

  PINDEX size = 1;

  for (VideoSources source = MainCamera; source < NumVideoSources; source++) {
    if (m_localCapability[source] != 0) {
      PTRACE(4, "Sending local capability: 0x" << hex << m_localCapability[source]);
      capabilities[size++] = (BYTE)((m_localCapability[source] >> 8) | (source << 4));
      capabilities[size++] = (BYTE)  m_localCapability[source];
    }
  }

  m_h224Handler->SendExtraCapabilitiesMessage(*this, capabilities, size);
}


void OpalH281Client::OnReceivedExtraCapabilities(const BYTE *capabilities, PINDEX size)
{
  memset(m_remoteCapability, 0, sizeof(m_remoteCapability));
  m_remoteNumberOfPresets = (capabilities[0] & 0x0f);

  PINDEX i = 1;

  while (i < size) {
    unsigned videoSource = (capabilities[i] >> 4) & 0x0f;
    if (videoSource < NumVideoSources) {
      m_remoteCapability[videoSource] = *(PUInt16b*)&capabilities[i];
      PTRACE(4, "Remote camera capability for source " << videoSource
             << ": 0x" << hex << m_remoteCapability[videoSource]);
      i += 2;
    }
    else {
      // video sources from 6 to 15 are not supported but still need to be parsed
      do {
        i++;
      } while (capabilities[i] != 0);

      // scan past the pan/tilt/zoom/focus field
      i++;
    }
  }

  OnRemoteCapabilitiesChanged();
}


void OpalH281Client::OnReceivedMessage(const H224_Frame & h224Frame)
{
  const H281_Frame & message = (const H281_Frame &)h224Frame;
  H281_Frame::RequestType requestType = message.GetRequestType();

  switch (requestType) {
    case H281_Frame::StartAction :
      if (m_receiveTimer.IsRunning())
        OnStopAction(); // an action is already running and thus is stopped

      int directions[PVideoControlInfo::NumTypes];
      for (PVideoControlInfo::Types type = PVideoControlInfo::BeginTypes; type < PVideoControlInfo::EndTypes; ++type)
        directions[type] = message.GetDirection(type);
      OnStartAction(directions);
      // Do continue action case, restart timer;

    case H281_Frame::ContinueAction :
      m_receiveTimer = 800; // timeout is always 800 msec
      break;

    case H281_Frame::StopAction :
      OnStopAction();
      break;

    case H281_Frame::SelectVideoSource :
      m_localSource = VideoSourcesFromInt(message.GetVideoSourceNumber());
      OnSelectVideoSource(m_localSource, message.GetVideoMode());
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
    m_capabilityChanged(*this, m_remoteSource);
}


void OpalH281Client::OnSelectVideoSource(VideoSources /*source*/, H281_Frame::VideoMode /*videoMode*/)
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

  m_transmitFrame.SetRequestType(H281_Frame::IllegalRequest);
}


void OpalH281Client::StopActionLocally(PTimer &, P_INT_PTR)
{
  OnStopAction();
}


///////////////////////////////////////////////////////////////////////////////

OpalFarEndCameraControl::OpalFarEndCameraControl()
  : m_stepRate(100)
{
  memset(m_videoInputDevices, 0, sizeof(m_videoInputDevices));
  memset(m_step, 0, sizeof(m_step));

  m_stepTimer.SetNotifier(PCREATE_NOTIFIER(StepCamera));
}


void OpalFarEndCameraControl::Attach(PVideoInputDevice * device, VideoSources source)
{
  if (m_videoInputDevices[source] == device)
    return;

  m_videoInputDevices[source] = device;
  m_localCapability[source] = 0;

  if (device != NULL) {
    PTRACE(3, "Attaching " << device->GetDeviceName());

    if (m_localSource == NumVideoSources)
      m_localSource = source;

    PVideoInputDevice::Capabilities caps;
    if (m_videoInputDevices[source]->GetDeviceCapabilities(&caps)) {
      m_localCapability[source] = H281_Frame::CanMotionVideo;
      for (std::list<PVideoControlInfo>::iterator it = caps.controls.begin(); it != caps.controls.end(); ++it) {
        if (it->IsValid()) {
          switch (it->GetType()) {
            case PVideoControlInfo::Pan :
              m_localCapability[source] |= H281_Frame::CanPan;
              break;
            case PVideoControlInfo::Tilt :
              m_localCapability[source] |= H281_Frame::CanTilt;
              break;
            case PVideoControlInfo::Zoom :
              m_localCapability[source] |= H281_Frame::CanZoom;
              break;
            case PVideoControlInfo::Focus :
              m_localCapability[source] |= H281_Frame::CanFocus;
              break;
          }
        }
      }
    }
  }

  SendExtraCapabilities();
}


void OpalFarEndCameraControl::Detach(PVideoInputDevice * device)
{
  if (device == NULL)
    return;

  PTRACE(3, "Detaching " << device->GetDeviceName());

  for (VideoSources source = BeginVideoSources; source < EndVideoSources; ++source) {
    if (m_videoInputDevices[source] == device) {
      m_videoInputDevices[source] = NULL;
      m_localCapability[source] = 0;
      SendExtraCapabilities();

      for (m_localSource = BeginVideoSources; m_localSource < EndVideoSources; ++m_localSource) {
        if (m_videoInputDevices[m_localSource] != NULL) {
          PTRACE(3, "Switched to source " << m_localSource);
          break;
        }
      }
    }
  }
}


bool OpalFarEndCameraControl::SelectVideoDevice(PVideoInputDevice * device, H281_Frame::VideoMode mode)
{
  PWaitAndSignal m(m_mutex);

  for (VideoSources source = BeginVideoSources; source < EndVideoSources; ++source) {
    if (m_videoInputDevices[source] == device)
      return SelectVideoSource(source, mode);
  }

  PTRACE(3, "Video device " << device->GetDeviceName() << " not attached.");
  return false;
}


void OpalFarEndCameraControl::OnStartAction(int directions[PVideoControlInfo::NumTypes])
{
  PWaitAndSignal m(m_mutex);

  if (m_videoInputDevices[m_localSource] == NULL) {
    PTRACE(3, "Stepping unattached source " << m_localSource);
    return;
  }

  for (PVideoControlInfo::Types type = PVideoControlInfo::BeginTypes; type < PVideoControlInfo::EndTypes; ++type)
    m_step[type] = directions[type] * m_videoInputDevices[m_localSource]->GetControlInfo(type).GetStep();

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

  if (m_videoInputDevices[m_localSource] == NULL) {
    PTRACE(3, "Stepping unattached source " << m_localSource);
    return;
  }

  for (PVideoControlInfo::Types type = PVideoControlInfo::BeginTypes; type < PVideoControlInfo::EndTypes; ++type) {
    if (m_step[type] != 0)
      m_videoInputDevices[m_localSource]->SetControl(type, m_step[type], PVideoInputDevice::RelativeControl);
  }
}


#endif // OPAL_HAS_H281
