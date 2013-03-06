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
#include <ptclib/http.h>
#include <ptclib/pxml.h>
#include <ptclib/threadpool.h>
#include <opal/transports.h>
#include <rtp/rtpconn.h>

 
class OpalMediaFormatList;
class OpalProductInfo;

class SIPEndPoint;
class SIPConnection;
class SIPHandler;
class SIP_PDU;
class SIPSubscribeHandler;
class SIPDialogContext;
class SIPMIMEInfo;
class SIPTransaction;
class SDPSessionDescription;


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
    static const WORD DefaultPort;
    static const WORD DefaultSecurePort;

    SIPURL();

    SIPURL(
      const PURL & url
    ) : PURL(url) { }
    SIPURL & operator=(
      const PURL & url
    ) { PURL::operator=(url); return *this; }

    /** str goes straight to Parse()
      */
    SIPURL(
      const char * cstr,    ///<  C string representation of the URL.
      const char * defaultScheme = NULL ///<  Default scheme for URL
    );
    SIPURL & operator=(
      const char * cstr
    ) { Parse(cstr); return *this; }

    /** str goes straight to Parse()
      */
    SIPURL(
      const PString & str,  ///<  String representation of the URL.
      const char * defaultScheme = NULL ///<  Default scheme for URL
    );
    SIPURL & operator=(
      const PString & str
    ) { Parse(str); return *this; }

    /** If name does not start with 'sip' then construct URI in the form
        <pre><code>
          sip:name\@host:port;transport=transport
        </code></pre>
        where host comes from address,
        port is listenerPort or port from address if that was 0
        transport is udp unless address specified tcp
        Send name starting with 'sip' or constructed URI to Parse()
     */
    SIPURL(
      const PString & name,
      const OpalTransportAddress & address,
      WORD listenerPort = 0,
      const char * scheme = NULL
    );

    SIPURL(
      const OpalTransportAddress & address, 
      WORD listenerPort = 0,
      const char * scheme = NULL
    );
    SIPURL & operator=(
      const OpalTransportAddress & address
    );

    SIPURL(
      const SIPMIMEInfo & mime,
      const char * name
    );

    /**Compare the two SIPURLs and return their relative rank.
       Note that does an intelligent comparison according to the rules
       in RFC3261 Section 19.1.4.

     @return
       <code>LessThan</code>, <code>EqualTo</code> or <code>GreaterThan</code>
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
    PString GetDisplayName(PBoolean useDefault = true) const;
    
    void SetDisplayName(const PString & str) 
    {
      m_displayName = str;
    }

    /// Return string options in field parameters
    const PStringOptions & GetFieldParameters() const { return m_fieldParameters; }
          PStringOptions & GetFieldParameters()       { return m_fieldParameters; }

    /// Return the correct "transport" parameter, using correct default based on scheme
    PCaselessString GetTransportProto() const;

    /**Get the host and port as a transport address.
       If \p dnsEntry is != P_MAX_INDEX, then follows the rules of RFC3263 to
       get the remote server address, e.g. using DNS SRV records etc. If there
       are multiple server addresses, then this gets the "entry"th entry.
      */
    OpalTransportAddress GetTransportAddress(
      PINDEX dnsEntry = P_MAX_INDEX
    ) const;

    /**Set the host and port as a transport address.
      */
    void SetHostAddress(const OpalTransportAddress & addr);

    enum UsageContext {
      ExternalURI,   ///< URI used anywhere outside of protocol
      RequestURI,    ///< Request-URI (after the INVITE)
      ToURI,         ///< To header field
      FromURI,       ///< From header field
      RouteURI,      ///< Record-Route header field
      RedirectURI,   ///< Redirect Contact header field
      ContactURI,    ///< General Contact header field
      RegContactURI, ///< Registration Contact header field
      RegisterURI    ///< URI on REGISTER request line.
    };

    /** Removes tag parm & query vars and recalculates urlString
        (scheme, user, password, host, port & URI parms (like transport))
        which are not allowed in the context specified, e.g. Request-URI etc
        According to RFC3261, 19.1.1 Table 1
      */
    void Sanitise(
      UsageContext context  ///< Context for URI
    );

    /// Generate a unique string suitable as a dialog tag
    static PString GenerateTag();

    /// Set a tag with a new unique ID.
    void SetTag(
      const PString & tag = PString::Empty(),
      bool force = false
    );

    /// Get a tag
    PString GetTag() const;

  protected:
    void ParseAsAddress(
      const PString & name,
      const OpalTransportAddress & address,
      WORD listenerPort,
      const char * scheme);

    // Override from PURL()
    virtual PBoolean InternalParse(
      const char * cstr,
      const char * defaultScheme
    ) { return ReallyInternalParse(false, cstr, defaultScheme); }

    bool ReallyInternalParse(
      bool fromField,
      const char * cstr,
      const char * defaultScheme
    );
    WORD GetDefaultPort() const;

    PString        m_displayName;
    PStringOptions m_fieldParameters;
};


