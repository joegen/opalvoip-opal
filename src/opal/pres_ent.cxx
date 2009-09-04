/*
 * pres_ent.cxx
 *
 * Presence Entity classes for Opal
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

#include <opal/pres_ent.h>

#include <opal/manager.h>
#include <ptclib/url.h>
#include <sip/sipep.h>

const char * OpalPresentity::AddressOfRecordKey = "address_of_record";
const char * OpalPresentity::AuthNameKey        = "auth_name";
const char * OpalPresentity::AuthPasswordKey    = "auth_password";
const char * OpalPresentity::FullNameKey        = "full_name";
const char * OpalPresentity::GUIDKey            = "guid";
const char * OpalPresentity::SchemeKey          = "scheme";
const char * OpalPresentity::TimeToLiveKey      = "time_to_live";


OpalPresentity::OpalPresentity()
{
}

bool OpalPresentity::Open(OpalManager *)
{
  m_guid = OpalGloballyUniqueID();
  SetAttribute(GUIDKey, m_guid.AsString());
  return true;
}


bool OpalPresentity::Close()
{
  return false;
}

OpalPresentity * OpalPresentity::Create(const PString & url)
{
  PString scheme = PURL(url).GetScheme();

  OpalPresentity * presEntity = PFactory<OpalPresentity>::CreateInstance(scheme);
  if (presEntity == NULL) 
    return NULL;

  presEntity->SetAttribute(AddressOfRecordKey, url);

  return presEntity;
}

bool OpalPresentity::SetPresence(State state, const PString & note)
{
  Command * cmd = new SetPresenceCommand(state, note);
  SendCommand(cmd);
  return true;
}

bool OpalPresentity::SubscribeToPresence(const PString & presentity)
{
  Command * cmd = new SimpleCommand(e_SubscribeToPresence, presentity);
  SendCommand(cmd);
  return true;
}


/////////////////////////////////////////////////////////////////////////////

