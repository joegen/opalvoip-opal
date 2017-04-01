/*
 * pres_ent.cxx
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
 */


#include <ptlib.h>
#include <opal_config.h>

#if OPAL_HAS_PRESENCE

#include <opal/pres_ent.h>

#include <opal/manager.h>
#include <opal/endpoint.h>
#include <ptclib/url.h>
#include <sip/sipep.h>


#define PTraceModule() "Presence"


///////////////////////////////////////////////////////////////////////

PURL_LEGACY_SCHEME(pres, true, false, true, true, false, true, true, false, false, false, 0)

const PCaselessString & OpalPresentity::AuthNameKey()     { static const PConstCaselessString s("Auth ID");       return s; }
const PCaselessString & OpalPresentity::AuthPasswordKey() { static const PConstCaselessString s("Auth Password"); return s; }
const PCaselessString & OpalPresentity::TimeToLiveKey()   { static const PConstCaselessString s("Time to Live");  return s; }


OpalPresenceInfo::OpalPresenceInfo(State state)
  : m_state(state)
{
}


OpalPresenceInfo::OpalPresenceInfo(const PString & str)
  : m_state(FromString(str))
{
  if (m_state == EndState) {
    m_state = Available;
    m_activities += str;
  }
}


PString OpalPresenceInfo::AsString() const
{
  return AsString(m_state);
}

PString OpalPresenceInfo::AsString(State state)
{
  PStringStream strm;
  strm << state;
  return strm;
}


OpalPresenceInfo::State OpalPresenceInfo::FromString(const PString & stateString)
{
  if ( stateString.IsEmpty() ||
      (stateString *= "None") ||
      (stateString *= "Offline") ||
      (stateString *= "Invisible"))
    return NoPresence;

  return stateString.As<State>(InternalError);
}


PObject::Comparison OpalPresenceInfo::Compare(const PObject & obj) const
{
  const OpalPresenceInfo & other = dynamic_cast<const OpalPresenceInfo &>(obj);
  if (m_entity < other.m_entity)
    return LessThan;
  if (m_entity > other.m_entity)
    return GreaterThan;

  if (m_target < other.m_target)
    return LessThan;
  if (m_target > other.m_target)
    return GreaterThan;

  if (m_when < other.m_when)
    return LessThan;
  if (m_when > other.m_when)
    return GreaterThan;

  return EqualTo;
}


///////////////////////////////////////////////////////////////////////

OpalPresentity::OpalPresentity()
  : m_manager(NULL)
  , m_open(false)
  , m_temporarilyUnavailable(false)
  , m_localInfo(OpalPresenceInfo::NoPresence)
{
}


OpalPresentity::OpalPresentity(const OpalPresentity & other)
  : PSafeObject(other)
  , m_manager(other.m_manager)
  , m_attributes(other.m_attributes)
  , m_open(false)
  , m_temporarilyUnavailable(false)
  , m_localInfo(OpalPresenceInfo::NoPresence)
{
}


OpalPresentity::~OpalPresentity()
{
}


void OpalPresentity::SetAOR(const PURL & aor)
{
  m_aor = aor;
}


OpalPresentity * OpalPresentity::Create(OpalManager & manager, const PURL & url, const PString & scheme)
{
  OpalPresentity * presEntity = PFactory<OpalPresentity>::CreateInstance(scheme.IsEmpty() ? url.GetScheme() : scheme);
  if (presEntity == NULL) 
    return NULL;

  presEntity->m_manager = &manager;
  presEntity->SetAOR(url);

  return presEntity;
}


bool OpalPresentity::Open()
{
  if (m_open.exchange(true))
    return false; // Already open

  PTRACE(3, m_aor << " opening.");
  return true;
}


bool OpalPresentity::Close()
{
  if (!m_open.exchange(false))
    return false; // Aleady closed

  PTRACE(3, m_aor << " closing.");
  return true;
}


bool OpalPresentity::SubscribeToPresence(const PURL & presentity, bool subscribe, const PString & note)
{
  if (!IsOpen())
    return false;

  OpalSubscribeToPresenceCommand * cmd = CreateCommand<OpalSubscribeToPresenceCommand>();
  if (cmd == NULL)
    return false;

  cmd->m_presentity = presentity;
  cmd->m_subscribe  = subscribe;
  cmd->m_note       = note;
  SendCommand(cmd);
  return true;
}


