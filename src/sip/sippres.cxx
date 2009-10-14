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
static PFactory<OpalPresentity>::Worker<SIPXCAP_Presentity>  sip_xcap_PresentityWorker1("sip-xcap");
static PFactory<OpalPresentity>::Worker<SIPXCAP_Presentity>  sip_xcap_PresentityWorker2("sip");


//////////////////////////////////////////////////////////////////////////////////////

SIP_Presentity::SIP_Presentity()
  : m_endpoint(NULL)
  , m_localPresence(NoPresence)
{ 
}

SIP_Presentity::~SIP_Presentity()
{
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

  if (!InternalOpen()) {
    PTRACE(1, "SIPPres\tCannot open presence client");
    return false;
  }

  return true;
}


bool SIP_Presentity::IsOpen() const
{
  return m_endpoint != NULL;
}


//////////////////////////////////////////////////////////////

SIPLocal_Presentity::~SIPLocal_Presentity()
{
  Close();
}


bool SIPLocal_Presentity::InternalOpen()
{
  return true;
}


bool SIPLocal_Presentity::Close()
{
  m_endpoint = NULL;
  return true;
}


//////////////////////////////////////////////////////////////

const PString & SIPXCAP_Presentity::XcapRootKey()     { static const PString s = "xcap_root";     return s; }
const PString & SIPXCAP_Presentity::XcapAuthIdKey()   { static const PString s = "xcap_auth_id";  return s; }
const PString & SIPXCAP_Presentity::XcapPasswordKey() { static const PString s = "xcap_password"; return s; }

SIPXCAP_Presentity::SIPXCAP_Presentity()
{
}


SIPXCAP_Presentity::~SIPXCAP_Presentity()
{
  Close();
}


bool SIPXCAP_Presentity::InternalOpen()
{
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

  // initialised variables
  m_watcherSubscriptionAOR.MakeEmpty();
  m_watcherInfoVersion = -1;

  StartThread();

  // subscribe to presence watcher infoformation
  SendCommand(CreateCommand<SIPWatcherInfoCommand>());


  return true;
}

bool SIPXCAP_Presentity::Close()
{
  if (m_endpoint == NULL)
    return false;

  StopThread();

  Internal_SubscribeToWatcherInfo(true);

  m_notificationMutex.Wait();
  while (!m_watcherSubscriptionAOR.IsEmpty()) {
    m_notificationMutex.Signal();
    PThread::Sleep(100);
    m_notificationMutex.Wait();
  }
  m_notificationMutex.Signal();

  m_endpoint = NULL;

  return true;
}


OPAL_DEFINE_COMMAND(OpalSetLocalPresenceCommand,     SIPXCAP_Presentity, Internal_SendLocalPresence);
OPAL_DEFINE_COMMAND(OpalSubscribeToPresenceCommand,  SIPXCAP_Presentity, Internal_SubscribeToPresence);
OPAL_DEFINE_COMMAND(OpalAuthorisationRequestCommand, SIPXCAP_Presentity, Internal_AuthorisationRequest);
OPAL_DEFINE_COMMAND(SIPWatcherInfoCommand,           SIPXCAP_Presentity, Internal_SubscribeToWatcherInfo);


void SIPXCAP_Presentity::Internal_SubscribeToWatcherInfo(const SIPWatcherInfoCommand & cmd)
{
  Internal_SubscribeToWatcherInfo(cmd.m_unsubscribe);
}


