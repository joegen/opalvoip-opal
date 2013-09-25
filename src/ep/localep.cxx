/*
 * localep.cxx
 *
 * Local EndPoint/Connection.
 *
 * Open Phone Abstraction Library (OPAL)
 *
 * Copyright (c) 2008 Vox Lucida Pty. Ltd.
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
#pragma implementation "localep.h"
#endif

#include <opal_config.h>

#include <ep/localep.h>
#include <opal/call.h>
#include <im/rfc4103.h>
#include <h224/h224handler.h>
#include <h224/h281handler.h>


#define new PNEW


/////////////////////////////////////////////////////////////////////////////

OpalLocalEndPoint::OpalLocalEndPoint(OpalManager & mgr, const char * prefix, bool useCallbacks)
  : OpalEndPoint(mgr, prefix, CanTerminateCall)
  , m_deferredAlerting(false)
  , m_deferredAnswer(false)
  , m_defaultAudioSynchronicity(e_Synchronous)
  , m_defaultVideoSourceSynchronicity(e_Synchronous)
{
  PTRACE(3, "LocalEP\tCreated endpoint.");

  if (useCallbacks) {
    OpalMediaTypeList mediaTypes = OpalMediaType::GetList();
    for (OpalMediaTypeList::iterator it = mediaTypes.begin(); it != mediaTypes.end(); ++it)
      m_useCallback[*it] = UseSourceCallback|UseSinkCallback;
  }
}


OpalLocalEndPoint::~OpalLocalEndPoint()
{
  PTRACE(4, "LocalEP\tDeleted endpoint.");
}


OpalMediaFormatList OpalLocalEndPoint::GetMediaFormats() const
{
  return manager.GetCommonMediaFormats(false, true);
}


PSafePtr<OpalConnection> OpalLocalEndPoint::MakeConnection(OpalCall & call,
                                                      const PString & /*remoteParty*/,
                                                               void * userData,
                                                         unsigned int options,
                                      OpalConnection::StringOptions * stringOptions)
{
  return AddConnection(CreateConnection(call, userData, options, stringOptions));
}


OpalLocalConnection * OpalLocalEndPoint::CreateConnection(OpalCall & call,
                                                              void * userData,
                                                            unsigned options,
                                     OpalConnection::StringOptions * stringOptions)
{
  return new OpalLocalConnection(call, *this, userData, options, stringOptions);
}


bool OpalLocalEndPoint::OnOutgoingSetUp(const OpalLocalConnection & /*connection*/)
{
  return true;
}


bool OpalLocalEndPoint::OnOutgoingCall(const OpalLocalConnection & /*connection*/)
{
  return true;
}


bool OpalLocalEndPoint::OnIncomingCall(OpalLocalConnection & connection)
{
  if (!m_deferredAnswer)
    connection.AcceptIncoming();
  return true;
}


bool OpalLocalEndPoint::AlertingIncomingCall(const PString & token,
                                             OpalConnection::StringOptions * options,
                                             bool withMedia)
{
  PSafePtr<OpalLocalConnection> connection = GetLocalConnectionWithLock(token, PSafeReadOnly);
  if (connection == NULL) {
    PTRACE(2, "LocalEP\tCould not find connection using token \"" << token << '"');
    return false;
  }

  if (options != NULL)
    connection->SetStringOptions(*options, false);

  connection->AlertingIncoming(withMedia);
  return true;
}


bool OpalLocalEndPoint::AcceptIncomingCall(const PString & token, OpalConnection::StringOptions * options)
{
  PSafePtr<OpalLocalConnection> connection = GetLocalConnectionWithLock(token, PSafeReadOnly);
  if (connection == NULL) {
    PTRACE(2, "LocalEP\tCould not find connection using token \"" << token << '"');
    return false;
  }

  if (options != NULL)
    connection->SetStringOptions(*options, false);

  connection->AcceptIncoming();
  return true;
}


bool OpalLocalEndPoint::RejectIncomingCall(const PString & token, const OpalConnection::CallEndReason & reason)
{
  PSafePtr<OpalLocalConnection> connection = GetLocalConnectionWithLock(token, PSafeReadOnly);
  if (connection == NULL) {
    PTRACE(2, "LocalEP\tCould not find connection using token \"" << token << '"');
    return false;
  }

  PTRACE(3, "LocalEP\tRejecting incoming call with reason " << reason);
  connection->Release(reason);
  return true;
}


