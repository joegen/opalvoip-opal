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
 * Revision 1.2001  2001/07/27 15:48:25  robertj
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
  return connectionsActive.Contains(token) != NULL;
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
  inUseFlag.Wait();
  connectionsActive.SetAt(connection.GetToken(), NULL);
  inUseFlag.Signal();

  return manager.OnReleased(connection);
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


void OpalEndPoint::ClearAllCalls(OpalConnection::CallEndReason reason,
                                 BOOL wait)
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
