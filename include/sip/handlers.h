/*
 * handlers.h
 *
 * Session Initiation Protocol endpoint.
 *
 * Open Phone Abstraction Library (OPAL)
 *
 * Copyright (c) 2000 Equivalence Pty. Ltd.
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
 * The Initial Developer of the Original Code is Damien Sandras. 
 *
 * Contributor(s): ______________________________________.
 *
 * $Log: handlers.h,v $
 * Revision 1.7  2007/09/21 01:34:09  rjongbloed
 * Rewrite of SIP transaction handling to:
 *   a) use PSafeObject and safe collections
 *   b) only one database of transactions, remove connection copy
 *   c) fix timers not always firing due to bogus deadlock avoidance
 *   d) cleaning up only occurs in the existing garbage collection thread
 *   e) use of read/write mutex on endpoint list to avoid possible deadlock
 *
 * Revision 1.6  2007/09/04 11:54:49  csoutheren
 * Fixed compilation on Linux
 *
 * Revision 1.5  2007/09/04 05:42:55  rjongbloed
 * Added OnRegistrationStatus() call back function so can distinguish
 *   between initial registration and refreshes.
 * Fixed handler states so Refreshing state is actually used!
 *
 * Revision 1.4  2007/06/30 16:43:19  dsandras
 * Make sure transactions are completed before allowing destruction using
 * WaitForTransactionCompletion. If we have a timeout while unsusbscribing,
 * then allow deleting the handler.
 *
 * Revision 1.3  2007/06/10 08:55:11  rjongbloed
 * Major rework of how SIP utilises sockets, using new "socket bundling" subsystem.
 *
 * Revision 1.2  2007/05/21 04:30:30  dereksmithies
 * put #ifndef _PTLIB_H protection around the include of ptlib.h
 *
 * Revision 1.1  2007/05/15 20:45:09  dsandras
 * Added various handlers to manage subscriptions for presence, message
 * waiting indications, registrations, state publishing,
 * message conversations, ...
 * Adds/fixes support for RFC3856, RFC3903, RFC3863, RFC3265, ...
 * Many improvements over the original SIPInfo code.
 * Code contributed by NOVACOM (http://www.novacom.be) thanks to
 * EuroWeb (http://www.euroweb.hu).
 *
 *
 */

#ifndef __OPAL_HANDLERS_H
#define __OPAL_HANDLERS_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif

#ifndef _PTLIB_H
#include <ptlib.h>
#endif

#include <ptlib/safecoll.h>

#include <sip/sippdu.h>
#include <sip/sipep.h>


/* Class to handle SIP REGISTER, SUBSCRIBE, MESSAGE, and renew
 * the 'bindings' before they expire.
 */
class SIPHandler : public PSafeObject 
{
  PCLASSINFO(SIPHandler, PSafeObject);

protected:
  SIPHandler(
    SIPEndPoint & ep, 
    const PString & to,
    const PTimeInterval & retryMin = PMaxTimeInterval,
    const PTimeInterval & retryMax = PMaxTimeInterval
  );

public:
  ~SIPHandler();

  enum State {

    Subscribed,       // The registration is active
    Subscribing,      // The registration is in process
    Refreshing,       // The registration is being refreshed
    Unsubscribing,    // The unregistration is in process
    Unsubscribed      // The registrating is inactive
  };

  inline void SetState (SIPHandler::State s) 
    {
      state = s;
    }

  inline SIPHandler::State GetState () 
    {
      return state;
    }

  virtual OpalTransport &GetTransport()
    { return *transport; }

  virtual SIPAuthentication & GetAuthentication()
    { return authentication; }

  virtual const SIPURL & GetTargetAddress()
    { return targetAddress; }

  virtual const PString GetRemotePartyAddress();

  virtual PBoolean OnReceivedNOTIFY(SIP_PDU & response);

  // An expire time of -1 corresponds to an invalid SIPHandler that 
  // should be deleted.
  virtual void SetExpire(int e);

  virtual int GetExpire()
    { return expire; }

  virtual PString GetCallID()
    { return callID; }

  virtual PBoolean CanBeDeleted();

  virtual void SetAuthUser(const PString & u)
    { authUser = u;}

  virtual PString GetAuthUser() const
    { return authUser;}

  virtual void SetPassword(const PString & p)
    { password = p;}

