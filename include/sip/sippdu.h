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
 * Revision 1.2016  2004/03/14 10:14:13  rjongbloed
 * Changes to REGISTER to support authentication
 *
 * Revision 2.14  2004/03/14 08:34:09  csoutheren
 * Added ability to set User-Agent string
 *
 * Revision 2.13  2004/03/13 06:32:17  rjongbloed
 * Fixes for removal of SIP and H.323 subsystems.
 * More registration work.
 *
 * Revision 2.12  2004/03/09 12:09:55  rjongbloed
 * More work on SIP register.
 *
 * Revision 2.11  2003/12/16 10:22:45  rjongbloed
 * Applied enhancements, thank you very much Ted Szoczei
 *
 * Revision 2.10  2003/12/15 11:56:17  rjongbloed
 * Applied numerous bug fixes, thank you very much Ted Szoczei
 *
 * Revision 2.9  2002/09/16 02:52:35  robertj
 * Added #define so can select if #pragma interface/implementation is used on
 *   platform basis (eg MacOS) rather than compiler, thanks Robert Monaghan.
 *
 * Revision 2.8  2002/04/16 07:53:15  robertj
 * Changes to support calls through proxies.
 *
 * Revision 2.7  2002/04/12 12:23:03  robertj
 * Allowed for endpoint listener that is not on port 5060.
 *
 * Revision 2.6  2002/04/10 08:12:17  robertj
 * Added call back for when transaction completed, used for invite descendant.
 *
 * Revision 2.5  2002/04/10 03:16:02  robertj
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

#ifdef P_USE_PRAGMA
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

    /** str goes straight to Parse()
      */
    SIPURL(
      const char * cstr,    /// C string representation of the URL.
      const char * defaultScheme = NULL /// Default scheme for URL
    );

    /** str goes straight to Parse()
      */
    SIPURL(
      const PString & str,  /// String representation of the URL.
      const char * defaultScheme = NULL /// Default scheme for URL
    );

    /** If name does not start with 'sip' then construct URI in the form
          sip:name@host:port;transport=transport
        where host comes from address,
        port is listenerPort or port from address if that was 0
        transport is udp unless address specified tcp
        Send name starting with 'sip' or constructed URI to Parse()
     */
    SIPURL(
      const PString & name,
      const OpalTransportAddress & address,
      WORD listenerPort = 0
    );

    /** Returns complete SIPURL as one string, including displayname (in
        quotes) and address in angle brackets.
      */
    PString AsQuotedString() const;

    /** Returns display name only
      */
    PString GetDisplayName() const
      { return displayName; }
    
    void SetDisplayName(const PString & str) 
      { displayName = str; }
    
    OpalTransportAddress GetHostAddress() const;

    /** Removes tag parm & query vars and recalculates urlString
        (scheme, user, password, host, port & URI parms (like transport))
      */
    void AdjustForRequestURI();

  protected:
    /** Parses name-addr, like:
        "displayname"<scheme:user:password@host:port;transport=type>;tag=value
        into:
        displayname (quotes around name are optional, all before '<' is used)
        scheme
        username
        password
        hostname
        port
        pathStr
        path
        paramVars
        queryVars
        fragment

        Note that tag parameter outside of <> will be lost,
        but tag in URL without <> will be kept until AdjustForRequestURI
     */
    virtual BOOL InternalParse(
      const char * cstr,
      const char * defaultScheme
    );

    PString displayName;
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

    PString GetAccept() const;
    void SetAccept(const PString & v);

    PString GetAcceptEncoding() const;
    void SetAcceptEncoding(const PString & v);

    PString GetAcceptLanguage() const;
    void SetAcceptLanguage(const PString & v);

    PString GetAllow() const;
    void SetAllow(const PString & v);

    PString GetCallID() const;
    void SetCallID(const PString & v);

    PString GetContact() const;
    void SetContact(const PString & v);
    void SetContact(const SIPURL & url);

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

    PString GetDate() const;
    void SetDate(const PString & v);
    void SetDate(const PTime & t);
    void SetDate(void); // set to current date

    unsigned GetExpires(unsigned dflt = UINT_MAX) const;// returns default value if not found
    void SetExpires(unsigned v);

    PINDEX GetMaxForwards() const;
    void SetMaxForwards(PINDEX v);

    PINDEX GetMinExpires() const;
    void SetMinExpires(PINDEX v);

    PString GetProxyAuthenticate() const;
    void SetProxyAuthenticate(const PString & v);

    PStringList GetRoute() const;
    void SetRoute(const PStringList & v);

    PStringList GetRecordRoute() const;
    void SetRecordRoute(const PStringList & v);

    unsigned GetCSeqIndex() const { return GetCSeq().AsUnsigned(); }

    PString GetSupported() const;
    void SetSupported(const PString & v);

    PString GetUnsupported() const;
    void SetUnsupported(const PString & v);

    PString GetUserAgent() const;
    void SetUserAgent(const SIPEndPoint & sipep);        // normally "OPAL/2.0"

    PString GetWWWAuthenticate() const;
    void SetWWWAuthenticate(const PString & v);

  protected:
    PStringList GetRouteList(const char * name) const;
    void SetRouteList(const char * name, const PStringList & v);
    PString GetFullOrCompact(const char * fullForm, char compactForm) const;

    /// Encode using compact form
    BOOL compactForm;
};


/////////////////////////////////////////////////////////////////////////

class SIPAuthentication : public PObject
{
    PCLASSINFO(SIPAuthentication, PObject);
  public:
    SIPAuthentication(
      const PString & username = PString::Empty(),
      const PString & password = PString::Empty()
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
      const SIPURL & dest,
      const PString & to,
      const PString & from,
      const PString & callID,
      unsigned cseq,
      const OpalTransportAddress & via
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
      const SIPURL & dest,
      const PString & to,
      const PString & from,
      const PString & callID,
      unsigned cseq,
      const OpalTransportAddress & via
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

    BOOL Start();
    BOOL IsInProgress() const { return state == Trying && state == Proceeding; }
    BOOL IsFailed() const { return state > Terminated_Success; }
    BOOL IsFinished()     { return finished.Wait(0); }
    void Wait();
    BOOL SendCANCEL();

    virtual BOOL OnReceivedResponse(SIP_PDU & response);
    virtual BOOL OnCompleted(SIP_PDU & response);

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
    virtual BOOL OnCompleted(SIP_PDU & response);

    RTP_SessionManager & GetSessionManager() { return rtpSessions; }

  protected:
    RTP_SessionManager rtpSessions;
};


/////////////////////////////////////////////////////////////////////////

class SIPRegister : public SIPTransaction
{
    PCLASSINFO(SIPRegister, SIPTransaction);
  public:
    SIPRegister(
      SIPEndPoint   & endpoint,
      OpalTransport & transport,
      const SIPURL & address,
      const PString & id
    );
};


#endif // __OPAL_SIPPDU_H


// End of File ///////////////////////////////////////////////////////////////
