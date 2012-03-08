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

#include <ptclib/url.h>
#include <ptclib/threadpool.h>

#include <opal/transports.h>

class OpalIM : public PObject
{
  public:
    OpalIM();

    enum Type {
      Text,
      CompositionIndication_Idle,   // aka RFC 3994
      CompositionIndication_Active, // aka RFC 3994
      Disposition                   // aka RFC 5438
    } m_type;

    PURL    m_to;
    PURL    m_from;
    PString m_fromName;
    PString m_mimeType;
    PString m_body;
    PString m_conversationId;

    OpalTransportAddress m_fromAddr;
    OpalTransportAddress m_toAddr;

    PAtomicInteger::IntegerType m_messageId;

    static PAtomicInteger::IntegerType GetNextMessageId();
};

#if OPAL_HAS_IM

#include <opal/mediastrm.h>
#include <im/rfc4103.h>

class OpalIMMediaType : public OpalMediaTypeDefinition 
{
  public:
    OpalIMMediaType(
      const char * mediaType, ///< name of the media type (audio, video etc)
      const char * sdpType    ///< name of the SDP type 
    )
      : OpalMediaTypeDefinition(mediaType, sdpType, 0, OpalMediaType::DontOffer)
    { }

    PString GetRTPEncoding() const { return PString::Empty(); }
    RTP_UDP * CreateRTPSession(OpalRTPConnection & , unsigned , bool ) { return NULL; }
    virtual bool UsesRTP() const { return false; }
};

/////////////////////////////////////////////////////////////////////

class OpalIMManager;
class OpalPresentity;

class OpalIMContext : public PSafeObject
{
  PCLASSINFO(OpalIMContext, PSafeObject);

  public:
    friend class OpalIMManager;

    OpalIMContext();
    ~OpalIMContext();

    static PSafePtr<OpalIMContext> Create(
      OpalManager & manager, 
      const PURL & localURL, 
      const PURL & remoteURL
    );

    static PSafePtr<OpalIMContext> Create(
      OpalManager & manager,
      PSafePtr<OpalConnection> conn
    );

    static PSafePtr<OpalIMContext> Create(
      OpalManager & manager,
      PSafePtr<OpalPresentity> presentity,
      const PURL & remoteURL
    );

    enum SentStatus {
      SentOK,
      SentPending,
      SentAccepted,
      SentUnacceptableContent,
      SentInvalidContent,
      SentConnectionClosed,
      SentNoTransport,
      SentNoAnswer,
      SentDestinationUnknown,
      SentFailedGeneric,
    };

    // send text message in this conversation
    virtual SentStatus Send(OpalIM * message);
    virtual SentStatus SendCompositionIndication(bool active = true);

    /// Called when an outgoing message has been delivered for this context
    /// Default implementation calls MessageSentNotifier, if set
    struct MessageSentInfo {
      PAtomicInteger::IntegerType messageId;
      OpalIMContext::SentStatus status;
    };
    virtual void OnMessageSent(const MessageSentInfo & info);

    typedef PNotifierTemplate<const MessageSentInfo &> MessageSentNotifier;
    #define PDECLARE_MessageSentNotifier(cls, fn) PDECLARE_NOTIFIER2(OpalIMContext, cls, fn, const MessageSentInfo &)
    #define PCREATE_MessageSentNotifier(fn) PCREATE_NOTIFIER2(fn, const MessageSentInfo &)

    /// Set the notifier for the OnMessageSent() function.
    void SetMessageSentNotifier(
      const MessageSentNotifier & notifier   ///< Notifier to be called by OnIncomingIM()
    );

    /// Called when an incoming message arrives for this context
    /// Default implementation calls IncomingIMNotifier, if set, else returns true
    virtual SentStatus OnIncomingIM(OpalIM & message);

    typedef PNotifierTemplate<const OpalIM &> IncomingIMNotifier;
    #define PDECLARE_IncomingIMNotifier(cls, fn) PDECLARE_NOTIFIER2(OpalIMContext, cls, fn, const OpalIM &)
    #define PCREATE_IncomingIMNotifier(fn) PCREATE_NOTIFIER2(fn, const OpalIM &)

