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

#ifdef __GNUC__
#pragma implementation "im.h"
#pragma implementation "im_ep.h"
#endif

#include <ptlib.h>

#include <im/im.h>
#include <im/im_ep.h>

#if OPAL_HAS_IM

#include <ptclib/mime.h>
#include <opal/manager.h>
#include <opal/endpoint.h>
#include <opal/pres_ent.h>
#include <opal/guid.h>
#include <im/rfc4103.h>


#define new PNEW


const PCaselessString & OpalIMContext::CompositionIndicationActive() { static const PConstCaselessString s("active"); return s; }
const PCaselessString & OpalIMContext::CompositionIndicationIdle()   { static const PConstCaselessString s("idle"); return s; }

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
  : m_endpoint(NULL)
  , m_weStartedCall(false)
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


PString OpalIMContext::CreateKey(const PString & local, const PString & remote)
{
  PString key;
  if (local < remote)
    key = local + "|" + remote;
  else
    key = remote + "|" + local;
  return key;
}


bool OpalIMContext::Open(bool)
{
  return true;
}


void OpalIMContext::Close()
{
  if (m_weStartedCall && m_call != NULL)
    m_call->Clear();
  if (m_endpoint != NULL)
    m_endpoint->RemoveContext(this, false);
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
  if (message->m_from.IsEmpty())
    message->m_from = GetLocalURL();
  if (message->m_to.IsEmpty())
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
  if (m_call == NULL) 
    return InternalSendOutsideCall(*m_currentOutgoingMessage);

  // make connection read write
  if (!m_call.SetSafetyMode(PSafeReadWrite)) {
    delete m_currentOutgoingMessage;
    m_currentOutgoingMessage = NULL;
    PTRACE(3, "OpalIM\tConnection to '" << m_remoteURL << "' has been removed");
    m_call.SetNULL();
    return ConversationClosed;
  }

  PTRACE(4, "OpalIM\tSending IM to '" << m_remoteURL << "' via call '" << *m_call << "'");
  MessageDisposition stat = InternalSendInsideCall(*m_currentOutgoingMessage);

  // make connection reference while not being used
  // as we have already sent the message, no need to check the error status - it will
  // be set correctly the next time we try to send a message
  m_call.SetSafetyMode(PSafeReference);

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
  PTRACE(3, "OpalIM\tSending IM outside call to '" << m_remoteURL << "' not supported");
  return UnsupportedFeature;
}


OpalIMContext::MessageDisposition OpalIMContext::InternalSendInsideCall(OpalIM & /*message*/)
{
  PTRACE(3, "OpalIM\tSending IM inside call to '" << m_remoteURL << "' not supported");
  return UnsupportedFeature;
}


OpalIMContext::MessageDisposition OpalIMContext::OnMessageReceived(const OpalIM & message)
{
  if (message.m_bodies.IsEmpty())
    return DeliveryOK;

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
    m_endpoint->OnMessageReceived(message);
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
  if (m_messageDispositionNotifier.IsNULL())
    m_endpoint->GetManager().OnMessageDisposition(info);
  else
    m_messageDispositionNotifier(*this, info);
}

void OpalIMContext::OnCompositionIndication(const CompositionInfo & info)
{
  PWaitAndSignal mutex(m_notificationMutex);
  if (m_compositionIndicationNotifier.IsNULL())
    m_endpoint->GetManager().OnCompositionIndication(info);
  else
    m_compositionIndicationNotifier(*this, info);
}

void OpalIMContext::SetCompositionIndicationNotifier(const CompositionIndicationNotifier & notifier)
{
  PWaitAndSignal mutex(m_notificationMutex);
  m_compositionIndicationNotifier = notifier;
}


//////////////////////////////////////////////////////////////////////////////////////////

const PCaselessString & OpalIMEndPoint::Prefix() { static PConstCaselessString s("im"); return s; }

