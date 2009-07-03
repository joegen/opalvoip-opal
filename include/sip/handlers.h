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
 * $Revision$
 * $Author$
 * $Date$
 */

#ifndef OPAL_SIP_HANDLERS_H
#define OPAL_SIP_HANDLERS_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif

#ifndef _PTLIB_H
#include <ptlib.h>
#endif

#include <opal/buildopts.h>

#if OPAL_SIP

#include <ptlib/safecoll.h>

#include <opal/connection.h>
#include <sip/sippdu.h>


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
    int expireTime,
    int offlineExpire = 30,
    const PTimeInterval & retryMin = PMaxTimeInterval,
    const PTimeInterval & retryMax = PMaxTimeInterval
  );

public:
  ~SIPHandler();

  virtual bool ShutDown();

  enum State {

    Subscribed,       // The registration is active
    Subscribing,      // The registration is in process
    Unavailable,      // The registration is offline and still being attempted
    Refreshing,       // The registration is being refreshed
    Restoring,        // The registration is trying to be restored after being offline
    Unsubscribing,    // The unregistration is in process
    Unsubscribed      // The registrating is inactive
  };

  void SetState (SIPHandler::State s);

  inline SIPHandler::State GetState () 
  { return state; }

  virtual OpalTransport * GetTransport();

  virtual SIPAuthentication * GetAuthentication()
  { return authentication; }

  virtual const SIPURL & GetTargetAddress()
    { return targetAddress; }

  virtual const PString GetRemotePartyAddress();

  virtual PBoolean OnReceivedNOTIFY(SIP_PDU & response);

  virtual void SetExpire(int e);

  virtual int GetExpire()
    { return expire; }

  virtual PString GetCallID()
    { return callID; }

  virtual void SetBody(const PString & b)
    { body = b;}

  virtual SIPTransaction * CreateTransaction(OpalTransport & t) = 0;

  virtual SIP_PDU::Methods GetMethod() = 0;
  virtual PCaselessString GetEventPackage() const
  { return PString::Empty(); }

  virtual void OnReceivedAuthenticationRequired(SIPTransaction & transaction, SIP_PDU & response);
  virtual void OnReceivedOK(SIPTransaction & transaction, SIP_PDU & response);
  virtual void OnTransactionFailed(SIPTransaction & transaction);
  virtual void OnFailed(SIP_PDU::StatusCodes);

  virtual PBoolean SendRequest(SIPHandler::State state);

  const PStringList & GetRouteSet() const { return routeSet; }

  const OpalProductInfo & GetProductInfo() const { return m_productInfo; }

  const PString & GetUsername() const     { return authenticationUsername; }
  const PString & GetPassword() const     { return authenticationPassword; }
  const PString & GetRealm() const        { return authenticationAuthRealm; }

protected:
  void CollapseFork(SIPTransaction & transaction);
  PDECLARE_NOTIFIER(PTimer, SIPHandler, OnExpireTimeout);
  static PBoolean WriteSIPHandler(OpalTransport & transport, void * info);
  bool WriteSIPHandler(OpalTransport & transport);

  SIPEndPoint               & endpoint;

  SIPAuthentication           * authentication;

  PSafeList<SIPTransaction>   transactions;
  OpalTransport             * transport;
  SIPURL                      targetAddress;
  PString                     callID;
  int	                        expire;
  int                         originalExpire;
  int                         offlineExpire;
  PStringList                 routeSet;
  PString		                  body;
  unsigned                    authenticationAttempts;
  State                       state;
  PTimer                      expireTimer; 
  PTimeInterval               retryTimeoutMin; 
  PTimeInterval               retryTimeoutMax; 
  PString remotePartyAddress;
  SIPURL proxy;
  OpalProductInfo             m_productInfo;
  PString                     authenticationUsername;
  PString                     authenticationPassword;
  PString                     authenticationAuthRealm;
};

#if PTRACING
ostream & operator<<(ostream & strm, SIPHandler::State state);
#endif


class SIPRegisterHandler : public SIPHandler
{
  PCLASSINFO(SIPRegisterHandler, SIPHandler);

public:
  SIPRegisterHandler(
    SIPEndPoint & ep,
    const SIPRegister::Params & params
  );

  ~SIPRegisterHandler();

  virtual SIPTransaction * CreateTransaction(OpalTransport &);
  virtual void OnReceivedOK(SIPTransaction & transaction, SIP_PDU & response);
  virtual SIP_PDU::Methods GetMethod()
    { return SIP_PDU::Method_REGISTER; }

  virtual void OnFailed(SIP_PDU::StatusCodes r);
  virtual PBoolean SendRequest(SIPHandler::State state);

