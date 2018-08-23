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
 */

#include <ptlib.h>

#ifdef __GNUC__
#pragma implementation "manager.h"
#endif

#include <opal_config.h>

#include <opal/manager.h>
#include <opal/endpoint.h>
#include <opal/call.h>
#include <opal/patch.h>
#include <opal/mediastrm.h>
#include <codec/g711codec.h>
#include <codec/vidcodec.h>
#include <codec/rfc4175.h>
#include <codec/rfc2435.h>
#include <codec/opalpluginmgr.h>
#include <im/im_ep.h>
#include <ep/opalmixer.h>

#if OPAL_HAS_H281
  #include <h224/h281.h>
#endif

#include <ptclib/random.h>
#include <ptclib/url.h>
#include <ptclib/mime.h>
#include <ptclib/pssl.h>

#include "../../version.h"
#include "../../revision.h"


static const char * const DefaultMediaFormatOrder[] = {
  OPAL_OPUS48,
  OPAL_G7222,
  OPAL_G7221_32K,
  OPAL_G7221_24K,
  OPAL_G722,
  OPAL_GSMAMR,
  OPAL_G7231_6k3,
  OPAL_G729B,
  OPAL_G729AB,
  OPAL_G729,
  OPAL_G729A,
  OPAL_iLBC,
  OPAL_GSM0610,
  OPAL_G728,
  OPAL_G726_40K,
  OPAL_G726_32K,
  OPAL_G726_24K,
  OPAL_G726_16K,
  OPAL_G711_ULAW_64K,
  OPAL_G711_ALAW_64K,
#if OPAL_VIDEO
  OPAL_H264_High,   // High profile
  OPAL_H264_MODE1,  // Packetisation mode 1
  OPAL_H264_MODE0,  // Packetisation mode 0
  OPAL_VP8,
  OPAL_MPEG4,
  OPAL_H263plus,
  OPAL_H263,
  OPAL_H261,
#endif
#if OPAL_HAS_SIPIM
  OPAL_SIPIM,
#endif
#if OPAL_HAS_RFC4103
  OPAL_T140,
#endif
#if OPAL_HAS_MSRP
  OPAL_MSRP
#endif
};

static const char * const DefaultMediaFormatMask[] = {
  OPAL_iLBC"-13k3",
  OPAL_iLBC"-15k2",
#if OPAL_RFC4175
  OPAL_RFC4175_YCbCr420,
  OPAL_RFC4175_RGB,
#endif
#if OPAL_HAS_MSRP
  OPAL_MSRP,
#endif
  OPAL_H264_High   // Do not include by default, interop issues.
};

// G.711 is *always* available
// Yes, it would make more sense for this to be in g711codec.cxx, but on 
// Linux it would not get loaded due to static initialisation optimisation
OPAL_REGISTER_G711();

// Same deal for RC4175 video
#if OPAL_RFC4175
OPAL_REGISTER_RFC4175();
#endif

// Same deal for RC2435 video
#if OPAL_RFC2435
OPAL_REGISTER_RFC2435_JPEG();
#endif


#define new PNEW

#define PTraceModule() "OpalMan"


/////////////////////////////////////////////////////////////////////////////

ostream & operator<<(ostream & strm, OpalConferenceState::ChangeType type)
{
  static const char * const Names[] = { "Created", "Destroyed", "UserAdded", "UserRemoved" };
  if (type >= 0 && type < PARRAYSIZE(Names))
    strm << Names[type];
  else
    strm << '<' << (unsigned)type << '>';
  return strm;
}


/////////////////////////////////////////////////////////////////////////////