class SIPURLList : public std::list<SIPURL>
{
  public:
    bool FromString(
      const PString & str,
      SIPURL::UsageContext context = SIPURL::RouteURI,
      bool reversed = false
    );
    PString ToString() const;
    friend ostream & operator<<(ostream & strm, const SIPURLList & urls);
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
    virtual bool InternalAddMIME(const PString & fieldName, const PString & fieldValue);

    void SetCompactForm(bool form) { compactForm = form; }

    PCaselessString GetContentType(bool includeParameters = false) const;
    void SetContentType(const PString & v);

    PCaselessString GetContentEncoding() const;
    void SetContentEncoding(const PString & v);

    SIPURL GetFrom() const;
    void SetFrom(const SIPURL & v);

    SIPURL GetPAssertedIdentity() const;
    void SetPAssertedIdentity(const PString & v);

    SIPURL GetPPreferredIdentity() const;
    void SetPPreferredIdentity(const PString & v);

    PString GetAccept() const;
    void SetAccept(const PString & v);

    PString GetAcceptEncoding() const;
    void SetAcceptEncoding(const PString & v);

    PString GetAcceptLanguage() const;
    void SetAcceptLanguage(const PString & v);

    PString GetAllow() const;
    unsigned GetAllowBitMask() const;
    void SetAllow(const PString & v);

    PString GetCallID() const;
    void SetCallID(const PString & v);

    SIPURL GetContact() const;
    bool GetContacts(SIPURLList & contacts) const;
    void SetContact(const PString & v);

    PString GetSubject() const;
    void SetSubject(const PString & v);

    SIPURL GetTo() const;
    void SetTo(const SIPURL & v);

    PString GetVia() const;
    void SetVia(const PString & v);

    bool GetViaList(PStringList & v) const;
    void SetViaList(const PStringList & v);

    PString GetFirstVia() const;
    OpalTransportAddress GetViaReceivedAddress() const;

    SIPURL GetReferTo() const;
    void SetReferTo(const PString & r);

    SIPURL GetReferredBy() const;
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

    unsigned GetMaxForwards() const;
    void SetMaxForwards(PINDEX v);

    unsigned GetMinExpires() const;
    void SetMinExpires(PINDEX v);

    PString GetProxyAuthenticate() const;
    void SetProxyAuthenticate(const PString & v);

    PString GetRoute() const;
    bool GetRoute(SIPURLList & proxies) const;
    void SetRoute(const PString & v);
    void SetRoute(const SIPURLList & proxies);

    PString GetRecordRoute() const;
    bool GetRecordRoute(SIPURLList & proxies, bool reversed) const;
    void SetRecordRoute(const PString & v);
    void SetRecordRoute(const SIPURLList & proxies);

    unsigned GetCSeqIndex() const { return GetCSeq().AsUnsigned(); }

    PStringSet GetRequire() const;
    void SetRequire(const PStringSet & v);
    void AddRequire(const PString & v);

    PStringSet GetSupported() const;
    void SetSupported(const PStringSet & v);
    void AddSupported(const PString & v);

    PStringSet GetUnsupported() const;
    void SetUnsupported(const PStringSet & v);
    void AddUnsupported(const PString & v);
    
    PString GetEvent() const;
    void SetEvent(const PString & v);
    
    PCaselessString GetSubscriptionState(PStringToString & info) const;
    void SetSubscriptionState(const PString & v);
    
    PString GetUserAgent() const;
    void SetUserAgent(const PString & v);

    PString GetOrganization() const;
    void SetOrganization(const PString & v);

    void GetProductInfo(OpalProductInfo & info) const;
    void SetProductInfo(const PString & ua, const OpalProductInfo & info);

    PString GetWWWAuthenticate() const;
    void SetWWWAuthenticate(const PString & v);

    PString GetSIPIfMatch() const;
    void SetSIPIfMatch(const PString & v);

    PString GetSIPETag() const;
    void SetSIPETag(const PString & v);

    void GetAlertInfo(PString & info, int & appearance) const;
    void SetAlertInfo(const PString & info, int appearance);

    PString GetCallInfo() const;

    PString GetAllowEvents() const;
    void SetAllowEvents(const PString & v);
    void SetAllowEvents(const PStringSet & list);

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
    PStringSet GetTokenSet(const char * field) const;
    void AddTokenSet(const char * field, const PString & token);
    void SetTokenSet(const char * field, const PStringSet & tokens);

    /// Encode using compact form
    bool compactForm;
};


/////////////////////////////////////////////////////////////////////////
// SIPAuthentication

typedef PHTTPClientAuthentication SIPAuthentication;

class SIPAuthenticator : public PHTTPClientAuthentication::AuthObject
{
  public:
    SIPAuthenticator(SIP_PDU & pdu);
    virtual PMIMEInfo & GetMIME();
    virtual PString GetURI();
    virtual PString GetEntityBody();
    virtual PString GetMethod();