    /// Set the notifier for the OnIncomingMessage() function.
    void SetIncomingIMNotifier(
      const IncomingIMNotifier & notifier   ///< Notifier to be called by OnIncomingIM()
    );

    /// Called when the remote composition indication changes state for this context
    /// Default implementation calls IncomingIMNotifier, if set, else returns true
    virtual void OnCompositionIndicationChanged(const PString & state);

    typedef PNotifierTemplate<const PString &> CompositionIndicationChangedNotifier;
    #define PDECLARE_CompositionIndicationChangedNotifier(cls, fn) PDECLARE_NOTIFIER2(OpalIMContext, cls, fn, const PString &)
    #define PCREATE_CompositionIndicationChangedNotifier(fn) PCREATE_NOTIFIER2(fn, const PString &)

    /// Set the notifier for the OnIncomingMessage() function.
    void SetCompositionIndicationChangedNotifier(
      const CompositionIndicationChangedNotifier & notifier   ///< Notifier to be called by OnIncomingIM()
    );

    virtual bool CheckContentType(const PString & contentType) const;
    virtual PStringArray GetContentTypes() const;

    // start of internal functions

    PString GetID() const          { return m_id; }
    void SetID(const PString & id) { m_id = id; }
    PString GetKey() const         { return m_key; }
    PString GetLocalURL() const    { return m_localURL; }
    PString GetRemoteURL() const   { return m_remoteURL; }

  /**@name Attributes */
  //@{
    ///< Get the attributes for this presentity.
    PStringOptions & GetAttributes() { return m_attributes; }
    const PStringOptions & GetAttributes() const { return m_attributes; }

    virtual bool OnNewIncomingIM();

    virtual bool AddIncomingIM(OpalIM * message);

    virtual void OnCompositionIndicationTimeout();

    OpalIM * GetIncomingMessage();

    virtual void InternalOnMessageSent(const MessageSentInfo & info);

    static PString CreateKey(const PString & from, const PString & to);

    void ResetLastUsed();

  protected:
    virtual SentStatus InternalSend();
    virtual SentStatus InternalSendOutsideCall(OpalIM * message);
    virtual SentStatus InternalSendInsideCall(OpalIM * message);

    PMutex m_notificationMutex;
    IncomingIMNotifier                   m_incomingMessageNotifier;
    MessageSentNotifier                  m_messageSentNotifier;
    CompositionIndicationChangedNotifier m_compositionIndicationChangedNotifier;

    OpalManager  * m_manager;
    PStringOptions m_attributes;

    PSafePtr<OpalConnection> m_connection;
    PSafePtr<OpalPresentity> m_presentity;

    PMutex m_incomingMessagesMutex;
    PQueue<OpalIM> m_incomingMessages;

    PMutex m_outgoingMessagesMutex;
    OpalIM * m_currentOutgoingMessage;
    PQueue<OpalIM> m_outgoingMessages;

    PMutex m_lastUsedMutex;
    PTime m_lastUsed;

  private:    
    PString m_id, m_localURL, m_remoteURL, m_key;

};

class OpalConnectionIMContext : public OpalIMContext
{
  public:
    OpalConnectionIMContext();
};

class OpalPresentityIMContext : public OpalIMContext
{
  public:
    OpalPresentityIMContext();
};

/////////////////////////////////////////////////////////////////////

class OpalIMManager : public PObject
{
  public:
    OpalIMManager(OpalManager & manager);
    ~OpalIMManager();

    class IM_Work;

    OpalIMContext::SentStatus OnIncomingMessage(OpalIM * im, PString & conversationId, PSafePtr<OpalConnection> conn = NULL);
    void OnCompositionIndicationTimeout(const PString & conversationId);

    void AddContext(PSafePtr<OpalIMContext> context);
    void RemoveContext(OpalIMContext * context);

    void GarbageCollection();

