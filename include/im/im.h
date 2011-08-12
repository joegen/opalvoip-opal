/*
 * im_mf.h
 *
 * Media formats for Instant Messaging
 *
 * Open Phone Abstraction Library (OPAL)
 *
 * Copyright (c) 2008 Post Increment
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

#ifndef OPAL_IM_IM_H
#define OPAL_IM_IM_H

#include <ptlib.h>
#include <opal/buildopts.h>

#if OPAL_HAS_IM

#include <ptclib/url.h>
#include <ptclib/threadpool.h>

#include <opal/transports.h>
#include <opal/mediastrm.h>
#include <im/rfc4103.h>


class OpalPresentity;


/////////////////////////////////////////////////////////////////////

/** Class representing an Instant Message event.
    Whenever something happens with an IM conversation, this encapsulates all
    the information relevant to the event. For example the text of the instant
    message would arrive this way, and also status indication such as
    composition indication.
  */
class OpalIM : public PObject
{
  public:
    OpalIM();

    PString m_conversationId;         ///< Identifier for the conversation of messages

    PURL                  m_to;       ///< URL for destination of message
    OpalTransportAddress  m_toAddr;   ///< Physical address for destination of message
    PURL                  m_from;     ///< URL for source of message
    OpalTransportAddress  m_fromAddr; ///< Physical address for source of message
    PString               m_fromName; ///< Alias (human readable) name for source of message
    PStringToString       m_bodies;   ///< Map of MIME types to body text, e.g. "text/plain", "Hello Bob!"

    PAtomicInteger::IntegerType m_messageId;  /**< Unique identifier for OpalIM instance for matching
                                                   multiple simultaneous message states. */

    static PAtomicInteger::IntegerType GetNextMessageId();
};


/////////////////////////////////////////////////////////////////////

/** Class representing an Instant Messaging "conversation".
    This keeps the context for Instant Messages between two entities. It is an
    abstract class whose concrete classes are dependant on the individual
    protocol being used for the messages. The URL "scheme" field is usually
    used to distinguish the protocol.
  */
class OpalIMContext : public PSafeObject
{
  PCLASSINFO(OpalIMContext, PSafeObject);

  /**@name Construction */
  //@{
  protected:
    /// Construct base for context
    OpalIMContext();

  public:
    /// Destroy context
    ~OpalIMContext();

    /**Create a Instant Mesaging context based on URL scheme.
       This uses a factory to create an approriate concrete type for the
       protocol indicated by the scheme of the \p remoteURL.

       The \p localURL may be empty and will be derived from the \p remoteURL
       and the default user name from the manager.
      */
    static PSafePtr<OpalIMContext> Create(
      OpalManager & manager,    ///< Manager for OPAL
      const PURL & localURL,    ///< Local URL for conversation
      const PURL & remoteURL,   ///< Remote URL for conversation
      bool byRemote = false     ///< Created by remote entity
    );

    /**Create a Instant Mesaging context based on a connection.
       This uses a factory to create an approriate concrete type for the
       protocol used by the connection. Note that only SIPConnection is
       supported at this time.
      */
    static PSafePtr<OpalIMContext> Create(
      PSafePtr<OpalConnection> connection,  ///< Connection to begin conversation in.
      bool byRemote = false                 ///< Created by remote entity
    );

    /**Create a Instant Mesaging context based on a presentity.
       This uses a factory to create an approriate concrete type for the
       protocol used by the presentity.
      */
    static PSafePtr<OpalIMContext> Create(
      PSafePtr<OpalPresentity> presentity,    ///< Presentity for local end of conversation
      const PURL & remoteURL,                 ///< Remote URL for conversation
      bool byRemote = false                   ///< Created by remote entity
    );

    /**Close the context (conversation)
      */
    virtual void Close();
  //@}


  /**@name Message transmit */
  //@{
    /**Disposition of message transmission.
       Get the status of the sent message throughout its life, including how it
       is handled by teh network, and how it is handled by the remote entity,
       such as described in RFC5438.
      */
    enum MessageDisposition {
      DispositionPending,     ///< Indicates mesage is on it's way
      DispositionAccepted,    ///< Indicates message was accepted by remote system
      DeliveryOK,             ///< Indicates message delivered to remote entity (RFC5438)
      DisplayConfirmed,       ///< Indicates message has been confirmed as displayed (RFC5438)
      ProcessedNotification,  ///< Indicates message is being processed (RFC5438)
      StorageNotification,    ///< Indicates message has been stored for forwarding (RFC5438)