bool OpalPresentity::UnsubscribeFromPresence(const PURL & presentity)
{
  return SubscribeToPresence(presentity, false);
}


bool OpalPresentity::SetPresenceAuthorisation(const PURL & presentity, Authorisation authorisation)
{
  if (!IsOpen())
    return false;

  OpalAuthorisationRequestCommand * cmd = CreateCommand<OpalAuthorisationRequestCommand>();
  if (cmd == NULL)
    return false;

  cmd->m_presentity = presentity;
  cmd->m_authorisation = authorisation;
  SendCommand(cmd);
  return true;
}


bool OpalPresentity::SetLocalPresence(const OpalPresenceInfo & info)
{
  if (!IsOpen())
    return false;

  m_localInfo = info;

  OpalSetLocalPresenceCommand * cmd = CreateCommand<OpalSetLocalPresenceCommand>();
  if (cmd == NULL)
    return false;

  *static_cast<OpalPresenceInfo *>(cmd) = info;
  SendCommand(cmd);
  return true;
}


bool OpalPresentity::SetLocalPresence(OpalPresenceInfo::State state, const PString & note)
{
  m_localInfo.m_state = state;
  m_localInfo.m_note = note;
  return SetLocalPresence(m_localInfo);
}


bool OpalPresentity::GetLocalPresence(OpalPresenceInfo & info)
{
  info = m_localInfo;
  return true;
}


bool OpalPresentity::GetLocalPresence(OpalPresenceInfo::State & state, PString & note)
{
  if (!IsOpen())
    return false;

  state = m_localInfo.m_state;
  note  = m_localInfo.m_note;

  return true;
}


#if OPAL_HAS_IM

bool OpalPresentity::SendMessageTo(const OpalIM & message)
{
  if (!IsOpen())
    return false;

  OpalSendMessageToCommand * cmd = CreateCommand<OpalSendMessageToCommand>();
  if (cmd == NULL)
    return false;

  cmd->m_message = message;
  SendCommand(cmd);
  return true;
}


void OpalPresentity::OnReceivedMessage(const OpalIM & message)
{
  PWaitAndSignal mutex(m_notificationMutex);

  if (!m_onReceivedMessageNotifier.IsNULL())
    m_onReceivedMessageNotifier(*this, message);
}


void OpalPresentity::SetReceivedMessageNotifier(const ReceivedMessageNotifier & notifier)
{
  PWaitAndSignal mutex(m_notificationMutex);
  m_onReceivedMessageNotifier = notifier;
}

#endif // OPAL_HAS_IM


void OpalPresentity::OnAuthorisationRequest(const AuthorisationRequest & request)
{
  PWaitAndSignal mutex(m_notificationMutex);

  if (m_onAuthorisationRequestNotifier.IsNULL())
    SetPresenceAuthorisation(request.m_presentity, AuthorisationPermitted);
  else
    m_onAuthorisationRequestNotifier(*this, request);
}


void OpalPresentity::SetAuthorisationRequestNotifier(const AuthorisationRequestNotifier & notifier)
{
  PWaitAndSignal mutex(m_notificationMutex);
  m_onAuthorisationRequestNotifier = notifier;
}


void OpalPresentity::OnPresenceChange(const OpalPresenceInfo & info)
{
  PWaitAndSignal mutex(m_notificationMutex);

  if (!m_onPresenceChangeNotifier.IsNULL())
    m_onPresenceChangeNotifier(*this, std::auto_ptr<OpalPresenceInfo>(info.CloneAs<OpalPresenceInfo>()));
}


void OpalPresentity::SetPresenceChangeNotifier(const PresenceChangeNotifier & notifier)
{
  PWaitAndSignal mutex(m_notificationMutex);

  m_onPresenceChangeNotifier = notifier;
}


OpalPresentity::BuddyStatus OpalPresentity::GetBuddyListEx(BuddyList &)
{
  if (!IsOpen())
    return BuddyStatus_AccountNotLoggedIn;

  if (m_temporarilyUnavailable)
    return BuddyStatus_ListTemporarilyUnavailable;

  return BuddyStatus_ListFeatureNotImplemented;
}


