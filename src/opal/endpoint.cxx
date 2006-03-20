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
 * $Log: endpoint.cxx,v $
 * Revision 1.2035.2.3  2006/03/20 05:01:47  csoutheren
 * Removed change markers
 *
 * Revision 2.34.2.2  2006/03/20 02:25:27  csoutheren
 * Backports from CVS head
 *
 * Revision 2.34.2.1  2006/03/16 07:07:24  csoutheren
 * Removed warning on Windows
 *
 * Revision 2.35  2006/03/12 06:36:57  rjongbloed
 * Fixed DevStudio warning
 *
 * Revision 2.34  2006/02/22 10:45:11  csoutheren
 * Added patch #1375116 from Frederic Heem
 * Set default bandwith to a sensible value
 *
 * Revision 2.33  2006/02/22 10:40:10  csoutheren
 * Added patch #1374583 from Frederic Heem
 * Added additional H.323 virtual function
 *
 * Revision 2.32  2005/10/08 19:26:38  dsandras
 * Added OnForwarded callback in case of call forwarding.
 *
 * Revision 2.31  2005/08/24 10:43:51  rjongbloed
 * Changed create video functions yet again so can return pointers that are not to be deleted.
 *
 * Revision 2.30  2005/08/23 12:45:09  rjongbloed
 * Fixed creation of video preview window and setting video grab/display initial frame size.
 *
 * Revision 2.29  2005/07/14 08:51:19  csoutheren
 * Removed CreateExternalRTPAddress - it's not needed because you can override GetMediaAddress
 * to do the same thing
 * Fixed problems with logic associated with media bypass
 *
 * Revision 2.28  2005/07/11 06:52:16  csoutheren
 * Added support for outgoing calls using external RTP
 *
 * Revision 2.27  2005/07/11 01:52:26  csoutheren
 * Extended AnsweringCall to work for SIP as well as H.323
 * Fixed problems with external RTP connection in H.323
 * Added call to OnClosedMediaStream
 *
 * Revision 2.26  2005/04/10 21:13:55  dsandras
 * Added callback that is called when a connection is put on hold.
 *
 * Revision 2.25  2004/08/14 07:56:39  rjongbloed
 * Major revision to utilise the PSafeCollection classes for the connections and calls.
 *
 * Revision 2.24  2004/07/14 13:26:14  rjongbloed
 * Fixed issues with the propagation of the "established" phase of a call. Now
 *   calling an OnEstablished() chain like OnAlerting() and OnConnected() to
 *   finally arrive at OnEstablishedCall() on OpalManager
 *
 * Revision 2.23  2004/07/11 12:42:13  rjongbloed
 * Added function on endpoints to get the list of all media formats any
 *   connection the endpoint may create can support.
 *
 * Revision 2.22  2004/05/15 12:53:03  rjongbloed
 * Added default username and display name to manager, for all endpoints.
 *
 * Revision 2.21  2004/04/26 06:30:34  rjongbloed
 * Added ability to specify more than one defualt listener for an endpoint,
 *   required by SIP which listens on both UDP and TCP.
 *
 * Revision 2.20  2004/04/25 02:53:29  rjongbloed
 * Fixed GNU 3.4 warnings
 *
 * Revision 2.19  2004/03/13 06:25:54  rjongbloed
 * Slight rearrangement of local party name and alias list to beter match common
 *   behaviour in ancestor.
 * Abstracted local party name for endpoint into ancestor from H.,323.
 *
 * Revision 2.18  2004/02/19 10:47:05  rjongbloed
 * Merged OpenH323 version 1.13.1 changes.
 *
 * Revision 2.17  2003/03/24 04:36:53  robertj
 * Changed StartListsners() so if have empty list, starts default.
 *
 * Revision 2.16  2003/03/17 10:27:00  robertj
 * Added video support.
 *
 * Revision 2.15  2002/11/10 11:33:19  robertj
 * Updated to OpenH323 v1.10.3
 *
 * Revision 2.14  2002/09/06 02:42:03  robertj
 * Fixed bug where get endless wait if clearing calls and there aren't any.
 *
 * Revision 2.13  2002/07/04 07:41:47  robertj
 * Fixed memory/thread leak of transports.
 *
 * Revision 2.12  2002/07/01 04:56:33  robertj
 * Updated to OpenH323 v1.9.1
 *
 * Revision 2.11  2002/06/16 02:24:43  robertj
 * Fixed memory leak of failed listeners, thanks Ted Szoczei
 *
 * Revision 2.10  2002/04/05 10:37:46  robertj
 * Rearranged OnRelease to remove the connection from endpoints connection
 *   list at the end of the release phase rather than the beginning.
 *
 * Revision 2.9  2002/01/22 05:12:27  robertj
 * Revamp of user input API triggered by RFC2833 support
 *
 * Revision 2.8  2001/11/14 01:31:55  robertj
 * Corrected placement of adjusting media format list.
 *
 * Revision 2.7  2001/11/13 04:29:48  robertj
 * Changed OpalTransportAddress CreateTransport and CreateListsner functions
 *   to have extra parameter to control local binding of sockets.
 *
 * Revision 2.6  2001/08/23 05:51:17  robertj
 * Completed implementation of codec reordering.
 *
 * Revision 2.5  2001/08/22 10:20:09  robertj
 * Changed connection locking to use double mutex to guarantee that
 *   no threads can ever deadlock or access deleted connection.
 *
 * Revision 2.4  2001/08/17 08:26:26  robertj
 * Moved call end reasons enum from OpalConnection to global.
 *
 * Revision 2.3  2001/08/01 05:44:40  robertj
 * Added function to get all media formats possible in a call.
 *
 * Revision 2.2  2001/07/30 07:22:25  robertj
 * Abstracted listener management from H.323 to OpalEndPoint class.
 *
 * Revision 2.1  2001/07/30 01:40:01  robertj
 * Fixed GNU C++ warnings.
 *
 * Revision 2.0  2001/07/27 15:48:25  robertj
 * Conversion of OpenH323 to Open Phone Abstraction Library (OPAL)
 *
 */

