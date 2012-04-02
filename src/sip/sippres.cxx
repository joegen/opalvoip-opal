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
 * RFC 3863 "Presence Information Data Format (PIDF)"
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

#if P_EXPAT && OPAL_SIP

#include <sip/sippres.h>
#include <ptclib/pdns.h>
#include <ptclib/pxml.h>
#include <ptclib/random.h>


PFACTORY_CREATE(PFactory<OpalPresentity>, SIP_Presentity, "sip", false);
static bool Synonym_for_pres_URL = PFactory<OpalPresentity>::RegisterAs("pres", "sip");

const PCaselessString & SIP_Presentity::PIDFEntityKey()    { static const PConstCaselessString s("PIDF-Entity");    return s; }
const PCaselessString & SIP_Presentity::SubProtocolKey()   { static const PConstCaselessString s("Sub-Protocol");   return s; }
const PCaselessString & SIP_Presentity::PresenceAgentKey() { static const PConstCaselessString s("Presence Agent"); return s; }
const PCaselessString & SIP_Presentity::TransportKey()     { static const PConstCaselessString s("Transport");      return s; }
const PCaselessString & SIP_Presentity::XcapRootKey()      { static const PConstCaselessString s("XCAP Root");      return s; }
const PCaselessString & SIP_Presentity::XcapAuthIdKey()    { static const PConstCaselessString s("XCAP Auth ID");   return s; }
const PCaselessString & SIP_Presentity::XcapPasswordKey()  { static const PConstCaselessString s("XCAP Password");  return s; }
const PCaselessString & SIP_Presentity::XcapAuthAuidKey()  { static const PConstCaselessString s("XCAP AUID");      return s; }
const PCaselessString & SIP_Presentity::XcapAuthFileKey()  { static const PConstCaselessString s("XCAP AuthFile");  return s; }
const PCaselessString & SIP_Presentity::XcapBuddyListKey() { static const PConstCaselessString s("XCAP BuddyList"); return s; }

static struct SIPAttributeInfo {
  const char * m_name;
  const char * m_type;
} const AttributeInfo[] = {
  /*  0 */ { SIP_Presentity::PIDFEntityKey(),    "URL" },
  /*  1 */ { SIP_Presentity::SubProtocolKey(),   "Enum\nPeerToPeer,Agent,XCAP,OMA\nAgent" },
  /*  2 */ { SIP_Presentity::PresenceAgentKey(), "String" },
  /*  3 */ { SIP_Presentity::TransportKey(),     "Enum\nUDP,TCP,TLS\nTCP" },
  /*  4 */ { OpalPresentity::AuthNameKey(),      "String" },
  /*  5 */ { OpalPresentity::AuthPasswordKey(),  "Password" },
  /*  6 */ { SIP_Presentity::XcapRootKey(),      "URL" },
  /*  7 */ { SIP_Presentity::XcapAuthIdKey(),    "String" },
  /*  8 */ { SIP_Presentity::XcapPasswordKey(),  "Password" },
  /*  9 */ { SIP_Presentity::XcapAuthAuidKey(),  "String" },
  /* 10 */ { SIP_Presentity::XcapAuthFileKey(),  "String" },
  /* 11 */ { SIP_Presentity::XcapBuddyListKey(), "String" },
  /* 12 */ { OpalPresentity::TimeToLiveKey(),    "Integer\n1,999999999\n300" }
};

OPAL_DEFINE_COMMAND(OpalSetLocalPresenceCommand,     SIP_Presentity, Internal_SendLocalPresence);
OPAL_DEFINE_COMMAND(OpalSubscribeToPresenceCommand,  SIP_Presentity, Internal_SubscribeToPresence);
OPAL_DEFINE_COMMAND(SIPWatcherInfoCommand,           SIP_Presentity, Internal_SubscribeToWatcherInfo);
OPAL_DEFINE_COMMAND(OpalAuthorisationRequestCommand, SIP_Presentity, Internal_AuthorisationRequest);

static const char * const AuthNames[OpalPresentity::NumAuthorisations] = { "allow", "block", "polite-block", "confirm", "remove" };


//////////////////////////////////////////////////////////////////////////////////////

SIP_Presentity::SIP_Presentity()
  : m_endpoint(NULL)
  , m_subProtocol(e_OMA)
  , m_watcherInfoVersion(-1)
{
  m_attributes.Set(SubProtocolKey, "OMA");
}


SIP_Presentity::SIP_Presentity(const SIP_Presentity & other)
  : OpalPresentityWithCommandThread(other)
  , m_endpoint(other.m_endpoint)
  , m_watcherInfoVersion(-1)
{
}


SIP_Presentity::~SIP_Presentity()
{
  Close();
}


PStringArray SIP_Presentity::GetAttributeNames() const
{
  PStringArray names;
  for (PINDEX i = 0; i < PARRAYSIZE(AttributeInfo); ++i)
    names += AttributeInfo[i].m_name;
  return names;
}


PStringArray SIP_Presentity::GetAttributeTypes() const
{
  PStringArray types;
  for (PINDEX i = 0; i < PARRAYSIZE(AttributeInfo); ++i)
    types += AttributeInfo[i].m_type;
  return types;
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

  PCaselessString subProto = m_attributes.Get(SubProtocolKey);
  if (subProto == "PeerToPeer")
    m_subProtocol = e_PeerToPeer;
  else if (subProto == "Agent")
    m_subProtocol = e_WithAgent;
  else if (subProto == "XCAP")
    m_subProtocol = e_XCAP;
  else if (subProto == "OMA")
    m_subProtocol = e_OMA;
  else {
    PTRACE(1, "SIPPres\tUnknown sub-protocol \"" << subProto << '"');
    return false;
  }

  m_presenceAgent.MakeEmpty(); // Make invalid

  if (m_subProtocol == e_PeerToPeer) {
    PTRACE(3, "SIPPres\tUsing peer to peer mode for " << m_aor);
  }
  else {
    // find presence server for Presentity as per RFC 3861
    // if not found, look for default presence server setting
    // if none, use hostname portion of domain name
    m_presenceAgent = m_attributes.Get(PresenceAgentKey);
    if (m_presenceAgent.IsEmpty()) {
      m_presenceAgent = m_aor.AsString(PURL::HostPortOnly);

#if P_DNS
      if (m_aor.GetScheme() == "pres") {
        PStringList hosts;
        bool found = PDNS::LookupSRV(m_aor.GetHostName(), "_pres._sip", hosts) && !hosts.IsEmpty();
        PTRACE(2, "SIPPres\tSRV lookup for '_pres._sip." << m_aor.GetHostName()
               << "' " << (found ? "succeeded" : "failed"));
        if (found)
          m_presenceAgent = hosts.front();
      }
#endif // P_DNS
    }

    PTRACE(3, "SIPPres\tUsing " << m_presenceAgent << " as presence server for " << m_aor);
  }

  m_watcherSubscriptionAOR.MakeEmpty();
  m_watcherInfoVersion = -1;

  StartThread();

  // subscribe to presence watcher infoformation
  SendCommand(CreateCommand<SIPWatcherInfoCommand>());

  return true;
}