      ///< All dispositions greater than this are errors
      DispositionErrors,
      GenericError,           ///< Generic error
      UnacceptableContent,    ///< The message content (MIME type) is unacceptable
      InvalidContent,         ///< The message content was invalid (e.g. bad XML)
      DestinationUnknown,     ///< Cannot find the remote party.
      DestinationUnavailable, ///< Remote party exists but is currently unavailable.
      TransmissionTimeout,    ///< Message to remote timed out waiting for confirmation.
      TransportFailure,       ///< Underlying network transport failed
      ConversationClosed,     ///< The conversation with remote was closed
      UnsupportedFeature,     ///< Feature is not supported
      DeliveryFailed,         ///< Indicates message could not be delivered to remote entity (RFC5438)
    };

    /** Send message in this conversation.
        This is generally asynchronous and will return quickly with the
        message procesing happening inthe background.

        The eventual disposition of the message transmission is indicated via
        the OnMessageDisposition() function and it's notifier.

        The \p message parameter should be allocated by the caller and will be
        destroyed by the context when it is finished with.
      */
    virtual MessageDisposition Send(
      OpalIM * message    ///< Message to be sent
    );

    /**Information on the message disposition.
      */
    struct DispositionInfo {
      PAtomicInteger::IntegerType m_messageId;    ///< Id of message disposition is of
      MessageDisposition          m_disposition;  ///< Disposition status
    };

    /**Callback indicating the dispostion of a messagesent via Send().
       The default behaviour checks for a notifier and calls that if set.
      */
    virtual void OnMessageDisposition(
      const DispositionInfo & info    ///< Information on the message disposition
    );

    /// Type for disposition notifiers
    typedef PNotifierTemplate<DispositionInfo> MessageDispositionNotifier;

    /// Macro to declare correctly typed disposition notifier
    #define PDECLARE_MessageDispositionNotifier(cls, fn) PDECLARE_NOTIFIER2(OpalIMContext, cls, fn, OpalIMContext::DispositionInfo)

    /// Macro to declare correctly typed asynchronous disposition notifier
    #define PDECLARE_ASYNC_MessageDispositionNotifier(cls, fn) PDECLARE_NOTIFIER2(OpalIMContext, cls, fn, OpalIMContext::DispositionInfo)

    /// Macro to create correctly typed disposition notifier
    #define PCREATE_MessageDispositionNotifier(fn) PCREATE_NOTIFIER2(fn, OpalIMContext::DispositionInfo)

    /// Set the notifier for the OnMessageDisposition() function.
    void SetMessageDispositionNotifier(
      const MessageDispositionNotifier & notifier   ///< Notifier to be called by protocol
    );
  //@}


  /**@name Message receipt */
  //@{
    /** Called when an incoming message arrives for this context.
        Default implementation checks for valid MIME content and then calls
        the notifier, if set. If no notifier is set, then the
        OpalManager::OnMessageReceived() function is called.
      */
    virtual MessageDisposition OnMessageReceived(
      const OpalIM & message    ///< Received message
    );

    /// Type for message received notifiers
    typedef PNotifierTemplate<OpalIM> MessageReceivedNotifier;

    /// Macro to declare correctly typed message received notifier
    #define PDECLARE_MessageReceivedNotifier(cls, fn) PDECLARE_NOTIFIER2(OpalIMContext, cls, fn, OpalIM)

    /// Macro to declare correctly typed asynchronous message received notifier
    #define PDECLARE_ASYNC_MessageReceivedNotifier(cls, fn) PDECLARE_ASYNC_NOTIFIER2(OpalIMContext, cls, fn, OpalIM)

    /// Macro to create correctly typed message received notifier
    #define PCREATE_MessageReceivedNotifier(fn) PCREATE_NOTIFIER2(fn, OpalIM)

    /// Set the notifier for the OnMessageReceived() function.
    void SetMessageReceivedNotifier(
      const MessageReceivedNotifier & notifier   ///< Notifier to be called by protocol
    );
  //@}


  /**@name Message composition */
  //@{
    static const PCaselessString & CompositionIndicationActive();  ///< CompositionIndication active status
    static const PCaselessString & CompositionIndicationIdle();    ///< CompositionIndication idle status

    /**Composition information.
      */
    struct CompositionInfo
    {
      PString m_state;
      PString m_contentType;

      CompositionInfo(
        const PString & state = CompositionIndicationIdle(),
        const PString & contentType = PString::Empty()
      ) : m_state(state)
        , m_contentType(contentType)
      { }
    };

    /**Send a composition indication to remote.
       The text is usually either CompositionIndicationActive() or
       CompositionIndicationIdle(), howver other extension values may be possible.
      */
    virtual bool SendCompositionIndication(
      const CompositionInfo & info  ///< Composition information
    );

    /** Called when the remote composition indication changes state.
       The default behaviour checks for a notifier and calls that if set.
      */
    virtual void OnCompositionIndication(
      const CompositionInfo & info     ///< New composition state information
    );

