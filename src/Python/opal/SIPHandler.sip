/*
 * Python bindings.
 *
 * Open Phone Abstraction Library (OPAL)
 *
 * Copyright (c) 2011 Demetrius Cassidy
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
 * The Original Code is Open Phone Abstraction Library (OPAL)
 *
 * The Initial Developer of the Original Code is Demetrius Cassidy
 *
 * Contributor(s): ______________________________________.
 *
 * $Revision$
 * $Author$
 * $Date$
 */

%ModuleHeaderCode
#include <ptlib.h>
#include <sip/handlers.h>
%End


/* Class to handle SIP REGISTER, SUBSCRIBE, MESSAGE, and renew
 * the 'bindings' before they expire.
 */
class SIPHandler : PSafeObject
{

protected:
  SIPHandler(
	SIP_PDU::Methods method,
    SIPEndPoint & ep,
    const SIPParameters & params
  );

public:
  ~SIPHandler();

  virtual PObject::Comparison Compare(const PObject & other) const;

  virtual bool ShutDown();

  enum State {
    Subscribed,       // The registration is active
    Subscribing,      // The registration is in process
    Unavailable,      // The registration is offline and still being attempted
    Refreshing,       // The registration is being refreshed
    Restoring,        // The registration is trying to be restored after being offline
    Unsubscribing,    // The unregistration is in process
    Unsubscribed,     // The registrating is inactive
    NumStates
  };

  void SetState (SIPHandler::State s);

  SIPHandler::State GetState ();

  virtual OpalTransport * GetTransport();

  virtual SIPAuthentication * GetAuthentication();

  virtual const SIPURL & GetAddressOfRecord();

  virtual PBoolean OnReceivedNOTIFY(SIP_PDU & response);

  virtual void SetExpire(int e);

  virtual int GetExpire();

  virtual PString GetCallID();

  virtual void SetBody(const PString & b);

  virtual bool IsDuplicateCSeq(unsigned );

  virtual SIPTransaction * CreateTransaction(OpalTransport & t) = 0;

  virtual SIP_PDU::Methods GetMethod();
  virtual SIPSubscribe::EventPackage GetEventPackage() const;
  
  virtual void OnReceivedResponse(SIPTransaction & transaction, SIP_PDU & response);
  virtual void OnReceivedIntervalTooBrief(SIPTransaction & transaction, SIP_PDU & response);
  virtual void OnReceivedTemporarilyUnavailable(SIPTransaction & transaction, SIP_PDU & response);
  virtual void OnReceivedAuthenticationRequired(SIPTransaction & transaction, SIP_PDU & response);
  virtual void OnReceivedOK(SIPTransaction & transaction, SIP_PDU & response);
  virtual void OnTransactionFailed(SIPTransaction & transaction);

  virtual void OnFailed(const SIP_PDU & response);
  virtual void OnFailed(SIP_PDU::StatusCodes);

  bool ActivateState(SIPHandler::State state);
  virtual bool SendNotify(const PObject * /*body*/);

  SIPEndPoint & GetEndPoint() const;

  const OpalProductInfo & GetProductInfo() const;

  const PString & GetUsername() const;
  const PString & GetPassword() const;
  const PString & GetRealm() const;
  const SIPURL & GetRemoteAddress() const;
  const SIPURL & GetProxy() const;

  SIPMIMEInfo m_mime;

protected:
  virtual PBoolean SendRequest(SIPHandler::State state);
  void RetryLater(unsigned after);
  static PBoolean WriteSIPHandler(OpalTransport & transport, void * info);
  virtual bool WriteSIPHandler(OpalTransport & transport, bool forked);
};


class SIPRegisterHandler : SIPHandler
{
public:
  SIPRegisterHandler(
    SIPEndPoint & ep,
    const SIPRegister::Params & params
  );

  virtual SIPTransaction * CreateTransaction(OpalTransport &);
  virtual void OnReceivedOK(SIPTransaction & transaction, SIP_PDU & response);

  virtual void OnFailed(SIP_PDU::StatusCodes r);

  void UpdateParameters(const SIPRegister::Params & params);

