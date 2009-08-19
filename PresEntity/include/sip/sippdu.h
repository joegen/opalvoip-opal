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

#ifndef OPAL_SIP_SIPPDU_H
#define OPAL_SIP_SIPPDU_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif

#include <opal/buildopts.h>

#if OPAL_SIP

#include <ptclib/mime.h>
#include <ptclib/url.h>
#include <sip/sdp.h>
#include <opal/rtpconn.h>

 
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

    SIPURL(
      const OpalTransportAddress & _address, 
      WORD listenerPort = 0
    );

    /**Compare the two SIPURLs and return their relative rank.
       Note that does an intelligent comparison according to the rules
       in RFC3261 Section 19.1.4.

     @return
       #LessThan#, #EqualTo# or #GreaterThan#
       according to the relative rank of the objects.
     */
    virtual Comparison Compare(
      const PObject & obj   ///< Object to compare against.
    ) const;

    /** Returns complete SIPURL as one string, including displayname (in
        quotes) and address in angle brackets.
      */
    PString AsQuotedString() const;

    /** Returns display name only
      */
    PString GetDisplayName(PBoolean useDefault = PTrue) const;
    
    void SetDisplayName(const PString & str) 
      { displayName = str; }

    /**Returns the field parameter (outside of <>)
      */
    PString GetFieldParameters() const { return fieldParameters; }

    /**Returns the field parameter (outside of <>)
      */
    void SetFieldParameters(const PString & str ) { fieldParameters = str; }

    /**Get the host and port as a transport address.
      */
    OpalTransportAddress GetHostAddress() const;

    enum UsageContext {
      ExternalURI,  ///< URI used anywhere outside of protocol
      RequestURI,   ///< Request-URI (after the INVITE)
      ToURI,        ///< To header field
      FromURI,      ///< From header field
      ContactURI,   ///< Registration or Redirection Contact header field
      RouteURI,     ///< Dialog Contact header field, or Record-Route header field
      RegisterURI   ///< URI on REGISTER request line.
    };

    /** Removes tag parm & query vars and recalculates urlString
        (scheme, user, password, host, port & URI parms (like transport))
        which are not allowed in the context specified, e.g. Request-URI etc
        According to RFC3261, 19.1.1 Table 1
      */
    void Sanitise(
      UsageContext context  ///< Context for URI
    );

    /** This will adjust the current URL according to RFC3263, using DNS SRV records.

        @return FALSE if DNS is available but entry is larger than last SRV record entry,
                TRUE if DNS lookup fails or no DNS is available
      */
    PBoolean AdjustToDNS(
      PINDEX entry = 0  /// Entry in the SRV record to adjust to
    );

    /// Generate a unique string suitable as a dialog tag
    static PString GenerateTag();

    /// Set a tag with a new unique ID.
    void SetTag(
      const PString & tag = GenerateTag()
    );

  protected:
    void ParseAsAddress(const PString & name, const OpalTransportAddress & _address, WORD listenerPort = 0);

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
        but tag in URL without <> will be kept until Sanitise()
     */
    virtual PBoolean InternalParse(
      const char * cstr,
      const char * defaultScheme
    );

    PString displayName;
    PString fieldParameters;
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
    SIPMIMEInfo(bool compactForm = false);

    virtual void PrintOn(ostream & strm) const;
    virtual void ReadFrom(istream & strm);

    void SetCompactForm(bool form) { compactForm = form; }

    PCaselessString GetContentType(bool includeParameters = false) const;
    void SetContentType(const PString & v);

    PCaselessString GetContentEncoding() const;
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
    bool GetContacts(std::list<SIPURL> & contacts) const;
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
    void SetRoute(const PString & v);
    void SetRoute(const PStringList & v);

    PStringList GetRecordRoute(bool reversed) const;
    void SetRecordRoute(const PStringList & v);

    unsigned GetCSeqIndex() const { return GetCSeq().AsUnsigned(); }

    PString GetSupported() const;
    void SetSupported(const PString & v);

    PString GetUnsupported() const;
    void SetUnsupported(const PString & v);
    
    PString GetEvent() const;
    void SetEvent(const PString & v);
    
    PCaselessString GetSubscriptionState() const;
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

    PString GetRequire() const;
    void SetRequire(const PString & v, bool overwrite);

    void GetAlertInfo(PString & info, int & appearance);
    void SetAlertInfo(const PString & info, int appearance);

    /** return the value of a header field parameter, empty if none
     */
    PString GetFieldParameter(
      const PString & fieldName,    ///< Field name in dictionary
      const PString & paramName,    ///< Field parameter name
      const PString & defaultValue = PString::Empty()  ///< Default value for parameter
    ) const { return ExtractFieldParameter((*this)(fieldName), paramName, defaultValue); }

    /** set the value for a header field parameter, replace the
     *  current value, or add the parameter and its
     *  value if not already present.
     */
    void SetFieldParameter(
      const PString & fieldName,    ///< Field name in dictionary
      const PString & paramName,    ///< Field parameter name
      const PString & newValue      ///< New value for parameter
    ) { SetAt(fieldName, InsertFieldParameter((*this)(fieldName), paramName, newValue)); }

    /** return the value of a header field parameter, empty if none
     */
    static PString ExtractFieldParameter(
      const PString & fieldValue,   ///< Value of field string
      const PString & paramName,    ///< Field parameter name
      const PString & defaultValue = PString::Empty()  ///< Default value for parameter
    );

    /** set the value for a header field parameter, replace the
     *  current value, or add the parameter and its
     *  value if not already present.
     */
    static PString InsertFieldParameter(
      const PString & fieldValue,   ///< Value of field string
      const PString & paramName,    ///< Field parameter name
      const PString & newValue      ///< New value for parameter
    );

  protected:
    	/** return list of route values from internal comma-delimited list
	 */
    PStringList GetRouteList(const char * name, bool reversed) const;

	/** store string list as one comma-delimited string of route values
	    value formed as "<v[0]>,<v[1]>,<v[2]>" etc
	 */
    void SetRouteList(const char * name, const PStringList & v);

    /// Encode using compact form
    bool compactForm;
};


