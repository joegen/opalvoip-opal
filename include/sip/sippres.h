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

#if P_EXPAT

#include <opal/pres_ent.h>
#include <ptclib/pxml.h>


class SIPWatcherInfoCommand : public OpalPresentityCommand {
  public:
    SIPWatcherInfoCommand(bool unsubscribe = false) : m_unsubscribe(unsubscribe) { }

    bool m_unsubscribe;
};


class SIP_Presentity : public OpalPresentityWithCommandThread
{
  public:
    static const PString & DefaultPresenceServerKey();
    static const PString & PresenceServerKey();

    ~SIP_Presentity();

    virtual bool Open();
    virtual bool IsOpen() const;

    SIPEndPoint & GetEndpoint() { return *m_endpoint; }

  protected:
    SIP_Presentity();

    virtual bool InternalOpen() = 0;

    SIPEndPoint * m_endpoint;

    int m_watcherInfoVersion;

    State   m_localPresence;
    PString m_localPresenceNote;
};


class SIPLocal_Presentity : public SIP_Presentity
{
  public:
    ~SIPLocal_Presentity();

    virtual bool Close();

  protected:
    virtual bool InternalOpen();
};


class SIPXCAP_Presentity : public SIP_Presentity
{
  public:
    static const PString & XcapRootKey();
    static const PString & XcapAuthIdKey();
    static const PString & XcapPasswordKey();

    SIPXCAP_Presentity();
    ~SIPXCAP_Presentity();

    virtual bool Close();

    virtual void OnReceivedWatcherStatus(PXMLElement * watcher);

    void Internal_SendLocalPresence(const OpalSetLocalPresenceCommand & cmd);
    void Internal_SubscribeToPresence(const OpalSubscribeToPresenceCommand & cmd);
    void Internal_AuthorisationRequest(const OpalAuthorisationRequestCommand & cmd);
    void Internal_SubscribeToWatcherInfo(const SIPWatcherInfoCommand & cmd);

  protected:
    PDECLARE_NOTIFIER2(SIPSubscribeHandler, SIPXCAP_Presentity, OnWatcherInfoSubscriptionStatus, const SIPSubscribe::SubscriptionStatus &);
    PDECLARE_NOTIFIER2(SIPSubscribeHandler, SIPXCAP_Presentity, OnWatcherInfoNotify, SIPSubscribe::NotifyCallbackInfo &);
    PDECLARE_NOTIFIER2(SIPSubscribeHandler, SIPXCAP_Presentity, OnPresenceSubscriptionStatus, const SIPSubscribe::SubscriptionStatus &);
    PDECLARE_NOTIFIER2(SIPSubscribeHandler, SIPXCAP_Presentity, OnPresenceNotify, SIPSubscribe::NotifyCallbackInfo &);

    virtual bool InternalOpen();
    unsigned GetExpiryTime() const;

    PIPSocketAddressAndPort m_presenceServer;
    PString                 m_watcherSubscriptionAOR;

    typedef std::map<PString, PString> StringMap;
    StringMap m_watcherAorById;
    StringMap m_presenceIdByAor;
    StringMap m_presenceAorById;
    StringMap m_authorisationIdByAor;
};


class XCAPClient : public PHTTPClient
{
  public:
    XCAPClient();

    bool GetXmlDocument(
      const PString & name,
      PXML & xml
    );
    bool PutXmlDocument(
      const PString & name,
      const PXML & xml
    );

    bool GetXmlNode(
      const PString & docname,
      const PString & node,
      PXML & xml
    );
    bool PutXmlNode(
      const PString & docname,
      const PString & node,
      const PXML & xml
    );

    void SetRoot(
      const PString & server
    ) { m_root = server; }

    void SetApplicationUniqueID(
      const PString & id
    ) { m_auid = id; }

    void SetGlobal() { m_global = true; }

    void SetUserIdentifier(
      const PString & id
    ) { m_global = false; m_xui = id; }

    void SetContentType(
      const PString & type
    ) { m_contentType = type; }

  protected:
    PURL BuildURL(const PString & name, const PString & node = PString::Empty());

    PString m_root;
    PString m_auid;
    bool    m_global;
    PString m_xui;
    PString m_contentType;
};


#endif // P_EXPAT

#endif // OPAL_SIP_SIPPRES_H
