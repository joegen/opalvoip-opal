/*
 * prese_ent.h
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

#ifndef OPAL_IM_PRES_ENT_H
#define OPAL_IM_PRES_ENT_H

#include <ptlib.h>
#include <opal/buildopts.h>

#include <opal/manager.h>
#include <ptclib/url.h>
#include <sip/sipep.h>

////////////////////////////////////////////////////////////////////////////////////////////////////

class OpalPresEntity : public PSafeObject
{
  public:
    static const char * AddressOfRecordKey;
    static const char * AuthNameKey;
    static const char * AuthPasswordKey;
    static const char * FullNameKey;
    static const char * TimeToLiveKey;

    enum State {
      // basic states (from RFC 3863) - must be same order as SIPPresenceInfo::BasicStates
      Unknown         = SIPPresenceInfo::Unknown,
      Available,
      NotAvailable,
      Unchanged,

      // extended states (from RFC 4480) - must be same order as SIPPresenceInfo::ExtendedStates
      ExtendedBase    = 100,
      UnknownExtended = ExtendedBase + SIPPresenceInfo::UnknownExtended,
      Appointment,
      Away,
      Breakfast,
      Busy,
      Dinner,
      Holiday,
      InTransit,
      LookingForWork,
      Lunch,
      Meal,
      Meeting,
      OnThePhone,
      Other,
      Performance,
      PermanentAbsence,
      Playing,
      Presentation,
      Shopping,
      Sleeping,
      Spectator,
      Steering,
      Travel,
      TV,
      Vacation,
      Working,
      Worship
    };

    static OpalPresEntity * Create(
      const PString & url
    );

    virtual bool Open(
      OpalManager & manager
    );

    virtual bool Open(
      OpalManager & manager,
      const OpalGloballyUniqueID & uid
    ) = 0;

    virtual bool IsOpen() const = 0;

    virtual bool Close() = 0;

    virtual bool SetPresence(
      State state,
      const PString & note = PString::Empty()
    ) = 0;

    virtual bool RemovePresence() = 0;
  
    virtual bool HasAttribute(const PString & key) const
    { return m_attributes.Contains(key); }

    virtual PString GetAttribute(const PString & key, const PString & deflt = PString::Empty()) const
    { return m_attributes(key, deflt); }

    virtual void SetAttribute(const PString & key, const PString & value)
    { m_attributes.SetAt(key, value); }


  protected:
    OpalGloballyUniqueID m_guid;

    PStringToString m_attributes;
};

#endif  // OPAL_IM_PRES_ENT_H

