/*
 * main.h
 *
 * PWLib application header file for OPAL Server
 *
 * Copyright (c) 2003 Equivalence Pty. Ltd.
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
 * The Original Code is Portable Windows Library.
 *
 * The Initial Developer of the Original Code is Equivalence Pty. Ltd.
 *
 * Contributor(s): ______________________________________.
 *
 * $Revision$
 * $Author$
 * $Date$
 */

#ifndef _OPALSRV_MAIN_H
#define _OPALSRV_MAIN_H


class MyManager;

#if OPAL_H323

class MyGatekeeperServer;

class MyGatekeeperCall : public H323GatekeeperCall
{
    PCLASSINFO(MyGatekeeperCall, H323GatekeeperCall);
  public:
    MyGatekeeperCall(
      MyGatekeeperServer & server,
      const OpalGloballyUniqueID & callIdentifier, /// Unique call identifier
      Direction direction
    );
    ~MyGatekeeperCall();

    virtual H323GatekeeperRequest::Response OnAdmission(
      H323GatekeeperARQ & request
    );

#ifdef H323_TRANSNEXUS_OSP
    PBoolean AuthoriseOSPCall(H323GatekeeperARQ & info);
    OpalOSP::Transaction * ospTransaction;
#endif
};


class MyGatekeeperServer : public H323GatekeeperServer
{
    PCLASSINFO(MyGatekeeperServer, H323GatekeeperServer);
  public:
    MyGatekeeperServer(H323EndPoint & ep);

    // Overrides
    virtual H323GatekeeperCall * CreateCall(
      const OpalGloballyUniqueID & callIdentifier,
      H323GatekeeperCall::Direction direction
    );
    virtual PBoolean TranslateAliasAddress(
      const H225_AliasAddress & alias,
      H225_ArrayOf_AliasAddress & aliases,
      H323TransportAddress & address,
      PBoolean & isGkRouted,
      H323GatekeeperCall * call
    );

    // new functions
    bool Configure(PConfig & cfg, PConfigPage * rsrc);
    PString OnLoadEndPointStatus(const PString & htmlBlock);

#ifdef H323_TRANSNEXUS_OSP
    OpalOSP::Provider * GetOSPProvider() const
    { return ospProvider; }
#endif

  private:
    H323EndPoint & endpoint;

    class RouteMap : public PObject {
        PCLASSINFO(RouteMap, PObject);
      public:
        RouteMap(
          const PString & alias,
          const PString & host
        );
        RouteMap(
          const RouteMap & map
        ) : PObject(map), alias(map.alias), regex(map.alias), host(map.host) { }

        void PrintOn(
          ostream & strm
        ) const;

        bool IsValid() const;

        bool IsMatch(
          const PString & alias
        ) const;

        const H323TransportAddress & GetHost() const { return host; }

      private:
        PString              alias;
        PRegularExpression   regex;
        H323TransportAddress host;
    };
    PList<RouteMap> routes;

    PMutex reconfigurationMutex;

#ifdef H323_TRANSNEXUS_OSP
    OpalOSP::Provider * ospProvider;
    PString ospRoutingURL;
    PString ospPrivateKeyFileName;
    PString ospPublicKeyFileName;
    PString ospServerKeyFileName;
#endif
};


class MyH323EndPoint : public H323ConsoleEndPoint
{
    PCLASSINFO(MyH323EndPoint, H323ConsoleEndPoint);
  public:
    MyH323EndPoint(MyManager & mgr);

    bool Configure(PConfig & cfg, PConfigPage * rsrc);

    const MyGatekeeperServer & GetGatekeeperServer() const { return m_gkServer; }
          MyGatekeeperServer & GetGatekeeperServer()       { return m_gkServer; }

  protected:
    MyManager        & m_manager;
    MyGatekeeperServer m_gkServer;
};

#endif // OPAL_H323


#if OPAL_SIP

class MySIPEndPoint : public SIPConsoleEndPoint
{
  PCLASSINFO(MySIPEndPoint, SIPConsoleEndPoint);
public:
  MySIPEndPoint(MyManager & mgr);

  bool Configure(PConfig & cfg, PConfigPage * rsrc);

protected:
  MyManager & m_manager;
};

#endif // OPAL_SIP


class MyManager : public OpalManagerCLI
{
    PCLASSINFO(MyManager, OpalManagerCLI);
  public:
    MyManager();
    ~MyManager();

    virtual void EndRun(bool interrupt);