bool OpalLocalEndPoint::OnUserInput(const OpalLocalConnection &, const PString &)
{
  return true;
}


bool OpalLocalEndPoint::UseCallback(const OpalMediaFormat & mediaFormat, bool isSource) const
{
  OpalLocalEndPoint::CallbackMap::const_iterator it = m_useCallback.find(mediaFormat.GetMediaType());
  return it != m_useCallback.end() && (it->second & (isSource ? UseSourceCallback : UseSinkCallback));
}


bool OpalLocalEndPoint::SetCallbackUsage(const OpalMediaType & mediaType, CallbackUsage usage)
{
  if (mediaType.empty())
    return false;

  m_useCallback[mediaType] |= usage;
  return true;
}


bool OpalLocalEndPoint::OnReadMediaFrame(const OpalLocalConnection & /*connection*/,
                                         const OpalMediaStream & /*mediaStream*/,
                                         RTP_DataFrame & /*frame*/)
{
  return false;
}


bool OpalLocalEndPoint::OnWriteMediaFrame(const OpalLocalConnection & /*connection*/,
                                          const OpalMediaStream & /*mediaStream*/,
                                          RTP_DataFrame & /*frame*/)
{
  return false;
}


bool OpalLocalEndPoint::OnReadMediaData(const OpalLocalConnection & /*connection*/,
                                        const OpalMediaStream & /*mediaStream*/,
                                        void * data,
                                        PINDEX size,
                                        PINDEX & length)
{
  memset(data, 0, size);
  length = size;
  return true;
}


bool OpalLocalEndPoint::OnWriteMediaData(const OpalLocalConnection & /*connection*/,
                                         const OpalMediaStream & /*mediaStream*/,
                                         const void * /*data*/,
                                         PINDEX length,
                                         PINDEX & written)
{
  written = length;
  return true;
}


#if OPAL_VIDEO

bool OpalLocalEndPoint::CreateVideoInputDevice(const OpalConnection & connection,
                                               const OpalMediaFormat & mediaFormat,
                                               PVideoInputDevice * & device,
                                               bool & autoDelete)
{
  return manager.CreateVideoInputDevice(connection, mediaFormat, device, autoDelete);
}


bool OpalLocalEndPoint::CreateVideoOutputDevice(const OpalConnection & connection,
                                                const OpalMediaFormat & mediaFormat,
                                                bool preview,
                                                PVideoOutputDevice * & device,
                                                bool & autoDelete)
{
  return manager.CreateVideoOutputDevice(connection, mediaFormat, preview, device, autoDelete);
}

#endif // OPAL_VIDEO


OpalLocalEndPoint::Synchronicity
OpalLocalEndPoint::GetSynchronicity(const OpalMediaFormat & mediaFormat,
                                    bool isSource) const
{
  if (mediaFormat.GetMediaType() == OpalMediaType::Audio())
    return m_defaultAudioSynchronicity;

  if (isSource && mediaFormat.GetMediaType() == OpalMediaType::Audio())
    return m_defaultVideoSourceSynchronicity;

  return e_Asynchronous;
}


/////////////////////////////////////////////////////////////////////////////

OpalLocalConnection::OpalLocalConnection(OpalCall & call,
                                OpalLocalEndPoint & ep,
                                             void * userData,
                                           unsigned options,
                    OpalConnection::StringOptions * stringOptions,
                                               char tokenPrefix)
  : OpalConnection(call, ep, ep.GetManager().GetNextToken(tokenPrefix), options, stringOptions)
  , m_endpoint(ep)
  , m_userData(userData)
#if OPAL_HAS_H224
  , m_h224Handler(new OpalH224Handler)
#endif
{
#if OPAL_PTLIB_DTMF
  m_sendInBandDTMF = m_detectInBandDTMF = false;
#endif

#if OPAL_HAS_H281
  m_farEndCameraControl = new OpalFarEndCameraControl;
  m_h224Handler->AddClient(*m_farEndCameraControl);
#endif

  PTRACE(4, "LocalCon\tCreated connection with token \"" << callToken << '"');
}


OpalLocalConnection::~OpalLocalConnection()
{
#if OPAL_HAS_H281
  delete m_farEndCameraControl;
#endif
#if OPAL_HAS_H224
  delete m_h224Handler;
#endif
  PTRACE(4, "LocalCon\tDeleted connection.");
}


