/*
 * sippdu.h
 *
 * Session Initiation Protocol PDU support.
 *
 * Open Phone Abstraction Library (OPAL)
 * Formally known as the Open H323 project.
 *
 * Copyright (c) 2002 Equivalence Pty. Ltd.
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
 * The Initial Developer of the Original Code is Equivalence Pty. Ltd.
 *
 * Contributor(s): ______________________________________.
 *
 * $Log: sippdu.h,v $
 * Revision 1.2006  2002/04/10 03:16:02  robertj
 * Major changes to RTP session management when initiating an INVITE.
 * Improvements in error handling and transaction cancelling.
 *
 * Revision 2.4  2002/04/09 01:02:14  robertj
 * Fixed problems with restarting INVITE on  authentication required response.
 *
 * Revision 2.3  2002/04/05 10:42:04  robertj
 * Major changes to support transactions (UDP timeouts and retries).
 *
 * Revision 2.2  2002/03/08 06:28:19  craigs
 * Changed to allow Authorisation to be included in other PDUs
 *
 * Revision 2.1  2002/02/01 04:53:01  robertj
 * Added (very primitive!) SIP support.
 *
 */

#ifndef __OPAL_SIPPDU_H
#define __OPAL_SIPPDU_H

#ifdef __GNUC__
#pragma interface
#endif


#include <ptclib/mime.h>
#include <ptclib/url.h>
#include <sip/sdp.h>


class OpalTransport;
class OpalTransportAddress;

class SIPEndPoint;
class SIPConnection;
class SIP_PDU;


/////////////////////////////////////////////////////////////////////////

class SIPURL : public PURL
{
  PCLASSINFO(SIPURL, PURL);
  public:
    SIPURL();
    SIPURL(
      const char * str
    );
    SIPURL(
      const PString & str
    );
    SIPURL(
      const PString & name,
      const OpalTransportAddress & address,
      WORD listenerPort = 0
    );

    void Parse(
      const char * cstr
    );

    PString GetDisplayAddress() const
      { return displayAddress; }
    
    void SetDisplayAddress(const PString & str) 
      { displayAddress = str; }
    
    OpalTransportAddress GetHostAddress() const;

  protected:
    PString displayAddress;
};


/////////////////////////////////////////////////////////////////////////

/**Session Initiation Protocol MIME info container
 */

class SIPMIMEInfo : public PMIMEInfo
{
  PCLASSINFO(SIPMIMEInfo, PMIMEInfo);
  public:
    SIPMIMEInfo(BOOL compactForm = FALSE);

    void SetForm(BOOL v) { compactForm = v; }

    PString GetContentType() const;
    void SetContentType(const PString & v);

    PString GetContentEncoding() const;
    void SetContentEncoding(const PString & v);

    PString GetFrom() const;
    void SetFrom(const PString & v);

    PString GetCallID() const;
    void SetCallID(const PString & v);

    PString GetContact() const;
    void SetContact(const PString & v);
    void SetContact(const PURL & url, const char * name = NULL);

    PString GetSubject() const;
    void SetSubject(const PString & v);

    PString GetTo() const;
    void SetTo(const PString & v);

    PString GetVia() const;
    void SetVia(const PString & v);

    PINDEX  GetContentLength() const;
    void SetContentLength(PINDEX v);

    PString GetCSeq() const;
    void SetCSeq(const PString & v);

    PString GetRecordRoute() const;
    void SetRecordRoute(const PString & v);

    unsigned GetCSeqIndex() const { return GetCSeq().AsUnsigned(); }

  protected:
    PString GetSIPString(const char * fullForm, char compactForm) const;

    BOOL compactForm;
};


/////////////////////////////////////////////////////////////////////////

class SIPAuthentication : public PObject
{
    PCLASSINFO(SIPAuthentication, PObject);
  public:
    SIPAuthentication(
      const PString & username,
      const PString & password
    );

    BOOL Parse(
      const PCaselessString & auth,
      BOOL proxy
    );

