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
 * $Revision$
 * $Author$
 * $Date$
 */

#ifndef OPAL_IM_PRES_ENT_H
#define OPAL_IM_PRES_ENT_H

#include <ptlib.h>
#include <opal_config.h>

#include <ptlib/pfactory.h>
#include <ptlib/safecoll.h>
#include <ptclib/url.h>
#include <ptclib/guid.h>

#ifdef P_VCARD
#include <ptclib/vcard.h>
#endif

#include <im/im.h>

#include <list>
#include <queue>

class OpalManager;
class OpalPresentityCommand;


////////////////////////////////////////////////////////////////////////////////////////////////////

/**Presencu state information
  */
class OpalPresenceInfo : public PObject
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
    PTime   m_when;     ///< Time/date of state change

    OpalPresenceInfo(State state = Unchanged) : m_state(state) { }

    static PString AsString(State state);
    static State FromString(const PString & str);
    PString AsString() const;

    Comparison Compare(const PObject & other) const;
};

ostream & operator<<(ostream & strm, OpalPresenceInfo::State state);

////////////////////////////////////////////////////////////////////////////////////////////////////

class OpalSetLocalPresenceCommand;
class OpalSubscribeToPresenceCommand;
class OpalAuthorisationRequestCommand;
class OpalSendMessageToCommand;

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
    OpalPresentity(const OpalPresentity & other);

  public:
    ~OpalPresentity();

    /**Create a concrete class based on the scheme of the URL provided.
      */
    static OpalPresentity * Create(
      OpalManager & manager, ///< Manager for presentity
      const PURL & url,      ///< URL for presence identity
      const PString & scheme = PString::Empty() ///< Overide the url scheme
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
    virtual bool Open();

    /** Indicate if the presentity has been successfully opened.
      */
    virtual bool IsOpen() const { return m_open; }

    /**Close the presentity.
      */
    virtual bool Close();
  //@}

  /**@name Attributes */
  //@{
    ///< Get the attributes for this presentity.
    PStringOptions & GetAttributes() { return m_attributes; }

    ///< Get all attribute names for this presentity class.
    virtual PStringArray GetAttributeNames() const = 0;

    ///< Get all attribute types for this presentity class.
    virtual PStringArray GetAttributeTypes() const = 0;

    static const PCaselessString & AuthNameKey();         ///< Key for authentication name attribute
    static const PCaselessString & AuthPasswordKey();     ///< Key for authentication password attribute
    static const PCaselessString & TimeToLiveKey();       ///< Key for Time-To-Live attribute, in seconds for underlying protocol

    /** Get the address-of-record for the presentity.
        This is typically a URL which represents our local identity in the
        presense system.
      */
    const PURL & GetAOR() const { return m_aor; }
  //@}

  /**@name Commands */
  //@{
    /** Subscribe to presence state of another presentity.
        This function is a wrapper and the OpalSubscribeToPresenceCommand
        command.

        Returns true if the command was queued. Note that this does not
        mean the command succeeded, only that the underlying protocol is
        capabable of the action.
      */
    virtual bool SubscribeToPresence(
      const PURL & presentity,      ///< Other presentity to monitor
      bool subscribe = true,        ///< true if to subscribe, else unsubscribe
      const PString & note = PString::Empty() ///< Optional extra note attached to subscription request
    );

    /** Unsubscribe to presence state of another presentity.
        This function is a wrapper and the OpalSubscribeToPresenceCommand
        command.

        Returns true if the command was queued. Note that this does not
        mean the command succeeded, only that the underlying protocol is
        capabable of the action.
      */
    virtual bool UnsubscribeFromPresence(
      const PURL & presentity    ///< Other presentity to monitor
    );

    /// Authorisation modes for SetPresenceAuthorisation()
    enum Authorisation {
      AuthorisationPermitted,
      AuthorisationDenied,
      AuthorisationDeniedPolitely,
      AuthorisationConfirming,
      AuthorisationRemove,
      NumAuthorisations
    };

    /** Called to allow/deny another presentity access to our presence
        information.

        This function is a wrapper and the OpalAuthorisationRequestCommand
        command.

        Returns true if the command was queued. Note that this does not
        mean the command succeeded, only that the underlying protocol is
        capabable of the action.
      */
    virtual bool SetPresenceAuthorisation(
      const PURL & presentity,        ///< Remote presentity to be authorised
      Authorisation authorisation     ///< Authorisation mode
    );

    /** Set our presence state.
        This function is a wrapper and the OpalSetLocalPresenceCommand command.

        Returns true if the command was queued. Note that this does not
        mean the command succeeded, only that the underlying protocol is
        capabable of the action.
      */
    virtual bool SetLocalPresence(
      OpalPresenceInfo::State state,            ///< New state for our presentity
      const PString & note = PString::Empty()   ///< Additional note attached to the state change
    );

    /** Get our presence state
    */
    virtual bool GetLocalPresence(
      OpalPresenceInfo::State & state, 
      PString & note
    );


    /** Low level function to create a command.
        As commands have protocol specific implementations, we use a factory
        to create them.
      */
    template <class cls>
    __inline cls * CreateCommand()
    {
      return dynamic_cast<cls *>(InternalCreateCommand(typeid(cls).name()));
    }

    /** Lowlevel function to send a command to the presentity handler.
        All commands are asynchronous. They will usually initiate an action
        for which an indication (callback) will give a result.

        Note that the command is typically created by the CreateCommand()
        function and is subsequently deleted by this function.

        Returns true if the command was queued. Note that this does not
        mean the command succeeded, only that the underlying protocol is
        processing commands.
      */
    virtual bool SendCommand(
      OpalPresentityCommand * cmd   ///< Command to be processed
    );
  //@}

  /**@name Indications (callbacks) */
  //@{
    struct AuthorisationRequest
    {
      PURL    m_presentity; ///< Other presentity requesting our presence
      PString m_note;       ///< Optional extra note attached to request
    };

    /** Callback when another presentity requests access to our presence.
        It is expected that the handler will call SetPresenceAuthorisation
        with whatever policy is appropriate.

        Default implementation calls m_onRequestPresenceNotifier if non-NULL
        otherwise will authorise the request.
      */
    virtual void OnAuthorisationRequest(
      const AuthorisationRequest & request  ///< Authorisation request information
    );

    typedef PNotifierTemplate<AuthorisationRequest> AuthorisationRequestNotifier;
    #define PDECLARE_AuthorisationRequestNotifier(cls, fn) PDECLARE_NOTIFIER2(OpalPresentity, cls, fn, OpalPresentity::AuthorisationRequest)
    #define PDECLARE_ASYNC_AuthorisationRequestNotifier(cls, fn) PDECLARE_ASYNC_NOTIFIER2(OpalPresentity, cls, fn, OpalPresentity::AuthorisationRequest)
    #define PCREATE_AuthorisationRequestNotifier(fn) PCREATE_NOTIFIER2(fn, OpalPresentity::AuthorisationRequest)

    /// Set the notifier for the OnAuthorisationRequest() function.
    void SetAuthorisationRequestNotifier(
      const AuthorisationRequestNotifier & notifier   ///< Notifier to be called by OnAuthorisationRequest()
    );

    /** Callback when presentity has changed its state.
        Note if the m_entity and m_target fields of the OpalPresenceInfo
        structure are the same, then this indicates that the local presentity
        itself has successfully "registered" itself with the presence agent
        server.

        Default implementation calls m_onPresenceChangeNotifier.
      */
    virtual void OnPresenceChange(
      const OpalPresenceInfo & info ///< Info on other presentity that changed state
    );

    typedef PNotifierTemplate<OpalPresenceInfo> PresenceChangeNotifier;
    #define PDECLARE_PresenceChangeNotifier(cls, fn) PDECLARE_NOTIFIER2(OpalPresentity, cls, fn, OpalPresenceInfo)
    #define PDECLARE_ASYNC_PresenceChangeNotifier(cls, fn) PDECLARE_ASYNC_NOTIFIER2(OpalPresentity, cls, fn, OpalPresenceInfo)
    #define PCREATE_PresenceChangeNotifier(fn) PCREATE_NOTIFIER2(fn, OpalPresenceInfo)

    /// Set the notifier for the OnPresenceChange() function.
    void SetPresenceChangeNotifier(
      const PresenceChangeNotifier & notifier   ///< Notifier to be called by OnPresenceChange()
    );
  //@}

  /**@name Buddy Lists */
  //@{
    /**Buddy list entry.
       The buddy list is a list of presentities that the application is
       expecting to get presence status for.
      */
    struct BuddyInfo {
      BuddyInfo(
        const PURL & presentity = PString::Empty(),
        const PString & displayName = PString::Empty()
      ) : m_presentity(presentity)
        , m_displayName(displayName)
      { }

      PURL    m_presentity;   ///< Typicall URI address-of-record
      PString m_displayName;  ///< Human readable name

      // RFC4482 contact fields, note most of these are duplicated
      // in the vCard structure
#ifdef P_VCARD
      PvCard  m_vCard;        /**< vCard for the buddy. This is
                                   either a URI pointing to the image, or the image
                                   itself in the form: "data:text/x-vcard,xxxxx
                                   as per RFC2397 */
#endif
      PURL    m_icon;         /**< The icon/avatar/photo for the buddy. This is
                                   either a URI pointing to the image, or the image
                                   itself in the form: "data:image/jpeg;base64,xxxxx
                                   as per RFC2397 */
      PURL    m_map;          /**< The map reference for the buddy. This is may be URL
                                   to a GIS system or an image file similar to m_icon */
      PURL    m_sound;        /**< The sound relating to the buddy. This is
                                   either a URI pointing to the sound, or the sound
                                   itself in the form: "data:audio/mpeg;base64,xxxxx
                                   as per RFC2397 */
      PURL    m_homePage;     ///< Home page for buddy

      // Extra field for protocol dependent "get out of gaol" card
      PString m_contentType;  ///< MIME type code for XML
      PString m_rawXML;       ///< Raw XML of buddy list entry
    };

    typedef std::list<BuddyInfo> BuddyList;

    enum BuddyStatus {
      BuddyStatus_GenericFailure             = -1,
      BuddyStatus_OK                         = 0,
      BuddyStatus_SpecifiedBuddyNotFound,
      BuddyStatus_ListFeatureNotImplemented,
      BuddyStatus_ListTemporarilyUnavailable,
      BuddyStatus_ListMayBeIncomplete,
      BuddyStatus_BadBuddySpecification,
      BuddyStatus_ListSubscribeFailed,
      BuddyStatus_AccountNotLoggedIn
    };

    /**Get complete buddy list.
      */
    virtual BuddyStatus GetBuddyListEx(
      BuddyList & buddies   ///< List of buddies
    );
    virtual bool GetBuddyList(
      BuddyList & buddies   ///< List of buddies
    )
    { return GetBuddyListEx(buddies) == BuddyStatus_OK; }

    /**Set complete buddy list.
      */
    virtual BuddyStatus SetBuddyListEx(
      const BuddyList & buddies   ///< List of buddies
    );
    virtual bool SetBuddyList(
      const BuddyList & buddies   ///< List of buddies
    )
    { return SetBuddyListEx(buddies) == BuddyStatus_OK; }


    /**Delete the buddy list.
      */
    virtual BuddyStatus DeleteBuddyListEx();
    virtual bool DeleteBuddyList() { return DeleteBuddyListEx() == BuddyStatus_OK; }

    /**Get a specific buddy from the buddy list.
       Note the buddy.m_presentity field must be preset to the URI to search
       the buddy list for.
      */
    virtual BuddyStatus GetBuddyEx(
      BuddyInfo & buddy
    );
    virtual bool GetBuddy(
      BuddyInfo & buddy
    )
    { return GetBuddyEx(buddy) == BuddyStatus_OK; }

    /**Set/Add a buddy to the buddy list.
      */
    virtual BuddyStatus SetBuddyEx(
      const BuddyInfo & buddy
    );
    virtual bool SetBuddy(
      const BuddyInfo & buddy
    )
    { return SetBuddyEx(buddy) == BuddyStatus_OK; }

    /**Delete a buddy to the buddy list.
      */
    virtual BuddyStatus DeleteBuddyEx(
      const PURL & presentity
    );
    virtual bool DeleteBuddy(
      const PURL & presentity
    )
    { return DeleteBuddyEx(presentity) == BuddyStatus_OK; }

    /**Subscribe to buddy list.
       Send a subscription for the presence of every presentity in the current
       buddy list. This might cause multiple calls to SubscribeToPresence() or
       if the underlying protocol allows a single call for all.
      */
    virtual BuddyStatus SubscribeBuddyListEx(
      PINDEX & successfulCount,
      bool subscribe = true
    );
    virtual bool SubscribeBuddyList(
      bool subscribe = true
    )
    { PINDEX successfulCount; return SubscribeBuddyListEx(successfulCount, subscribe) == BuddyStatus_OK; }

    /**Unsubscribe to buddy list.
       Send an unsubscription for the presence of every presentity in the current
       buddy list. This might cause multiple calls to UnsubscribeFromPresence() or
       if the underlying protocol allows a single call for all.
      */
    virtual BuddyStatus UnsubscribeBuddyListEx();
    virtual bool UnsubscribeBuddyList()
    { return UnsubscribeBuddyListEx() == BuddyStatus_OK; }
  //@}
  
  
#if OPAL_HAS_IM
  /**@name Instant Messaging */
  //@{
    virtual bool SendMessageTo(
      const OpalIM & message
    );

    /**Called when Instant Message event is received.
       See OpalIM for details on events. This includes text or composition
       indication events.

        Default implementation calls m_onReceivedMessageNotifier.
      */
    virtual void OnReceivedMessage(
      const OpalIM & message ///< incoming message
    );

    typedef PNotifierTemplate<OpalIM> ReceivedMessageNotifier;
    #define PDECLARE_ReceivedMessageNotifier(cls, fn) PDECLARE_NOTIFIER2(OpalPresentity, cls, fn, OpalIM)
    #define PDECLARE_ASYNC_ReceivedMessageNotifier(cls, fn) PDECLARE_ASYNC_NOTIFIER2(OpalPresentity, cls, fn, OpalIM)
    #define PCREATE_ReceivedMessageNotifier(fn) PCREATE_NOTIFIER2(fn, OpalIM)

    /// Set the notifier for the OnPresenceChange() function.
    void SetReceivedMessageNotifier(
      const ReceivedMessageNotifier & notifier   ///< Notifier to be called by OnReceivedMessage()
    );

    void Internal_SendMessageToCommand(const OpalSendMessageToCommand & cmd);
  //@}
#endif // OPAL_HAS_IM

    /** Used to set the AOR after the presentity is created
        This override allows the descendant class to convert the internal URL into a real AOR,
        usually by changing the scheme.
      */
    virtual void SetAOR(
      const PURL & aor
    );

    OpalManager & GetManager() const { return *m_manager; }

  protected:
    OpalPresentityCommand * InternalCreateCommand(const char * cmdName);

    OpalManager        * m_manager;
    PGloballyUniqueID    m_guid;
    PURL                 m_aor;
    PStringOptions       m_attributes;

    AuthorisationRequestNotifier m_onAuthorisationRequestNotifier;
    PresenceChangeNotifier       m_onPresenceChangeNotifier;
#if OPAL_HAS_IM
    ReceivedMessageNotifier      m_onReceivedMessageNotifier;
#endif // OPAL_HAS_IM

    PAtomicBoolean m_open;
    PMutex m_notificationMutex;
    bool m_temporarilyUnavailable;
    OpalPresenceInfo::State m_localState;      ///< our presentity state
    PString m_localStateNote;                  ///< Additional note attached to the 
};


