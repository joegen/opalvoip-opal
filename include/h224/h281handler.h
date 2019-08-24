/*
 * h281handler.h
 *
 * H.281 protocol handler implementation for the OpenH323 Project.
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

#ifndef OPAL_H224_H281HANDLER_H
#define OPAL_H224_H281HANDLER_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif

#ifndef _PTLIB_H
#include <ptlib.h>
#endif

#include <opal_config.h>

#if OPAL_HAS_H281

#include <h224/h224handler.h>
#include <h224/h281.h>

class OpalH224Handler;

/** This class implements a default H.281 handler
 */
class OpalH281Client : public OpalH224Client
{
    PCLASSINFO(OpalH281Client, OpalH224Client);
  public:
    OpalH281Client();
    ~OpalH281Client();

    P_DECLARE_ENUM(VideoSourceIds,
      CurrentVideoSource,
      MainCameraId,
      AuxiliaryCameraId,
      DocumentCameraId,
      AuxiliaryDocumentCameraId,
      VideoPlaybackSourceId,
      UserDefinedSourceId1,
      UserDefinedSourceId2,
      UserDefinedSourceId3,
      UserDefinedSourceId4,
      UserDefinedSourceId5,
      UserDefinedSourceId6,
      UserDefinedSourceId7,
      UserDefinedSourceId8,
      UserDefinedSourceId9,
      UserDefinedSourceId10
    );

    // Predefined source names
    static const PConstString & MainCamera();
    static const PConstString & AuxiliaryCamera();
    static const PConstString & DocumentCamera();
    static const PConstString & AuxiliaryDocumentCamera();
    static const PConstString & VideoPlayback();

    /**Overriding default OpalH224Client methods */
    virtual BYTE GetClientID() const { return OpalH224Client::H281ClientID; }
    virtual bool HasExtraCapabilities() const { return true; }

    /**Process incoming frames. Overrides from OpalH224Client */
    virtual void OnReceivedExtraCapabilities(const BYTE *capabilities, PINDEX size);
    virtual void OnReceivedMessage(const H224_Frame & message);

    // Presets
    unsigned GetLocalNumberOfPresets() const { return m_localNumberOfPresets; }
    void SetLocalNumberOfPresets(unsigned presets) { m_localNumberOfPresets = presets; }

    unsigned GetRemoteNumberOfPresets() const { return m_remoteNumberOfPresets; }

    /** Causes the H.281 handler to start the desired action
        if \p duration os zero, then the action continues until an Action with
        \p direction of zero is executed.
     */
    bool Action(PVideoControlInfo::Types type, int direction, const PTimeInterval & duration);

    /** Tells the remote side to select the desired video source using the
      mode specified. Does nothing if either video source or mode aren't
      available
    */
    bool SelectVideoSource(
      const PString & source,
      H281_Frame::VideoMode mode = H281_Frame::MotionVideo
    );

    /** Tells the remote side to store the current camera settings as a preset
      with the preset number given
     */
    void StoreAsPreset(BYTE presetNumber);

    /** Tells the remote side to activate the given preset
     */
    void ActivatePreset(BYTE presetNumber);

    /** Causes the H.281 handler to send its capabilities.
      Capabilities include the number of available cameras, (default one)
      the camera abilities (default none) and the number of presets that
        can be stored (default zero)
     */
    void SendExtraCapabilities() const;

    /*
     * methods that subclasses can override.
     * The default handler does not implement FECC on the local side.
     * Thus, the default behaviour is to do nothing.
     */

    /** Called each time a remote endpoint sends its capability list
     */
    virtual void OnRemoteCapabilitiesChanged();

    /** Indicates to start the action specified
     */
    virtual void OnStartAction(int directions[PVideoControlInfo::NumTypes]) = 0;

    /** Indicates to stop the action stared with OnStartAction()
     */
    virtual void OnStopAction() = 0;

    /** Indicates to select the desired video source
     */
    virtual void OnSelectVideoSource(const PString & source, H281_Frame::VideoMode videoMode);

