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
 * $Revision$
 * $Author$
 * $Date$
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

#if OPAL_VIDEO
#include <codec/vidcodec.h>
#endif

#include <h224/h224handler.h>
#include <h224/h281handler.h>

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


OpalProductInfo::OpalProductInfo()
  : vendor(PProcess::Current().GetManufacturer())
  , name(PProcess::Current().GetName())
  , version(PProcess::Current().GetVersion())
  , t35CountryCode(9)     // Country code for Australia
  , t35Extension(0)       // No extension code for Australia
  , manufacturerCode(61)  // Allocated by Australian Communications Authority, Oct 2000;
{
}

OpalProductInfo & OpalProductInfo::Default()
{
  static OpalProductInfo instance;
  return instance;
}


PCaselessString OpalProductInfo::AsString() const
{
  PStringStream str;
  str << name << '\t' << version << '\t';
  if (t35CountryCode != 0 && manufacturerCode != 0) {
    str << (unsigned)t35CountryCode;
    if (t35Extension != 0)
      str << '.' << (unsigned)t35Extension;
    str << '/' << manufacturerCode;
  }
  str << '\t' << vendor;
  return str;
}


/////////////////////////////////////////////////////////////////////////////

#ifdef _MSC_VER
#pragma warning(disable:4355)
#endif

OpalManager::OpalManager()
  : defaultUserName(PProcess::Current().GetUserName())
  , defaultDisplayName(defaultUserName)
#ifdef _WIN32
  , rtpIpTypeofService(IPTOS_PREC_CRITIC_ECP|IPTOS_LOWDELAY)
#else
  , rtpIpTypeofService(IPTOS_LOWDELAY) // Don't use IPTOS_PREC_CRITIC_ECP on Unix platforms as then need to be root
#endif
  , rtpPayloadSizeMax(576-20-16-12) // Max safe MTU size (576 bytes as per RFC879) minus IP, UDP an RTP headers
  , minAudioJitterDelay(50)  // milliseconds
  , maxAudioJitterDelay(250) // milliseconds
  , mediaFormatOrder(PARRAYSIZE(DefaultMediaFormatOrder), DefaultMediaFormatOrder)
  , noMediaTimeout(0, 0, 5)     // Minutes
  , translationAddress(0)       // Invalid address to disable
  , stun(NULL)
  , interfaceMonitor(NULL)
  , activeCalls(*this)
  , clearingAllCalls(PFalse)
#if OPAL_RTP_AGGREGATE
  , useRTPAggregation(false)
#endif
{
  rtpIpPorts.current = rtpIpPorts.base = 5000;
  rtpIpPorts.max = 5999;

  // use dynamic port allocation by default
  tcpPorts.current = tcpPorts.base = tcpPorts.max = 0;
  udpPorts.current = udpPorts.base = udpPorts.max = 0;

#ifndef NO_OPAL_VIDEO
  PStringArray devices = PVideoInputDevice::GetDriversDeviceNames("*"); // Get all devices on all drivers
  PINDEX i;
  for (i = 0; i < devices.GetSize(); ++i) {
    if ((devices[i] *= "*.yuv") || (devices[i] *= "fake")) 
      continue;
    videoInputDevice.deviceName = devices[i];
    break;
  }
  autoStartTransmitVideo = !videoInputDevice.deviceName.IsEmpty();

  devices = PVideoOutputDevice::GetDriversDeviceNames("*"); // Get all devices on all drivers
  for (i = 0; i < devices.GetSize(); ++i) {
    if ((devices[i] *= "*.yuv") || (devices[i] *= "null"))
      continue;
    videoOutputDevice.deviceName = devices[i];
    break;
  }
  autoStartReceiveVideo = !videoOutputDevice.deviceName.IsEmpty();

  if (autoStartReceiveVideo)
    videoPreviewDevice = videoOutputDevice;
#endif

  garbageCollector = PThread::Create(PCREATE_NOTIFIER(GarbageMain), "OpalGarbage");

  PTRACE(4, "OpalMan\tCreated manager.");
}

#ifdef _MSC_VER
#pragma warning(default:4355)
#endif


OpalManager::~OpalManager()
{
  ShutDownEndpoints();

  // Shut down the cleaner thread
  garbageCollectExit.Signal();
  garbageCollector->WaitForTermination();

  // Clean up any calls that the cleaner thread missed on the way out
  GarbageCollection();

  delete garbageCollector;

  delete stun;
  delete interfaceMonitor;

  PTRACE(4, "OpalMan\tDeleted manager.");
}


PList<OpalEndPoint> OpalManager::GetEndPoints() const
{
  PList<OpalEndPoint> list;
  list.AllowDeleteObjects(false);

  PReadWaitAndSignal mutex(endpointsMutex);

  for (PList<OpalEndPoint>::const_iterator it = endpointList.begin(); it != endpointList.end(); ++it)
    list.Append((OpalEndPoint *)&*it);

  return list;
}