  protected:  
    SIP_PDU & m_pdu;
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
      Method_PRACK,
      NumMethods
    };

    enum StatusCodes {
      IllegalStatusCode,
      Local_TransportError,
      Local_BadTransportAddress,
      Local_Timeout,
      Local_NoCompatibleListener,
      Local_CannotMapScheme,
      Local_TransportLost,
      Local_KeepAlive,
      Local_NotAuthenticated,

      Information_Trying                  = 100,
      Information_Ringing                 = 180,
      Information_CallForwarded           = 181,
      Information_Queued                  = 182,
      Information_Session_Progress        = 183,

      Successful_OK                       = 200,
      Successful_Accepted                 = 202,

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

    static PString GetStatusCodeDescription(int code);
    friend ostream & operator<<(ostream & strm, StatusCodes status);

    SIP_PDU(
      Methods method = SIP_PDU::NumMethods,
      const OpalTransportPtr & transport = NULL
    );

    /** Construct a Response message
        extra is passed as message body
     */
    SIP_PDU(
      const SIP_PDU & request,
      StatusCodes code,
      const SDPSessionDescription * sdp = NULL
    );

    SIP_PDU(const SIP_PDU &);
    SIP_PDU & operator=(const SIP_PDU &);
    ~SIP_PDU();

    void PrintOn(
      ostream & strm
    ) const;

    void InitialiseHeaders(
      const SIPURL & dest,
      const SIPURL & to,
      const SIPURL & from,
      const PString & callID,
      unsigned cseq
    );
    void InitialiseHeaders(
      SIPDialogContext & dialog,
      unsigned cseq = 0
    );
    void InitialiseHeaders(
      SIPConnection & connection,
      unsigned cseq = 0
    );
    void InitialiseHeaders(
      const SIP_PDU & request
    );

    /**Add and populate Route header following the given routeSet.
      If first route is strict, exchange with URI.
      Returns true if routeSet.
      */
    bool SetRoute(const SIPURLList & routeSet);
    bool SetRoute(const SIPURL & proxy);

    /**Set mime allow field to all supported methods.
      */
    void SetAllow(unsigned bitmask);

    /**Read PDU from the specified transport.
      */
    StatusCodes Read();
    StatusCodes Parse(
      istream & strm,
      bool truncated
    );

    /**Write the PDU to the transport.
      */
    bool Send();

    /**Write PDU as a response to a request.
    */
    bool SendResponse(
      StatusCodes code
    );

    /** Construct the PDU string to output.
        Returns the total length of the PDU.
      */
    void Build(PString & pduStr, PINDEX & pduLen);

    const PString & GetTransactionID() const { return m_transactionID; }

    Methods GetMethod() const                { return m_method; }
    StatusCodes GetStatusCode () const       { return m_statusCode; }
    void SetStatusCode (StatusCodes c)       { m_statusCode = c; }
    const SIPURL & GetURI() const            { return m_uri; }
    void SetURI(const SIPURL & newuri)       { m_uri = newuri; }
    unsigned GetVersionMajor() const         { return m_versionMajor; }
    unsigned GetVersionMinor() const         { return m_versionMinor; }
    void SetCSeq(unsigned cseq);
    const PString & GetEntityBody() const    { return m_entityBody; }
    void SetEntityBody(const PString & body) { m_entityBody = body; }
    void SetEntityBody();
    const PString & GetInfo() const          { return m_info; }
    void SetInfo(const PString & info)       { m_info = info; }
    const SIPMIMEInfo & GetMIME() const      { return m_mime; }
          SIPMIMEInfo & GetMIME()            { return m_mime; }
    SDPSessionDescription * GetSDP()         { return m_SDP; }
    void SetSDP(SDPSessionDescription * sdp);
    bool DecodeSDP(const OpalMediaFormatList & masterList);

    const PString & GetExternalTransportAddress() const { return m_externalTransportAddress; }
    OpalTransportPtr GetTransport()  const    { return m_transport; }

  protected:
    void CalculateVia();
    StatusCodes InternalSend(bool canDoTCP);

    Methods     m_method;                 // Request type, ==NumMethods for Response
    StatusCodes m_statusCode;
    SIPURL      m_uri;                    // display name & URI, no tag
    unsigned    m_versionMajor;
    unsigned    m_versionMinor;
    PString     m_info;
    SIPMIMEInfo m_mime;
    PString     m_entityBody;
    PString     m_transactionID;

    SDPSessionDescription * m_SDP;

    OpalTransportPtr     m_transport;
    OpalTransportAddress m_viaAddress;
    OpalTransportAddress m_externalTransportAddress;
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
    SIPDialogContext(const SIPMIMEInfo & mime);

    PString AsString() const;
    bool FromString(
      const PString & str
    );

    const PString & GetCallID() const { return m_callId; }
    void SetCallID(const PString & id) { m_callId = id; }

    const SIPURL & GetRequestURI() const { return m_requestURI; }
    void SetRequestURI(const SIPURL & url);

    const PString & GetLocalTag() const { return m_localTag; }
    void SetLocalTag(const PString & tag) { m_localTag = tag; }

    const SIPURL & GetLocalURI() const { return m_localURI; }
    void SetLocalURI(const SIPURL & url);

    const PString & GetRemoteTag() const { return m_remoteTag; }
    void SetRemoteTag(const PString & tag) { m_remoteTag = tag; }

    const SIPURL & GetRemoteURI() const { return m_remoteURI; }
    void SetRemoteURI(const SIPURL & url);

    const SIPURLList & GetRouteSet() const { return m_routeSet; }
    void SetRouteSet(const PString & str) { m_routeSet.FromString(str); }

    const SIPURL & GetProxy() const { return m_proxy; }
    void SetProxy(const SIPURL & proxy, bool addToRouteSet);

    void Update(const SIP_PDU & response);

    unsigned GetNextCSeq();
    void IncrementCSeq(unsigned inc) { m_lastSentCSeq += inc; }

    bool IsDuplicateCSeq(unsigned sequenceNumber);

    bool IsEstablished() const
    {
      return !m_callId.IsEmpty() &&
             !m_requestURI.IsEmpty() &&
             !m_localTag.IsEmpty() &&
             !m_remoteTag.IsEmpty();
    }

    OpalTransportAddress GetRemoteTransportAddress(PINDEX dnsEntry) const;
    const PString & GetInterface() const { return m_interface; }
    void SetInterface(const PString & newInterface) { m_interface = newInterface; }

    void SetForking(bool f) { m_forking = f; }

  protected:
    PString     m_callId;
    SIPURL      m_requestURI;
    SIPURL      m_localURI;
    PString     m_localTag;
    SIPURL      m_remoteURI;
    PString     m_remoteTag;
    SIPURLList  m_routeSet;
    unsigned    m_lastSentCSeq;
    unsigned    m_lastReceivedCSeq;
    OpalTransportAddress m_externalTransportAddress;
    bool        m_forking;
    SIPURL      m_proxy;
    PString     m_interface;
};