    BOOL IsValid() const;

    BOOL Authorise(
      SIP_PDU & pdu
    ) const;

    enum Algorithm {
      Algorithm_MD5,
      NumAlgorithms
    };

    BOOL IsProxy() const                { return isProxy; }
    const PString & GetRealm() const    { return realm; }
    const PString & GetUsername() const { return username; }
    const PString & GetPassword() const { return password; }
    const PString & GetNonce() const    { return nonce; }
    Algorithm GetAlgorithm() const      { return algorithm; }

    void SetUsername(const PString & user) { username = user; }
    void SetPassword(const PString & pass) { password = pass; }

  protected:
    BOOL      isProxy;
    PString   realm;
    PString   username;
    PString   password;
    PString   nonce;
    Algorithm algorithm;
};


/////////////////////////////////////////////////////////////////////////

class SIP_PDU : public PObject
{
  PCLASSINFO(SIP_PDU, PObject);
  public:
    enum Methods {
      Method_INVITE,
      Method_ACK,
      Method_OPTIONS,
      Method_BYE,
      Method_CANCEL,
      Method_REGISTER,
      NumMethods
    };

    enum StatusCodes {
      IllegalStatusCode,

      Information_Trying                       = 100,
      Information_Ringing                      = 180,
      Information_CallForwarded                = 181,
      Information_Queued                       = 182,
      Information_Session_Progress             = 183,

      Successful_OK                            = 200,

      Redirection_MovedTemporarily             = 302,

      Failure_BadRequest                  = 400,
      Failure_UnAuthorised                = 401,
      Failure_PaymentRequired             = 402,
      Failure_Forbidden                   = 403,
      Failure_NotFound                    = 404,
      Failure_MethodNotAllowed            = 405,
      Failure_NotAcceptable               = 406,
      Failure_ProxyAuthenticationRequired = 407,
      Failure_RequestTimeout              = 408,
      Failure_Conflict                    = 409,
      Failure_Gone                        = 410,
      Failure_LengthRequired              = 411,
      Failure_RequestEntityTooLarge       = 412,
      Failure_RequestURITooLong           = 413,
      Failure_UnsupportedMediaType        = 414,
      Failure_BadExtension                = 420,
      Failure_TemporarilyUnavailable      = 480,
      Failure_TransactionDoesNotExist     = 481,
      Failure_LoopDetected                = 482,
      Failure_TooManyHops                 = 483,
      Failure_AddressIncomplete           = 484,
      Failure_Ambiguous                   = 485,
      Failure_BusyHere                    = 486,
      Failure_RequestTerminated           = 487,

      Failure_BadGateway                  = 502,

      Failure_Decline                     = 603,

      MaxStatusCode                       = 699
    };

    enum {
      MaxSize = 65535
    };

    SIP_PDU();
    SIP_PDU(
      Methods method,
      const PString & to,
      const PString & from,
      const PString & callID,
      unsigned cseq,
      const OpalTransport & via
    );
    SIP_PDU(
      Methods method,
      SIPConnection & connection,
      const OpalTransport & transport
    );
    SIP_PDU(
      const SIP_PDU & request,
      StatusCodes code,
      const char * extra = NULL
    );
    SIP_PDU(const SIP_PDU &);
    SIP_PDU & operator=(const SIP_PDU &);
    ~SIP_PDU();

    void PrintOn(
      ostream & strm
    ) const;

    void Construct(
      Methods method
    );
    void Construct(
      Methods method,
      const PString & to,
      const PString & from,
      const PString & callID,
      unsigned cseq,
      const OpalTransport & via
    );
    void Construct(
      Methods method,
      SIPConnection & connection,
      const OpalTransport & transport
    );

    /**Read PDU from the specified transport.
      */
    BOOL Read(
      OpalTransport & transport
    );

    /**Write the PDU to the transport.
      */
    BOOL Write(
      OpalTransport & transport
    );

