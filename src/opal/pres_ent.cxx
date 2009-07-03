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

const char * OpalPresEntity::AddressOfRecordKey = "address_of_record";
const char * OpalPresEntity::AuthNameKey        = "auth_name";
const char * OpalPresEntity::AuthPasswordKey    = "auth_password";
const char * OpalPresEntity::FullNameKey        = "full_name";
const char * OpalPresEntity::TimeToLiveKey      = "time_to_live";

bool OpalPresEntity::Open(OpalManager & manager)
{
  return Open(manager, OpalGloballyUniqueID()); 
}


bool OpalPresEntity::Close()
{
  return false;
}

OpalPresEntity * OpalPresEntity::Create(const PString & url)
{
  PString scheme = PURL(url).GetScheme();

  OpalPresEntity * presEntity = PFactory<OpalPresEntity>::CreateInstance(scheme);
  if (presEntity == NULL) 
    return NULL;

  presEntity->SetAttribute(AddressOfRecordKey, url);

  return presEntity;
}

