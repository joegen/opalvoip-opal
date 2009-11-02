/*
 * sippres.cxx
 *
 * SIP Presence classes for Opal
 *
 * Open Phone Abstraction Library (OPAL)
 *
 * Copyright (c) 2009 Post Increment
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
 * The Initial Developer of the Original Code is Post Increment
 *
 * Contributor(s): ______________________________________.
 *
 * $Revision: 22858 $
 * $Author: csoutheren $
 * $Date: 2009-06-12 22:50:19 +1000 (Fri, 12 Jun 2009) $
 */

/*
 * This code implements all or part of the following RFCs
 *
 * RFC 3856 "A Presence Event Package for the Session Initiation Protocol (SIP)"
 * RFC 3857 "A Watcher Information Event Template-Package for the Session Initiation Protocol (SIP)"
 * RFC 3858 "An Extensible Markup Language (XML) Based Format for Watcher Information"
 * RFC 4825 "The Extensible Markup Language (XML) Configuration Access Protocol (XCAP)"
 * RFC 4827 "An Extensible Markup Language (XML) Configuration Access Protocol (XCAP) Usage for Manipulating Presence Document Contents"
 * RFC 5025 "Presence Authorization Rules"
 *
 * This code does not implement the following RFCs
 *
 * RFC 5263 "Session Initiation Protocol (SIP) Extension for Partial Notification of Presence Information"
 *
 */


#include <ptlib.h>
#include <opal/buildopts.h>

#if P_EXPAT

#include <sip/sippres.h>
#include <ptclib/pdns.h>
#include <ptclib/pxml.h>

const PString & SIP_Presentity::DefaultPresenceServerKey() { static const PString s = "default_presence_server"; return s; }
const PString & SIP_Presentity::PresenceServerKey()        { static const PString s = "presence_server";         return s; }

static PFactory<OpalPresentity>::Worker<SIPLocal_Presentity> sip_local_PresentityWorker("sip-local");
static PFactory<OpalPresentity>::Worker<SIPXCAP_Presentity>  sip_xcap_PresentityWorker("sip-xcap");
static PFactory<OpalPresentity>::Worker<SIPOMA_Presentity>   sip_oma_PresentityWorker("sip-oma");
static bool sip_default_PresentityWorker = PFactory<OpalPresentity>::RegisterAs("sip", "sip-oma");

static const char * const AuthNames[OpalPresentity::NumAuthorisations] = { "allow", "block", "polite-block", "confirm" };


//////////////////////////////////////////////////////////////////////////////////////

SIP_Presentity::SIP_Presentity()
  : m_endpoint(NULL)
  , m_watcherInfoVersion(-1)
{
}


SIP_Presentity::~SIP_Presentity()
{
}


bool SIP_Presentity::SetDefaultPresentity(const PString & prefix)
{
  if (PFactory<OpalPresentity>::RegisterAs("sip", prefix))
    return true;

  PTRACE(1, "SIPPres\tCannot set default 'sip' presentity handler to '" << prefix << '\'');
  return false;
}


bool SIP_Presentity::Open()
{
  Close();

  // find the endpoint
  m_endpoint = dynamic_cast<SIPEndPoint *>(m_manager->FindEndPoint("sip"));
  if (m_endpoint == NULL) {
    PTRACE(1, "SIPPres\tCannot open SIP_Presentity without sip endpoint");
    return false;
  }

  m_watcherInfoVersion = -1;

  return true;
}


bool SIP_Presentity::IsOpen() const
{
  return m_endpoint != NULL;
}


bool SIP_Presentity::Close()
{
  m_endpoint = NULL;
  return true;
}


//////////////////////////////////////////////////////////////

SIPLocal_Presentity::~SIPLocal_Presentity()
{
  Close();
}


//////////////////////////////////////////////////////////////

const PString & SIPXCAP_Presentity::XcapRootKey()      { static const PString s = "xcap_root";      return s; }
const PString & SIPXCAP_Presentity::XcapAuthIdKey()    { static const PString s = "xcap_auth_id";   return s; }
const PString & SIPXCAP_Presentity::XcapPasswordKey()  { static const PString s = "xcap_password";  return s; }
const PString & SIPXCAP_Presentity::XcapAuthAuidKey()  { static const PString s = "xcap_auid";      return s; }
const PString & SIPXCAP_Presentity::XcapAuthFileKey()  { static const PString s = "xcap_authfile";  return s; }
const PString & SIPXCAP_Presentity::XcapBuddyListKey() { static const PString s = "xcap_buddylist"; return s; }

SIPXCAP_Presentity::SIPXCAP_Presentity()
{
  m_attributes.Set(SIPXCAP_Presentity::XcapAuthAuidKey,  "pres-rules");
  m_attributes.Set(SIPXCAP_Presentity::XcapAuthFileKey,  "index");
  m_attributes.Set(SIPXCAP_Presentity::XcapBuddyListKey, "buddylist");
}


SIPXCAP_Presentity::~SIPXCAP_Presentity()
{
  Close();
}