bool SIP_Presentity::IsOpen() const
{
  return m_endpoint != NULL;
}


bool SIP_Presentity::Close()
{
  if (!IsOpen())
    return false;

  PTRACE(3, "SIPPres\t'" << m_aor << "' closing.");
  StopThread();

  if (!m_publishedTupleId.IsEmpty()) {
    SIP_Presentity_OpalSetLocalPresenceCommand cmd;
    cmd.m_state = OpalPresenceInfo::NoPresence;
    Internal_SendLocalPresence(cmd);
  }

  m_notificationMutex.Wait();

  PString watcherSubscriptionAOR = m_watcherSubscriptionAOR;
    m_watcherSubscriptionAOR.MakeEmpty();

  StringMap presenceIdByAor = m_presenceIdByAor;
  m_watcherAorById.clear();
  m_presenceIdByAor.clear();
  m_presenceAorById.clear();
  m_authorisationIdByAor.clear();

  m_notificationMutex.Signal();

  if (!watcherSubscriptionAOR.IsEmpty()) {
    PTRACE(3, "SIPPres\t'" << m_aor << "' sending final unsubscribe for own presence watcher");
    m_endpoint->Unsubscribe(SIPSubscribe::Presence | SIPSubscribe::Watcher, watcherSubscriptionAOR, true);
  }

  for (StringMap::iterator subs = presenceIdByAor.begin(); subs != presenceIdByAor.end(); ++subs) {
    PTRACE(3, "SIPPres\t'" << m_aor << "' sending final unsubscribe to " << subs->first);
    m_endpoint->Unsubscribe(SIPSubscribe::Presence, subs->second, true);
  }

  if (!m_publishedTupleId.IsEmpty() && m_subProtocol != e_PeerToPeer)
    m_endpoint->Publish(m_aor.AsString(), PString::Empty(), 0);

  PTRACE(4, "SIPPres\t'" << m_aor << "' awaiting unsubscriptions to complete.");
  while (m_endpoint->IsSubscribed(SIPSubscribe::Presence | SIPSubscribe::Watcher, watcherSubscriptionAOR))
    PThread::Sleep(100);
  for (StringMap::iterator subs = presenceIdByAor.begin(); subs != presenceIdByAor.end(); ++subs) {
    while (m_endpoint->IsSubscribed(SIPSubscribe::Presence, subs->second))
      PThread::Sleep(100);
  }

  m_endpoint = NULL;
  PTRACE(3, "SIPPres\t'" << m_aor << "' closed.");
  return true;
}


void SIP_Presentity::Internal_SubscribeToPresence(const OpalSubscribeToPresenceCommand & cmd)
{
  if (cmd.m_subscribe) {
    if (m_presenceIdByAor.find(cmd.m_presentity) != m_presenceIdByAor.end()) {
      PTRACE(3, "SIPPres\t'" << m_aor << "' already subscribed to presence of '" << cmd.m_presentity << '\'');
      return;
    }

    PTRACE(3, "SIPPres\t'" << m_aor << "' subscribing to presence of '" << cmd.m_presentity << '\'');

    // subscribe to the presence event on the presence server
    SIPSubscribe::Params param(SIPSubscribe::Presence);

    param.m_localAddress    = m_aor.AsString();
    param.m_addressOfRecord = cmd.m_presentity;
    if (m_subProtocol >= e_XCAP)
      param.m_remoteAddress = m_presenceAgent + ";transport=" + m_attributes.Get(TransportKey, "tcp").ToLower();
    param.m_authID          = m_attributes.Get(OpalPresentity::AuthNameKey, m_aor.GetUserName());
    param.m_password        = m_attributes.Get(OpalPresentity::AuthPasswordKey);
    param.m_expire          = GetExpiryTime();
    param.m_contentType     = "application/pidf+xml";
    param.m_eventList       = true;

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
      PTRACE(3, "SIPPres\t'" << m_aor << "' already unsubscribed to presence of '" << cmd.m_presentity << '\'');
      return;
    }

    PTRACE(3, "SIPPres\t'" << m_aor << "' unsubscribing to presence of '" << cmd.m_presentity << '\'');
    m_endpoint->Unsubscribe(SIPSubscribe::Presence, id->second);
  }
}


void SIP_Presentity::OnPresenceSubscriptionStatus(SIPSubscribeHandler &, const SIPSubscribe::SubscriptionStatus & status)
{
  if (status.m_reason == SIP_PDU::Information_Trying)
    return;

  m_notificationMutex.Wait();
  if (!status.m_wasSubscribing || status.m_reason >= 400) {
    PString id = status.m_handler->GetCallID();
    StringMap::iterator aor = m_presenceAorById.find(id);
    if (aor != m_presenceAorById.end()) {
      PTRACE(status.m_reason >= 400 ? 2 : 3, "SIPPres\t'" << m_aor << "' "
             << (status.m_wasSubscribing ? "error " : "un")
             << "subscribing to presence of '" << aor->second << '\'');
      m_endpoint->Unsubscribe(SIPSubscribe::Presence, status.m_addressofRecord, true);
      m_presenceIdByAor.erase(aor->second);
      m_presenceAorById.erase(aor);
    }
  }
  m_notificationMutex.Signal();
}


