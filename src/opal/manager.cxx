/*
 * manager.cxx
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
 * $Log: manager.cxx,v $
 * Revision 1.2004  2001/08/23 05:51:17  robertj
 * Completed implementation of codec reordering.
 *
 * Revision 2.2  2001/08/17 08:25:41  robertj
 * Added call end reason for whole call, not just connection.
 * Moved call end reasons enum from OpalConnection to global.
 *
 * Revision 2.1  2001/08/01 05:44:41  robertj
 * Added function to get all media formats possible in a call.
 *
 * Revision 2.0  2001/07/27 15:48:25  robertj
 * Conversion of OpenH323 to Open Phone Abstraction Library (OPAL)
 *
 */

#include <ptlib.h>

#ifdef __GNUC__
#pragma implementation "manager.h"
#endif

#include <opal/manager.h>

#include <opal/patch.h>
#include <opal/mediastrm.h>


#ifndef IPTOS_PREC_CRITIC_ECP
#define IPTOS_PREC_CRITIC_ECP (5 << 5)
#endif

#ifndef IPTOS_LOWDELAY
#define IPTOS_LOWDELAY 0x10
#endif


static const char * const DefaultMediaFormatOrder[] = {
  OPAL_G7231,
  OPAL_G729B,
  OPAL_G729AB,
  OPAL_G729,
  OPAL_G729A,
  OPAL_GSM0610,
  OPAL_G728,
  OPAL_G711_ULAW_64K,
  OPAL_G711_ALAW_64K
};

#define new PNEW


/////////////////////////////////////////////////////////////////////////////

OpalManager::OpalManager()
  : mediaFormatOrder(PARRAYSIZE(DefaultMediaFormatOrder), DefaultMediaFormatOrder)
{
  autoStartReceiveVideo = autoStartTransmitVideo = TRUE;

  rtpIpPortBase = 5000;
  rtpIpPortMax  = 6000;

  // use dynamic port allocation by default
  tcpPortBase   = tcpPortMax = 0;

#ifdef _WIN32
  rtpIpTypeofService = IPTOS_PREC_CRITIC_ECP|IPTOS_LOWDELAY;
#else
  // Don't use IPTOS_PREC_CRITIC_ECP on Unix platforms as then need to be root
  rtpIpTypeofService = IPTOS_LOWDELAY;
#endif

  maxAudioDelayJitter = 60;

  lastCallTokenID = 1;

  callsActive.DisallowDeleteObjects();
  collectingGarbage = TRUE;

  garbageCollector = PThread::Create(PCREATE_NOTIFIER(GarbageMain), 0,
                                     PThread::NoAutoDeleteThread,
                                     PThread::LowPriority,
                                     "OpalGarbage");

  PTRACE(3, "OpalMan\tCreated manager.");
}


OpalManager::~OpalManager()
{
  // Clear any pending calls on this endpoint
  ClearAllCalls();

  // Shut down the cleaner thread
  collectingGarbage = FALSE;
  garbageCollectFlag.Signal();
  garbageCollector->WaitForTermination();

  // Clean up any calls that the cleaner thread missed
  GarbageCollection();

  // Kill off the endpoints, could wait till compiler generated destructor but
  // prefer to keep the PTRACE's in sequence.
  endpoints.RemoveAll();

  PTRACE(3, "OpalMan\tDeleted manager.");
}


void OpalManager::AttachEndPoint(OpalEndPoint * endpoint)
{
  PAssertNULL(endpoint);

  inUseFlag.Wait();

  if (endpoints.GetObjectsIndex(endpoint) == P_MAX_INDEX)
    endpoints.Append(endpoint);

  inUseFlag.Signal();
}


void OpalManager::RemoveEndPoint(OpalEndPoint * endpoint)
{
  inUseFlag.Wait();
  endpoints.Remove(endpoint);
  inUseFlag.Signal();
}


