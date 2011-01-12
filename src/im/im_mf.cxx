/*
 * im_mf.cxx
 *
 * Instant Messaging Media Format descriptions
 *
 * Open Phone Abstraction Library
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
 * The Original Code is Open Phone Abstraction Library
 *
 * The Initial Developer of the Original Code is Post Increment
 *
 * Contributor(s): ______________________________________.
 *
 * $Revision$
 * $Author$
 * $Date$
 */

#include <ptlib.h>
#include <opal/buildopts.h>

#if OPAL_HAS_IM

#include <opal/mediafmt.h>
#include <opal/connection.h>
#include <opal/patch.h>
#include <im/im.h>
#include <im/msrp.h>
#include <im/sipim.h>
#include <im/rfc4103.h>
#include <rtp/rtp.h>

#define new PNEW

/////////////////////////////////////////////////////////////////////////////

#if OPAL_HAS_MSRP

OPAL_INSTANTIATE_MEDIATYPE(msrp, OpalMSRPMediaType);

/////////////////////////////////////////////////////////////////////////////


#define DECLARE_MSRP_ENCODING(title, encoding) \
class IM##title##OpalMSRPEncoding : public OpalMSRPEncoding \
{ \
}; \
static PFactory<OpalMSRPEncoding>::Worker<IM##title##OpalMSRPEncoding> worker_##IM##title##OpalMSRPEncoding(encoding, true); \

/////////////////////////////////////////////////////////////////////////////

DECLARE_MSRP_ENCODING(Text, "text/plain");
DECLARE_MSRP_ENCODING(CPIM, "message/cpim");
DECLARE_MSRP_ENCODING(HTML, "message/html");

const OpalMediaFormat & GetOpalMSRP() 
{ 
  static class IMMSRPMediaFormat : public OpalMediaFormat { 
    public: 
      IMMSRPMediaFormat() 
        : OpalMediaFormat(OPAL_MSRP, 
                          "msrp", 
                          RTP_DataFrame::MaxPayloadType, 
                          "+", 
                          false,  
                          1440, 
                          512, 
                          0, 
                          1000)            // as defined in RFC 4103 - good as anything else   
      { 
        PFactory<OpalMSRPEncoding>::KeyList_T types = PFactory<OpalMSRPEncoding>::GetKeyList();
        PFactory<OpalMSRPEncoding>::KeyList_T::iterator r;

        PString acceptTypes;
        for (r = types.begin(); r != types.end(); ++r) {
          if (!acceptTypes.IsEmpty())
            acceptTypes += " ";
          acceptTypes += *r;
        }

        OpalMediaOptionString * option = new OpalMediaOptionString("Accept Types", false, acceptTypes);
        option->SetMerge(OpalMediaOption::AlwaysMerge);
        AddOption(option);

        option = new OpalMediaOptionString("Path", false, "");
        option->SetMerge(OpalMediaOption::MaxMerge);
        AddOption(option);
      } 
  } const f; 
  return f; 
} 

#endif // OPAL_HAS_MSRP


//////////////////////////////////////////////////////////////////////////////////////////

#if OPAL_HAS_SIPIM

OPAL_INSTANTIATE_MEDIATYPE2(sipim, "sip-im", OpalSIPIMMediaType);

const OpalMediaFormat & GetOpalSIPIM() 
{ 
  static class IMSIPMediaFormat : public OpalMediaFormat { 
    public: 
      IMSIPMediaFormat() 
        : OpalMediaFormat(OPAL_SIPIM, 
                          "sip-im", 
                          RTP_DataFrame::MaxPayloadType, 
                          "+", 
                          false,  
                          1440, 
                          512, 
                          0, 
                          1000)     // as defined in RFC 4103 - good as anything else
      { 
        OpalMediaOptionString * option = new OpalMediaOptionString("URL", false, "");
        option->SetMerge(OpalMediaOption::NoMerge);
        AddOption(option);
      } 
  } const f; 
  return f; 
} 

#endif // OPAL_HAS_SIPIM

