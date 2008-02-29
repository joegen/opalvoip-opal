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
 * Revision 1.2048  2005/07/11 06:52:17  csoutheren
 * Added support for outgoing calls using external RTP
 *
 * Revision 2.46  2005/07/11 01:52:26  csoutheren
 * Extended AnsweringCall to work for SIP as well as H.323
 * Fixed problems with external RTP connection in H.323
 * Added call to OnClosedMediaStream
 *
 * Revision 2.45  2005/07/09 06:59:28  rjongbloed
 * Changed SetSTUNServer so returns the determined NAT type.
 * Fixed SetSTUNServer so does not try and get external router address is have blocking NAT,
 *   just takes up more time!
 * Fixed possible deadlock on exiting program, waiting for garbage collector to finish.
 *
 * Revision 2.44  2005/06/02 13:18:02  rjongbloed
 * Fixed compiler warnings
 *
 * Revision 2.43  2005/05/25 17:04:07  dsandras
 * Fixed ClearAllCalls when being executed synchronously. Fixes a crash on exit when a call is in progress. Thanks Robert!
 *
 * Revision 2.42  2005/05/23 21:22:04  dsandras
 * Wait on the mutex for calls missed by the garbage collector.
 *
 * Revision 2.41  2005/04/10 21:15:08  dsandras
 * Added callback that is called when a connection is put on hold (local or remote).
 *
 * Revision 2.40  2005/02/21 12:19:55  rjongbloed
 * Added new "options list" to the OpalMediaFormat class.
 *
 * Revision 2.39  2004/10/16 03:10:04  rjongbloed
 * Fixed correct detection of when to use STUN, this does not include the local host!
 *
 * Revision 2.38  2004/10/03 15:16:04  rjongbloed
 * Removed no trace warning
 *
 * Revision 2.37  2004/08/18 13:03:23  rjongbloed
 * Changed to make calling OPalManager::OnClearedCall() in override optional.
 *
 * Revision 2.36  2004/08/14 07:56:41  rjongbloed
 * Major revision to utilise the PSafeCollection classes for the connections and calls.
 *
 * Revision 2.35  2004/07/14 13:26:14  rjongbloed
 * Fixed issues with the propagation of the "established" phase of a call. Now
 *   calling an OnEstablished() chain like OnAlerting() and OnConnected() to
 *   finally arrive at OnEstablishedCall() on OpalManager
 *
 * Revision 2.34  2004/07/11 12:42:13  rjongbloed
 * Added function on endpoints to get the list of all media formats any
 *   connection the endpoint may create can support.
 *
 * Revision 2.33  2004/05/15 12:53:03  rjongbloed
 * Added default username and display name to manager, for all endpoints.
 *
 * Revision 2.32  2004/05/01 10:00:53  rjongbloed
 * Fixed ClearCallSynchronous so now is actually signalled when call is destroyed.
 *
 * Revision 2.31  2004/04/29 11:48:32  rjongbloed
 * Fixed possible deadlock if close all and close synchronous.
 *
 * Revision 2.30  2004/04/25 02:53:29  rjongbloed
 * Fixed GNU 3.4 warnings
 *
 * Revision 2.29  2004/04/18 13:35:28  rjongbloed
 * Fixed ability to make calls where both endpoints are specified a priori. In particular
 *   fixing the VXML support for an outgoing sip/h323 call.
 *
 * Revision 2.28  2004/04/18 07:09:12  rjongbloed
 * Added a couple more API functions to bring OPAL into line with similar functions in OpenH323.
 *
 * Revision 2.27  2004/02/24 11:37:02  rjongbloed
 * More work on NAT support, manual external address translation and STUN
 *
 * Revision 2.26  2004/02/19 10:47:06  rjongbloed
 * Merged OpenH323 version 1.13.1 changes.
 *
 * Revision 2.25  2004/01/18 15:36:07  rjongbloed
 * Added stun support
 *
 * Revision 2.24  2003/12/15 11:43:07  rjongbloed
 * Updated to new API due to plug-ins
 *
 * Revision 2.23  2003/03/17 10:27:00  robertj
 * Added video support.
 *
 * Revision 2.22  2003/03/07 08:13:42  robertj
 * Fixed validation of "protocol:" part of address in routing system.
 *
 * Revision 2.21  2003/03/06 03:57:47  robertj
 * IVR support (work in progress) requiring large changes everywhere.
 *
 * Revision 2.20  2003/01/07 04:39:53  robertj
 * Updated to OpenH323 v1.11.2
 *
 * Revision 2.19  2002/11/10 11:33:19  robertj
 * Updated to OpenH323 v1.10.3
 *
 * Revision 2.18  2002/09/06 02:44:30  robertj
 * Added routing table system to route calls by regular expressions.
 *
 * Revision 2.17  2002/09/04 06:01:49  robertj
 * Updated to OpenH323 v1.9.6
 *
 * Revision 2.16  2002/07/01 04:56:33  robertj
 * Updated to OpenH323 v1.9.1
 *
 * Revision 2.15  2002/06/16 02:25:11  robertj
 * Fixed memory leak of garbage collector, thanks Ted Szoczei
 *
 * Revision 2.14  2002/04/17 07:19:38  robertj
 * Fixed incorrect trace message for OnReleased
 *
 * Revision 2.13  2002/04/10 08:15:31  robertj
 * Added functions to set ports.
 *
 * Revision 2.12  2002/04/08 02:40:13  robertj
 * Fixed issues with using double originate call, eg from simple app command line.
 *
 * Revision 2.11  2002/02/19 07:49:47  robertj
 * Restructured media bypass functions to fix problems with RFC2833.
 *
 * Revision 2.10  2002/02/11 07:42:17  robertj
 * Added media bypass for streams between compatible protocols.
 *
 * Revision 2.9  2002/01/22 05:12:51  robertj
 * Revamp of user input API triggered by RFC2833 support
 *
 * Revision 2.8  2002/01/14 06:35:58  robertj
 * Updated to OpenH323 v1.7.9
 *
 * Revision 2.7  2001/12/07 08:56:43  robertj
 * Renamed RTP to be more general UDP port base, and TCP to be IP.
 *
 * Revision 2.6  2001/11/14 01:31:55  robertj
 * Corrected placement of adjusting media format list.
 *
 * Revision 2.5  2001/11/13 06:25:56  robertj
 * Changed SetUpConnection() so returns BOOL as returning
 *   pointer to connection is not useful.
 *
 * Revision 2.4  2001/10/04 00:44:26  robertj
 * Added function to remove wildcard from list.
 *
 * Revision 2.3  2001/08/23 05:51:17  robertj
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

