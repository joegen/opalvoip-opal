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
const char * SIP_PresEntity::ProfileKey               = "profile";

//////////////////////////////////////////////////////////////////////////////////////

class LocalProfile : public SIP_PresEntity::Profile {
  public:
    LocalProfile()
    { }

    bool Open(SIP_PresEntity * presEntity);

    bool Close() 
    { return true; }

    virtual bool SetNotifySubscriptions(bool /*on*/)
    { return true; }

    virtual bool SetPresence(SIP_PresEntity::State /*state*/, const PString & /*note*/)
    { return false; }

    virtual bool RemovePresence()
    { return false; }
};

static PFactory<SIP_PresEntity::Profile>::Worker<LocalProfile> SIP_PresEntity_LocalProfile("local");

//////////////////////////////////////////////////////////////////////////////////////

class XCAPProfile : public SIP_PresEntity::Profile {
  public:
    XCAPProfile();
    virtual bool Open(SIP_PresEntity * presentity);
    virtual bool Close();
    virtual bool SetNotifySubscriptions(bool on);
    virtual bool SetPresence(SIP_PresEntity::State state, const PString & note);
    virtual bool RemovePresence();

  protected:
    PIPSocketAddressAndPort m_presenceServer;
};

static PFactory<SIP_PresEntity::Profile>::Worker<XCAPProfile>  SIP_PresEntity_XCAPProfile("xcap");

//////////////////////////////////////////////////////////////////////////////////////

SIP_PresEntity::SIP_PresEntity()
  : m_endpoint(NULL)
  , m_profile(NULL)
{ 
}

SIP_PresEntity::~SIP_PresEntity()
{
  Close();
}

bool SIP_PresEntity::Open(OpalManager & manager, const OpalGloballyUniqueID & uid)
{
  Close();

  // find the endpoint
  m_endpoint = dynamic_cast<SIPEndPoint *>(manager.FindEndPoint("sip"));
  if (m_endpoint == NULL) {
    PTRACE(1, "OpalPres\tCannot open SIP_PresEntity without sip endpoint");
    return false;
  }

  PString profile(GetAttribute("profile", "local"));
  m_profile = PFactory<SIP_PresEntity::Profile>::CreateInstance(profile);
  if (m_profile == NULL) {
    PTRACE(1, "OpalPres\tCannot find SIP_PresEntity profile '" << m_profile << "'");
    return false;
  }

  if (!m_profile->Open(this)) {
    PTRACE(1, "OpalPres\tCannot open profile");
    return false;
  }

  m_guid = uid;

  // ask profile to send notifies
  if (!m_profile->SetNotifySubscriptions(true))
    return false;

  return true;
}

bool SIP_PresEntity::Close()
{
  if (m_profile != NULL) {
    m_profile->SetNotifySubscriptions(false);
    m_profile->Close();
  }

  m_endpoint = NULL;

  return true;
}


bool SIP_PresEntity::SetPresence(State state, const PString & note)
{
  if (m_profile == NULL)
    return false;

  return m_profile->SetPresence(state, note);
}

bool SIP_PresEntity::RemovePresence()
{
  if (m_profile == NULL)
    return false;

  return m_profile->RemovePresence();
}


typedef SIP_PresEntity Pres_PresEntity;

static PFactory<OpalPresEntity>::Worker<SIP_PresEntity> OpalSIPPresEntityProfile_worker("sip");
static PFactory<OpalPresEntity>::Worker<Pres_PresEntity> OpalPRESPresEntityProfile_worker("pres");

//////////////////////////////////////////////////////////////

SIP_PresEntity::Profile::Profile()
  : m_presEntity(NULL)
{ }

bool SIP_PresEntity::Profile::Open(SIP_PresEntity * presEntity)
{ 
  m_presEntity = presEntity; 
  return true;
}

//////////////////////////////////////////////////////////////

bool LocalProfile::Open(SIP_PresEntity * presEntity)
{
  if (!Profile::Open(presEntity))
    return false;

  return true;
}

//////////////////////////////////////////////////////////////

