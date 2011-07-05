/*
 * Python bindings.
 *
 * Open Phone Abstraction Library (OPAL)
 *
 * Copyright (c) 2011 Demetrius Cassidy
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
 * The Original Code is Open Phone Abstraction Library (OPAL)
 *
 * The Initial Developer of the Original Code is Demetrius Cassidy
 *
 * Contributor(s): ______________________________________.
 *
 * $Revision$
 * $Author$
 * $Date$
 */

%ModuleHeaderCode
#include <ptlib.h>
#include <opal/pres_ent.h>
%End


////////////////////////////////////////////////////////////////////////////////////////////////////

/**Presencu state information
  */
class OpalPresenceInfo
{
  public:
    /// Presence states.
    enum State {
      InternalError = -3,    // something bad happened
      Forbidden     = -2,    // access to presence information was specifically forbidden
      NoPresence    = -1,    // remove presence status - not the same as Unavailable or Away

      // basic states (from RFC 3863)
      Unchanged,
      Available,
      Unavailable,

      // extended states (from RFC 4480)
      // if this is changed, also change the tables in sippres.cxx and handlers.cxx - look for RFC 4480
      ExtendedBase    = 100,
      UnknownExtended = ExtendedBase,
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

    State   m_state;    ///< New state for presentity
    PString m_note;     ///< Additional information about state change
    PURL    m_entity;   ///< The presentity whose state had changed
    PURL    m_target;   ///< The presentity that is being informed about the state change

    OpalPresenceInfo(State state = Unchanged);

    static PString AsString(State state);
    static State FromString(const PString & str);
    PString AsString() const;
};