OpalCall * OpalManager::SetUpCall(const PString & partyA,
                                  const PString & partyB,
                                  PString & token)
{
  PTRACE(3, "OpalMan\tSet up call from " << partyA << " to " << partyB);

  OpalCall * call = CreateCall();
  token = call->GetToken();

  if (SetUpConnection(*call, partyA) != NULL &&
      SetUpConnection(*call, partyB) != NULL)
    return call;

  call->Clear();
  return NULL;
}


void OpalManager::OnEstablishedCall(OpalCall & /*call*/)
{
}


BOOL OpalManager::HasCall(const PString & token)
{
  PWaitAndSignal wait(callsMutex);

  return FindCallWithoutLocks(token) != NULL;
}


OpalCall * OpalManager::FindCallWithLock(const PString & token)
{
  PWaitAndSignal wait(callsMutex);

  OpalCall * call = FindCallWithoutLocks(token);
  if (call == NULL)
    return NULL;

  call->Lock();
  return call;
}


OpalCall * OpalManager::FindCallWithoutLocks(const PString & token)
{
  if (token.IsEmpty())
    return NULL;

  OpalCall * conn_ptr = callsActive.GetAt(token);
  if (conn_ptr != NULL)
    return conn_ptr;

  return NULL;
}


BOOL OpalManager::ClearCall(const PString & token,
                            OpalCallEndReason reason,
                            PSyncPoint * sync)
{
  /*The hugely multi-threaded nature of the OpalCall objects means that
    to avoid many forms of race condition, a call is cleared by moving it from
    the "active" call dictionary to a list of calls to be cleared that will be
    processed by a background thread specifically for the purpose of cleaning
    up cleared calls. So that is all that this function actually does.
    The real work is done in the OpalGarbageCollector thread.
   */

  PWaitAndSignal wait(callsMutex);

  // Find the call by token, callid or conferenceid
  OpalCall * call = FindCallWithoutLocks(token);
  if (call == NULL)
    return FALSE;

  call->Clear(reason);

  if (sync != NULL)
    sync->Wait();

  return TRUE;
}


void OpalManager::ClearAllCalls(OpalCallEndReason reason, BOOL wait)
{
  /*The hugely multi-threaded nature of the OpalCall objects means that
    to avoid many forms of race condition, a call is cleared by moving it from
    the "active" call dictionary to a list of calls to be cleared that will be
    processed by a background thread specifically for the purpose of cleaning
    up cleared calls. So that is all that this function actually does.
    The real work is done in the OpalGarbageCollector thread.
   */

  callsMutex.Wait();

  // Remove all calls from the active list first
  PINDEX i;
  for (i = 0; i < callsActive.GetSize(); i++)
    callsActive.GetDataAt(i).Clear(reason);

  callsMutex.Signal();

  SignalGarbageCollector();

  if (wait)
    allCallsCleared.Wait();
}


void OpalManager::OnClearedCall(OpalCall & /*call*/)
{
}


OpalCall * OpalManager::CreateCall()
{
  return new OpalCall(*this);
}


void OpalManager::DestroyCall(OpalCall * call)
{
  delete call;
}


PString OpalManager::GetNextCallToken()
{
  PString token;
  callsMutex.Wait();
  token.sprintf("%u", lastCallTokenID++);
  callsMutex.Signal();
  return token;
}


void OpalManager::AttachCall(OpalCall * call)
{
  callsMutex.Wait();
  callsActive.SetAt(call->GetToken(), call);
  callsMutex.Signal();
}


OpalConnection * OpalManager::SetUpConnection(OpalCall & call,
                                              const PString & remoteParty)
{
  PTRACE(3, "OpalMan\tSet up connection to \"" << remoteParty << '"');

  if (endpoints.IsEmpty())
    return NULL;

  PCaselessString epname = remoteParty.Left(remoteParty.Find(':'));
  if (epname.IsEmpty())
    epname = endpoints[0].GetPrefixName();

  for (PINDEX i = 0; i < endpoints.GetSize(); i++) {
    if (epname == endpoints[i].GetPrefixName()) {
      OpalConnection * connection = endpoints[i].SetUpConnection(call, remoteParty, NULL);
      if (connection != NULL)
        return connection;
    }
  }

  return NULL;
}