////////////////////////////////////////////////////////////////////////////////////////////////////

/**Representation of a presence identity that uses a background thread to
   process commands.
  */
class OpalPresentityWithCommandThread : public OpalPresentity
{
    PCLASSINFO(OpalPresentityWithCommandThread, OpalPresentity);

  /**@name Construction */
  //@{
  protected:
    /// Construct the presentity class that uses a command thread.
    OpalPresentityWithCommandThread();
    OpalPresentityWithCommandThread(const OpalPresentityWithCommandThread & other);

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

        Returns true if the command was queued. Note that this does not
        mean the command succeeded, only that the underlying protocol is
        processing commands, that is the backgrund thread is running
      */
    virtual bool SendCommand(
      OpalPresentityCommand * cmd   ///< Command to execute
    );
  //@}

  /**@name new functions */
  //@{
    /**Start the background thread to handle commands.
       This is typically called from the concrete classes Open() function.

       If the argument is true (the default) then the thread starts processing
       queue entries ASAP.

       If the argument is false, the thread is still created and still runs, 
       but it does not process queue entries. This allows for presenties that
       may need to allow commands to be paused, for example during initialisation
      */
    void StartThread(
      bool startQueue = true
    );

    /**Stop the background thread to handle commands.
       This is typically called from the concrete classes Close() function.
       It is also called fro the destructor to be sure the thread has stopped
       before the object is destroyed.
      */
    void StopThread();