//////////////////////////////////////////////////////////////////////////////////////////

const OpalMediaFormat & GetOpalT140() 
{ 
  static class T140MediaFormat : public OpalMediaFormat { 
    public: 
      T140MediaFormat() 
        : OpalMediaFormat(OPAL_T140, 
                          "t140", 
                          RTP_DataFrame::DynamicBase, 
                          "text", 
                          false,  
                          1440, 
                          512, 
                          0, 
                          1000)    // as defined in RFC 4103
      { 
      } 
  } const f; 
  return f; 
} 

OPAL_INSTANTIATE_MEDIATYPE(t140, OpalT140MediaType);

//////////////////////////////////////////////////////////////////////////////////////////

RTP_IMFrame::RTP_IMFrame(const BYTE * data, PINDEX len, bool dynamic)
  : RTP_DataFrame(data, len, dynamic)
{
}


RTP_IMFrame::RTP_IMFrame()
{
  SetExtension(true);
  SetExtensionSizeDWORDs(0);
  SetPayloadSize(0);
}

RTP_IMFrame::RTP_IMFrame(const PString & contentType)
{
  SetExtension(true);
  SetExtensionSizeDWORDs(0);
  SetPayloadSize(0);
  SetContentType(contentType);
}

RTP_IMFrame::RTP_IMFrame(const PString & contentType, const T140String & content)
{
  SetExtension(true);
  SetExtensionSizeDWORDs(0);
  SetPayloadSize(0);
  SetContentType(contentType);
  SetContent(content);
}

void RTP_IMFrame::SetContentType(const PString & contentType)
{
  PINDEX newExtensionBytes  = contentType.GetLength();
  PINDEX newExtensionDWORDs = (newExtensionBytes + 3) / 4;
  PINDEX oldPayloadSize = GetPayloadSize();

  // adding an extension adds 4 bytes to the header,
  //  plus the number of 32 bit words needed to hold the extension
  if (!GetExtension()) {
    SetPayloadSize(4 + newExtensionDWORDs + oldPayloadSize);
    if (oldPayloadSize > 0)
      memcpy(GetPayloadPtr() + newExtensionBytes + 4, GetPayloadPtr(), oldPayloadSize);
  }

  // if content type has not changed, nothing to do
  else if (GetContentType() == contentType) 
    return;

  // otherwise copy the new extension in
  else {
    PINDEX oldExtensionDWORDs = GetExtensionSizeDWORDs();
    if (oldPayloadSize != 0) {
      if (newExtensionDWORDs <= oldExtensionDWORDs) {
        memcpy(GetExtensionPtr() + newExtensionBytes, GetPayloadPtr(), oldPayloadSize);
      }
      else {
        SetPayloadSize((newExtensionDWORDs - oldExtensionDWORDs)*4 + oldPayloadSize);
        memcpy(GetExtensionPtr() + newExtensionDWORDs*4, GetPayloadPtr(), oldPayloadSize);
      }
    }
  }
  
  // reset lengths
  SetExtensionSizeDWORDs(newExtensionDWORDs);
  memcpy(GetExtensionPtr(), (const char *)contentType, newExtensionBytes);
  SetPayloadSize(oldPayloadSize);
  if (newExtensionDWORDs*4 > newExtensionBytes)
    memset(GetExtensionPtr() + newExtensionBytes, 0, newExtensionDWORDs*4 - newExtensionBytes);
}

PString RTP_IMFrame::GetContentType() const
{
  if (!GetExtension() || (GetExtensionSizeDWORDs() == 0))
    return PString::Empty();

  const char * p = (const char *)GetExtensionPtr();
  return PString(p, strlen(p));
}

void RTP_IMFrame::SetContent(const T140String & text)
{
  SetPayloadSize(text.GetSize());
  memcpy(GetPayloadPtr(), (const BYTE *)text, text.GetSize());
}

bool RTP_IMFrame::GetContent(T140String & text) const
{
  if (GetPayloadSize() == 0) 
    text.SetSize(0);
  else 
    text = T140String((const BYTE *)GetPayloadPtr(), GetPayloadSize());
  return true;
}

