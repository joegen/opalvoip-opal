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
 * $Revision$
 * $Author$
 * $Date$
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
class OpalProductInfo;

class SIPEndPoint;
class SIPConnection;
class SIP_PDU;


/////////////////////////////////////////////////////////////////////////
// SIPURL

/** This class extends PURL to include displayname, optional "<>" delimiters
	and extended parameters - like tag.
	It may be used for From:, To: and Contact: lines.
 */

class SIPURL : public PURL
{
  PCLASSINFO(SIPURL, PURL);
  public:
    SIPURL();

    /** str goes straight to Parse()
      */
    SIPURL(
      const char * cstr,    ///<  C string representation of the URL.
      const char * defaultScheme = NULL ///<  Default scheme for URL
    );

    /** str goes straight to Parse()
      */
    SIPURL(
      const PString & str,  ///<  String representation of the URL.
      const char * defaultScheme = NULL ///<  Default scheme for URL
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
    PString GetDisplayName(PBoolean useDefault = PTrue) const;
    
    void SetDisplayName(const PString & str) 
      { displayName = str; }
    
    OpalTransportAddress GetHostAddress() const;

    /** Removes tag parm & query vars and recalculates urlString
        (scheme, user, password, host, port & URI parms (like transport))
      */
    void AdjustForRequestURI();

    /** This will adjust the current URL according to RFC3263, using DNS SRV records.

        @return FALSE if DNS is available but entry is larger than last SRV record entry,
                TRUE if DNS lookup fails or no DNS is available
      */
    PBoolean AdjustToDNS(
      PINDEX entry = 0  /// Entry in the SRV record to adjust to
    );

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
    virtual PBoolean InternalParse(
      const char * cstr,
      const char * defaultScheme
    );

    PString displayName;
};


/////////////////////////////////////////////////////////////////////////
// SIPMIMEInfo

/** Session Initiation Protocol MIME info container
   This is a string dictionary: for each item mime header is key, value
   is value.
   Headers may be full ("From") or compact ("f"). Colons not included.
   PMIMEInfo::ReadFrom (>>) parses from stream. That adds a header-value
   element for each mime line. If a mime header is duplicated in the
   stream then the additional value is appended to the existing, 
   separated by "/n".
   PMIMEInfo::ReadFrom supports multi-line values if the next line starts
   with a space - it just appends the next line to the existing string
   with the separating space.
   There is no checking of header names or values.
   compactForm decides whether 'Set' methods store full or compact headers.
   'Set' methods replace values, there is no method for appending except
   ReadFrom.
   'Get' methods work whether stored headers are full or compact.

   to do to satisfy RFC3261 (mandatory(*) & should):
    Accept
    Accept-Encoding
    Accept-Language
   *Allow
   *Max-Forwards
   *Min-Expires
   *Proxy-Authenticate
    Supported
   *Unsupported
   *WWW-Authenticate
 */

class SIPMIMEInfo : public PMIMEInfo
{
  PCLASSINFO(SIPMIMEInfo, PMIMEInfo);
  public:
    SIPMIMEInfo(PBoolean compactForm = PFalse);

    void SetForm(PBoolean v) { compactForm = v; }

    PString GetContentType() const;
    void SetContentType(const PString & v);

    PString GetContentEncoding() const;
    void SetContentEncoding(const PString & v);

    PString GetFrom() const;
    void SetFrom(const PString & v);

    PString GetPAssertedIdentity() const;
    void SetPAssertedIdentity(const PString & v);

    PString GetPPreferredIdentity() const;
    void SetPPreferredIdentity(const PString & v);

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

    PStringList GetViaList() const;
    void SetViaList(const PStringList & v);

    PString GetReferTo() const;
    void SetReferTo(const PString & r);

    PString GetReferredBy() const;
    void SetReferredBy(const PString & r);

    PINDEX  GetContentLength() const;
    void SetContentLength(PINDEX v);
		PBoolean IsContentLengthPresent() const;

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
    
    PString GetEvent() const;
    void SetEvent(const PString & v);
    
    PString GetSubscriptionState() const;
    void SetSubscriptionState(const PString & v);
    
    PString GetUserAgent() const;
    void SetUserAgent(const PString & v);

    PString GetOrganization() const;
    void SetOrganization(const PString & v);

    void GetProductInfo(OpalProductInfo & info);
    void SetProductInfo(const PString & ua, const OpalProductInfo & info);

    PString GetWWWAuthenticate() const;
    void SetWWWAuthenticate(const PString & v);

    PString GetSIPIfMatch() const;
    void SetSIPIfMatch(const PString & v);