void OpalLocalConnection::OnApplyStringOptions()
{
  OpalConnection::OnApplyStringOptions();

  PSafePtr<OpalConnection> otherConnection = GetOtherPartyConnection();
  if (otherConnection != NULL && dynamic_cast<OpalLocalConnection*>(&*otherConnection) == NULL) {
    PTRACE(4, "LocalCon\tPassing string options to " << *otherConnection);
    otherConnection->SetStringOptions(m_stringOptions, false);
  }
}


PBoolean OpalLocalConnection::OnIncomingConnection(unsigned int options, OpalConnection::StringOptions * stringOptions)
{
  if (!OpalConnection::OnIncomingConnection(options, stringOptions))
    return false;

  if (OnOutgoingSetUp())
    return true;

  PTRACE(4, "LocalCon\tOnOutgoingSetUp returned false on " << *this);
  Release(EndedByNoAccept);
  return false;
}


PBoolean OpalLocalConnection::SetUpConnection()
{
  bool notIncoming = ownerCall.GetConnection(0) == this || ownerCall.IsEstablished();

  if (!OpalConnection::SetUpConnection())
    return false;

  if (notIncoming)
    return true;

  if (!OnIncoming()) {
    PTRACE(4, "LocalCon\tOnIncoming returned false on " << *this);
    Release(EndedByLocalBusy);
    return false;
  }

  if (!m_endpoint.IsDeferredAlerting())
    AlertingIncoming();

  return true;
}


PBoolean OpalLocalConnection::SetAlerting(const PString & calleeName, PBoolean)
{
  PTRACE(3, "LocalCon\tSetAlerting(" << calleeName << ')');
  SetPhase(AlertingPhase);
  remotePartyName = calleeName;
  return m_endpoint.OnOutgoingCall(*this);
}


PBoolean OpalLocalConnection::SetConnected()
{
  PTRACE(3, "LocalCon\tSetConnected()");

  if (GetMediaStream(PString::Empty(), true) == NULL)
    AutoStartMediaStreams(); // if no media streams, try and start them

  return OpalConnection::SetConnected();
}

OpalMediaStream * OpalLocalConnection::CreateMediaStream(const OpalMediaFormat & mediaFormat,
                                                         unsigned sessionID,
                                                         PBoolean isSource)
{
  if (m_endpoint.UseCallback(mediaFormat, isSource))
    return new OpalLocalMediaStream(*this, mediaFormat, sessionID, isSource,
                                    m_endpoint.GetSynchronicity(mediaFormat, isSource));

#if OPAL_VIDEO
  if (mediaFormat.GetMediaType() == OpalMediaType::Video()) {
    if (isSource) {
      PVideoInputDevice * videoDevice;
      PBoolean autoDeleteGrabber;
      if (CreateVideoInputDevice(mediaFormat, videoDevice, autoDeleteGrabber)) {
        PTRACE(4, "OpalCon\tCreated capture device \"" << videoDevice->GetDeviceName() << '"');

        PVideoOutputDevice * previewDevice;
        PBoolean autoDeletePreview;
        if (CreateVideoOutputDevice(mediaFormat, true, previewDevice, autoDeletePreview))
          PTRACE(4, "OpalCon\tCreated preview device \"" << previewDevice->GetDeviceName() << '"');
        else
          previewDevice = NULL;

        return new OpalVideoMediaStream(*this, mediaFormat, sessionID, videoDevice, previewDevice, autoDeleteGrabber, autoDeletePreview);
      }
      PTRACE(2, "OpalCon\tCould not create video input device");
    }
    else {
      PVideoOutputDevice * videoDevice;
      PBoolean autoDelete;
      if (CreateVideoOutputDevice(mediaFormat, false, videoDevice, autoDelete)) {
        PTRACE(4, "OpalCon\tCreated display device \"" << videoDevice->GetDeviceName() << '"');
        return new OpalVideoMediaStream(*this, mediaFormat, sessionID, NULL, videoDevice, false, autoDelete);
      }
      PTRACE(2, "OpalCon\tCould not create video output device");
    }

    return NULL;
  }
#endif // OPAL_VIDEO

#if OPAL_HAS_H224
  if (mediaFormat.GetMediaType() == OpalH224MediaType())
    return new OpalH224MediaStream(*this, *m_h224Handler, mediaFormat, sessionID, isSource);
#endif

#if OPAL_HAS_RFC4103
  if (mediaFormat.GetMediaType() == OpalT140.GetMediaType())
    return new OpalT140MediaStream(*this, mediaFormat, sessionID, isSource);
#endif

  return OpalConnection::CreateMediaStream(mediaFormat, sessionID, isSource);
}


