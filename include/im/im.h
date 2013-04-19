/*
 * im.h
 *
 * Instant Messaging classes
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
#include <opal_config.h>

#if OPAL_HAS_IM

#include <ptclib/url.h>
#include <opal//transports.h>


class OpalCall;
class OpalMediaFormat;
class OpalIMEndPoint;


/////////////////////////////////////////////////////////////////////

#if OPAL_HAS_SIPIM
  #define OPAL_SIPIM             "SIP-IM"
  #define OpalSIPIM              GetOpalSIPIM()
  extern const OpalMediaFormat & GetOpalSIPIM();
#endif

#if OPAL_HAS_RFC4103
  #define OPAL_T140              "T.140"
  #define OpalT140               GetOpalT140()
  extern const OpalMediaFormat & GetOpalT140();
#endif

#if OPAL_HAS_MSRP
  #define OPAL_MSRP              "MSRP"
  #define OpalMSRP               GetOpalMSRP()
  extern const OpalMediaFormat & GetOpalMSRP();
#endif

#define OPAL_IM_MEDIA_TYPE_PREFIX "im-"


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

    /**Information in notification on conversation state change.
      */
    struct ConversationInfo
    {
      PSafePtr<OpalIMContext> m_context;  ///< Context opening/closing
      bool                    m_opening;  ///< Opening or closing conversation
      bool                    m_byRemote; ///< Operation is initiated by remote user or local user
    };

    /**Open the context (conversation)
       Default behaviour simply returns true.
      */
    virtual bool Open(
      bool byRemote   ///< Context was created by remote (incoming message)
    );

    /**Close the context (conversation)
       Default behaviour removes the context from the OpalIMEndPoint
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
      PString                     m_conversationId; ///< Conversation ID to get OpalIMContext
      PAtomicInteger::IntegerType m_messageId;      ///< Id of message disposition is of
      MessageDisposition          m_disposition;    ///< Disposition status
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
      PString m_conversationId; ///< Conversation ID to get OpalIMContext
      PString m_state;          ///< New state, usually CompositionIndicationActive() or CompositionIndicationIdle()
      PString m_contentType;    ///< MIME type of composed message

      CompositionInfo(
        const PString & id,
        const PString & state,
        const PString & contentType = PString::Empty()
      ) : m_conversationId(id)
        , m_state(state)
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
    const PURL & GetRemoteURL() const      { return m_remoteURL; }

    /// Get remote display name for conversation.
    const PString & GetRemoteName() const  { return m_remoteName; }

    /// Get local URL for conversation.
    const PURL & GetLocalURL() const       { return m_localURL; }

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

    OpalIMEndPoint * m_endpoint;
    PStringOptions   m_attributes;

    PSafePtr<OpalCall> m_call;
    bool               m_weStartedCall;

    PMutex                        m_notificationMutex;
    MessageDispositionNotifier    m_messageDispositionNotifier;
    MessageReceivedNotifier       m_messageReceivedNotifier;
    CompositionIndicationNotifier m_compositionIndicationNotifier;

    PMutex         m_outgoingMessagesMutex;
    OpalIM       * m_currentOutgoingMessage;
    PQueue<OpalIM> m_outgoingMessages;

    PMutex m_lastUsedMutex;
    PTime  m_lastUsed;

    PString m_conversationId;
    PURL    m_localURL;
    PString m_localName;
    PURL    m_remoteURL;
    PString m_remoteName;
    PString m_key;

  friend class OpalIMEndPoint;
  friend class OpalIMConnection;
};


#endif // OPAL_HAS_IM

#endif // OPAL_IM_IM_H
