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

class OpalPresentity : public PSafeObject
{
  public:
    static const char * AddressOfRecordKey;
    static const char * AuthNameKey;
    static const char * AuthPasswordKey;
    static const char * FullNameKey;
    static const char * GUIDKey;
    static const char * SchemeKey;
    static const char * TimeToLiveKey;

    class Attributes :  public PStringToString
    {
      public:
        virtual bool Has(const PString & key) const
        { return Contains(key); }

        virtual PString Get(const PString & key, const PString & deflt = PString::Empty()) const
        { return (*this)(key, deflt); }

        virtual void Set(const PString & key, const PString & value)
        { SetAt(key, value); }
    };

    enum State {
      NoPresence      = -1,    // remove presence status - not the same as NotAvailable or Away

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

    static OpalPresentity * Create(
      const PString & url
    );

    OpalPresentity();

    virtual bool Open(
      OpalManager * manager = NULL
    );

    virtual bool IsOpen() const = 0;

    virtual bool Close() = 0;

    PString GetAOR() const { return m_attributes.Get(AddressOfRecordKey); }

    enum {
      e_SetPresenceState         = 1,
      e_SubscribeToPresence,
      e_UnsubscribeFromPresence,
      e_AuthorisePresence,
      e_DenyPresence,
      e_PoliteDenyPresence,
      e_ProtocolSpecificCommand  = 10000
    };

    //
    //  Set our presence state
    // 
    virtual bool SetPresence(
      State state, 
      const PString & note = PString::Empty()
    );


    //
    //  Subscribe to presence state of another presentity
    // 
    virtual bool SubscribeToPresence(
      const PString & presentity
    );


    //
    //  Called when another presentity requests access to our presence information
    // 
    virtual bool OnRequestPresence(
      const PString & presentity
    );


    //
    //  Called to allow/deny another presentity access to our presence information
    //
    virtual bool SetPresenceAuthorisation(
      const PString & presentity,
      int authoriseMode
    );

    Attributes & GetAttributes() { return m_attributes; }

    typedef PAtomicInteger::IntegerType CmdSeqType;

    class Command {
      public:
        Command(unsigned c, bool responseNeeded = false) 
          : m_cmd(c), m_responseNeeded(responseNeeded)
        { }
        virtual ~Command() { }

        unsigned m_cmd;
        bool m_responseNeeded;
        CmdSeqType m_sequence;
    };

    class SetPresenceCommand : public Command {
      public:
        SetPresenceCommand(State state, const PString & note = PString::Empty()) 
          : Command(e_SetPresenceState), m_state(state), m_note(note)
        { }
        State m_state;
        PString m_note;
    };


    class SimpleCommand : public Command {
      public:
        SimpleCommand(unsigned c, const PString & e, bool responseNeeded = false) 
          : Command(c, responseNeeded), m_presEntity(e)
        { }
        PString m_presEntity;
    };

    virtual CmdSeqType SendCommand(
      Command * cmd
    ) = 0;

    void SetRequestPresenceNotifier(const PNotifier2 & n)
    { PWaitAndSignal m(m_onRequestPresenceNotifierMutex); m_onRequestPresenceNotifier = n; }

  protected:
    Attributes m_attributes;
    OpalGloballyUniqueID m_guid;

    PNotifier2 m_onRequestPresenceNotifier;
    PMutex m_onRequestPresenceNotifierMutex;
};

#endif  // OPAL_IM_PRES_ENT_H