  void UpdateParameters(const SIPRegister::Params & params);

private:
  void SendStatus(SIP_PDU::StatusCodes code);

  SIPRegister::Params m_parameters;
  unsigned            m_sequenceNumber;
};


class SIPSubscribeHandler : public SIPHandler
{
  PCLASSINFO(SIPSubscribeHandler, SIPHandler);
public:
  SIPSubscribeHandler(SIPEndPoint & ep, const SIPSubscribe::Params & params);
  ~SIPSubscribeHandler();

  virtual SIPTransaction * CreateTransaction (OpalTransport &);
  virtual void OnReceivedOK(SIPTransaction & transaction, SIP_PDU & response);
  virtual PBoolean OnReceivedNOTIFY(SIP_PDU & response);
  virtual void OnFailed(SIP_PDU::StatusCodes r);
  virtual PBoolean SendRequest(SIPHandler::State state);
  virtual SIP_PDU::Methods GetMethod ()
    { return SIP_PDU::Method_SUBSCRIBE; }
  virtual PCaselessString GetEventPackage() const
    { return m_parameters.m_eventPackage; }

  void UpdateParameters(const SIPSubscribe::Params & params);

private:
  void SendStatus(SIP_PDU::StatusCodes code);
  PBoolean OnReceivedMWINOTIFY(SIP_PDU & response);
  PBoolean OnReceivedPresenceNOTIFY(SIP_PDU & response);

  SIPSubscribe::Params m_parameters;

  PString  localPartyAddress;
  PBoolean dialogCreated;
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
  virtual void OnReceivedOK(SIPTransaction & transaction, SIP_PDU & response);
  virtual SIP_PDU::Methods GetMethod()
    { return SIP_PDU::Method_PUBLISH; }
  virtual void SetBody(const PString & body);
  static PString BuildBody(const PString & to,
                           const PString & basic,
                           const PString & note);

private:
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
  virtual SIP_PDU::Methods GetMethod ()
    { return SIP_PDU::Method_MESSAGE; }
  virtual void OnFailed (SIP_PDU::StatusCodes);
  virtual void SetBody(const PString & b);

private:
  virtual void OnExpireTimeout(PTimer &, INT);
};


class SIPPingHandler : public SIPHandler
{
  PCLASSINFO(SIPPingHandler, SIPHandler);
public:
  SIPPingHandler(SIPEndPoint & ep, 
                 const PString & to);
  virtual SIPTransaction * CreateTransaction (OpalTransport &);
  virtual SIP_PDU::Methods GetMethod ()
    { return SIP_PDU::Method_MESSAGE; }
};


/** This dictionary is used both to contain the active and successful
 * registrations, and subscriptions. 
 */
class SIPHandlersList
{
  public:
    /** Append a new handler to the list
      */
    void Append(SIPHandler * handler)
      { m_handlersList.Append(handler); }

    /** Remove a handler from the list.
        Handler is not immediately deleted but marked for deletion later by
        DeleteObjectsToBeRemoved() when all references are done with the handler.
      */
    void Remove(SIPHandler * handler)
      { m_handlersList.Remove(handler); }

    /** Clean up lists of handler.
      */
    bool DeleteObjectsToBeRemoved()
      { return m_handlersList.DeleteObjectsToBeRemoved(); }

    /** Get the first handler in the list. Further enumeration may be done by
        the ++operator on the safe pointer.
     */
    PSafePtr<SIPHandler> GetFirstHandler(PSafetyMode mode = PSafeReference) const
      { return PSafePtr<SIPHandler>(m_handlersList, mode); }

    /**
     * Return the number of registered accounts
     */
    unsigned GetCount(SIP_PDU::Methods meth, const PString & eventPackage = PString::Empty()) const;

    /**
     * Return a list of the active address of records for each handler.
     */
    PStringList GetAddresses(bool includeOffline, SIP_PDU::Methods meth, const PString & eventPackage = PString::Empty()) const;

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
    PSafePtr<SIPHandler> FindSIPHandlerByUrl(const PString & url, SIP_PDU::Methods meth, const PString & eventPackage, PSafetyMode m);

    /**
     * Find the SIPHandler object with the specified registration host.
     * For example, in the above case, the name parameter
     * could be "sip.seconix.com" or "seconix.com".
     */
    PSafePtr <SIPHandler> FindSIPHandlerByDomain(const PString & name, SIP_PDU::Methods meth, PSafetyMode m);

  protected:
    PSafeList<SIPHandler> m_handlersList;
};


#endif // OPAL_SIP

#endif // OPAL_SIP_HANDLERS_H
