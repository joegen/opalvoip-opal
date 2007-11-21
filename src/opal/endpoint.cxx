/*
 * endpoint.cxx
 *
 * Media channels abstraction
 *
 * Open Phone Abstraction Library (OPAL)
 * Formally known as the Open H323 project.
 *
 * Copyright (c) 2001 Equivalence Pty. Ltd.
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
 * $Id$
 */

#include <ptlib.h>

#ifdef __GNUC__
#pragma implementation "endpoint.h"
#endif

#include <opal/buildopts.h>

#include <opal/endpoint.h>

#include <opal/manager.h>
#include <opal/call.h>
#include <rtp/rtp.h>

//
// TODO find the correct value, usually the bandwidth for pure audio call is 1280 kb/sec 
// Set the bandwidth to 10Mbits, as setting the bandwith to UINT_MAX causes problems with cisco gatekeepers 
//
#define BANDWITH_DEFAULT_INITIAL 100000

#define new PNEW

/////////////////////////////////////////////////////////////////////////////

OpalEndPoint::OpalEndPoint(OpalManager & mgr,
                           const PCaselessString & prefix,
                           unsigned attributes)
  : manager(mgr),
    prefixName(prefix),
    attributeBits(attributes),
    productInfo(mgr.GetProductInfo()),
    defaultLocalPartyName(manager.GetDefaultUserName()),
    defaultDisplayName(manager.GetDefaultDisplayName())
#if OPAL_RTP_AGGREGATE
    ,useRTPAggregation(manager.UseRTPAggregation()),
    rtpAggregationSize(10),
    rtpAggregator(NULL)
#endif
{
  manager.AttachEndPoint(this);

  defaultSignalPort = 0;

  initialBandwidth = BANDWITH_DEFAULT_INITIAL;
  defaultSendUserInputMode = OpalConnection::SendUserInputAsProtocolDefault;

  if (defaultLocalPartyName.IsEmpty())
    defaultLocalPartyName = PProcess::Current().GetName() & "User";

  PTRACE(4, "OpalEP\tCreated endpoint: " << prefixName);

  defaultSecurityMode = mgr.GetDefaultSecurityMode();
}


OpalEndPoint::~OpalEndPoint()
{
#if OPAL_RTP_AGGREGATE
  // delete aggregators
  {
    PWaitAndSignal m(rtpAggregationMutex);
    if (rtpAggregator != NULL) {
      delete rtpAggregator;
      rtpAggregator = NULL;
    }
  }
#endif

  PTRACE(4, "OpalEP\t" << prefixName << " endpoint destroyed.");
}

BOOL OpalEndPoint::MakeConnection(OpalCall & /*call*/, const PString & /*party*/, void * /*userData*/, unsigned int /*options*/, OpalConnection::StringOptions * /*stringOptions*/)
{
  PAssertAlways("Must implement descendant of OpalEndPoint::MakeConnection");
  return FALSE;
}

void OpalEndPoint::PrintOn(ostream & strm) const
{
  strm << "EP<" << prefixName << '>';
}


BOOL OpalEndPoint::GarbageCollection()
{
  return connectionsActive.DeleteObjectsToBeRemoved();
}


BOOL OpalEndPoint::StartListeners(const PStringArray & listenerAddresses)
{
  PStringArray interfaces = listenerAddresses;
  if (interfaces.IsEmpty()) {
    interfaces = GetDefaultListeners();
    if (interfaces.IsEmpty())
      return FALSE;
  }

  BOOL startedOne = FALSE;

  for (PINDEX i = 0; i < interfaces.GetSize(); i++) {
    if (interfaces[i].Find('$') != P_MAX_INDEX) {
      if (StartListener(interfaces[i]))
        startedOne = TRUE;
    }
    else {
      PStringArray transports = GetDefaultTransport().Tokenise(',');
      for (PINDEX j = 0; j < transports.GetSize(); j++) {
        OpalTransportAddress iface(interfaces[i], GetDefaultSignalPort(), transports[j]);
        if (StartListener(iface))
          startedOne = TRUE;
      }
    }
  }

  return startedOne;
}