bool SIPXCAP_Presentity::Open()
{
  if (!SIP_Presentity::Open())
    return false;

  // find presence server for Presentity as per RFC 3861
  // if not found, look for default presence server setting
  // if none, use hostname portion of domain name
  if (!m_attributes.Has(SIP_Presentity::PresenceServerKey)) {
#if P_DNS
    PIPSocketAddressAndPortVector addrs;
    if (PDNS::LookupSRV(m_aor.GetHostName(), "_pres._sip", m_aor.GetPort(), addrs) && addrs.size() > 0) {
      PTRACE(1, "SIPPres\tSRV lookup for '" << m_aor.GetHostName() << "_pres._sip' succeeded");
      m_presenceServer = addrs[0];
    }
    else
#endif
      if (m_attributes.Has(SIP_Presentity::DefaultPresenceServerKey)) 
        m_presenceServer.Parse(m_attributes.Get(SIP_Presentity::DefaultPresenceServerKey), m_endpoint->GetDefaultSignalPort());
      else
        m_presenceServer.Parse(m_aor.GetHostName(), m_aor.GetPort());

    // set presence server
    m_attributes.Set(SIP_Presentity::PresenceServerKey, m_presenceServer.AsString());
  }

  if (!m_presenceServer.GetAddress().IsValid()) {
    PTRACE(1, "SIPPres\tUnable to lookup hostname for '" << m_presenceServer.GetAddress() << "'");
    return false;
  }

  m_watcherSubscriptionAOR.MakeEmpty();

  StartThread();

  // subscribe to presence watcher infoformation
  SendCommand(CreateCommand<SIPWatcherInfoCommand>());

  return true;
}


bool SIPXCAP_Presentity::Close()
{
  if (!IsOpen())
    return false;

  StopThread();

  m_notificationMutex.Wait();

  if (!m_watcherSubscriptionAOR.IsEmpty()) {
    PTRACE(3, "SIPPres\t'" << m_aor << "' sending unsubscribe for own presence watcher");
    m_endpoint->Unsubscribe(SIPSubscribe::Presence | SIPSubscribe::Watcher, m_watcherSubscriptionAOR);
  }

  for (StringMap::iterator subs = m_presenceIdByAor.begin(); subs != m_presenceIdByAor.end(); ++subs) {
    PTRACE(3, "SIPPres\t'" << m_aor << "' sending unsubscribe to " << subs->first);
    m_endpoint->Unsubscribe(SIPSubscribe::Presence, subs->second);
  }

  while (!m_watcherSubscriptionAOR.IsEmpty() || !m_presenceIdByAor.empty()) {
    m_notificationMutex.Signal();
    PThread::Sleep(100);
    m_notificationMutex.Wait();
  }

  m_notificationMutex.Signal();

  return SIP_Presentity::Close();
}


OPAL_DEFINE_COMMAND(OpalSetLocalPresenceCommand,     SIPXCAP_Presentity, Internal_SendLocalPresence);
OPAL_DEFINE_COMMAND(OpalSubscribeToPresenceCommand,  SIPXCAP_Presentity, Internal_SubscribeToPresence);
OPAL_DEFINE_COMMAND(OpalAuthorisationRequestCommand, SIPXCAP_Presentity, Internal_AuthorisationRequest);
OPAL_DEFINE_COMMAND(SIPWatcherInfoCommand,           SIPXCAP_Presentity, Internal_SubscribeToWatcherInfo);


void SIPXCAP_Presentity::Internal_SubscribeToWatcherInfo(const SIPWatcherInfoCommand & cmd)
{

  if (cmd.m_unsubscribe) {
    if (m_watcherSubscriptionAOR.IsEmpty()) {
      PTRACE(3, "SIPPres\tAlredy unsubscribed presence watcher for " << m_aor);
      return;
    }

    PTRACE(3, "SIPPres\t'" << m_aor << "' sending unsubscribe for own presence watcher");
    m_endpoint->Unsubscribe(SIPSubscribe::Presence | SIPSubscribe::Watcher, m_watcherSubscriptionAOR);
    return;
  }

  // subscribe to the presence.winfo event on the presence server
  SIPSubscribe::Params param(SIPSubscribe::Presence | SIPSubscribe::Watcher);

  PString aorStr = m_aor.AsString();
  PTRACE(3, "SIPPres\t'" << aorStr << "' sending subscribe for own presence.watcherinfo");

  param.m_localAddress     = aorStr;
  param.m_addressOfRecord  = aorStr;
  param.m_remoteAddress    = m_presenceServer.AsString();
  param.m_authID           = m_attributes.Get(OpalPresentity::AuthNameKey, aorStr);
  param.m_password         = m_attributes.Get(OpalPresentity::AuthPasswordKey);
  param.m_expire           = GetExpiryTime();
  param.m_onSubcribeStatus = PCREATE_NOTIFIER2(OnWatcherInfoSubscriptionStatus, const SIPSubscribe::SubscriptionStatus &);
  param.m_onNotify         = PCREATE_NOTIFIER2(OnWatcherInfoNotify, SIPSubscribe::NotifyCallbackInfo &);

  m_endpoint->Subscribe(param, m_watcherSubscriptionAOR);
}

static PXMLValidator::ElementInfo watcherValidation[] = {
  { PXMLValidator::RequiredNonEmptyAttribute,  "id"  },
  { PXMLValidator::RequiredAttributeWithValue, "status",  "pending\nactive\nwaiting\nterminated" },
  { PXMLValidator::RequiredAttributeWithValue, "event",   "subscribe\napproved\ndeactivated\nprobation\nrejected\ntimeout\ngiveup" },
  { 0 }
};

static PXMLValidator::ElementInfo watcherListValidation[] = {
  { PXMLValidator::RequiredNonEmptyAttribute,  "resource" },
  { PXMLValidator::RequiredAttributeWithValue, "package", "presence" },

  { PXMLValidator::Subtree,                    "watcher",  watcherValidation, "0" },
  { 0 }
};