OpalIMEndPoint::OpalIMEndPoint(OpalManager & manager)
  : OpalEndPoint(manager, Prefix(), 0)
  , m_manager(manager)
  , m_deleting(false)
{
#if OPAL_HAS_SIPIM
  GetOpalSIPIM();
#endif
#if OPAL_HAS_RFC4103
  GetOpalT140();
#endif
#if OPAL_HAS_MSRP
  GetOpalMSRP();
#endif
}


OpalIMEndPoint::~OpalIMEndPoint()
{
  m_deleting = true;
}


PSafePtr<OpalConnection> OpalIMEndPoint::MakeConnection(OpalCall & call,
                                                        const PString &,
                                                        void * userData,
                                                        unsigned int options,
                                                        OpalConnection::StringOptions * stringOptions)
{
  return new OpalIMConnection(call, *this, userData, options, stringOptions);
}


OpalMediaFormatList OpalIMEndPoint::GetMediaFormats() const
{
  OpalMediaFormatList list;
#if OPAL_HAS_SIPIM
  list += OpalSIPIM;
#endif
#if OPAL_HAS_RFC4103
  list += OpalT140;
#endif
#if OPAL_HAS_MSRP
  list += OpalMSRP;
#endif
  return list;
}


PSafePtr<OpalIMContext> OpalIMEndPoint::CreateContext(OpalCall & call)
{
  if (call.IsNetworkOriginated())
    return InternalCreateContext(call.GetPartyB(), call.GetPartyA(), NULL, false, &call);
  else
    return InternalCreateContext(call.GetPartyA(), call.GetPartyB(), NULL, false, &call);
}


PSafePtr<OpalIMContext> OpalIMEndPoint::InternalCreateContext(const PURL & localURL,
                                                              const PURL & remoteURL,
                                                              const char * overrideScheme,
                                                              bool byRemote,
                                                              OpalCall * call)
{
  PCaselessString scheme = overrideScheme;
  if (scheme.IsEmpty())
    scheme = remoteURL.GetScheme();
  if (scheme.IsEmpty())
    scheme = localURL.GetScheme();

  // must have a remote scheme
  if (scheme.IsEmpty()) {
    PTRACE(2, "OpalIM\tURLs have no scheme");
    return NULL;
  }

  // create the IM context
  OpalIMContext * context = PFactory<OpalIMContext>::CreateInstance(scheme);
  if (context == NULL) {
    PTRACE2(3, NULL, "OpalIM\tCannot find IM handler for '" << remoteURL << "'");
    return NULL;
  }

  // populate the context
  context->m_endpoint    = this;
  context->m_localURL    = localURL;
  context->m_localName   = manager.GetDefaultDisplayName();
  context->m_remoteURL   = remoteURL;
  context->m_remoteName  = remoteURL.GetUserName();
  context->m_call        = call;
  context->GetAttributes().Set("scheme", scheme);
  context->m_key = OpalIMContext::CreateKey(context->m_localURL, context->m_remoteURL);
  context->ResetLastUsed();

  if (!context->Open(byRemote)) {
    context->m_endpoint = NULL;
    delete context;
    return NULL;
  }

  m_contextsByConversationId.SetAt(context->GetID(), context);

  PWaitAndSignal m(m_contextsByNamesMutex);
  m_contextsByNames.insert(ContextsByNames::value_type(context->m_key, context->GetID()));

  PTRACE(3, "OpalIM\tCreated IM context '" << context->GetID() << "' for scheme '" << scheme << "' from " << localURL << " to " << remoteURL);

  OpalIMContext::ConversationInfo info;
  info.m_context = context;
  info.m_opening = true;
  info.m_byRemote = byRemote;
  OnConversation(info);

  return context;
}