//////////////////////////////////////////////////////////////////////////////////////////

OpalIMMediaStream::OpalIMMediaStream(
      OpalConnection & conn,
      const OpalMediaFormat & mediaFormat, ///<  Media format for stream
      unsigned sessionID,                  ///<  Session number for stream
      bool isSource                        ///<  Is a source stream
    )
  : OpalMediaStream(conn, mediaFormat, sessionID, isSource)
{
}

bool OpalIMMediaStream::ReadPacket(RTP_DataFrame & /*packet*/)
{
  PAssertAlways("Cannot ReadData from OpalIMMediaStream");
  return false;
}

bool OpalIMMediaStream::WritePacket(RTP_DataFrame & frame)
{
  RTP_IMFrame imFrame(frame.GetPointer(), frame.GetSize());
  //connection.OnReceiveInternalIM(mediaFormat, imFrame);
  return true;
}

//////////////////////////////////////////////////////////////////////////////////////////

PAtomicInteger OpalIM::m_messageIdCounter;

OpalIM::OpalIM()
  : m_type(Text)
  , m_messageId(m_messageIdCounter++)
{
  PTRACE(3, "OpalIM\tcreate new IM");
}

//////////////////////////////////////////////////////////////////////////////////////////

OpalIMContext::OpalIMContext()
  : m_incomingMessageNotifier(NULL)
  , m_manager(NULL)
  , m_currentOutgoingMessage(NULL)
{ 
  m_id = OpalGloballyUniqueID().AsString();
}


OpalIMContext::~OpalIMContext()
{
  if (m_manager != NULL)
    m_manager->GetIMManager().RemoveContext(this);
}


void OpalIMContext::ResetLastUsed()
{
  PWaitAndSignal m(m_lastUsedMutex);
  m_lastUsed = PTime();
}


PSafePtr<OpalIMContext> OpalIMContext::Create(OpalManager & manager, const PURL & localURL_, const PURL & remoteURL)
{
  PURL localURL(localURL_);
  PString userName = localURL.GetUserName();

  // must have a remote scheme
  PString remoteScheme   = remoteURL.GetScheme();
  if (remoteURL.GetScheme().IsEmpty()) {
    PTRACE(3, "OpalIMContext\tTo URL '" << remoteURL << "' has no scheme");
    return NULL;
  }

  // force local scheme to same as remote scheme
  if (localURL.GetScheme() != remoteScheme) {
    PTRACE(3, "OpalIMContext\tForcing local scheme to '" << remoteScheme << "'");
    localURL.SetScheme(remoteScheme);
  }

  // if the remote scheme has a domain/make sure the local URL has a domain too
  if (!remoteURL.GetHostName().IsEmpty()) {
    if (localURL.GetHostName().IsEmpty()) {
      localURL.SetHostName(PIPSocket::GetHostName());
    }
  }

  // create the IM context
  PSafePtr<OpalIMContext> imContext = PFactory<OpalIMContext>::CreateInstance(remoteScheme);
  if (imContext == NULL) {
    PTRACE(3, "OpalIMContext\tCannot find IM handler for scheme '" << remoteScheme << "'");
    return NULL;
  }

  // populate the context
  imContext->m_manager     = &manager;
  imContext->m_localURL    = localURL.AsString();
  imContext->m_remoteURL   = remoteURL.AsString();
  imContext->GetAttributes().Set("scheme", remoteScheme);

  // save the context into the lookup maps
  manager.GetIMManager().AddContext(imContext);

  imContext->ResetLastUsed();

  PTRACE(3, "OpalIMContext\tCreated IM context '" << imContext->GetID() << "' for scheme '" << remoteScheme << "' from " << localURL << " to " << remoteURL);

  return imContext;
}

PString OpalIMContext::CreateKey(const PString & local, const PString & remote)
{
  PString key;
  if (local < remote)
    key = local + "|" + remote;
  else
    key = remote + "|" + local;
  return key;
}

