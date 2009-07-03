/*
 * sippres.h
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

#ifndef OPAL_SIP_SIPPRES_H
#define OPAL_SIP_SIPPRES_H

#include <ptlib.h>
#include <opal/buildopts.h>

#include <opal/pres_ent.h>

class SIP_PresEntity : public OpalPresEntity
{
  public:
    static const char * DefaultPresenceServerKey;
    static const char * PresenceServerKey;
    static const char * ProfileKey;

    class Profile {
      public:
        Profile();

        virtual bool Open(SIP_PresEntity * presEntity);

        virtual bool Close() = 0;

        virtual bool SetNotifySubscriptions(bool on) = 0;

        virtual bool SetPresence(State state, const PString & note) = 0;

        virtual bool RemovePresence() = 0;
  
      protected:
        SIP_PresEntity * m_presEntity;
    };

    SIP_PresEntity();
    ~SIP_PresEntity();

    virtual bool IsOpen() const { return m_endpoint != NULL; }

    virtual bool Open(OpalManager & manager, const OpalGloballyUniqueID & uid);

    virtual bool Close();

    virtual bool SetPresence(
      State state,
      const PString & note = PString::Empty()
    );

    virtual bool RemovePresence();
  
    SIPURL GetSIPAOR() const { return SIPURL(GetAttribute(AddressOfRecordKey));  }

    SIPEndPoint & GetEndpoint() { return *m_endpoint; }

  protected:
    SIPEndPoint * m_endpoint;
    Profile * m_profile;
};

#endif // OPAL_SIP_SIPPRES_H