PString OpalGetVersion()
{
  static const PProcess::VersionInfo ver = { MAJOR_VERSION, MINOR_VERSION, PProcess::BUILD_TYPE, BUILD_NUMBER, 0, GIT_COMMIT };
  return ver.AsString();
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
  : t35CountryCode(0)
  , t35Extension(0)
  , manufacturerCode(0)
{
}


OpalProductInfo::OpalProductInfo(const char * vend,
                                 const char * nam,
                                 const char * ver,
                                 BYTE country,
                                 WORD manufacturer,
                                 const char * o)
  : vendor(vend)
  , name(nam)
  , version(ver)
  , t35CountryCode(country)     
  , t35Extension(0)
  , manufacturerCode(manufacturer)
  , oid(o)
{
  // Sanitise the product name to be compatible with SIP User-Agent rules
  name.Replace(' ', '-', true);
  PINDEX pos;
  while ((pos = name.FindSpan("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-.!%*_+`'~")) != P_MAX_INDEX)
    name.Delete(pos, 1);
}


const OpalProductInfo & OpalProductInfo::Default()
{
  static OpalProductInfo instance(PProcess::Current().GetManufacturer(),
                                  PProcess::Current().GetName(),
                                  PProcess::Current().GetVersion(),
                                  9,    // Country code for Australia
                                  61,   // Allocated by Australian Communications Authority, Oct 2000
                                  NULL);
  return instance;
}


PCaselessString OpalProductInfo::AsString() const
{
  PStringStream str;
  str << *this;
  return str;
}


void OpalProductInfo::operator=(const OpalProductInfo & other)
{
  name = other.name;
  version = other.version;
  comments = other.comments;
  t35CountryCode = other.t35CountryCode;
  t35Extension = other.t35Extension;
  manufacturerCode = other.manufacturerCode;

  // Look for special case of vendor being numeric "<manufacturer>,<country>[,<ext>]"
  if (other.vendor.FindSpan("0123456789,") == P_MAX_INDEX) {
    PStringArray fields = other.vendor.Tokenise(',');
    switch (fields.GetSize()) {
      default :
        break;
      case 3:
        t35Extension = (BYTE)fields[2].AsUnsigned();
      case 2 :
        manufacturerCode = (WORD)fields[1].AsUnsigned();
        t35CountryCode = (BYTE)fields[0].AsUnsigned();
        vendor.MakeEmpty();
        return;
    }
  }

  vendor = other.vendor;
}


bool OpalProductInfo::operator==(const OpalProductInfo & other) const
{
  if (name != other.name)
    return false;

  if (!other.version.IsEmpty() && version != other.version)
    return false;

  if (vendor != other.vendor)
    return false;

  if (!vendor.IsEmpty())
    return true;

  return t35CountryCode == other.t35CountryCode ||
         t35CountryCode != other.t35CountryCode ||
         manufacturerCode == other.manufacturerCode;
}


ostream & operator<<(ostream & strm, const OpalProductInfo & info)
{
  if (info.name.IsEmpty() &&
      info.version.IsEmpty() &&
      info.vendor.IsEmpty() &&
      info.t35CountryCode == 0 &&
      info.manufacturerCode == 0)
    return strm;

  strm << info.name << '\t' << info.version << '\t';

  if (info.t35CountryCode != 0 && info.manufacturerCode != 0) {
    strm << (unsigned)info.t35CountryCode;
    if (info.t35Extension != 0)
      strm << '.' << (unsigned)info.t35Extension;
    strm << '/' << info.manufacturerCode;
  }

  strm << '\t' << info.vendor;

  return strm;
}


/////////////////////////////////////////////////////////////////////////////

OpalManager::OpalManager()
  : m_productInfo(OpalProductInfo::Default())
  , m_defaultUserName(PProcess::Current().GetUserName())
  , m_defaultDisplayName(m_defaultUserName)
  , m_rtpPayloadSizeMax(1400) // RFC879 recommends 576 bytes, but that is ancient history, 99.999% of the time 1400+ bytes is used.
  , m_rtpPacketSizeMax(10*1024)
  , m_mediaFormatOrder(PARRAYSIZE(DefaultMediaFormatOrder), DefaultMediaFormatOrder)
  , m_mediaFormatMask(PARRAYSIZE(DefaultMediaFormatMask), DefaultMediaFormatMask)
  , m_disableDetectInBandDTMF(false)
  , m_noMediaTimeout(0, 0, 5)     // Minutes
  , m_txMediaTimeout(0, 10)       // Seconds
  , m_signalingTimeout(0, 10)     // Seconds
  , m_transportIdleTime(0, 0, 1)  // Minute
  , m_natKeepAliveTime(0, 30)     // Seconds
#if OPAL_ICE
  , m_iceTimeout(0, 15)           // Seconds, as per RFC 5245
#endif
  , m_staleReceiverTimeout(0,0,1) // Minutes
#if OPAL_SRTP
  , m_dtlsTimeout(0, 3)           // Seconds
#endif
  , m_rtpIpPorts(5000, 5999)
#if OPAL_PTLIB_SSL
  , m_caFiles(PProcess::Current().GetHomeDirectory() + "certificates")
  , m_certificateFile(PProcess::Current().GetHomeDirectory() + "opal_certificate.pem")
  , m_privateKeyFile(PProcess::Current().GetHomeDirectory() + "opal_private_key.pem")
  , m_autoCreateCertificate(true)
#endif
#if OPAL_PTLIB_NAT
  , m_natMethods(new PNatMethods(true))
  , m_onInterfaceChange(PCREATE_InterfaceNotifier(OnInterfaceChange))
#endif
  , lastCallTokenID(0)
  , P_DISABLE_MSVC_WARNINGS(4355, m_activeCalls(*this))
  , m_clearingAllCallsCount(0)
  , m_garbageCollector(NULL)
  , m_decoupledEventPool(5, 0, "OPAL-Event")
#if OPAL_SCRIPT
  , m_script(NULL)
#endif
{
  m_mediaQoS[OpalMediaType::Audio()].m_type = PIPSocket::VoiceQoS;

#if OPAL_VIDEO
  m_mediaQoS[OpalMediaType::Video()].m_type = PIPSocket::VideoQoS;

  for (OpalVideoFormat::ContentRole role = OpalVideoFormat::BeginContentRole; role < OpalVideoFormat::EndContentRole; ++role)
    m_videoInputDevice[role].deviceName = m_videoPreviewDevice[role].deviceName = m_videoOutputDevice[role].deviceName = P_NULL_VIDEO_DEVICE;

  PStringArray devices = PVideoInputDevice::GetDriversDeviceNames("*"); // Get all devices on all drivers
  PINDEX i;
  for (i = 0; i < devices.GetSize(); ++i) {
    PCaselessString dev = devices[i];
    if (dev[0] != '*' && dev.NumCompare(P_FAKE_VIDEO_PREFIX) != EqualTo) {
      PTRACE(3, "Default video grabber set to \"" << dev << '"');
      m_videoInputDevice[OpalVideoFormat::eNoRole].deviceName = m_videoInputDevice[OpalVideoFormat::eMainRole].deviceName = dev;
      SetAutoStartTransmitVideo(true);
#if PTRACING
      static unsigned const Level = 3;
      if (PTrace::CanTrace(Level)) {
        ostream & trace = PTRACE_BEGIN(Level);
        trace << "Default video input device set to \"" << dev << '"';
        PVideoInputDevice::Capabilities caps;
        if (PVideoInputDevice::GetDeviceCapabilities(dev, &caps))
          trace << '\n' << caps;
        else
          trace << " - no capabilities.";
        trace << PTrace::End;
      }
#endif
      break;
    }
  }

  devices = PVideoOutputDevice::GetDriversDeviceNames("*"); // Get all devices on all drivers
  for (i = 0; i < devices.GetSize(); ++i) {
    PCaselessString dev = devices[i];
    if (dev[0] != '*' && dev != P_NULL_VIDEO_DEVICE) {
      PTRACE(3, "Default video display set to \"" << dev << '"');
      for (OpalVideoFormat::ContentRole role = OpalVideoFormat::BeginContentRole; role < OpalVideoFormat::EndContentRole; ++role)
        m_videoOutputDevice[role].deviceName = dev;
      m_videoPreviewDevice[OpalVideoFormat::eNoRole] = m_videoOutputDevice[OpalVideoFormat::eNoRole];
      SetAutoStartReceiveVideo(true);
      PTRACE(3, "Default video output/preview device set to \"" << devices[i] << '"');
      break;
    }
  }
#endif

#if OPAL_PTLIB_NAT
  PInterfaceMonitor::GetInstance().AddNotifier(m_onInterfaceChange);
#endif

  PTRACE(4, "Created manager, OPAL version " << OpalGetVersion());
}


OpalManager::~OpalManager()
{
  ShutDownEndpoints();

#if OPAL_SCRIPT
  delete m_script;
#endif

  // Shut down the cleaner thread
  m_garbageCollectExit.Signal();
  PThread::WaitAndDelete(m_garbageCollector);

  // Clean up any calls that the cleaner thread missed on the way out
  GarbageCollection();

#if OPAL_PTLIB_NAT
  PInterfaceMonitor::GetInstance().RemoveNotifier(m_onInterfaceChange);
  delete m_natMethods;
#endif

  OpalMediaFormat::RemoveRegisteredMediaFormats("*");

  PTRACE(4, "Deleted manager.");
}


PList<OpalEndPoint> OpalManager::GetEndPoints() const
{
  PList<OpalEndPoint> list;
  list.AllowDeleteObjects(false);

  PReadWaitAndSignal mutex(m_endpointsMutex);

  for (PList<OpalEndPoint>::const_iterator it = m_endpointList.begin(); it != m_endpointList.end(); ++it)
    list.Append((OpalEndPoint *)&*it);

  return list;
}


PStringList OpalManager::GetPrefixNames(const OpalEndPoint * endpoint) const
{
  PStringList list;

  PReadWaitAndSignal mutex(m_endpointsMutex);

  for (std::map<PString, OpalEndPoint *>::const_iterator it = m_endpointMap.begin(); it != m_endpointMap.end(); ++it) {
    if (endpoint == NULL || it->second == endpoint)
      list += it->first;
  }

  return list;
}


void OpalManager::ShutDownEndpoints()
{
  PTRACE(3, "Shutting down manager.");

  // Clear any pending calls, set flag so no calls can be received before endpoints removed
  InternalClearAllCalls(OpalConnection::EndedByLocalUser, true, m_clearingAllCallsCount++ == 0);

#if OPAL_HAS_PRESENCE
  // Remove (and unsubscribe) all the presentities
  PTRACE(4, "Shutting down all presentities");
  for (PSafePtr<OpalPresentity> presentity(m_presentities, PSafeReference); presentity != NULL; ++presentity)
    presentity->Close();
  m_presentities.RemoveAll();
#endif // OPAL_HAS_PRESENCE

  PTRACE(4, "Shutting down endpoints.");
  // Deregister the endpoints
  m_endpointsMutex.StartRead();
  for (PList<OpalEndPoint>::iterator ep = m_endpointList.begin(); ep != m_endpointList.end(); ++ep)
    ep->ShutDown();
  m_endpointsMutex.EndRead();

  m_endpointsMutex.StartWrite();
  m_endpointMap.clear();
  m_endpointList.RemoveAll();
  m_endpointsMutex.EndWrite();

  --m_clearingAllCallsCount; // Allow for endpoints to be added again.

#if OPAL_SCRIPT
  if (m_script != NULL)
    m_script->Call("OnShutdown");
#endif

  PTRACE(2, "Shut down completed.");
}


void OpalManager::AttachEndPoint(OpalEndPoint * endpoint, const PString & prefix)
{
  if (PAssertNULL(endpoint) == NULL)
    return;

  PString thePrefix = prefix.IsEmpty() ? endpoint->GetPrefixName() : prefix;

  PWriteWaitAndSignal mutex(m_endpointsMutex);

  std::map<PString, OpalEndPoint *>::iterator it = m_endpointMap.find(thePrefix);
  if (it != m_endpointMap.end()) {
    PTRACE_IF(1, it->second != endpoint, "An endpoint is already attached to prefix " << thePrefix);
    return;
  }

  if (m_endpointList.GetObjectsIndex(endpoint) == P_MAX_INDEX)
    m_endpointList.Append(endpoint);
  m_endpointMap[thePrefix] = endpoint;

  /* Avoid strange race condition caused when garbage collection occurs
     on the endpoint instance which has not completed construction. This
     is an ulgly hack and relies on the ctors taking less than one second. */
  m_garbageCollectChangeTime.SetCurrentTime();

  // Have something which requires garbage collections, so start it up
  if (m_garbageCollector == NULL)
    m_garbageCollector = PThread::Create(PCREATE_NOTIFIER(GarbageMain), "Opal Garbage");

  PTRACE(3, "Attached endpoint with prefix " << thePrefix);
}


void OpalManager::DetachEndPoint(OpalEndPoint * endpoint)
{ 
  if (PAssertNULL(endpoint) == NULL)
    return;

  endpoint->ShutDown();

  m_endpointsMutex.StartWrite();

  if (m_endpointList.Remove(endpoint)) {
    // Was in list, remove from map too
    std::map<PString, OpalEndPoint *>::iterator it = m_endpointMap.begin();
    while (it != m_endpointMap.end()) {
      if (it->second != endpoint)
        ++it;
      else {
        m_endpointMap.erase(it);
        it = m_endpointMap.begin();
      }
    }
  }

  m_endpointsMutex.EndWrite();
}


void OpalManager::DetachEndPoint(const PString & prefix)
{
  PReadWaitAndSignal mutex(m_endpointsMutex);

  std::map<PString, OpalEndPoint *>::iterator it = m_endpointMap.find(prefix);
  if (it == m_endpointMap.end())
    return;

  OpalEndPoint * endpoint = it->second;

  m_endpointsMutex.StartWrite();
  m_endpointMap.erase(it);
  m_endpointsMutex.EndWrite();

  // See if other references
  for (it = m_endpointMap.begin(); it != m_endpointMap.end(); ++it) {
    if (it->second == endpoint)
      return; // Still a reference to it
  }

  // Last copy, delete it now
  DetachEndPoint(endpoint);
}


OpalEndPoint * OpalManager::FindEndPoint(const PString & prefix) const
{
  PReadWaitAndSignal mutex(m_endpointsMutex);
  std::map<PString, OpalEndPoint *>::const_iterator it = m_endpointMap.find(prefix);
  return it != m_endpointMap.end() ? it->second : NULL;
}


PBoolean OpalManager::SetUpCall(const PString & partyA,
                                const PString & partyB,
                                      PString & token,
                                         void * userData,
                                   unsigned int options,
                OpalConnection::StringOptions * stringOptions)
{
  token.MakeEmpty();

  PSafePtr<OpalCall> call = SetUpCall(partyA, partyB, userData, options, stringOptions);
  if (call == NULL)
    return false;

  token = call->GetToken();
  return true;
}


#if OPAL_HAS_MIXER

bool OpalManager::SetUpConference(OpalCall & call, const char * mixerURI, const char * localURI)
{
  PString confURI(mixerURI);
  if (confURI.IsEmpty())
    confURI = OPAL_MIXER_PREFIX":local-conference";

  PString mixerPrefix, mixerNode;
  if (!confURI.Split(':', mixerPrefix, mixerNode)) {
    mixerPrefix = OPAL_MIXER_PREFIX;
    mixerNode = confURI;
    confURI.Splice(OPAL_MIXER_PREFIX":", 0);
  }

  OpalMixerEndPoint * mixerEP = FindEndPointAs<OpalMixerEndPoint>(mixerPrefix);
  if (mixerEP == NULL) {
    PTRACE(2, "No mixer endpoint using prefix \"" << mixerPrefix << '"');
    return false;
  }

  PSafePtr<OpalLocalConnection> connection = call.GetConnectionAs<OpalLocalConnection>();
  if (connection == NULL) {
    PTRACE(2, "Cannot conference gateway call " << call);
    return false;
  }

  PSafePtr<OpalMixerNode> node = mixerEP->FindNode(mixerNode);
  if (node == NULL) {
    node = mixerEP->AddNode(new OpalMixerNodeInfo(mixerNode));
    PTRACE(3, "Created mixer node \"" << mixerNode << '"');
  }

  if (!call.Transfer(confURI, connection)) {
    PTRACE(2, "Could not add call " << call << " to conference \"" << confURI << '"');
    return false;
  }

  call.Retrieve(); // Make sure is not still on hold

  PTRACE(3, &call, "Added call " << call << " to conference \"" << confURI << '"');

  PString uri = localURI != NULL ? localURI : "pc:*";
  if (uri.IsEmpty() || node->GetConnectionCount() > 1)
    return true;

#if OPAL_VIDEO
  /* If after adding the above to conference, it's the only one there, and we
     have a local URI specified (typically pc:*) then we add that too. */
  if (connection->GetMediaStream(OpalMediaType::Video(), true) == NULL)
    uri += ";" OPAL_URL_PARAM_PREFIX OPAL_OPT_AUTO_START "=video:no";
#endif

  PSafePtr<OpalCall> localCall = SetUpCall(uri + ";" OPAL_URL_PARAM_PREFIX OPAL_OPT_CONF_OWNER "=yes", confURI);
  if (localCall != NULL) {
    PTRACE(3, localCall, "Added local call \"" << uri << "\" to conference \"" << confURI << '"');
    return true;
  }

  PTRACE(2, "Could not start local call into conference");
  return false;
}

#endif // OPAL_HAS_MIXER


static void SynchCallSetUp(PSafePtr<OpalConnection> connection)
{
  if (connection->SetUpConnection())
    return;

  if (connection->GetCallEndReason() == OpalConnection::NumCallEndReasons)
    connection->Release(OpalConnection::EndedByTemporaryFailure);
  PTRACE(2, "Could not set up connection on " << *connection << ' ' << connection->GetCallEndReason());
}


static void AsynchCallSetUp(PSafePtr<OpalConnection> connection)
{
  PTRACE_CONTEXT_ID_PUSH_THREAD(connection);

  if (connection.SetSafetyMode(PSafeReadWrite))
    SynchCallSetUp(connection);
}


PSafePtr<OpalCall> OpalManager::SetUpCall(const PString & partyA,
                                          const PString & partyB,
                                                   void * userData,
                                             unsigned int options,
                          OpalConnection::StringOptions * stringOptions)
{
  PTRACE(3, "Set up call from " << partyA << " to " << partyB);

  OpalCall * call = InternalCreateCall(userData);
  if (call == NULL)
    return NULL;

  PTRACE_CONTEXT_ID_PUSH_THREAD(call);

  call->SetPartyB(partyB.Trim());

  // If we are the A-party then need to initiate a call now in this thread and
  // go through the routing engine via OnIncomingConnection. If we were the
  // B-Party then SetUpConnection() gets called in the context of the A-party
  // thread.
  PSafePtr<OpalConnection> connection = MakeConnection(*call, partyA.Trim(), userData, options, stringOptions);
  if (connection != NULL) {
    if ((options & OpalConnection::SynchronousSetUp) != 0)
      SynchCallSetUp(connection);
    else {
      PTRACE(4, "SetUpCall started, call=" << *call);
      new PThread1Arg< PSafePtr<OpalConnection> >(connection, AsynchCallSetUp, true, "SetUpCall");
    }
    return call;
  }

  PTRACE(2, "Could not create connection for \"" << partyA << '"');

  OpalConnection::CallEndReason endReason = call->GetCallEndReason();
  if (endReason == OpalConnection::NumCallEndReasons)
    endReason = OpalConnection::EndedByTemporaryFailure;
  call->Clear(endReason);

  return NULL;
}


bool OpalManager::OnLocalIncomingCall(OpalLocalConnection &)
{
  return true;
}


bool OpalManager::OnLocalOutgoingCall(const OpalLocalConnection &)
{
  return true;
}


#if OPAL_SCRIPT
void OpalManager::OnEstablishedCall(OpalCall & call)
{
  if (m_script != NULL)
    m_script->Call("OnEstablished", "s", (const char *)call.GetToken());
}
#else
void OpalManager::OnEstablishedCall(OpalCall & /*call*/)
{
}
#endif


PBoolean OpalManager::IsCallEstablished(const PString & token)
{
  PSafePtr<OpalCall> call = m_activeCalls.Find(token, PSafeReadOnly);
  if (call == NULL)
    return false;

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

  // Find the call by token, callid or conferenceid
  PSafePtr<OpalCall> call = m_activeCalls.Find(token, PSafeReference);
  if (call == NULL) {
    PTRACE(2, "Could not find/lock call token \"" << token << '"');
    return false;
  }

  call->Clear(reason, sync);
  return true;
}


PBoolean OpalManager::ClearCallSynchronous(const PString & token,
                                       OpalConnection::CallEndReason reason)
{
  PSyncPoint wait;
  if (!ClearCall(token, reason, &wait))
    return false;

  wait.Wait();
  return false;
}


void OpalManager::ClearAllCalls(OpalConnection::CallEndReason reason, PBoolean wait)
{
  InternalClearAllCalls(reason, wait, m_clearingAllCallsCount++ == 0);
  --m_clearingAllCallsCount;
}


void OpalManager::InternalClearAllCalls(OpalConnection::CallEndReason reason, bool wait, bool firstThread)
{
  if (m_garbageCollector == NULL)
    return;

  PTRACE(3, "Clearing all calls " << (wait ? "and waiting" : "asynchronously")
                      << ", " << (firstThread ? "primary" : "secondary") << " thread.");

  if (firstThread) {
    // Clear all the currentyl active calls
    for (PSafePtr<OpalCall> call = m_activeCalls; call != NULL; ++call)
      call->Clear(reason);
  }

  if (wait) {
    /* This is done this way as PSyncPoint only works for one thread at a time,
       all subsequent threads will wait on the mutex for the first one to be
       released from the PSyncPoint wait. */
    m_clearingAllCallsMutex.Wait();
    if (firstThread)
      PAssert(m_allCallsCleared.Wait(PTimeInterval(0,m_activeCalls.GetSize()*2,1)), "All calls not cleared in a timely manner");
    m_clearingAllCallsMutex.Signal();
  }

  PTRACE(3, "All calls cleared.");
}


void OpalManager::OnClearedCall(OpalCall & PTRACE_PARAM(call))
{
  PTRACE(3, "OnClearedCall " << call << " from \"" << call.GetPartyA() << "\" to \"" << call.GetPartyB() << '"');
}


OpalCall * OpalManager::InternalCreateCall(void * userData)
{
  if (m_clearingAllCallsCount != 0) {
    PTRACE(2, "Create call not performed as currently clearing all calls.");
    return NULL;
  }

  return CreateCall(userData);
}


OpalCall * OpalManager::CreateCall(void * /*userData*/)
{
  return new OpalCall(*this);
}


void OpalManager::DestroyCall(OpalCall * call)
{
  delete call;
}


PString OpalManager::GetNextToken(char prefix)
{
  return psprintf("%c%08x%u", prefix, PRandom::Number(), ++lastCallTokenID);
}

PSafePtr<OpalConnection> OpalManager::MakeConnection(OpalCall & call,
                                                const PString & remoteParty,
                                                         void * userData,
                                                   unsigned int options,
                                OpalConnection::StringOptions * stringOptions)
{
  PTRACE(3, "Set up connection to \"" << remoteParty << '"');

  PReadWaitAndSignal mutex(m_endpointsMutex);

  PCaselessString epname = PURL::ExtractScheme(remoteParty);
  OpalEndPoint * ep = FindEndPoint(epname);
  if (ep != NULL)
    return ep->MakeConnection(call, remoteParty, userData, options, stringOptions);

  PTRACE(1, "Could not find endpoint for protocol \"" << epname << '"');
  return NULL;
}


PBoolean OpalManager::OnIncomingConnection(OpalConnection & connection, unsigned options, OpalConnection::StringOptions * stringOptions)
{
  PTRACE(3, "OnIncoming connection " << connection);

  connection.OnApplyStringOptions();

  // See if we already have a B-Party in the call. If not, make one.
  if (connection.GetOtherPartyConnection() != NULL)
    return true;

  OpalCall & call = connection.GetCall();

  PTRACE_CONTEXT_ID_PUSH_THREAD(call);

  // See if have pre-allocated B party address, otherwise
  // get destination from incoming connection
  PString destination = call.GetPartyB();
  if (destination.IsEmpty()) {
    destination = connection.GetDestinationAddress();
    if (destination.IsEmpty()) {
      PTRACE(2, "Cannot complete call, no destination address from connection " << connection);
      return false;
    }
  }

  OpalConnection::StringOptions mergedOptions = connection.GetStringOptions();
  if (stringOptions != NULL) {
    for (PStringToString::iterator it = stringOptions->begin(); it != stringOptions->end(); ++it)
      mergedOptions.SetAt(it->first, it->second);
  }

#if OPAL_HAS_IM
  /* If A-Party is not "im" and it only has im media formats, then direct
     connect to the "im" endpoint without going throught routing engine. */
  if (connection.IsNetworkConnection()) {
    OpalIMEndPoint * imEP = FindEndPointAs<OpalIMEndPoint>(OpalIMEndPoint::Prefix());
    if (imEP != NULL) {
      OpalMediaFormatList imFormats = imEP->GetMediaFormats();
      connection.AdjustMediaFormats(true, NULL, imFormats);
      if (!imFormats.IsEmpty()) {
        OpalMediaFormatList formats = connection.GetMediaFormats();
        if (!formats.IsEmpty()) {
          PStringStream autoStart;
          OpalMediaFormatList::iterator it;
          for (it = formats.begin(); it != formats.end(); ++it) {
            static const char prefix[] = OPAL_IM_MEDIA_TYPE_PREFIX;
            if (it->GetMediaType().compare(0, sizeof(prefix)-1, prefix) != 0)
              break;
            autoStart << it->GetMediaType() << ":sendrecv\n";
          }
          if (it == formats.end()) {
            mergedOptions.SetAt(OPAL_OPT_AUTO_START, autoStart);
            return imEP->MakeConnection(call, OpalIMEndPoint::Prefix() + ":*", NULL, options, &mergedOptions);
          }
        }
      }
    }
  }
#endif // OPAL_HAS_IM

#if OPAL_SCRIPT
  if (m_script != NULL) {
    PScriptLanguage::Signature sig;
    sig.m_arguments.resize(5);
    sig.m_arguments[0].SetDynamicString(call.GetToken());
    sig.m_arguments[1].SetDynamicString(connection.GetToken());
    sig.m_arguments[2].SetDynamicString(connection.GetRemotePartyURL());
    sig.m_arguments[3].SetDynamicString(connection.GetLocalPartyURL());
    sig.m_arguments[4].SetDynamicString(destination);
    if (m_script->Call("OnIncoming", sig) && !sig.m_results.empty())
      destination = sig.m_results[0].AsString();
  }
#endif

  // Use a routing algorithm to figure out who the B-Party is, and make second connection
  PStringSet routesTried;
  return OnRouteConnection(routesTried, connection.GetRemotePartyURL(), destination, call, options, &mergedOptions);
}


bool OpalManager::OnRouteConnection(PStringSet & routesTried,
                                    const PString & a_party,
                                    const PString & b_party,
                                    OpalCall & call,
                                    unsigned options,
                                    OpalConnection::StringOptions * stringOptions)
{
  PINDEX tableEntry = 0;
  for (;;) {
    PString route = ApplyRouteTable(a_party, b_party, tableEntry);
    if (route.IsEmpty()) {
      if (a_party == b_party) {
        PTRACE(2, "Circular route a=b=\"" << a_party << "\", call=" << call);
        return false;
      }

      // Check for if B-Party is an explicit address
      PCaselessString scheme = PURL::ExtractScheme(b_party);
      if (FindEndPoint(scheme) != NULL)
        return MakeConnection(call, b_party, NULL, options, stringOptions) != NULL;

      if (scheme.IsEmpty()) {
        PReadWaitAndSignal mutex(m_endpointsMutex);
        for (PList<OpalEndPoint>::iterator it = m_endpointList.begin(); it != m_endpointList.end(); ++it) {
          if (it->HasAttribute(OpalEndPoint::IsNetworkEndPoint) == (call.GetConnectionCount() > 0))
            return MakeConnection(call, it->GetPrefixName() + ':' + b_party, NULL, options, stringOptions) != NULL;
        }
      }

      PTRACE(2, "Could not route a=\"" << a_party << "\", b=\"" << b_party << "\", call=" << call);
      return false;
    }

    // See if already tried, keep searching if this route has already failed
    if (routesTried[route])
      continue;
    routesTried += route;

    // See if this route can be connected
    if (MakeConnection(call, route, NULL, options, stringOptions) != NULL)
      return true;

    if (call.GetConnection(0)->IsReleased())
      return false;

    // Recursively call with translated route
    if (OnRouteConnection(routesTried, a_party, route, call, options, stringOptions))
      return true;
  }
}


void OpalManager::OnProceeding(OpalConnection & connection)
{
  PTRACE(3, "OnProceeding " << connection);

  connection.GetCall().OnProceeding(connection);

#if OPAL_SCRIPT
  if (m_script != NULL)
    m_script->Call("OnProceeding", "ss",
                   (const char *)connection.GetCall().GetToken(),
                   (const char *)connection.GetToken());
#endif
}


void OpalManager::OnAlerting(OpalConnection & connection, bool withMedia)
{
  PTRACE(3, "OnAlerting " << connection << (withMedia ? " with media" : " normal"));

  connection.GetCall().OnAlerting(connection, withMedia);

#if OPAL_SCRIPT
  if (m_script != NULL)
    m_script->Call("OnAlerting", "ssb",
                   (const char *)connection.GetCall().GetToken(),
                   (const char *)connection.GetToken(),
                   withMedia);
#endif
}


void OpalManager::OnAlerting(OpalConnection & connection)
{
  connection.GetCall().OnAlerting(connection);
}


OpalConnection::AnswerCallResponse OpalManager::OnAnswerCall(OpalConnection & connection,
                                                             const PString & caller)
{
  return connection.GetCall().OnAnswerCall(connection, caller);
}


void OpalManager::OnConnected(OpalConnection & connection)
{
  PTRACE(3, "OnConnected " << connection);

#if OPAL_SCRIPT
  if (m_script != NULL)
    m_script->Call("OnConnected", "ss",
                   (const char *)connection.GetCall().GetToken(),
                   (const char *)connection.GetToken());
#endif

  connection.GetCall().OnConnected(connection);
}


void OpalManager::OnEstablished(OpalConnection & connection)
{
  PTRACE(3, "OnEstablished " << connection);

  connection.GetCall().OnEstablished(connection);
}


void OpalManager::OnReleased(OpalConnection & connection)
{
  PTRACE(3, "OnReleased " << connection);

  connection.GetCall().OnReleased(connection);
}


void OpalManager::OnHold(OpalConnection & connection, bool PTRACE_PARAM(fromRemote), bool PTRACE_PARAM(onHold))
{
  PTRACE(3, (onHold ? "On" : "Off") << " Hold "
         << (fromRemote ? "from remote" : "request succeeded")
         << " on " << connection);
  connection.GetEndPoint().OnHold(connection);
}


void OpalManager::OnHold(OpalConnection & /*connection*/)
{
}


PBoolean OpalManager::OnForwarded(OpalConnection & PTRACE_PARAM(connection),
			      const PString & /*forwardParty*/)
{
  PTRACE(4, "OnForwarded " << connection);
  return true;
}


bool OpalManager::OnTransferNotify(OpalConnection & PTRACE_PARAM(connection), const PStringToString & info)
{
  PTRACE(4, "OnTransferNotify for " << connection << '\n' << info);
  return info["result"] != "success";
}


OpalMediaFormatList OpalManager::GetCommonMediaFormats(bool transportable, bool pcmAudio) const
{
  OpalMediaFormatList formats;

  if (transportable) {
    OpalMediaFormatList allFormats = OpalMediaFormat::GetAllRegisteredMediaFormats();
    for (OpalMediaFormatList::iterator iter = allFormats.begin(); iter != allFormats.end(); ++iter) {
      if (iter->IsTransportable())
        formats += *iter;
    }
  }

  if (pcmAudio) {
    // Sound cards can only do 16 bit PCM, but at various sample rates
    // The following will be in order of preference, so lets do wideband first
    formats += OpalPCM16S_48KHZ;
    formats += OpalPCM16S_32KHZ;
    formats += OpalPCM16S_16KHZ;
    formats += OpalPCM16_48KHZ;
    formats += OpalPCM16_32KHZ;
    formats += OpalPCM16_16KHZ;
    formats += OpalPCM16;
    formats += OpalRFC2833;
#if OPAL_T38_CAPABILITY
    formats += OpalCiscoNSE;
#endif
  }

#if OPAL_VIDEO
  if (!GetVideoInputDevice().deviceName.IsEmpty())
    formats += OpalYUV420P;
#endif

#if OPAL_HAS_MSRP
  formats += OpalMSRP;
#endif

#if OPAL_HAS_SIPIM
  formats += OpalSIPIM;
#endif

#if OPAL_HAS_RFC4103
  formats += OpalT140;
#endif

#if OPAL_HAS_H281
  formats += OpalFECC_RTP;
  formats += OpalFECC_HDLC;
#endif

  return formats;
}


void OpalManager::AdjustMediaFormats(bool local,
                                     const OpalConnection & connection,
                                     OpalMediaFormatList & mediaFormats) const
{
  mediaFormats.Remove(m_mediaFormatMask);

  // Don't do the reorder done if acting as gateway, use other network endpoints ordering
  if (local) {
    bool reorder = true;
    if (connection.IsNetworkConnection()) {
      PSafePtr<OpalConnection> otherConnection = connection.GetOtherPartyConnection();
      if (otherConnection != NULL && otherConnection->IsNetworkConnection())
        reorder = false;
    }
    if (reorder)
      mediaFormats.Reorder(m_mediaFormatOrder);
  }

  connection.GetCall().AdjustMediaFormats(local, connection, mediaFormats);
}


OpalManager::MediaTransferMode OpalManager::GetMediaTransferMode(const OpalConnection & /*provider*/,
                                                                 const OpalConnection & /*consumer*/,
                                                                  const OpalMediaType & /*mediaType*/) const
{
  return MediaTransferForward;
}


bool OpalManager::GetMediaTransportAddresses(const OpalConnection & provider,
                                             const OpalConnection & consumer,
                                                         unsigned   PTRACE_PARAM(sessionId),
                                              const OpalMediaType & mediaType,
                                        OpalTransportAddressArray &) const
{
  if (!provider.IsNetworkConnection() || !consumer.IsNetworkConnection())
    return false;

  MediaTransferMode mode = GetMediaTransferMode(provider, consumer, mediaType);
  PTRACE(3, "Media transfer mode set to " << mode << " for " << mediaType
         << " session " << sessionId << ", from " << provider << " to " << consumer);
  return mode == OpalManager::MediaTransferBypass;
}


PBoolean OpalManager::OnOpenMediaStream(OpalConnection & PTRACE_PARAM(connection),
                                        OpalMediaStream & PTRACE_PARAM(stream))
{
  PTRACE(3, "OnOpenMediaStream " << connection << ',' << stream);
  return true;
}


bool OpalManager::OnLocalRTP(OpalConnection & PTRACE_PARAM(connection1),
                             OpalConnection & PTRACE_PARAM(connection2),
                             unsigned         PTRACE_PARAM(sessionID),
                             bool             PTRACE_PARAM(started)) const
{
  PTRACE(3, "OnLocalRTP(" << connection1 << ',' << connection2 << ',' << sessionID << ',' << started);
  return false;
}


static bool PassOneThrough(OpalMediaStreamPtr source,
                           OpalMediaStreamPtr sink,
                           bool bypass)
{
  if (source == NULL) {
    PTRACE(2, "SetMediaPassThrough could not complete as source stream does not exist");
    return false;
  }

  if (sink == NULL) {
    PTRACE(2, "SetMediaPassThrough could not complete as sink stream does not exist");
    return false;
  }

  // Note SetBypassPatch() will do PTRACE() on status.
  return source->SetMediaPassThrough(*sink, bypass);
}


bool OpalManager::SetMediaPassThrough(OpalConnection & connection1,
                                      OpalConnection & connection2,
                                      bool bypass,
                                      unsigned sessionID)
{
  bool gotOne = false;

  if (sessionID != 0) {
    // Do not use || as McCarthy will not execute the second bypass
    if (PassOneThrough(connection1.GetMediaStream(sessionID, true), connection2.GetMediaStream(sessionID, false), bypass))
      gotOne = true;
    if (PassOneThrough(connection2.GetMediaStream(sessionID, true), connection1.GetMediaStream(sessionID, false), bypass))
      gotOne = true;
  }
  else {
    OpalMediaStreamPtr stream;
    while ((stream = connection1.GetMediaStream(OpalMediaType(), true , stream)) != NULL) {
      if (PassOneThrough(stream, connection2.GetMediaStream(stream->GetSessionID(), false), bypass))
        gotOne = true;
    }
    while ((stream = connection2.GetMediaStream(OpalMediaType(), true, stream)) != NULL) {
      if (PassOneThrough(stream, connection1.GetMediaStream(stream->GetSessionID(), false), bypass))
        gotOne = true;
    }
  }

  return gotOne;
}


bool OpalManager::SetMediaPassThrough(const PString & token1,
                                      const PString & token2,
                                      bool bypass,
                                      unsigned sessionID,
                                      bool network)
{
  PSafePtr<OpalCall> call1 = FindCallWithLock(token1);
  PSafePtr<OpalCall> call2 = FindCallWithLock(token2);

  if (call1 == NULL || call2 == NULL) {
    PTRACE(2, "SetMediaPassThrough could not complete as one call does not exist");
    return false;
  }

  PSafePtr<OpalConnection> connection1 = call1->GetConnection(0, PSafeReadOnly);
  while (connection1 != NULL && connection1->IsNetworkConnection() == network)
    ++connection1;

  PSafePtr<OpalConnection> connection2 = call2->GetConnection(0, PSafeReadOnly);
  while (connection2 != NULL && connection2->IsNetworkConnection() == network)
    ++connection2;

  if (connection1 == NULL || connection2 == NULL) {
    PTRACE(2, "SetMediaPassThrough could not complete as network connection not present in calls");
    return false;
  }

  return OpalManager::SetMediaPassThrough(*connection1, *connection2, bypass, sessionID);
}


void OpalManager::OnClosedMediaStream(const OpalMediaStream & PTRACE_PARAM(channel))
{
  PTRACE(5, "OnClosedMediaStream " << channel);
}


void OpalManager::OnFailedMediaStream(OpalConnection & PTRACE_PARAM(connection),
                                      bool PTRACE_PARAM(fromRemote),
                                      const PString & PTRACE_PARAM(reason))
{
  PTRACE(3, "OnFailedMediaStream \"" << reason << "\" " << (fromRemote ? "from remote" : "by local") << ' ' << connection);
}


#if OPAL_VIDEO

PBoolean OpalManager::CreateVideoInputDevice(const OpalConnection & connection,
                                         const OpalMediaFormat & mediaFormat,
                                         PVideoInputDevice * & device,
                                         PBoolean & autoDelete)
{
  // Make copy so we can adjust the size
  OpalVideoFormat::ContentRole role = mediaFormat.GetOptionEnum(OpalVideoFormat::ContentRoleOption(), OpalVideoFormat::eNoRole);
  PVideoDevice::OpenArgs args = m_videoInputDevice[role];
  PTRACE(3, "Using " << role << " video device \"" << args.deviceName << "\" for " << *this);
  mediaFormat.AdjustVideoArgs(args);
  return CreateVideoInputDevice(connection, args, device, autoDelete);
}


PBoolean OpalManager::CreateVideoInputDevice(const OpalConnection & /*connection*/,
                                         const PVideoDevice::OpenArgs & args,
                                         PVideoInputDevice * & device,
                                         PBoolean & autoDelete)
{
  autoDelete = true;
  device = PVideoInputDevice::CreateOpenedDevice(args, false);
  PTRACE_IF(3, device == NULL, "Could not open video input device \"" << args.deviceName << '"');
  return device != NULL;
}


PBoolean OpalManager::CreateVideoOutputDevice(const OpalConnection & connection,
                                          const OpalMediaFormat & mediaFormat,
                                          PBoolean preview,
                                          PVideoOutputDevice * & device,
                                          PBoolean & autoDelete)
{
  // Make copy so we can adjust the size
  OpalVideoFormat::ContentRole role = mediaFormat.GetOptionEnum(OpalVideoFormat::ContentRoleOption(), OpalVideoFormat::eNoRole);
  PVideoDevice::OpenArgs args = preview ? m_videoPreviewDevice[role] : m_videoOutputDevice[role];
  if (args.deviceName.IsEmpty() && args.driverName.IsEmpty()) {
    PTRACE(4, "No video " << (preview ? "preview" : "output") << " device specified for " << role);
    return false; // Disabled
  }

  mediaFormat.AdjustVideoArgs(args);

  if (preview) {
    static PConstString const LocalPreview("TITLE=\"Local Preview\"");
    args.deviceName.Replace("TITLE=\"Video Output\"", LocalPreview);
  }
  else {
    PINDEX start = args.deviceName.Find("TITLE=\"");
    if (start != P_MAX_INDEX) {
      start += 7;
      PINDEX end = args.deviceName.Find('"', start + 7) - 1;
      if (start == end)
        args.deviceName.Splice(connection.GetRemotePartyName(), start, 0);
      else if (args.deviceName(start, end) == "Video Output")
        args.deviceName.Splice(connection.GetRemotePartyName(), start, 13);
      else if ((start = args.deviceName.Find("%REMOTE%")) < end)
        args.deviceName.Splice(connection.GetRemotePartyName(), start, 8);
    }
  }

  return CreateVideoOutputDevice(connection, args, device, autoDelete);
}


bool OpalManager::CreateVideoOutputDevice(const OpalConnection & /*connection*/,
                                          const PVideoDevice::OpenArgs & args,
                                          PVideoOutputDevice * & device,
                                          PBoolean & autoDelete)
{
  autoDelete = true;
  device = PVideoOutputDevice::CreateOpenedDevice(args, false);
  PTRACE_IF(4, device == NULL, "Could not open video output device \"" << args.deviceName << '"');
  return device != NULL;
}

#endif // OPAL_VIDEO


OpalMediaPatch * OpalManager::CreateMediaPatch(OpalMediaStream & source,
                                               PBoolean requiresPatchThread)
{
  if (requiresPatchThread)
    return new OpalMediaPatch(source);
  else
    return new OpalPassiveMediaPatch(source);
}


#if OPAL_SCRIPT
static void OnStartStopMediaPatch(PScriptLanguage * script, const char * fn, OpalConnection & connection, OpalMediaPatch & patch)
{
  if (script == NULL)
    return;

  OpalMediaFormat mediaFormat = patch.GetSource().GetMediaFormat();
  script->Call(fn, "sss",
               (const char *)connection.GetCall().GetToken(),
               (const char *)patch.GetSource().GetID(),
               (const char *)mediaFormat.GetName());
}
#endif // OPAL_SCRIPT


void OpalManager::OnStartMediaPatch(OpalConnection & connection, OpalMediaPatch & patch)
{
#if OPAL_SCRIPT
  OnStartStopMediaPatch(m_script, "OnStartMedia", connection, patch);
#endif
  PTRACE(3, "OnStartMediaPatch " << patch << " on " << connection);

  if (&patch.GetSource().GetConnection() == &connection) {
    PSafePtr<OpalConnection> other = connection.GetOtherPartyConnection();
    if (other != NULL)
      other->OnStartMediaPatch(patch);
  }
}


void OpalManager::OnStopMediaPatch(OpalConnection & connection, OpalMediaPatch & patch)
{
  PTRACE(3, "OnStopMediaPatch " << patch << " on " << connection);

#if OPAL_SCRIPT
  OnStartStopMediaPatch(m_script, "OnStopMedia", connection, patch);
#endif

  if (&patch.GetSource().GetConnection() == &connection) {
    PSafePtr<OpalConnection> other = connection.GetOtherPartyConnection();
    if (other != NULL)
      other->OnStopMediaPatch(patch);
  }
}


bool OpalManager::OnMediaFailed(OpalConnection & connection, unsigned)
{
  if (connection.AllMediaFailed()) {
    PTRACE(2, "All media failed, releasing " << connection);
    connection.Release(OpalConnection::EndedByMediaFailed);
  }
  return true;
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
  PTRACE(3, "ReadUserInput from " << connection);

  connection.PromptUserInput(true);
  PString digit = connection.GetUserInput(firstDigitTimeout);
  connection.PromptUserInput(false);

  if (digit.IsEmpty()) {
    PTRACE(2, "ReadUserInput first character timeout (" << firstDigitTimeout << " seconds) on " << *this);
    return PString::Empty();
  }

  PString input;
  while (digit.FindOneOf(terminators) == P_MAX_INDEX) {
    input += digit;

    digit = connection.GetUserInput(lastDigitTimeout);
    if (digit.IsEmpty()) {
      PTRACE(2, "ReadUserInput last character timeout (" << lastDigitTimeout << " seconds) on " << *this);
      return input; // Input so far will have to do
    }
  }

  return input.IsEmpty() ? digit : input;
}


void OpalManager::OnMWIReceived(const PString & PTRACE_PARAM(party),
                                MessageWaitingType PTRACE_PARAM(type),
                                const PString & PTRACE_PARAM(extraInfo))
{
  PTRACE(3, "OnMWIReceived(" << party << ',' << type << ',' << extraInfo << ')');
}


bool OpalManager::GetConferenceStates(OpalConferenceStates & states, const PString & name) const
{
  PReadWaitAndSignal mutex(m_endpointsMutex);

  for (PList<OpalEndPoint>::const_iterator ep = m_endpointList.begin(); ep != m_endpointList.end(); ++ep) {
    if (ep->GetConferenceStates(states, name))
      return true;
  }

  return false;
}


void OpalManager::OnConferenceStatusChanged(OpalEndPoint & endpoint, const PString & uri, OpalConferenceState::ChangeType change)
{
  PTRACE(4, "OnConferenceStatusChanged(" << endpoint << ",\"" << uri << "\"," << change << ')');

  PReadWaitAndSignal mutex(m_endpointsMutex);

  for (PList<OpalEndPoint>::iterator ep = m_endpointList.begin(); ep != m_endpointList.end(); ++ep) {
    if (&endpoint != &*ep)
      ep->OnConferenceStatusChanged(endpoint, uri, change);
  }
}


bool OpalManager::OnChangedPresentationRole(OpalConnection & PTRACE_PARAM(connection),
                                           const PString & PTRACE_PARAM(newChairURI),
                                           bool)
{
  PTRACE(3, "OnChangedPresentationRole to " << newChairURI << " on " << connection);
  return true;
}


PStringList OpalManager::GetNetworkURIs(const PString & name) const
{
  PStringList list;

  PReadWaitAndSignal mutex(m_endpointsMutex);

  for (PList<OpalEndPoint>::const_iterator ep = m_endpointList.begin(); ep != m_endpointList.end(); ++ep)
    list += ep->GetNetworkURIs(name);

  return list;
}


OpalManager::RouteEntry::RouteEntry(const PString & partyA, const PString & partyB, const PString & dest)
  : m_partyA(partyA)
  , m_partyB(partyB)
  , m_destination(dest)
{
  CompileRegEx();
}


OpalManager::RouteEntry::RouteEntry(const PString & spec)
{
  // Make sure we test for backward compatibility format(s)

  PINDEX equal = spec.Find('=');
  if (equal == P_MAX_INDEX)
    return; // Must have

  m_destination = spec.Mid(equal+1).Trim();

  PINDEX colon = spec.Find(':');
  if (colon == 0)
    return; // No scheme?

  PINDEX tab = spec.Find('\t');
  if (tab == P_MAX_INDEX)
    tab = spec.Find("\\t");
  if (tab == 0)
    return; // No source?

  if (tab < equal) {
    m_partyA = spec.Left(tab).Trim();
    m_partyB = spec(tab+(spec[tab]=='\t'?1:2), equal-1).Trim();
  }
  else if (colon < equal) {
    m_partyA = spec.Left(colon+1)+".*";
    m_partyB = spec(colon+1, equal-1).Trim();
  }
  else
    m_partyA = spec.Left(equal).Trim();

  if (m_partyB.IsEmpty())
    m_partyB = ".*";

  CompileRegEx();
}


void OpalManager::RouteEntry::CompileRegEx()
{
  PStringStream pattern;
  pattern << "^(" << m_partyA << ")\t(" << m_partyB << ")$";
  if (!m_regex.Compile(pattern, PRegularExpression::IgnoreCase|PRegularExpression::Extended)) {
    PTRACE(1, "Could not compile route regular expression \"" << pattern << '"');
  }
}


void OpalManager::RouteEntry::PrintOn(ostream & strm) const
{
  strm << m_partyA << "\\t" << m_partyB << '=' << m_destination;
}


bool OpalManager::RouteEntry::IsValid() const
{
  return !m_destination.IsEmpty() && m_regex.GetErrorCode() == PRegularExpression::NoError;
}


bool OpalManager::RouteEntry::IsMatch(const PString & search) const
{
  PINDEX dummy;
  bool ok = m_regex.Execute(search, dummy);
  PTRACE(4, (ok ? "Matched" : "Did not match") << " regex \"" << m_regex.GetPattern() << "\" (" << *this << ')');
  return ok;
}


PBoolean OpalManager::AddRouteEntry(const PString & spec)
{
  if (spec[0] == '#') // Comment
    return false;

  if (spec[0] == '@') { // Load from file
    PTextFile file;
    if (!file.Open(spec.Mid(1), PFile::ReadOnly)) {
      PTRACE(1, "Could not open route file \"" << file.GetFilePath() << '"');
      return false;
    }
    PTRACE(4, "Adding routes from file \"" << file.GetFilePath() << '"');
    PBoolean ok = false;
    PString line;
    while (file.good()) {
      file >> line;
      if (AddRouteEntry(line))
        ok = true;
    }
    return ok;
  }

  RouteEntry * entry = new RouteEntry(spec);
  if (!entry->IsValid()) {
    PTRACE(2, "Illegal specification for route table entry: \"" << spec << '"');
    delete entry;
    return false;
  }

  PTRACE(4, "Added route \"" << *entry << '"');
  m_routeMutex.Wait();
  m_routeTable.Append(entry);
  m_routeMutex.Signal();
  return true;
}


PBoolean OpalManager::SetRouteTable(const PStringArray & specs)
{
  PBoolean ok = false;

  m_routeMutex.Wait();
  m_routeTable.RemoveAll();

  for (PINDEX i = 0; i < specs.GetSize(); i++) {
    if (AddRouteEntry(specs[i].Trim()))
      ok = true;
  }

  m_routeMutex.Signal();

  return ok;
}


void OpalManager::SetRouteTable(const RouteTable & table)
{
  m_routeMutex.Wait();
  m_routeTable = table;
  m_routeTable.MakeUnique();
  m_routeMutex.Signal();
}


PString OpalManager::ApplyRouteTable(const PString & a_party, const PString & b_party, PINDEX & routeIndex)
{
  PString destination;

  {
    PWaitAndSignal mutex(m_routeMutex);

    if (m_routeTable.IsEmpty())
      return routeIndex++ == 0 ? b_party : PString::Empty();

    PString search = a_party + '\t' + b_party;
    PTRACE(4, "Searching for route \"" << search << '"');

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

    while (routeIndex < m_routeTable.GetSize()) {
      RouteEntry & entry = m_routeTable[routeIndex++];
      if (entry.IsMatch(search)) {
        search = entry.GetDestination();

        if (search.NumCompare("label:") != EqualTo) {
          destination = search;
          break;
        }

        // restart search in table using label.
        routeIndex = 0;
      }
    }
  }

  // No route found
  if (destination.IsEmpty())
    return PString::Empty();

  // See if B-Party a parsable URL
  PURL b_url(b_party, NULL);

  // We are backward compatibility mode and the supplied address can be called
  if (destination.Find("<da>") != P_MAX_INDEX) {
    if (b_url.IsEmpty()) {
      PINDEX colon = b_party.Find(':');
      if (colon != P_MAX_INDEX && FindEndPoint(b_party.Left(colon)) != NULL)
        return b_party;
    }
    else {
      if (FindEndPoint(b_url.GetScheme()) != NULL)
        return b_party;
    }
  }

  PString user, nonUser, digits, nonDigits;
  if (b_url.IsEmpty()) {
    PINDEX colon = b_party.Find(':');
    if (colon == P_MAX_INDEX)
      colon = 0;
    else
      ++colon;

    PINDEX at = b_party.Find('@', colon);
    user = b_party(colon, at-1);
    nonUser = b_party.Mid('@');

    if (nonUser.IsEmpty() && PIPSocket::IsLocalHost(user.Left(user.Find(':')))) {
      nonUser = user;
      user.MakeEmpty();
    }

    PINDEX nonDigitPos = b_party.FindSpan("0123456789*#-.()", colon + (b_party[colon] == '+'));
    digits = b_party(colon, nonDigitPos-1);
    nonDigits = b_party.Mid(nonDigitPos);
  }
  else {
    user = b_url.GetUserName();
    nonUser = b_url.GetHostPort() + b_url.AsString(PURL::RelativeOnly);
    if (OpalIsE164(user)) {
      digits = user;
      nonDigits = nonUser;
    }
    else
      nonDigits = b_party;
  }

  // This hack is to avoid double '@'
  if (nonUser.Find('@') != P_MAX_INDEX) {
    PINDEX at = destination.Find('@');
    if (at != P_MAX_INDEX) {
      PINDEX du = destination.Find("<!du>", at);
      if (du != P_MAX_INDEX)
        destination.Delete(at, du-at);
    }
  }

  // Substute from source(s) to destination
  destination.Replace("<da>", b_party, true);
  destination.Replace("<db>", b_party, true);
  destination.Replace("<du>", user, true);
  destination.Replace("<!du>", nonUser, true);
  destination.Replace("<cu>", a_party(a_party.Find(':') + 1, a_party.Find('@') - 1), true);
  destination.Replace("<dn>", digits, true);
  destination.Replace("<!dn>", nonDigits, true);

  PINDEX pos;
  static PRegularExpression const DNx("<dn[1-9]>");
  while (DNx.Execute(destination, pos))
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
  m_productInfo = info;

  if (updateAll) {
    m_endpointsMutex.StartWrite();
    for (PList<OpalEndPoint>::iterator ep = m_endpointList.begin(); ep != m_endpointList.end(); ++ep)
      ep->SetProductInfo(info);
    m_endpointsMutex.EndWrite();
  }
}


void OpalManager::SetDefaultUserName(const PString & name, bool updateAll)
{
  m_defaultUserName = name;

  if (updateAll) {
    m_endpointsMutex.StartWrite();
    for (PList<OpalEndPoint>::iterator ep = m_endpointList.begin(); ep != m_endpointList.end(); ++ep)
      ep->SetDefaultLocalPartyName(name);
    m_endpointsMutex.EndWrite();
  }
}


void OpalManager::SetDefaultDisplayName(const PString & name, bool updateAll)
{
  m_defaultDisplayName = name;

  if (updateAll) {
    m_endpointsMutex.StartWrite();
    for (PList<OpalEndPoint>::iterator ep = m_endpointList.begin(); ep != m_endpointList.end(); ++ep)
      ep->SetDefaultDisplayName(name);
    m_endpointsMutex.EndWrite();
  }
}


#if OPAL_PTLIB_SSL
bool OpalManager::ApplySSLCredentials(const OpalEndPoint & /*ep*/,
                                    PSSLContext & context,
                                    bool create) const
{
  return context.SetCredentials(m_caFiles, m_certificateFile, m_privateKeyFile, create && m_autoCreateCertificate);
}
#endif


PBoolean OpalManager::IsLocalAddress(const PIPSocket::Address & ip) const
{
#if OPAL_PTLIB_NAT
  return m_natMethods->IsLocalAddress(ip);
#else
  /* Check if the remote address is a private IP, broadcast, or us */
  return ip.IsAny() || ip.IsBroadcast() || ip.IsRFC1918() || PIPSocket::IsLocalHost(ip);
#endif
}


PBoolean OpalManager::IsRTPNATEnabled(OpalConnection & /*conn*/,
                                      const PIPSocket::Address & localAddr,
                                      const PIPSocket::Address & peerAddr,
                                      const PIPSocket::Address & sigAddr,
                                      PBoolean PTRACE_PARAM(incoming))
{
  PTRACE(4, "Checking " << (incoming ? "incoming" : "outgoing") << " call for NAT: local=" << localAddr << ", peer=" << peerAddr << ", sig=" << sigAddr);

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
     sourceCallSignalAddress or SIP "Contact" field.

     So this is the first test to make: if those addresses the same, we will
     assume the other guy is public or LAN/VPN and either no NAT is involved,
     or we leave them in charge of any NAT traversal as he has the ability to
     do it. In either case we don't do anything.
     */

  if (peerAddr == sigAddr)
    return false;

  /* Next test is to see if BOTH addresses are "public", non RFC1918. There are
     some cases with proxies, particularly with SIP, where this is possible. We
     will assume that NAT never occurs between two public addresses though it
     could occur between two private addresses */

  if (!peerAddr.IsRFC1918() && !sigAddr.IsRFC1918())
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

  /* This looks for seriously confused systems were NAT is between two private
      networks. Unfortunately, we don't have a netmask so we can only guess based
      on the IP address class. */
  if ( peerAddr.IsRFC1918() && sigAddr.IsRFC1918() &&
      !peerAddr.IsSubNet(sigAddr, PIPAddress::GetAny(peerAddr.GetVersion())))
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
  if (!IsLocalAddress(localAddress))
    return false; // Is already translated

  if (IsLocalAddress(remoteAddress))
    return false; // Does not need to be translated

#if OPAL_PTLIB_NAT
  PNatMethod * natMethod = m_natMethods->GetMethod(localAddress);
  if (natMethod != NULL)
    return natMethod->GetExternalAddress(localAddress); // Translate it!
#endif

  return false; // Have nothing to translate it to
}


#if OPAL_PTLIB_NAT

bool OpalManager::SetNATServer(const PString & method,
                               const PString & server,
                               bool activate,
                               unsigned priority,
                               const PString & iface)
{
  PNatMethod * natMethod = m_natMethods->GetMethodByName(method);
  if (natMethod == NULL) {
    PTRACE(2, "Unknown NAT method \"" << method << '"');
    return false;
  }

  natMethod->Activate(activate);
  m_natMethods->SetMethodPriority(method, priority);

  natMethod->SetPortRanges(GetUDPPortRange().GetBase(), GetUDPPortRange().GetMax(),
                           GetRtpIpPortRange().GetBase(), GetRtpIpPortRange().GetMax());
  if (!natMethod->SetServer(server)) {
    PTRACE(2, "Invalid server \"" << server << "\" for " << method << " NAT method");
    return false;
  }

  if (activate) {
    PIPSocket::Address ifaceIP = PIPSocket::GetDefaultIpAny();
    if (!iface.IsEmpty() && !ifaceIP.FromString(iface)) {
      PTRACE(2, "Invalid interface \"" << iface << "\" for " << method << " NAT method");
      return false;
    }

    if (!natMethod->Open(ifaceIP)) {
      PTRACE(2, "Could not open server \"" << server << " for " << method << " NAT method");
      return false;
    }
  }

  PTRACE(3, "NAT: " << *natMethod);
  return true;
}


PString OpalManager::GetNATServer(const PString & method) const 
{ 
  PNatMethod * natMethod = m_natMethods->GetMethodByName(method);
  return natMethod == NULL ? PString::Empty() : natMethod->GetServer();
}
#endif  // OPAL_PTLIB_NAT


void OpalManager::SetTCPPorts(unsigned tcpBase, unsigned tcpMax)
{
  m_tcpPorts.Set(tcpBase, tcpMax, 49, 0);
}


void OpalManager::SetUDPPorts(unsigned udpBase, unsigned udpMax)
{
  m_udpPorts.Set(udpBase, udpMax, 99, 0);

#if OPAL_PTLIB_NAT
  GetNatMethods().SetPortRanges(GetUDPPortRange().GetBase(), GetUDPPortRange().GetMax(),
                                GetRtpIpPortRange().GetBase(), GetRtpIpPortRange().GetMax());
#endif
}


void OpalManager::SetRtpIpPorts(unsigned rtpIpBase, unsigned rtpIpMax)
{
  m_rtpIpPorts.Set(rtpIpBase&0xfffe, rtpIpMax&0xfffe, 198, 5000);

#if OPAL_PTLIB_NAT
  GetNatMethods().SetPortRanges(GetUDPPortRange().GetBase(), GetUDPPortRange().GetMax(),
                                GetRtpIpPortRange().GetBase(), GetRtpIpPortRange().GetMax());
#endif
}


const PIPSocket::QoS & OpalManager::GetMediaQoS(const OpalMediaType & type) const
{
  return m_mediaQoS[type];
}


void OpalManager::SetMediaQoS(const OpalMediaType & type, const PIPSocket::QoS & qos)
{
  m_mediaQoS[type] = qos;
}


BYTE OpalManager::GetMediaTypeOfService(const OpalMediaType & type) const
{
  return (BYTE)(m_mediaQoS[type].m_dscp << 2);
}


void OpalManager::SetMediaTypeOfService(const OpalMediaType & type, unsigned tos)
{
  m_mediaQoS[type].m_dscp = (tos>>2)&0x3f;
}


BYTE OpalManager::GetMediaTypeOfService() const
{
  return (BYTE)(m_mediaQoS[OpalMediaType::Audio()].m_dscp << 2);
}


void OpalManager::SetMediaTypeOfService(unsigned tos)
{
  for (MediaQoSMap::iterator it = m_mediaQoS.begin(); it != m_mediaQoS.end(); ++it)
    it->second.m_dscp = (tos>>2)&0x3f;
}


void OpalManager::SetAudioJitterDelay(unsigned minDelay, unsigned maxDelay)
{
  if (minDelay == 0) {
    // Disable jitter buffer completely if minimum is zero.
    m_jitterParams.m_minJitterDelay = m_jitterParams.m_maxJitterDelay = 0;
    return;
  }

  PAssert(minDelay <= 10000 && maxDelay <= 10000, PInvalidParameter);

  if (minDelay < 10)
    minDelay = 10;
  m_jitterParams.m_minJitterDelay = minDelay;

  if (maxDelay < minDelay)
    maxDelay = minDelay;
  m_jitterParams.m_maxJitterDelay = maxDelay;
}


void OpalManager::SetMediaFormatOrder(const PStringArray & order)
{
  m_mediaFormatOrder = order;
  PTRACE(3, "SetMediaFormatOrder(" << setfill(',') << order << ')');
}


void OpalManager::SetMediaFormatMask(const PStringArray & mask)
{
  m_mediaFormatMask = mask;
  PTRACE(3, "SetMediaFormatMask(" << setfill(',') << mask << ')');
}


#if OPAL_VIDEO
bool OpalManager::SetVideoInputDevice(const PVideoDevice::OpenArgs & args, OpalVideoFormat::ContentRole role)
{
  return args.Validate<PVideoInputDevice>(m_videoInputDevice[role]);
}


PBoolean OpalManager::SetVideoPreviewDevice(const PVideoDevice::OpenArgs & args, OpalVideoFormat::ContentRole role)
{
  return args.Validate<PVideoOutputDevice>(m_videoPreviewDevice[role]);
}


PBoolean OpalManager::SetVideoOutputDevice(const PVideoDevice::OpenArgs & args, OpalVideoFormat::ContentRole role)
{
  return args.Validate<PVideoOutputDevice>(m_videoOutputDevice[role]);
}

#endif // OPAL_VIDEO


void OpalManager::GarbageCollection()
{
#if OPAL_HAS_PRESENCE
  m_presentities.DeleteObjectsToBeRemoved();
#endif // OPAL_HAS_PRESENCE

  bool allCleared = m_activeCalls.DeleteObjectsToBeRemoved();

  m_endpointsMutex.StartRead();

  static PTimeInterval const SkipTime(0, 1);
  if (m_garbageCollectChangeTime.GetElapsed() > SkipTime) {
    for (PList<OpalEndPoint>::iterator ep = m_endpointList.begin(); ep != m_endpointList.end(); ++ep) {
      if (!ep->GarbageCollection())
        allCleared = false;
    }
  }

  m_endpointsMutex.EndRead();

  if (allCleared && m_clearingAllCallsCount != 0)
    m_allCallsCleared.Signal();
}


void OpalManager::CallDict::DeleteObject(PObject * object) const
{
  manager.DestroyCall(PDownCast(OpalCall, object));
}


void OpalManager::GarbageMain(PThread &, P_INT_PTR)
{
  while (!m_garbageCollectExit.Wait(1000))
    GarbageCollection();
}


void OpalManager::OnNewConnection(OpalConnection & /*conn*/)
{
}

#if OPAL_HAS_MIXER

bool OpalManager::StartRecording(const PString & callToken,
                                 const PFilePath & fn,
                                 const OpalRecordManager::Options & options)
{
  PSafePtr<OpalCall> call = m_activeCalls.Find(callToken, PSafeReadWrite);
  if (call == NULL)
    return false;

  return call->StartRecording(fn, options);
}


bool OpalManager::IsRecording(const PString & callToken)
{
  PSafePtr<OpalCall> call = FindCallWithLock(callToken, PSafeReadWrite);
  return call != NULL && call->IsRecording();
}


bool OpalManager::StopRecording(const PString & callToken)
{
  PSafePtr<OpalCall> call = m_activeCalls.Find(callToken, PSafeReadWrite);
  if (call == NULL)
    return false;

  call->StopRecording();
  return true;
}

#endif


void OpalManager::OnApplyStringOptions(OpalConnection &, OpalConnection::StringOptions & stringOptions)
{
  for (OpalConnection::StringOptions::iterator it  = m_defaultConnectionOptions.begin();
                                               it != m_defaultConnectionOptions.end(); ++it) {
    if (!stringOptions.Contains(it->first))
      stringOptions.SetAt(it->first, it->second);
  }
}


#if OPAL_HAS_PRESENCE
PSafePtr<OpalPresentity> OpalManager::AddPresentity(const PString & presentity)
{
  if (presentity.IsEmpty())
    return NULL;

  PSafePtr<OpalPresentity> oldPresentity = m_presentities.Find(presentity, PSafeReadWrite);
  if (oldPresentity != NULL)
    return oldPresentity;

  OpalPresentity * newPresentity = OpalPresentity::Create(*this, presentity);
  if (newPresentity == NULL)
    return NULL;

  PTRACE(4, "Added presentity for " << *newPresentity);
  m_presentities.SetAt(presentity, newPresentity);
  return PSafePtr<OpalPresentity>(newPresentity, PSafeReadWrite);
}


PSafePtr<OpalPresentity> OpalManager::GetPresentity(const PString & presentity, PSafetyMode mode)
{
  return m_presentities.Find(presentity, mode);
}


PStringList OpalManager::GetPresentities() const
{
  PStringList presentities;

  for (PSafePtr<OpalPresentity> presentity(m_presentities, PSafeReference); presentity != NULL; ++presentity)
    presentities += presentity->GetAOR().AsString();

  return presentities;
}


bool OpalManager::RemovePresentity(const PString & presentity)
{
  PTRACE(4, "Removing presentity for " << presentity);
  return m_presentities.RemoveAt(presentity);
}
#endif // OPAL_HAS_PRESENCE


#if OPAL_HAS_IM

void OpalManager::OnConversation(const OpalIMContext::ConversationInfo &)
{
}


PBoolean OpalManager::Message(const PString & to, const PString & body)
{
  OpalIM message;
  message.m_to   = to;
  message.m_bodies.SetAt(PMIMEInfo::TextPlain(), body);
  return Message(message);
}


PBoolean OpalManager::Message(const PURL & to, const PString & type, const PString & body, PURL & from, PString & conversationId)
{
  OpalIM message;
  message.m_to             = to;
  message.m_from           = from;
  message.m_conversationId = conversationId;
  message.m_bodies.SetAt(type, body);

  bool stat = Message(message);

  from           = message.m_from;
  conversationId = message.m_conversationId;

  return stat;
}


bool OpalManager::Message(OpalIM & message)
{
  OpalIMEndPoint * ep = FindEndPointAs<OpalIMEndPoint>(OpalIMEndPoint::Prefix());
  if (ep == NULL) {
    return false;
  }

  PSafePtr<OpalIMContext> context = ep->FindContextForMessageWithLock(message);
  if (context == NULL) {
    context = ep->CreateContext(message.m_from, message.m_to);
    if (context == NULL)
      return false;

    message.m_conversationId = context->GetID();
  }

  return context->Send(message.CloneAs<OpalIM>()) < OpalIMContext::DispositionErrors;
}


#if OPAL_HAS_PRESENCE
void OpalManager::OnMessageReceived(const OpalIM & message)
{
  // find a presentity to give the message to
  for (PSafePtr<OpalPresentity> presentity(m_presentities, PSafeReference); presentity != NULL; ++presentity) {
    if (message.m_to == presentity->GetAOR()) {
      presentity->OnReceivedMessage(message);
      break;
    }
  }
}
#else
void OpalManager::OnMessageReceived(const OpalIM &)
{
}
#endif // OPAL_HAS_PRESENCE


void OpalManager::OnMessageDisposition(const OpalIMContext::DispositionInfo & )
{
}


void OpalManager::OnCompositionIndication(const OpalIMContext::CompositionInfo &)
{
}

#endif // OPAL_HAS_IM


#if OPAL_SCRIPT
bool OpalManager::RunScript(const PString & script, const char * language)
{
  delete m_script;
  m_script = PFactory<PScriptLanguage>::CreateInstance(language);
  if (m_script == NULL)
    return false;

  if (!m_script->Load(script)) {
    delete m_script;
    m_script = NULL;
    return false;
  }

  m_script->CreateComposite(OPAL_SCRIPT_CALL_TABLE_NAME);
  return m_script->Run();
}
#endif // OPAL_SCRIPT


/////////////////////////////////////////////////////////////////////////////

#if OPAL_PTLIB_NAT
void OpalManager::OnInterfaceChange(PInterfaceMonitor &, PInterfaceMonitor::InterfaceChange entry)
{
  PIPSocket::Address addr;

  for (PNatMethods::iterator nat = m_natMethods->begin(); nat != m_natMethods->end(); ++nat) {
    if (entry.m_added) {
      if (!nat->GetInterfaceAddress(addr) || entry.GetAddress() != addr)
        nat->Open(entry.GetAddress());
    }
    else {
      if (nat->GetInterfaceAddress(addr) && entry.GetAddress() == addr)
        nat->Close();
    }
  }
}
#endif // OPAL_PTLIB_NAT


/////////////////////////////////////////////////////////////////////////////