PSafePtr<OpalIMContext> OpalIMContext::Create(OpalManager & manager, PSafePtr<OpalConnection> conn)
{
  PSafePtr<OpalIMContext> context = OpalIMContext::Create(manager,
                                                          conn->GetLocalPartyURL(), 
                                                          conn->GetRemotePartyURL());
  if (context != NULL) {
    context->m_connection     = conn;
    context->m_connection.SetSafetyMode(PSafeReference);
  }
  return context;
}


PSafePtr<OpalIMContext> OpalIMContext::Create(OpalManager & manager, PSafePtr<OpalPresentity> presentity, const PURL & remoteURL)
{
  PSafePtr<OpalIMContext> context = OpalIMContext::Create(manager, presentity->GetAOR(), remoteURL);
  if (context != NULL) {
    context->m_presentity     = presentity;
    context->m_presentity.SetSafetyMode(PSafeReference);
  }

  return context;
}


OpalIMContext::SentStatus OpalIMContext::SendCompositionIndication(bool active)
{
  OpalIM * message = new OpalIM;
  message->m_conversationId = GetID();
  message->m_type   = active ? OpalIM::CompositionIndication_Active : OpalIM::CompositionIndication_Idle;
  message->m_from   = GetLocalURL();
  message->m_to     = GetRemoteURL();
  return Send(message);
}


OpalIMContext::SentStatus OpalIMContext::Send(OpalIM * message)
{
  ResetLastUsed();

  // set content type, if not set
  if ((message->m_type == OpalIM::Text) && message->m_mimeType.IsEmpty())
    message->m_mimeType = "text/plain";

  // set conversation ID
  message->m_conversationId = GetID();

  // if outgoing message still pending, queue this message
  m_outgoingMessagesMutex.Wait();
  if (m_currentOutgoingMessage != NULL) {
    m_outgoingMessages.Enqueue(message);
    m_outgoingMessagesMutex.Signal();
    return SentPending;
  }
  m_currentOutgoingMessage = message;
  m_outgoingMessagesMutex.Signal();

  return InternalSend();
}


OpalIMContext::SentStatus OpalIMContext::InternalSend()
{
  PAssert(m_currentOutgoingMessage != NULL, "No message to send");

  // if sent outside connection, 
  if (m_connection == NULL) 
    return InternalSendOutsideCall(m_currentOutgoingMessage);

  // make connection read write
  if (m_connection.SetSafetyMode(PSafeReadWrite)) {
    delete m_currentOutgoingMessage;
    PTRACE(3, "OpalIMContext\tConnection to '" << m_attributes.Get("remote") << "' has been removed");
    m_connection.SetNULL();
    return SentConnectionClosed;
  }

  /// TODO - send IM here
  PTRACE(4, "OpalIMContext\tSending IM to '" << m_attributes.Get("remote") << "' via connection '" << m_connection << "'");
  SentStatus stat = InternalSendInsideCall(m_currentOutgoingMessage);

  // make connection reference while not being used
  // as we have already sent the message, no need to check the error status - it will
  // be set correctly the next time we try to send a message
  m_connection.SetSafetyMode(PSafeReference);

  return stat;
}

void OpalIMContext::InternalOnMessageSent(const MessageSentInfo & info)
{
  // double check that message that was sent was the correct one
  m_outgoingMessagesMutex.Wait();
  if (m_currentOutgoingMessage == NULL) {
    PTRACE(2, "OpalIMContext\tReceived sent confirmation when no message was sent");
    m_outgoingMessagesMutex.Signal();
    return;
  }
  if (m_currentOutgoingMessage->m_messageId != info.messageId) {
    PTRACE(2, "OpalIMContext\tReceived sent confirmation for wrong message - " << m_currentOutgoingMessage->m_messageId << " instead of " << info.messageId);
    m_outgoingMessagesMutex.Signal();
    return;
  }

  // free up the outgoing message buffer
  OpalIM * message = m_currentOutgoingMessage;

  // if there are more messages to send, get one started
  if (m_outgoingMessages.GetSize() == 0) 
    m_currentOutgoingMessage = NULL;
  else
    m_currentOutgoingMessage = m_outgoingMessages.Dequeue();
  
  // unlock the queue
  m_outgoingMessagesMutex.Signal();

  // invoke callback for message sent
  OnMessageSent(info);

  // cleanup the message just cleared
  delete message;

  // see if another message ready to be sent
  if (m_currentOutgoingMessage != NULL)
    InternalSend();
}