static PXMLValidator::ElementInfo watcherInfoValidation[] = {
  { PXMLValidator::ElementName,                "watcherinfo", },
  { PXMLValidator::RequiredAttributeWithValue, "xmlns",   "urn:ietf:params:xml:ns:watcherinfo" },
  { PXMLValidator::RequiredNonEmptyAttribute,  "version"},
  { PXMLValidator::RequiredAttributeWithValue, "state",   "full\npartial" },

  { PXMLValidator::Subtree,                    "watcher-list", watcherListValidation, "0" },
  { 0 }
};


void SIPXCAP_Presentity::OnWatcherInfoSubscriptionStatus(SIPSubscribeHandler &, const SIPSubscribe::SubscriptionStatus & status)
{
  if (status.m_reason == SIP_PDU::Information_Trying)
    return;

  m_notificationMutex.Wait();
  if (!status.m_wasSubscribing)
    m_watcherSubscriptionAOR.MakeEmpty();
  m_notificationMutex.Signal();
}


void SIPXCAP_Presentity::OnWatcherInfoNotify(SIPSubscribeHandler & handler, SIPSubscribe::NotifyCallbackInfo & status)
{
  SIPMIMEInfo & sipMime(status.m_notify.GetMIME());

  if (sipMime.GetEvent() != "presence.winfo")
    return;

  // check the ContentType
  if (sipMime.GetContentType() != "application/watcherinfo+xml") {
    status.m_response.SetStatusCode(SIP_PDU::Failure_UnsupportedMediaType);
    status.m_response.GetMIME().SetAt("Accept", "application/watcherinfo+xml");
    status.m_response.GetInfo() = "Unsupported media type";
    PTRACE(3, "SIPPres\tReceived unsupported media type '" << status.m_notify.GetMIME().GetContentType() << "' for presence notify");
    return;
  }

  // load the XML
  PXML xml;
  if (!xml.Load(status.m_notify.GetEntityBody())) {
    status.m_response.SetStatusCode(SIP_PDU::Failure_BadRequest);
    PStringStream err;
    err << "XML parse error\nSyntax error in line " << xml.GetErrorLine() << ", col " << xml.GetErrorColumn() << "\n"
        << xml.GetErrorString() << endl;
    status.m_response.GetEntityBody() = err;
    PTRACE(3, "SIPPres\tError parsing XML in presence notify: " << err);
    return;
  }

  // validate the XML
  {
    PString errorString;
    PXMLValidator validator;
    if (!validator.Elements(&xml, watcherInfoValidation, errorString)) {
      status.m_response.SetStatusCode(SIP_PDU::Failure_BadRequest);
      PStringStream err;
      err << "XML parse error\nSyntax error in line " << xml.GetErrorLine() << ", col " << xml.GetErrorColumn() << "\n"
          << xml.GetErrorString() << endl;
      status.m_response.GetEntityBody() = err;
      PTRACE(3, "SIPPres\tError parsing XML in presence notify: " << err);
      return;
    }
  }

  // send 200 OK now, and flag caller NOT to send 20o OK
  status.m_sendResponse = false;
  status.m_response.SetStatusCode(SIP_PDU::Successful_OK);
  status.m_response.GetInfo() = "OK";
  status.m_response.SendResponse(*handler.GetTransport(), status.m_response, &handler.GetEndPoint());

  PTRACE(3, "SIPPres\t'" << m_aor << "' received NOTIFY for own presence.watcherinfo");

  PXMLElement * rootElement = xml.GetRootElement();

  int version = rootElement->GetAttribute("version").AsUnsigned();

  PWaitAndSignal mutex(m_notificationMutex);

  // check version number
  bool sendRefresh = false;
  if (m_watcherInfoVersion < 0)
    m_watcherInfoVersion = version;
  else {
    sendRefresh = version != m_watcherInfoVersion+1;
    version = m_watcherInfoVersion;
  }

  // if this is a full list of watcher info, we can empty out our pending lists
  if (rootElement->GetAttribute("state") *= "full") {
    PTRACE(3, "SIPPres\t'" << m_aor << "' received full watcher list");
    m_watcherAorById.clear();
  }

  // go through list of watcher information
  PINDEX watcherListIndex = 0;
  PXMLElement * watcherList = rootElement->GetElement("watcher-list", watcherListIndex++);

  // scan through the watcher list
  while (watcherList != NULL) {
    if (watcherList != NULL) {
      PString aor = watcherList->GetAttribute("resource");
      PINDEX watcherIndex = 0;
      PXMLElement * watcher = watcherList->GetElement("watcher", watcherIndex++);
      while (watcher != NULL) {
        OnReceivedWatcherStatus(watcher);
        watcher = watcherList->GetElement("watcher", watcherIndex++);
      }
    }
    watcherList = rootElement->GetElement("watcher-list", watcherListIndex++);
  }

  // send refresh, if needed
  if (sendRefresh) {
    PTRACE(3, "SIPPres\t'" << m_aor << "' received NOTIFY for own presence.watcherinfo without out of sequence version");
    // TODO
  }
}


void SIPXCAP_Presentity::OnReceivedWatcherStatus(PXMLElement * watcher)
{
  PString id       = watcher->GetAttribute("id");
  PString status   = watcher->GetAttribute("status");
  PString eventStr = watcher->GetAttribute("event");
  PString otherAOR = watcher->GetData().Trim();

  StringMap::iterator existingAOR = m_watcherAorById.find(id);

  // save pending subscription status from this user
  if (status == "pending") {
    if (existingAOR != m_watcherAorById.end()) {
      PTRACE(3, "SIPPres\t'" << m_aor << "' received followup to request from '" << otherAOR << "' for access to presence information");
    } 
    else {
      m_watcherAorById[id] = otherAOR;
      PTRACE(3, "SIPPres\t'" << otherAOR << "' has requested access to presence information of '" << m_aor << "'");
      OnAuthorisationRequest(otherAOR);
    }
  }
  else {
    PTRACE(3, "SIPPres\t'" << m_aor << "' has received presence status '" << status << "' for '" << otherAOR << "'");
  }
}


