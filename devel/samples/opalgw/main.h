/*
 * main.h
 *
 * PWLib application header file for OPAL Gateway
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
 * $Log: main.h,v $
 * Revision 1.2  2007/10/04 06:48:04  rjongbloed
 * High speed collision between opengk and opalgw.
 *
 * Revision 1.1  2003/03/26 02:49:00  robertj
 * Added service/daemon sample application.
 *
 */

#ifndef _OpalGw_MAIN_H
#define _OpalGw_MAIN_H


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
    BOOL AuthoriseOSPCall(H323GatekeeperARQ & info);
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
    virtual BOOL TranslateAliasAddress(
      const H225_AliasAddress & alias,
      H225_ArrayOf_AliasAddress & aliases,
      H323TransportAddress & address,
      BOOL & isGkRouted,
      H323GatekeeperCall * call
    );

    // new functions
    BOOL Initialise(PConfig & cfg, PConfigPage * rsrc);

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
        ) : alias(map.alias), regex(map.alias), host(map.host) { }

        void PrintOn(
          ostream & strm
        ) const;

        BOOL IsValid() const;

        BOOL IsMatch(
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


#endif


class MyManager : public OpalManager
{
  PCLASSINFO(MyManager, OpalManager);

  public:
    MyManager();
    ~MyManager();

    BOOL Initialise(PConfig & cfg, PConfigPage * rsrc);
#if OPAL_H323
    BOOL OnPostControl(const PStringToString & data, PHTML & msg);
    PString OnLoadEndPointStatus(const PString & htmlBlock);
    PString OnLoadCallStatus(const PString & htmlBlock);
#endif

  protected:
#if OPAL_H323
    H323EndPoint       * h323EP;
    MyGatekeeperServer * gkServer;
#endif
#if OPAL_SIP
    SIPEndPoint      * sipEP;
#endif
    OpalPOTSEndPoint * potsEP;
    OpalPSTNEndPoint * pstnEP;
#if P_EXPAT
    OpalIVREndPoint  * ivrEP;
#endif

  friend class OpalGw;
};


class OpalGw : public OpalGwProcessAncestor
{
  PCLASSINFO(OpalGw, OpalGwProcessAncestor)

  public:
    OpalGw();
    virtual void Main();
    virtual BOOL OnStart();
    virtual void OnStop();
    virtual void OnControl();
    virtual void OnConfigChanged();
    virtual BOOL Initialise(const char * initMsg);

    static OpalGw & Current() { return (OpalGw &)PProcess::Current(); }

    MyManager manager;
};


class MainStatusPage : public PServiceHTTPString
{
  PCLASSINFO(MainStatusPage, PServiceHTTPString);

  public:
    MainStatusPage(OpalGw & app, PHTTPAuthority & auth);
    
    virtual BOOL Post(
      PHTTPRequest & request,
      const PStringToString &,
      PHTML & msg
    );
  
  private:
    OpalGw & app;
};


#endif  // _OpalGw_MAIN_H


// End of File ///////////////////////////////////////////////////////////////