    /** Indicates to store the current camera settings as a preset
     */
    virtual void OnStoreAsPreset(BYTE presetNumber);

    /** Indicates to activate the given preset number
     */
    virtual void OnActivatePreset(BYTE presetNumber);

    /**Set notifier for when remote camera capabilties change
      */
    void SetCapabilityChangedNotifier(const PNotifier & notifier);

    /**Set notifier for when remote system sends action to local side.
       The notifier PINT_PTR parameter is actually a pointer to an array of
       integers, PVideoControlInfo::NumTypes long, with -1, 0 or 1 in them.
       A NULL pointer indicates that the action has stopped for all types.
      */
    void SetOnActionNotifier(const PNotifier & notifier);

    struct Capability
    {
      Capability();
      PINDEX Encode(VideoSourceIds sourceId, BYTE * capabilities, PINDEX offset) const;
      PINDEX Decode(VideoSourceIds sourceId, const BYTE * capabilities, PINDEX offset);

      PString m_name;
      bool    m_available;

      enum Attributes
      {
        // First four are from PVideoControlInfo::Types
        MotionVideo = PVideoControlInfo::NumTypes,
        NormalResolutionStillImage,
        DoubleResolutionStillImage,
        NumAttributes
      };
      bool m_attribute[NumAttributes];

#if PTRACING
      friend ostream & operator<<(ostream & strm, const Capability & cap);
#endif
    };

    /** Set local video source capability
    */
    void SetLocalCapability(
      VideoSourceIds sourceId,   ///< From 1 to NumVideoSources
      const Capability & capability
    ) { m_localCapability[sourceId == CurrentVideoSource ? m_localSourceId : sourceId] = capability; }

    /** Get remote video source capability.
    */
    const Capability & GetRemoteCapability(
      VideoSourceIds sourceId   ///< From 1 to NumVideoSources
    ) const { return m_remoteCapability[sourceId == CurrentVideoSource ? m_remoteSourceId : sourceId]; }

  protected:
    PDECLARE_NOTIFIER(PTimer, OpalH281Client, ContinueAction);
    PDECLARE_NOTIFIER(PTimer, OpalH281Client, StopAction);
    PDECLARE_NOTIFIER(PTimer, OpalH281Client, ReceiveActionTimeout);
    void SendStopAction();

    PDECLARE_MUTEX(m_mutex);

    PNotifier      m_capabilityChanged;
    PNotifier      m_onAction;

    VideoSourceIds m_localSourceId;
    Capability     m_localCapability[NumVideoSourceIds];
    unsigned       m_localNumberOfPresets;
    PTimer         m_receiveTimer;

    VideoSourceIds m_remoteSourceId;
    Capability     m_remoteCapability[NumVideoSourceIds];
    unsigned       m_remoteNumberOfPresets;

    H281_Frame     m_transmitFrame;
    PTimer         m_continueTimer;
    PTimer         m_stopTimer;
};


/** This class implements a H.281 handler for PVideoInputDevice
 */
class OpalFarEndCameraControl : public OpalH281Client
{
    PCLASSINFO(OpalFarEndCameraControl, OpalH281Client);
  public:
    OpalFarEndCameraControl();

    /// Attach an active video input device to be controlled
    void Attach(
      PVideoInputDevice * device,
      const PString & source = MainCamera()
    );
    void Detach(
      PVideoInputDevice * device
    );
    bool SelectVideoDevice(
      PVideoInputDevice * device,
      H281_Frame::VideoMode mode = H281_Frame::MotionVideo
    );

    virtual void OnStartAction(int directions[PVideoControlInfo::NumTypes]);
    virtual void OnStopAction();

  protected:
    PDECLARE_NOTIFIER(PTimer, OpalFarEndCameraControl, StepCamera);

    PTimeInterval       m_stepRate;
    PVideoInputDevice * m_videoInputDevices[NumVideoSourceIds];
    int                 m_step[PVideoControlInfo::NumTypes];
    PTimer              m_stepTimer;
};


#endif // OPAL_HAS_H281

#endif // OPAL_H224_H281HANDLER_H