  const SIPRegister::Params & GetParams() const;

  const std::list<SIPURL> & GetContacts() const;

protected:
  virtual PBoolean SendRequest(SIPHandler::State state);
  void SendStatus(SIP_PDU::StatusCodes code, State state);
};


class SIPSubscribeHandler : SIPHandler
{
public:
  SIPSubscribeHandler(SIPEndPoint & ep, const SIPSubscribe::Params & params);
  ~SIPSubscribeHandler();

  virtual SIPTransaction * CreateTransaction (OpalTransport &);
  virtual void OnReceivedOK(SIPTransaction & transaction, SIP_PDU & response);
  virtual PBoolean OnReceivedNOTIFY(SIP_PDU & response);
  virtual void OnFailed(const SIP_PDU & response);
  virtual SIPEventPackage GetEventPackage() const;

  void UpdateParameters(const SIPSubscribe::Params & params);

  virtual bool IsDuplicateCSeq(unsigned sequenceNumber);

  const SIPSubscribe::Params & GetParams() const;

protected:
  virtual PBoolean SendRequest(SIPHandler::State state);
  void SendStatus(SIP_PDU::StatusCodes code, State state);
  bool DispatchNOTIFY(SIP_PDU & request, SIP_PDU & response);
};


class SIPNotifyHandler : SIPHandler
{
public:
  SIPNotifyHandler(
    SIPEndPoint & ep,
    const PString & targetAddress,
    const SIPEventPackage & eventPackage,
    const SIPDialogContext & dialog
  );
  ~SIPNotifyHandler();

  virtual SIPTransaction * CreateTransaction(OpalTransport &);
  virtual SIPEventPackage GetEventPackage() const;
  virtual void SetBody(const PString & body);

  virtual bool IsDuplicateCSeq(unsigned sequenceNumber);
  virtual bool SendNotify(const PObject * body);

  enum Reasons {
    Deactivated,
    Probation,
    Rejected,
    Timeout,
    GiveUp,
    NoResource
  };

protected:
  virtual PBoolean SendRequest(SIPHandler::State state);
};


class SIPPublishHandler : SIPHandler
{

public:
  SIPPublishHandler(SIPEndPoint & ep, 
                    const SIPSubscribe::Params & params,
                    const PString & body);
					
  virtual void SetBody(const PString & body);

  virtual SIPTransaction * CreateTransaction(OpalTransport &);
  virtual void OnReceivedOK(SIPTransaction & transaction, SIP_PDU & response);
  virtual SIPEventPackage GetEventPackage() const;
};


class SIPMessageHandler : SIPHandler
{
public:
  SIPMessageHandler(SIPEndPoint & ep, const SIPMessage::Params & params);
                    
   virtual void SetBody(const PString & body);

  virtual SIPTransaction * CreateTransaction (OpalTransport &);
  virtual void OnFailed (SIP_PDU::StatusCodes);
  virtual void OnExpireTimeout(PTimer &, INT);
  virtual void OnReceivedOK(SIPTransaction & transaction, SIP_PDU & response);
  const PString & GetLocalAddress() const;
  const PString & GetIdentifier() const;
};

class SIPOptionsHandler : SIPHandler
{
	public:
	  SIPOptionsHandler(SIPEndPoint & ep, const SIPOptions::Params & params);
	  virtual void SetBody(const PString & body);
	  virtual SIPTransaction * CreateTransaction (OpalTransport &);
	  virtual void OnFailed(SIP_PDU::StatusCodes);
	  virtual void OnFailed(const SIP_PDU & response /Out/);
	  virtual void OnExpireTimeout(PTimer &, INT);
	  virtual void OnReceivedOK(SIPTransaction & transaction, SIP_PDU & response);
};

class SIPPingHandler : SIPHandler
{
public:
  SIPPingHandler(SIPEndPoint & ep, const PURL & to);
  virtual SIPTransaction * CreateTransaction (OpalTransport &);
};


/** This dictionary is used both to contain the active and successful
 * registrations, and subscriptions. 
 */