    PString GetSIPETag() const;
    void SetSIPETag(const PString & v);

    /** return the value of a header field parameter, empty if none
     */
    PString GetFieldParameter(const PString &,
			      const PString &);
    
    /** set the value for a header field parameter, replace the
     *  current value, or add the parameter and its
     *  value if not already present.
     */
    void SetFieldParameter(const PString &,
			   PString &,
			   const PString &);
    
    /** return PTrue if the header field parameter is present
     */
    PBoolean HasFieldParameter(const PString &,
			   const PString &);

  protected:
    	/** return list of route values from internal comma-delimited list
	 */
    PStringList GetRouteList(const char * name) const;

	/** store string list as one comma-delimited string of route values
	    value formed as "<v[0]>,<v[1]>,<v[2]>" etc
	 */
    void SetRouteList(const char * name, const PStringList & v);

	/** return string keyed by full or compact header
	 */
    PString GetFullOrCompact(const char * fullForm, char compactForm) const;

    /// Encode using compact form
    PBoolean compactForm;
};


/////////////////////////////////////////////////////////////////////////
// SIPAuthentication

class SIPAuthentication : public PObject
{
  PCLASSINFO(SIPAuthentication, PObject);
  public:
    SIPAuthentication();

    virtual bool EquivalentTo(
      const SIPAuthentication & _oldAuth
    ) = 0;

    virtual PBoolean Parse(
      const PString & auth,
      PBoolean proxy
    ) = 0;

    virtual PBoolean Authorise(
      SIP_PDU & pdu
    ) const =  0;

    virtual PBoolean IsProxy() const               { return isProxy; }

    virtual PString GetUsername() const   { return username; }
    virtual PString GetPassword() const   { return password; }
    virtual PString GetAuthRealm() const  { return authRealm; }

    virtual void SetUsername(const PString & user) { username = user; }
    virtual void SetPassword(const PString & pass) { password = pass; }
    virtual void SetAuthRealm(const PString & r)   { authRealm = r; }

    PString GetAuthParam(const PString & auth, const char * name) const;
    PString AsHex(PMessageDigest5::Code & digest) const;
    PString AsHex(const PBYTEArray & data) const;

    static SIPAuthentication * ParseAuthenticationRequired(bool isProxy,
                                                const PString & line,
                                                      PString & errorMsg);

  protected:
    PBoolean  isProxy;

    PString   username;
    PString   password;
    PString   authRealm;
};

typedef PFactory<SIPAuthentication> SIPAuthenticationFactory;

/////////////////////////////////////////////////////////////////////////

class SIPDigestAuthentication : public SIPAuthentication
{
  PCLASSINFO(SIPDigestAuthentication, SIPAuthentication);
  public:
    SIPDigestAuthentication();

    SIPDigestAuthentication & operator =(
      const SIPDigestAuthentication & auth
    );

    bool EquivalentTo(
      const SIPAuthentication & _oldAuth
    );

    PBoolean Parse(
      const PString & auth,
      PBoolean proxy
    );

    PBoolean Authorise(
      SIP_PDU & pdu
    ) const;

    enum Algorithm {
      Algorithm_MD5,
      NumAlgorithms
    };
    const PString & GetNonce() const       { return nonce; }
    Algorithm GetAlgorithm() const         { return algorithm; }
    const PString & GetOpaque() const      { return opaque; }

  protected:
    PString   nonce;
    Algorithm algorithm;
    PString   opaque;

    PBoolean qopAuth;
    PBoolean qopAuthInt;
    PString cnonce;
    mutable PAtomicInteger nonceCount;
};

/////////////////////////////////////////////////////////////////////////
// SIP_PDU

/** Session Initiation Protocol message.
	Each message contains a header, MIME lines and possibly SDP.
	Class provides methods for reading from and writing to transport.
 */

class SIP_PDU : public PSafeObject
{
  PCLASSINFO(SIP_PDU, PSafeObject);
  public:
    enum Methods {
      Method_INVITE,
      Method_ACK,
      Method_OPTIONS,
      Method_BYE,
      Method_CANCEL,
      Method_REGISTER,
      Method_SUBSCRIBE,
      Method_NOTIFY,
      Method_REFER,
      Method_MESSAGE,
      Method_INFO,
      Method_PING,
      Method_PUBLISH,
      NumMethods
    };

    enum StatusCodes {
      IllegalStatusCode,

      Information_Trying                  = 100,
      Information_Ringing                 = 180,
      Information_CallForwarded           = 181,
      Information_Queued                  = 182,
      Information_Session_Progress        = 183,