/////////////////////////////////////////////////////////////////////////
// SIPAuthentication

class SIPAuthentication : public PObject
{
  PCLASSINFO(SIPAuthentication, PObject);
  public:
    SIPAuthentication();

    virtual Comparison Compare(
      const PObject & other
    ) const;

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
    virtual PString GetAuthRealm() const  { return PString::Empty(); }

    virtual void SetUsername(const PString & user) { username = user; }
    virtual void SetPassword(const PString & pass) { password = pass; }
    virtual void SetAuthRealm(const PString &)     { }

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

    virtual Comparison Compare(
      const PObject & other
    ) const;

    virtual PBoolean Parse(
      const PString & auth,
      PBoolean proxy
    );

    virtual PBoolean Authorise(
      SIP_PDU & pdu
    ) const;

    virtual PString GetAuthRealm() const         { return authRealm; }
    virtual void SetAuthRealm(const PString & r) { authRealm = r; }

    enum Algorithm {
      Algorithm_MD5,
      NumAlgorithms
    };
    const PString & GetNonce() const       { return nonce; }
    Algorithm GetAlgorithm() const         { return algorithm; }
    const PString & GetOpaque() const      { return opaque; }

  protected:
    PString   authRealm;
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
      Local_TransportError,
      Local_BadTransportAddress,
      Local_Timeout,

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
      Failure_BadEvent                    = 489,
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

	static const char * GetStatusCodeDescription(int code);
    friend ostream & operator<<(ostream & strm, StatusCodes status);

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
      const char * extra = NULL,
      const SDPSessionDescription * sdp = NULL
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
    bool SetRoute(const PStringList & routeSet);
    bool SetRoute(const SIPURL & proxy);

    /**Set mime allow field to all supported methods.
      */
    void SetAllow(unsigned bitmask);

    /**Update the VIA field following RFC3261, 18.2.1 and RFC3581.
      */
    void AdjustVia(OpalTransport & transport);
    
    /**Read PDU from the specified transport.
      */
    PBoolean Read(
      OpalTransport & transport
    );

    /**Write the PDU to the transport.
      */
    PBoolean Write(
      OpalTransport & transport,
      const OpalTransportAddress & remoteAddress = OpalTransportAddress(),
      const PString & localInterface = PString::Empty()
    );

