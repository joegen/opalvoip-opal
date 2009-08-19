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

const char * SIP_PresEntity::DefaultPresenceServerKey = "default_presence_server";
const char * SIP_PresEntity::PresenceServerKey        = "presence_server";

static PFactory<OpalPresEntity>::Worker<SIPLocal_PresEntity> sip_local_presentityWorker("sip-local");
static PFactory<OpalPresEntity>::Worker<SIPXCAP_PresEntity>  sip_xcap_presentityWorker1("sip-xcap");
static PFactory<OpalPresEntity>::Worker<SIPXCAP_PresEntity>  sip_xcap_presentityWorker2("sip");

//////////////////////////////////////////////////////////////////////////////////////

SIP_PresEntity::SIP_PresEntity()
  : m_endpoint(NULL)
{ 
}

SIP_PresEntity::~SIP_PresEntity()
{
}

bool SIP_PresEntity::Open(OpalManager & manager, const OpalGloballyUniqueID & uid)
{
  // if we have an endpoint, then close the entity
  if (m_endpoint != NULL)
    Close();

  // find the endpoint
  m_endpoint = dynamic_cast<SIPEndPoint *>(manager.FindEndPoint("sip"));
  if (m_endpoint == NULL) {
    PTRACE(1, "OpalPres\tCannot open SIP_PresEntity without sip endpoint");
    return false;
  }

  if (!InternalOpen()) {
    PTRACE(1, "OpalPres\tCannot open presence client");
    return false;
  }

  m_guid = uid;

  // ask profile to send notifies
  if (!SetNotifySubscriptions(true))
    return false;

  return true;
}

bool SIP_PresEntity::Close()
{
  SetNotifySubscriptions(false);
  InternalClose();

  m_endpoint = NULL;

  return true;
}

//////////////////////////////////////////////////////////////

SIPLocal_PresEntity::~SIPLocal_PresEntity()
{
  Close();
}

bool SIPLocal_PresEntity::InternalOpen()
{
  return true;
}

bool SIPLocal_PresEntity::InternalClose()
{
  return true;
}

bool SIPLocal_PresEntity::SetNotifySubscriptions(bool start)
{
  return false;
}


bool SIPLocal_PresEntity::SetPresence(SIP_PresEntity::State state_, const PString & note)
{
  return false;
}


bool SIPLocal_PresEntity::RemovePresence()
{
  return false;
}

//////////////////////////////////////////////////////////////

SIPXCAP_PresEntity::SIPXCAP_PresEntity()
{
}


SIPXCAP_PresEntity::~SIPXCAP_PresEntity()
{
  Close();
}


bool SIPXCAP_PresEntity::InternalOpen()
{
  // find presence server for presentity as per RFC 3861
  // if not found, look for default presence server setting
  // if none, use hostname portion of domain name
  SIPURL sipAOR(GetSIPAOR());
  PIPSocketAddressAndPortVector addrs;
  if (PDNS::LookupSRV(sipAOR.GetHostName(), "_pres._sip", sipAOR.GetPort(), addrs) && addrs.size() > 0) {
    PTRACE(1, "OpalPres\tSRV lookup for '" << sipAOR.GetHostName() << "_pres._sip' succeeded");
    m_presenceServer = addrs[0];
  }
  else if (HasAttribute(SIP_PresEntity::DefaultPresenceServerKey)) {
    m_presenceServer = GetAttribute(SIP_PresEntity::DefaultPresenceServerKey);
  } 
  else
    m_presenceServer = PIPSocketAddressAndPort(sipAOR.GetHostName(), sipAOR.GetPort());

  if (!m_presenceServer.GetAddress().IsValid()) {
    PTRACE(1, "OpalPres\tUnable to lookup hostname for '" << m_presenceServer.GetAddress() << "'");
    return false;
  }

  SetAttribute(SIP_PresEntity::PresenceServerKey, m_presenceServer.AsString());

  return true;
}

bool SIPXCAP_PresEntity::InternalClose()
{
  return true;
}

bool SIPXCAP_PresEntity::SetNotifySubscriptions(bool start)
{
  // subscribe to the presence.winfo event on the presence server
  unsigned type = SIPSubscribe::Presence | SIPSubscribe::Watcher;
  SIPURL aor(GetSIPAOR());
  PString aorStr(aor.AsString());

  int status;
  if (m_endpoint->IsSubscribed(type, aorStr, true))
    status = 0;
  else {
    SIPSubscribe::Params param(type);
    param.m_addressOfRecord   = aorStr;
    param.m_agentAddress      = m_presenceServer.GetAddress().AsString();
    param.m_authID            = GetAttribute(OpalPresEntity::AuthNameKey, aor.GetUserName() + '@' + aor.GetHostAddress());
    param.m_password          = GetAttribute(OpalPresEntity::AuthPasswordKey);
    unsigned ttl = GetAttribute(OpalPresEntity::TimeToLiveKey, "30").AsInteger();
    if (ttl == 0)
      ttl = 300;
    param.m_expire            = start ? ttl : 0;
    //param.m_contactAddress    = m_Contact.p_str();

    status = m_endpoint->Subscribe(param, aorStr) ? 1 : 2;
  }

  return status != 2;
}


bool SIPXCAP_PresEntity::SetPresence(SIP_PresEntity::State state_, const PString & note)
{
  SIPPresenceInfo info;

  info.m_presenceAgent = m_presenceServer.GetAddress().AsString();
  info.m_address       = GetSIPAOR().AsString();

  int state = state_;

  if (0 <= state && state <= SIPPresenceInfo::Unchanged)
    info.m_basic = (SIPPresenceInfo::BasicStates)state;
  else
    info.m_basic = SIPPresenceInfo::Unknown;

  info.m_note = note;

  return m_endpoint->PublishPresence(info);
}

bool SIPXCAP_PresEntity::RemovePresence()
{
  SIPPresenceInfo info;

  info.m_presenceAgent = m_presenceServer.GetAddress().AsString();
  info.m_address       = GetSIPAOR().AsString();
  info.m_basic         = SIPPresenceInfo::Unknown;

  return m_endpoint->PublishPresence(info, 0);
}