      Successful_OK                       = 200,
      Successful_Accepted		          = 202,

      Redirection_MultipleChoices         = 300,
      Redirection_MovedPermanently        = 301,
      Redirection_MovedTemporarily        = 302,
      Redirection_UseProxy                = 305,
      Redirection_AlternativeService      = 380,

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
      Failure_RequestEntityTooLarge       = 413,
      Failure_RequestURITooLong           = 414,
      Failure_UnsupportedMediaType        = 415,
      Failure_UnsupportedURIScheme        = 416,
      Failure_BadExtension                = 420,
      Failure_ExtensionRequired           = 421,
      Failure_IntervalTooBrief            = 423,
      Failure_TemporarilyUnavailable      = 480,
      Failure_TransactionDoesNotExist     = 481,
      Failure_LoopDetected                = 482,
      Failure_TooManyHops                 = 483,
      Failure_AddressIncomplete           = 484,
      Failure_Ambiguous                   = 485,
      Failure_BusyHere                    = 486,
      Failure_RequestTerminated           = 487,
      Failure_NotAcceptableHere           = 488,
      Failure_BadEvent			              = 489,
      Failure_RequestPending              = 491,
      Failure_Undecipherable              = 493,

      Failure_InternalServerError         = 500,
      Failure_NotImplemented              = 501,
      Failure_BadGateway                  = 502,
      Failure_ServiceUnavailable          = 503,
      Failure_ServerTimeout               = 504,
      Failure_SIPVersionNotSupported      = 505,
      Failure_MessageTooLarge             = 513,

      GlobalFailure_BusyEverywhere        = 600,
      GlobalFailure_Decline               = 603,
      GlobalFailure_DoesNotExistAnywhere  = 604,
      GlobalFailure_NotAcceptable         = 606,

      MaxStatusCode                       = 699
    };

	static const char * GetStatusCodeDescription (int code);

    enum {
      MaxSize = 65535
    };

    SIP_PDU();

    /** Construct a Request message
     */
    SIP_PDU(
      Methods method,
      const SIPURL & dest,
      const PString & to,
      const PString & from,
      const PString & callID,
      unsigned cseq,
      const OpalTransportAddress & via
    );
    /** Construct a Request message for requests in a dialog
     */
    SIP_PDU(
      Methods method,
      SIPConnection & connection,
      const OpalTransport & transport
    );

    /** Construct a Response message
        extra is passed as message body
     */
    SIP_PDU(
      const SIP_PDU & request,
      StatusCodes code,
      const char * contact = NULL,
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

    /**Add and populate Route header following the given routeSet.
      If first route is strict, exchange with URI.
      Returns PTrue if routeSet.
      */
    PBoolean SetRoute(const PStringList & routeSet);

    /**Set mime allow field to all supported methods.
      */
    void SetAllow(void);

    /**Update the VIA field following RFC3261, 18.2.1 and RFC3581.
      */
    void AdjustVia(OpalTransport & transport);
    
    /**Return the address from the via field. That address
     * should be used to send responses to incoming PDUs.
     */
    OpalTransportAddress GetViaAddress(OpalEndPoint &);
    
    /**Return the address to which the request PDU should be sent
     * according to the RFC, for a request in a dialog.
     */
    OpalTransportAddress GetSendAddress(const PStringList & routeSet);
    
    /**Read PDU from the specified transport.
      */
    PBoolean Read(
      OpalTransport & transport
    );

    /**Write the PDU to the transport.
      */
    PBoolean Write(
      OpalTransport & transport,
      const OpalTransportAddress & remoteAddress = OpalTransportAddress()
    );

    /** Construct the PDU string to output.
        Returns the total length of the PDU.
      */
    PString Build();

    PString GetTransactionID() const;

    Methods GetMethod() const                { return method; }
    StatusCodes GetStatusCode () const       { return statusCode; }
    const SIPURL & GetURI() const            { return uri; }
    unsigned GetVersionMajor() const         { return versionMajor; }
    unsigned GetVersionMinor() const         { return versionMinor; }
    const PString & GetEntityBody() const    { return entityBody; }
          PString & GetEntityBody()          { return entityBody; }
    const PString & GetInfo() const          { return info; }
    const SIPMIMEInfo & GetMIME() const      { return mime; }
          SIPMIMEInfo & GetMIME()            { return mime; }
    PBoolean HasSDP() const                      { return sdp != NULL; }
    SDPSessionDescription & GetSDP() const   { return *PAssertNULL(sdp); }
    void SetURI(const SIPURL & newuri)       { uri = newuri; }
    void SetSDP(SDPSessionDescription * s)   { sdp = s; }
    void SetSDP(const SDPSessionDescription & s) { sdp = new SDPSessionDescription(s); }

  protected:
    
    Methods     method;                 // Request type, ==NumMethods for Response
    StatusCodes statusCode;
    SIPURL      uri;                    // display name & URI, no tag
    unsigned    versionMajor;
    unsigned    versionMinor;
    PString     info;
    SIPMIMEInfo mime;
    PString     entityBody;

    OpalTransportAddress    lastTransportAddress;
    SDPSessionDescription * sdp;
};


PQUEUE(SIP_PDU_Queue, SIP_PDU);


#if PTRACING
ostream & operator<<(ostream & strm, SIP_PDU::Methods method);
#endif


/////////////////////////////////////////////////////////////////////////
// SIPTransaction

/** Session Initiation Protocol transaction.
    A transaction is a stateful independent entity that provides services to
    a connection (Transaction User). Transactions are contained within 
    connections.
    A client transaction handles sending a request and receiving its
    responses.
    A server transaction handles sending responses to a received request.
    In either case the SIP_PDU ancestor is the sent or received request.
 */

class SIPTransaction : public SIP_PDU
{
    PCLASSINFO(SIPTransaction, SIP_PDU);
  public:
    SIPTransaction(
      SIPEndPoint   & endpoint,
      OpalTransport & transport,
      const PTimeInterval & minRetryTime = PMaxTimeInterval, 
      const PTimeInterval & maxRetryTime = PMaxTimeInterval
    );
    /** Construct a transaction for requests in a dialog.
     *  The transport is used to determine the local address
     */
    SIPTransaction(
      SIPConnection & connection,
      OpalTransport & transport,
      Methods method = NumMethods
    );
    ~SIPTransaction();