OpalIMContext::SentStatus OpalIMContext::InternalSendOutsideCall(OpalIM * /*message*/)
{
  PTRACE(3, "OpalIMContext\tSending IM outside call to '" << m_attributes.Get("remote") << "' not supported");
  return SentFailedGeneric;
}


OpalIMContext::SentStatus OpalIMContext::InternalSendInsideCall(OpalIM * /*message*/)
{
  PTRACE(3, "OpalIMContext\tSending IM inside call to '" << m_attributes.Get("remote") << "' not supported");
  return SentFailedGeneric;
}


OpalIM * OpalIMContext::GetIncomingMessage()
{
  return m_incomingMessages.Dequeue();
}


void OpalIMContext::AddIncomingIM(OpalIM * message)
{
  m_incomingMessages.Append(message);
}


bool OpalIMContext::OnNewIncomingIM()
{
  OpalIM * message;

  // dequeue the next message
  {
    PWaitAndSignal m(m_incomingMessagesMutex);
    message = m_incomingMessages.Dequeue();
    if (message == NULL)
      return true;
  }

  ResetLastUsed();

  return OnIncomingIM(*message);
}


void OpalIMContext::OnCompositionIndicationTimeout()
{
}

void OpalIMContext::SetIncomingIMNotifier(const IncomingIMNotifier & notifier)
{
  PWaitAndSignal mutex(m_notificationMutex);
  m_incomingMessageNotifier = notifier;
}


bool OpalIMContext::OnIncomingIM(OpalIM & message)
{
  PWaitAndSignal mutex(m_notificationMutex);

  if (!GetAttributes().Has("preferred-content-type") && (!message.m_mimeType.IsEmpty()))
    GetAttributes().Set("preferred-content-type", message.m_mimeType);

  if (!m_incomingMessageNotifier.IsNULL())
    m_incomingMessageNotifier(*this, message);

  return true;
}


void OpalIMContext::SetMessageSentNotifier(const MessageSentNotifier & notifier)
{
  PWaitAndSignal mutex(m_notificationMutex);
  m_messageSentNotifier = notifier;
}


void OpalIMContext::OnMessageSent(const MessageSentInfo & info)
{
  PWaitAndSignal mutex(m_notificationMutex);
  if (!m_messageSentNotifier.IsNULL())
    m_messageSentNotifier(*this, info);
}

void OpalIMContext::OnCompositionIndicationChanged(const PString & state)
{
  PWaitAndSignal mutex(m_notificationMutex);
  if (!m_compositionIndicationChangedNotifier.IsNULL())
    m_compositionIndicationChangedNotifier(*this, state);
}

void OpalIMContext::SetCompositionIndicationChangedNotifier(const CompositionIndicationChangedNotifier & notifier)
{
  PWaitAndSignal mutex(m_notificationMutex);
  m_compositionIndicationChangedNotifier = notifier;
}

//////////////////////////////////////////////////////////////////////////////////////////

OpalConnectionIMContext::OpalConnectionIMContext()
{
}

//////////////////////////////////////////////////////////////////////////////////////////

OpalPresentityIMContext::OpalPresentityIMContext()
{
}


//////////////////////////////////////////////////////////////////////////////////////////

OpalIMManager::OpalIMManager(OpalManager & manager)
  : m_manager(manager)
  , m_deleting(false)
{
}


OpalIMManager::~OpalIMManager()
{
  m_deleting = true;
}


void OpalIMManager::AddWork(IM_Work * work)
{
  m_imThreadPool.AddWork(work);
}