#include <opal/endpoint.h>
#include <opal/call.h>
#include <opal/patch.h>
#include <opal/mediastrm.h>
#include <codec/vidcodec.h>

#include "../../version.h"


#ifndef IPTOS_PREC_CRITIC_ECP
#define IPTOS_PREC_CRITIC_ECP (5 << 5)
#endif

#ifndef IPTOS_LOWDELAY
#define IPTOS_LOWDELAY 0x10
#endif


static const char * const DefaultMediaFormatOrder[] = {
  OPAL_G7231_6k3,
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

PString OpalGetVersion()
{
#define AlphaCode   "alpha"
#define BetaCode    "beta"
#define ReleaseCode "."

  return psprintf("%u.%u%s%u", MAJOR_VERSION, MINOR_VERSION, BUILD_TYPE, BUILD_NUMBER);
}


unsigned OpalGetMajorVersion()
{
  return MAJOR_VERSION;
}

unsigned OpalGetMinorVersion()
{
  return MINOR_VERSION;
}

unsigned OpalGetBuildNumber()
{
  return BUILD_NUMBER;
}



/////////////////////////////////////////////////////////////////////////////

#ifdef _MSC_VER
#pragma warning(disable:4355)
#endif

OpalManager::OpalManager()
  : defaultUserName(PProcess::Current().GetUserName()),
    defaultDisplayName(defaultUserName),
    mediaFormatOrder(PARRAYSIZE(DefaultMediaFormatOrder), DefaultMediaFormatOrder),
    noMediaTimeout(0, 0, 5),     // Minutes
    translationAddress(0),       // Invalid address to disable
    activeCalls(*this)
{
  autoStartReceiveVideo = autoStartTransmitVideo = TRUE;

  rtpIpPorts.current = rtpIpPorts.base = 5000;
  rtpIpPorts.max = 5199;

  // use dynamic port allocation by default
  tcpPorts.current = tcpPorts.base = tcpPorts.max = 0;
  udpPorts.current = udpPorts.base = udpPorts.max = 0;

  stun = NULL;

  clearingAllCalls = FALSE;

#ifdef _WIN32
  rtpIpTypeofService = IPTOS_PREC_CRITIC_ECP|IPTOS_LOWDELAY;
#else
  // Don't use IPTOS_PREC_CRITIC_ECP on Unix platforms as then need to be root
  rtpIpTypeofService = IPTOS_LOWDELAY;
#endif

  minAudioJitterDelay = 50;  // milliseconds
  maxAudioJitterDelay = 250; // milliseconds

  PStringList drivers = PVideoInputDevice::GetDriverNames();
  if (drivers.GetSize() > 0) {
    PStringList devices = PVideoInputDevice::GetDriversDeviceNames(drivers[0]);
    if (devices.GetSize() > 0)
      videoInputDevice.deviceName = devices[0];
  }

  lastCallTokenID = 1;

  garbageCollector = PThread::Create(PCREATE_NOTIFIER(GarbageMain), 0,
                                     PThread::NoAutoDeleteThread,
                                     PThread::LowPriority,
                                     "OpalGarbage");

  PTRACE(3, "OpalMan\tCreated manager.");
}

#ifdef _MSC_VER
#pragma warning(default:4355)
#endif


OpalManager::~OpalManager()
{
  // Clear any pending calls on this endpoint
  ClearAllCalls();

  // Shut down the cleaner thread
  garbageCollectExit.Signal();
  garbageCollector->WaitForTermination();

  // Clean up any calls that the cleaner thread missed
  GarbageCollection();

  delete garbageCollector;

  // Kill off the endpoints, could wait till compiler generated destructor but
  // prefer to keep the PTRACE's in sequence.
  endpoints.RemoveAll();

  delete stun;

  PTRACE(3, "OpalMan\tDeleted manager.");
}


void OpalManager::AttachEndPoint(OpalEndPoint * endpoint)
{
  if (PAssertNULL(endpoint) == NULL)
    return;

  inUseFlag.Wait();

  if (endpoints.GetObjectsIndex(endpoint) == P_MAX_INDEX)
    endpoints.Append(endpoint);

  inUseFlag.Signal();
}


void OpalManager::DetachEndPoint(OpalEndPoint * endpoint)
{
  if (PAssertNULL(endpoint) == NULL)
    return;

  inUseFlag.Wait();
  endpoints.Remove(endpoint);
  inUseFlag.Signal();
}


OpalEndPoint * OpalManager::FindEndPoint(const PString & prefix)
{
  PWaitAndSignal mutex(inUseFlag);

  for (PINDEX i = 0; i < endpoints.GetSize(); i++) {
    if (endpoints[i].GetPrefixName() *= prefix)
      return &endpoints[i];
  }

  return NULL;
}


BOOL OpalManager::SetUpCall(const PString & partyA,
                            const PString & partyB,
                            PString & token)
{
  PTRACE(3, "OpalMan\tSet up call from " << partyA << " to " << partyB);

  OpalCall * call = CreateCall();
  token = call->GetToken();

  call->SetPartyB(partyB);

  if (MakeConnection(*call, partyA))
    return TRUE;

  call->Clear();
  return FALSE;
}


void OpalManager::OnEstablishedCall(OpalCall & /*call*/)
{
}


BOOL OpalManager::IsCallEstablished(const PString & token)
{
  PSafePtr<OpalCall> call = activeCalls.FindWithLock(token, PSafeReadOnly);
  if (call == NULL)
    return FALSE;

  return call->IsEstablished();
}


BOOL OpalManager::ClearCall(const PString & token,
                            OpalConnection::CallEndReason reason,
                            PSyncPoint * sync)
{
  /*The hugely multi-threaded nature of the OpalCall objects means that
    to avoid many forms of race condition, a call is cleared by moving it from
    the "active" call dictionary to a list of calls to be cleared that will be
    processed by a background thread specifically for the purpose of cleaning
    up cleared calls. So that is all that this function actually does.
    The real work is done in the OpalGarbageCollector thread.
   */

  {
    // Find the call by token, callid or conferenceid
    PSafePtr<OpalCall> call = activeCalls.FindWithLock(token);
    if (call == NULL)
      return FALSE;

    call->Clear(reason, sync);
  }

  if (sync != NULL)
    sync->Wait();

  return TRUE;
}


BOOL OpalManager::ClearCallSynchronous(const PString & token,
                                       OpalConnection::CallEndReason reason)
{
  PSyncPoint wait;
  return ClearCall(token, reason, &wait);
}


void OpalManager::ClearAllCalls(OpalConnection::CallEndReason reason, BOOL wait)
{
  // Remove all calls from the active list first
  for (PSafePtr<OpalCall> call = activeCalls; call != NULL; ++call) {
    call->Clear(reason);
  }

  if (wait) {
    clearingAllCalls = TRUE;
    allCallsCleared.Wait();
    clearingAllCalls = FALSE;
  }
}


void OpalManager::OnClearedCall(OpalCall & PTRACE_PARAM(call))
{
  PTRACE(3, "OpalMan\tOnClearedCall \"" << call.GetPartyA() << "\" to \"" << call.GetPartyB() << '"');
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
  inUseFlag.Wait();
  token.sprintf("%u", lastCallTokenID++);
  inUseFlag.Signal();
  return token;
}


BOOL OpalManager::MakeConnection(OpalCall & call, const PString & remoteParty)
{
  PTRACE(3, "OpalMan\tSet up connection to \"" << remoteParty << '"');

  if (endpoints.IsEmpty())
    return FALSE;

  PCaselessString epname = remoteParty.Left(remoteParty.Find(':'));
  if (epname.IsEmpty())
    epname = endpoints[0].GetPrefixName();

  for (PINDEX i = 0; i < endpoints.GetSize(); i++) {
    if (epname == endpoints[i].GetPrefixName()) {
      if (endpoints[i].MakeConnection(call, remoteParty, NULL))
        return TRUE;
    }
  }

  PTRACE(1, "OpalMan\tCould not find endpoint to handle protocol \"" << epname << '"');
  return FALSE;
}


BOOL OpalManager::OnIncomingConnection(OpalConnection & connection)
{
  PTRACE(3, "OpalMan\tOn incoming connection " << connection);

  OpalCall & call = connection.GetCall();
  if (call.GetOtherPartyConnection(connection) != NULL)
    return TRUE;

  // See if have pre-allocated B party address, otherwise use routing algorithm
  PString destinationAddress = call.GetPartyB();
  if (destinationAddress.IsEmpty())
    destinationAddress = OnRouteConnection(connection);

  // Nowhere to go
  if (destinationAddress.IsEmpty())
    return FALSE;

  return MakeConnection(call, destinationAddress);
}


PString OpalManager::OnRouteConnection(OpalConnection & connection)
{
  PString addr = connection.GetDestinationAddress();

  // No address, fail call
  if (addr.IsEmpty())
    return PString::Empty();

  // Have explicit protocol defined, so use that
  PINDEX colon = addr.Find(':');
  if (colon != P_MAX_INDEX && FindEndPoint(addr.Left(colon)) != NULL)
    return addr;

  // No routes specified, just return what we've got so far
  if (routeTable.IsEmpty())
    return addr;

  return ApplyRouteTable(connection.GetEndPoint().GetPrefixName(), addr);
}


void OpalManager::OnAlerting(OpalConnection & connection)
{
  PTRACE(3, "OpalMan\tOnAlerting " << connection);

  connection.GetCall().OnAlerting(connection);
}

OpalConnection::AnswerCallResponse
       OpalManager::OnAnswerCall(OpalConnection & connection,
                                  const PString & caller)
{
  return connection.GetCall().OnAnswerCall(connection, caller);
}

void OpalManager::OnConnected(OpalConnection & connection)
{
  PTRACE(3, "OpalMan\tOnConnected " << connection);

  connection.GetCall().OnConnected(connection);
}


void OpalManager::OnEstablished(OpalConnection & connection)
{
  PTRACE(3, "OpalMan\tOnEstablished " << connection);

  connection.GetCall().OnEstablished(connection);
}


void OpalManager::OnReleased(OpalConnection & connection)
{
  PTRACE(3, "OpalMan\tOnReleased " << connection);

  connection.GetCall().OnReleased(connection);
}


void OpalManager::OnHold(OpalConnection & PTRACE_PARAM(connection))
{
  PTRACE(3, "OpalMan\tOnHold " << connection);
}


void OpalManager::AdjustMediaFormats(const OpalConnection & /*connection*/,
                                     OpalMediaFormatList & mediaFormats) const
{
  mediaFormats.Remove(mediaFormatMask);
  mediaFormats.Reorder(mediaFormatOrder);
}


BOOL OpalManager::IsMediaBypassPossible(const OpalConnection & source,
                                        const OpalConnection & destination,
                                        unsigned sessionID) const
{
  PTRACE(3, "OpalMan\tIsMediaBypassPossible: session " << sessionID);

  return source.IsMediaBypassPossible(sessionID) &&
         destination.IsMediaBypassPossible(sessionID);
}


BOOL OpalManager::OnOpenMediaStream(OpalConnection & connection,
                                    OpalMediaStream & stream)
{
  PTRACE(3, "OpalMan\tOnOpenMediaStream " << connection << ',' << stream);

  if (stream.IsSource() && stream.RequiresPatchThread())
    return connection.GetCall().PatchMediaStreams(connection, stream);

  return TRUE;
}


void OpalManager::OnClosedMediaStream(const OpalMediaStream & /*channel*/)
{
}


void OpalManager::AddVideoMediaFormats(OpalMediaFormatList & mediaFormats,
                                       const OpalConnection * /*connection*/) const
{
  if (!videoInputDevice.deviceName) {
    mediaFormats += OPAL_RGB24;
    mediaFormats += OPAL_RGB32;
    mediaFormats += OPAL_YUV420P;
  }
}


PVideoInputDevice * OpalManager::CreateVideoInputDevice(const OpalConnection & /*connection*/)
{
  PStringList drivers = PVideoInputDevice::GetDriverNames();
  for (PINDEX i = 0; i < drivers.GetSize(); i++) {
    PVideoInputDevice * videoDevice = PVideoInputDevice::CreateDevice(drivers[i]);
    if (videoDevice != NULL) {
      if (videoDevice->OpenFull(videoInputDevice, FALSE))
        return videoDevice;
      delete videoDevice;
    }
  }

  return NULL;
}


PVideoOutputDevice * OpalManager::CreateVideoOutputDevice(const OpalConnection & /*connection*/)
{
  PStringList drivers = PVideoOutputDevice::GetDriverNames();
  for (PINDEX i = 0; i < drivers.GetSize(); i++) {
    PVideoOutputDevice * videoDevice = PVideoOutputDevice::CreateDevice(drivers[i]);
    if (videoDevice != NULL) {
      if (videoDevice->OpenFull(videoOutputDevice, FALSE))
        return videoDevice;
      delete videoDevice;
    }
  }

  return NULL;
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


void OpalManager::OnUserInputString(OpalConnection & connection,
                                    const PString & value)
{
  connection.GetCall().OnUserInputString(connection, value);
}


void OpalManager::OnUserInputTone(OpalConnection & connection,
                                  char tone,
                                  int duration)
{
  connection.GetCall().OnUserInputTone(connection, tone, duration);
}


OpalT120Protocol * OpalManager::CreateT120ProtocolHandler(const OpalConnection & ) const
{
  return NULL;
}


OpalT38Protocol * OpalManager::CreateT38ProtocolHandler(const OpalConnection & ) const
{
  return NULL;
}

BOOL OpalManager::GetExternalRTPAddress(
      const OpalConnection & connection,      /// connection using external RTP
      unsigned sessionID,                     /// RTP session ID
      OpalTransportAddress & data,            /// return data address
      OpalTransportAddress & control          /// return control address
)
{
  return FALSE;
}

OpalManager::RouteEntry::RouteEntry(const PString & pat, const PString & dest)
  : pattern(pat),
    destination(dest),
    regex('^'+pat+'$')
{
}


void OpalManager::RouteEntry::PrintOn(ostream & strm) const
{
  strm << pattern << '=' << destination;
}


BOOL OpalManager::AddRouteEntry(const PString & spec)
{
  if (spec[0] == '#') // Comment
    return FALSE;

  if (spec[0] == '@') { // Load from file
    PTextFile file;
    if (!file.Open(spec.Mid(1), PFile::ReadOnly)) {
      PTRACE(1, "OpalMan\tCould not open route file \"" << file.GetFilePath() << '"');
      return FALSE;
    }
    PTRACE(4, "OpalMan\tAdding routes from file \"" << file.GetFilePath() << '"');
    BOOL ok = FALSE;
    PString line;
    while (file.good()) {
      file >> line;
      if (AddRouteEntry(line))
        ok = TRUE;
    }
    return ok;
  }

  PINDEX equal = spec.Find('=');
  if (equal == P_MAX_INDEX) {
    PTRACE(2, "OpalMan\tInvalid route table entry: \"" << spec << '"');
    return FALSE;
  }

  RouteEntry * entry = new RouteEntry(spec.Left(equal).Trim(), spec.Mid(equal+1).Trim());
  if (entry->regex.GetErrorCode() != PRegularExpression::NoError) {
    PTRACE(1, "OpalMan\tIllegal regular expression in route table entry: \"" << spec << '"');
    delete entry;
    return FALSE;
  }

  PTRACE(4, "OpalMan\tAdded route \"" << *entry << '"');
  routeTableMutex.Wait();
  routeTable.Append(entry);
  routeTableMutex.Signal();
  return TRUE;
}


BOOL OpalManager::SetRouteTable(const PStringArray & specs)
{
  BOOL ok = FALSE;

  routeTableMutex.Wait();
  routeTable.RemoveAll();

  for (PINDEX i = 0; i < specs.GetSize(); i++) {
    if (AddRouteEntry(specs[i].Trim()))
      ok = TRUE;
  }

  routeTableMutex.Signal();

  return ok;
}


void OpalManager::SetRouteTable(const RouteTable & table)
{
  routeTableMutex.Wait();
  routeTable = table;
  routeTable.MakeUnique();
  routeTableMutex.Signal();
}


PString OpalManager::ApplyRouteTable(const PString & proto, const PString & addr)
{
  PWaitAndSignal mutex(routeTableMutex);

  PString destination;
  PString search = proto + ':' + addr;
  PTRACE(4, "OpalMan\tSearching for route \"" << search << '"');
  for (PINDEX i = 0; i < routeTable.GetSize(); i++) {
    RouteEntry & entry = routeTable[i];
    PINDEX pos;
    if (entry.regex.Execute(search, pos)) {
      destination = routeTable[i].destination;
      break;
    }
  }

  if (destination.IsEmpty())
    return PString::Empty();

  destination.Replace("<da>", addr);

  PINDEX pos;
  if ((pos = destination.Find("<dn>")) != P_MAX_INDEX)
    destination.Splice(addr.Left(::strspn(addr, "0123456789*#")), pos, 4);

  if ((pos = destination.Find("<!dn>")) != P_MAX_INDEX)
    destination.Splice(addr.Mid(::strspn(addr, "0123456789*#")), pos, 5);

  // Do meta character substitutions
  if ((pos = destination.Find("<dn2ip>")) != P_MAX_INDEX) {
    PStringStream route;
    PStringArray stars = addr.Tokenise('*');
    switch (stars.GetSize()) {
      case 0 :
      case 1 :
      case 2 :
      case 3 :
        route << addr;
        break;

      case 4 :
        route << stars[0] << '.' << stars[1] << '.'<< stars[2] << '.'<< stars[3];
        break;

      case 5 :
        route << stars[0] << '@'
              << stars[1] << '.' << stars[2] << '.'<< stars[3] << '.'<< stars[4];
        break;

      default :
        route << stars[0] << '@'
              << stars[1] << '.' << stars[2] << '.'<< stars[3] << '.'<< stars[4]
              << ':' << stars[5];
        break;
    }
    destination.Splice(route, pos, 7);
  }

  return destination;
}


BOOL OpalManager::IsLocalAddress(const PIPSocket::Address & ip) const
{
  /* Check if the remote address is a private IP, broadcast, or us */
  return ip.IsRFC1918() || ip.IsBroadcast() || PIPSocket::IsLocalHost(ip);
}


BOOL OpalManager::TranslateIPAddress(PIPSocket::Address & localAddress,
                                     const PIPSocket::Address & remoteAddress)
{
  if (!translationAddress.IsValid())
    return FALSE; // Have nothing to translate it to

  if (!IsLocalAddress(localAddress))
    return FALSE; // Is already translated

  if (IsLocalAddress(remoteAddress))
    return FALSE; // Does not need to be translated

  // Tranlsate it!
  localAddress = translationAddress;
  return TRUE;
}


PSTUNClient * OpalManager::GetSTUN(const PIPSocket::Address & ip) const
{
  if (ip.IsValid() && IsLocalAddress(ip))
    return NULL;

  return stun;
}


PSTUNClient::NatTypes OpalManager::SetSTUNServer(const PString & server)
{
  delete stun;

  if (server.IsEmpty()) {
    stun = NULL;
    return PSTUNClient::UnknownNat;
  }

    stun = new PSTUNClient(server,
                           GetUDPPortBase(), GetUDPPortMax(),
                           GetRtpIpPortBase(), GetRtpIpPortMax());
  PSTUNClient::NatTypes type = stun->GetNatType();
  if (type != PSTUNClient::BlockedNat)
    stun->GetExternalAddress(translationAddress);

  PTRACE(2, "OPAL\tSTUN server \"" << server << "\" replies " << type << ", external IP " << translationAddress);

  return type;
}


void OpalManager::PortInfo::Set(unsigned newBase,
                                unsigned newMax,
                                unsigned range,
                                unsigned dflt)
{
  if (newBase == 0) {
    newBase = dflt;
    newMax = dflt;
    if (dflt > 0)
      newMax += range;
  }
  else {
    if (newBase < 1024)
      newBase = 1024;
    else if (newBase > 65500)
      newBase = 65500;

    if (newMax <= newBase)
      newMax = newBase + range;
    if (newMax > 65535)
      newMax = 65535;
  }

  mutex.Wait();

  current = base = (WORD)newBase;
  max = (WORD)newMax;

  mutex.Signal();
}


WORD OpalManager::PortInfo::GetNext(unsigned increment)
{
  PWaitAndSignal m(mutex);

  if (current < base || current >= (max-increment))
    current = base;

  if (current == 0)
    return 0;

  WORD p = current;
  current = (WORD)(current + increment);
  return p;
}


void OpalManager::SetTCPPorts(unsigned tcpBase, unsigned tcpMax)
{
  tcpPorts.Set(tcpBase, tcpMax, 49, 0);
}


WORD OpalManager::GetNextTCPPort()
{
  return tcpPorts.GetNext(1);
}


void OpalManager::SetUDPPorts(unsigned udpBase, unsigned udpMax)
{
  udpPorts.Set(udpBase, udpMax, 99, 0);

  if (stun != NULL)
    stun->SetPortRanges(GetUDPPortBase(), GetUDPPortMax(), GetRtpIpPortBase(), GetRtpIpPortMax());
}


WORD OpalManager::GetNextUDPPort()
{
  return udpPorts.GetNext(1);
}


void OpalManager::SetRtpIpPorts(unsigned rtpIpBase, unsigned rtpIpMax)
{
  rtpIpPorts.Set((rtpIpBase+1)&0xfffe, rtpIpMax&0xfffe, 199, 5000);

  if (stun != NULL)
    stun->SetPortRanges(GetUDPPortBase(), GetUDPPortMax(), GetRtpIpPortBase(), GetRtpIpPortMax());
}


WORD OpalManager::GetRtpIpPortPair()
{
  return rtpIpPorts.GetNext(2);
}


void OpalManager::SetAudioJitterDelay(unsigned minDelay, unsigned maxDelay)
{
  PAssert(minDelay <= 10000 && maxDelay <= 10000, PInvalidParameter);

  if (minDelay < 10)
    minDelay = 10;
  minAudioJitterDelay = minDelay;

  if (maxDelay < minDelay)
    maxDelay = minDelay;
  maxAudioJitterDelay = maxDelay;
}


BOOL OpalManager::SetVideoInputDevice(const PVideoDevice::OpenArgs & args)
{
  PStringList drivers = PVideoInputDevice::GetDriverNames();
  for (PINDEX i = 0; i < drivers.GetSize(); i++) {
    PStringList devices = PVideoInputDevice::GetDriversDeviceNames(drivers[i]);
    if (args.deviceName[0] == '#') {
      PINDEX id = args.deviceName.Mid(1).AsUnsigned();
      if (id > 0 && id <= devices.GetSize()) {
        videoInputDevice = args;
        videoInputDevice.deviceName = devices[id-1];
        return TRUE;
      }
    }
    else {
      if (devices.GetValuesIndex(args.deviceName) != P_MAX_INDEX) {
        videoInputDevice = args;
        return TRUE;
      }
    }
  }

  return FALSE;
}


BOOL OpalManager::SetVideoOutputDevice(const PVideoDevice::OpenArgs & args)
{
  videoOutputDevice = args;
  return TRUE;
}


BOOL OpalManager::SetNoMediaTimeout(const PTimeInterval & newInterval) 
{
  if (newInterval < 10)
    return FALSE;

  PWaitAndSignal mutex(inUseFlag);

  noMediaTimeout = newInterval; 
  return TRUE; 
}


void OpalManager::GarbageCollection()
{
  BOOL allCleared = activeCalls.DeleteObjectsToBeRemoved();
  for (PINDEX i = 0; i < endpoints.GetSize(); i++) {
    if (!endpoints[i].connectionsActive.DeleteObjectsToBeRemoved())
      allCleared = FALSE;
  }
  if (allCleared && clearingAllCalls)
    allCallsCleared.Signal();
}


void OpalManager::CallDict::DeleteObject(PObject * object) const
{
  manager.DestroyCall(PDownCast(OpalCall, object));
}


void OpalManager::GarbageMain(PThread &, INT)
{
  while (!garbageCollectExit.Wait(1000))
      GarbageCollection();
}

/////////////////////////////////////////////////////////////////////////////