    PBoolean Start();
    PBoolean IsInProgress() const { return state == Trying || state == Proceeding; }
    PBoolean IsFailed() const { return state > Terminated_Success; }
    PBoolean IsCompleted() const { return state >= Completed; }
    PBoolean IsCanceled() const { return state == Cancelling || state == Terminated_Cancelled || state == Terminated_Aborted; }
    PBoolean IsTerminated() const { return state >= Terminated_Success; }

    void WaitForCompletion();
    PBoolean Cancel();
    void Abort();

    virtual PBoolean OnReceivedResponse(SIP_PDU & response);
    virtual PBoolean OnCompleted(SIP_PDU & response);

    OpalTransport & GetTransport() const  { return transport; }
    SIPConnection * GetConnection() const { return connection; }
    PString         GetInterface() const { return localInterface; }

  protected:
    void Construct(
      const PTimeInterval & minRetryTime = PMaxTimeInterval,
      const PTimeInterval & maxRetryTime = PMaxTimeInterval
    );
    bool SendPDU(SIP_PDU & pdu);
    bool ResendCANCEL();

    PDECLARE_NOTIFIER(PTimer, SIPTransaction, OnRetry);
    PDECLARE_NOTIFIER(PTimer, SIPTransaction, OnTimeout);

    enum States {
      NotStarted,
      Trying,
      Aborting,
      Proceeding,
      Cancelling,
      Completed,
      Terminated_Success,
      Terminated_Timeout,
      Terminated_RetriesExceeded,
      Terminated_TransportError,
      Terminated_Cancelled,
      Terminated_Aborted,
      NumStates
    };
    virtual void SetTerminated(States newState);

    SIPEndPoint           & endpoint;
    OpalTransport         & transport;
    PSafePtr<SIPConnection> connection;
    PTimeInterval           retryTimeoutMin; 
    PTimeInterval           retryTimeoutMax; 

    States     state;
    unsigned   retry;
    PTimer     retryTimer;
    PTimer     completionTimer;
    PSyncPoint completed;
    PString    localInterface;
};


/////////////////////////////////////////////////////////////////////////
// SIPInvite

/** Session Initiation Protocol transaction for INVITE
    INVITE implements a three-way handshake to handle the human input and 
    extended duration of the transaction.
 */

class SIPInvite : public SIPTransaction
{
    PCLASSINFO(SIPInvite, SIPTransaction);
  public:
    SIPInvite(
      SIPConnection & connection,
      OpalTransport & transport
    );
    SIPInvite(
      SIPConnection & connection,
      OpalTransport & transport,
      RTP_SessionManager & sm
    );

    virtual PBoolean OnReceivedResponse(SIP_PDU & response);

    RTP_SessionManager & GetSessionManager() { return rtpSessions; }