void OpalIMEndPoint::RemoveContext(OpalIMContext * context, bool byRemote)
{
  PString id = context->GetID();

  if (m_deleting || !m_contextsByConversationId.Contains(id))
    return;

  OpalIMContext::ConversationInfo info;
  info.m_context = context;
  info.m_opening = false;
  info.m_byRemote = byRemote;
  OnConversation(info);

  // remove local/remote pair from multimap
  {
    PString key = context->GetKey();
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
  if (m_contextsByConversationId.RemoveAt(id)) {
    PTRACE(5, "OpalIM\tContext '" << id << "' removed");
  }
}


void OpalIMEndPoint::OnConversation(const OpalIMContext::ConversationInfo & info)
{
  // get the scheme used by the context
  PString scheme = info.m_context->GetAttributes().Get("scheme");

  bool managerFallback = true;

  // call all of the notifiers 
  m_notifierMutex.Wait();
  ConversationMap::iterator it = m_notifiers.find(scheme);
  if (it == m_notifiers.end())
    it = m_notifiers.find(scheme = "*");
  while (it != m_notifiers.end() && it->first == scheme) {
    (it->second)(*info.m_context, info);
    managerFallback = false;
    ++it;
  }
  m_notifierMutex.Signal();

  if (managerFallback)
    manager.OnConversation(info);
}


void OpalIMEndPoint::ShutDown()
{
  PTRACE(3, "OpalIM\tShutting down all IM contexts");
  PSafePtr<OpalIMContext> context(m_contextsByConversationId, PSafeReadWrite);
  while (context != NULL)
    (context++)->Close();
}


bool OpalIMEndPoint::GarbageCollection()
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

  m_contextsByConversationId.DeleteObjectsToBeRemoved();
  return true;
}


PSafePtr<OpalIMContext> OpalIMEndPoint::FindContextByIdWithLock(const PString & id, PSafetyMode mode) 
{
  return m_contextsByConversationId.FindWithLock(id, mode);
}


PSafePtr<OpalIMContext> OpalIMEndPoint::FindContextByNamesWithLock(const PString & local, const PString & remote, PSafetyMode mode) 
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


void OpalIMEndPoint::AddNotifier(const ConversationNotifier & notifier, const PString & scheme)
{
  PWaitAndSignal mutex(m_notifierMutex);
  m_notifiers.insert(ConversationMap::value_type(scheme, notifier));
}


bool OpalIMEndPoint::RemoveNotifier(const ConversationNotifier & notifier, const PString & scheme)
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


PSafePtr<OpalIMContext> OpalIMEndPoint::FindContextForMessageWithLock(OpalIM & im, OpalConnection * conn)
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
          OpalIMEndPoint::OnRawMessageReceived(OpalIM & message,
                                               OpalConnection * connection,
                                               PString & errorInfo)
{
  PSafePtr<OpalIMContext> context = FindContextForMessageWithLock(message, connection);

  // if context found, add message to it
  if (context == NULL) {
    // create a context based on the connection
    if (connection != NULL)
      context = InternalCreateContext(connection->GetLocalPartyURL(), connection->GetRemotePartyCallbackURL(), NULL, true, &connection->GetCall());
    else
      context = InternalCreateContext(message.m_to, message.m_from, NULL, true, NULL);

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


OpalIMConnection::OpalIMConnection(OpalCall & call,
                                   OpalIMEndPoint & ep,
                                   void * userData,
                                   unsigned options,
                                   OpalConnection::StringOptions * stringOptions)
  : OpalConnection(call, ep, ep.GetManager().GetNextToken('M'), options, stringOptions)
  , m_context(reinterpret_cast<OpalIMContext *>(userData))
{
}


PBoolean OpalIMConnection::OnSetUpConnection()
{
  if (!OpalConnection::OnSetUpConnection())
    return false;

  if (!LockReadWrite())
    return false;

  OnConnectedInternal();
  UnlockReadWrite();
  return true;
}


void OpalIMConnection::OnEstablished()
{
  if (m_context != NULL) {
    m_context->InternalSend();
  }
  else {
    PString error;
    OpalIM message;
    if (((OpalIMEndPoint&)endpoint).OnRawMessageReceived(message, this, error) > OpalIMContext::DispositionErrors) {
      Release();
    }
  }

  OpalConnection::OnEstablished();
}


void OpalIMConnection::OnReleased()
{
  if (m_context != NULL)
    m_context->Close();
  OpalConnection::OnReleased();
}


#endif // OPAL_HAS_IM
