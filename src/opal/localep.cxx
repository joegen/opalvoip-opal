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


#define new PNEW


/////////////////////////////////////////////////////////////////////////////

OpalLocalEndPoint::OpalLocalEndPoint(OpalManager & mgr, const char * prefix)
  : OpalEndPoint(mgr, prefix, CanTerminateCall)
  , m_deferredAlerting(false)
  , m_deferredAnswer(false)
{
  PTRACE(3, "LocalEP\tCreated endpoint.");
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


bool OpalLocalEndPoint::AlertingIncomingCall(const PString & token, OpalConnection::StringOptions * options)
{
  PSafePtr<OpalLocalConnection> connection = GetLocalConnectionWithLock(token, PSafeReadOnly);
  if (connection == NULL) {
    PTRACE(2, "LocalEP\tCould not find connection using token \"" << token << '"');
    return false;
  }

  if (options != NULL)
    connection->SetStringOptions(*options, false);

  connection->AlertingIncoming();
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
  if (connection == NULL)
    return false;

  connection->Release(reason);
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

OpalLocalConnection::OpalLocalConnection(OpalCall & call,
                                OpalLocalEndPoint & ep,
                                             void * /*userData*/,
                                           unsigned options,
                    OpalConnection::StringOptions * stringOptions,
                                               char tokenPrefix)
  : OpalConnection(call, ep, ep.GetManager().GetNextToken(tokenPrefix), options, stringOptions)
  , endpoint(ep)
  , userData(NULL)
{
#if OPAL_PTLIB_DTMF
  m_sendInBandDTMF = m_detectInBandDTMF = false;
#endif
  PTRACE(4, "LocalCon\tCreated connection with token \"" << callToken << '"');
}


OpalLocalConnection::~OpalLocalConnection()
{
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


PBoolean OpalLocalConnection::SetUpConnection()
{
  bool notIncoming = ownerCall.GetConnection(0) == this || ownerCall.IsEstablished();

  if (!OpalConnection::SetUpConnection())
    return false;

  if (notIncoming)
    return true;

  if (!endpoint.OnIncomingCall(*this)) {
    Release(EndedByLocalBusy);
    return false;
  }

  if (!endpoint.IsDeferredAlerting())
    AlertingIncoming();

  return true;
}


PBoolean OpalLocalConnection::SetAlerting(const PString & calleeName, PBoolean)
{
  PTRACE(3, "LocalCon\tSetAlerting(" << calleeName << ')');
  SetPhase(AlertingPhase);
  remotePartyName = calleeName;
  return endpoint.OnOutgoingCall(*this);
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


bool OpalLocalConnection::SendUserInputString(const PString & value)
{
  PTRACE(3, "LocalCon\tSendUserInputString(" << value << ')');
  return endpoint.OnUserInput(*this, value);
}


void OpalLocalConnection::AlertingIncoming()
{
  if (LockReadWrite()) {
    if (GetPhase() < AlertingPhase) {
      SetPhase(AlertingPhase);
      OnAlerting();
    }
    UnlockReadWrite();
  }
}


void OpalLocalConnection::AcceptIncoming()
{
  // Defer this a teency amount so get don't block processing for too long
  new ::PThreadObj<OpalLocalConnection>(*this, &OpalLocalConnection::InternalAcceptIncoming, true, "LocalAccept");
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
