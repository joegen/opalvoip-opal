/*
 * h281.h
 *
 * H.281 PDU implementation for the OpenH323 Project.
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
 */

#ifndef OPAL_H224_H281_H
#define OPAL_H224_H281_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif

#include <opal_config.h>

#if OPAL_HAS_H281

#include <h224/h224.h>

class H281_Frame : public H224_Frame
{
  PCLASSINFO(H281_Frame, H224_Frame);

public:

  enum RequestType {
    IllegalRequest      = 0x00,
    StartAction         = 0x01,
    ContinueAction      = 0x02,
    StopAction          = 0x03,
    SelectVideoSource   = 0x04,
    VideoSourceSwitched = 0x05,
    StoreAsPreset       = 0x06,
    ActivatePreset      = 0x07
  };

  enum VideoMode {
    MotionVideo                 = 0x00,
    IllegalVideoMode            = 0x01,
    NormalResolutionStillImage  = 0x02,
    DoubleResolutionStillImage  = 0x03
  };

  H281_Frame();
  ~H281_Frame();

  RequestType GetRequestType() const { return (RequestType)(GetClientDataPtr())[0]; }
  void SetRequestType(RequestType requestType);

  // The following methods are only valid when
  // request type is either StartAction, ContinueAction or StopAction
  int GetDirection(PVideoControlInfo::Types type) const;
  void SetDirection(PVideoControlInfo::Types type, int direction);

  // Only valid when RequestType is StartAction
  BYTE GetTimeout() const;
  void SetTimeout(BYTE timeout);

  // Only valid when RequestType is SelectVideoSource or VideoSourceSwitched
  BYTE GetVideoSourceNumber() const;
  void SetVideoSourceNumber(BYTE videoSourceNumber);

  VideoMode GetVideoMode() const;
  void SetVideoMode(VideoMode videoMode);

  // Only valid when RequestType is StoreAsPreset or ActivatePreset
  BYTE GetPresetNumber() const;
  void SetPresetNumber(BYTE presetNumber);
};


extern const OpalMediaFormat & GetOpalFECC_RTP();
extern const OpalMediaFormat & GetOpalFECC_HDLC();

#define OpalFECC_RTP  GetOpalFECC_RTP()
#define OpalFECC_HDLC GetOpalFECC_HDLC()


#endif // OPAL_HAS_H281

#endif // OPAL_H224_H281_H