BOOL OpalEndPoint::StartListener(const OpalTransportAddress & listenerAddress)
{
  OpalListener * listener;

  OpalTransportAddress iface = listenerAddress;

  if (iface.IsEmpty()) {
    PStringArray interfaces = GetDefaultListeners();
    if (interfaces.IsEmpty())
      return FALSE;
    iface = OpalTransportAddress(interfaces[0], defaultSignalPort);
  }

  listener = iface.CreateListener(*this, OpalTransportAddress::FullTSAP);
  if (listener == NULL) {
    PTRACE(1, "OpalEP\tCould not create listener: " << iface);
    return FALSE;
  }

  if (StartListener(listener))
    return TRUE;

  PTRACE(1, "OpalEP\tCould not start listener: " << iface);
  return FALSE;
}


BOOL OpalEndPoint::StartListener(OpalListener * listener)
{
  if (listener == NULL)
    return FALSE;

  // as the listener is not open, this will have the effect of immediately
  // stopping the listener thread. This is good - it means that the 
  // listener Close function will appear to have stopped the thread
  if (!listener->Open(PCREATE_NOTIFIER(ListenerCallback))) {
    delete listener;
    return FALSE;
  }

  listeners.Append(listener);
  return TRUE;
}

PString OpalEndPoint::GetDefaultTransport() const
{
  return "tcp$";
}

PStringArray OpalEndPoint::GetDefaultListeners() const
{
  PStringArray listenerAddresses;
  PStringArray transports = GetDefaultTransport().Tokenise(',');
  for (PINDEX i = 0; i < transports.GetSize(); i++) {
    PString listenerAddress = transports[i] + '*';
    if (defaultSignalPort != 0)
      listenerAddress.sprintf(":%u", defaultSignalPort);
    listenerAddresses += listenerAddress;
  }
  return listenerAddresses;
}


OpalListener * OpalEndPoint::FindListener(const OpalTransportAddress & iface)
{
  for (PINDEX i = 0; i < listeners.GetSize(); i++) {
    if (listeners[i].GetTransportAddress().IsEquivalent(iface))
      return &listeners[i];
  }
  return NULL;
}


BOOL OpalEndPoint::StopListener(const OpalTransportAddress & iface)
{
  OpalListener * listener = FindListener(iface);
  return listener != NULL && RemoveListener(listener);
}


BOOL OpalEndPoint::RemoveListener(OpalListener * listener)
{
  if (listener != NULL)
    return listeners.Remove(listener);

  listeners.RemoveAll();
  return TRUE;
}


OpalTransportAddressArray OpalEndPoint::GetInterfaceAddresses(BOOL excludeLocalHost,
                                                              OpalTransport * associatedTransport)
{
  return OpalGetInterfaceAddresses(listeners, excludeLocalHost, associatedTransport);
}


void OpalEndPoint::ListenerCallback(PThread &, INT param)
{
  // Error in accept, usually means listener thread stopped, so just exit
  if (param == 0)
    return;

  OpalTransport * transport = (OpalTransport *)param;
  if (NewIncomingConnection(transport))
    delete transport;
}


BOOL OpalEndPoint::NewIncomingConnection(OpalTransport * /*transport*/)
{
  return TRUE;
}


PStringList OpalEndPoint::GetAllConnections()
{
  PStringList tokens;

  for (PSafePtr<OpalConnection> connection(connectionsActive, PSafeReadOnly); connection != NULL; ++connection)
    tokens.AppendString(connection->GetToken());

  return tokens;
}


BOOL OpalEndPoint::HasConnection(const PString & token)
{
  PWaitAndSignal wait(inUseFlag);
  return connectionsActive.Contains(token);
}


BOOL OpalEndPoint::AddConnection(OpalConnection * connection)
{
  if (connection == NULL)
    return FALSE;

  OnNewConnection(connection->GetCall(), *connection);

  connectionsActive.SetAt(connection->GetToken(), connection);

  return TRUE;
}


void OpalEndPoint::DestroyConnection(OpalConnection * connection)
{
  delete connection;
}