/////////////////////////////////////////////////////////////////////////

struct SIPParameters
{
  SIPParameters(
    const PString & aor = PString::Empty(),
    const PString & remote = PString::Empty()
  );

  void Normalise(
    const PString & defaultUser,
    const PTimeInterval & defaultExpire
  );

  PCaselessString m_remoteAddress;
  PCaselessString m_localAddress;
  PCaselessString m_proxyAddress;
  PCaselessString m_addressOfRecord;
  PCaselessString m_contactAddress;
  PCaselessString m_interface;
  SIPMIMEInfo     m_mime;
  PString         m_authID;
  PString         m_password;
  PString         m_realm;
  unsigned        m_expire;
  unsigned        m_restoreTime;
  PTimeInterval   m_minRetryTime;
  PTimeInterval   m_maxRetryTime;
  void          * m_userData;
};


#if PTRACING
ostream & operator<<(ostream & strm, const SIPParameters & params);
#endif


/////////////////////////////////////////////////////////////////////////
// Thread pooling stuff
//
// Timer call back mechanism using PNOTIFIER is too prone to deadlocks, we
// want to use the existing thread pool for handling incoming PDUs.

class SIPWorkItem : public PObject
{
    PCLASSINFO(SIPWorkItem, PObject);
  public:
    SIPWorkItem(SIPEndPoint & ep, const PString & token);

    virtual void Work() = 0;

    bool GetTarget(PSafePtr<SIPTransaction> & transaction);
    bool GetTarget(PSafePtr<SIPConnection> & connection);
    bool GetTarget(PSafePtr<SIPHandler> & handler);

  protected:
    SIPEndPoint & m_endpoint;
    PString       m_token;
};


class SIPThreadPool : public PQueuedThreadPool<SIPWorkItem>
{
    typedef PQueuedThreadPool<SIPWorkItem> BaseClass;
    PCLASSINFO(SIPThreadPool, BaseClass);
  public:
    SIPThreadPool(unsigned maxWorkers, const char * threadName)
      : BaseClass(maxWorkers, 0, threadName, PThread::HighPriority)
    {
    }
};


template <class Target_T>
class SIPTimeoutWorkItem : public SIPWorkItem
{
    PCLASSINFO(SIPTimeoutWorkItem, SIPWorkItem);
  public:
    typedef void (Target_T::* Callback)();

    SIPTimeoutWorkItem(SIPEndPoint & ep, const PString & token, Callback callback)
      : SIPWorkItem(ep, token)
      , m_callback(callback)
    {
    }

    virtual void Work()
    {
      PSafePtr<Target_T> target;
      if (GetTarget(target)) {
        PTRACE_CONTEXT_ID_PUSH_THREAD(target);
        (target->*m_callback)();
        PTRACE(4, "SIP\tHandled timeout");
      }
    }

  protected:
    Callback m_callback;
};