  virtual void SetAuthRealm(const PString & r)
    { authRealm = r; authentication.SetAuthRealm(r);}

  virtual void SetBody(const PString & b)
    { body = b;}

  virtual SIPTransaction * CreateTransaction(OpalTransport & t) = 0;

  virtual SIP_PDU::Methods GetMethod() = 0;
  virtual SIPSubscribe::SubscribeType GetSubscribeType() 
    { return SIPSubscribe::Unknown; }

  virtual void OnReceivedOK(SIP_PDU & response);
  virtual void OnTransactionTimeout(SIPTransaction & transaction);
  virtual void OnFailed(SIP_PDU::StatusCodes);

  virtual PBoolean SendRequest(SIPHandler::State s = Subscribing);

  int GetAuthenticationAttempts() { return authenticationAttempts; };
  void SetAuthenticationAttempts(unsigned attempts) { authenticationAttempts = attempts; };
  const PStringList & GetRouteSet() const { return routeSet; }

protected:
  SIPEndPoint               & endpoint;
  SIPAuthentication           authentication;
  PSafePtr<SIPTransaction>    transaction;
  OpalTransport             * transport;
  SIPURL                      targetAddress;
  PString                     callID;
  int                         originalExpire;
  int	                      expire;
  PString	              authRealm;
  PString                     authUser;
  PString 	              password;
  PStringList                 routeSet;
  PString		      body;
  unsigned                    authenticationAttempts;
  State                       state;
  PTimer                      expireTimer; 
  PTimeInterval               retryTimeoutMin; 
  PTimeInterval               retryTimeoutMax; 
  PString remotePartyAddress;

private:
  static PBoolean WriteSIPHandler(
    OpalTransport & transport, 
    void * info
  );
};


class SIPRegisterHandler : public SIPHandler
{
  PCLASSINFO(SIPRegisterHandler, SIPHandler);

public:
  SIPRegisterHandler(SIPEndPoint & ep, 
                     const PString & to,
                     const PString & authName, 
                     const PString & password, 
                     const PString & realm,
                     int expire,
                     const PTimeInterval & minRetryTime, 
                     const PTimeInterval & maxRetryTime);

  ~SIPRegisterHandler();

  virtual SIPTransaction * CreateTransaction(OpalTransport &);
  virtual void OnReceivedOK(SIP_PDU & response);
  virtual void OnTransactionTimeout(SIPTransaction & transaction);
  virtual SIP_PDU::Methods GetMethod()
    { return SIP_PDU::Method_REGISTER; }

  virtual void OnFailed(SIP_PDU::StatusCodes r);
private:
  void SendStatus(SIP_PDU::StatusCodes code);
  PDECLARE_NOTIFIER(PTimer, SIPRegisterHandler, OnExpireTimeout);
};


class SIPSubscribeHandler : public SIPHandler
{
  PCLASSINFO(SIPSubscribeHandler, SIPHandler);
public:
  SIPSubscribeHandler(SIPEndPoint & ep, 
                      SIPSubscribe::SubscribeType type,
                      const PString & to,
                      int expire);
  ~SIPSubscribeHandler();

  virtual SIPTransaction * CreateTransaction (OpalTransport &);
  virtual void OnReceivedOK(SIP_PDU & response);
  virtual void OnTransactionTimeout(SIPTransaction & transaction);
  virtual PBoolean OnReceivedNOTIFY(SIP_PDU & response);
  virtual SIP_PDU::Methods GetMethod ()
    { return SIP_PDU::Method_SUBSCRIBE; }
  virtual SIPSubscribe::SubscribeType GetSubscribeType() 
    { return type; }

  virtual void OnFailed (SIP_PDU::StatusCodes);
  unsigned GetNextCSeq() { return ++lastSentCSeq; }

private:
  PDECLARE_NOTIFIER(PTimer, SIPSubscribeHandler, OnExpireTimeout);

  PBoolean OnReceivedMWINOTIFY(SIP_PDU & response);
  PBoolean OnReceivedPresenceNOTIFY(SIP_PDU & response);

  SIPSubscribe::SubscribeType type;
  PBoolean dialogCreated;
  PString localPartyAddress;
  unsigned lastSentCSeq;
  unsigned lastReceivedCSeq;
};