    PSafePtr<OpalIMContext> FindContextByIdWithLock(
      const PString & key,
      PSafetyMode mode = PSafeReadWrite
    );

    PSafePtr<OpalIMContext> FindContextByNamesWithLock(
      const PString & local, 
      const PString & remote, 
      PSafetyMode mode = PSafeReadWrite
    );

    PSafePtr<OpalIMContext> FindContextForMessageWithLock(OpalIM & im, OpalConnection * conn = NULL);

    typedef PNotifierTemplate<OpalIMContext &> NewConversationNotifier;
    #define PDECLARE_NewConversationNotifier(cls, fn) PDECLARE_NOTIFIER2(OpalIMManager, cls, fn, OpalIMContext &)
    #define PCREATE_NewConversationNotifier(fn) PCREATE_NOTIFIER2(fn, OpalIMContext &)

    class NewConversationCallBack : public PObject {
      public:
        NewConversationNotifier m_notifier;
        PString m_scheme;
    };

    void AddNotifier(const NewConversationNotifier & notifier, const PString & scheme);
    bool RemoveNotifier(const NewConversationNotifier & notifier, const PString & scheme);


    // thread pool declarations
    class IM_Work
    {
      public:
        IM_Work(OpalIMManager & mgr, const PString & conversationId);
        virtual ~IM_Work();

        virtual void Work() = 0;

        OpalIMManager & m_mgr;
        PString m_conversationId;
    };

    class NewIncomingIM_Work : public IM_Work 
    {
      public:
        NewIncomingIM_Work(OpalIMManager & mgr, const PString & conversationId)
          : IM_Work(mgr, conversationId)
        { }
        virtual void Work()
        { m_mgr.InternalOnNewIncomingIM(m_conversationId); }
    };

    class NewConversation_Work : public IM_Work 
    {
      public:
        NewConversation_Work(OpalIMManager & mgr, const PString & conversationId)
          : IM_Work(mgr, conversationId)
        { }
        virtual void Work()
        { m_mgr.InternalOnNewConversation(m_conversationId); }
    };

    class MessageSent_Work : public IM_Work 
    {
      public:
        MessageSent_Work(OpalIMManager & mgr, const PString & conversationId, const OpalIMContext::MessageSentInfo & info)
          : IM_Work(mgr, conversationId)
          , m_info(info)
        { }
        virtual void Work()
        { m_mgr.InternalOnMessageSent(m_conversationId, m_info); }

        OpalIMContext::MessageSentInfo m_info;
    };

    class CompositionIndicationTimeout_Work : public IM_Work 
    {
      public:
        CompositionIndicationTimeout_Work(OpalIMManager & mgr, const PString & conversationId)
          : IM_Work(mgr, conversationId)
        { }
        virtual void Work()
        { m_mgr.InternalOnCompositionIndicationTimeout(m_conversationId); }
    };


    void AddWork(IM_Work * work);
    virtual void InternalOnNewConversation(const PString & conversation);
    virtual void InternalOnNewIncomingIM(const PString & conversation);
    virtual void InternalOnMessageSent(const PString & conversation, const OpalIMContext::MessageSentInfo & info);
    virtual void InternalOnCompositionIndicationTimeout(const PString & conversationId);

  protected:
    PQueuedThreadPool<IM_Work> m_imThreadPool;

    PTime m_lastGarbageCollection;
    OpalManager & m_manager;
    bool m_deleting;
    typedef PSafeDictionary<PString, OpalIMContext> ContextsByConversationId;
    ContextsByConversationId m_contextsByConversationId;

    PMutex m_contextsByNamesMutex;
    typedef std::multimap<std::string, PString> ContextsByNames;
    ContextsByNames m_contextsByNames;

    PMutex m_notifierMutex;
    PList<NewConversationCallBack> m_callbacks;
};

/////////////////////////////////////////////////////////////////////

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

  protected:
    virtual void InternalClose() { }
};

#endif // OPAL_HAS_IM

#endif // OPAL_IM_IM_H
