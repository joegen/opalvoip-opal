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
#include <opal/endpoint.h>
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
                          "t140", 
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

const PCaselessString & OpalIMContext::CompositionIndicationActive() { static const PConstCaselessString s("active"); return s; }
const PCaselessString & OpalIMContext::CompositionIndicationIdle()   { static const PConstCaselessString s("idle"); return s; }

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

OpalIM::OpalIM()
  : m_messageId(GetNextMessageId())
{
}


PAtomicInteger::IntegerType OpalIM::GetNextMessageId()
{
  static PAtomicInteger messageIdCounter;
  return ++messageIdCounter;
}


//////////////////////////////////////////////////////////////////////////////////////////

OpalIMContext::OpalIMContext()
  : m_manager(NULL)
  , m_currentOutgoingMessage(NULL)
{ 
  m_conversationId = OpalGloballyUniqueID().AsString();
  PTRACE(5, "OpalIM\tContext '" << m_conversationId << "' created");
}


OpalIMContext::~OpalIMContext()
{
  Close();
  PTRACE(5, "OpalIM\tContext '" << m_conversationId << "' destroyed");
}


void OpalIMContext::ResetLastUsed()
{
  PWaitAndSignal m(m_lastUsedMutex);
  m_lastUsed.SetCurrentTime();
}