OpalMediaStreamPtr OpalLocalConnection::OpenMediaStream(const OpalMediaFormat & mediaFormat, unsigned sessionID, bool isSource)
{
#if OPAL_VIDEO
  if ( isSource &&
       mediaFormat.GetMediaType() == OpalMediaType::Video() &&
      !ownerCall.IsEstablished() &&
      !endpoint.GetManager().CanAutoStartTransmitVideo()) {
    PTRACE(3, "LocalCon\tOpenMediaStream auto start disabled, refusing video open");
    return NULL;
  }
#endif

  return OpalConnection::OpenMediaStream(mediaFormat, sessionID, isSource);
}


void OpalLocalConnection::OnClosedMediaStream(const OpalMediaStream & stream)
{
#if OPAL_HAS_H281
  const OpalVideoMediaStream * video = dynamic_cast<const OpalVideoMediaStream *>(&stream);
  if (video != NULL)
    m_farEndCameraControl->Detach(video->GetVideoInputDevice());
#endif

  OpalConnection::OnClosedMediaStream(stream);
}


bool OpalLocalConnection::SendUserInputString(const PString & value)
{
  PTRACE(3, "LocalCon\tSendUserInputString(" << value << ')');
  return m_endpoint.OnUserInput(*this, value);
}


bool OpalLocalConnection::OnOutgoingSetUp()
{
  return m_endpoint.OnOutgoingSetUp(*this);
}


bool OpalLocalConnection::OnOutgoing()
{
  return m_endpoint.OnOutgoingCall(*this);
}


bool OpalLocalConnection::OnIncoming()
{
  return m_endpoint.OnIncomingCall(*this);
}


void OpalLocalConnection::AlertingIncoming(bool withMedia)
{
  if (LockReadWrite()) {
    if (GetPhase() < AlertingPhase) {
      SetPhase(AlertingPhase);

      if (withMedia) {
        PSafePtr<OpalConnection> conn = GetOtherPartyConnection();
        if (conn != NULL)
          conn->AutoStartMediaStreams();
        AutoStartMediaStreams();
      }

      OnAlerting();
    }
    UnlockReadWrite();
  }
}


void OpalLocalConnection::AcceptIncoming()
{
  GetEndPoint().GetManager().QueueDecoupledEvent(
        new PSafeWorkNoArg<OpalLocalConnection>(this, &OpalLocalConnection::InternalAcceptIncoming));
}


void OpalLocalConnection::InternalAcceptIncoming()
{
  PThread::Sleep(100);

  if (LockReadWrite()) {
    AlertingIncoming();
    OnConnectedInternal();
    AutoStartMediaStreams();
    UnlockReadWrite();
  }
}


#if OPAL_VIDEO

bool OpalLocalConnection::CreateVideoInputDevice(const OpalMediaFormat & mediaFormat,
                                                 PVideoInputDevice * & device,
                                                 bool & autoDelete)
{
  if (!m_endpoint.CreateVideoInputDevice(*this, mediaFormat, device, autoDelete))
    return false;

#if OPAL_HAS_H281
  m_farEndCameraControl->Attach(device);
#endif
  return true;
}


bool OpalLocalConnection::CreateVideoOutputDevice(const OpalMediaFormat & mediaFormat,
                                                  bool preview,
                                                  PVideoOutputDevice * & device,
                                                  bool & autoDelete)
{
  return m_endpoint.CreateVideoOutputDevice(*this, mediaFormat, preview, device, autoDelete);
}


bool OpalLocalConnection::ChangeVideoInputDevice(const PVideoDevice::OpenArgs & deviceArgs, unsigned sessionID)
{
  PSafePtr<OpalVideoMediaStream> stream = PSafePtrCast<OpalMediaStream, OpalVideoMediaStream>(
                      sessionID != 0 ? GetMediaStream(sessionID, true) : GetMediaStream(OpalMediaType::Video(), true));
  if (stream == NULL)
    return false;

  PVideoInputDevice * newDevice;
  bool autoDelete;
  if (!m_endpoint.GetManager().CreateVideoInputDevice(*this, deviceArgs, newDevice, autoDelete)) {
    PTRACE(2, "OpalCon\tCould not open video device \"" << deviceArgs.deviceName << '"');
    return false;
  }

  stream->SetVideoInputDevice(newDevice, autoDelete);
#if OPAL_HAS_H281
  m_farEndCameraControl->Attach(newDevice);
#endif
  return true;
}