void SIPXCAP_Presentity::Internal_SendLocalPresence(const OpalSetLocalPresenceCommand & cmd)
{
  PTRACE(3, "SIPPres\t'" << m_aor << "' sending own presence " << cmd.m_state << "/" << cmd.m_note);

  m_localPresence.m_presenceAgent = m_presenceServer.AsString();
  m_localPresence.m_entity = m_aor.AsString();

  if (cmd.m_state == NoPresence) {
    m_localPresence.m_basic = SIPPresenceInfo::Closed;
    m_endpoint->PublishPresence(m_localPresence, 0);
    return;
  }

  if (cmd.m_state < (int)SIPPresenceInfo::NumBasicStates)
    m_localPresence.m_basic = (SIPPresenceInfo::BasicStates)cmd.m_state;
  else
    m_localPresence.m_basic = SIPPresenceInfo::Open;

  if (cmd.m_state > ExtendedBase)
    m_localPresence.m_activity = (SIPPresenceInfo::Activities)(cmd.m_state - ExtendedBase);

  m_localPresence.m_note = cmd.m_note;

  m_endpoint->PublishPresence(m_localPresence, GetExpiryTime());
}


unsigned SIPXCAP_Presentity::GetExpiryTime() const
{
  int ttl = m_attributes.Get(OpalPresentity::TimeToLiveKey, "300").AsInteger();
  return ttl > 0 ? ttl : 300;
}


void SIPXCAP_Presentity::Internal_SubscribeToPresence(const OpalSubscribeToPresenceCommand & cmd)
{
  if (cmd.m_subscribe) {
    if (m_presenceIdByAor.find(cmd.m_presentity) != m_presenceIdByAor.end()) {
      PTRACE(3, "SIPPres\t'" << m_aor << "' already subscribed to presence of '" << cmd.m_presentity << "'");
      return;
    }

    PTRACE(3, "SIPPres\t'" << m_aor << "' subscribing to presence of '" << cmd.m_presentity << "'");

    // subscribe to the presence event on the presence server
    SIPSubscribe::Params param(SIPSubscribe::Presence);

    param.m_localAddress    = m_aor.AsString();
    param.m_addressOfRecord = cmd.m_presentity;
    param.m_remoteAddress   = m_presenceServer.AsString();
    param.m_authID          = m_attributes.Get(OpalPresentity::AuthNameKey, m_aor.GetUserName());
    param.m_password        = m_attributes.Get(OpalPresentity::AuthPasswordKey);
    param.m_expire          = GetExpiryTime();

    param.m_onSubcribeStatus = PCREATE_NOTIFIER2(OnPresenceSubscriptionStatus, const SIPSubscribe::SubscriptionStatus &);
    param.m_onNotify         = PCREATE_NOTIFIER2(OnPresenceNotify, SIPSubscribe::NotifyCallbackInfo &);

    PString id;
    if (m_endpoint->Subscribe(param, id, false)) {
      m_presenceIdByAor[cmd.m_presentity] = id;
      m_presenceAorById[id] = cmd.m_presentity;
    }
  }
  else {
    StringMap::iterator id = m_presenceIdByAor.find(cmd.m_presentity);
    if (id == m_presenceIdByAor.end()) {
      PTRACE(3, "SIPPres\t'" << m_aor << "' already unsubscribed to presence of '" << cmd.m_presentity << "'");
      return;
    }

    PTRACE(3, "SIPPres\t'" << m_aor << "' unsubscribing to presence of '" << cmd.m_presentity << "'");
    m_endpoint->Unsubscribe(SIPSubscribe::Presence, id->second);
  }
}


void SIPXCAP_Presentity::OnPresenceSubscriptionStatus(SIPSubscribeHandler &, const SIPSubscribe::SubscriptionStatus & status)
{
  if (status.m_reason == SIP_PDU::Information_Trying)
    return;

  m_notificationMutex.Wait();
  if (!status.m_wasSubscribing) {
    PString id = status.m_handler->GetCallID();
    StringMap::iterator aor = m_presenceAorById.find(id);
    if (aor != m_presenceAorById.end()) {
      PTRACE(3, "SIPPres\t'" << m_aor << "' unsubscribed to presence of '" << aor->second << "'");
      m_presenceIdByAor.erase(aor->second);
      m_presenceAorById.erase(aor);
    }
    else {
      PTRACE(2, "SIPPres\tAlready unsubscribed for id " << id);
    }
  }
  m_notificationMutex.Signal();
}


void SIPXCAP_Presentity::OnPresenceNotify(SIPSubscribeHandler &, SIPSubscribe::NotifyCallbackInfo & status)
{
  const SIPMIMEInfo & sipMime = status.m_notify.GetMIME();

  if (!sipMime.Contains("Subscription-State")) {
    status.m_response.GetEntityBody() = "No Subscription-State field";
    PTRACE(3, "SIPPres\tNOTIFY received for presence subscribe without Subscription-State header");
    return;
  }

  PStringArray params = sipMime["Subscription-State"].Tokenise(';', false);
  if (params.GetSize() == 0) {
    status.m_response.GetEntityBody() = "Subscription-State field malformed";
    PTRACE(3, "SIPPres\tNOTIFY received for presence subscribe with malformed Subscription-State header");
    return;
  }

  if (params[0] *= "active") {
    ;
  }
  else if (params[0] *= "pending") {
    ;
  }
  else if (params[0] *= "terminated") {
    ;
  }
  else {
    ;
  }

  // get entity requesting access to presence information
  PString fromStr = status.m_notify.GetMIME().GetFrom();

  PTRACE(3, "SIPPres\t'" << fromStr << "' request for presence of '" << m_aor << "' is " << params[0]);

  // return OK;
  status.m_response.SetStatusCode(SIP_PDU::Successful_OK);
  status.m_response.GetInfo() = "OK";
}