template <class Target_T>
class SIPPoolTimer : public PPoolTimerArg3<SIPTimeoutWorkItem<Target_T>,
                                           SIPEndPoint &,
                                           PString,
                                           void (Target_T::*)(),
                                           SIPWorkItem>
{
    typedef PPoolTimerArg3<SIPTimeoutWorkItem<Target_T>, SIPEndPoint &, PString, void (Target_T::*)(), SIPWorkItem> BaseClass;
    PCLASSINFO(SIPPoolTimer, BaseClass);
  public:
    SIPPoolTimer(SIPThreadPool & pool, SIPEndPoint & ep, const PString & token, void (Target_T::* callback)())
      : BaseClass(pool, ep, token, callback)
    {
    }

    PTIMER_OPERATORS(SIPPoolTimer);
};


/////////////////////////////////////////////////////////////////////////
// SIPTransaction

class SIPTransactionOwner
{
  public:
    SIPTransactionOwner(
      PSafeObject & object,
      SIPEndPoint & endpoint
    );
    virtual ~SIPTransactionOwner();

    virtual PString GetAuthID() const = 0;
    virtual PString GetPassword() const { return PString::Empty(); }
    virtual unsigned GetAllowedMethods() const;

    virtual void OnStartTransaction(SIPTransaction & /*transaction*/) { }
    virtual void OnReceivedResponse(SIPTransaction & transaction, SIP_PDU & response);
    virtual void OnTransactionFailed(SIPTransaction & transaction);

    void FinaliseForking(SIPTransaction & transaction, SIP_PDU & response);
    bool CleanPendingTransactions();
    void AbortPendingTransactions();

    virtual SIP_PDU::StatusCodes StartTransaction(
      const OpalTransport::WriteConnectCallback & function
    );

    SIP_PDU::StatusCodes SwitchTransportProto(const char * proto, OpalTransportPtr * transport);

    SIP_PDU::StatusCodes HandleAuthentication(const SIP_PDU & response);

    SIPEndPoint & GetEndPoint() const { return m_endpoint; }
    OpalTransportAddress GetRemoteTransportAddress() const { return m_dialog.GetRemoteTransportAddress(m_dnsEntry); }
    const SIPURL & GetRequestURI() const { return m_dialog.GetRequestURI(); }
    const SIPURL & GetRemoteURI() const { return m_dialog.GetRemoteURI(); }
    const SIPURL & GetProxy() const { return m_dialog.GetProxy(); }
    const PString & GetInterface() const { return m_dialog.GetInterface(); }
    void ResetInterface() { m_dialog.SetInterface(PString::Empty()); }
    PINDEX GetDNSEntry() const { return m_dnsEntry; }
    SIPAuthentication * GetAuthenticator() const { return m_authentication; }
    SIPDialogContext & GetDialog() { return m_dialog; }
    const SIPDialogContext & GetDialog() const { return m_dialog; }

  protected:
    PSafeObject       & m_object;
    SIPEndPoint       & m_endpoint;
    SIPDialogContext    m_dialog; // Not all derived classes use this as a dialog, but many fields useful
    PINDEX              m_dnsEntry;
    SIPAuthentication * m_authentication;
    unsigned            m_authenticateErrors;

    PSafeList<SIPTransaction> m_transactions;

  friend class SIPTransaction;
};


class SIPTransactionBase : public SIP_PDU
{
    PCLASSINFO(SIPTransactionBase, SIP_PDU);
  protected:
    SIPTransactionBase(
      Methods method
    ) : SIP_PDU(method) { }

  public:
    SIPTransactionBase(
      const PString & transactionID
    ) { m_transactionID = transactionID; }

    Comparison Compare(
      const PObject & other
    ) const;

    virtual bool IsTerminated() const { return true; }
};


/** Session Initiation Protocol transaction.
    A transaction is a stateful independent entity that provides services to
    a connection (Transaction User). Transactions are contained within 
    connections.
    A client transaction handles sending a request and receiving its
    responses.
    A server transaction handles sending responses to a received request.
    In either case the SIP_PDU ancestor is the sent or received request.
 */

class SIPTransaction : public SIPTransactionBase
{
    PCLASSINFO(SIPTransaction, SIPTransactionBase);
  protected:
    SIPTransaction(
      Methods method,
      SIPTransactionOwner * owner,
      OpalTransport * transport,
      bool deleteOwner = false
    );
  public:
    ~SIPTransaction();

    /* Under some circumstances a new transaction with all the same parameters
       but different ID needs to be created, e.g. when get authentication error. */
    virtual SIPTransaction * CreateDuplicate() const = 0;

    bool Start();
    bool IsTrying()     const { return m_state == Trying; }
    bool IsProceeding() const { return m_state == Proceeding; }
    bool IsInProgress() const { return m_state == Trying || m_state == Proceeding; }
    bool IsFailed()     const { return m_state > Terminated_Success; }
    bool IsCompleted()  const { return m_state >= Completed; }
    bool IsCanceled()   const { return m_state == Cancelling || m_state == Terminated_Cancelled || m_state == Terminated_Aborted; }
    bool IsTerminated() const { return m_state >= Terminated_Success; }

    void WaitForCompletion();
    PBoolean Cancel();
    void Abort();