void SIPXCAP_Presentity::Internal_SubscribeToWatcherInfo(bool unsubscribe)
{

  if (unsubscribe) {
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

  param.m_localAddress    = aorStr;
  param.m_addressOfRecord = aorStr;
  param.m_remoteAddress   = m_presenceServer.AsString();
  param.m_authID          = m_attributes.Get(OpalPresentity::AuthNameKey, aorStr);
  param.m_password        = m_attributes.Get(OpalPresentity::AuthPasswordKey);
  param.m_expire          = GetExpiryTime();
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
    m_idToAorMap.clear();
    m_aorToIdMap.clear();
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
  IdToAorMap::iterator r = m_idToAorMap.find(id);

  // save pending subscription status from this user
  if (status == "pending") {
    if (r != m_idToAorMap.end()) {
      PTRACE(3, "SIPPres\t'" << m_aor << "' received followup to request from '" << otherAOR << "' for access to presence information");
    } 
    else {
      m_idToAorMap.insert(IdToAorMap::value_type(id, otherAOR));
      m_aorToIdMap.insert(AorToIdMap::value_type(otherAOR, id));
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
  m_localPresence     = cmd.m_state;
  m_localPresenceNote = cmd.m_note;

  PTRACE(3, "SIPPres\t'" << m_aor << "' sending own presence " << m_localPresence << "/" << m_localPresenceNote);

  // send presence
  SIPPresenceInfo info;

  info.m_presenceAgent = m_presenceServer.GetAddress().AsString();
  info.m_address       = m_aor.AsString();

  if (m_localPresence == NoPresence) {
    m_endpoint->PublishPresence(info, 0);
    return;
  }

  unsigned expire = m_attributes.Get(OpalPresentity::TimeToLiveKey, "30").AsInteger();

  if ((0 <= m_localPresence) && ((int)m_localPresence <= SIPPresenceInfo::Unchanged))
    info.m_basic = (SIPPresenceInfo::BasicStates)m_localPresence;
  else
    info.m_basic = SIPPresenceInfo::Unknown;

  info.m_note = m_localPresenceNote;

  m_endpoint->PublishPresence(info, expire);
}


unsigned SIPXCAP_Presentity::GetExpiryTime() const
{
  int ttl = m_attributes.Get(OpalPresentity::TimeToLiveKey, "30").AsInteger();
  return ttl > 0 ? ttl : 300;
}


void SIPXCAP_Presentity::Internal_SubscribeToPresence(const OpalSubscribeToPresenceCommand & cmd)
{
  if (cmd.m_subscribe) {
    if (m_presenceInfo.find(cmd.m_presentity) != m_presenceInfo.end()) {
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

    //param.m_onSubcribeStatus = PCREATE_NOTIFIER2(OnPresenceSubscriptionStatus, const SIPSubscribe::SubscriptionStatus &);
    param.m_onNotify         = PCREATE_NOTIFIER2(OnPresenceNotify, SIPSubscribe::NotifyCallbackInfo &);

    m_endpoint->Subscribe(param, m_presenceInfo[cmd.m_presentity].m_subscriptionAOR);
  }
  else {
    PresenceInfoMap::iterator info = m_presenceInfo.find(cmd.m_presentity);
    if (info == m_presenceInfo.end()) {
      PTRACE(3, "SIPPres\t'" << m_aor << "' already unsubscribed to presence of '" << cmd.m_presentity << "'");
      return;
    }

    PTRACE(3, "SIPPres\t'" << m_aor << "' unsubscribing to presence of '" << cmd.m_presentity << "'");
    m_endpoint->Unsubscribe(SIPSubscribe::Presence, info->second.m_subscriptionAOR);
    m_presenceInfo.erase(info);
  }
}


void SIPXCAP_Presentity::OnPresenceNotify(SIPSubscribeHandler &, SIPSubscribe::NotifyCallbackInfo & status)
{
  SIPMIMEInfo & sipMime(status.m_notify.GetMIME());

  if (sipMime.GetEvent() != "event")
    return;

  if (!sipMime.Contains("Subscription-State")) {
    status.m_response.GetEntityBody() = "No Subscription-State field";
    PTRACE(3, "SIPPres\tNOTIFY received for presence subscribe without Subscription-State header");
    return;
  }

  PString subscriptionState = sipMime("Subscription-State");
  PStringArray params = subscriptionState.Tokenise(';', false);
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


void SIPXCAP_Presentity::Internal_AuthorisationRequest(const OpalAuthorisationRequestCommand & cmd)
{
  PString aorStr = m_aor.AsString();

  XCAPClient xcap;
  xcap.SetRoot(m_attributes.Get(SIPXCAP_Presentity::XcapRootKey));
  xcap.SetApplicationUniqueID("pres-rules");            // As per RFC5025/9.1
  xcap.SetContentType("application/auth-policy+xml");   // As per RFC5025/9.4
  xcap.SetUserIdentifier(aorStr);                       // As per RFC5025/9.7
  xcap.SetAuthenticationInfo(m_attributes.Get(SIPXCAP_Presentity::XcapAuthIdKey, m_attributes.Get(OpalPresentity::AuthNameKey, aorStr)),
                             m_attributes.Get(SIPXCAP_Presentity::XcapPasswordKey, m_attributes.Get(OpalPresentity::AuthPasswordKey)));

  PXML xml;
  xcap.GetXmlDocument("index", xml); // See if already have a rule

  if (!xml.IsLoaded())
    xml.Load("<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
             "<cr:ruleset xmlns=\"urn:ietf:params:xml:ns:pres-rules\""
                        " xmlns:pr=\"urn:ietf:params:xml:ns:pres-rules\""
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

  PXMLElement * rule = xml.GetElement("cr:rule");
  rule->SetAttribute("id", "A12345");  // As per http://www.w3.org/TR/1999/REC-xml-names-19990114/#NT-NCName
  rule->GetElement("cr:conditions")->GetElement("cr:identity")->GetElement("cr:one")->SetAttribute("id", cmd.m_presentity);
  static const char * const AuthNames[] = { "allow", "block", "polite-block" };
  rule->GetElement("cr:actions")->GetElement("pr:sub-handling")->SetData(AuthNames[cmd.m_authorisation]);
  rule->GetElement("cr:transformations")->GetElement("pr:provide-services")->GetElement("pr:service-uri-scheme")->SetData(m_aor.GetScheme());

  xcap.PutXmlDocument("index", xml);
}


//////////////////////////////////////////////////////////////

XCAPClient::XCAPClient()
  : m_global(false)
{
}


PURL XCAPClient::BuildURL(const PString & name)
{
  PURL uri(m_root);                              // XCAP root
  uri.AppendPath(m_auid);                        // Application Unique ID
  uri.AppendPath(m_global ? "global" : "users"); // RFC4825/6.2, The path segment after the AUID MUST either be "global" or "users".
  if (!m_global)
    uri.AppendPath(m_xui);                       // XCAP User Identifier
  if (!name.IsEmpty())
    uri.AppendPath(name);                        // Final resource name
  return uri;
}


bool XCAPClient::GetXmlDocument(const PString & name, PXML & xml)
{
  PString str;
  if (!GetTextDocument(BuildURL(name), str, m_contentType))
    return false;

  return xml.Load(str);
}


bool XCAPClient::PutXmlDocument(const PString & name, const PXML & xml)
{
  PStringStream strm;
  strm << xml;
  return PutTextDocument(BuildURL(name), strm, m_contentType);
}


#endif // P_EXPAT