    PString GetTransactionID() const;

    Methods GetMethod() const                { return method; }
    StatusCodes GetStatusCode () const       { return statusCode; }
    const SIPURL & GetURI() const            { return uri; }
    unsigned GetVersionMajor() const         { return versionMajor; }
    unsigned GetVersionMinor() const         { return versionMinor; }
    const PString & GetEntityBody() const    { return entityBody; }
    const PString & GetInfo() const          { return info; }
    const SIPMIMEInfo & GetMIME() const      { return mime; }
          SIPMIMEInfo & GetMIME()            { return mime; }
    BOOL HasSDP() const                      { return sdp != NULL; }
    SDPSessionDescription & GetSDP() const   { return *PAssertNULL(sdp); }
    void SetSDP(SDPSessionDescription * s)   { sdp = s; }
    void SetSDP(const SDPSessionDescription & s) { sdp = new SDPSessionDescription(s); }

  protected:
    Methods     method;
    StatusCodes statusCode;
    SIPURL      uri;
    unsigned    versionMajor;
    unsigned    versionMinor;
    PString     info;
    SIPMIMEInfo mime;
    PString     entityBody;

    SDPSessionDescription * sdp;
};


PQUEUE(SIP_PDU_Queue, SIP_PDU);


/////////////////////////////////////////////////////////////////////////

class SIPTransaction : public SIP_PDU
{
    PCLASSINFO(SIPTransaction, SIP_PDU);
  public:
    SIPTransaction(
      SIPEndPoint   & endpoint,
      OpalTransport & transport
    );
    SIPTransaction(
      SIPConnection & connection,
      OpalTransport & transport,
      Methods method = NumMethods
    );
    ~SIPTransaction();

    void BuildREGISTER(
      const SIPURL & name,
      const SIPURL & contact
    );

    BOOL Start();
    BOOL IsInProgress() const { return state == Trying && state == Proceeding; }
    BOOL IsFailed() const { return state > Terminated_Success; }
    BOOL IsFinished()     { return finished.Wait(0); }
    void Wait();
    BOOL SendCANCEL();

    virtual BOOL OnReceivedResponse(SIP_PDU & response);

    OpalTransport & GetTransport() const  { return transport; }
    SIPConnection * GetConnection() const { return connection; }

    const OpalTransportAddress & GetLocalAddress() const { return localAddress; }

  protected:
    void Construct();
    BOOL ResendCANCEL();

    PDECLARE_NOTIFIER(PTimer, SIPTransaction, OnRetry);
    PDECLARE_NOTIFIER(PTimer, SIPTransaction, OnTimeout);

    enum States {
      NotStarted,
      Trying,
      Proceeding,
      Cancelling,
      Completed,
      Terminated_Success,
      Terminated_Timeout,
      Terminated_RetriesExceeded,
      Terminated_TransportError,
      Terminated_Cancelled,
      NumStates
    };
    void SetTerminated(States newState);

    SIPEndPoint   & endpoint;
    OpalTransport & transport;
    SIPConnection * connection;

    States   state;
    unsigned retry;
    PTimer   retryTimer;
    PTimer   completionTimer;

    PSyncPoint finished;
    PMutex     mutex;

    OpalTransportAddress localAddress;
};


PLIST(SIPTransactionList, SIPTransaction);
PDICTIONARY(SIPTransactionDict, PString, SIPTransaction);


/////////////////////////////////////////////////////////////////////////

class SIPInvite : public SIPTransaction
{
    PCLASSINFO(SIPInvite, SIPTransaction);
  public:
    SIPInvite(
      SIPConnection & connection,
      OpalTransport & transport
    );

    virtual BOOL OnReceivedResponse(SIP_PDU & response);

    RTP_SessionManager & GetSessionManager() { return rtpSessions; }

  protected:
    RTP_SessionManager rtpSessions;
};


#endif // __OPAL_SIPPDU_H


// End of File ///////////////////////////////////////////////////////////////