    virtual PBoolean OnReceivedResponse(SIP_PDU & response);
    virtual PBoolean OnCompleted(SIP_PDU & response);

    SIPEndPoint & GetEndPoint() const { return m_owner->GetEndPoint(); }
    SIPConnection * GetConnection() const;
    PString         GetInterface()  const { return m_localInterface; }

    static PString GenerateCallID();

  protected:
    bool ResendCANCEL();
    void SetParameters(const SIPParameters & params);

    typedef SIPPoolTimer<SIPTransaction> PoolTimer;

    void OnRetry();
    void OnTimeout();

    P_DECLARE_TRACED_ENUM(States,
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
      Terminated_Aborted
    );
    virtual void SetTerminated(States newState);

    SIPTransactionOwner * m_owner;
    bool                  m_deleteOwner;

    PTimeInterval m_retryTimeoutMin; 
    PTimeInterval m_retryTimeoutMax; 

    States     m_state;
    unsigned   m_retry;
    PoolTimer  m_retryTimer;
    PoolTimer  m_completionTimer;
    PSyncPoint m_completed;

    PString         m_localInterface;

  friend class SIPConnection;
};


#define OPAL_PROXY_PARAM     "OPAL-proxy"
#define OPAL_LOCAL_ID_PARAM  "OPAL-local-id"
#define OPAL_INTERFACE_PARAM "OPAL-interface"


/////////////////////////////////////////////////////////////////////////
// SIPResponse

/** When we receive a command, we need a transaction to send repeated responses.
 */
class SIPResponse : public SIPTransaction
{
    PCLASSINFO(SIPResponse, SIPTransaction);
  public:
    SIPResponse(
      SIPEndPoint & endpoint,
      const SIP_PDU & command,
      StatusCodes code
    );

    virtual SIPTransaction * CreateDuplicate() const;

    bool Resend(SIP_PDU & command);
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
      OpalTransport * transport = NULL
    );

    virtual SIPTransaction * CreateDuplicate() const;

    virtual PBoolean OnReceivedResponse(SIP_PDU & response);

    mutable OpalRTPConnection::SessionMap m_sessions;
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
      const SIPTransaction & invite,
      const SIP_PDU & response
    );

    virtual SIPTransaction * CreateDuplicate() const;
};


/////////////////////////////////////////////////////////////////////////

/* This is a BYE request
 */
class SIPBye : public SIPTransaction
{
    PCLASSINFO(SIPBye, SIPTransaction);
  public:
    SIPBye(
      SIPEndPoint & endpoint,
      SIPDialogContext & dialog
    );
    SIPBye(
      SIPConnection & conn
    );

    virtual SIPTransaction * CreateDuplicate() const;
};


/////////////////////////////////////////////////////////////////////////

class SIPRegister : public SIPTransaction
{
    PCLASSINFO(SIPRegister, SIPTransaction);
  public:
    P_DECLARE_TRACED_ENUM(CompatibilityModes,
      e_FullyCompliant,                 /**< Registrar is fully compliant, we will register
                                             all listeners of all types (e.g. sip, sips etc)
                                             in the Contact field. */
      e_CannotRegisterMultipleContacts, /**< Registrar refuses to register more than one
                                             contact field. Correct behaviour is to return
                                             a contact with the fields it can accept in
                                             the 200 OK */
      e_CannotRegisterPrivateContacts,  /**< Registrar refuses to register any RFC 1918
                                             contact field. Correct behaviour is to return
                                             a contact with the fields it can accept in
                                             the 200 OK */
      e_HasApplicationLayerGateway,     /**< Router has Application Layer Gateway code that
                                             is doing address transations, so we do not try
                                             to do it ourselves as well or it goes horribly
                                             wrong. */
      e_RFC5626                         /**< Connect using RFC 5626 rules. Only a single
                                             contact is included and this will contain the
                                             m_instance field as it's GUID. */
    );

    /// Registrar parameters
    struct Params : public SIPParameters {
      Params()
        : m_registrarAddress(m_remoteAddress)
        , m_compatibility(SIPRegister::e_FullyCompliant)
        , m_instanceId(NULL)
      { }

      Params(const Params & param)
        : SIPParameters(param)
        , m_registrarAddress(m_remoteAddress)
        , m_compatibility(param.m_compatibility)
        , m_instanceId(param.m_instanceId)
      { }

      PCaselessString  & m_registrarAddress; // For backward compatibility
      CompatibilityModes m_compatibility;
      PGloballyUniqueID  m_instanceId;
    };

    SIPRegister(
      SIPTransactionOwner & owner,
      OpalTransport & transport,
      const PString & callId,
      unsigned cseq,
      const Params & params
    );

    virtual SIPTransaction * CreateDuplicate() const;
};


