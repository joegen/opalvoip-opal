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
 * Revision 1.2007  2001/08/23 05:51:17  robertj
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


#define new PNEW


/////////////////////////////////////////////////////////////////////////////

OpalEndPoint::OpalEndPoint(OpalManager & mgr,
                           const PCaselessString & prefix,
                           unsigned attributes)
  : manager(mgr),
    prefixName(prefix),
    attributeBits(attributes)
{
  manager.AttachEndPoint(this);

  defaultSignalPort = 0;
  initialBandwidth = UINT_MAX; // Infinite bandwidth

  connectionsActive.DisallowDeleteObjects();

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


BOOL OpalEndPoint::StartListeners(const PStringArray & interfaces)
{
  BOOL startedOne = FALSE;

  for (PINDEX i = 0; i < interfaces.GetSize(); i++) {
    OpalTransportAddress iface(interfaces[i], defaultSignalPort);
    if (StartListener(iface))
      startedOne = TRUE;
  }

  return startedOne;
}


BOOL OpalEndPoint::StartListener(const OpalTransportAddress & iface)
{
  OpalListener * listener;

  if (iface.IsEmpty())
    listener = new OpalListenerTCP(*this, INADDR_ANY, defaultSignalPort);
  else {
    listener = iface.CreateListener(*this);
    if (listener == NULL) {
      PTRACE(1, "OpalEP\tCould not create listener: " << iface);
      return FALSE;
    }
  }

  if (StartListener(listener))
    return TRUE;

  PTRACE(1, "OpalEP\tCould not start listener: " << iface);
  return FALSE;
}


BOOL OpalEndPoint::StartListener(OpalListener * listener)
{
  // Use default if NULL specified
  if (listener == NULL)
    listener = new OpalListenerTCP(*this, INADDR_ANY, defaultSignalPort);

  // as the listener is not open, this will have the effect of immediately
  // stopping the listener thread. This is good - it means that the 
  // listener Close function will appear to have stopped the thread
  if (!listener->Open(PCREATE_NOTIFIER(ListenerCallback)))
    return FALSE;

  listeners.Append(listener);
  return TRUE;
}


BOOL OpalEndPoint::RemoveListener(OpalListener * listener)
{
  if (listener != NULL)
    return listeners.Remove(listener);

  listeners.RemoveAll();
  return TRUE;
}


void OpalEndPoint::ListenerCallback(PThread &, INT param)
{
  // Error in accept, usually means listener thread stopped, so just exit
  if (param == 0)
    return;

  NewIncomingConnection((OpalTransport *)param);
}


void OpalEndPoint::NewIncomingConnection(OpalTransport * /*transport*/)
{
}


OpalConnection * OpalEndPoint::GetConnectionWithLock(const PString & token)
{
  PWaitAndSignal wait(inUseFlag);

  OpalConnection * connection = connectionsActive.GetAt(token);
  if (connection == NULL)
    return NULL;

  if (connection->Lock())
    return connection;

  return NULL;
}


BOOL OpalEndPoint::HasConnection(const PString & token)
{
  PWaitAndSignal wait(inUseFlag);
  return connectionsActive.Contains(token);
}


void OpalEndPoint::RemoveConnection(OpalConnection * connection)
{
  PAssertNULL(connection);
  inUseFlag.Wait();
  connectionsActive.SetAt(connection->GetToken(), NULL);
  inUseFlag.Signal();
}


BOOL OpalEndPoint::OnIncomingConnection(OpalConnection & connection)
{
  return manager.OnIncomingConnection(connection);
}


void OpalEndPoint::OnAlerting(OpalConnection & connection)
{
  manager.OnAlerting(connection);
}


void OpalEndPoint::OnConnected(OpalConnection & connection)
{
  manager.OnConnected(connection);
}


void OpalEndPoint::OnEstablished(OpalConnection & /*connection*/)
{
}


BOOL OpalEndPoint::OnReleased(OpalConnection & connection)
{
  return manager.OnReleased(connection);
}


BOOL OpalEndPoint::ClearCall(const PString & token,
                             OpalCallEndReason reason,
                             PSyncPoint * sync)
{
  return manager.ClearCall(token, reason, sync);
}


BOOL OpalEndPoint::ClearCallSynchronous(const PString & token,
                                        OpalCallEndReason reason,
                                        PSyncPoint * sync)
{
  PSyncPoint syncPoint;
  if (sync == NULL)
    sync = &syncPoint;
  return manager.ClearCall(token, reason, sync);
}


void OpalEndPoint::ClearAllCalls(OpalCallEndReason reason, BOOL wait)
{
  inUseFlag.Wait();

  // Add all connections to the to be deleted set
  PINDEX i;
  for (i = 0; i < connectionsActive.GetSize(); i++)
    manager.ClearCall(connectionsActive.GetKeyAt(i), reason);

  inUseFlag.Signal();

  if (wait)
    allConnectionsCleared.Wait();
}


void OpalEndPoint::AdjustMediaFormats(const OpalConnection & connection,
                                      OpalMediaFormatList & mediaFormats)
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


void OpalEndPoint::OnUserIndicationString(OpalConnection & connection,
                                    const PString & value)
{
  manager.OnUserIndicationString(connection, value);
}


void OpalEndPoint::OnUserIndicationTone(OpalConnection & connection,
                                  char tone,
                                  int duration)
{
  manager.OnUserIndicationTone(connection, tone, duration);
}


/////////////////////////////////////////////////////////////////////////////