bool SIPXCAP_Presentity::ChangeAuthNode(XCAPClient & xcap, const OpalAuthorisationRequestCommand & cmd)
{
  PString ruleId = m_authorisationIdByAor[cmd.m_presentity];

  XCAPClient::NodeSelector node;
  node.SetNamespace("urn:ietf:params:xml:ns:pres-rules", "pr");
  node.SetNamespace("urn:ietf:params:xml:ns:common-policy", "cr");
  node.AddElement("cr:ruleset");
  node.AddElement("cr:rule", "id", ruleId);

  PXML xml;
  if (!xcap.GetXmlNode(node, xml)) {
    PTRACE(4, "SIPPres\tCould not locate existing rule id=" << ruleId
           << " for '" << cmd.m_presentity << "' at '" << m_aor << "\'\n"
           << xcap.GetLastResponseCode() << ' ' << xcap.GetLastResponseInfo());
    return false;
  }

  PXMLElement * root, * actions, * subHandling = NULL;
  if ((root = xml.GetRootElement()) == NULL ||
      (actions = root->GetElement("cr:actions")) == NULL ||
      (subHandling = actions->GetElement("pr:sub-handling")) == NULL) {
    PTRACE(2, "SIPPres\tInvalid XML in existing rule id=" << ruleId
           << " for '" << cmd.m_presentity << "' at '" << m_aor << '\'');
    return false;
  }

  if (subHandling->GetData() *= AuthNames[cmd.m_authorisation]) {
    PTRACE(3, "SIPPres\tRule id=" << ruleId << " already set to "
           << AuthNames[cmd.m_authorisation] << " for '" << cmd.m_presentity << "' at '" << m_aor << '\'');
    return true;
  }

  // Adjust the authorisation
  subHandling->SetData(AuthNames[cmd.m_authorisation]);

  if (xcap.PutXmlNode(node, xml)) {
    PTRACE(3, "SIPPres\tRule id=" << ruleId << " changed to"
           << AuthNames[cmd.m_authorisation] << " for '" << cmd.m_presentity << "' at '" << m_aor << '\'');
    return true;
  }

  PTRACE(3, "SIPPres\tCould not change existing rule id=" << ruleId
         << " for '" << cmd.m_presentity << "' at '" << m_aor << "\'\n"
         << xcap.GetLastResponseCode() << ' '  << xcap.GetLastResponseInfo());
  return false;
}


static PString GetNewRuleId()
{
  static PAtomicInteger NextRuleId(PRandom::Number());
  return PString(PString::Printf, "R%08X", ++NextRuleId);  // As per http://www.w3.org/TR/1999/REC-xml-names-19990114/#NT-NCName
}