#if PTRACING
ostream & operator<<(ostream & strm, SIPRegister::CompatibilityModes mode);
ostream & operator<<(ostream & strm, const SIPRegister::Params & params);
#endif


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
      Reg,
      Conference,

      NumPredefinedPackages,

      Watcher = 0x8000,

      MessageSummaryWatcher = Watcher|MessageSummary,
      PresenceWatcher       = Watcher|Presence,
      DialogWatcher         = Watcher|Dialog,

      PackageMask = Watcher-1
    };
    friend PredefinedPackages operator|(PredefinedPackages p1, PredefinedPackages p2) { return (PredefinedPackages)((int)p1|(int)p2); }

    class EventPackage : public PCaselessString
    {
      PCLASSINFO(EventPackage, PCaselessString);
      public:
        EventPackage(PredefinedPackages = NumPredefinedPackages);
        explicit EventPackage(const PString & str) : PCaselessString(str) { }
        explicit EventPackage(const char   *  str) : PCaselessString(str) { }

        EventPackage & operator=(PredefinedPackages pkg);
        EventPackage & operator=(const PString & str) { PCaselessString::operator=(str); return *this; }
        EventPackage & operator=(const char   *  str) { PCaselessString::operator=(str); return *this; }

        bool operator==(PredefinedPackages pkg) const { return Compare(EventPackage(pkg)) == EqualTo; }
        bool operator==(const PString & str) const { return Compare(str) == EqualTo; }
        bool operator==(const char * cstr) const { return InternalCompare(0, P_MAX_INDEX, cstr) == EqualTo; }
        virtual Comparison InternalCompare(PINDEX offset, PINDEX length, const char * cstr) const;

        bool IsWatcher() const;
    };

    /** Information provided on the subscription status. */
    struct SubscriptionStatus {
      SIPSubscribeHandler * m_handler;           ///< Handler for subscription
      PString               m_addressofRecord;   ///< Address of record for registration
      bool                  m_wasSubscribing;    ///< Was registering or unregistering
      bool                  m_reSubscribing;     ///< Was a registration refresh
      SIP_PDU::StatusCodes  m_reason;            ///< Reason for status change
      OpalProductInfo       m_productInfo;       ///< Server product info from registrar if available.
      void                * m_userData;          ///< User data corresponding to this registration
    };

    struct NotifyCallbackInfo {
      NotifyCallbackInfo(
        SIPSubscribeHandler & handler,
        SIPEndPoint & ep,
        SIP_PDU & request,
        SIP_PDU & response
      );

#if P_EXPAT
      bool LoadAndValidate(
        PXML & xml,
        const PXML::ValidationInfo * validator,
        PXML::Options options = PXML::WithNS
      );
#endif

      bool SendResponse(
        SIP_PDU::StatusCodes status = SIP_PDU::Successful_OK,
        const char * extra = NULL
      );

      SIPSubscribeHandler & m_handler;
      SIPEndPoint         & m_endpoint;
      SIP_PDU             & m_request;
      SIP_PDU             & m_response;
      bool                  m_sendResponse;
    };

    struct Params : public SIPParameters
    {
      Params(PredefinedPackages pkg = NumPredefinedPackages)
        : m_agentAddress(m_remoteAddress)
        , m_eventPackage(pkg)
        , m_eventList(false)
      { }

      Params(const Params & param)
        : SIPParameters(param)
        , m_agentAddress(m_remoteAddress)
        , m_eventPackage(param.m_eventPackage)
        , m_eventList(param.m_eventList)
        , m_contentType(param.m_contentType)
        , m_onSubcribeStatus(param.m_onSubcribeStatus)
        , m_onNotify(param.m_onNotify)
      { }

      PCaselessString & m_agentAddress; // For backward compatibility
      EventPackage      m_eventPackage;
      bool              m_eventList;    // Enable RFC4662
      PCaselessString   m_contentType;  // May be \n separated list of types

      PNotifierTemplate<const SubscriptionStatus &> m_onSubcribeStatus;
      PNotifierTemplate<NotifyCallbackInfo &> m_onNotify;
    };

    SIPSubscribe(
        SIPTransactionOwner & owner,
        OpalTransport & transport,
        SIPDialogContext & dialog,
        const Params & params
    );

    virtual SIPTransaction * CreateDuplicate() const;
};


#if PTRACING
ostream & operator<<(ostream & strm, const SIPSubscribe::Params & params);
#endif


typedef SIPSubscribe::EventPackage SIPEventPackage;


/////////////////////////////////////////////////////////////////////////

class SIPHandler;

class SIPEventPackageHandler : public PObject
{
  unsigned m_expectedSequenceNumber;

public:
  SIPEventPackageHandler() : m_expectedSequenceNumber(UINT_MAX) { }

  virtual ~SIPEventPackageHandler() { }
  virtual PCaselessString GetContentType() const = 0;
  virtual bool ValidateContentType(const PString & type, const SIPMIMEInfo & mime);
  virtual bool ValidateNotificationSequence(SIPSubscribeHandler & handler, unsigned newSequenceNumber, bool fullUpdate);
  virtual void OnReceivedNOTIFY(SIPSubscribe::NotifyCallbackInfo & notifyInfo) = 0;
  virtual PString OnSendNOTIFY(SIPHandler & /*handler*/, const PObject * /*body*/) { return PString::Empty(); }

