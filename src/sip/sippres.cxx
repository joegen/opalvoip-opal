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
    SIPURL sipAOR(GetAOR());
#if P_DNS
    PIPSocketAddressAndPortVector addrs;
    if (PDNS::LookupSRV(sipAOR.GetHostName(), "_pres._sip", sipAOR.GetPort(), addrs) && addrs.size() > 0) {
      PTRACE(1, "SIPPres\tSRV lookup for '" << sipAOR.GetHostName() << "_pres._sip' succeeded");
      m_presenceServer = addrs[0];
    }
    else
#endif
      if (m_attributes.Has(SIP_Presentity::DefaultPresenceServerKey)) 
        m_presenceServer.Parse(m_attributes.Get(SIP_Presentity::DefaultPresenceServerKey), m_endpoint->GetDefaultSignalPort());
      else
        m_presenceServer.Parse(sipAOR.GetHostName(), sipAOR.GetPort());

    // set presence server
    m_attributes.Set(SIP_Presentity::PresenceServerKey, m_presenceServer.AsString());
  }

  if (!m_presenceServer.GetAddress().IsValid()) {
    PTRACE(1, "SIPPres\tUnable to lookup hostname for '" << m_presenceServer.GetAddress() << "'");
    return false;
  }

  // initialised variables
  m_watcherInfoSubscribed = false;
  m_watcherInfoVersion = 0;

  StartThread();

  // subscribe to presence watcher infoformation
  SendCommand(CreateCommand<SIPWatcherInfoCommand>());


  return true;
}

bool SIPXCAP_Presentity::Close()
{
  if (m_endpoint == NULL)
    return false;

  Internal_SubscribeToWatcherInfo(false);
  StopThread();
  m_endpoint = NULL;

  return true;
}


OPAL_DEFINE_COMMAND(OpalSetPresenceCommand,         SIPXCAP_Presentity, Internal_SendLocalPresence);
OPAL_DEFINE_COMMAND(OpalSubscribeToPresenceCommand, SIPXCAP_Presentity, Internal_SubscribeToPresence);
OPAL_DEFINE_COMMAND(OpalAuthorisePresenceCommand,   SIPXCAP_Presentity, Internal_AuthorisePresence);
OPAL_DEFINE_COMMAND(SIPWatcherInfoCommand,          SIPXCAP_Presentity, Internal_SubscribeToWatcherInfo);