void SIPXCAP_Presentity::Internal_AuthorisationRequest(const OpalAuthorisationRequestCommand & cmd)
{
  XCAPClient xcap;
  xcap.SetRoot(m_attributes.Get(SIPXCAP_Presentity::XcapRootKey));
  xcap.SetApplicationUniqueID(m_attributes.Get(SIPXCAP_Presentity::XcapAuthAuidKey)); // As per RFC5025/9.1
  xcap.SetContentType("application/auth-policy+xml");   // As per RFC5025/9.4
  xcap.SetUserIdentifier(m_aor.AsString());             // As per RFC5025/9.7
  xcap.SetAuthenticationInfo(m_attributes.Get(SIPXCAP_Presentity::XcapAuthIdKey, m_attributes.Get(OpalPresentity::AuthNameKey, xcap.GetUserIdentifier())),
                             m_attributes.Get(SIPXCAP_Presentity::XcapPasswordKey, m_attributes.Get(OpalPresentity::AuthPasswordKey)));
  xcap.SetDefaultFilename(m_attributes.Get(SIPXCAP_Presentity::XcapAuthFileKey));

  // See if we can use teh quick method
  if (m_authorisationIdByAor.find(cmd.m_presentity) != m_authorisationIdByAor.end()) {
    if (ChangeAuthNode(xcap, cmd))
      return;
  }

  PXML xml;
  PXMLElement * element;

  // Could not find rule element quickly, get the whole document
  if (!xcap.GetXmlDocument(xml)) {
    if (xcap.GetLastResponseCode() != PHTTP::NotFound) {
      PTRACE(2, "SIPPres\tUnexpected error getting rule file for '"
             << cmd.m_presentity << "' at '" << m_aor << "\'\n"
             << xcap.GetLastResponseCode() << ' '  << xcap.GetLastResponseInfo());
      return;
    }

    PString newRuleId = GetNewRuleId();
    // No whole document, create a fresh one
    xml.Load("<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
             "<cr:ruleset xmlns:pr=\"urn:ietf:params:xml:ns:pres-rules\""
                        " xmlns:cr=\"urn:ietf:params:xml:ns:common-policy\">"
               "<cr:rule>"
                 "<cr:conditions>"
                   "<cr:identity>"
                     "<cr:one/>"
                   "</cr:identity>"
                 "</cr:conditions>"
                 "<cr:actions>"
                   "<pr:sub-handling></pr:sub-handling>"
                 "</cr:actions>"
                 "<cr:transformations>"
                   "<pr:provide-services>"
                     "<pr:service-uri-scheme></pr:service-uri-scheme>"
                   "</pr:provide-services>"
                   "<pr:provide-persons>"
                     "<pr:all-persons/>"
                   "</pr:provide-persons>"
                   "<pr:provide-activities>true</pr:provide-activities>"
                   "<pr:provide-user-input>bare</pr:provide-user-input>"
                 "</cr:transformations>"
               "</cr:rule>"
             "</cr:ruleset>");
    element = xml.GetElement("cr:rule");
    element->SetAttribute("id", newRuleId);
    element->GetElement("cr:conditions")->GetElement("cr:identity")->GetElement("cr:one")->SetAttribute("id", cmd.m_presentity);
    element->GetElement("cr:actions")->GetElement("pr:sub-handling")->SetData(AuthNames[cmd.m_authorisation]);
    element->GetElement("cr:transformations")->GetElement("pr:provide-services")->GetElement("pr:service-uri-scheme")->SetData(m_aor.GetScheme());

    if (xcap.PutXmlDocument(xml)) {
      PTRACE(3, "SIPPres\tNew rule file set to " << AuthNames[cmd.m_authorisation] << " for '" << cmd.m_presentity << "' at '" << m_aor << '\'');
      m_authorisationIdByAor[cmd.m_presentity] = newRuleId;
    }
    else {
      PTRACE(2, "SIPPres\tCould not add new rule file for '" << cmd.m_presentity << "' at '" << m_aor << "\'\n"
             << xcap.GetLastResponseCode() << ' '  << xcap.GetLastResponseInfo());
    }

    return;
  }

  // Extract all the rules from it
  m_authorisationIdByAor.clear();

  bool existingRuleForWatcher = false;

  PINDEX ruleIndex = 0;
  while ((element = xml.GetElement("cr:rule", ruleIndex++)) != NULL) {
    PString ruleId = element->GetAttribute("id");
    if ((element = element->GetElement("cr:conditions")) != NULL &&
        (element = element->GetElement("cr:identity")) != NULL &&
        (element = element->GetElement("cr:one")) != NULL) {
      PString watcher = element->GetAttribute("id");

      m_authorisationIdByAor[watcher] = ruleId;
      PTRACE(4, "SIPPres\tGetting rule id=" << ruleId << " for '" << watcher << "' at '" << m_aor << '\'');

      if (watcher == cmd.m_presentity)
        existingRuleForWatcher = true;
    }
  }

  if (existingRuleForWatcher) {
    ChangeAuthNode(xcap, cmd);
    return;
  }

  // Create new rule
  PString newRuleId = GetNewRuleId();
  xml.Load("<cr:rule xmlns:pr=\"urn:ietf:params:xml:ns:pres-rules\""
                   " xmlns:cr=\"urn:ietf:params:xml:ns:common-policy\">"
             "<cr:conditions>"
               "<cr:identity>"
                 "<cr:one/>"
               "</cr:identity>"
             "</cr:conditions>"
             "<cr:actions>"
               "<pr:sub-handling></pr:sub-handling>"
             "</cr:actions>"
             "<cr:transformations>"
               "<pr:provide-services>"
                 "<pr:service-uri-scheme></pr:service-uri-scheme>"
               "</pr:provide-services>"
               "<pr:provide-persons>"
                 "<pr:all-persons/>"
               "</pr:provide-persons>"
               "<pr:provide-activities>true</pr:provide-activities>"
               "<pr:provide-user-input>bare</pr:provide-user-input>"
             "</cr:transformations>"
           "</cr:rule>",
           PXML::FragmentOnly);
  element = xml.GetRootElement();
  element->SetAttribute("id", GetNewRuleId());
  element->GetElement("cr:conditions")->GetElement("cr:identity")->GetElement("cr:one")->SetAttribute("id", cmd.m_presentity);
  element->GetElement("cr:actions")->GetElement("pr:sub-handling")->SetData(AuthNames[cmd.m_authorisation]);
  element->GetElement("cr:transformations")->GetElement("pr:provide-services")->GetElement("pr:service-uri-scheme")->SetData(m_aor.GetScheme());

  XCAPClient::NodeSelector node;
  node.SetNamespace("urn:ietf:params:xml:ns:pres-rules", "pr");
  node.SetNamespace("urn:ietf:params:xml:ns:common-policy", "cr");
  node.AddElement("cr:ruleset");
  node.AddElement("cr:rule", "id", newRuleId);

  if (xcap.PutXmlNode(node, xml)) {
    PTRACE(3, "SIPPres\tNew rule set to" << AuthNames[cmd.m_authorisation] << " for '" << cmd.m_presentity << "' at '" << m_aor << '\'');
    m_authorisationIdByAor[cmd.m_presentity] = newRuleId;
    return;
  }

  PTRACE(2, "SIPPres\tCould not add new rule for '" << cmd.m_presentity << "' at '" << m_aor << "\'\n"
         << xcap.GetLastResponseCode() << ' '  << xcap.GetLastResponseInfo());
}