BOOL OpalManager::OnIncomingConnection(OpalConnection & connection)
{
  PTRACE(3, "OpalMan\tOn incoming connection " << connection);

  OpalCall & call = connection.GetCall();
  if (call.GetConnectionCount() > 1)
    return TRUE;

  PString destinationAddress = OnRouteConnection(connection);
  if (!destinationAddress) {
    OpalConnection * outgoingConnection = SetUpConnection(call, destinationAddress);
    if (outgoingConnection != NULL) {
      outgoingConnection->Unlock();
      return TRUE;
    }
  }

  return FALSE;
}


PString OpalManager::OnRouteConnection(OpalConnection & connection)
{
  return connection.GetDestinationAddress();
}


void OpalManager::OnAlerting(OpalConnection & connection)
{
  PTRACE(3, "OpalMan\tOnAlerting " << connection);

  connection.GetCall().OnAlerting(connection);
}


void OpalManager::OnConnected(OpalConnection & connection)
{
  PTRACE(3, "OpalMan\tOnConnected " << connection);

  connection.GetCall().OnConnected(connection);
}


BOOL OpalManager::OnReleased(OpalConnection & connection)
{
  PTRACE(3, "OpalMan\tOnConnected " << connection);

  connection.GetCall().OnReleased(connection);
  return TRUE;
}


void OpalManager::AdjustMediaFormats(const OpalConnection & /*connection*/,
                                     OpalMediaFormatList & mediaFormats)
{
  PINDEX i;
  for (i = 0; i < mediaFormatMask.GetSize(); i++)
    mediaFormats -= mediaFormatMask[i];

  mediaFormats.Reorder(mediaFormatOrder);
}


BOOL OpalManager::OnOpenMediaStream(OpalConnection & connection,
                                    OpalMediaStream & stream)
{
  PTRACE(3, "OpalMan\tOnOpenMediaStream " << connection << ',' << stream);

  if (stream.IsSource())
    return connection.GetCall().PatchMediaStreams(connection, stream);

  return TRUE;
}


void OpalManager::OnClosedMediaStream(const OpalMediaStream & /*channel*/)
{
}


OpalMediaPatch * OpalManager::CreateMediaPatch(OpalMediaStream & source)
{
  return new OpalMediaPatch(source);
}


void OpalManager::DestroyMediaPatch(OpalMediaPatch * patch)
{
  delete patch;
}


BOOL OpalManager::OnStartMediaPatch(const OpalMediaPatch & /*patch*/)
{
  return TRUE;
}


void OpalManager::OnUserIndicationString(OpalConnection & connection,
                                         const PString & value)
{
  connection.SetUserIndication(value);
}


void OpalManager::OnUserIndicationTone(OpalConnection & connection,
                                       char tone,
                                       int /*duration*/)
{
  connection.OnUserIndicationString(tone);
}


void OpalManager::GarbageCollection()
{
  OpalCallList callsToRemove;
  callsToRemove.DisallowDeleteObjects();

  callsMutex.Wait();

  PINDEX i;
  for (i = 0; i < callsActive.GetSize(); i++) {
    OpalCall & call = callsActive.GetDataAt(i);
    if (call.GarbageCollection())
      callsToRemove.Append(&call);
  }

  for (i = 0; i < callsToRemove.GetSize(); i++)
    callsActive.SetAt(callsToRemove[i].GetToken(), NULL);

  callsMutex.Signal();

  for (i = 0; i < callsToRemove.GetSize(); i++)
    callsToRemove[i].OnCleared();

  for (i = 0; i < callsToRemove.GetSize(); i++)
    DestroyCall(&callsToRemove[i]);

  allCallsCleared.Signal();
}


void OpalManager::SignalGarbageCollector()
{
  // Signal the background threads that there is some stuff to process.
  garbageCollectFlag.Signal();
}


void OpalManager::GarbageMain(PThread &, INT)
{
  while (collectingGarbage) {
    garbageCollectFlag.Wait();
    GarbageCollection();
  }
}


/////////////////////////////////////////////////////////////////////////////