    /**Start/resume processing of queue commands
      */
    void StartQueue(
      bool startQueue = true
    );
    
  //@}

  protected:
    void ThreadMain();

    typedef std::queue<OpalPresentityCommand *> CommandQueue;
    CommandQueue   m_commandQueue;
    PMutex         m_commandQueueMutex;
    PAtomicInteger m_commandSequence;
    PSyncPoint     m_commandQueueSync;

    bool      m_threadRunning;
    bool      m_queueRunning;
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
    PURL       m_presentity;
};

/** Macro to define the factory that creates a concrete command class.
  */
#define OPAL_DEFINE_COMMAND(command, entity, func) \
  class entity##_##command : public command \
  { \
    public: virtual void Process(OpalPresentity & presentity) { dynamic_cast<entity &>(presentity).func(*this); } \
  }; \
  static PFactory<OpalPresentityCommand>::Worker<entity##_##command> \
  s_##entity##_##command(PDefaultPFactoryKey(entity::Class())+typeid(command).name())


/** Command for subscribing to the status of another presentity.
  */
class OpalSubscribeToPresenceCommand : public OpalPresentityCommand {
  public:
    OpalSubscribeToPresenceCommand(bool subscribe = true) : m_subscribe(subscribe) { }

    bool    m_subscribe;  ///< Flag to indicate subscribing/unsubscribing
    PString m_note;       ///< Optional extra note attached to subscription request
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
    PString m_note;                                 ///< Optional extra note attached to subscription request
};


/** Command for adusting our current status.
    This action typically causes all presentities to be notified of the
    change. The mechanism by which this happens is dependent on the concrete
    OpalPresentity class and it's underlying protocol.
  */
class OpalSetLocalPresenceCommand : public OpalPresentityCommand, public OpalPresenceInfo {
  public:
    OpalSetLocalPresenceCommand(State state = NoPresence) : OpalPresenceInfo(state) { }
};


#if OPAL_HAS_IM
/** Command for sending an IM 
  */
class OpalSendMessageToCommand : public OpalPresentityCommand
{
  public:
    OpalSendMessageToCommand() { }

    OpalIM m_message;
};
#endif // OPAL_HAS_IM


///////////////////////////////////////////////////////////////////////////////

// Include concrete classes here so the factories are initialised
#if OPAL_SIP && OPAL_PTLIB_EXPAT
PFACTORY_LOAD(SIP_Presentity);
#endif


#endif  // OPAL_IM_PRES_ENT_H