  P_REMOVE_VIRTUAL(bool, OnReceivedNOTIFY(SIPHandler &, SIP_PDU &), false);
};


typedef PFactory<SIPEventPackageHandler, SIPEventPackage> SIPEventPackageFactory;


/////////////////////////////////////////////////////////////////////////

class SIPNotify : public SIPTransaction
{
    PCLASSINFO(SIPNotify, SIPTransaction);
  public:
    SIPNotify(
        SIPTransactionOwner & owner,
        OpalTransport & transport,
        SIPDialogContext & dialog,
        const SIPEventPackage & eventPackage,
        const PString & state,
        const PString & body
    );

    virtual SIPTransaction * CreateDuplicate() const;
};


/////////////////////////////////////////////////////////////////////////

class SIPPublish : public SIPTransaction
{
    PCLASSINFO(SIPPublish, SIPTransaction);
  public:
    SIPPublish(
      SIPTransactionOwner & owner,
      OpalTransport & transport,
      const PString & id,
      const PString & sipIfMatch,
      const SIPSubscribe::Params & params,
      const PString & body
    );

    virtual SIPTransaction * CreateDuplicate() const;
};


/////////////////////////////////////////////////////////////////////////

class SIPRefer : public SIPTransaction
{
  PCLASSINFO(SIPRefer, SIPTransaction);
  public:
    SIPRefer(
      SIPConnection & connection,
      const SIPURL & referTo,
      const SIPURL & referred_by,
      bool referSub
    );

    virtual SIPTransaction * CreateDuplicate() const;
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
      StatusCodes code
    );

    virtual SIPTransaction * CreateDuplicate() const;
};


/////////////////////////////////////////////////////////////////////////

/* This is a MESSAGE PDU, with a body
 */
class SIPMessage : public SIPTransaction
{
  PCLASSINFO(SIPMessage, SIPTransaction);
  public:
    struct Params : public SIPParameters
    {
      Params()
        : m_contentType("text/plain;charset=UTF-8")
      { 
        m_expire = 5000;
      }

      PCaselessString             m_contentType;
      PString                     m_id;
      PString                     m_body;
      PAtomicInteger::IntegerType m_messageId;
    };

    SIPMessage(
      SIPTransactionOwner & owner,
      OpalTransport & transport,
      const Params & params
    );
    SIPMessage(
      SIPConnection & connection,
      const Params & params
    );

    virtual SIPTransaction * CreateDuplicate() const;

    const Params & GetParameters() const { return m_parameters; }
    const SIPURL & GetLocalAddress() const { return m_localAddress; }

  private:
    void Construct(const Params & params);

    Params m_parameters;
    SIPURL m_localAddress;
};


/////////////////////////////////////////////////////////////////////////

/* This is an OPTIONS request
 */
class SIPOptions : public SIPTransaction
{
    PCLASSINFO(SIPOptions, SIPTransaction);
    
  public:
    struct Params : public SIPParameters
    {
      Params()
        : m_acceptContent("application/sdp, application/media_control+xml, application/dtmf, application/dtmf-relay")
      { 
      }

      PCaselessString m_acceptContent;
      PCaselessString m_contentType;
      PString         m_body;
    };

    SIPOptions(
      SIPEndPoint & endpoint,
      const Params & params
    );
    SIPOptions(
      SIPConnection & conn,
      const Params & params
    );

    virtual SIPTransaction * CreateDuplicate() const;

  protected:
    void Construct(const Params & params);
};


/////////////////////////////////////////////////////////////////////////

/* This is an INFO request
 */
class SIPInfo : public SIPTransaction
{
    PCLASSINFO(SIPInfo, SIPTransaction);
    
  public:
    struct Params
    {
      Params(const PString & contentType = PString::Empty(),
             const PString & body = PString::Empty())
        : m_contentType(contentType)
        , m_body(body)
      {
      }

      PCaselessString m_contentType;
      PString         m_body;
    };

    SIPInfo(
      SIPConnection & conn,
      const Params & params
    );

    virtual SIPTransaction * CreateDuplicate() const;
};


/////////////////////////////////////////////////////////////////////////

/* This is a PING PDU, with a body
 */
class SIPPing : public SIPTransaction
{
  PCLASSINFO(SIPPing, SIPTransaction);

  public:
    SIPPing(
      SIPTransactionOwner & owner,
      OpalTransport & transport,
      const SIPURL & address
    );

    virtual SIPTransaction * CreateDuplicate() const;
};


/////////////////////////////////////////////////////////////////////////

/* This is a PRACK PDU
 */
class SIPPrack : public SIPTransaction
{
  PCLASSINFO(SIPPrack, SIPTransaction);

  public:
    SIPPrack(
      SIPConnection & conn,
      OpalTransport & transport,
      const PString & rack
    );

    virtual SIPTransaction * CreateDuplicate() const;
};


#endif // OPAL_SIP

#endif // OPAL_SIP_SIPPDU_H


// End of File ///////////////////////////////////////////////////////////////