void SIP_Presentity::OnPresenceNotify(SIPSubscribeHandler & handler, SIPSubscribe::NotifyCallbackInfo & status)
{
  list<SIPPresenceInfo> infoList;
  PString error;
  PString body = status.m_notify.GetEntityBody();
  if (handler.GetProductInfo().name.Find("Asterisk") != P_MAX_INDEX) {
    PString to = status.m_notify.GetMIME().GetTo().AsString();
    PString from = status.m_notify.GetMIME().GetFrom().AsString();
    PTRACE(4, "SIP\tCompensating for " << handler.GetProductInfo().name << ","
              " replacing " << to << " with " << from);
    body.Replace(to, from);
  }
  if (!SIPPresenceInfo::ParseXML(body, infoList, error)) {
    status.m_response.SetEntityBody(error);
    return;
  }

  // send 200 OK now, and flag caller NOT to send the response
  status.SendResponse(SIP_PDU::Successful_OK);

  m_notificationMutex.Wait();
  for (list<SIPPresenceInfo>::iterator it = infoList.begin(); it != infoList.end(); ++it) {
    SetPIDFEntity(it->m_target);
    PTRACE(3, "SIPPres\t'" << m_aor << "' request for presence of '" << it->m_entity << "' is " << it->m_state);
    OnPresenceChange(*it);
  }
  m_notificationMutex.Signal();
}


unsigned SIP_Presentity::GetExpiryTime() const
{
  int ttl = m_attributes.Get(OpalPresentity::TimeToLiveKey, "300").AsInteger();
  return ttl > 0 ? ttl : 300;
}


void SIP_Presentity::Internal_SubscribeToWatcherInfo(const SIPWatcherInfoCommand & cmd)
{
  if (m_subProtocol == e_PeerToPeer) {
    PTRACE(3, "SIPPres\tRequires agent to do watcher, aor=" << m_aor);
    return;
  }

  if (cmd.m_unsubscribe) {
    if (m_watcherSubscriptionAOR.IsEmpty()) {
      PTRACE(3, "SIPPres\tAlredy unsubscribed presence watcher for " << m_aor);
      return;
    }

    PTRACE(3, "SIPPres\t'" << m_aor << "' sending unsubscribe for own presence watcher");
    m_endpoint->Unsubscribe(SIPSubscribe::Presence | SIPSubscribe::Watcher, m_watcherSubscriptionAOR);
    return;
  }

  PString aorStr = m_aor.AsString();
  PTRACE(3, "SIPPres\t'" << aorStr << "' sending subscribe for own presence.watcherinfo");

  // subscribe to the presence.winfo event on the presence server
  SIPSubscribe::Params param(SIPSubscribe::Presence | SIPSubscribe::Watcher);
  param.m_contentType      = "application/watcherinfo+xml";
  param.m_localAddress     = aorStr;
  param.m_addressOfRecord  = aorStr;
  param.m_remoteAddress    = m_presenceAgent + ";transport=" + m_attributes.Get(TransportKey, "tcp").ToLower();
  param.m_authID           = m_attributes.Get(OpalPresentity::AuthNameKey, m_aor.GetUserName());
  param.m_password         = m_attributes.Get(OpalPresentity::AuthPasswordKey);
  param.m_expire           = GetExpiryTime();
  param.m_onSubcribeStatus = PCREATE_NOTIFIER2(OnWatcherInfoSubscriptionStatus, const SIPSubscribe::SubscriptionStatus &);
  param.m_onNotify         = PCREATE_NOTIFIER2(OnWatcherInfoNotify, SIPSubscribe::NotifyCallbackInfo &);

  m_endpoint->Subscribe(param, m_watcherSubscriptionAOR);
}


void SIP_Presentity::SetPIDFEntity(PURL & entity)
{
  if (entity.Parse(m_attributes.Get(SIP_Presentity::PIDFEntityKey), "pres")) {
    PTRACE(4, "SIPPres\tPIDF entity set via attribute to " << entity);
    return;
  }

  if (m_aor.GetScheme() == "pres") {
    entity = m_aor;
    PTRACE(4, "SIPPres\tPIDF entity set via AOR to " << entity);
  }

  if (entity.Parse(m_aor.GetUserName() + '@' + m_aor.GetHostName(), "pres")) {
    PTRACE(4, "SIPPres\tPIDF entity derived from AOR as " << entity);
    return;
  }

  entity = m_aor;
  PTRACE(4, "SIPPres\tPIDF entity set via failsafe AOR of " << entity);
}


void SIP_Presentity::OnWatcherInfoSubscriptionStatus(SIPSubscribeHandler &, const SIPSubscribe::SubscriptionStatus & status)
{
   if (status.m_reason == SIP_PDU::Information_Trying)
    return;

  OpalPresenceInfo info(status.m_wasSubscribing ? OpalPresenceInfo::Unchanged : OpalPresenceInfo::NoPresence);
  SetPIDFEntity(info.m_entity);
  info.m_target = info.m_entity;

  m_notificationMutex.Wait();

  if (status.m_reason/100 == 4)
    info.m_state = OpalPresenceInfo::Forbidden;
  else if (status.m_reason/100 != 2) 
    info.m_state = OpalPresenceInfo::InternalError;

  OnPresenceChange(info);

  if (!status.m_wasSubscribing) {
    m_endpoint->Unsubscribe(SIPSubscribe::Presence | SIPSubscribe::Watcher, status.m_addressofRecord, true);
    m_watcherSubscriptionAOR.MakeEmpty();
  }

  m_notificationMutex.Signal();
}


