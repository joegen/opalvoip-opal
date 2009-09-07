/*
 * sippres.h
 *
 * SIP Presence classes for Opal
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

#ifndef OPAL_SIP_SIPPRES_H
#define OPAL_SIP_SIPPRES_H

#include <ptlib.h>
#include <opal/buildopts.h>

#include <opal/pres_ent.h>
#include <ptclib/pxml.h>

class SIP_Presentity : public OpalPresentity
{
  public:
    static const char * DefaultPresenceServerKey;
    static const char * PresenceServerKey;

    SIP_Presentity();
    ~SIP_Presentity();

    virtual bool Open(
      OpalManager * manager = NULL
    );

    virtual bool IsOpen() const { return m_endpoint != NULL; }

    virtual bool Close();

    OpalPresentity::CmdSeqType SendCommand(Command * cmd);

    enum {
      e_Start                   = e_ProtocolSpecificCommand - 1,
      e_StartWatcherInfo,
      e_StopWatcherInfo
    };

  protected:
    SIPEndPoint & GetEndpoint() { return *m_endpoint; }

    virtual bool InternalOpen() = 0;
    virtual bool InternalClose() = 0;

    OpalManager * m_manager;
    SIPEndPoint * m_endpoint;

    PMutex m_commandQueueMutex;
    std::queue<Command *> m_commandQueue;
    PAtomicInteger m_commandSequence;
    PSyncPoint m_commandQueueSync;

    bool m_threadRunning;
    PThread * m_thread;

    unsigned int m_watcherInfoVersion;

    State m_localPresence;
    PString m_localPresenceNote;

    typedef std::map<std::string, PString> IdToAorMap;
    typedef std::map<std::string, PString> AorToIdMap;
    IdToAorMap m_idToAorMap;
    AorToIdMap m_aorToIdMap;
};


class SIPLocal_Presentity : public SIP_Presentity
{
  public:
    ~SIPLocal_Presentity();

  protected:
    virtual bool InternalOpen();
    virtual bool InternalClose();
};


class SIPXCAP_Presentity : public SIP_Presentity
{
  public:
    SIPXCAP_Presentity();
    ~SIPXCAP_Presentity();

    PDECLARE_NOTIFIER(SIPSubscribeHandler, SIPXCAP_Presentity, OnWatcherInfoSubscriptionStatus);
    PDECLARE_NOTIFIER(SIPSubscribeHandler, SIPXCAP_Presentity, OnWatcherInfoNotify);

    PDECLARE_NOTIFIER(SIPSubscribeHandler, SIPXCAP_Presentity, OnPresenceNotify);

    void Thread();

    virtual void OnReceivedWatcherStatus(PXMLElement * watcher);

  protected:
    virtual bool InternalOpen();
    virtual bool InternalClose();

    void SubscribeToWatcherInfo(bool on);
    void Internal_SendLocalPresence();
    void Internal_SubscribeToPresence(const PString & presentity, bool start);
    void Internal_AuthorisePresence(const PString & presentity);

    PIPSocketAddressAndPort m_presenceServer;
    bool m_watcherInfoSubscribed;

    struct PresenceInfo {
      enum SubscriptionState {
        e_Unsubscribed,
        e_Subscribing,
        e_Subscribed,
        e_Unsubscribing
      };

      PresenceInfo()
        : m_subscriptionState(e_Unsubscribed)
      { }

      SubscriptionState m_subscriptionState;
      State   m_presenceState;
      PString m_note;
      PString m_id;
    };
    typedef std::map<std::string, PresenceInfo> PresenceInfoMap;
    PresenceInfoMap m_presenceInfo;
};


#endif // OPAL_SIP_SIPPRES_H