void SIPXCAP_Presentity::Internal_SubscribeToWatcherInfo(const SIPWatcherInfoCommand & cmd)
{
  // don't bother checking if already subscribed - this allows resubscribe
  //if (m_watcherInfoSubscribed == start)
  //  return true;

  // subscribe to the presence.winfo event on the presence server
  SIPSubscribe::Params param(SIPSubscribe::Presence | SIPSubscribe::Watcher);

  PString aor(GetAOR());

  PTRACE(3, "SIPPres\t'" << aor << "' sending subscribe for own presence.watcherinfo");

  param.m_localAddress    = aor;
  param.m_addressOfRecord = aor;
  param.m_remoteAddress   = PString("sip:") + m_presenceServer.AsString();
  param.m_authID          = m_attributes.Get(OpalPresentity::AuthNameKey, aor);
  param.m_password        = m_attributes.Get(OpalPresentity::AuthPasswordKey);
  param.m_expire          = GetExpiryTime(cmd.m_subscribe);
  param.m_onSubcribeStatus = PCREATE_NOTIFIER2(OnWatcherInfoSubscriptionStatus, const SIPSubscribe::SubscriptionStatus &);
  param.m_onNotify         = PCREATE_NOTIFIER2(OnWatcherInfoNotify, SIPSubscribe::NotifyCallbackInfo &);

  m_watcherInfoSubscribed = false;

  m_endpoint->Subscribe(param, aor);
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

void SIPXCAP_Presentity::OnWatcherInfoSubscriptionStatus(SIPSubscribeHandler &, const SIPSubscribe::SubscriptionStatus & /*status*/)
{
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

  PTRACE(3, "SIPPres\t'" << GetAOR() << "' received NOTIFY for own presence.watcherinfo");

  PXMLElement * rootElement = xml.GetRootElement();

  unsigned version = rootElement->GetAttribute("version").AsUnsigned();

  PWaitAndSignal m(m_onRequestPresenceNotifierMutex);

  // check version number
  bool sendRefresh = false;
  if (m_watcherInfoSubscribed) 
    sendRefresh = version != m_watcherInfoVersion+1;
  else {
    m_watcherInfoVersion = version;
    m_watcherInfoSubscribed = true;
  }
  version = m_watcherInfoVersion;

  // if this is a full list of watcher info, we can empty out our pending lists
  if (rootElement->GetAttribute("state") *= "full") {
    PTRACE(3, "SIPPres\t'" << GetAOR() << "' received full watcher list");
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
    PTRACE(3, "SIPPres\t'" << GetAOR() << "' received NOTIFY for own presence.watcherinfo without out of sequence version");
    // TODO
  }
}

void SIPXCAP_Presentity::OnReceivedWatcherStatus(PXMLElement * watcher)
{
  PString id       = watcher->GetAttribute("id");
  PString status   = watcher->GetAttribute("status");
  PString eventStr = watcher->GetAttribute("event");
  PString aor      = watcher->GetData().Trim();
  IdToAorMap::iterator r = m_idToAorMap.find(id);

  // save pending subscription status from this user
  if (status == "pending") {
    if (r != m_idToAorMap.end()) {
      PTRACE(3, "SIPPres\t'" << GetAOR() << "' received followup to request from '" << aor << "' for access to presence information");
    } 
    else {
      m_idToAorMap.insert(IdToAorMap::value_type(id, aor));
      m_aorToIdMap.insert(AorToIdMap::value_type(aor, id));
      PTRACE(3, "SIPPres\t'" << aor << "' has requested access to presence information of '" << GetAOR() << "'");
      OnRequestPresence(aor);
    }
  }
  else {
    PTRACE(3, "SIPPres\t'" << GetAOR() << "' has received presence status '" << status << "' for '" << aor << "'");
  }
}


void SIPXCAP_Presentity::Internal_SendLocalPresence(const OpalSetPresenceCommand & cmd)
{
  m_localPresence     = cmd.m_state;
  m_localPresenceNote = cmd.m_note;

  PTRACE(3, "SIPPres\t'" << GetAOR() << "' sending own presence " << m_localPresence << "/" << m_localPresenceNote);

  // send presence
  SIPPresenceInfo info;

  info.m_presenceAgent = m_presenceServer.GetAddress().AsString();
  info.m_address       = GetAOR();

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


unsigned SIPXCAP_Presentity::GetExpiryTime(bool subscribing) const
{
  if (!subscribing)
    return 0;

  int ttl = m_attributes.Get(OpalPresentity::TimeToLiveKey, "30").AsInteger();
  return ttl > 0 ? ttl : 300;
}


void SIPXCAP_Presentity::Internal_SubscribeToPresence(const OpalSubscribeToPresenceCommand & cmd)
{
  {
    PresenceInfoMap::iterator r = m_presenceInfo.find(cmd.m_presentity);
    if (cmd.m_subscribe && (r != m_presenceInfo.end())) {
      PTRACE(3, "SIPPres\t'" << GetAOR() << "' already subscribed to presence of '" << cmd.m_presentity << "'");
      return;
    }
    else if (!cmd.m_subscribe && (r == m_presenceInfo.end())) {
      PTRACE(3, "SIPPres\t'" << GetAOR() << "' already unsubscribed to presence of '" << cmd.m_presentity << "'");
      return;
    }
  }

  PTRACE(3, "SIPPres\t'" << GetAOR() << "' " << (cmd.m_subscribe ? "" : "un") << "subscribing to presence of '" << cmd.m_presentity << "'");

  // subscribe to the presence event on the presence server
  SIPSubscribe::Params param(SIPSubscribe::Presence);

  SIPURL aor(GetAOR());
  PString aorStr(aor.AsString());

  param.m_localAddress    = aorStr;
  param.m_addressOfRecord = cmd.m_presentity;
  param.m_remoteAddress   = PString("sip:") + m_presenceServer.AsString();
  param.m_authID            = m_attributes.Get(OpalPresentity::AuthNameKey, aor.GetUserName() + '@' + aor.GetHostAddress());
  param.m_password          = m_attributes.Get(OpalPresentity::AuthPasswordKey);
  param.m_expire            = GetExpiryTime(cmd.m_subscribe);

  //param.m_onSubcribeStatus = PCREATE_NOTIFIER2(OnPresenceSubscriptionStatus, const SIPSubscribe::SubscriptionStatus &);
  param.m_onNotify         = PCREATE_NOTIFIER2(OnPresenceNotify, SIPSubscribe::NotifyCallbackInfo &);

  if (!cmd.m_subscribe)
    m_presenceInfo.erase(cmd.m_presentity);
  else {
    PresenceInfo info;
    info.m_subscriptionState = PresenceInfo::e_Subscribing;
    m_presenceInfo.insert(PresenceInfoMap::value_type(cmd.m_presentity, info));
  }

  PString actualAor;
  m_endpoint->Subscribe(param, actualAor);
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

  PTRACE(3, "SIPPres\t'" << fromStr << "' request for presence of '" << GetAOR() << "' is " << params[0]);

  // return OK;
  status.m_response.SetStatusCode(SIP_PDU::Successful_OK);
  status.m_response.GetInfo() = "OK";
}


void SIPXCAP_Presentity::Internal_AuthorisePresence(const OpalAuthorisePresenceCommand & cmd)
{
  // send command to XCAP server
  PStringStream xml;
  xml << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
         "<cr:ruleset xmlns=\"urn:ietf:params:xml:ns:pres-rules\""
                     "xmlns:pr=\"urn:ietf:params:xml:ns:pres-rules\""
                     "xmlns:cr=\"urn:ietf:params:xml:ns:common-policy\">"
           "<cr:rule id=\"" << m_guid << "\">"
             "<cr:conditions>"
               "<cr:identity>"
                 "<cr:one id=\"" << cmd.m_presentity << "\"/>"
               "</cr:identity>"
             "</cr:conditions>"
             "<cr:actions>"
               "<pr:sub-handling>allow</pr:sub-handling>"
             "</cr:actions>"
             "<cr:transformations>"
               "<pr:provide-services>"
                 "<pr:service-uri-scheme>sip</pr:service-uri-scheme>"
               "</pr:provide-services>"
               "<pr:provide-persons>"
                 "<pr:all-persons/>"
               "</pr:provide-persons>"
               "<pr:provide-activities>true</pr:provide-activities>"
               "<pr:provide-user-input>bare</pr:provide-user-input>"
             "</cr:transformations>"
           "</cr:rule>"
         "</cr:ruleset>";
}

#endif // P_EXPAT