void SIP_Presentity::OnWatcherInfoNotify(SIPSubscribeHandler &, SIPSubscribe::NotifyCallbackInfo & status)
{
  // Check for empty body, if so then is OK, just a ping ...
  if (status.m_notify.GetEntityBody().IsEmpty()) {
    PTRACE(4, "SIPPres\tEmpty body on presence watcher NOTIFY, ignoring");
    status.m_response.SetStatusCode(SIP_PDU::Successful_OK);
    return;
  }

  static PXML::ValidationInfo const WatcherValidation[] = {
    { PXML::RequiredNonEmptyAttribute,  "id"  },
    { PXML::RequiredAttributeWithValue, "status",  { "pending\nactive\nwaiting\nterminated" } },
    { PXML::RequiredAttributeWithValue, "event",   { "subscribe\napproved\ndeactivated\nprobation\nrejected\ntimeout\ngiveup" } },
    { PXML::EndOfValidationList }
  };

  static PXML::ValidationInfo const WatcherListValidation[] = {
    { PXML::RequiredNonEmptyAttribute,  "resource" },
    { PXML::RequiredAttributeWithValue, "package", { "presence" } },

    { PXML::Subtree,                    "watcher", { WatcherValidation } , 0 },
    { PXML::EndOfValidationList }
  };

  static PXML::ValidationInfo const WatcherInfoValidation[] = {
    { PXML::SetDefaultNamespace,        "urn:ietf:params:xml:ns:watcherinfo" },
    { PXML::ElementName,                "watcherinfo", },
    { PXML::RequiredNonEmptyAttribute,  "version"},
    { PXML::RequiredAttributeWithValue, "state",   { "full\npartial" } },

    { PXML::Subtree,                    "watcher-list", { WatcherListValidation }, 0 },
    { PXML::EndOfValidationList }
  };

  PXML xml;
  PString error;
  if (!xml.LoadAndValidate(status.m_notify.GetEntityBody(), WatcherInfoValidation, error, PXML::WithNS)) {
    status.m_response.SetEntityBody(error);
    PTRACE(2, "SIPPres\tError parsing XML in presence watcher NOTIFY: " << error);
    return;
  }

  // send 200 OK now, and flag caller NOT to send the response
  status.SendResponse(SIP_PDU::Successful_OK);

  PXMLElement * rootElement = xml.GetRootElement();

  int version = rootElement->GetAttribute("version").AsUnsigned();

  PWaitAndSignal mutex(m_notificationMutex);

  // check version number
  if (m_watcherInfoVersion >= 0 && version <= m_watcherInfoVersion) {
    PTRACE(3, "SIPPres\t'" << m_aor << "' received repeated NOTIFY for own presence.watcherinfo, already processed");
    return;
  }

  // if this is a full list of watcher info, we can empty out our pending lists
  if (rootElement->GetAttribute("state") *= "full") {
    PTRACE(3, "SIPPres\t'" << m_aor << "' received full watcher list for own presence.watcherinfo");
    m_watcherAorById.clear();
  }
  else if (m_watcherInfoVersion < 0) {
    PTRACE(2, "SIPPres\t'" << m_aor << "' received partial watcher list for own presence.watcherinfo, but awaiting full list");
    return;
  }
  else if (version != m_watcherInfoVersion+1) {
    PTRACE(2, "SIPPres\t'" << m_aor << "' received partial watcher list for own presence.watcherinfo, but have missing sequence number, resubscribing");
    m_watcherInfoVersion = -1;
    SendCommand(CreateCommand<SIPWatcherInfoCommand>());
    return;
  }
  else {
    PTRACE(3, "SIPPres\t'" << m_aor << "' received partial watcher list for own presence.watcherinfo");
  }

  m_watcherInfoVersion = version;

  // go through list of watcher information
  PINDEX watcherListIndex = 0;
  PXMLElement * watcherList;
  while ((watcherList = rootElement->GetElement("watcher-list", watcherListIndex++)) != NULL) {
    PINDEX watcherIndex = 0;
    PXMLElement * watcher;
    while ((watcher = watcherList->GetElement("watcher", watcherIndex++)) != NULL)
      OnReceivedWatcherStatus(watcher);
  }
}


void SIP_Presentity::OnReceivedWatcherStatus(PXMLElement * watcher)
{
  PString id     = watcher->GetAttribute("id");
  PString status = watcher->GetAttribute("status");

  AuthorisationRequest authreq;
  authreq.m_presentity = watcher->GetData().Trim();

  StringMap::iterator existingAOR = m_watcherAorById.find(id);

  // save pending subscription status from this user
  if (status == "pending") {
    if (existingAOR != m_watcherAorById.end()) {
      PTRACE(3, "SIPPres\t'" << m_aor << "' received followup to request from '" << authreq.m_presentity << "' for access to presence information");
    } 
    else {
      m_watcherAorById[id] = authreq.m_presentity;
      PTRACE(3, "SIPPres\t'" << authreq.m_presentity << "' has requested access to presence information of '" << m_aor << '\'');
      OnAuthorisationRequest(authreq);
    }
  }
  else {
    PTRACE(3, "SIPPres\t'" << m_aor << "' has received event '" << watcher->GetAttribute("event")
           << "', status '" << status << "', for '" << authreq.m_presentity << '\'');
  }
}


void SIP_Presentity::Internal_SendLocalPresence(const OpalSetLocalPresenceCommand & cmd)
{
  PTRACE(3, "SIPPres\t'" << m_aor << "' sending own presence " << cmd.m_state << "/" << cmd.m_note);

  SIPPresenceInfo sipPresence;
  sipPresence.m_personId = GetID();
  SetPIDFEntity(sipPresence.m_entity);
  sipPresence.m_contact =  m_aor;  // As required by OMA-TS-Presence_SIMPLE-V2_0-20090917-C
  if (m_subProtocol != e_PeerToPeer)
    sipPresence.m_presenceAgent = m_presenceAgent;
  sipPresence.m_state = cmd.m_state;
  sipPresence.m_note = cmd.m_note;

  if (m_publishedTupleId.IsEmpty())
    m_publishedTupleId = sipPresence.m_tupleId;
  else
    sipPresence.m_tupleId = m_publishedTupleId;

  if (m_subProtocol != e_PeerToPeer)
    m_endpoint->PublishPresence(sipPresence, GetExpiryTime());
  else
    m_endpoint->Notify(m_aor, SIPSubscribe::Presence, sipPresence.AsXML());
}