OpalPresentity::BuddyStatus OpalPresentity::SetBuddyListEx(const BuddyList &)
{
  if (!IsOpen())
    return BuddyStatus_AccountNotLoggedIn;

  if (m_temporarilyUnavailable)
    return BuddyStatus_ListTemporarilyUnavailable;

  return BuddyStatus_ListFeatureNotImplemented;
}


OpalPresentity::BuddyStatus OpalPresentity::DeleteBuddyListEx()
{
  if (!IsOpen())
    return BuddyStatus_AccountNotLoggedIn;

  if (m_temporarilyUnavailable)
    return BuddyStatus_ListTemporarilyUnavailable;

  return BuddyStatus_ListFeatureNotImplemented;
}


OpalPresentity::BuddyStatus OpalPresentity::GetBuddyEx(BuddyInfo & buddy)
{
  if (!IsOpen())
    return BuddyStatus_AccountNotLoggedIn;

  if (buddy.m_presentity.IsEmpty())
    return BuddyStatus_BadBuddySpecification;

  if (m_temporarilyUnavailable)
    return BuddyStatus_ListTemporarilyUnavailable;

  BuddyList buddies;
  BuddyStatus status = GetBuddyListEx(buddies);
  if (status != BuddyStatus_OK)
    return status;

  for (BuddyList::iterator it = buddies.begin(); it != buddies.end(); ++it) {
    if (it->m_presentity == buddy.m_presentity) {
      buddy = *it;
      return BuddyStatus_OK;
    }
  }

  return BuddyStatus_SpecifiedBuddyNotFound;
}


OpalPresentity::BuddyStatus OpalPresentity::SetBuddyEx(const BuddyInfo & buddy)
{  
  if (!IsOpen())
    return BuddyStatus_AccountNotLoggedIn;

  if (m_temporarilyUnavailable)
    return BuddyStatus_ListTemporarilyUnavailable;

  if (buddy.m_presentity.IsEmpty())
    return BuddyStatus_BadBuddySpecification;

  BuddyList buddies;
  BuddyStatus status = GetBuddyListEx(buddies);
  if (status != BuddyStatus_OK)
    return status;

  buddies.push_back(buddy);
  return SetBuddyListEx(buddies);
}


OpalPresentity::BuddyStatus OpalPresentity::DeleteBuddyEx(const PURL & presentity)
{
  if (!IsOpen())
    return BuddyStatus_AccountNotLoggedIn;

  if (presentity.IsEmpty())
    return BuddyStatus_BadBuddySpecification;

  if (m_temporarilyUnavailable)
    return BuddyStatus_ListTemporarilyUnavailable;

  BuddyList buddies;
  BuddyStatus status = GetBuddyListEx(buddies);
  if (status != BuddyStatus_OK)
    return status;

  for (BuddyList::iterator it = buddies.begin(); it != buddies.end(); ++it) {
    if (it->m_presentity == presentity) {
      buddies.erase(it);
      return SetBuddyListEx(buddies);
    }
  }

  return BuddyStatus_SpecifiedBuddyNotFound;
}


OpalPresentity::BuddyStatus OpalPresentity::SubscribeBuddyListEx(PINDEX & successfulCount, bool subscribe)
{
  if (!IsOpen())
    return BuddyStatus_AccountNotLoggedIn;

  if (m_temporarilyUnavailable)
    return BuddyStatus_ListTemporarilyUnavailable;

  BuddyList buddies;
  BuddyStatus status = GetBuddyListEx(buddies);
  if (status != BuddyStatus_OK)
    return status;

  successfulCount = 0;
  for (BuddyList::iterator it = buddies.begin(); it != buddies.end(); ++it) {
    if (!SubscribeToPresence(it->m_presentity, subscribe))
      return BuddyStatus_ListSubscribeFailed;
    ++successfulCount;
  }

  return BuddyStatus_OK;
}

OpalPresentity::BuddyStatus OpalPresentity::UnsubscribeBuddyListEx()
{
  PINDEX successfulCount;
  return SubscribeBuddyListEx(successfulCount, false);
}


bool OpalPresentity::SendCommand(OpalPresentityCommand * cmd)
{
  if (cmd == NULL)
    return false;

  cmd->Process(*this);
  delete cmd;
  return true;
}


