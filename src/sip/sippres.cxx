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

const char * SIP_Presentity::DefaultPresenceServerKey = "default_presence_server";
const char * SIP_Presentity::PresenceServerKey        = "presence_server";

static PFactory<OpalPresentity>::Worker<SIPLocal_Presentity> sip_local_PresentityWorker("sip-local");
static PFactory<OpalPresentity>::Worker<SIPXCAP_Presentity>  sip_xcap_PresentityWorker1("sip-xcap");
static PFactory<OpalPresentity>::Worker<SIPXCAP_Presentity>  sip_xcap_PresentityWorker2("sip");

//////////////////////////////////////////////////////////////////////////////////////

SIP_Presentity::SIP_Presentity()
  : m_manager(NULL)
  , m_endpoint(NULL)
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
    PTRACE(1, "OpalPres\tCannot open SIP_Presentity without sip endpoint");
    return false;
  }

  if (!InternalOpen()) {
    PTRACE(1, "OpalPres\tCannot open presence client");
    return false;
  }

  // subscribe to our own watcher information
  if (!SetNotifySubscriptions(true))
    return false;

  return true;
}

bool SIP_Presentity::Close()
{
  SetNotifySubscriptions(false);
  InternalClose();

  m_endpoint = NULL;

  return true;
}


bool SIP_Presentity::Save(OpalPresentityStore * store)
{
  return false;
}

bool SIP_Presentity::Restore(OpalPresentityStore * store)
{
  return false;
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

bool SIPLocal_Presentity::SetNotifySubscriptions(bool start)
{
  return false;
}


bool SIPLocal_Presentity::SetPresence(SIP_Presentity::State state_, const PString & note)
{
  return false;
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
    PTRACE(1, "OpalPres\tSRV lookup for '" << sipAOR.GetHostName() << "_pres._sip' succeeded");
    m_presenceServer = addrs[0];
  }
  else if (HasAttribute(SIP_Presentity::DefaultPresenceServerKey)) {
    m_presenceServer = GetAttribute(SIP_Presentity::DefaultPresenceServerKey);
  } 
  else
    m_presenceServer = PIPSocketAddressAndPort(sipAOR.GetHostName(), sipAOR.GetPort());

  if (!m_presenceServer.GetAddress().IsValid()) {
    PTRACE(1, "OpalPres\tUnable to lookup hostname for '" << m_presenceServer.GetAddress() << "'");
    return false;
  }

  SetAttribute(SIP_Presentity::PresenceServerKey, m_presenceServer.AsString());

  return true;
}

bool SIPXCAP_Presentity::InternalClose()
{
  return true;
}

bool SIPXCAP_Presentity::SetNotifySubscriptions(bool start)
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
    param.m_authID            = GetAttribute(OpalPresentity::AuthNameKey, aor.GetUserName() + '@' + aor.GetHostAddress());
    param.m_password          = GetAttribute(OpalPresentity::AuthPasswordKey);
    unsigned ttl = GetAttribute(OpalPresentity::TimeToLiveKey, "30").AsInteger();
    if (ttl == 0)
      ttl = 300;
    param.m_expire            = start ? ttl : 0;
    //param.m_contactAddress    = m_Contact.p_str();

    status = m_endpoint->Subscribe(param, aorStr) ? 1 : 2;
  }

  return status != 2;
}


bool SIPXCAP_Presentity::SetPresence(SIP_Presentity::State state_, const PString & note)
{
  SIPPresenceInfo info;

  info.m_presenceAgent = m_presenceServer.GetAddress().AsString();
  info.m_address       = GetSIPAOR().AsString();

  if (state_ == NoPresence)
    return m_endpoint->PublishPresence(info, 0);

  int state = state_;

  if (0 <= state && state <= SIPPresenceInfo::Unchanged)
    info.m_basic = (SIPPresenceInfo::BasicStates)state;
  else
    info.m_basic = SIPPresenceInfo::Unknown;

  info.m_note = note;

  return m_endpoint->PublishPresence(info);
}