void OpalManager::ShutDownEndpoints()
{
  // Clear any pending calls, set flag so no calls can be received before endpoints removed
  clearingAllCalls = true;
  ClearAllCalls();

  // Deregister the endpoints
  endpointsMutex.StartRead();
  for (PList<OpalEndPoint>::iterator ep = endpointList.begin(); ep != endpointList.end(); ++ep)
    ep->ShutDown();
  endpointsMutex.EndRead();

  endpointsMutex.StartWrite();
  endpointMap.clear();
  endpointList.RemoveAll();
  endpointsMutex.EndWrite();
}


void OpalManager::AttachEndPoint(OpalEndPoint * endpoint, const PString & prefix)
{
  if (PAssertNULL(endpoint) == NULL)
    return;

  PString thePrefix = prefix.IsEmpty() ? endpoint->GetPrefixName() : prefix;

  PWriteWaitAndSignal mutex(endpointsMutex);

  if (endpointMap.find(thePrefix) != endpointMap.end()) {
    PTRACE(1, "OpalMan\tCannot re-register endpoint prefix " << thePrefix);
    return;
  }

  if (endpointList.GetObjectsIndex(endpoint) == P_MAX_INDEX)
    endpointList.Append(endpoint);
  endpointMap[thePrefix] = endpoint;
  PTRACE(1, "OpalMan\tRegistered endpoint with prefix " << thePrefix);
}


void OpalManager::DetachEndPoint(OpalEndPoint * endpoint)
{ 
  if (PAssertNULL(endpoint) == NULL)
    return;

  endpoint->ShutDown();

  endpointsMutex.StartWrite();

  if (endpointList.Remove(endpoint)) {
    // Was in list, remove from map too
    std::map<PString, OpalEndPoint *>::iterator it = endpointMap.begin();
    while (it != endpointMap.end()) {
      if (it->second != endpoint)
        ++it;
      else {
        endpointMap.erase(it);
        it = endpointMap.begin();
      }
    }
  }

  endpointsMutex.EndWrite();
}


void OpalManager::DetachEndPoint(const PString & prefix)
{
  PReadWaitAndSignal mutex(endpointsMutex);

  std::map<PString, OpalEndPoint *>::iterator it = endpointMap.find(prefix);
  if (it == endpointMap.end())
    return;

  OpalEndPoint * endpoint = it->second;

  endpointsMutex.StartWrite();
  endpointMap.erase(it);
  endpointsMutex.EndWrite();

  // See if other references
  for (it = endpointMap.begin(); it != endpointMap.end(); ++it) {
    if (it->second == endpoint)
      return; // Still a reference to it
  }

  // Last copy, delete it now
  DetachEndPoint(endpoint);
}


OpalEndPoint * OpalManager::FindEndPoint(const PString & prefix)
{
  PReadWaitAndSignal mutex(endpointsMutex);
  std::map<PString, OpalEndPoint *>::iterator it = endpointMap.find(prefix);
  return it != endpointMap.end() ? it->second : NULL;
}


PBoolean OpalManager::SetUpCall(const PString & partyA,
                            const PString & partyB,
                            PString & token,
                            void * userData,
                            unsigned int options,
                            OpalConnection::StringOptions * stringOptions)
{
  if (clearingAllCalls) {
    PTRACE(2, "OpalMan\tSet up call not performed as clearing all calls.");
    return false;
  }

  PTRACE(3, "OpalMan\tSet up call from " << partyA << " to " << partyB);

  OpalCall * call = CreateCall(userData);
  token = call->GetToken();

  call->SetPartyB(partyB);

  // If we are the A-party then need to initiate a call now in this thread and
  // go through the routing engine via OnIncomingConnection. If we were the
  // B-Party then SetUpConnection() gets called in the context of the A-party
  // thread.
  if (MakeConnection(*call, partyA, userData, options, stringOptions) && call->GetConnection(0)->SetUpConnection()) {
    PTRACE(3, "OpalMan\tSetUpCall succeeded, call=" << *call);
    return PTrue;
  }

  PSafePtr<OpalConnection> connection = call->GetConnection(0);
  OpalConnection::CallEndReason endReason = connection != NULL ? connection->GetCallEndReason() : OpalConnection::NumCallEndReasons;
  call->Clear(endReason != OpalConnection::NumCallEndReasons ? endReason : OpalConnection::EndedByTemporaryFailure);

  if (!activeCalls.RemoveAt(token)) {
    PTRACE(2, "OpalMan\tSetUpCall could not remove call from active call list");
  }

  token.MakeEmpty();

  return PFalse;
}


void OpalManager::OnEstablishedCall(OpalCall & /*call*/)
{
}


PBoolean OpalManager::IsCallEstablished(const PString & token)
{
  PSafePtr<OpalCall> call = activeCalls.FindWithLock(token, PSafeReadOnly);
  if (call == NULL)
    return PFalse;

  return call->IsEstablished();
}


PBoolean OpalManager::ClearCall(const PString & token,
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
    PSafePtr<OpalCall> call = activeCalls.FindWithLock(token, PSafeReference);
    if (call == NULL)
      return PFalse;

    call->Clear(reason, sync);
  }

  if (sync != NULL)
    sync->Wait();

  return PTrue;
}