    bool Configure(PConfig & cfg, PConfigPage * rsrc);
#if OPAL_PTLIB_SSL
    void ConfigureSecurity(
      OpalEndPoint * ep,
      const PString & signalingKey,
      const PString & suitesKey,
      PConfig & cfg,
      PConfigPage * rsrc
    );
#endif

    virtual MediaTransferMode GetMediaTransferMode(
      const OpalConnection & source,      ///<  Source connection
      const OpalConnection & destination, ///<  Destination connection
      const OpalMediaType & mediaType     ///<  Media type for session
    ) const;
    void AdjustMediaFormats(
      bool local,
      const OpalConnection & connection,
      OpalMediaFormatList & mediaFormats
    ) const;
    virtual void OnStartMediaPatch(
      OpalConnection & connection,  ///< Connection patch is in
      OpalMediaPatch & patch        ///< Media patch being started
    );


    PString OnLoadCallStatus(const PString & htmlBlock);

#if OPAL_H323
    virtual H323ConsoleEndPoint * CreateH323EndPoint();
    MyH323EndPoint & GetH323EndPoint() const { return *FindEndPointAs<MyH323EndPoint>(OPAL_PREFIX_H323); }
#endif
#if OPAL_SIP
    virtual SIPConsoleEndPoint * CreateSIPEndPoint();
    MySIPEndPoint & GetSIPEndPoint() const { return *FindEndPointAs<MySIPEndPoint>(OPAL_PREFIX_SIP); }
#endif

  protected:
    MediaTransferMode m_mediaTransferMode;

#if OPAL_CAPI
    bool               m_enableCAPI;
#endif
#if OPAL_SCRIPT
    PString m_scriptLanguage;
    PString m_scriptText;
#endif
};


class MyProcess : public MyProcessAncestor
{
    PCLASSINFO(MyProcess, MyProcessAncestor)
  public:
    MyProcess();
    virtual PBoolean OnStart();
    virtual void OnStop();
    virtual void OnControl();
    virtual void OnConfigChanged();
    virtual PBoolean Initialise(const char * initMsg);

  protected:
    MyManager m_manager;
};


class BaseStatusPage : public PServiceHTTPString
{
    PCLASSINFO(BaseStatusPage, PServiceHTTPString);
  public:
    BaseStatusPage(MyManager & mgr, PHTTPAuthority & auth, const char * name);

    virtual PString LoadText(
      PHTTPRequest & request    // Information on this request.
    );

    virtual PBoolean Post(
      PHTTPRequest & request,
      const PStringToString &,
      PHTML & msg
    );

  protected:
    virtual const char * GetTitle() const = 0;
    virtual void CreateContent(PHTML & html) const = 0;
    virtual bool OnPostControl(const PStringToString & data, PHTML & msg) = 0;

    MyManager & m_manager;
};


#if OPAL_H323 | OPAL_SIP

class RegistrationStatusPage : public BaseStatusPage
{
    PCLASSINFO(RegistrationStatusPage, BaseStatusPage);
  public:
    RegistrationStatusPage(MyManager & mgr, PHTTPAuthority & auth);

  protected:
    virtual const char * GetTitle() const;
    virtual void CreateContent(PHTML & html) const;
    virtual bool OnPostControl(const PStringToString & data, PHTML & msg);

  friend class PServiceMacro_H323RegistrationStatus;
  friend class PServiceMacro_SIPRegistrationStatus;
};

#endif

class CallStatusPage : public BaseStatusPage
{
    PCLASSINFO(CallStatusPage, BaseStatusPage);
  public:
    CallStatusPage(MyManager & mgr, PHTTPAuthority & auth);

  protected:
    virtual const char * GetTitle() const;
    virtual void CreateContent(PHTML & html) const;
    virtual bool OnPostControl(const PStringToString & data, PHTML & msg);

  friend class PServiceMacro_CallStatus;
};


#if OPAL_H323

class GkStatusPage : public BaseStatusPage
{
    PCLASSINFO(GkStatusPage, BaseStatusPage);
  public:
    GkStatusPage(MyManager & gk, PHTTPAuthority & auth);

  protected:
    virtual const char * GetTitle() const;
    virtual void CreateContent(PHTML & html) const;
    virtual bool OnPostControl(const PStringToString & data, PHTML & msg);

    MyGatekeeperServer & m_gkServer;

  friend class PServiceMacro_EndPointStatus;
};

#endif // OPAL_H323


#endif  // _OPALSRV_MAIN_H


// End of File ///////////////////////////////////////////////////////////////