class SIPPublishHandler : public SIPHandler
{
  PCLASSINFO(SIPPublishHandler, SIPHandler);

public:
  SIPPublishHandler(SIPEndPoint & ep, 
                    const PString & to,
                    const PString & body,
                    int expire);
  ~SIPPublishHandler();

  virtual SIPTransaction * CreateTransaction(OpalTransport &);
  virtual void OnReceivedOK(SIP_PDU & response);
  virtual void OnTransactionTimeout(SIPTransaction & transaction);
  virtual SIP_PDU::Methods GetMethod()
    { return SIP_PDU::Method_PUBLISH; }
  virtual void OnFailed(SIP_PDU::StatusCodes r);
  virtual void SetBody(const PString & body);
  static PString BuildBody(const PString & to,
                           const PString & basic,
                           const PString & note);

private:
  PDECLARE_NOTIFIER(PTimer, SIPPublishHandler, OnExpireTimeout);
  PDECLARE_NOTIFIER(PTimer, SIPPublishHandler, OnPublishTimeout);
  PTimer publishTimer;
  PString sipETag;
  PBoolean stateChanged;
};

class SIPMessageHandler : public SIPHandler
{
  PCLASSINFO(SIPMessageHandler, SIPHandler);
public:
  SIPMessageHandler(SIPEndPoint & ep, 
                    const PString & to,
                    const PString & body);
  ~SIPMessageHandler();

  virtual SIPTransaction * CreateTransaction (OpalTransport &);
  virtual void OnReceivedOK(SIP_PDU & response);
  virtual void OnTransactionTimeout(SIPTransaction & transaction);
  virtual SIP_PDU::Methods GetMethod ()
    { return SIP_PDU::Method_MESSAGE; }
  virtual void OnFailed (SIP_PDU::StatusCodes);

private:
  PDECLARE_NOTIFIER(PTimer, SIPMessageHandler, OnExpireTimeout);
  unsigned timeoutRetry;
};

class SIPPingHandler : public SIPHandler
{
  PCLASSINFO(SIPPingHandler, SIPHandler);
public:
  SIPPingHandler(SIPEndPoint & ep, 
                 const PString & to);
  virtual SIPTransaction * CreateTransaction (OpalTransport &);
  virtual void OnReceivedOK(SIP_PDU & response);
  virtual void OnTransactionTimeout(SIPTransaction & transaction);
  virtual SIP_PDU::Methods GetMethod ()
    { return SIP_PDU::Method_MESSAGE; }
  virtual void OnFailed (SIP_PDU::StatusCodes);

private:
  PDECLARE_NOTIFIER(PTimer, SIPPingHandler, OnExpireTimeout);
};


/** This dictionary is used both to contain the active and successful
 * registrations, and subscriptions. Currently, only MWI subscriptions
 * are supported.
 */
class SIPHandlersList : public PSafeList<SIPHandler>
{
public:
    /**
     * Return the number of registered accounts
     */
    unsigned GetRegistrationsCount();

    /**
     * Find the SIPHandler object with the specified callID
     */
    PSafePtr<SIPHandler> FindSIPHandlerByCallID(const PString & callID, PSafetyMode m);

    /**
     * Find the SIPHandler object with the specified authRealm
     */
    PSafePtr<SIPHandler> FindSIPHandlerByAuthRealm(const PString & authRealm, const PString & userName, PSafetyMode m);

    /**
     * Find the SIPHandler object with the specified URL. The url is
     * the registration address, for example, 6001@sip.seconix.com
     * when registering 6001 to sip.seconix.com with realm seconix.com
     * or 6001@seconix.com when registering 6001@seconix.com to
     * sip.seconix.com
     */
    PSafePtr<SIPHandler> FindSIPHandlerByUrl(const PString & url, SIP_PDU::Methods meth, PSafetyMode m);
    PSafePtr<SIPHandler> FindSIPHandlerByUrl(const PString & url, SIP_PDU::Methods meth, SIPSubscribe::SubscribeType type, PSafetyMode m);

    /**
     * Find the SIPHandler object with the specified registration host.
     * For example, in the above case, the name parameter
     * could be "sip.seconix.com" or "seconix.com".
     */
    PSafePtr <SIPHandler> FindSIPHandlerByDomain(const PString & name, SIP_PDU::Methods meth, PSafetyMode m);
};


#endif