    /// Type for composition indication notifiers
    typedef PNotifierTemplate<CompositionInfo> CompositionIndicationNotifier;

    /// Macro to declare correctly typed composition indication notifier
    #define PDECLARE_CompositionIndicationNotifier(cls, fn) PDECLARE_NOTIFIER2(OpalIMContext, cls, fn, OpalIMContext::CompositionInfo)

    /// Macro to declare correctly typed asynchronous composition indication notifier
    #define PDECLARE_ASYNC_CompositionIndicationNotifier(cls, fn) PDECLARE_ASYNC_NOTIFIER2(OpalIMContext, cls, fn, OpalIMContext::CompositionInfo)

    /// Macro to create correctly typed composition indication notifier
    #define PCREATE_CompositionIndicationNotifier(fn) PCREATE_NOTIFIER2(fn, OpalIMContext::CompositionInfo)

    /// Set the notifier for the OnCompositionIndication() function.
    void SetCompositionIndicationNotifier(
      const CompositionIndicationNotifier & notifier   ///< Notifier to be called by protocol
    );
  //@}


  /**@name Support functions */
  //@{
    /// Check that the context type is valid for protocol
    virtual bool CheckContentType(
      const PString & contentType   ///< MIME Content type to check
    ) const;

    /// Return array of all valid content types.
    virtual PStringArray GetContentTypes() const;

    /// Calculate a key based on the from an to addresses
    static PString CreateKey(const PString & from, const PString & to);
  //@}


  /**@name Member variables */
  //@{
    /// Get conversation ID
    const PString & GetID() const          { return m_conversationId; }

    /// Get key for context based on to/from addresses.
    const PString & GetKey() const         { return m_key; }

    /// Get remote URL for conversation.
    const PString & GetRemoteURL() const   { return m_remoteURL; }

    /// Get remote display name for conversation.
    const PString & GetRemoteName() const  { return m_remoteName; }

    /// Get local URL for conversation.
    const PString & GetLocalURL() const    { return m_localURL; }

    /// Get local display for conversation.
    const PString & GetLocalName() const    { return m_localName; }

    /// Set local display for conversation.
    void SetLocalName(const PString & name) { m_localName = name; }

    /// Get the attributes for this presentity.
    PStringOptions & GetAttributes() { return m_attributes; }
    const PStringOptions & GetAttributes() const { return m_attributes; }
  //@}

  protected:
    void ResetLastUsed();

    virtual MessageDisposition InternalSend();
    virtual MessageDisposition InternalSendOutsideCall(OpalIM & message);
    virtual MessageDisposition InternalSendInsideCall(OpalIM & message);
    virtual void InternalOnMessageSent(const DispositionInfo & info);

    PMutex                        m_notificationMutex;
    MessageDispositionNotifier    m_messageDispositionNotifier;
    MessageReceivedNotifier       m_messageReceivedNotifier;
    CompositionIndicationNotifier m_compositionIndicationNotifier;

    OpalManager  * m_manager;
    PStringOptions m_attributes;

    PSafePtr<OpalConnection> m_connection;
    PSafePtr<OpalPresentity> m_presentity;

    PMutex         m_outgoingMessagesMutex;
    OpalIM       * m_currentOutgoingMessage;
    PQueue<OpalIM> m_outgoingMessages;

    PMutex m_lastUsedMutex;
    PTime  m_lastUsed;

    PString m_conversationId;
    PString m_localURL;
    PString m_localName;
    PString m_remoteURL;
    PString m_remoteName;
    PString m_key;

  friend class OpalIMManager;
};


/////////////////////////////////////////////////////////////////////

/**Manager for IM contexts.
 */
class OpalIMManager : public PObject
{
  public:
  /**@name Construction */
  //@{
    OpalIMManager(OpalManager & manager);
    ~OpalIMManager();
  //@}


  /**@name Context management */
  //@{
    /** Add a new context.
        Generally only used internally.
        This will call notifiers indicating converstaion start.
      */
    void AddContext(PSafePtr<OpalIMContext> context, bool byRemote);

    /** Remove a new context.
        Generally only used internally.
        This will call notifiers indicating converstaion end.
      */
    void RemoveContext(OpalIMContext * context, bool byRemote);

    /** Look for old conversations and remove them.
        Generally only used internally.
        This will call notifiers indicating converstaion end.
      */
    bool GarbageCollection();

    /** Find a context given a key.
        Generally only used internally.
      */
    PSafePtr<OpalIMContext> FindContextByIdWithLock(
      const PString & key,
      PSafetyMode mode = PSafeReadWrite
    );

    /** Find a context given a from/to addresses.
        Generally only used internally.
      */
    PSafePtr<OpalIMContext> FindContextByNamesWithLock(
      const PString & local, 
      const PString & remote, 
      PSafetyMode mode = PSafeReadWrite
    );

