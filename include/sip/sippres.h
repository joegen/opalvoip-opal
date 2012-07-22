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
 * $Revision$
 * $Author$
 * $Date$
 */

#ifndef OPAL_SIP_SIPPRES_H
#define OPAL_SIP_SIPPRES_H

#include <ptlib.h>
#include <opal/buildopts.h>
#include <sip/sipep.h>
#include <ptlib/notifier_ext.h>


#if P_EXPAT && OPAL_SIP

#include <opal/pres_ent.h>
#include <ptclib/pxml.h>


class XCAPClient : public PHTTPClient
{
  public:
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

        void AddElement(
          const PString & name,
          const PString & position = PString::Empty()
        ) { push_back(ElementSelector(name, position)); }

        void AddElement(
          const PString & name,
          const PString & attribute,
          const PString & value
        ) { push_back(ElementSelector(name, attribute, value)); }

        void AddElement(
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


    XCAPClient();

    bool GetXml(
      PXML & xml
    ) { return GetXml(BuildURL(), xml); }

    bool GetXml(
      const PURL & url,
      PXML & xml
    );

    bool PutXml(
      const PXML & xml
    ) { return PutXml(BuildURL(), xml); }

    bool PutXml(
      const PURL & url,
      const PXML & xml
    );

    bool DeleteXml() { return DeleteDocument(BuildURL()); }


    PURL BuildURL();


    void SetRoot(
      const PURL & server
    ) { m_root = server; }
    const PURL & GetRoot() const { return m_root; }

    void SetApplicationUniqueID(
      const PString & id
    ) { m_auid = id; }
    const PString & GetApplicationUniqueID() const { return m_auid; }

    void SetGlobal() { m_global = true; }
    bool IsGlobal() const { return m_global; }

    void SetUserIdentifier(
      const PString & id
    ) { m_global = false; m_xui = id; }
    const PString & GetUserIdentifier() const { return m_xui; }

    void SetFilename(
      const PString & fn
    ) { m_filename = fn; }
    const PString & GetFilename() const { return m_filename; }

    void SetNode(
      const NodeSelector & node
    ) { m_node = node; }
    const NodeSelector & GetNode() const { return m_node; }
    void ClearNode() { m_node.clear(); }

    void SetContentType(
      const PString & type
    ) { m_contentType = type; }
    const PString & GetContentType() const { return m_contentType; }

  protected:
    PURL         m_root;
    PString      m_auid;
    bool         m_global;
    PString      m_xui;
    PString      m_filename;
    NodeSelector m_node;
    PString      m_contentType;
};


class SIPWatcherInfoCommand : public OpalPresentityCommand {
  public:
    SIPWatcherInfoCommand(bool unsubscribe = false) : m_unsubscribe(unsubscribe) { }

    bool m_unsubscribe;
};


/** Class of the SIP presence systems.
  */
class SIP_Presentity : public OpalPresentityWithCommandThread, public PValidatedNotifierTarget
{
    PCLASSINFO(SIP_Presentity, OpalPresentityWithCommandThread);

  public:
    SIP_Presentity();
    SIP_Presentity(const SIP_Presentity & other);
    ~SIP_Presentity();

    virtual PObject * Clone() const { return new SIP_Presentity(*this); }

    enum SubProtocol {
      // Note order is important
      e_PeerToPeer,
      e_WithAgent,
      e_XCAP,
      e_OMA
    };

    static const PCaselessString & PIDFEntityKey();
    static const PCaselessString & SubProtocolKey();
    static const PCaselessString & PresenceAgentKey();
    static const PCaselessString & TransportKey();
    static const PCaselessString & XcapRootKey();
    static const PCaselessString & XcapAuthIdKey();
    static const PCaselessString & XcapPasswordKey();
    static const PCaselessString & XcapAuthAuidKey();
    static const PCaselessString & XcapAuthFileKey();
    static const PCaselessString & XcapBuddyListKey();

    virtual PStringArray GetAttributeNames() const;
    virtual PStringArray GetAttributeTypes() const;

    virtual bool Open();
    virtual bool Close();
    virtual BuddyStatus GetBuddyListEx(BuddyList & buddies);
    virtual BuddyStatus SetBuddyListEx(const BuddyList & buddies);
    virtual BuddyStatus DeleteBuddyListEx();
    virtual BuddyStatus GetBuddyEx(BuddyInfo & buddy);
    virtual BuddyStatus SetBuddyEx(const BuddyInfo & buddy);
    virtual BuddyStatus DeleteBuddyEx(const PURL & presentity);
    virtual BuddyStatus SubscribeBuddyListEx(PINDEX & successful, bool subscribe = true);

    SIPEndPoint & GetEndpoint() { return *m_endpoint; }

    void Internal_SendLocalPresence(const OpalSetLocalPresenceCommand & cmd);
    void Internal_SubscribeToPresence(const OpalSubscribeToPresenceCommand & cmd);
    void Internal_SubscribeToWatcherInfo(const SIPWatcherInfoCommand & cmd);
    void Internal_AuthorisationRequest(const OpalAuthorisationRequestCommand & cmd);

    unsigned GetExpiryTime() const;

  protected:
    PDECLARE_VALIDATED_NOTIFIER2(SIPSubscribeHandler, SIP_Presentity, OnPresenceSubscriptionStatus, const SIPSubscribe::SubscriptionStatus &);
    PDECLARE_VALIDATED_NOTIFIER2(SIPSubscribeHandler, SIP_Presentity, OnPresenceNotify, SIPSubscribe::NotifyCallbackInfo &);
    PDECLARE_VALIDATED_NOTIFIER2(SIPSubscribeHandler, SIP_Presentity, OnWatcherInfoSubscriptionStatus, const SIPSubscribe::SubscriptionStatus &);
    PDECLARE_VALIDATED_NOTIFIER2(SIPSubscribeHandler, SIP_Presentity, OnWatcherInfoNotify, SIPSubscribe::NotifyCallbackInfo &);
    void OnReceivedWatcherStatus(PXMLElement * watcher);
    void SetPIDFEntity(PURL & entity);
    bool ChangeAuthNode(XCAPClient & xcap, const OpalAuthorisationRequestCommand & cmd);
    void InitRootXcap(XCAPClient & xcap);
    void InitBuddyXcap(
      XCAPClient & xcap,
      const PString & entryName = PString::Empty(),
      const PString & listName = PString::Empty()
    );

    SIPEndPoint * m_endpoint;
    SubProtocol   m_subProtocol;
    PString       m_presenceAgent;
    PString       m_watcherSubscriptionAOR;
    int           m_watcherInfoVersion;
    PString       m_publishedTupleId;

    typedef std::map<PString, PString> StringMap;
    StringMap m_watcherAorById;
    StringMap m_presenceIdByAor;
    StringMap m_presenceAorById;
    StringMap m_authorisationIdByAor;

  private:
    void operator=(const SIP_Presentity &) { }
};


#endif // P_EXPAT && OPAL_SIP

#endif // OPAL_SIP_SIPPRES_H
