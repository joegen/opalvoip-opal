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


class OpalPresentityCommand;


////////////////////////////////////////////////////////////////////////////////////////////////////

/**Representation of a presence identity.
   This class contains an abstraction of the functionality for "presence"
   using a URL string as the "identity". The concrete class depends on the
   scheme of the identity URL.

   The general architecture is that commands will be sent to the preentity
   concrete class via concrete versions of OpalPresentityCommand class. These
   may be protocol specific or one of the abstracted versions.
  */
class OpalPresentity : public PSafeObject
{
    PCLASSINFO(OpalPresentity, PSafeObject);

  /**@name Construction */
  //@{
  protected:
    /// Construct the presentity class
    OpalPresentity();

  public:
    /**Create a concrete class based on the scheme of the URL provided.
      */
    static OpalPresentity * Create(
      OpalManager & manager, ///< Manager for presentity
      const PString & url   ///< URL for presence identity
    );
  //@}

  /**@name Initialisation */
  //@{
    /** Open the presentity handler.
        This will perform whatever is required to allow this presentity
        to access servers etc for the underlying protocol. It may start open
        network channels, start threads etc.

        Note that a return value of true does not necessarily mean that the
        presentity has successfully been indicated as "present" on the server
        only that the underlying system can do so at some time.
      */
    virtual bool Open() = 0;

    /** Indicate if the presentity has been successfully opened.
      */
    virtual bool IsOpen() const = 0;

    /**Close the presentity.
      */
    virtual bool Close() = 0;
  //@}

  /**@name Attributes */
  //@{
    /**Dictionary of attributes associated with this presentity.
      */
    class Attributes :  public PStringToString
    {
      public:
        /// Determine of the attribute exists.
        virtual bool Has(const PString &   key   ) const { return Contains(key  ); }
        virtual bool Has(const PString & (*key)()) const { return Contains(key()); }

        /// Get the attribute value.
        virtual PString Get(const PString &   key   , const PString & deflt = PString::Empty()) const { return (*this)(key  , deflt); }
                PString Get(const PString & (*key)(), const PString & deflt = PString::Empty()) const { return (*this)(key(), deflt); }

        /// Set the attribute value.
        virtual void Set(const PString &   key   , const PString & value) { SetAt(key  , value); }
                void Set(const PString & (*key)(), const PString & value) { SetAt(key(), value); }
    };

    ///< Get the attributes for this presentity.
    Attributes & GetAttributes() { return m_attributes; }

    static const PString & AuthNameKey();         ///< Key for authentication name attribute
    static const PString & AuthPasswordKey();     ///< Key for authentication password attribute
    static const PString & FullNameKey();         ///< Key for full name attribute
    static const PString & SchemeKey();           ///< Key for scheme used attribute
    static const PString & TimeToLiveKey();       ///< Key for Time-To-Live attribute, in seconds for underlying protocol

    /** Get the address-of-record for the presentity.
        This is typically a URL which represents our local identity in the
        presense system.
      */
    const PURL & GetAOR() const { return m_aor; }
  //@}

  /**@name Commands */
  //@{
    /** Subscribe to presence state of another presentity.
        This function is a wrapper and the OpalSubscribeToPresenceCommand command.
      */
    virtual bool SubscribeToPresence(
      const PString & presentity    ///< Other presentity to monitor
    );

    /** Unsubscribe to presence state of another presentity.
        This function is a wrapper and the OpalSubscribeToPresenceCommand command.
      */
    virtual bool UnsubscribeFromPresence(
      const PString & presentity    ///< Other presentity to monitor
    );

    /// Authorisation modes for SetPresenceAuthorisation()
    enum Authorisation {
      AuthorisationPermitted,
      AuthorisationDenied,
      AuthorisationDeniedPolitely,
      AuthorisationConfirming,
      NumAuthorisations
    };

    /** Called to allow/deny another presentity access to our presence information.
        This function is a wrapper and the OpalAuthorisationRequestCommand command.
      */
    virtual bool SetPresenceAuthorisation(
      const PString & presentity,     ///< Remote presentity to be authorised
      Authorisation authorisation     ///< Authorisation mode
    );