OpalPresentityCommand * OpalPresentityCommand::Create(OpalPresentity & presentity, const char * cmdName)
{
  OpalPresentityCommand * cmd = PFactory<OpalPresentityCommand>::CreateInstance(OpalPresentityCommand::MakeKey(presentity.GetClass(), cmdName));
  if (cmd == NULL && (cmd = PFactory<OpalPresentityCommand>::CreateInstance(OpalPresentityCommand::MakeKey(typeid(OpalPresentity).name(), cmdName))) == NULL) {
    PAssertAlways(PUnimplementedFunction);
    return NULL;
  }

  PTRACE(3, &presentity, "Creating presentity command '" << typeid(*cmd).name() << "'");
  return cmd;
}


PDefaultPFactoryKey OpalPresentityCommand::MakeKey(const char * className, const char * cmdName)
{
  PDefaultPFactoryKey key(className);
  key += '\t';
  key += cmdName;
  return key;
}


#if OPAL_HAS_IM

void OpalPresentity::Internal_SendMessageToCommand(const OpalSendMessageToCommand & cmd)
{
  OpalEndPoint * endpoint = m_manager->FindEndPoint(m_aor.GetScheme());
  if (endpoint == NULL) {
    PTRACE(1, "Cannot find endpoint for '" << m_aor.GetScheme() << "'");
    return;
  }

  // need writable message object
  OpalIM msg(cmd.m_message);

  if (msg.m_from.IsEmpty())
    msg.m_from = m_aor;

  endpoint->Message(msg);
}


OPAL_PRESENTITY_COMMAND(OpalSendMessageToCommand, OpalPresentity, Internal_SendMessageToCommand);

#endif // OPAL_HAS_IM


/////////////////////////////////////////////////////////////////////////////

OpalPresentityWithCommandThread::OpalPresentityWithCommandThread()
  : m_commandSequence(0)
  , m_threadRunning(false)
  , m_queueRunning(false)
  , m_thread(NULL)
{
}


OpalPresentityWithCommandThread::OpalPresentityWithCommandThread(
                           const OpalPresentityWithCommandThread & other)
  : OpalPresentity(other)
  , m_commandSequence(0)
  , m_threadRunning(false)
  , m_queueRunning(false)
  , m_thread(NULL)
{
}


OpalPresentityWithCommandThread::~OpalPresentityWithCommandThread()
{
  StopThread();

  while (!m_commandQueue.empty()) {
    delete m_commandQueue.front();
    m_commandQueue.pop();
  }
}


void OpalPresentityWithCommandThread::StartThread(bool startQueue)
{
  if (m_threadRunning)
    return;

  // start handler thread
  m_threadRunning = true;
  m_queueRunning  = startQueue;
  m_thread = new PThreadObj<OpalPresentityWithCommandThread>(*this, &OpalPresentityWithCommandThread::ThreadMain, false, "Presence");
}

void OpalPresentityWithCommandThread::StartQueue(bool startQueue)
{
  if (m_threadRunning) {
    m_queueRunning = startQueue;
    m_commandQueueSync.Signal();
  }
}

void OpalPresentityWithCommandThread::StopThread()
{
  m_threadRunning = false;
  m_commandQueueSync.Signal();
  PThread::WaitAndDelete(m_thread);
}


bool OpalPresentityWithCommandThread::SendCommand(OpalPresentityCommand * cmd)
{
  if (!m_threadRunning) {
    delete cmd;
    return false;
  }

  {
    PWaitAndSignal m(m_commandQueueMutex);
    cmd->m_sequence = ++m_commandSequence;
    m_commandQueue.push(cmd);
  }

  m_commandQueueSync.Signal();

  return true;
}



void OpalPresentityWithCommandThread::ThreadMain()
{
  PTRACE(4, "Command thread started");

  while (m_threadRunning) {
    if (m_queueRunning) {
      OpalPresentityCommand * cmd = NULL;

      {
        PWaitAndSignal mutex(m_commandQueueMutex);
        if (!m_commandQueue.empty()) {
          cmd = m_commandQueue.front();
          m_commandQueue.pop();
        }
      }
  
      if (cmd != NULL) {
        cmd->Process(*this);
        delete cmd;
      }
    }

    m_commandQueueSync.Wait(1000);
  }

  PTRACE(4, "Command thread ended");
}

#endif // OPAL_HAS_PRESENCE

/////////////////////////////////////////////////////////////////////////////