BOOL OpalEndPoint::OnSetUpConnection(OpalConnection & PTRACE_PARAM(connection))
{
  PTRACE(3, "OpalEP\tOnSetUpConnection " << connection);
  return TRUE;
}


BOOL OpalEndPoint::OnIncomingConnection(OpalConnection & /*connection*/)
{
  return TRUE;
}


BOOL OpalEndPoint::OnIncomingConnection(OpalConnection & /*connection*/, unsigned /*options*/)
{
  return TRUE;
}


BOOL OpalEndPoint::OnIncomingConnection(OpalConnection & connection, unsigned options, OpalConnection::StringOptions * stringOptions)
{
  return OnIncomingConnection(connection) &&
         OnIncomingConnection(connection, options) &&
         manager.OnIncomingConnection(connection, options, stringOptions);
}


void OpalEndPoint::OnAlerting(OpalConnection & connection)
{
  manager.OnAlerting(connection);
}

OpalConnection::AnswerCallResponse
       OpalEndPoint::OnAnswerCall(OpalConnection & connection,
                                  const PString & caller)
{
  return manager.OnAnswerCall(connection, caller);
}

void OpalEndPoint::OnConnected(OpalConnection & connection)
{
  manager.OnConnected(connection);
}


void OpalEndPoint::OnEstablished(OpalConnection & connection)
{
  manager.OnEstablished(connection);
}


void OpalEndPoint::OnReleased(OpalConnection & connection)
{
  PTRACE(4, "OpalEP\tOnReleased " << connection);
  connectionsActive.RemoveAt(connection.GetToken());
  manager.OnReleased(connection);
}


void OpalEndPoint::OnHold(OpalConnection & connection)
{
  PTRACE(4, "OpalEP\tOnHold " << connection);
  manager.OnHold(connection);
}


BOOL OpalEndPoint::OnForwarded(OpalConnection & connection,
			       const PString & forwardParty)
{
  PTRACE(4, "OpalEP\tOnForwarded " << connection);
  return manager.OnForwarded(connection, forwardParty);
}


void OpalEndPoint::ConnectionDict::DeleteObject(PObject * object) const
{
  OpalConnection * connection = PDownCast(OpalConnection, object);
  if (connection != NULL)
    connection->GetEndPoint().DestroyConnection(connection);
}


BOOL OpalEndPoint::ClearCall(const PString & token,
                             OpalConnection::CallEndReason reason,
                             PSyncPoint * sync)
{
  return manager.ClearCall(token, reason, sync);
}


BOOL OpalEndPoint::ClearCallSynchronous(const PString & token,
                                        OpalConnection::CallEndReason reason,
                                        PSyncPoint * sync)
{
  PSyncPoint syncPoint;
  if (sync == NULL)
    sync = &syncPoint;
  return manager.ClearCall(token, reason, sync);
}


void OpalEndPoint::ClearAllCalls(OpalConnection::CallEndReason reason, BOOL wait)
{
  manager.ClearAllCalls(reason, wait);
}


void OpalEndPoint::AdjustMediaFormats(const OpalConnection & connection,
                                      OpalMediaFormatList & mediaFormats) const
{
  manager.AdjustMediaFormats(connection, mediaFormats);
}


BOOL OpalEndPoint::OnOpenMediaStream(OpalConnection & connection,
                                     OpalMediaStream & stream)
{
  return manager.OnOpenMediaStream(connection, stream);
}


void OpalEndPoint::OnClosedMediaStream(const OpalMediaStream & stream)
{
  manager.OnClosedMediaStream(stream);
}

#if OPAL_VIDEO
void OpalEndPoint::AddVideoMediaFormats(OpalMediaFormatList & mediaFormats,
                                        const OpalConnection * connection) const
{
  manager.AddVideoMediaFormats(mediaFormats, connection);
}


BOOL OpalEndPoint::CreateVideoInputDevice(const OpalConnection & connection,
                                          const OpalMediaFormat & mediaFormat,
                                          PVideoInputDevice * & device,
                                          BOOL & autoDelete)
{
  return manager.CreateVideoInputDevice(connection, mediaFormat, device, autoDelete);
}