void SIPXCAP_Presentity::InitBuddyXcap(XCAPClient & xcap, XCAPClient::NodeSelector & node, const PString & entry)
{
  xcap.SetRoot(m_attributes.Get(SIPXCAP_Presentity::XcapRootKey));
  xcap.SetApplicationUniqueID("resource-lists");            // As per RFC5025/9.1
  xcap.SetContentType("application/resource-lists+xml");    // As per RFC5025/9.4
  xcap.SetUserIdentifier(m_aor.AsString());                 // As per RFC5025/9.7
  xcap.SetAuthenticationInfo(m_attributes.Get(SIPXCAP_Presentity::XcapAuthIdKey, m_attributes.Get(OpalPresentity::AuthNameKey, xcap.GetUserIdentifier())),
                             m_attributes.Get(SIPXCAP_Presentity::XcapPasswordKey, m_attributes.Get(OpalPresentity::AuthPasswordKey)));
  xcap.SetDefaultFilename("index");

  node.SetNamespace("urn:ietf:params:xml:ns:resource-lists");
  node.AddElement("resource-lists");
  node.AddElement("list", "name", m_attributes.Get(SIPXCAP_Presentity::XcapBuddyListKey));

  if (!entry.IsEmpty())
    node.AddElement("entry", "uri", entry);
}


static PXMLElement * BuddyInfoToXML(const OpalPresentity::BuddyInfo & buddy, PXMLElement * parent)
{
  PXMLElement * element = new PXMLElement(parent, "entry");
  element->SetAttribute("uri", buddy.m_presentity);

  if (!buddy.m_displayName.IsEmpty())
    element->AddChild(new PXMLElement(element, "display-name", buddy.m_displayName));

  return element;
}


static bool XMLToBuddyInfo(const PXMLElement * element, OpalPresentity::BuddyInfo & buddy)
{
  if (element == NULL || element->GetName() != "entry")
    return false;

  buddy.m_presentity = element->GetAttribute("uri");

  PXMLElement * displayName = element->GetElement("display-name");
  if (displayName != NULL)
    buddy.m_displayName = displayName->GetData();

  buddy.m_contentType = "application/resource-lists+xml";
  buddy.m_rawXML = element->AsString();
  return true;
}


bool SIPXCAP_Presentity::GetBuddyList(BuddyList & buddies)
{
  XCAPClient xcap;
  XCAPClient::NodeSelector node;

  InitBuddyXcap(xcap, node, PString::Empty());

  PXML xml;
  if (!xcap.GetXmlNode(node, xml)) {
    PTRACE(2, "SIPPres\tError getting buddy list of '" << m_aor << "\'\n"
           << xcap.GetLastResponseCode() << ' '  << xcap.GetLastResponseInfo());
    return false;
  }

  PXMLElement * element;
  PINDEX idx = 0;
  while ((element = xml.GetElement("entry", idx++)) != NULL) {
    BuddyInfo buddy;
    if (XMLToBuddyInfo(element, buddy))
      buddies.push_back(buddy);
  }

  return true;
}


bool SIPXCAP_Presentity::SetBuddyList(const BuddyList & buddies)
{
  PXMLElement * root = new PXMLElement(NULL, "list");
  root->SetAttribute("xmlns", "urn:ietf:params:xml:ns:resource-lists");
  root->SetAttribute("name", m_attributes.Get(SIPXCAP_Presentity::XcapBuddyListKey));

  for (BuddyList::const_iterator it = buddies.begin(); it != buddies.end(); ++it)
    root->AddChild(BuddyInfoToXML(*it, root));

  PXML xml(PXML::FragmentOnly);
  xml.SetRootElement(root);

  XCAPClient xcap;
  XCAPClient::NodeSelector node;

  InitBuddyXcap(xcap, node, PString::Empty());

  if (xcap.PutXmlNode(node, xml))
    return true;

  if (xcap.GetLastResponseCode() == PHTTP::Conflict && xcap.GetLastResponseInfo().Find("Parent") != P_MAX_INDEX) {
    // Got "Parent does not exist" error, so need to add whole file
    root = new PXMLElement(NULL, "resource-lists");
    root->SetAttribute("xmlns", "urn:ietf:params:xml:ns:resource-lists");

    PXMLElement * listElement = root->AddChild(new PXMLElement(root, "list"));
    listElement->SetAttribute("name", m_attributes.Get(SIPXCAP_Presentity::XcapBuddyListKey));

    for (BuddyList::const_iterator it = buddies.begin(); it != buddies.end(); ++it)
      listElement->AddChild(BuddyInfoToXML(*it, root));

    xml.SetRootElement(root);
    xml.SetOptions(PXML::NoOptions);
    if (xcap.PutXmlDocument(xml))
      return true;
  }

  PTRACE(2, "SIPPres\tError setting buddy list of '" << m_aor << "\'\n"
         << xcap.GetLastResponseCode() << ' '  << xcap.GetLastResponseInfo());
  return false;
}


bool SIPXCAP_Presentity::DeleteBuddyList()
{
  XCAPClient xcap;
  XCAPClient::NodeSelector node;

  InitBuddyXcap(xcap, node, PString::Empty());

  if (xcap.DeleteXmlNode(node))
    return true;

  PTRACE(2, "SIPPres\tError deleting buddy list of '" << m_aor << "\'\n"
         << xcap.GetLastResponseCode() << ' '  << xcap.GetLastResponseInfo());
  return false;
}


bool SIPXCAP_Presentity::GetBuddy(BuddyInfo & buddy)
{
  XCAPClient xcap;
  XCAPClient::NodeSelector node;

  InitBuddyXcap(xcap, node, buddy.m_presentity);

  PXML xml(PXML::FragmentOnly);
  if (xcap.GetXmlNode(node, xml))
    return XMLToBuddyInfo(xml.GetRootElement(), buddy);

  PTRACE(2, "SIPPres\tError getting buddy '" << buddy.m_presentity << "' of '" << m_aor << "\'\n"
         << xcap.GetLastResponseCode() << ' '  << xcap.GetLastResponseInfo());
  return false;
}