    /** Find a context given a message.
        Generally only used internally.
      */
    PSafePtr<OpalIMContext> FindContextForMessageWithLock(
      OpalIM & im,
      OpalConnection * conn = NULL
    );

    /**Shut down all conversations.
      */
    void ShutDown();
  //@}


  /**@name Conversation notification */
  //@{
    /**Information in notification on converstaion state change.
      */
    struct ConversationInfo
    {
      PSafePtr<OpalIMContext> m_context;  ///< Context opening/closing
      bool                    m_opening;  ///< Opening or closing conversation
      bool                    m_byRemote; ///< Operation is initiated by remote user or local user
    };

    /** Called when a context state changes, e.g. openign or closing.
        Default implementation calls the notifier, if set. If no notifier is
        set, then the OpalManager::OnConversation() function is called.
      */
    virtual void OnConversation(
      const ConversationInfo & info
    );

    /// Type for converstaion notifiers
    typedef PNotifierTemplate<ConversationInfo> ConversationNotifier;

    /// Macro to declare correctly typed converstaion notifier
    #define PDECLARE_ConversationNotifier(cls, fn) PDECLARE_NOTIFIER2(OpalIMContext, cls, fn, OpalIMManager::ConversationInfo)

    /// Macro to declare correctly typed asynchronous converstaion notifier
    #define PDECLARE_ASYNC_ConversationNotifier(cls, fn) PDECLARE_ASYNC_NOTIFIER2(OpalIMContext, cls, fn, OpalIMManager::ConversationInfo)

    /// Macro to create correctly typed converstaion notifier
    #define PCREATE_ConversationNotifier(fn) PCREATE_NOTIFIER2(fn, OpalIMManager::ConversationInfo)

    /** Set the notifier for the OnConversation() function.
        The notifier can be called only for when a specific scheme is being
        used. If \p scheme is "*" then all scemes will call that notifier.

        More than one notifier can be applied and all are called.
      */
    void AddNotifier(
      const ConversationNotifier & notifier,  ///< New notifier to add
      const PString & scheme                  ///< Scheme to be notified about
    );

    /** Remove notifier for the OnConversation() function.
      */
    bool RemoveNotifier(const ConversationNotifier & notifier, const PString & scheme);
  //@}


    OpalIMContext::MessageDisposition OnMessageReceived(
      OpalIM & message,
      OpalConnection * connnection,
      PString & errorInfo
    );

  protected:
    OpalManager & m_manager;
    typedef PSafeDictionary<PString, OpalIMContext> ContextsByConversationId;
    ContextsByConversationId m_contextsByConversationId;

    PMutex m_contextsByNamesMutex;
    typedef std::multimap<PString, PString> ContextsByNames;
    ContextsByNames m_contextsByNames;

    PMutex m_notifierMutex;
    typedef std::multimap<PString, ConversationNotifier> ConversationMap;
    ConversationMap m_notifiers;

    PTime m_lastGarbageCollection;
    bool m_deleting;
};

/////////////////////////////////////////////////////////////////////

class OpalIMMediaType : public OpalMediaTypeDefinition 
{
  public:
    OpalIMMediaType(
      const char * mediaType, ///< name of the media type (audio, video etc)
      const char * sdpType    ///< name of the SDP type 
    )
      : OpalMediaTypeDefinition(mediaType, sdpType, 0, OpalMediaType::DontOffer)
    { }
};


class RTP_IMFrame : public RTP_DataFrame
{
  public:
    RTP_IMFrame();
    RTP_IMFrame(const PString & contentType);
    RTP_IMFrame(const PString & contentType, const T140String & content);
    RTP_IMFrame(const BYTE * data, PINDEX len, PBoolean dynamic = true);

    void SetContentType(const PString & contentType);
    PString GetContentType() const;

    void SetContent(const T140String & text);
    bool GetContent(T140String & text) const;

    PString AsString() const { return PString((const char *)GetPayloadPtr(), GetPayloadSize()); }
};


class OpalIMMediaStream : public OpalMediaStream
{
  public:
    OpalIMMediaStream(
      OpalConnection & conn,
      const OpalMediaFormat & mediaFormat, ///<  Media format for stream
      unsigned sessionID,                  ///<  Session number for stream
      bool isSource                        ///<  Is a source stream
    );

    virtual PBoolean IsSynchronous() const         { return false; }
    virtual PBoolean RequiresPatchThread() const   { return false; }

    bool ReadPacket(RTP_DataFrame & packet);
    bool WritePacket(RTP_DataFrame & packet);
};

#endif // OPAL_HAS_IM

#endif // OPAL_IM_IM_H