XCAPProfile::XCAPProfile()
{
}

bool XCAPProfile::Open(SIP_PresEntity * presEntity)
{
  if (!Profile::Open(presEntity))
    return false;

  // find presence server for presentity as per RFC 3861
  // if not found, look for default presence server setting
  // if none, use hostname portion of domain name
  SIPURL sipAOR(m_presEntity->GetSIPAOR());
  PIPSocketAddressAndPortVector addrs;
  if (PDNS::LookupSRV(sipAOR.GetHostName(), "_pres._sip", sipAOR.GetPort(), addrs) && addrs.size() > 0) {
    PTRACE(1, "OpalPres\tSRV lookup for '" << sipAOR.GetHostName() << "_pres._sip' succeeded");
    m_presenceServer = addrs[0];
  }
  else if (m_presEntity->HasAttribute(SIP_PresEntity::DefaultPresenceServerKey)) {
    m_presenceServer = m_presEntity->GetAttribute(SIP_PresEntity::DefaultPresenceServerKey);
  } 
  else
    m_presenceServer = PIPSocketAddressAndPort(sipAOR.GetHostName(), sipAOR.GetPort());

  if (!m_presenceServer.GetAddress().IsValid()) {
    PTRACE(1, "OpalPres\tUnable to lookup hostname for '" << m_presenceServer.GetAddress() << "'");
    return false;
  }

  m_presEntity->SetAttribute(SIP_PresEntity::PresenceServerKey, m_presenceServer.AsString());

  return true;
}

bool XCAPProfile::Close()
{
  return true;
}

bool XCAPProfile::SetNotifySubscriptions(bool start)
{
  // subscribe to the presence.winfo event on the presence server
  SIPEndPoint & sipEP = m_presEntity->GetEndpoint();
  unsigned type = SIPSubscribe::Presence | SIPSubscribe::Watcher;
  SIPURL aor(m_presEntity->GetSIPAOR());
  PString aorStr(aor.AsString());

  int status;
  if (sipEP.IsSubscribed(type, aorStr, true))
    status = 0;
  else {
    SIPSubscribe::Params param(type);
    param.m_addressOfRecord   = aorStr;
    param.m_agentAddress      = m_presenceServer.GetAddress().AsString();
    param.m_authID            = m_presEntity->GetAttribute(OpalPresEntity::AuthNameKey, aor.GetUserName() + '@' + aor.GetHostAddress());
    param.m_password          = m_presEntity->GetAttribute(OpalPresEntity::AuthPasswordKey);
    unsigned ttl = m_presEntity->GetAttribute(OpalPresEntity::TimeToLiveKey, "30").AsInteger();
    if (ttl == 0)
      ttl = 300;
    param.m_expire            = start ? ttl : 0;
    //param.m_contactAddress    = m_Contact.p_str();

    status = sipEP.Subscribe(param, aorStr) ? 1 : 2;
  }

  return status != 2;
}


bool XCAPProfile::SetPresence(SIP_PresEntity::State state_, const PString & note)
{
  SIPPresenceInfo info;

  info.m_presenceAgent = m_presenceServer.GetAddress().AsString();
  info.m_address       = m_presEntity->GetSIPAOR().AsString();

  int state = state_;

  if (0 <= state && state <= SIPPresenceInfo::Unchanged)
    info.m_basic = (SIPPresenceInfo::BasicStates)state;
  else
    info.m_basic = SIPPresenceInfo::Unknown;

  info.m_note = note;

  SIPEndPoint & sipEP = m_presEntity->GetEndpoint();
  return sipEP.PublishPresence(info);
}

bool XCAPProfile::RemovePresence()
{
  SIPPresenceInfo info;

  info.m_presenceAgent = m_presenceServer.GetAddress().AsString();
  info.m_address       = m_presEntity->GetSIPAOR().AsString();
  info.m_basic         = SIPPresenceInfo::Unknown;

  SIPEndPoint & sipEP = m_presEntity->GetEndpoint();
  return sipEP.PublishPresence(info, 0);
}