BOOL OpalEndPoint::CreateVideoOutputDevice(const OpalConnection & connection,
                                           const OpalMediaFormat & mediaFormat,
                                           BOOL preview,
                                           PVideoOutputDevice * & device,
                                           BOOL & autoDelete)
{
  return manager.CreateVideoOutputDevice(connection, mediaFormat, preview, device, autoDelete);
}
#endif


void OpalEndPoint::OnUserInputString(OpalConnection & connection,
                                     const PString & value)
{
  manager.OnUserInputString(connection, value);
}


void OpalEndPoint::OnUserInputTone(OpalConnection & connection,
                                   char tone,
                                   int duration)
{
  manager.OnUserInputTone(connection, tone, duration);
}


PString OpalEndPoint::ReadUserInput(OpalConnection & connection,
                                    const char * terminators,
                                    unsigned lastDigitTimeout,
                                    unsigned firstDigitTimeout)
{
  return manager.ReadUserInput(connection, terminators, lastDigitTimeout, firstDigitTimeout);
}


#if OPAL_T120DATA

OpalT120Protocol * OpalEndPoint::CreateT120ProtocolHandler(const OpalConnection & connection) const
{
  return manager.CreateT120ProtocolHandler(connection);
}

#endif

#if OPAL_T38FAX

OpalT38Protocol * OpalEndPoint::CreateT38ProtocolHandler(const OpalConnection & connection) const
{
  return manager.CreateT38ProtocolHandler(connection);
}

#endif

#if OPAL_H224

OpalH224Handler * OpalEndPoint::CreateH224ProtocolHandler(OpalConnection & connection, 
														  unsigned sessionID) const
{
  return manager.CreateH224ProtocolHandler(connection, sessionID);
}

OpalH281Handler * OpalEndPoint::CreateH281ProtocolHandler(OpalH224Handler & h224Handler) const
{
  return manager.CreateH281ProtocolHandler(h224Handler);
}

#endif

void OpalEndPoint::OnNewConnection(OpalCall & call, OpalConnection & conn)
{
  call.GetManager().OnNewConnection(conn);
}

#if P_SSL
PString OpalEndPoint::GetSSLCertificate() const
{
  return "server.pem";
}
#endif

PHandleAggregator * OpalEndPoint::GetRTPAggregator()
{
#if OPAL_RTP_AGGREGATE
  PWaitAndSignal m(rtpAggregationMutex);
  if (rtpAggregationSize == 0)
    return NULL;

  if (rtpAggregator == NULL)
    rtpAggregator = new PHandleAggregator(rtpAggregationSize);

  return rtpAggregator;
#else
  return NULL;
#endif
}

BOOL OpalEndPoint::UseRTPAggregation() const
{ 
#if OPAL_RTP_AGGREGATE
  return useRTPAggregation; 
#else
  return FALSE;
#endif
}

void OpalEndPoint::SetRTPAggregationSize(PINDEX 
#if OPAL_RTP_AGGREGATE
                             size
#endif
                             )
{ 
#if OPAL_RTP_AGGREGATE
  rtpAggregationSize = size; 
#endif
}

PINDEX OpalEndPoint::GetRTPAggregationSize() const
{ 
#if OPAL_RTP_AGGREGATE
  return rtpAggregationSize; 
#else
  return 0;
#endif
}

BOOL OpalEndPoint::AdjustInterfaceTable(PIPSocket::Address & /*remoteAddress*/, 
                                        PIPSocket::InterfaceTable & /*interfaceTable*/)
{
  return TRUE;
}


BOOL OpalEndPoint::IsRTPNATEnabled(OpalConnection & conn, 
                         const PIPSocket::Address & localAddr, 
                         const PIPSocket::Address & peerAddr,
                         const PIPSocket::Address & sigAddr,
                                               BOOL incoming)
{
  return GetManager().IsRTPNATEnabled(conn, localAddr, peerAddr, sigAddr, incoming);
}


/////////////////////////////////////////////////////////////////////////////