    /// Presence states.
    enum State {
      NoPresence      = -1,    // remove presence status - not the same as NotAvailable or Away

      // basic states (from RFC 3863) - must be same order as SIPPresenceInfo::BasicStates
      Unchanged       = SIPPresenceInfo::Unchanged,
      Available       = SIPPresenceInfo::Open,
      NotAvailable    = SIPPresenceInfo::Closed,

      // extended states (from RFC 4480) - must be same order as SIPPresenceInfo::ExtendedStates
      ExtendedBase    = 100,
      UnknownExtended = ExtendedBase + SIPPresenceInfo::UnknownActivity,
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

    /** Set our presence state.
        This function is a wrapper and the OpalSetLocalPresenceCommand command
      */
    virtual bool SetLocalPresence(
      State state,                              ///< New state for our presentity
      const PString & note = PString::Empty()   ///< Additional note attached to the state change
    );

    /** Low level function to create a command.
        As commands have protocol specific implementations, we use a factory
        to create them.
      */
    template <class cls>
    cls * CreateCommand()
    {
      cls * p = dynamic_cast<cls *>(PFactory<OpalPresentityCommand>::CreateInstance(
                       PDefaultPFactoryKey(typeid(*this).name())+typeid(cls).name()));
      return PAssertNULL(p);
    }

    /** Lowlevel function to send a command to the presentity handler.
        All commands are asynchronous. They will usually initiate an action
        for which an indication (callback) will give a result.

        Note that the command is typically created by the CreateCommand()
        function and is deleted by this function.
      */
    virtual bool SendCommand(
      OpalPresentityCommand * cmd   ///< Command to be processed
    ) = 0;
  //@}

  /**@name Indications (callbacks) */
  //@{
    /** Callback when another presentity requests access to our presence.
        It is expected that the handler will call SetPresenceAuthorisation
        with whatever policy is appropriate.

        Default implementation calls m_onRequestPresenceNotifier if non-NULL
        otherwise will authorise the request.
      */
    virtual void OnAuthorisationRequest(
      const PString & presentity    ///< Other presentity requesting our presence
    );

    typedef PNotifierTemplate<const PString &> AuthorisationRequestNotifier;
    #define PDECLARE_AuthorisationRequestNotifier(cls, fn) PDECLARE_NOTIFIER2(OpalPresentity, cls, fn, const PString &)
    #define PCREATE_AuthorisationRequestNotifier(fn) PCREATE_NOTIFIER2(fn, const PString &)

    /// Set the notifier for the OnAuthorisationRequest() function.
    void SetAuthorisationRequestNotifier(
      const AuthorisationRequestNotifier & notifier   ///< Notifier to be called by OnAuthorisationRequest()
    );

    /** Callback when another presentity has changed its state.
        Default implementation calls m_onPresenceChangeNotifier.
      */
    virtual void OnPresenceChange(
      const SIPPresenceInfo & info ///< Info on other presentity that changed state
    );

    typedef PNotifierTemplate<const SIPPresenceInfo &> PresenceChangeNotifier;
    #define PDECLARE_PresenceChangeNotifier(cls, fn) PDECLARE_NOTIFIER2(OpalPresentity, cls, fn, const SIPPresenceInfo &)
    #define PCREATE_PresenceChangeNotifier(fn) PCREATE_NOTIFIER2(fn, const SIPPresenceInfo &)

    /// Set the notifier for the OnPresenceChange() function.
    void SetPresenceChangeNotifier(
      const PresenceChangeNotifier & notifier   ///< Notifier to be called by OnPresenceChange()
    );
  //@}

  protected:
    OpalManager        * m_manager;
    OpalGloballyUniqueID m_guid;
    PURL                 m_aor;
    Attributes           m_attributes;

    AuthorisationRequestNotifier m_onAuthorisationRequestNotifier;
    PresenceChangeNotifier       m_onPresenceChangeNotifier;

    PMutex m_notificationMutex;
};


////////////////////////////////////////////////////////////////////////////////////////////////////

/**Representation of a presence identity that uses a background thread to
   process commands.
  */
class OpalPresentityWithCommandThread : public OpalPresentity
{
  /**@name Construction */
  //@{
  protected:
    /// Construct the presentity class that uses a command thread.
    OpalPresentityWithCommandThread();