PBoolean OpalManager::ClearCallSynchronous(const PString & token,
                                       OpalConnection::CallEndReason reason)
{
  PSyncPoint wait;
  return ClearCall(token, reason, &wait);
}


void OpalManager::ClearAllCalls(OpalConnection::CallEndReason reason, PBoolean wait)
{
  bool oldFlag = clearingAllCalls;
  clearingAllCalls = true;

  // Remove all calls from the active list first
  for (PSafePtr<OpalCall> call = activeCalls; call != NULL; ++call)
    call->Clear(reason);

  if (wait)
    allCallsCleared.Wait();

  clearingAllCalls = oldFlag;
}


void OpalManager::OnClearedCall(OpalCall & PTRACE_PARAM(call))
{
  PTRACE(3, "OpalMan\tOnClearedCall " << call << " from \"" << call.GetPartyA() << "\" to \"" << call.GetPartyB() << '"');
}


OpalCall * OpalManager::InternalCreateCall()
{
  if (clearingAllCalls) {
    PTRACE(2, "OpalMan\tCreate call not performed as clearing all calls.");
    return NULL;
  }

  return CreateCall(NULL);
}


OpalCall * OpalManager::CreateCall()
{
  return new OpalCall(*this);
}

OpalCall * OpalManager::CreateCall(void * /*userData*/)
{
  return CreateCall();
}


void OpalManager::DestroyCall(OpalCall * call)
{
  delete call;
}


PString OpalManager::GetNextCallToken()
{
  return psprintf("%u", ++lastCallTokenID);
}

PBoolean OpalManager::MakeConnection(OpalCall & call, const PString & remoteParty, void * userData, unsigned int options, OpalConnection::StringOptions * stringOptions)
{
  PTRACE(3, "OpalMan\tSet up connection to \"" << remoteParty << '"');

  if (remoteParty.IsEmpty())
    return PFalse;

  PCaselessString epname = remoteParty.Left(remoteParty.Find(':'));

  PReadWaitAndSignal mutex(endpointsMutex);

  OpalEndPoint * ep = NULL;
  if (epname.IsEmpty()) {
    if (endpointMap.size() > 0)
      ep = endpointMap.begin()->second;
  }
  else
    ep = FindEndPoint(epname);

  if (ep != NULL) {
    if (ep->MakeConnection(call, remoteParty, userData, options, stringOptions))
      return PTrue;
    PTRACE(1, "OpalMan\tCould not use endpoint for protocol \"" << epname << '"');
  }
  else {
    PTRACE(1, "OpalMan\tCould not find endpoint to handle protocol \"" << epname << '"');
  }
  return PFalse;
}

PBoolean OpalManager::OnIncomingConnection(OpalConnection & /*connection*/)
{
  return PTrue;
}

PBoolean OpalManager::OnIncomingConnection(OpalConnection & /*connection*/, unsigned /*options*/)
{
  return PTrue;
}

PBoolean OpalManager::OnIncomingConnection(OpalConnection & connection, unsigned options, OpalConnection::StringOptions * stringOptions)
{
  PTRACE(3, "OpalMan\tOn incoming connection " << connection);

  if (!OnIncomingConnection(connection))
    return PFalse;

  if (!OnIncomingConnection(connection, options))
    return PFalse;

  OpalCall & call = connection.GetCall();

  // See if we already have a B-Party in the call. If not, make one.
  if (call.GetOtherPartyConnection(connection) != NULL)
    return PTrue;

  // Use a routing algorithm to figure out who the B-Party is, then make a connection
  PINDEX tableEntry = 0;
  for (;;) {
    PString destination = OnRouteConnection(connection);
    if (destination.IsEmpty()) {
      PTRACE(3, "OpalMan\tCould not find destination for " << connection);
      break;
    }

    destination = ApplyRouteTable(connection.GetLocalPartyURL(), destination, tableEntry);
    if (destination.IsEmpty()) {
      PTRACE(3, "OpalMan\tCould not route " << connection);
      break;
    }

    if (MakeConnection(call, destination, NULL, options, stringOptions))
      return true;
  }

  return false;
}