void OpalIMManager::AddContext(PSafePtr<OpalIMContext> imContext)
{
  // set the key
  PString key(OpalIMContext::CreateKey(imContext->m_localURL, imContext->m_remoteURL));
  imContext->m_key = key;

  PTRACE(2, "OpalIM\tAdded IM context '" << imContext->GetID() << "' to manager");

  m_contextsByConversationId.SetAt(imContext->GetID(), imContext);

  PWaitAndSignal m(m_contextsByNamesMutex);
  std::string skey((const char *)key);
  m_contextsByNames.insert(ContextsByNames::value_type(skey, imContext->GetID()));
}


void OpalIMManager::RemoveContext(OpalIMContext * context)
{
  if (m_deleting)
    return;

  PString key = context->GetKey();
  PString id = context->GetID();

  // remove local/remote pair from multimap
  {
    PWaitAndSignal m(m_contextsByNamesMutex);
    ContextsByNames::iterator r = m_contextsByNames.find((const char *)key);
    for (r = m_contextsByNames.find((const char *)key); 
         (r != m_contextsByNames.end() && (r->first == (const char *)key));
         ++r) {
       if (r->second == id) {
         m_contextsByNames.erase(r);
         break;
       }
    }
  }

  // remove conversation ID from dictionary
  m_contextsByConversationId.RemoveAt(id);

  PTRACE(5, "OpalIM\tContext '" << id << "' removed");
}


void OpalIMManager::GarbageCollection()
{
  PTime now;

  if ((now - m_lastGarbageCollection).GetMilliSeconds() < 30000)
    return;

  PStringArray conversations;
  {
    PSafePtr<OpalIMContext> context(m_contextsByConversationId, PSafeReadOnly);
    while (context != NULL) {
      conversations.AppendString(context->GetID());
      ++context;
    }
  }

  for (PINDEX i = 0; i < conversations.GetSize(); ++i) {
    PSafePtr<OpalIMContext> context = m_contextsByConversationId.FindWithLock(conversations[i], PSafeReadWrite);
    if (context != NULL) {
      int timeout = context->GetAttributes().Get("timeout", "300000").AsInteger();
      PTimeInterval diff(now - context->m_lastUsed);
      if (diff.GetMilliSeconds() > timeout)
        m_contextsByConversationId.RemoveAt(conversations[i]);
    }
  }

  m_contextsByConversationId.DeleteObjectsToBeRemoved();
}


PSafePtr<OpalIMContext> OpalIMManager::FindContextByIdWithLock(const PString & id, PSafetyMode mode) 
{
  return m_contextsByConversationId.FindWithLock(id, mode);
}


PSafePtr<OpalIMContext> OpalIMManager::FindContextByNamesWithLock(const PString & local, const PString & remote, PSafetyMode mode) 
{
  PString id;
  {
    PString key(OpalIMContext::CreateKey(local, remote));
    PWaitAndSignal m(m_contextsByNamesMutex);
    ContextsByNames::iterator r = m_contextsByNames.find((const char *)key);
    if (r == m_contextsByNames.end())
      return NULL;
    id = r->second;
  }
  return FindContextByIdWithLock(id, mode);
}


void OpalIMManager::AddNotifier(const NewConversationNotifier & notifier, const PString & scheme)
{
  NewConversationCallBack * callback = new NewConversationCallBack;
  callback->m_scheme   = scheme;
  callback->m_notifier = notifier;

  PWaitAndSignal mutex(m_notifierMutex);

  for (PList<NewConversationCallBack>::iterator f = m_callbacks.begin(); f != m_callbacks.end(); ++f)
    if (f->m_notifier == notifier && f->m_scheme == scheme)
      return;

  m_callbacks.Append(callback);
}


bool OpalIMManager::RemoveNotifier(const NewConversationNotifier & notifier, const PString & scheme)
{
  PWaitAndSignal mutex(m_notifierMutex);

  for (PList<NewConversationCallBack>::iterator f = m_callbacks.begin(); f != m_callbacks.end(); ++f) {
    if (f->m_notifier == notifier && f->m_scheme == scheme) {
      m_callbacks.erase(f);
      return true;
    }
  }

  return false;
}

