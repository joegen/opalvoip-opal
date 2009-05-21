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

#include <opal/buildopts.h>

#include <opal/localep.h>
#include <opal/call.h>
#include <opal/patch.h>
#include <codec/echocancel.h>


#define new PNEW


/////////////////////////////////////////////////////////////////////////////

OpalLocalEndPoint::OpalLocalEndPoint(OpalManager & mgr, const char * prefix)
  : OpalEndPoint(mgr, prefix, CanTerminateCall)
  , m_deferredAlerting(false)
{
  PTRACE(3, "LocalEP\tCreated endpoint.\n");
}


OpalLocalEndPoint::~OpalLocalEndPoint()
{
  PTRACE(4, "LocalEP\tDeleted endpoint.");
}


OpalMediaFormatList OpalLocalEndPoint::GetMediaFormats() const
{
  OpalMediaFormatList list = manager.GetCommonMediaFormats(false, true);
  list += OpalL16_STEREO_48KHZ;
  return list;
}


PBoolean OpalLocalEndPoint::MakeConnection(OpalCall & call,
                                      const PString & /*remoteParty*/,
                                               void * userData,
                                       unsigned int   /*options*/,
                      OpalConnection::StringOptions * /* stringOptions*/)
{
  return AddConnection(CreateConnection(call, userData));
}


OpalLocalConnection * OpalLocalEndPoint::CreateConnection(OpalCall & call, void * userData)
{
  return new OpalLocalConnection(call, *this, userData);
}


bool OpalLocalEndPoint::OnOutgoingCall(const OpalLocalConnection & /*connection*/)
{
  return true;
}


bool OpalLocalEndPoint::OnIncomingCall(OpalLocalConnection & connection)
{
  connection.AcceptIncoming();
  return true;
}


bool OpalLocalEndPoint::AlertingIncomingCall(const PString & token)
{
  PSafePtr<OpalLocalConnection> connection = GetLocalConnectionWithLock(token, PSafeReadOnly);
  if (connection == NULL) {
    PTRACE(2, "LocalEP\tCould not find connection using token \"" << token << '"');
    return false;
  }

  connection->AlertingIncoming();
  return true;
}


bool OpalLocalEndPoint::AcceptIncomingCall(const PString & token)
{
  PSafePtr<OpalLocalConnection> connection = GetLocalConnectionWithLock(token, PSafeReadOnly);
  if (connection == NULL) {
    PTRACE(2, "LocalEP\tCould not find connection using token \"" << token << '"');
    return false;
  }

  connection->AcceptIncoming();
  return true;
}


bool OpalLocalEndPoint::RejectIncomingCall(const PString & token)
{
  PSafePtr<OpalLocalConnection> connection = GetLocalConnectionWithLock(token, PSafeReadOnly);
  if (connection == NULL)
    return false;

  connection->Release(OpalConnection::EndedByAnswerDenied);
  return true;
}