PString OpalManager::OnRouteConnection(OpalConnection & connection)
{
  // See if have pre-allocated B party address, otherwise use routing algorithm
  PString addr = connection.GetCall().GetPartyB();
  if (addr.IsEmpty())
    addr = connection.GetDestinationAddress();
  return addr;
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
  PTRACE(3, "OpalMan\tOnAnswerCall " << connection);

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


PBoolean OpalManager::OnForwarded(OpalConnection & PTRACE_PARAM(connection),
			      const PString & /*forwardParty*/)
{
  PTRACE(4, "OpalEP\tOnForwarded " << connection);
  return PTrue;
}


void OpalManager::AdjustMediaFormats(const OpalConnection & /*connection*/,
                                     OpalMediaFormatList & mediaFormats) const
{
  mediaFormats.Remove(mediaFormatMask);
  mediaFormats.Reorder(mediaFormatOrder);
}


PBoolean OpalManager::IsMediaBypassPossible(const OpalConnection & source,
                                        const OpalConnection & destination,
                                        unsigned sessionID) const
{
  PTRACE(3, "OpalMan\tIsMediaBypassPossible: session " << sessionID);

  return source.IsMediaBypassPossible(sessionID) &&
         destination.IsMediaBypassPossible(sessionID);
}


PBoolean OpalManager::OnOpenMediaStream(OpalConnection & PTRACE_PARAM(connection),
                                        OpalMediaStream & PTRACE_PARAM(stream))
{
  PTRACE(3, "OpalMan\tOnOpenMediaStream " << connection << ',' << stream);

  return PTrue;
}


void OpalManager::OnRTPStatistics(const OpalConnection & connection, const RTP_Session & session)
{
  connection.GetCall().OnRTPStatistics(connection, session);
}


void OpalManager::OnClosedMediaStream(const OpalMediaStream & /*channel*/)
{
}

#if OPAL_VIDEO

void OpalManager::AddVideoMediaFormats(OpalMediaFormatList & mediaFormats,
                                       const OpalConnection * /*connection*/) const
{
  if (videoInputDevice.deviceName.IsEmpty())
      return;

  mediaFormats += OpalYUV420P;
  mediaFormats += OpalRGB32;
  mediaFormats += OpalRGB24;
}


PBoolean OpalManager::CreateVideoInputDevice(const OpalConnection & /*connection*/,
                                         const OpalMediaFormat & mediaFormat,
                                         PVideoInputDevice * & device,
                                         PBoolean & autoDelete)
{
  // Make copy so we can adjust the size
  PVideoDevice::OpenArgs args = videoInputDevice;
  args.width = mediaFormat.GetOptionInteger(OpalVideoFormat::FrameWidthOption(), PVideoFrameInfo::QCIFWidth);
  args.height = mediaFormat.GetOptionInteger(OpalVideoFormat::FrameHeightOption(), PVideoFrameInfo::QCIFHeight);
  args.rate = mediaFormat.GetClockRate()/mediaFormat.GetFrameTime();

  autoDelete = PTrue;
  device = PVideoInputDevice::CreateOpenedDevice(args, false);
  PTRACE_IF(2, device == NULL, "OpalCon\tCould not open video device \"" << args.deviceName << '"');
  return device != NULL;
}


PBoolean OpalManager::CreateVideoOutputDevice(const OpalConnection & connection,
                                          const OpalMediaFormat & mediaFormat,
                                          PBoolean preview,
                                          PVideoOutputDevice * & device,
                                          PBoolean & autoDelete)
{
  // Donot use our one and only SDL window, if we need it for the video output.
  if (preview && videoPreviewDevice.driverName == "SDL" && videoOutputDevice.driverName == "SDL")
    return PFalse;

  // Make copy so we can adjust the size
  PVideoDevice::OpenArgs args = preview ? videoPreviewDevice : videoOutputDevice;
  args.width = mediaFormat.GetOptionInteger(OpalVideoFormat::FrameWidthOption(), PVideoFrameInfo::QCIFWidth);
  args.height = mediaFormat.GetOptionInteger(OpalVideoFormat::FrameHeightOption(), PVideoFrameInfo::QCIFHeight);
  args.rate = mediaFormat.GetClockRate()/mediaFormat.GetFrameTime();

  PINDEX start = args.deviceName.Find("TITLE=\"");
  if (start != P_MAX_INDEX) {
    start += 7;
    args.deviceName.Splice(preview ? "Local Preview" : connection.GetRemotePartyName(), start, args.deviceName.Find('"', start)-start);
  }

  autoDelete = PTrue;
  device = PVideoOutputDevice::CreateOpenedDevice(args, false);
  return device != NULL;
}

#endif // OPAL_VIDEO


OpalMediaPatch * OpalManager::CreateMediaPatch(OpalMediaStream & source,
                                               PBoolean requiresPatchThread)
{
  if (requiresPatchThread) {
    return new OpalMediaPatch(source);
  } else {
    return new OpalPassiveMediaPatch(source);
  }
}


void OpalManager::DestroyMediaPatch(OpalMediaPatch * patch)
{
  delete patch;
}


PBoolean OpalManager::OnStartMediaPatch(const OpalMediaPatch & /*patch*/)
{
  return PTrue;
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


PString OpalManager::ReadUserInput(OpalConnection & connection,
                                  const char * terminators,
                                  unsigned lastDigitTimeout,
                                  unsigned firstDigitTimeout)
{
  PTRACE(3, "OpalCon\tReadUserInput from " << connection);

  connection.PromptUserInput(PTrue);
  PString digit = connection.GetUserInput(firstDigitTimeout);
  connection.PromptUserInput(PFalse);

  if (digit.IsEmpty()) {
    PTRACE(2, "OpalCon\tReadUserInput first character timeout (" << firstDigitTimeout << "ms) on " << *this);
    return PString::Empty();
  }

  PString input;
  while (digit.FindOneOf(terminators) == P_MAX_INDEX) {
    input += digit;

    digit = connection.GetUserInput(lastDigitTimeout);
    if (digit.IsEmpty()) {
      PTRACE(2, "OpalCon\tReadUserInput last character timeout (" << lastDigitTimeout << "ms) on " << *this);
      return input; // Input so far will have to do
    }
  }

  return input.IsEmpty() ? digit : input;
}

#if OPAL_T120DATA

OpalT120Protocol * OpalManager::CreateT120ProtocolHandler(const OpalConnection & ) const
{
  return NULL;
}

#endif

#if OPAL_T38FAX

OpalT38Protocol * OpalManager::CreateT38ProtocolHandler(const OpalConnection & ) const
{
  return NULL;
}

#endif

#ifdef OPAL_H224

OpalH224Handler * OpalManager::CreateH224ProtocolHandler(OpalRTPConnection & connection, unsigned sessionID) const
{
  return new OpalH224Handler(connection, sessionID);
}

OpalH281Handler * OpalManager::CreateH281ProtocolHandler(OpalH224Handler & h224Handler) const
{
  return new OpalH281Handler(h224Handler);
}

#endif

OpalManager::RouteEntry::RouteEntry(const PString & pat, const PString & dest)
  : pattern(pat),
    destination(dest)
{
  PString adjustedPattern = '^';

  // Test for backward compatibility format
  PINDEX colon = pattern.Find(':');
  if (colon != P_MAX_INDEX && pattern.Find('\t', colon) == P_MAX_INDEX)
    adjustedPattern += pattern.Left(colon+1) + ".*\t" + pattern.Mid(colon+1);
  else
    adjustedPattern += pattern;

  adjustedPattern += '$';

  if (!regex.Compile(adjustedPattern)) {
    PTRACE(1, "OpalMan\tCould not compile route regular expression \"" << adjustedPattern << '"');
  }
}


void OpalManager::RouteEntry::PrintOn(ostream & strm) const
{
  strm << pattern << '=' << destination;
}


PBoolean OpalManager::AddRouteEntry(const PString & spec)
{
  if (spec[0] == '#') // Comment
    return PFalse;

  if (spec[0] == '@') { // Load from file
    PTextFile file;
    if (!file.Open(spec.Mid(1), PFile::ReadOnly)) {
      PTRACE(1, "OpalMan\tCould not open route file \"" << file.GetFilePath() << '"');
      return PFalse;
    }
    PTRACE(4, "OpalMan\tAdding routes from file \"" << file.GetFilePath() << '"');
    PBoolean ok = PFalse;
    PString line;
    while (file.good()) {
      file >> line;
      if (AddRouteEntry(line))
        ok = PTrue;
    }
    return ok;
  }

  PINDEX equal = spec.Find('=');
  if (equal == P_MAX_INDEX) {
    PTRACE(2, "OpalMan\tInvalid route table entry: \"" << spec << '"');
    return PFalse;
  }

  RouteEntry * entry = new RouteEntry(spec.Left(equal).Trim(), spec.Mid(equal+1).Trim());
  if (entry->regex.GetErrorCode() != PRegularExpression::NoError) {
    PTRACE(2, "OpalMan\tIllegal regular expression in route table entry: \"" << spec << '"');
    delete entry;
    return PFalse;
  }

  PTRACE(4, "OpalMan\tAdded route \"" << *entry << '"');
  routeTableMutex.Wait();
  routeTable.Append(entry);
  routeTableMutex.Signal();
  return PTrue;
}


PBoolean OpalManager::SetRouteTable(const PStringArray & specs)
{
  PBoolean ok = PFalse;

  routeTableMutex.Wait();
  routeTable.RemoveAll();

  for (PINDEX i = 0; i < specs.GetSize(); i++) {
    if (AddRouteEntry(specs[i].Trim()))
      ok = PTrue;
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


PString OpalManager::ApplyRouteTable(const PString & a_party, const PString & b_party, PINDEX & routeIndex)
{
  PWaitAndSignal mutex(routeTableMutex);

  if (routeTable.IsEmpty())
    return routeIndex++ == 0 ? b_party : PString::Empty();

  PString search = a_party + '\t' + b_party;
  PTRACE(4, "OpalMan\tSearching for route \"" << search << '"');

  /* Examples:
        Call from UI       pc:USB Audio Device\USB Audio Device      sip:fred@boggs.com
                           pc:USB Audio Device\USB Audio Device      h323:fred@boggs.com
                           pc:USB Audio Device\USB Audio Device      fred
        Call from handset  pots:TigerJet:USB Audio Device            123
        Call from SIP      sip:me@here.net                           sip:you@there.com
                           sip:me@here.net:5061                      sip:you@there.com
        Call from H.323    h323:me@here.net                          h323:there.com
                           h323:me@here.net:1721                     h323:fred

     Table:
        .*:#  = ivr:
        pots:.*\\*.*\\*.* = sip:<dn2ip>
        pots:.*           = sip:<da>
        pc:.*             = sip:<da>
        h323:.*           = pots:<dn>
        sip:.*            = pots:<dn>
        h323:.*           = pc:
        sip:.*            = pc:
   */

  PString destination;
  while (routeIndex < routeTable.GetSize()) {
    RouteEntry & entry = routeTable[routeIndex++];
    PINDEX pos;
    if (entry.regex.Execute(search, pos)) {
      if (entry.destination.NumCompare("label:") != EqualTo) {
        destination = entry.destination;
        break;
      }

      // restart search in table using label.
      search = entry.destination;
      routeIndex = 0;
    }
  }

  // No route found
  if (destination.IsEmpty())
    return PString::Empty();

  // We are backward compatibility mode and the supplied address can be called
  PINDEX colon = b_party.Find(':');
  if (colon == P_MAX_INDEX || FindEndPoint(b_party.Left(colon)) == NULL)
    colon = 0;
  else {
    if (destination.Find("<da>") != P_MAX_INDEX)
      return b_party;
    colon++;
  }

  PINDEX nonDigitPos = b_party.FindSpan("0123456789*#", colon);
  PString digits = b_party(colon, nonDigitPos-1);

  PINDEX at = b_party.Find('@', colon);

  destination.Replace("<da>", b_party, true);
  destination.Replace("<du>", b_party(colon, at-1), true);
  destination.Replace("<!du>", b_party.Mid(at), true);
  destination.Replace("<dn>", digits, true);
  destination.Replace("<!dn>", b_party.Mid(nonDigitPos), true);
  destination.Replace("<db>",  b_party.Mid(colon), true);

  PINDEX pos;
  while ((pos = destination.FindRegEx("<dn[1-9]>")) != P_MAX_INDEX)
    destination.Splice(digits.Mid(destination[pos+3]-'0'), pos, 5);

  // Do meta character substitutions
  while ((pos = destination.Find("<dn2ip>")) != P_MAX_INDEX) {
    PStringStream route;
    PStringArray stars = digits.Tokenise('*');
    switch (stars.GetSize()) {
      case 0 :
      case 1 :
      case 2 :
      case 3 :
        route << digits;
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


void OpalManager::SetProductInfo(const OpalProductInfo & info, bool updateAll)
{
  productInfo = info;

  if (updateAll) {
    endpointsMutex.StartWrite();
    for (PList<OpalEndPoint>::iterator ep = endpointList.begin(); ep != endpointList.end(); ++ep)
      ep->SetProductInfo(info);
    endpointsMutex.EndWrite();
  }
}


void OpalManager::SetDefaultUserName(const PString & name, bool updateAll)
{
  defaultUserName = name;

  if (updateAll) {
    endpointsMutex.StartWrite();
    for (PList<OpalEndPoint>::iterator ep = endpointList.begin(); ep != endpointList.end(); ++ep)
      ep->SetDefaultLocalPartyName(name);
    endpointsMutex.EndWrite();
  }
}


void OpalManager::SetDefaultDisplayName(const PString & name, bool updateAll)
{
  defaultDisplayName = name;

  if (updateAll) {
    endpointsMutex.StartWrite();
    for (PList<OpalEndPoint>::iterator ep = endpointList.begin(); ep != endpointList.end(); ++ep)
      ep->SetDefaultDisplayName(name);
    endpointsMutex.EndWrite();
  }
}


PBoolean OpalManager::IsLocalAddress(const PIPSocket::Address & ip) const
{
  /* Check if the remote address is a private IP, broadcast, or us */
  return ip.IsAny() || ip.IsBroadcast() || ip.IsRFC1918() || PIPSocket::IsLocalHost(ip);
}


PBoolean OpalManager::IsRTPNATEnabled(OpalConnection & /*conn*/, 
                                      const PIPSocket::Address & localAddr,
                                      const PIPSocket::Address & peerAddr,
                                      const PIPSocket::Address & sigAddr,
                                      PBoolean PTRACE_PARAM(incoming))
{
  PTRACE(4, "OPAL\tChecking " << (incoming ? "incoming" : "outgoing") << " call for NAT: local=" << localAddr << ", peer=" << peerAddr << ", sig=" << sigAddr);

  /* The peer endpoint may be on a public address, the local network (LAN) or
     NATed. If the last, it either knows it is behind the NAT or is blissfully
     unaware of it.

     If the remote is public or on a LAN/VPN then no special treatment of RTP
     is needed. We do make the assumption that the remote will indicate correct
     addresses everywhere, in SETUP/OLC and INVITE/SDP.

     Now if the endpoint is NATed and knows it is behind a NAT, and is doing
     it correctly, it is indistinguishable from being on a public address.

     If the remote endpoint is unaware of it's NAT status then there will be a
     discrepency between the physical address of the connection and the
     signaling adddress indicated in the protocol, the H.323 SETUP
     sourceCallSignalAddress or SIP "To" or "Contact" fields.

     So this is the first test to make: if those addresses the same, we will
     assume the other guy is public or LAN/VPN and either no NAT is involved,
     or we leave them in charge of any NAT traversal as he has the ability to
     do it. In either case we don't do anything.
   */

  if (peerAddr == sigAddr)
    return false;

  /* So now we have a remote that is confused in some way, so needs help. Our
     next test is for cases of where we are on a multi-homed machine and we
     ended up with a call from interface to another. No NAT needed.
   */
  if (PIPSocket::IsLocalHost(peerAddr))
    return false;

  /* So, call is from a remote host somewhere and is still confused. We now
     need to check if we are actually ABLE to help. We test if the local end
     of the connection is public, i.e. no NAT at this end so we can help.
   */
  if (!localAddr.IsRFC1918())
    return true;

  /* Another test for if we can help, we are behind a NAT too, but the user has
     provided information so we can compensate for it, i.e. we "know" about the
     NAT. We determine this by translating the localAddr and seing if it gets
     changed to the NAT router address. If so, we can help.
   */
  PIPSocket::Address natAddr = localAddr;
  if (TranslateIPAddress(natAddr, peerAddr))
    return true;

  /* If we get here, we appear to be in a situation which, if we tried to do the
     NAT translation, we could end up in a staring match as the NAT traversal
     technique does not send anything till it receives something. If both side
     do that then .....

     So, we do nothing and hope for the best. This means that either the call
     gets no media, or there is some other magic entity (smart router, proxy,
     etc) between the two endpoints we know nothing about that will do NAT
     translations for us.

     Are there any other cases?
  */
  return false;
}


PBoolean OpalManager::TranslateIPAddress(PIPSocket::Address & localAddress,
                                     const PIPSocket::Address & remoteAddress)
{
  if (!translationAddress.IsValid())
    return PFalse; // Have nothing to translate it to

  if (!IsLocalAddress(localAddress))
    return PFalse; // Is already translated

  if (IsLocalAddress(remoteAddress))
    return PFalse; // Does not need to be translated

  // Tranlsate it!
  localAddress = translationAddress;
  return PTrue;
}


bool OpalManager::SetTranslationHost(const PString & host)
{
  if (PIPSocket::GetHostAddress(host, translationAddress)) {
    translationHost = host;
    return true;
  }

  translationHost = PString::Empty();
  translationAddress = PIPSocket::GetDefaultIpAny();
  return false;
}


void OpalManager::SetTranslationAddress(const PIPSocket::Address & address)
{
  translationAddress = address;
  translationHost = PIPSocket::GetHostName(address);
}


PSTUNClient * OpalManager::GetSTUN(const PIPSocket::Address & ip) const
{
  if (ip.IsValid() && IsLocalAddress(ip))
    return NULL;

  return stun;
}


PSTUNClient::NatTypes OpalManager::SetSTUNServer(const PString & server)
{
  stunServer = server;

  if (server.IsEmpty()) {
    if (stun) {
      PInterfaceMonitor::GetInstance().OnRemoveSTUNClient(stun);
    }
    delete stun;
    delete interfaceMonitor;
    stun = NULL;
    interfaceMonitor = NULL;
    return PSTUNClient::UnknownNat;
  }

  if (stun == NULL) {
    stun = new PSTUNClient(server,
                           GetUDPPortBase(), GetUDPPortMax(),
                           GetRtpIpPortBase(), GetRtpIpPortMax());
    interfaceMonitor = new InterfaceMonitor(stun);
  } else
    stun->SetServer(server);

  PSTUNClient::NatTypes type = stun->GetNatType();
  if (type != PSTUNClient::BlockedNat)
    stun->GetExternalAddress(translationAddress);

  PTRACE(3, "OPAL\tSTUN server \"" << server << "\" replies " << type << ", external IP " << translationAddress);

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


#if OPAL_VIDEO
template<class PVideoXxxDevice>
static PBoolean SetVideoDevice(const PVideoDevice::OpenArgs & args, PVideoDevice::OpenArgs & member)
{
  // Check that the input device is legal
  PVideoXxxDevice * pDevice = PVideoXxxDevice::CreateDeviceByName(args.deviceName, args.driverName, args.pluginMgr);
  if (pDevice != NULL) {
    delete pDevice;
    member = args;
    return PTrue;
  }

  if (args.deviceName[0] != '#')
    return PFalse;

  // Selected device by ordinal
  PStringArray devices = PVideoXxxDevice::GetDriversDeviceNames(args.driverName, args.pluginMgr);
  if (devices.IsEmpty())
    return PFalse;

  PINDEX id = args.deviceName.Mid(1).AsUnsigned();
  if (id <= 0 || id > devices.GetSize())
    return PFalse;

  member = args;
  member.deviceName = devices[id-1];
  return PTrue;
}


PBoolean OpalManager::SetVideoInputDevice(const PVideoDevice::OpenArgs & args)
{
  return SetVideoDevice<PVideoInputDevice>(args, videoInputDevice);
}


PBoolean OpalManager::SetVideoPreviewDevice(const PVideoDevice::OpenArgs & args)
{
  return SetVideoDevice<PVideoOutputDevice>(args, videoPreviewDevice);
}


PBoolean OpalManager::SetVideoOutputDevice(const PVideoDevice::OpenArgs & args)
{
  return SetVideoDevice<PVideoOutputDevice>(args, videoOutputDevice);
}

#endif

PBoolean OpalManager::SetNoMediaTimeout(const PTimeInterval & newInterval) 
{
  if (newInterval < 10)
    return PFalse;

  noMediaTimeout = newInterval; 
  return PTrue; 
}


void OpalManager::GarbageCollection()
{
  PBoolean allCleared = activeCalls.DeleteObjectsToBeRemoved();

  endpointsMutex.StartRead();

  for (PList<OpalEndPoint>::iterator ep = endpointList.begin(); ep != endpointList.end(); ++ep) {
    if (!ep->GarbageCollection())
      allCleared = PFalse;
  }

  endpointsMutex.EndRead();

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

void OpalManager::OnNewConnection(OpalConnection & /*conn*/)
{
}

PBoolean OpalManager::UseRTPAggregation() const
{ 
#if OPAL_RTP_AGGREGATE
  return useRTPAggregation; 
#else
  return PFalse;
#endif
}

PBoolean OpalManager::StartRecording(const PString & callToken, const PFilePath & fn)
{
  PSafePtr<OpalCall> call = activeCalls.FindWithLock(callToken, PSafeReadWrite);
  if (call == NULL)
    return PFalse;

  return call->StartRecording(fn);
}


bool OpalManager::IsRecording(const PString & callToken)
{
  PSafePtr<OpalCall> call = FindCallWithLock(callToken, PSafeReadWrite);
  return call != NULL && call->IsRecording();
}


void OpalManager::StopRecording(const PString & callToken)
{
  PSafePtr<OpalCall> call = activeCalls.FindWithLock(callToken, PSafeReadWrite);
  if (call != NULL)
    call->StopRecording();
}


/////////////////////////////////////////////////////////////////////////////

OpalManager::InterfaceMonitor::InterfaceMonitor(PSTUNClient * _stun)
: PInterfaceMonitorClient(OpalManagerInterfaceMonitorClientPriority),
  stun(_stun)
{
}

void OpalManager::InterfaceMonitor::OnAddInterface(const PIPSocket::InterfaceEntry & /*entry*/)
{
  stun->InvalidateExternalAddressCache();
}

void OpalManager::InterfaceMonitor::OnRemoveInterface(const PIPSocket::InterfaceEntry & /*entry*/)
{
  stun->InvalidateExternalAddressCache();
}


/////////////////////////////////////////////////////////////////////////////

OpalRecordManager::Mixer_T::Mixer_T()
  : OpalAudioMixer(PTrue)
{ 
  mono = PFalse; 
  started = PFalse; 
}

PBoolean OpalRecordManager::Mixer_T::Open(const PFilePath & fn)
{
  PWaitAndSignal m(mutex);

  if (!started) {
    file.SetFormat(OpalWAVFile::fmt_PCM);
    file.Open(fn, PFile::ReadWrite);
    if (!mono)
      file.SetChannels(2);
    started = PTrue;
  }
  return PTrue;
}

PBoolean OpalRecordManager::Mixer_T::Close()
{
  PWaitAndSignal m(mutex);
  file.Close();
  return PTrue;
}

PBoolean OpalRecordManager::Mixer_T::OnWriteAudio(const MixerFrame & mixerFrame)
{
  if (file.IsOpen()) {
    OpalAudioMixerStream::StreamFrame frame;
    if (mono) {
      mixerFrame.GetMixedFrame(frame);
      file.Write(frame.GetPointerAndLock(), frame.GetSize());
      frame.Unlock();
    } else {
      mixerFrame.GetStereoFrame(frame);
      file.Write(frame.GetPointerAndLock(), frame.GetSize());
      frame.Unlock();
    }
  }
  return PTrue;
}

OpalRecordManager::OpalRecordManager()
{
  started = PFalse;
}

PBoolean OpalRecordManager::Open(const PString & _callToken, const PFilePath & fn)
{
  PWaitAndSignal m(mutex);

  if (_callToken.IsEmpty())
    return PFalse;

  if (token.IsEmpty())
    token = _callToken;
  else if (_callToken != token)
    return PFalse;

  return mixer.Open(fn);
}

PBoolean OpalRecordManager::CloseStream(const PString & _callToken, const std::string & _strm)
{
  {
    PWaitAndSignal m(mutex);
    if (_callToken.IsEmpty() || token.IsEmpty() || (token != _callToken))
      return PFalse;

    mixer.RemoveStream(_strm);
  }
  return PTrue;
}

PBoolean OpalRecordManager::Close(const PString & _callToken)
{
  {
    PWaitAndSignal m(mutex);
    if (_callToken.IsEmpty() || token.IsEmpty() || (token != _callToken))
      return PFalse;

    mixer.RemoveAllStreams();
  }
  mixer.Close();
  return PTrue;
}

PBoolean OpalRecordManager::WriteAudio(const PString & _callToken, const std::string & strm, const RTP_DataFrame & rtp)
{ 
  PWaitAndSignal m(mutex);
  if (_callToken.IsEmpty() || token.IsEmpty() || (token != _callToken))
    return PFalse;

  return mixer.Write(strm, rtp);
}