  public:
    /** Destory the presentity class that uses a command thread.
        Note this will block until the background thread has stopped.
      */
    ~OpalPresentityWithCommandThread();
  //@}

  /**@name Overrides from OpalPresentity */
  //@{
    /** Lowlevel function to send a command to the presentity handler.
        All commands are asynchronous. They will usually initiate an action
        for which an indication (callback) will give a result.

        The default implementation queues the command for the background
        thread to handle.
      */
    virtual bool SendCommand(
      OpalPresentityCommand * cmd
    );
  //@}

  /**@name Overrides from OpalPresentity */
  //@{
    /**Start the background thread to handle commands.
       This is typically called from the concrete classes Open() function.
      */
    void StartThread();

    /**Stop the background thread to handle commands.
       This is typically called from the concrete classes Close() function.
       It is also called fro the destructor to be sure the thread has stopped
       before the object is destroyed.
      */
    void StopThread();
  //@}

  protected:
    void ThreadMain();

    typedef std::queue<OpalPresentityCommand *> CommandQueue;
    CommandQueue   m_commandQueue;
    PMutex         m_commandQueueMutex;
    PAtomicInteger m_commandSequence;
    PSyncPoint     m_commandQueueSync;

    bool      m_threadRunning;
    PThread * m_thread;
};


////////////////////////////////////////////////////////////////////////////////////////////////////

/**Abstract class for all OpelPresentity commands.
  */
class OpalPresentityCommand {
  public:
    OpalPresentityCommand(bool responseNeeded = false) 
      : m_responseNeeded(responseNeeded)
    { }
    virtual ~OpalPresentityCommand() { }

    /**Function to process the command.
       This typically calls functions on the concrete OpalPresentity class.
      */
    virtual void Process(
      OpalPresentity & presentity
    ) = 0;

    typedef PAtomicInteger::IntegerType CmdSeqType;
    CmdSeqType m_sequence;
    bool       m_responseNeeded;
    PString    m_presentity;
};

/** Macro to define the factory that creates a concrete command class.
  */
#define OPAL_DEFINE_COMMAND(command, entity, func) \
  class entity##_##command : public command \
  { \
    public: virtual void Process(OpalPresentity & presentity) { dynamic_cast<entity &>(presentity).func(*this); } \
  }; \
  static PFactory<OpalPresentityCommand>::Worker<entity##_##command> \
        s_entity##_##command(PDefaultPFactoryKey(typeid(entity).name())+typeid(command).name())


/** Command for subscribing to the status of another presentity.
  */
class OpalSubscribeToPresenceCommand : public OpalPresentityCommand {
  public:
    OpalSubscribeToPresenceCommand(bool subscribe = true) : m_subscribe(subscribe) { }

    bool m_subscribe;   ///< Flag to indicate subscribing/unsubscribing
};


/** Command for authorising a request by some other presentity to see our status.
    This action is typically due to some other presentity doing the equivalent
    of a OpalSubscribeToPresenceCommand. The mechanism by which this happens
    is dependent on the concrete OpalPresentity class and it's underlying
    protocol.
  */
class OpalAuthorisationRequestCommand : public OpalPresentityCommand {
  public:
    OpalAuthorisationRequestCommand() : m_authorisation(OpalPresentity::AuthorisationPermitted) { }

    OpalPresentity::Authorisation m_authorisation;  ///< Authorisation mode to indicate to remote
};


/** Command for adusting our current status.
    This action typically causes all presentities to be notified of the
    change. The mechanism by which this happens is dependent on the concrete
    OpalPresentity class and it's underlying protocol.
  */
class OpalSetLocalPresenceCommand : public OpalPresentityCommand {
  public:
    OpalSetLocalPresenceCommand() : m_state(OpalPresentity::NoPresence) { }

    OpalPresentity::State m_state;    ///< New state to move to.
    PString               m_note;     ///< Additional note attached to the state change
};


#endif  // OPAL_IM_PRES_ENT_H

