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
 * $Revision: 22858 $
 * $Author: csoutheren $
 * $Date: 2009-06-12 22:50:19 +1000 (Fri, 12 Jun 2009) $
 */


#include <ptlib.h>
#include <opal/buildopts.h>

#include <opal/pres_ent.h>

#include <opal/manager.h>
#include <ptclib/url.h>
#include <sip/sipep.h>

const PString & OpalPresentity::AddressOfRecordKey() { static const PString s = "address_of_record"; return s; }
const PString & OpalPresentity::AuthNameKey()        { static const PString s = "auth_name";         return s; }
const PString & OpalPresentity::AuthPasswordKey()    { static const PString s = "auth_password";     return s; }
const PString & OpalPresentity::FullNameKey()        { static const PString s = "full_name";         return s; }
const PString & OpalPresentity::GUIDKey()            { static const PString s = "guid";              return s; }
const PString & OpalPresentity::SchemeKey()          { static const PString s = "scheme";            return s; }
const PString & OpalPresentity::TimeToLiveKey()      { static const PString s = "time_to_live";      return s; }


OpalPresentity::OpalPresentity()
  : m_manager(NULL)
{
  m_attributes.Set(GUIDKey, m_guid.AsString());
}


OpalPresentity * OpalPresentity::Create(OpalManager & manager, const PString & url)
{
  PString scheme = PURL(url).GetScheme();

  OpalPresentity * presEntity = PFactory<OpalPresentity>::CreateInstance(scheme);
  if (presEntity == NULL) 
    return NULL;

  presEntity->m_manager = &manager;
  presEntity->GetAttributes().Set(AddressOfRecordKey, url);

  return presEntity;
}


bool OpalPresentity::SubscribeToPresence(const PString & presentity)
{
  OpalSubscribeToPresenceCommand * cmd = CreateCommand<OpalSubscribeToPresenceCommand>();
  if (cmd == NULL)
    return false;

  cmd->m_presentity = presentity;
  SendCommand(cmd);
  return true;
}


bool OpalPresentity::UnsubscribeFromPresence(const PString & presentity)
{
  OpalSubscribeToPresenceCommand * cmd = CreateCommand<OpalSubscribeToPresenceCommand>();
  if (cmd == NULL)
    return false;

  cmd->m_presentity = presentity;
  cmd->m_subscribe = false;
  SendCommand(cmd);
  return true;
}


bool OpalPresentity::SetPresenceAuthorisation(const PString & presentity, Authorisation authorisation)
{
  OpalAuthorisationRequestCommand * cmd = CreateCommand<OpalAuthorisationRequestCommand>();
  if (cmd == NULL)
    return false;

  cmd->m_presentity = presentity;
  cmd->m_authorisation = authorisation;
  SendCommand(cmd);
  return true;
}


bool OpalPresentity::SetLocalPresence(State state, const PString & note)
{
  OpalSetLocalPresenceCommand * cmd = CreateCommand<OpalSetLocalPresenceCommand>();
  if (cmd == NULL)
    return false;

  cmd->m_state = state;
  cmd->m_note = note;
  SendCommand(cmd);
  return true;
}


void OpalPresentity::OnAuthorisationRequest(const PString & presentity)
{
  PWaitAndSignal mutex(m_notificationMutex);

  if (m_onAuthorisationRequestNotifier.IsNULL())
    SetPresenceAuthorisation(presentity, AuthorisationPermitted);
  else
    m_onAuthorisationRequestNotifier(*this, presentity);
}


void OpalPresentity::SetAuthorisationRequestNotifier(const AuthorisationRequestNotifier & notifier)
{
  PWaitAndSignal mutex(m_notificationMutex);

  m_onAuthorisationRequestNotifier = notifier;
}


void OpalPresentity::OnPresenceChange(const SIPPresenceInfo & info)
{
  PWaitAndSignal mutex(m_notificationMutex);

  if (!m_onPresenceChangeNotifier.IsNULL())
    m_onPresenceChangeNotifier(*this, info);
}


void OpalPresentity::SetPresenceChangeNotifier(const PresenceChangeNotifier & notifier)
{
  PWaitAndSignal mutex(m_notificationMutex);

  m_onPresenceChangeNotifier = notifier;
}


/////////////////////////////////////////////////////////////////////////////

OpalPresentityWithCommandThread::OpalPresentityWithCommandThread()
  : m_threadRunning(false)
  , m_thread(NULL)
{
}


OpalPresentityWithCommandThread::~OpalPresentityWithCommandThread()
{
  StopThread();

  while (!m_commandQueue.empty())
    delete m_commandQueue.front();
}


void OpalPresentityWithCommandThread::StartThread()
{
  if (m_threadRunning)
    return;

  // start handler thread
  m_threadRunning = true;
  m_thread = new PThreadObj<OpalPresentityWithCommandThread>(*this, &OpalPresentityWithCommandThread::ThreadMain);
}


void OpalPresentityWithCommandThread::StopThread()
{
  if (m_threadRunning && m_thread != NULL) {
    m_threadRunning = false;
    m_commandQueueSync.Signal();
    m_thread->WaitForTermination();
    delete m_thread;
    m_thread = NULL;
  }
}


bool OpalPresentityWithCommandThread::SendCommand(OpalPresentityCommand * cmd)
{
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
  while (m_threadRunning) {
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

    m_commandQueueSync.Wait(1000);
  }
}


/////////////////////////////////////////////////////////////////////////////