  protected:
    RTP_SessionManager rtpSessions;
};


/////////////////////////////////////////////////////////////////////////

class SIPRegister : public SIPTransaction
{
    PCLASSINFO(SIPRegister, SIPTransaction);
  public:
    struct Params {
      Params();

      PString       m_addressOfRecord;
      PString       m_contactAddress;
      PString       m_authID;
      PString       m_password;
      PString       m_realm;
      unsigned      m_expire;
      PTimeInterval m_minRetryTime;
      PTimeInterval m_maxRetryTime;
    };

    SIPRegister(
      SIPEndPoint   & endpoint,
      OpalTransport & transport,
      const PStringList & routeSet,
      const PString & id,
      const Params & params
    );
};


/////////////////////////////////////////////////////////////////////////

class SIPSubscribe : public SIPTransaction
{
    PCLASSINFO(SIPSubscribe, SIPTransaction);
  public:
    /** Valid types for a presence event
     */
    enum SubscribeType {
      Unknown,
      MessageSummary,
      Presence
    };

    /** Valid types for a MWI
    */
    enum MWIType { 
      
      VoiceMessage, 
      FaxMessage, 
      PagerMessage, 
      MultimediaMessage, 
      TextMessage, 
      None 
    };
    SIPSubscribe(
        SIPEndPoint & ep,
        OpalTransport & trans,
        SIPSubscribe::SubscribeType & type,
        const PStringList & routeSet,
        const SIPURL & targetAddress,
        const PString & remotePartyAddress,
        const PString & localPartyAddress,
        const PString & id,
        const unsigned & cseq,
        unsigned expires
    );
};


/////////////////////////////////////////////////////////////////////////

class SIPPublish : public SIPTransaction
{
    PCLASSINFO(SIPPublish, SIPTransaction);
  public:
    SIPPublish(
      SIPEndPoint & ep,
      OpalTransport & trans,
      const PStringList & routeSet,
      const SIPURL & targetAddress,
      const PString & sipIfMatch,
      const PString & body,
      unsigned expires
    );
};


/////////////////////////////////////////////////////////////////////////

class SIPRefer : public SIPTransaction
{
  PCLASSINFO(SIPRefer, SIPTransaction);
  public:
    SIPRefer(
      SIPConnection & connection,
      OpalTransport & transport,
      const SIPURL & refer
    );
    SIPRefer(
      SIPConnection & connection,
      OpalTransport & transport,
      const SIPURL & refer,
      const SIPURL & referred_by
    );
  protected:
    void Construct(
      SIPConnection & connection,
      OpalTransport & transport,
      const SIPURL & refer,
      const SIPURL & referred_by
    );
};


/////////////////////////////////////////////////////////////////////////

/* This is not a generic NOTIFY PDU, but the minimal one
 * that gets sent when receiving a REFER
 */
class SIPReferNotify : public SIPTransaction
{
    PCLASSINFO(SIPReferNotify, SIPTransaction);
  public:
    SIPReferNotify(
      SIPConnection & connection,
      OpalTransport & transport,
      StatusCodes code
    );
};


/////////////////////////////////////////////////////////////////////////

/* This is a MESSAGE PDU, with a body
 */
class SIPMessage : public SIPTransaction
{
    PCLASSINFO(SIPMessage, SIPTransaction);
    
  public:
    SIPMessage(
	       SIPEndPoint & ep,
	       OpalTransport & trans,
	       const SIPURL & to,
               const PStringList & routeSet,
	       const PString & body
    );
};


/////////////////////////////////////////////////////////////////////////

/* This is the ACK request sent when receiving a response to an outgoing
 * INVITE.
 */
class SIPAck : public SIP_PDU
{
    PCLASSINFO(SIPAck, SIP_PDU);
  public:
    SIPAck(
      SIPTransaction & invite,
      SIP_PDU & response
    );
};


/////////////////////////////////////////////////////////////////////////

/* This is an OPTIONS request
 */
class SIPOptions : public SIPTransaction
{
    PCLASSINFO(SIPOptions, SIPTransaction);
    
  public:
    SIPOptions(
        SIPEndPoint & ep,
      OpalTransport & trans,
       const SIPURL & address
    );
};


/////////////////////////////////////////////////////////////////////////

/* This is a PING PDU, with a body
 */
class SIPPing : public SIPTransaction
{
  PCLASSINFO(SIPPing, SIPTransaction);

  public:
    SIPPing(
               SIPEndPoint & ep,
             OpalTransport & trans,
              const SIPURL & address,
              const PString & body = PString::Empty()
   );
};


#endif // __OPAL_SIPPDU_H


// End of File ///////////////////////////////////////////////////////////////