    /**Write PDU as a response to a request.
    */
    bool SendResponse(
      OpalTransport & transport,
      StatusCodes code,
      SIPEndPoint * endpoint = NULL,
      const char * contact = NULL,
      const char * extra = NULL
    );
    bool SendResponse(
      OpalTransport & transport,
      SIP_PDU & response,
      SIPEndPoint * endpoint = NULL
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
    void SetURI(const SIPURL & newuri)       { uri = newuri; }
    SDPSessionDescription * GetSDP();
    void SetSDP(SDPSessionDescription * sdp);

  protected:
    Methods     method;                 // Request type, ==NumMethods for Response
    StatusCodes statusCode;
    SIPURL      uri;                    // display name & URI, no tag
    unsigned    versionMajor;
    unsigned    versionMinor;
    PString     info;
    SIPMIMEInfo mime;
    PString     entityBody;

    SDPSessionDescription * m_SDP;

    mutable PString transactionID;

    bool m_usePeerTransportAddress;
};


PQUEUE(SIP_PDU_Queue, SIP_PDU);


#if PTRACING
ostream & operator<<(ostream & strm, SIP_PDU::Methods method);
#endif


/////////////////////////////////////////////////////////////////////////
// SIPDialogContext

/** Session Initiation Protocol dialog context information.
  */
class SIPDialogContext
{
  public:
    SIPDialogContext();

    PString AsString() const;
    bool FromString(
      const PString & str
    );

    const PString & GetCallID() const { return m_callId; }
    void SetCallID(const PString & id) { m_callId = id; }

    const SIPURL & GetRequestURI() const { return m_requestURI; }
    void SetRequestURI(const SIPURL & url) { m_requestURI = url; }
    bool SetRequestURI(const PString & uri) { return m_requestURI.Parse(uri); }

    const PString & GetLocalTag() const { return m_localTag; }
    void SetLocalTag(const PString & tag) { m_localTag = tag; }

    const SIPURL & GetLocalURI() const { return m_localURI; }
    void SetLocalURI(const SIPURL & url);
    bool SetLocalURI(const PString & uri);

    const PString & GetRemoteTag() const { return m_remoteTag; }
    void SetRemoteTag(const PString & tag) { m_remoteTag = tag; }

    const SIPURL & GetRemoteURI() const { return m_remoteURI; }
    void SetRemoteURI(const SIPURL & url);
    bool SetRemoteURI(const PString & uri);

    const PStringList & GetRouteSet() const { return m_routeSet; }
    void SetRouteSet(const PStringList & routes) { m_routeSet = routes; }
    void UpdateRouteSet(const SIPURL & proxy);

    void Update(const SIP_PDU & response);

    unsigned GetNextCSeq(unsigned inc = 1) { return m_lastSentCSeq += inc; }
    bool IsDuplicateCSeq(unsigned sequenceNumber);

    bool IsEstablished() const
    {
      return !m_callId.IsEmpty() &&
             !m_requestURI.IsEmpty() &&
             !m_localTag.IsEmpty() &&
             !m_remoteTag.IsEmpty();
    }

    bool UsePeerTransportAddress() const { return m_usePeerTransportAddress; }

  protected:
    PString     m_callId;
    SIPURL      m_requestURI;
    SIPURL      m_localURI;
    PString     m_localTag;
    SIPURL      m_remoteURI;
    PString     m_remoteTag;
    PStringList m_routeSet;
    unsigned    m_lastSentCSeq;
    unsigned    m_lastReceivedCSeq;
    bool        m_usePeerTransportAddress;
};


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

    void Construct(
      const PTimeInterval & minRetryTime = PMaxTimeInterval,
      const PTimeInterval & maxRetryTime = PMaxTimeInterval
    );
    void Construct(
      Methods method, 
      SIPDialogContext & dialog
    );

    PBoolean Start();
    bool IsTrying()     const { return state == Trying; }
    bool IsProceeding() const { return state == Proceeding; }
    bool IsInProgress() const { return state == Trying || state == Proceeding; }
    bool IsFailed()     const { return state > Terminated_Success; }
    bool IsCompleted()  const { return state >= Completed; }
    bool IsCanceled()   const { return state == Cancelling || state == Terminated_Cancelled || state == Terminated_Aborted; }
    bool IsTerminated() const { return state >= Terminated_Success; }

    void WaitForCompletion();
    PBoolean Cancel();
    void Abort();

    virtual PBoolean OnReceivedResponse(SIP_PDU & response);
    virtual PBoolean OnCompleted(SIP_PDU & response);

    OpalTransport & GetTransport() const  { return transport; }
    SIPConnection * GetConnection() const { return connection; }
    PString         GetInterface() const { return m_localInterface; }

    static PString GenerateCallID();
    
