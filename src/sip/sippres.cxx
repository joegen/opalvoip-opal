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


#include <ptlib.h>
#include <opal/buildopts.h>

#include <sip/sippres.h>
#include <ptclib/pdns.h>
#include <ptclib/pxml.h>

const char * SIP_Presentity::DefaultPresenceServerKey = "default_presence_server";
const char * SIP_Presentity::PresenceServerKey        = "presence_server";

static PFactory<OpalPresentity>::Worker<SIPLocal_Presentity> sip_local_PresentityWorker("sip-local");
static PFactory<OpalPresentity>::Worker<SIPXCAP_Presentity>  sip_xcap_PresentityWorker1("sip-xcap");
static PFactory<OpalPresentity>::Worker<SIPXCAP_Presentity>  sip_xcap_PresentityWorker2("sip");

//////////////////////////////////////////////////////////////////////////////////////

SIP_Presentity::SIP_Presentity()
  : m_manager(NULL)
  , m_endpoint(NULL)
  , m_thread(NULL)
  , m_localPresence(NoPresence)
{ 
}

SIP_Presentity::~SIP_Presentity()
{
}

bool SIP_Presentity::Open(OpalManager * manager)
{
  // if we have an endpoint, then close the entity
  if (m_endpoint != NULL)
    Close();

  // save the manager
  m_manager = manager;

  // call the ancestor
  if (!OpalPresentity::Open())
    return false;

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

bool SIP_Presentity::Close()
{
  InternalClose();

  m_endpoint = NULL;

  return true;
}

OpalPresentity::CmdSeqType SIP_Presentity::SendCommand(Command * cmd)
{
  CmdSeqType seq;
  {
    PWaitAndSignal m(m_commandQueueMutex);
    seq = cmd->m_sequence = ++m_commandSequence;
    m_commandQueue.push(cmd);
  }

  m_commandQueueSync.Signal();

  return seq;
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

bool SIPLocal_Presentity::InternalClose()
{
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
  SIPURL sipAOR(GetSIPAOR());
  PIPSocketAddressAndPortVector addrs;
  if (PDNS::LookupSRV(sipAOR.GetHostName(), "_pres._sip", sipAOR.GetPort(), addrs) && addrs.size() > 0) {
    PTRACE(1, "SIPPres\tSRV lookup for '" << sipAOR.GetHostName() << "_pres._sip' succeeded");
    m_presenceServer = addrs[0];
  }
  else if (HasAttribute(SIP_Presentity::DefaultPresenceServerKey)) {
    m_presenceServer = GetAttribute(SIP_Presentity::DefaultPresenceServerKey);
  } 
  else
    m_presenceServer = PIPSocketAddressAndPort(sipAOR.GetHostName(), sipAOR.GetPort());

  if (!m_presenceServer.GetAddress().IsValid()) {
    PTRACE(1, "SIPPres\tUnable to lookup hostname for '" << m_presenceServer.GetAddress() << "'");
    return false;
  }

  SetAttribute(SIP_Presentity::PresenceServerKey, m_presenceServer.AsString());

  // start handler thread
  m_threadRunning = true;
  m_thread = new PThreadObj<SIPXCAP_Presentity>(*this, &SIPXCAP_Presentity::Thread);

  // subscribe to presence watcher infoformation
  m_watcherInfoSubscribed = false;
  Command * cmd = new Command(e_StartWatcherInfo);
  SendCommand(cmd);

  return true;
}

bool SIPXCAP_Presentity::InternalClose()
{
  SubscribeToWatcherInfo(false);

  m_threadRunning = false;
  m_thread->WaitForTermination();
  delete m_thread;
  m_thread = NULL;

  return true;
}

void SIPXCAP_Presentity::Thread()
{
  while (m_threadRunning) {
    m_commandQueueSync.Wait(1000);
    if (!m_threadRunning)
      break;

    Command * cmd = NULL;
    {
      PWaitAndSignal m(m_commandQueueMutex);
      if (m_commandQueue.size() != 0) {
        cmd = m_commandQueue.front();
        m_commandQueue.pop();
      }
    }
    if (cmd != NULL) {
      switch (cmd->m_cmd) {
        case e_SetPresenceState:
          {
            SetPresenceCommand * dcmd = dynamic_cast<SetPresenceCommand *>(cmd);
            if (dcmd != NULL) {
              m_localPresence     = dcmd->m_state;
              m_localPresenceNote = dcmd->m_note;
              Internal_SendLocalPresence();
            }
          }
          break;

        case e_StartWatcherInfo:
          SubscribeToWatcherInfo(true);
          break;

        case e_StopWatcherInfo:
          SubscribeToWatcherInfo(false);
          break;

        case e_SubscribeToPresence:
        case e_UnsubscribeFromPresence:
          {
            SimpleCommand * dcmd = dynamic_cast<SimpleCommand *>(cmd);
            if (dcmd != NULL) 
              Internal_SubscribeToPresence(dcmd->m_presEntity, cmd->m_cmd == e_SubscribeToPresence);
          }
          break;

          break;

        default:
          break;
      }
      delete cmd;
    }
  }
}


void SIPXCAP_Presentity::SubscribeToWatcherInfo(bool start)
{
  // don't bother checking if already subscribed - this allows resubscribe
  //if (m_watcherInfoSubscribed == start)
  //  return true;

  // subscribe to the presence.winfo event on the presence server
  SIPSubscribe::Params param(SIPSubscribe::Presence | SIPSubscribe::Watcher);

  SIPURL aor(GetSIPAOR());
  PString aorStr(aor.AsString());

  PTRACE(3, "SIPPres\t'" << aor << "' sending subscribe for own presence.watcherinfo");

  param.m_localAddress    = aorStr;
  param.m_addressOfRecord = aorStr;
  param.m_remoteAddress   = PString("sip:") + m_presenceServer.AsString();

  param.m_authID            = GetAttribute(OpalPresentity::AuthNameKey, aor.GetUserName() + '@' + aor.GetHostAddress());
  param.m_password          = GetAttribute(OpalPresentity::AuthPasswordKey);
  unsigned ttl = GetAttribute(OpalPresentity::TimeToLiveKey, "30").AsInteger();
  if (ttl == 0)
    ttl = 300;
  param.m_expire            = start ? ttl : 0;

  param.m_onSubcribeStatus = PCREATE_NOTIFIER(OnWatcherInfoSubscriptionStatus);
  param.m_onNotify         = PCREATE_NOTIFIER(OnWatcherInfoNotify);

  m_endpoint->Subscribe(param, aorStr);
}

static PXMLValidator::ElementInfo watcherValidation[] = {
  { PXMLValidator::RequiredAttribute,          "id"  },
  { PXMLValidator::RequiredAttributeWithValue, "status",  "pending\nactive\nwaiting\nterminated" },
  { PXMLValidator::RequiredAttributeWithValue, "event",   "subscribe\napproved\ndeactivated\nprobation\nrejected\ntimeout\ngiveup" },
  { 0 }
};

static PXMLValidator::ElementInfo watcherListValidation[] = {
  { PXMLValidator::RequiredAttribute,          "resource" },
  { PXMLValidator::RequiredAttributeWithValue, "package", "presence" },

  { PXMLValidator::Subtree,                    "watcher",  watcherValidation, "0" },
  { 0 }
};

static PXMLValidator::ElementInfo watcherInfoValidation[] = {
  { PXMLValidator::ElementName,                "watcherinfo", },
  { PXMLValidator::RequiredAttributeWithValue, "xmlns",   "urn:ietf:params:xml:ns:watcherinfo" },
  { PXMLValidator::RequiredAttributeWithValue, "version", "0\n1" },
  { PXMLValidator::RequiredAttributeWithValue, "state",   "full\npartial" },

  { PXMLValidator::Subtree,                    "watcher-list", watcherListValidation, "0" },
  { 0 }
};

void SIPXCAP_Presentity::OnWatcherInfoSubscriptionStatus(SIPSubscribeHandler &, INT /*s*/)
{
  //SIPSubscribe::Status & status = *(SIPSubscribe::Status *)s;
}

void SIPXCAP_Presentity::OnWatcherInfoNotify(SIPSubscribeHandler & handler, INT s)
{
  SIPSubscribe::NotifyCallbackInfo & status = *(SIPSubscribe::NotifyCallbackInfo *)s;

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

  // if we got this far, we are subscribed to our own watcherinfo
  m_watcherInfoSubscribed = true;

  PTRACE(3, "SIPPres\t'" << GetSIPAOR() << "' received NOTIFY for own presence.watcherinfo");

  // send 200 OK now, and flag caller NOT to send 20o OK
  status.m_sendResponse = false;
  status.m_response.SetStatusCode(SIP_PDU::Successful_OK);
  status.m_response.GetInfo() = "OK";
  status.m_response.SendResponse(*handler.GetTransport(), status.m_response, &handler.GetEndPoint());

  // TODO go through list of watcher information
  PXMLElement * rootElement = xml.GetRootElement();
  PINDEX watcherListIndex = 0;
  PXMLElement * watcherList = rootElement->GetElement("watcher-list", watcherListIndex++);
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
}

void SIPXCAP_Presentity::OnReceivedWatcherStatus(PXMLElement * watcher)
{
  PString status   = watcher->GetAttribute("status");
  PString id       = watcher->GetAttribute("id");
  PString eventStr = watcher->GetAttribute("event");
  PString name     = watcher->GetData();
  PTRACE(3, "SIPPres\t'" << GetSIPAOR() << "' received is being watched by '" << name << "'");
}


void SIPXCAP_Presentity::Internal_SendLocalPresence()
{
  PTRACE(3, "SIPPres\t'" << GetSIPAOR() << "' sending own presence " << m_localPresence << "/" << m_localPresenceNote);

  // send presence
  SIPPresenceInfo info;

  info.m_presenceAgent = m_presenceServer.GetAddress().AsString();
  info.m_address       = GetSIPAOR().AsString();

  if (m_localPresence == NoPresence) {
    m_endpoint->PublishPresence(info, 0);
    return;
  }

  unsigned expire = GetAttribute(OpalPresentity::TimeToLiveKey, "30").AsInteger();

  if (0 <= m_localPresence && m_localPresence <= SIPPresenceInfo::Unchanged)
    info.m_basic = (SIPPresenceInfo::BasicStates)m_localPresence;
  else
    info.m_basic = SIPPresenceInfo::Unknown;

  info.m_note = m_localPresenceNote;

  m_endpoint->PublishPresence(info, expire);
}

void SIPXCAP_Presentity::Internal_SubscribeToPresence(const PString & presentity, bool start)
{
  {
    PresenceInfoMap::iterator r = m_presenceInfo.find(presentity);
    if (start && (r != m_presenceInfo.end())) {
      PTRACE(3, "SIPPres\t'" << GetSIPAOR() << "' already subscribed to presence of '" << presentity << "'");
      return;
    }
    else if (!start && (r == m_presenceInfo.end())) {
      PTRACE(3, "SIPPres\t'" << GetSIPAOR() << "' already unsubscribed to presence of '" << presentity << "'");
      return;
    }
  }

  PTRACE(3, "SIPPres\t'" << GetSIPAOR() << "' " << (start ? "" : "un") << "subscribing to presence of '" << presentity << "'");

  // subscribe to the presence event on the presence server
  SIPSubscribe::Params param(SIPSubscribe::Presence);

  SIPURL aor(GetSIPAOR());
  PString aorStr(aor.AsString());

  param.m_localAddress    = aorStr;
  param.m_addressOfRecord = presentity;
  param.m_remoteAddress   = PString("sip:") + m_presenceServer.AsString();

  param.m_authID            = GetAttribute(OpalPresentity::AuthNameKey, aor.GetUserName() + '@' + aor.GetHostAddress());
  param.m_password          = GetAttribute(OpalPresentity::AuthPasswordKey);
  unsigned ttl = GetAttribute(OpalPresentity::TimeToLiveKey, "30").AsInteger();
  if (ttl == 0)
    ttl = 300;
  param.m_expire            = start ? ttl : 0;

  //param.m_onSubcribeStatus = PCREATE_NOTIFIER(OnPresenceSubscriptionStatus);
  param.m_onNotify         = PCREATE_NOTIFIER(OnPresenceNotify);

  if (!start)
    m_presenceInfo.erase(presentity);
  else {
    PresenceInfo info;
    info.m_subscriptionState = PresenceInfo::e_Subscribing;
    m_presenceInfo.insert(PresenceInfoMap::value_type(presentity, info));
  }

  PString actualAor;
  m_endpoint->Subscribe(param, actualAor);
}

void SIPXCAP_Presentity::OnPresenceNotify(SIPSubscribeHandler &, INT s)
{
  SIPSubscribe::NotifyCallbackInfo & status = *(SIPSubscribe::NotifyCallbackInfo *)s;

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

  if (params[0] *= "active")
    ;
  else if (params[0] *= "pending")
    ;
  else if (params[0] *= "terminated")
    ;
  else
    ;

  // get entity requesting access to presence information
  PString fromStr = status.m_notify.GetMIME().GetFrom();

  PTRACE(3, "SIPPres\t'" << fromStr << "' request for presence of '" << GetSIPAOR() << "' is " << params[0]);

  // return OK;
  status.m_response.SetStatusCode(SIP_PDU::Successful_OK);
  status.m_response.GetInfo() = "OK";
}