#include <ptlib.h>

#ifdef __GNUC__
#pragma implementation "endpoint.h"
#endif

#include <opal/endpoint.h>

#include <opal/manager.h>
#include <opal/call.h>

//
// TODO find the correct value, usually the bandwidth for pure audio call is 1280 kb/sec 
// Set the bandwidth to 10Mbits, as setting the bandwith to UINT_MAX causes problems with cisco gatekeepers 
//
#define BANDWITH_DEFAULT_INITIAL 10000

#define new PNEW

/////////////////////////////////////////////////////////////////////////////

OpalEndPoint::OpalEndPoint(OpalManager & mgr,
                           const PCaselessString & prefix,
                           unsigned attributes)
  : manager(mgr),
    prefixName(prefix),
    attributeBits(attributes),
    defaultLocalPartyName(manager.GetDefaultUserName()),
    defaultDisplayName(manager.GetDefaultDisplayName())
{
  manager.AttachEndPoint(this);

  defaultSignalPort = 0;

  initialBandwidth = BANDWITH_DEFAULT_INITIAL;

  if (defaultLocalPartyName.IsEmpty())
    defaultLocalPartyName = PProcess::Current().GetName() & "User";

  PTRACE(3, "OpalEP\tCreated endpoint: " << prefixName);
}


OpalEndPoint::~OpalEndPoint()
{
  PTRACE(3, "OpalEP\t" << prefixName << " endpoint destroyed.");
}


void OpalEndPoint::PrintOn(ostream & strm) const
{
  strm << "EP<" << prefixName << '>';
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
    OpalTransportAddress iface(interfaces[i], defaultSignalPort);
    if (StartListener(iface))
      startedOne = TRUE;
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


PStringArray OpalEndPoint::GetDefaultListeners() const
{
  PStringArray listenerAddresses;
  if (defaultSignalPort != 0)
    listenerAddresses.AppendString(psprintf("tcp$*:%u", defaultSignalPort));
  return listenerAddresses;
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


void OpalEndPoint::DestroyConnection(OpalConnection * connection)
{
  delete connection;
}

BOOL OpalEndPoint::OnSetUpConnection(OpalConnection & PTRACE_PARAM(connection))
{
  PTRACE(3, "OpalEP\tOnSetUpConnection " << connection);
  return TRUE;
}

BOOL OpalEndPoint::OnIncomingConnection(OpalConnection & connection)
{
  return manager.OnIncomingConnection(connection);
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
  BOOL releasedOne = FALSE;
  for (PSafePtr<OpalConnection> connection(connectionsActive, PSafeReadOnly); connection; ++connection) {
    connection->Release(reason);
    releasedOne = TRUE;
  }

  if (wait && releasedOne)
    allConnectionsCleared.Wait();
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


OpalT120Protocol * OpalEndPoint::CreateT120ProtocolHandler(const OpalConnection & connection) const
{
  return manager.CreateT120ProtocolHandler(connection);
}


OpalT38Protocol * OpalEndPoint::CreateT38ProtocolHandler(const OpalConnection & connection) const
{
  return manager.CreateT38ProtocolHandler(connection);
}

/////////////////////////////////////////////////////////////////////////////
