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


class XCAPClient;


class SIPWatcherInfoCommand : public OpalPresentityCommand {
  public:
    SIPWatcherInfoCommand(bool unsubscribe = false) : m_unsubscribe(unsubscribe) { }

    bool m_unsubscribe;
};


class SIP_Presentity : public OpalPresentityWithCommandThread
{
    PCLASSINFO(SIP_Presentity, OpalPresentityWithCommandThread);

  public:
    static const PString & DefaultPresenceServerKey();
    static const PString & PresenceServerKey();

    ~SIP_Presentity();

    virtual bool Open();
    virtual bool IsOpen() const;

    SIPEndPoint & GetEndpoint() { return *m_endpoint; }

    /** Set the default presentity class for "sip" scheme.
        When OpalPresentity::Create() is called with a URI containing the "sip"
        scheme several different preentity protocols could be used. This
        determines which, for example "sip-local", "sip-xcap", "sip-oma".
        */
    static bool SetDefaultPresentity(
      const PString & prefix
    );

  protected:
    SIP_Presentity();

    virtual bool InternalOpen() = 0;

    SIPEndPoint * m_endpoint;

    int m_watcherInfoVersion;

    SIPPresenceInfo m_localPresence;
};


class SIPLocal_Presentity : public SIP_Presentity
{
    PCLASSINFO(SIPLocal_Presentity, SIP_Presentity);

  public:
    ~SIPLocal_Presentity();

    virtual bool Close();

  protected:
    virtual bool InternalOpen();
};


class SIPXCAP_Presentity : public SIP_Presentity
{
    PCLASSINFO(SIPXCAP_Presentity, SIP_Presentity);

  public:
    static const PString & XcapRootKey();
    static const PString & XcapAuthIdKey();
    static const PString & XcapPasswordKey();
    static const PString & XcapAuthAuidKey();
    static const PString & XcapAuthFileKey();

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
    bool ChangeAuthNode(XCAPClient & xcap, const OpalAuthorisationRequestCommand & cmd);

    PIPSocketAddressAndPort m_presenceServer;
    PString                 m_watcherSubscriptionAOR;

    typedef std::map<PString, PString> StringMap;
    StringMap m_watcherAorById;
    StringMap m_presenceIdByAor;
    StringMap m_presenceAorById;
    StringMap m_authorisationIdByAor;
};


class SIPOMA_Presentity : public SIPXCAP_Presentity
{
    PCLASSINFO(SIPOMA_Presentity, SIPXCAP_Presentity);

  public:
    SIPOMA_Presentity();
};


class XCAPClient : public PHTTPClient
{
  public:
    XCAPClient();

    bool GetXmlDocument(
      PXML & xml
    ) { return GetXmlDocument(m_defaultFilename, xml); }

    bool GetXmlDocument(
      const PString & name,
      PXML & xml
    );

    bool PutXmlDocument(
      const PXML & xml
    ) { return PutXmlDocument(m_defaultFilename, xml); }

    bool PutXmlDocument(
      const PString & name,
      const PXML & xml
    );

    bool DeleteXmlDocument()
    { return PutXmlDocument(m_defaultFilename); }

    bool DeleteXmlDocument(
      const PString & name
    );


    struct ElementSelector {
      ElementSelector(
        const PString & name = PString::Empty(),
        const PString & position = PString::Empty()
      ) : m_name(name)
        , m_position(position)
      { PAssert(!m_name.IsEmpty(), PInvalidParameter); }

      ElementSelector(
        const PString & name,
        const PString & attribute,
        const PString & value
      ) : m_name(name)
        , m_attribute(attribute)
        , m_value(value)
      { PAssert(!m_name.IsEmpty(), PInvalidParameter); }

      ElementSelector(
        const PString & name,
        const PString & position,
        const PString & attribute,
        const PString & value
      ) : m_name(name)
        , m_position(position)
        , m_attribute(attribute)
        , m_value(value)
      { PAssert(!m_name.IsEmpty(), PInvalidParameter); }

      PString AsString() const;

      PString m_name;
      PString m_position;
      PString m_attribute;
      PString m_value;
    };

    class NodeSelector : public std::list<ElementSelector>
    {
      public:
        NodeSelector()
          { }
        NodeSelector(
          const ElementSelector & selector
        ) { push_back(selector); }
        NodeSelector(
          const ElementSelector & selector1,
          const ElementSelector & selector2
        ) { push_back(selector1); push_back(selector2); }
        NodeSelector(
          const ElementSelector & selector1,
          const ElementSelector & selector2,
          const ElementSelector & selector3
        ) { push_back(selector1); push_back(selector2); push_back(selector3); }

        void SetElement(
          const PString & name,
          const PString & position = PString::Empty()
        ) { push_back(ElementSelector(name, position)); }

        void SetElement(
          const PString & name,
          const PString & attribute,
          const PString & value
        ) { push_back(ElementSelector(name, attribute, value)); }

        void SetElement(
          const PString & name,
          const PString & position,
          const PString & attribute,
          const PString & value
        ) { push_back(ElementSelector(name, position, attribute, value)); }

        void SetNamespace(
          const PString & space,
          const PString & alias = PString::Empty()
        ) { PAssert(!space.IsEmpty(), PInvalidParameter); m_namespaces[alias] = space; }

        void AddToURL(
          PURL & url
        ) const;

      protected:
        std::map<PString, PString> m_namespaces;
    };

    bool GetXmlNode(
      const NodeSelector & node,
      PXML & xml
    ) { return GetXmlNode(m_defaultFilename, node, xml); }

    bool GetXmlNode(
      const PString & docname,
      const NodeSelector & node,
      PXML & xml
    );

    bool PutXmlNode(
      const NodeSelector & node,
      const PXML & xml
    ) { return PutXmlNode(m_defaultFilename, node, xml); }

    bool PutXmlNode(
      const PString & docname,
      const NodeSelector & node,
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
    const PString & GetUserIdentifier() const { return m_xui; }

    void SetDefaultFilename(
      const PString & fn
    ) { m_defaultFilename = fn; }

    void SetContentType(
      const PString & type
    ) { m_contentType = type; }

  protected:
    PURL BuildURL(const PString & name, const NodeSelector & node);

    PString m_root;
    PString m_auid;
    bool    m_global;
    PString m_xui;
    PString m_defaultFilename;
    PString m_contentType;
};


#endif // P_EXPAT

#endif // OPAL_SIP_SIPPRES_H