bool SIPXCAP_Presentity::SetBuddy(const BuddyInfo & buddy)
{
  if (buddy.m_presentity.IsEmpty())
    return false;

  XCAPClient xcap;
  XCAPClient::NodeSelector node;

  InitBuddyXcap(xcap, node, buddy.m_presentity);

  PXML xml(PXML::FragmentOnly);
  xml.SetRootElement(BuddyInfoToXML(buddy, NULL));

  if (xcap.PutXmlNode(node, xml))
    return true;

  if (xcap.GetLastResponseCode() != PHTTP::Conflict || xcap.GetLastResponseInfo().Find("Parent") == P_MAX_INDEX) {
    PTRACE(2, "SIPPres\tError setting buddy '" << buddy.m_presentity << "' of '" << m_aor << "\'\n"
           << xcap.GetLastResponseCode() << ' '  << xcap.GetLastResponseInfo());
    return false;
  }

  // Got "Parent does not exist" error, so need to add whole list
  BuddyList buddies;
  buddies.push_back(buddy);
  return SetBuddyList(buddies);
}


bool SIPXCAP_Presentity::DeleteBuddy(const PString & presentity)
{
  PXML xml;
  XCAPClient xcap;
  XCAPClient::NodeSelector node;

  InitBuddyXcap(xcap, node, presentity);

  if (xcap.DeleteXmlNode(node))
    return true;

  PTRACE(2, "SIPPres\tError deleting buddy '" << presentity << "' of '" << m_aor << "\'\n"
         << xcap.GetLastResponseCode() << ' '  << xcap.GetLastResponseInfo());
  return false;
}


bool SIPXCAP_Presentity::SubscribeBuddyList()
{
  return false;
}


//////////////////////////////////////////////////////////////

SIPOMA_Presentity::SIPOMA_Presentity()
{
  m_attributes.Set(SIPXCAP_Presentity::XcapAuthAuidKey,  "org.openmobilealliance.pres-rules");
  m_attributes.Set(SIPXCAP_Presentity::XcapAuthFileKey,  "pres-rules");
  m_attributes.Set(SIPXCAP_Presentity::XcapBuddyListKey, "oma_buddylist");
}


//////////////////////////////////////////////////////////////

XCAPClient::XCAPClient()
  : m_global(false)
{
}


PURL XCAPClient::BuildURL(const PString & name, const NodeSelector & node)
{
  PURL uri(m_root);                              // XCAP root

  uri.AppendPath(m_auid);                        // Application Unique ID

  uri.AppendPath(m_global ? "global" : "users"); // RFC4825/6.2, The path segment after the AUID MUST either be "global" or "users".

  if (!m_global)
    uri.AppendPath(m_xui);                       // XCAP User Identifier

  if (!name.IsEmpty()) {
    uri.AppendPath(name);                        // Final resource name
    node.AddToURL(uri);
  }

  return uri;
}


bool XCAPClient::GetXmlDocument(const PString & name, PXML & xml)
{
  PString str;
  if (!GetTextDocument(BuildURL(name, NodeSelector()), str, m_contentType))
    return false;

  return xml.Load(str);
}


bool XCAPClient::PutXmlDocument(const PString & name, const PXML & xml)
{
  PStringStream strm;
  strm << xml;
  return PutTextDocument(BuildURL(name, NodeSelector()), strm, m_contentType);
}


bool XCAPClient::DeleteXmlDocument(const PString & name)
{
  return DeleteDocument(BuildURL(name, NodeSelector()));
}


bool XCAPClient::GetXmlNode(const PString & docname, const NodeSelector & node, PXML & xml)
{
  PString str;
  if (!GetTextDocument(BuildURL(docname, node), str, "application/xcap-el+xml"))
    return false;

  return xml.Load(str, PXML::FragmentOnly);
}


bool XCAPClient::PutXmlNode(const PString & docname, const NodeSelector & node, const PXML & xml)
{
  PStringStream strm;
  strm << xml;
  return PutTextDocument(BuildURL(docname, node), strm, "application/xcap-el+xml");
}


bool XCAPClient::DeleteXmlNode(const PString & docname, const NodeSelector & node)
{
  return DeleteDocument(BuildURL(docname, node));
}


PString XCAPClient::ElementSelector::AsString() const
{
  PStringStream strm;

  strm << m_name;

  if (!m_position.IsEmpty())
    strm << '[' << m_position << ']';

  if (!m_attribute.IsEmpty())
    strm << "[@" << m_attribute << "=\"" << m_value << "\"]";

  return strm;
}


void XCAPClient::NodeSelector::AddToURL(PURL & uri) const
{
  if (empty())
    return;

  uri.AppendPath("~~");                      // Node selector

  for (const_iterator it = begin(); it != end(); ++it)
    uri.AppendPath(it->AsString());

  if (m_namespaces.empty())
    return;

  PStringStream query;
  for (std::map<PString, PString>::const_iterator it = m_namespaces.begin(); it != m_namespaces.end(); ++it) {
    query << "xmlns(";
    if (!it->first.IsEmpty())
      query << it->first << '=';
    query << it->second << ')';
  }

  // Non-standard format query parameter, we fake that by having one
  // dictionary element at key value empty string
  uri.SetQueryVar(PString::Empty(), query);
}


#endif // P_EXPAT