static bool CheckFromTo(OpalIMContext & context, OpalIM & im)
{
  PString local  = context.GetAttributes().Get("local");
  PString remote = context.GetAttributes().Get("remote");
  return (local == im.m_to) && (remote == im.m_from);
}


PSafePtr<OpalIMContext> OpalIMManager::FindContextForMessageWithLock(OpalIM & im, OpalConnection * conn)
{
  PSafePtr<OpalIMContext> context;

  // use connection-based information, if available
  if (conn != NULL) {
    if (im.m_conversationId.IsEmpty()) {
      PTRACE(2, "OpalIM\tconversation ID cannot be empty for connection based calls");
      return NULL;
    }
  }

  // see if conversation ID matches local/remote
  if (!im.m_conversationId.IsEmpty()) {
    context = FindContextByIdWithLock(im.m_conversationId);
    if ((context != NULL) && !CheckFromTo(*context, im)) {
      PTRACE(2, "OpalIM\tWARNING: Matched conversation ID for incoming message but did not match to/from");
    }
  }

  // if no context, see if we can match from/to
  if (context == NULL) {
    context = FindContextByNamesWithLock(im.m_to, im.m_from);
    if (context != NULL) {
      if (im.m_conversationId.IsEmpty())
        im.m_conversationId = context->GetID();
      else if (context->GetID() != im.m_conversationId) {
        PTRACE(2, "OpalIM\tWARNING: Matched to/from for incoming message but did not match conversation ID");
      }
    }
  }

  return context;
}


bool OpalIMManager::OnIncomingMessage(OpalIM * im, PSafePtr<OpalConnection> conn)
{
  PSafePtr<OpalIMContext> context = FindContextForMessageWithLock(*im, conn);

  // if context found, add message to it
  if (context != NULL)
    context->AddIncomingIM(im);
  else {

    // create a context based on the connection
    if (conn != NULL)
      context = OpalIMContext::Create(m_manager, conn);
    else
      context = OpalIMContext::Create(m_manager, im->m_to, im->m_from);

    if (context == NULL) {
      PTRACE(2, "OpalIM\tCannot create IM context for incoming message from '" << im->m_from);
      delete im;
      return false;
    }

    // set message conversation ID to the correct (new) value
    im->m_conversationId = context->GetID();

    // save the connection (if any)
    context->m_connection = conn;

    // save the first message
    context->AddIncomingIM(im);

    // queue work for processing, using the conversation ID as the key
    PTRACE(3, "OpalIM\tAdding new conversation work for conversation " << im->m_conversationId);
    m_imThreadPool.AddWork(new NewConversation_Work(*this, im->m_conversationId));
  }

  PTRACE(3, "OpalIM\tAdding new message work for conversation " << im->m_conversationId);
  m_imThreadPool.AddWork(new NewIncomingIM_Work(*this, im->m_conversationId));
  return true;
}


void OpalIMManager::InternalOnNewConversation(const PString & conversationId)
{
  PSafePtr<OpalIMContext> context = FindContextByIdWithLock(conversationId);
  if (context == NULL) {
    PTRACE(2, "OpalIM\tCannot find IM context for '" << conversationId << "'");
    return;
  }
  
  // get the scheme used by the context
  PString scheme = context->GetAttributes().Get("scheme");

  // call all of the notifiers 
  {
    PWaitAndSignal m(m_notifierMutex);
    if (m_callbacks.GetSize() > 0) {
      for (PList<NewConversationCallBack>::iterator i = m_callbacks.begin(); i != m_callbacks.end(); i++)
        if ((i->m_scheme == "*") || (i->m_scheme *= scheme))
          (i->m_notifier)(*this, *context);
    }
    return;
  }

  // if no notifiers, call the OpalManager functions
  OpalIM * im = context->GetIncomingMessage();
  if (im != NULL) {
    m_manager.OnMessageReceived(*im);
    delete im;
  }
}