PSafePtr<OpalIMContext> OpalIMContext::Create(OpalManager & manager,
                                              const PURL & localURL_,
                                              const PURL & remoteURL,
                                              bool byRemote)
{
  PURL localURL(localURL_);
  PString userName = localURL.GetUserName();

  // must have a remote scheme
  PCaselessString remoteScheme = remoteURL.GetScheme();
  if (remoteScheme.IsEmpty()) {
    PTRACE2(3, NULL, "OpalIM\tTo URL '" << remoteURL << "' has no scheme");
    return NULL;
  }

  // force local scheme to same as remote scheme
  if (localURL.GetScheme() != remoteScheme) {
    PTRACE2(3, NULL, "OpalIM\tForcing local scheme to '" << remoteScheme << "'");
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
    PTRACE2(3, NULL, "OpalIM\tCannot find IM handler for scheme '" << remoteScheme << "'");
    return NULL;
  }

  // populate the context
  imContext->m_manager     = &manager;
  imContext->m_localURL    = localURL.AsString();
  imContext->m_localName   = manager.GetDefaultDisplayName();
  imContext->m_remoteURL   = remoteURL.AsString();
  imContext->m_remoteName  = remoteURL.GetUserName();
  imContext->GetAttributes().Set("scheme", remoteScheme);

  imContext->ResetLastUsed();

  // save the context into the lookup maps
  manager.GetIMManager().AddContext(imContext, byRemote);

  PTRACE2(3, imContext, "OpalIM\tCreated IM context '" << imContext->GetID() << "' for scheme '" << remoteScheme << "' from " << localURL << " to " << remoteURL);

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

PSafePtr<OpalIMContext> OpalIMContext::Create(PSafePtr<OpalConnection> connection, bool byRemote)
{
  PSafePtr<OpalIMContext> context = Create(connection->GetEndPoint().GetManager(),
                                           connection->GetLocalPartyURL(), 
                                           connection->GetRemotePartyURL(),
                                           byRemote);
  if (context != NULL) {
    context->m_connection = connection;
    context->m_connection.SetSafetyMode(PSafeReference);
  }
  return context;
}


PSafePtr<OpalIMContext> OpalIMContext::Create(PSafePtr<OpalPresentity> presentity,
                                              const PURL & remoteURL,
                                              bool byRemote)
{
  PSafePtr<OpalIMContext> context = Create(presentity->GetManager(),
                                           presentity->GetAOR(),
                                           remoteURL,
                                           byRemote);
  if (context != NULL) {
    context->m_presentity = presentity;
    context->m_presentity.SetSafetyMode(PSafeReference);
  }

  return context;
}


void OpalIMContext::Close()
{
  if (m_manager != NULL)
    m_manager->GetIMManager().RemoveContext(this, false);
}


bool OpalIMContext::SendCompositionIndication(const CompositionInfo &)
{
  return false;
}


OpalIMContext::MessageDisposition OpalIMContext::Send(OpalIM * message)
{
  ResetLastUsed();

  // make sure various fields are from this context
  message->m_conversationId = GetID();
  message->m_from = GetLocalURL();
  message->m_to = GetRemoteURL();

  // if outgoing message still pending, queue this message
  m_outgoingMessagesMutex.Wait();
  if (m_currentOutgoingMessage != NULL) {
    m_outgoingMessages.Enqueue(message);
    m_outgoingMessagesMutex.Signal();
    return DispositionPending;
  }
  m_currentOutgoingMessage = message;
  m_outgoingMessagesMutex.Signal();

  MessageDisposition status = InternalSend();
  if (status >= DispositionErrors) {
    delete m_currentOutgoingMessage;
    m_currentOutgoingMessage = NULL;
  }
  return status;
}


OpalIMContext::MessageDisposition OpalIMContext::InternalSend()
{
  PAssert(m_currentOutgoingMessage != NULL, "No message to send");

  // if sent outside connection, 
  if (m_connection == NULL) 
    return InternalSendOutsideCall(*m_currentOutgoingMessage);

  // make connection read write
  if (m_connection.SetSafetyMode(PSafeReadWrite)) {
    delete m_currentOutgoingMessage;
    m_currentOutgoingMessage = NULL;
    PTRACE(3, "OpalIM\tConnection to '" << m_attributes.Get("remote") << "' has been removed");
    m_connection.SetNULL();
    return ConversationClosed;
  }

  /// TODO - send IM here
  PTRACE(4, "OpalIM\tSending IM to '" << m_attributes.Get("remote") << "' via connection '" << m_connection << "'");
  MessageDisposition stat = InternalSendInsideCall(*m_currentOutgoingMessage);

  // make connection reference while not being used
  // as we have already sent the message, no need to check the error status - it will
  // be set correctly the next time we try to send a message
  m_connection.SetSafetyMode(PSafeReference);

  return stat;
}

void OpalIMContext::InternalOnMessageSent(const DispositionInfo & info)
{
  // double check that message that was sent was the correct one
  m_outgoingMessagesMutex.Wait();
  if (m_currentOutgoingMessage == NULL) {
    PTRACE(2, "OpalIM\tReceived sent confirmation when no message was sent");
    m_outgoingMessagesMutex.Signal();
    return;
  }

  if (m_currentOutgoingMessage->m_messageId != info.m_messageId) {
    PTRACE(2, "OpalIM\tReceived sent confirmation for wrong message - " << m_currentOutgoingMessage->m_messageId << " instead of " << info.m_messageId);
    m_outgoingMessagesMutex.Signal();
    return;
  }

  // free up the outgoing message buffer
  OpalIM * message = m_currentOutgoingMessage;

  // if there are more messages to send, get one started
  if (m_outgoingMessages.IsEmpty())
    m_currentOutgoingMessage = NULL;
  else
    m_currentOutgoingMessage = m_outgoingMessages.Dequeue();
  
  // unlock the queue
  m_outgoingMessagesMutex.Signal();

  // invoke callback for message sent
  OnMessageDisposition(info);

  // cleanup the message just cleared
  delete message;

  // see if another message ready to be sent
  if (m_currentOutgoingMessage != NULL)
    InternalSend();
}


OpalIMContext::MessageDisposition OpalIMContext::InternalSendOutsideCall(OpalIM & /*message*/)
{
  PTRACE(3, "OpalIM\tSending IM outside call to '" << m_attributes.Get("remote") << "' not supported");
  return UnsupportedFeature;
}


OpalIMContext::MessageDisposition OpalIMContext::InternalSendInsideCall(OpalIM & /*message*/)
{
  PTRACE(3, "OpalIM\tSending IM inside call to '" << m_attributes.Get("remote") << "' not supported");
  return UnsupportedFeature;
}


OpalIMContext::MessageDisposition OpalIMContext::OnMessageReceived(const OpalIM & message)
{
  if (!message.m_fromName.IsEmpty())
    m_remoteName = message.m_fromName;

  bool allBad = false;
  for (PStringToString::const_iterator it = message.m_bodies.begin(); it != message.m_bodies.end(); ++it) {
    if (CheckContentType(it->first))
      allBad = false;
    else
      PTRACE(3, "OpalIM\tContent type '" << it->first << "' not acceptable for conversation id=" << GetID());
  }

  if (allBad)
    return UnacceptableContent;

  PTRACE(2, "OpalIM\tReceived message for '" << GetID() << "'");

  PWaitAndSignal mutex(m_notificationMutex);

  if (m_messageReceivedNotifier.IsNULL())
    m_manager->OnMessageReceived(message);
  else
    m_messageReceivedNotifier(*this, message);

  return DeliveryOK;
}


bool OpalIMContext::CheckContentType(const PString & contentType) const
{
  return GetContentTypes().GetStringsIndex(contentType) != P_MAX_INDEX;
}


PStringArray OpalIMContext::GetContentTypes() const
{
  return GetAttributes().Get("acceptable-content-types", PMIMEInfo::TextPlain()).Lines();
}


void OpalIMContext::SetMessageReceivedNotifier(const MessageReceivedNotifier & notifier)
{
  PWaitAndSignal mutex(m_notificationMutex);
  m_messageReceivedNotifier = notifier;
}


void OpalIMContext::SetMessageDispositionNotifier(const MessageDispositionNotifier & notifier)
{
  PWaitAndSignal mutex(m_notificationMutex);
  m_messageDispositionNotifier = notifier;
}


void OpalIMContext::OnMessageDisposition(const DispositionInfo & info)
{
  PWaitAndSignal mutex(m_notificationMutex);
  if (!m_messageDispositionNotifier.IsNULL())
    m_messageDispositionNotifier(*this, info);
}

void OpalIMContext::OnCompositionIndication(const CompositionInfo & info)
{
  PWaitAndSignal mutex(m_notificationMutex);
  if (!m_compositionIndicationNotifier.IsNULL())
    m_compositionIndicationNotifier(*this, info);
}

void OpalIMContext::SetCompositionIndicationNotifier(const CompositionIndicationNotifier & notifier)
{
  PWaitAndSignal mutex(m_notificationMutex);
  m_compositionIndicationNotifier = notifier;
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


void OpalIMManager::AddContext(PSafePtr<OpalIMContext> context, bool byRemote)
{
  // set the key
  context->m_key = OpalIMContext::CreateKey(context->m_localURL, context->m_remoteURL);

  PTRACE(2, "OpalIM\tAdded IM context '" << context->GetID() << "' to manager");

  m_contextsByConversationId.SetAt(context->GetID(), context);

  PWaitAndSignal m(m_contextsByNamesMutex);
  m_contextsByNames.insert(ContextsByNames::value_type(context->m_key, context->GetID()));

  ConversationInfo info;
  info.m_context = context;
  info.m_opening = true;
  info.m_byRemote = byRemote;
  OnConversation(info);
}


void OpalIMManager::RemoveContext(OpalIMContext * context, bool byRemote)
{
  if (m_deleting)
    return;

  ConversationInfo info;
  info.m_context = context;
  info.m_opening = false;
  info.m_byRemote = byRemote;
  OnConversation(info);

  PString key = context->GetKey();
  PString id = context->GetID();

  // remove local/remote pair from multimap
  {
    PWaitAndSignal m(m_contextsByNamesMutex);
    ContextsByNames::iterator it = m_contextsByNames.find(key);
    while (it != m_contextsByNames.end() && it->first == key) {
       if (it->second != id)
         ++it;
       else
         m_contextsByNames.erase(it++);
    }
  }

  // remove conversation ID from dictionary
  if (m_contextsByConversationId.RemoveAt(id))
    PTRACE(5, "OpalIM\tContext '" << id << "' removed");
}


void OpalIMManager::OnConversation(const ConversationInfo & info)
{
  // get the scheme used by the context
  PString scheme = info.m_context->GetAttributes().Get("scheme");

  // call all of the notifiers 
  bool useManagerCallback = true;
  {
    PWaitAndSignal m(m_notifierMutex);
    ConversationMap::iterator it = m_notifiers.find(scheme);
    if (it == m_notifiers.end())
      it = m_notifiers.find(scheme = "*");
    while (it != m_notifiers.end() && it->first == scheme) {
      (it->second)(*info.m_context, info);
      useManagerCallback = false;
      ++it;
    }
  }

  if (useManagerCallback)
    m_manager.OnConversation(info);
}


void OpalIMManager::ShutDown()
{
  PTRACE(3, "OpalIM\tShutting down all IM contexts");
  PSafePtr<OpalIMContext> context(m_contextsByConversationId, PSafeReadWrite);
  while (context != NULL)
    (context++)->Close();
}


bool OpalIMManager::GarbageCollection()
{
  PTime now;

  if ((now - m_lastGarbageCollection).GetSeconds() > 30) {
    PSafePtr<OpalIMContext> context(m_contextsByConversationId, PSafeReadWrite);
    while (context != NULL) {
      if ((now - context->m_lastUsed).GetSeconds() < context->GetAttributes().GetInteger("timeout", 3600))
        ++context;
      else
        (context++)->Close();
    }
  }

  return m_contextsByConversationId.DeleteObjectsToBeRemoved();
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


void OpalIMManager::AddNotifier(const ConversationNotifier & notifier, const PString & scheme)
{
  PWaitAndSignal mutex(m_notifierMutex);
  m_notifiers.insert(ConversationMap::value_type(scheme, notifier));
}


bool OpalIMManager::RemoveNotifier(const ConversationNotifier & notifier, const PString & scheme)
{
  bool removedOne = false;

  PWaitAndSignal mutex(m_notifierMutex);

  ConversationMap::iterator it = m_notifiers.find(scheme);
  while (it != m_notifiers.end() && it->first == scheme) {
    if (it->second != notifier)
      ++it;
    else {
      m_notifiers.erase(it++);
      removedOne = true;
    }
  }

  return removedOne;
}


PSafePtr<OpalIMContext> OpalIMManager::FindContextForMessageWithLock(OpalIM & im, OpalConnection * conn)
{
  // use connection-based information, if available
  if (conn != NULL) {
    if (im.m_conversationId.IsEmpty()) {
      PTRACE(2, "OpalIM\tconversation ID cannot be empty for connection based calls");
      return NULL;
    }
  }

  // see if conversation ID matches local/remote
  PSafePtr<OpalIMContext> context = FindContextByIdWithLock(im.m_conversationId);

  // if no context, see if we can match from/to
  if (context == NULL) {
    context = FindContextByNamesWithLock(im.m_to, im.m_from);
    if (context != NULL) {
      PTRACE_IF(2, !im.m_conversationId, "OpalIM\tWARNING: Matched to/from addresses but did not match ID");
      im.m_conversationId = context->GetID();
    }
  }

  return context;
}


OpalIMContext::MessageDisposition
          OpalIMManager::OnMessageReceived(OpalIM & message,
                                           OpalConnection * connection,
                                           PString & errorInfo)
{
  PSafePtr<OpalIMContext> context = FindContextForMessageWithLock(message, connection);

  // if context found, add message to it
  if (context == NULL) {
    // create a context based on the connection
    if (connection != NULL)
      context = OpalIMContext::Create(connection);
    else
      context = OpalIMContext::Create(m_manager, message.m_to, message.m_from);

    if (context == NULL) {
      PTRACE(2, "OpalIM\tCannot create IM context for incoming message from '" << message.m_from);
      return OpalIMContext::DestinationUnknown;
    }

    // set message conversation ID to the correct (new) value
    message.m_conversationId = context->GetID();
  }

  OpalIMContext::MessageDisposition status = context->OnMessageReceived(message);

  // check if content type was OK before "im" is given to the worker thread
  if (status == OpalIMContext::UnacceptableContent) {
    PStringArray contentTypes = context->GetContentTypes();
    if (contentTypes.IsEmpty())
      errorInfo = PMIMEInfo::TextPlain();
    else {
      PStringStream strm;
      strm << setfill(',') << contentTypes;
      errorInfo = strm;
    }
  }

  return status;
}


#endif // OPAL_HAS_IM