bool OpalLocalConnection::ChangeVideoOutputDevice(const PVideoDevice::OpenArgs & deviceArgs, unsigned sessionID, bool preview)
{
  PSafePtr<OpalVideoMediaStream> stream = PSafePtrCast<OpalMediaStream, OpalVideoMediaStream>(
                      sessionID != 0 ? GetMediaStream(sessionID, true) : GetMediaStream(OpalMediaType::Video(), preview));
  if (stream == NULL)
    return false;

  PVideoOutputDevice * newDevice = PVideoOutputDevice::CreateOpenedDevice(deviceArgs, false);
  if (newDevice == NULL) {
    PTRACE(2, "OpalCon\tCould not open video device \"" << deviceArgs.deviceName << '"');
    return false;
  }

  stream->SetVideoOutputDevice(newDevice);
  return true;
}

#endif // OPAL_VIDEO


#if OPAL_HAS_H281
bool OpalLocalConnection::FarEndCameraControl(PVideoControlInfo::Types what, int direction)
{
  return m_farEndCameraControl->Action(what, direction);
}


void OpalLocalConnection::SetFarEndCameraCapabilityChangedNotifier(const PNotifier & notifier)
{
  m_farEndCameraControl->SetCapabilityChangedNotifier(notifier);
}
#endif // OPAL_HAS_H281


/////////////////////////////////////////////////////////////////////////////

OpalLocalMediaStream::OpalLocalMediaStream(OpalLocalConnection & connection,
                                           const OpalMediaFormat & mediaFormat,
                                           unsigned sessionID,
                                           bool isSource,
                                           OpalLocalEndPoint::Synchronicity synchronicity)
  : OpalMediaStream(connection, mediaFormat, sessionID, isSource)
  , OpalMediaStreamPacing(mediaFormat)
  , m_synchronicity(synchronicity)
{
}


PBoolean OpalLocalMediaStream::ReadPacket(RTP_DataFrame & frame)
{
  if (!IsOpen())
    return false;

  OpalLocalEndPoint & ep = dynamic_cast<OpalLocalEndPoint &>(connection.GetEndPoint());
  OpalLocalConnection & conn = dynamic_cast<OpalLocalConnection &>(connection);
  if (ep.OnReadMediaFrame(conn, *this, frame))
    return true;

  return OpalMediaStream::ReadPacket(frame);
}


PBoolean OpalLocalMediaStream::WritePacket(RTP_DataFrame & frame)
{
  if (!IsOpen())
    return false;

  OpalLocalEndPoint & ep = dynamic_cast<OpalLocalEndPoint &>(connection.GetEndPoint());
  OpalLocalConnection & conn = dynamic_cast<OpalLocalConnection &>(connection);
  if (ep.OnWriteMediaFrame(conn, *this, frame))
    return true;

  return OpalMediaStream::WritePacket(frame);
}


PBoolean OpalLocalMediaStream::ReadData(BYTE * data, PINDEX size, PINDEX & length)
{
  OpalLocalEndPoint & ep = dynamic_cast<OpalLocalEndPoint &>(connection.GetEndPoint());
  OpalLocalConnection & conn = dynamic_cast<OpalLocalConnection &>(connection);
  if (!ep.OnReadMediaData(conn, *this, data, size, length))
    return false;

  if (m_synchronicity == OpalLocalEndPoint::e_SimulateSyncronous)
    Pace(true, size, marker);
  return true;
}


PBoolean OpalLocalMediaStream::WriteData(const BYTE * data, PINDEX length, PINDEX & written)
{
  OpalLocalEndPoint & ep = dynamic_cast<OpalLocalEndPoint &>(connection.GetEndPoint());
  OpalLocalConnection & conn = dynamic_cast<OpalLocalConnection &>(connection);
  if (!ep.OnWriteMediaData(conn, *this, data, length, written))
    return false;

  if (m_synchronicity == OpalLocalEndPoint::e_SimulateSyncronous)
    Pace(false, written, marker);
  return true;
}


PBoolean OpalLocalMediaStream::IsSynchronous() const
{
  return m_synchronicity != OpalLocalEndPoint::e_Asynchronous;
}


/////////////////////////////////////////////////////////////////////////////