void OpalIMManager::OnCompositionIndicationTimeout(const PString & conversationId)
{
  PTRACE(3, "OpalIM\tAdding composition indication timeout work for conversation " << conversationId);
  m_imThreadPool.AddWork(new CompositionIndicationTimeout_Work(*this, conversationId));
}


void OpalIMManager::InternalOnCompositionIndicationTimeout(const PString & conversationId)
{
  PSafePtr<OpalIMContext> context = FindContextByIdWithLock(conversationId);
  if (context == NULL) {
    PTRACE(2, "OpalIM\tCannot find IM context for '" << conversationId << "'");
    return;
  }

  context->OnCompositionIndicationTimeout();
}


void OpalIMManager::InternalOnNewIncomingIM(const PString & conversationId)
{
  PSafePtr<OpalIMContext> context = FindContextByIdWithLock(conversationId);
  if (context == NULL) {
    PTRACE(2, "OpalIM\tCannot find IM context for '" << conversationId << "'");
    return;
  }

  PTRACE(2, "OpalIM\tReceived message for '" << conversationId << "'");
  context->OnNewIncomingIM();
}


void OpalIMManager::InternalOnMessageSent(const PString & conversationId, const OpalIMContext::MessageSentInfo & info)
{
  PSafePtr<OpalIMContext> context = FindContextByIdWithLock(conversationId);
  if (context == NULL) {
    PTRACE(2, "OpalIM\tCannot find IM context for '" << conversationId << "'");
    return;
  }

  context->InternalOnMessageSent(info);
}

///////////////////////////////////////////////////////////////

OpalIMManager::WorkThreadPool::WorkerThreadBase * OpalIMManager::WorkThreadPool::CreateWorkerThread()
{ 
  return new IM_Work_Thread(*this); 
}

///////////////////////////////////////////////////////////////

OpalIMManager::IM_Work_Thread::IM_Work_Thread(WorkThreadPool & pool_)
  : WorkThreadPool::WorkerThread(pool_)
{
}

unsigned OpalIMManager::IM_Work_Thread::GetWorkSize() const 
{ 
  return m_cmdQueue.size(); 
}


void OpalIMManager::IM_Work_Thread::AddWork(IM_Work * work)
{
  PWaitAndSignal m(m_workerMutex);
  m_cmdQueue.push(work);
  if (m_cmdQueue.size() == 1)
    m_sync.Signal();
}


void OpalIMManager::IM_Work_Thread::RemoveWork(IM_Work * work)
{
  m_workerMutex.Wait();
  m_cmdQueue.pop();
  m_workerMutex.Signal();
  delete work;
}


void OpalIMManager::IM_Work_Thread::Shutdown()
{
  m_shutdown = true;
  m_sync.Signal();
}


void OpalIMManager::IM_Work_Thread::Main()
{
  while (!m_shutdown) {

    // get the work
    m_workerMutex.Wait();
    IM_Work * work = m_cmdQueue.size() > 0 ? m_cmdQueue.front() : NULL;
    m_workerMutex.Signal();

    // wait for work to become available
    if (work == NULL) {
      m_sync.Wait();
      continue;
    }

    // process the work
    work->Process();

    // indicate work is now free
    m_pool.RemoveWork(work);
  }
}



///////////////////////////////////////////////////////////////////////////////////////////////

OpalIMManager::IM_Work::IM_Work(OpalIMManager & mgr, const PString & conversationId)
  : m_mgr(mgr)
  , m_conversationId(conversationId)
{
}


OpalIMManager::IM_Work::~IM_Work()
{
}

///////////////////////////////////////////////////////////////////////////////////////////////

/*
void OpalIMManager::IncomingText::Execute(OpalIMManager & mgr)
{
  RTP_DataFrameList frames = m_rfc4103Context[0].ConvertToFrames(contentType, body);

  for (PINDEX i = 0; i < frames.GetSize(); ++i) {
    //OnReceiveExternalIM(OpalT140, (RTP_IMFrame &)frames[i]);
  }
}
*/

#endif // OPAL_HAS_IM