bool SIP_Presentity::ChangeAuthNode(XCAPClient & xcap, const OpalAuthorisationRequestCommand & cmd)
{
  PString ruleId = m_authorisationIdByAor[cmd.m_presentity];

  XCAPClient::NodeSelector node;
  node.SetNamespace("urn:ietf:params:xml:ns:pres-rules", "pr");
  node.SetNamespace("urn:ietf:params:xml:ns:common-policy", "cr");
  node.AddElement("cr:ruleset");
  node.AddElement("cr:rule", "id", ruleId);
  xcap.SetNode(node);

  if (cmd.m_authorisation == AuthorisationRemove) {
    if (xcap.DeleteXml()) {
      PTRACE(3, "SIPPres\tRule id=" << ruleId << " removed for '" << cmd.m_presentity << "' at '" << m_aor << '\'');
    }
    else {
      PTRACE(3, "SIPPres\tCould not remove rule id=" << ruleId
             << " for '" << cmd.m_presentity << "' at '" << m_aor << "\'\n"
             << xcap.GetLastResponseCode() << ' '  << xcap.GetLastResponseInfo());
    }
    return true;
  }

  PXML xml;
  if (!xcap.GetXml(xml)) {
    PTRACE(3, "SIPPres\tCould not locate existing rule id=" << ruleId
           << " for '" << cmd.m_presentity << "' at '" << m_aor << '\'');
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

  if (xcap.PutXml(xml)) {
    PTRACE(3, "SIPPres\tRule id=" << ruleId << " changed to"
           << AuthNames[cmd.m_authorisation] << " for '" << cmd.m_presentity << "' at '" << m_aor << '\'');
    return true;
  }

  PTRACE(3, "SIPPres\tCould not change existing rule id=" << ruleId
         << " for '" << cmd.m_presentity << "' at '" << m_aor << "\'\n"
         << xcap.GetLastResponseCode() << ' '  << xcap.GetLastResponseInfo());
  return false;
}


void SIP_Presentity::Internal_AuthorisationRequest(const OpalAuthorisationRequestCommand & cmd)
{
  if (m_subProtocol < e_XCAP) {
    PTRACE(4, "SIPPres\tRequires XCAP to do authorisation, aor=" << m_aor);
    return;
  }

  XCAPClient xcap;
  InitRootXcap(xcap);
  xcap.SetApplicationUniqueID(m_attributes.Get(XcapAuthAuidKey,
                m_subProtocol == e_OMA ? "org.openmobilealliance.pres-rules" : "pres-rules")); // As per RFC5025/9.1
  xcap.SetContentType("application/auth-policy+xml");   // As per RFC5025/9.4
  xcap.SetUserIdentifier(m_aor.AsString());             // As per RFC5025/9.7
  xcap.SetAuthenticationInfo(m_attributes.Get(XcapAuthIdKey, m_attributes.Get(AuthNameKey, xcap.GetUserIdentifier())),
                             m_attributes.Get(XcapPasswordKey, m_attributes.Get(AuthPasswordKey)));
  xcap.SetFilename(m_attributes.Get(XcapAuthFileKey, m_subProtocol == e_OMA ? "pres-rules" : "index"));

  // See if we can use teh quick method
  if (m_authorisationIdByAor.find(cmd.m_presentity) != m_authorisationIdByAor.end()) {
    if (ChangeAuthNode(xcap, cmd))
      return;
  }

  PXML xml;
  PXMLElement * element;

  // Could not find rule element quickly, get the whole document
  if (!xcap.GetXml(xml)) {
    if (xcap.GetLastResponseCode() != PHTTP::NotFound) {
      PTRACE(2, "SIPPres\tUnexpected error getting rule file for '"
             << cmd.m_presentity << "' at '" << m_aor << "\'\n"
             << xcap.GetLastResponseCode() << ' '  << xcap.GetLastResponseInfo());
      return;
    }

    // No whole document, create a fresh one
    xml.SetOptions(PXML::NoOptions);
    element = xml.SetRootElement("cr:ruleset");
    element->SetAttribute("xmlns:pr", "urn:ietf:params:xml:ns:pres-rules");
    element->SetAttribute("xmlns:cr", "urn:ietf:params:xml:ns:common-policy");

    element = element->AddElement("rule", "id", "presence_allow_own");

    element->AddElement("cr:conditions")->AddElement("cr:identity")->AddElement("cr:one")->SetAttribute("id", m_aor.AsString());
    element->AddElement("cr:actions")->AddElement("pr:sub-handling", AuthNames[AuthorisationPermitted]);
    element = element->AddElement("cr:transformations");
    element->AddElement("pr:provide-services")->AddElement("pr:all-services");
    element->AddElement("pr:provide-persons")->AddElement("pr:all-persons");
    element->AddElement("pr:provide-all-attributes");
    if (m_subProtocol == e_OMA) {
      PXMLElement * root = xml.GetRootElement();
      root->SetAttribute("xmlns:ocp", "urn:oma:xml:xdm:common-policy");

      element = root->AddElement("cr:rule", "id", "wp_prs_block_anonymous");
      element->AddElement("cr:conditions")->AddElement("ocp:anonymous-request");
      element->AddElement("cr:actions")->AddElement("pr:sub-handling", AuthNames[AuthorisationDenied]);

      element = root->AddElement("cr:rule", "id", "wp_prs_unlisted");
      element->AddElement("cr:conditions")->AddElement("ocp:other-identity");
      element->AddElement("cr:actions")->AddElement("pr:sub-handling", AuthNames[AuthorisationConfirming]);

      // Use an xcap object to calculate horrible URL
      XCAPClient xcap;
      InitBuddyXcap(xcap, PString::Empty(), "oma_grantedcontacts");

      element = root->AddElement("cr:rule", "id", "wp_prs_grantedcontacts");
      element->AddElement("cr:conditions")->AddElement("ocp:external-list")->AddElement("ocp:entry", "anc", xcap.BuildURL().AsString());
      element->AddElement("cr:actions")->AddElement("pr:sub-handling", AuthNames[AuthorisationPermitted]);
      element = element->AddElement("cr:transformations");
      element->AddElement("pr:provide-services")->AddElement("pr:all-services");
      element->AddElement("pr:provide-persons")->AddElement("pr:all-persons");
      element->AddElement("pr:provide-all-attributes");

      InitBuddyXcap(xcap, PString::Empty(), "oma_blockedcontacts");

      element = root->AddElement("cr:rule", "id", "wp_prs_blockedcontacts");
      element->AddElement("cr:conditions")->AddElement("ocp:external-list")->AddElement("ocp:entry", "anc", xcap.BuildURL().AsString());
      element->AddElement("cr:actions")->AddElement("pr:sub-handling", AuthNames[AuthorisationDenied]);
    }

    if (!xcap.PutXml(xml)) {
      PTRACE(2, "SIPPres\tCould not add new rule file for '" << m_aor << "\'\n"
             << xcap.GetLastResponseCode() << ' '  << xcap.GetLastResponseInfo());
      return;
    }

    PTRACE(3, "SIPPres\tNew rule file created for '" << m_aor << '\'');
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

  // Create new rule with id as per http://www.w3.org/TR/1999/REC-xml-names-19990114/#NT-NCName
  static PAtomicInteger NextRuleId(PRandom::Number());
  PString newRuleId(PString::Printf, "wp_prs%s_one_%lu",
                    cmd.m_authorisation == AuthorisationPermitted ? "_allow" : "",
                    ++NextRuleId);

  xml.SetOptions(PXML::FragmentOnly);
  element = xml.SetRootElement("cr:rule");
  element->SetAttribute("xmlns:pr", "urn:ietf:params:xml:ns:pres-rules");
  element->SetAttribute("xmlns:cr", "urn:ietf:params:xml:ns:common-policy");
  element->SetAttribute("id", newRuleId);

  element->AddElement("cr:conditions")->AddElement("cr:identity")->AddElement("cr:one")->SetAttribute("id", cmd.m_presentity);
  element->AddElement("cr:actions")->AddElement("pr:sub-handling")->SetData(AuthNames[cmd.m_authorisation]);
  element = element->AddElement("cr:transformations");
  element->AddElement("pr:provide-services")->AddElement("pr:all-services");
  element->AddElement("pr:provide-persons")->AddElement("pr:all-persons");
  element->AddElement("pr:provide-all-attributes");

  XCAPClient::NodeSelector node;
  node.SetNamespace("urn:ietf:params:xml:ns:pres-rules", "pr");
  node.SetNamespace("urn:ietf:params:xml:ns:common-policy", "cr");
  node.AddElement("cr:ruleset");
  node.AddElement("cr:rule", "id", newRuleId);
  xcap.SetNode(node);

  if (xcap.PutXml(xml)) {
    PTRACE(3, "SIPPres\tNew rule set to" << AuthNames[cmd.m_authorisation] << " for '" << cmd.m_presentity << "' at '" << m_aor << '\'');
    m_authorisationIdByAor[cmd.m_presentity] = newRuleId;
    return;
  }

  PTRACE(2, "SIPPres\tCould not add new rule for '" << cmd.m_presentity << "' at '" << m_aor << "\'\n"
         << xcap.GetLastResponseCode() << ' '  << xcap.GetLastResponseInfo());
}


void SIP_Presentity::InitRootXcap(XCAPClient & xcap)
{
  PString root = m_attributes.Get(XcapRootKey);
  if (root.IsEmpty())
    root = "http:" + m_presenceAgent + '/';
  xcap.SetRoot(root);
}


void SIP_Presentity::InitBuddyXcap(XCAPClient & xcap, const PString & entryName, const PString & listName)
{
  InitRootXcap(xcap);
  xcap.SetApplicationUniqueID("resource-lists");            // As per RFC5025/9.1
  xcap.SetContentType("application/resource-lists+xml");    // As per RFC5025/9.4
  xcap.SetUserIdentifier(m_aor.AsString());                 // As per RFC5025/9.7
  xcap.SetAuthenticationInfo(m_attributes.Get(XcapAuthIdKey, m_attributes.Get(OpalPresentity::AuthNameKey, xcap.GetUserIdentifier())),
                             m_attributes.Get(XcapPasswordKey, m_attributes.Get(AuthPasswordKey)));
  xcap.SetFilename("index");

  XCAPClient::NodeSelector node;
  node.SetNamespace("urn:ietf:params:xml:ns:resource-lists");
  node.AddElement("resource-lists");
  node.AddElement("list", "name", listName.IsEmpty() ? m_attributes.Get(XcapBuddyListKey,
                                        m_subProtocol == e_OMA ? "oma_buddylist" : "buddylist") : listName);

  if (!entryName.IsEmpty())
    node.AddElement("entry", "uri", entryName);

  xcap.SetNode(node);
}


static PXMLElement * BuddyInfoToXML(const OpalPresentity::BuddyInfo & buddy, PXMLElement * parent)
{
  PXMLElement * element = new PXMLElement(parent, "entry");
  element->SetAttribute("uri", buddy.m_presentity);

  if (!buddy.m_displayName.IsEmpty())
    element->AddElement("display-name", buddy.m_displayName);

  return element;
}


static bool XMLToBuddyInfo(const PXMLElement * element, OpalPresentity::BuddyInfo & buddy)
{
  if (element == NULL || element->GetName() != "entry")
    return false;

  buddy.m_presentity = element->GetAttribute("uri");

  PXMLElement * itemElement;
  if ((itemElement= element->GetElement("urn:ietf:params:xml:ns:pidf:cipid:display-name")) != NULL)
    buddy.m_displayName = itemElement->GetData();

  if ((itemElement= element->GetElement("urn:ietf:params:xml:ns:pidf:cipid:card")) != NULL) {
    PURL url;
    if (url.Parse(itemElement->GetData())) {
      PString str;
      if (url.LoadResource(str))
        buddy.m_vCard.Parse(str);
    }
  }

  if ((itemElement= element->GetElement("urn:ietf:params:xml:ns:pidf:cipid:icon")) != NULL)
    buddy.m_icon = itemElement->GetData();

  if ((itemElement= element->GetElement("urn:ietf:params:xml:ns:pidf:cipid:map")) != NULL)
    buddy.m_map = itemElement->GetData();

  if ((itemElement= element->GetElement("urn:ietf:params:xml:ns:pidf:cipid:sound")) != NULL)
    buddy.m_sound = itemElement->GetData();

  if ((itemElement= element->GetElement("urn:ietf:params:xml:ns:pidf:cipid:homepage")) != NULL)
    buddy.m_homePage = itemElement->GetData();

  buddy.m_contentType = "application/resource-lists+xml";
  buddy.m_rawXML = element->AsString();
  return true;
}


static bool RecursiveGetBuddyList(OpalPresentity::BuddyList & buddies, XCAPClient & xcap, const PURL & url)
{
  if (url.IsEmpty())
    return false;

  PXML xml;
  if (!xcap.GetXml(url, xml))
    return false;

  PXMLElement * element;
  PINDEX idx = 0;
  while ((element = xml.GetElement("entry", idx++)) != NULL) {
    OpalPresentity::BuddyInfo buddy;
    if (XMLToBuddyInfo(element, buddy))
      buddies.push_back(buddy);
  }

  idx = 0;
  while ((element = xml.GetElement("external", idx++)) != NULL)
    RecursiveGetBuddyList(buddies, xcap, element->GetAttribute("anchor"));

  idx = 0;
  while ((element = xml.GetElement("entry-ref", idx++)) != NULL) {
    PURL url(xcap.GetRoot());
    url.SetPathStr(url.GetPathStr() + element->GetAttribute("ref"));
    RecursiveGetBuddyList(buddies, xcap, url);
  }

  return true;
}


OpalPresentity::BuddyStatus SIP_Presentity::GetBuddyListEx(BuddyList & buddies)
{
  if (m_subProtocol < e_XCAP) {
    PTRACE(4, "SIPPres\tRequires XCAP to have buddies, aor=" << m_aor);
    return BuddyStatus_ListFeatureNotImplemented;
  }

  XCAPClient xcap;
  InitBuddyXcap(xcap);
  if (RecursiveGetBuddyList(buddies, xcap, xcap.BuildURL()) ||
      !buddies.empty() ||
      xcap.GetLastResponseCode() == PHTTP::NotFound)
    return BuddyStatus_OK;

  return BuddyStatus_GenericFailure;
}


OpalPresentity::BuddyStatus SIP_Presentity::SetBuddyListEx(const BuddyList & buddies)
{
  if (m_subProtocol < e_XCAP) {
    PTRACE(4, "SIPPres\tRequires XCAP to have buddies, aor=" << m_aor);
    return BuddyStatus_ListFeatureNotImplemented;
  }

  PXML xml(PXML::FragmentOnly);

  PString buddyListKey = m_subProtocol == e_OMA ? "oma_buddylist" : "buddylist";
  PXMLElement * root = xml.SetRootElement("list");
  root->SetAttribute("xmlns", "urn:ietf:params:xml:ns:resource-lists");
  root->SetAttribute("name", m_attributes.Get(XcapBuddyListKey, buddyListKey));

  for (BuddyList::const_iterator it = buddies.begin(); it != buddies.end(); ++it)
    root->AddChild(BuddyInfoToXML(*it, root));

  XCAPClient xcap;
  InitBuddyXcap(xcap);

  if (xcap.PutXml(xml))
    return BuddyStatus_OK;

  if (xcap.GetLastResponseCode() == PHTTP::Conflict && xcap.GetLastResponseInfo().Find("Parent") != P_MAX_INDEX) {
    // Got "Parent does not exist" error, so need to add whole file
    xml.SetOptions(PXML::NoOptions);
    root = xml.SetRootElement("resource-lists");
    root->SetAttribute("xmlns", "urn:ietf:params:xml:ns:resource-lists");

    PXMLElement * listElement = root->AddElement("list", "name", m_attributes.Get(XcapBuddyListKey, buddyListKey));

    for (BuddyList::const_iterator it = buddies.begin(); it != buddies.end(); ++it)
      listElement->AddChild(BuddyInfoToXML(*it, listElement));

    xcap.ClearNode();
    if (xcap.PutXml(xml))
      return BuddyStatus_OK;
  }

  PTRACE(2, "SIPPres\tError setting buddy list of '" << m_aor << "\'\n"
         << xcap.GetLastResponseCode() << ' '  << xcap.GetLastResponseInfo());

  return BuddyStatus_GenericFailure;
}


OpalPresentity::BuddyStatus SIP_Presentity::DeleteBuddyListEx()
{
  if (m_subProtocol < e_XCAP) {
    PTRACE(4, "SIPPres\tRequires XCAP to have buddies, aor=" << m_aor);
    return BuddyStatus_ListFeatureNotImplemented;
  }

  XCAPClient xcap;
  InitBuddyXcap(xcap);

  if (xcap.DeleteXml())
    return BuddyStatus_OK;

  PTRACE(2, "SIPPres\tError deleting buddy list of '" << m_aor << "\'\n"
         << xcap.GetLastResponseCode() << ' '  << xcap.GetLastResponseInfo());
  return BuddyStatus_GenericFailure;
}


OpalPresentity::BuddyStatus SIP_Presentity::GetBuddyEx(BuddyInfo & buddy)
{
  if (m_subProtocol < e_XCAP) {
    PTRACE(4, "SIPPres\tRequires XCAP to have buddies, aor=" << m_aor);
    return BuddyStatus_ListFeatureNotImplemented;
  }

  XCAPClient xcap;
  InitBuddyXcap(xcap, buddy.m_presentity);

  PXML xml;
  if (xcap.GetXml(xml) && XMLToBuddyInfo(xml.GetRootElement(), buddy))
    return BuddyStatus_OK;
  else
    return BuddyStatus_GenericFailure;
}


OpalPresentity::BuddyStatus SIP_Presentity::SetBuddyEx(const BuddyInfo & buddy)
{
  if (m_subProtocol < e_XCAP) {
    PTRACE(4, "SIPPres\tRequires XCAP to have buddies, aor=" << m_aor);
    return BuddyStatus_ListFeatureNotImplemented;
  }

  if (buddy.m_presentity.IsEmpty())
    return BuddyStatus_GenericFailure;

  XCAPClient xcap;
  InitBuddyXcap(xcap, buddy.m_presentity);

  PXML xml(PXML::FragmentOnly);
  xml.SetRootElement(BuddyInfoToXML(buddy, NULL));

  if (xcap.PutXml(xml))
    return BuddyStatus_OK;

  if (xcap.GetLastResponseCode() != PHTTP::Conflict || xcap.GetLastResponseInfo().Find("Parent") == P_MAX_INDEX) {
    PTRACE(2, "SIPPres\tError setting buddy '" << buddy.m_presentity << "' of '" << m_aor << "\'\n"
           << xcap.GetLastResponseCode() << ' '  << xcap.GetLastResponseInfo());
    return BuddyStatus_GenericFailure;
  }

  // Got "Parent does not exist" error, so need to add whole list
  BuddyList buddies;
  buddies.push_back(buddy);
  return SetBuddyListEx(buddies);
}


OpalPresentity::BuddyStatus SIP_Presentity::DeleteBuddyEx(const PURL & presentity)
{
  if (m_subProtocol < e_XCAP) {
    PTRACE(4, "SIPPres\tRequires XCAP to have buddies, aor=" << m_aor);
    return BuddyStatus_ListFeatureNotImplemented;
  }

  XCAPClient xcap;
  InitBuddyXcap(xcap, presentity);

  if (xcap.DeleteXml())
    return BuddyStatus_OK;

  PTRACE(2, "SIPPres\tError deleting buddy '" << presentity << "' of '" << m_aor << "\'\n"
         << xcap.GetLastResponseCode() << ' '  << xcap.GetLastResponseInfo());
  return BuddyStatus_GenericFailure;
}


OpalPresentity::BuddyStatus SIP_Presentity::SubscribeBuddyListEx(PINDEX & numSuccessful, bool subscribe)
{
  if (m_subProtocol < e_XCAP) {
    PTRACE(4, "SIPPres\tRequires XCAP to have buddies, aor=" << m_aor);
    return BuddyStatus_ListFeatureNotImplemented;
  }

  PXML xml;
  XCAPClient xcap;
  InitRootXcap(xcap);
  xcap.SetApplicationUniqueID("rls-services");
  xcap.SetContentType("application/rls-services+xml");
  xcap.SetUserIdentifier(m_aor.AsString());
  xcap.SetAuthenticationInfo(m_attributes.Get(XcapAuthIdKey, m_attributes.Get(AuthNameKey, xcap.GetUserIdentifier())),
                             m_attributes.Get(XcapPasswordKey, m_attributes.Get(AuthPasswordKey)));
  xcap.SetFilename("index");

  PString serviceURI = xcap.GetUserIdentifier() + ";pres-list=oma_buddylist";

  if (!xcap.GetXml(xml)) {
    if (xcap.GetLastResponseCode() != PHTTP::NotFound) {
      PTRACE(2, "SIPPres\tUnexpected error getting rls-services file for at '" << m_aor << "\'\n"
             << xcap.GetLastResponseCode() << ' '  << xcap.GetLastResponseInfo());
      return OpalPresentity::SubscribeBuddyListEx(numSuccessful, subscribe); // Do individual subscribes
    }

    // No file at all, add the root element.
    xml.SetRootElement("rls-services")->SetAttribute("xmlns", "urn:ietf:params:xml:ns:rls-services");
  }
  else {
    // Have file, see if have specific service
    PXMLElement * element = xml.GetElement("service", "uri", serviceURI);
    if (element != NULL) {
      PTRACE(4, "SIPPres\tConfirmed rls-services entry for '" << serviceURI << "\' is\n" << xml);
      numSuccessful = P_MAX_INDEX;
      return SubscribeToPresence(serviceURI, subscribe) ? BuddyStatus_OK : BuddyStatus_GenericFailure;
    }

    // Nope, so add it
  }

  // Added service to existing XML or newly rooted one
  PXMLElement * element = xml.GetRootElement()->AddElement("service");
  element->SetAttribute("uri", serviceURI);

  // Calculate the horrible URL using a temporary XCAP client
  XCAPClient xcapTemp;
  InitBuddyXcap(xcapTemp);
  element->AddElement("resource-list")->SetData(xcapTemp.BuildURL().AsString());

  element->AddElement("packages")->AddElement("package")->SetData("presence");

  if (xcap.PutXml(xml)) {
    numSuccessful = P_MAX_INDEX;
    return SubscribeToPresence(serviceURI, subscribe) ? BuddyStatus_OK : BuddyStatus_GenericFailure;
  }

  PTRACE(2, "SIPPres\tCould not add new rls-services entry for '" << m_aor << "\'\n"
         << xcap.GetLastResponseCode() << ' '  << xcap.GetLastResponseInfo());

  return OpalPresentity::SubscribeBuddyListEx(numSuccessful, subscribe); // Do individual subscribes
}


//////////////////////////////////////////////////////////////

XCAPClient::XCAPClient()
  : m_global(false)
  , m_filename("index")
{
}


PURL XCAPClient::BuildURL()
{
  PURL uri(m_root);                              // XCAP root

  uri.AppendPath(m_auid);                        // Application Unique ID

  uri.AppendPath(m_global ? "global" : "users"); // RFC4825/6.2, The path segment after the AUID MUST either be "global" or "users".

  if (!m_global)
    uri.AppendPath(m_xui);                       // XCAP User Identifier

  if (!m_filename.IsEmpty()) {
    uri.AppendPath(m_filename);                        // Final resource name
    m_node.AddToURL(uri);
  }

  return uri;
}


static bool HasNode(const PURL & url)
{
  const PStringArray & path = url.GetPath();
  for (PINDEX i = 0; i < path.GetSize(); ++i) {
    if (path[i] == "~~")
      return true;
  }

  return false;
}


bool XCAPClient::GetXml(const PURL & url, PXML & xml)
{
  bool hasNode = HasNode(url);

  PString body;
  if (!GetTextDocument(url, body, hasNode ? "application/xcap-el+xml" : m_contentType)) {
    PTRACE(3, "SIPPres\tError getting buddy list at '" << url << "\'\n"
           << GetLastResponseCode() << ' '  << GetLastResponseInfo());
    return false;
  }

  if (xml.Load(body, hasNode ? PXML::FragmentOnly : PXML::NoOptions))
    return true;

  PTRACE(2, "XCAP\tError parsing XML for '" << url << "\'\n"
            "Line " << xml.GetErrorLine() << ", Column " << xml.GetErrorColumn() << ": " << xml.GetErrorString());
  return false;
}


bool XCAPClient::PutXml(const PURL & url, const PXML & xml)
{
  PStringStream strm;
  strm << xml;
  return PutTextDocument(url, strm, HasNode(url) ? "application/xcap-el+xml" : m_contentType);
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


//////////////////////////////////////////////////////////////



#endif // P_EXPAT && OPAL_SIP