  protected:
    bool SendPDU(SIP_PDU & pdu);
    bool ResendCANCEL();

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
    PString              m_localInterface;
    OpalTransportAddress m_remoteAddress;
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
      OpalTransport & transport,
      const OpalRTPSessionManager & sm
    );

    virtual PBoolean OnReceivedResponse(SIP_PDU & response);

    const OpalRTPSessionManager & GetSessionManager() const { return rtpSessions; }
          OpalRTPSessionManager & GetSessionManager()       { return rtpSessions; }

  protected:
    OpalRTPSessionManager rtpSessions;
};


/////////////////////////////////////////////////////////////////////////

class SIPRegister : public SIPTransaction
{
    PCLASSINFO(SIPRegister, SIPTransaction);
  public:
    struct Params {
      Params();

      PString       m_addressOfRecord;
      PString       m_registrarAddress;
      PString       m_contactAddress;
      PString       m_authID;
      PString       m_password;
      PString       m_realm;
      unsigned      m_expire;
      unsigned      m_restoreTime;
      PTimeInterval m_minRetryTime;
      PTimeInterval m_maxRetryTime;
      void          * m_userData;
    };

    SIPRegister(
      SIPEndPoint   & endpoint,
      OpalTransport & transport,
      const SIPURL & proxy,
      const PString & callId,
      unsigned cseq,
      const Params & params
    );
};


/////////////////////////////////////////////////////////////////////////

class SIPSubscribe : public SIPTransaction
{
    PCLASSINFO(SIPSubscribe, SIPTransaction);
  public:
    /** Valid types for an event package
     */
    enum PredefinedPackages {
      MessageSummary,
      Presence,
      Dialog,
      NumPredefinedPackages,

      PackageMask = 0x7fff,
      Watcher     = 0x8000
    };

    class EventPackage : public PCaselessString
    {
      PCLASSINFO(EventPackage, PCaselessString);
      public:
        EventPackage(unsigned int);
        EventPackage(const PString & str);
        EventPackage(const char * cstr);
        bool operator==(PredefinedPackages) const;
        bool operator==(const PString & str) const { return Compare(str) == EqualTo; }
        bool operator==(const char * cstr) const { return InternalCompare(0, P_MAX_INDEX, cstr) == EqualTo; }
        virtual Comparison InternalCompare(PINDEX offset, PINDEX length, const char * cstr) const;

        bool IsWatcher() const   { return m_isWatcher; }

      protected:
        bool m_isWatcher;
    };

    struct Params {
      Params(unsigned pkg = NumPredefinedPackages);

      EventPackage  m_eventPackage;
      PString       m_agentAddress;
      PString       m_addressOfRecord;
      PString       m_contactAddress;
      PString       m_authID;
      PString       m_password;
      PString       m_realm;
      unsigned      m_expire;
      unsigned      m_restoreTime;
      PTimeInterval m_minRetryTime;
      PTimeInterval m_maxRetryTime;
      void          * m_userData;
      PString       m_from;
    };

    SIPSubscribe(
        SIPEndPoint & ep,
        OpalTransport & trans,
        SIPDialogContext & dialog,
        const Params & params
    );
};


typedef SIPSubscribe::EventPackage SIPEventPackage;


/////////////////////////////////////////////////////////////////////////

class SIPHandler;

class SIPEventPackageHandler
{
public:
  virtual ~SIPEventPackageHandler() { }
  virtual PCaselessString GetContentType() const = 0;
  virtual bool OnReceivedNOTIFY(SIPHandler & handler, SIP_PDU & request) = 0;
  virtual PString OnSendNOTIFY(SIPHandler & /*handler*/, const PObject * /*body*/) { return PString::Empty(); }
};


typedef PFactory<SIPEventPackageHandler, SIPEventPackage> SIPEventPackageFactory;


/////////////////////////////////////////////////////////////////////////

class SIPNotify : public SIPTransaction
{
    PCLASSINFO(SIPNotify, SIPTransaction);
  public:
    SIPNotify(
        SIPEndPoint & ep,
        OpalTransport & trans,
        SIPDialogContext & dialog,
        const SIPEventPackage & eventPackage,
        const PString & state,
        const PString & body
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
      const PString & id,
      const PString & sipIfMatch,
      SIPSubscribe::Params & params,
      const PString & body
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
      const SIPURL & proxy,
      const SIPURL & to,
      const PString & id,
      const PString & body,
      SIPURL & m_localAddress
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


#endif // OPAL_SIP

#endif // OPAL_SIP_SIPPDU_H


// End of File ///////////////////////////////////////////////////////////////