class SIPHandlersList
{
  public:
    /** Append a new handler to the list
      */
    void Append(SIPHandler * handler);

    /** Remove a handler from the list.
        Handler is not immediately deleted but marked for deletion later by
        DeleteObjectsToBeRemoved() when all references are done with the handler.
      */
    void Remove(SIPHandler * handler);

    /** Update indexes for handler in the list
      */
    void Update(SIPHandler * handler);

    /** Clean up lists of handler.
      */
    bool DeleteObjectsToBeRemoved();

    /** Get the first handler in the list. Further enumeration may be done by
        the ++operator on the safe pointer.
     */
    // PSafePtr<SIPHandler> GetFirstHandler(PSafetyMode mode = PSafeReference) const;

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
    // PSafePtr<SIPHandler> FindSIPHandlerByCallID(const PString & callID, PSafetyMode m);

    /**
     * Find the SIPHandler object with the specified authRealm
     */
    // PSafePtr<SIPHandler> FindSIPHandlerByAuthRealm(const PString & authRealm, const PString & userName, PSafetyMode m);

    /**
     * Find the SIPHandler object with the specified URL. The url is
     * the registration address, for example, 6001@sip.seconix.com
     * when registering 6001 to sip.seconix.com with realm seconix.com
     * or 6001@seconix.com when registering 6001@seconix.com to
     * sip.seconix.com
     */
    // PSafePtr<SIPHandler> FindSIPHandlerByUrl(const PURL & url, SIP_PDU::Methods meth, PSafetyMode m);
    // PSafePtr<SIPHandler> FindSIPHandlerByUrl(const PURL & url, SIP_PDU::Methods meth, const PString & eventPackage, PSafetyMode m);

    /**
     * Find the SIPHandler object with the specified registration host.
     * For example, in the above case, the name parameter
     * could be "sip.seconix.com" or "seconix.com".
     */
    // PSafePtr<SIPHandler> FindSIPHandlerByDomain(const PString & name, SIP_PDU::Methods meth, PSafetyMode m);

  protected:
    void RemoveIndexes(SIPHandler * handler);
};


/** Information for SIP "presence" event package notification messages.
  */
class SIPPresenceInfo : OpalPresenceInfo
{
public:
  SIPPresenceInfo(
    const PString & personId = PString::Empty(),
    State state = Unchanged
  );

  // basic presence defined by RFC 3863
  PString m_tupleId;
  PString m_contact;

  // presence extensions defined by RFC 4480
  PStringArray m_activities;  // list of activities, seperated by newline

  // presence agent
  PString m_presenceAgent;

  PString AsXML() const;

  static State FromSIPActivityString(const PString & str);

  static bool AsSIPActivityString(State state, PString & str);
  bool AsSIPActivityString(PString & str) const;

  PString m_personId;
};


/** Information for SIP "dialog" event package notification messages.
  */
struct SIPDialogNotification : PObject
{

  enum States {
    Terminated,
    Trying,
    Proceeding,
    Early,
    Confirmed,

    FirstState = Terminated,
    LastState = Confirmed
  };

  static PString GetStateName(States state);
  PString GetStateName() const;

  enum Events {
    NoEvent = -1,
    Cancelled,
    Rejected,
    Replaced,
    LocalBye,
    RemoteBye,
    Error,
    Timeout,

    FirstEvent = Cancelled,
    LastEvent = Timeout
  };

  static PString GetEventName(Events state);
  PString GetEventName() const;

  enum Rendering {
    RenderingUnknown = -1,
    NotRenderingMedia,
    RenderingMedia
  };

  PString  m_entity;
  PString  m_dialogId;
  PString  m_callId;
  bool     m_initiator;
  States   m_state;
  Events   m_eventType;
  unsigned m_eventCode;
  
  struct Participant {
    Participant();
    PString   m_URI;
    PString   m_dialogTag;
    PString   m_identity;
    PString   m_display;
    int       m_appearance;
    bool      m_byeless;
    SIPDialogNotification::Rendering m_rendering;
  };
  
  Participant m_local;
  Participant m_remote;

  SIPDialogNotification(const PString & entity = PString::Empty());

};