bool OpalLocalEndPoint::OnUserInput(const OpalLocalConnection &, const PString &)
{
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


bool OpalLocalEndPoint::IsSynchronous() const
{
  return true;
}


/////////////////////////////////////////////////////////////////////////////

static unsigned LastConnectionTokenID;

OpalLocalConnection::OpalLocalConnection(OpalCall & call,
                                OpalLocalEndPoint & ep,
                                             void * /*userData*/,
                                           unsigned options,
                    OpalConnection::StringOptions * stringOptions)
  : OpalConnection(call, ep, psprintf("%u", ++LastConnectionTokenID), options, stringOptions)
  , endpoint(ep), userData(NULL)
{
  PTRACE(4, "LocalCon\tCreated connection with token \"" << callToken << '"');
}


OpalLocalConnection::~OpalLocalConnection()
{
  PTRACE(4, "LocalCon\tDeleted connection.");
}


PBoolean OpalLocalConnection::SetUpConnection()
{
  originating = true;

  // Check if we are A-Party in thsi call, so need to do things differently
  if (ownerCall.GetConnection(0) == this) {
    SetPhase(SetUpPhase);
    if (!OnIncomingConnection(0, NULL)) {
      Release(EndedByCallerAbort);
      return false;
    }

    PTRACE(3, "LocalCon\tOutgoing call routed to " << ownerCall.GetPartyB() << " for " << *this);
    if (!ownerCall.OnSetUp(*this)) {
      Release(EndedByNoAccept);
      return false;
    }

    return true;
  }

  PTRACE(3, "LocalCon\tSetUpConnection(" << remotePartyName << ')');

  if (!endpoint.OnIncomingCall(*this))
    return false;

  if (endpoint.IsDeferredAlerting())
    return true;

  SetPhase(AlertingPhase);
  OnAlerting();

  return true;
}


PBoolean OpalLocalConnection::SetAlerting(const PString & calleeName, PBoolean)
{
  PTRACE(3, "LocalCon\tSetAlerting(" << calleeName << ')');
  SetPhase(AlertingPhase);
  remotePartyName = calleeName;
  return endpoint.OnOutgoingCall(*this);
}


OpalMediaStream * OpalLocalConnection::CreateMediaStream(const OpalMediaFormat & mediaFormat,
                                                         unsigned sessionID,
                                                         PBoolean isSource)
{
  return new OpalLocalMediaStream(*this, mediaFormat, sessionID, isSource, endpoint.IsSynchronous());
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


void OpalLocalConnection::OnPatchMediaStream(PBoolean isSource, OpalMediaPatch & patch)
{
  int clockRate;
  if (patch.GetSource().GetMediaFormat().GetMediaType() == OpalMediaType::Audio()) {
    PTRACE(3, "LocalCon\tAdding filters to patch");
    if (isSource)
      patch.AddFilter(silenceDetector->GetReceiveHandler(), OpalPCM16);
    clockRate = patch.GetSource().GetMediaFormat().GetClockRate();
    echoCanceler->SetParameters(endpoint.GetManager().GetEchoCancelParams());
    echoCanceler->SetClockRate(clockRate);
    patch.AddFilter(isSource?echoCanceler->GetReceiveHandler():echoCanceler->GetSendHandler(), OpalPCM16);
  }
}


bool OpalLocalConnection::SendUserInputString(const PString & value)
{
  PTRACE(3, "LocalCon\tSendUserInputString(" << value << ')');
  return endpoint.OnUserInput(*this, value);
}


void OpalLocalConnection::AlertingIncoming()
{
  if (LockReadWrite()) {
    SetPhase(AlertingPhase);
    OnAlerting();
    UnlockReadWrite();
  }
}


void OpalLocalConnection::AcceptIncoming()
{
  if (LockReadWrite()) {
    OnConnectedInternal();
    OpalMediaTypeFactory::KeyList_T mediaTypes = OpalMediaType::GetList();
    for (OpalMediaTypeFactory::KeyList_T::iterator iter = mediaTypes.begin(); iter != mediaTypes.end(); ++iter) {
      OpalMediaType mediaType = *iter;
      switch (GetAutoStart(mediaType)) {
        case OpalMediaType::Transmit :
        case OpalMediaType::TransmitReceive :
          if (GetMediaStream(mediaType, true) == NULL)
            ownerCall.OpenSourceMediaStreams(*this, mediaType);

        default :
          break;
      }
    }
    UnlockReadWrite();
  }
}


/////////////////////////////////////////////////////////////////////////////

OpalLocalMediaStream::OpalLocalMediaStream(OpalLocalConnection & connection,
                                           const OpalMediaFormat & mediaFormat,
                                           unsigned sessionID,
                                           bool isSource,
                                           bool isSynchronous)
  : OpalMediaStream(connection, mediaFormat, sessionID, isSource)
  , OpalMediaStreamPacing(mediaFormat)
  , m_isSynchronous(isSynchronous)
{
}


PBoolean OpalLocalMediaStream::ReadPacket(RTP_DataFrame & frame)
{
  if (!isOpen)
    return false;

  OpalLocalEndPoint & ep = dynamic_cast<OpalLocalEndPoint &>(connection.GetEndPoint());
  OpalLocalConnection & conn = dynamic_cast<OpalLocalConnection &>(connection);
  if (ep.OnReadMediaFrame(conn, *this, frame))
    return true;

  return OpalMediaStream::ReadPacket(frame);
}


PBoolean OpalLocalMediaStream::WritePacket(RTP_DataFrame & frame)
{
  if (!isOpen)
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

  if (!m_isSynchronous)
    Pace(true, size, marker);
  return true;
}


PBoolean OpalLocalMediaStream::WriteData(const BYTE * data, PINDEX length, PINDEX & written)
{
  OpalLocalEndPoint & ep = dynamic_cast<OpalLocalEndPoint &>(connection.GetEndPoint());
  OpalLocalConnection & conn = dynamic_cast<OpalLocalConnection &>(connection);
  if (!ep.OnWriteMediaData(conn, *this, data, length, written))
    return false;

  if (!m_isSynchronous)
    Pace(false, written, marker);
  return true;
}


PBoolean OpalLocalMediaStream::IsSynchronous() const
{
  return m_isSynchronous;
}


/////////////////////////////////////////////////////////////////////////////
