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
#include <sip/sippdu.h>
%End

%MappedType SIPURLList
{

%ConvertFromTypeCode
   PyObject *l;

   // Create the Python list of the correct length.
   if ((l = PyList_New(sipCpp -> size())) == NULL)
       return NULL;

   // Go through each element in the C++ instance and convert it to a
   // wrapped TYPE.

   int i=0;
   for (SIPURLList::iterator iter = sipCpp->begin(); iter != sipCpp->end(); iter++)
   {
       PyObject *pobj;

       // Get the Python wrapper for the Type instance, creating a new
       if ((pobj = sipConvertFromType(&(*iter), sipType_SIPURL, NULL)) == NULL)
       // one if necessary, and handle any ownership transfer.
       {
           // There was an error so garbage collect the Python list.
           Py_DECREF(l);
           return NULL;
       }

       // Add the wrapper to the list.
       PyList_SET_ITEM(l, i++, pobj);
   }

   // Return the Python list.
   return l;
%End

%ConvertToTypeCode
   // Check if type is compatible
   if (sipIsErr == NULL)
   {
       // Must be any iterable
       PyObject *i = PyObject_GetIter(sipPy);
       bool iterable = (i != NULL);
       Py_XDECREF(i);
       return iterable;
   }

   // Iterate over the object
   PyObject *iterator = PyObject_GetIter(sipPy);
   PyObject *item;

   SIPURLList *l = new SIPURLList;

   while ((item = PyIter_Next(iterator)))
   {
       if (!sipCanConvertToType(item, sipType_SIPURL, SIP_NOT_NONE))
       {
           PyErr_Format(PyExc_TypeError, "object in iterable cannot be converted to SIPURL");
           *sipIsErr = 1;
           break;
       }

       int state;
       // make sure ownership is kept with Python to avoid memory leaks.
       SIPURL* url = reinterpret_cast<SIPURL*>(
            sipConvertToType(item, sipType_SIPURL, Py_None, SIP_NOT_NONE, &state, sipIsErr));

       if (!*sipIsErr)
           l->push_back(SIPURL(*url));

       sipReleaseType(url, sipType_SIPURL, state);

       if( url != NULL )
       {
           delete url;
           url = NULL;
       }

       Py_DECREF(item);
   }

   Py_DECREF(iterator);

   if (*sipIsErr)
   {
       delete l;
       return 0;
   }

   *sipCppPtr = l;
   return sipGetState(sipTransferObj);
%End
};


/////////////////////////////////////////////////////////////////////////
// SIPURL

/** This class extends PURL to include displayname, optional "<>" delimiters
	and extended parameters - like tag.
	It may be used for From:, To: and Contact: lines.
 */

class SIPURL : PURL
{
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
       <code>LessThan</code>, <code>EqualTo</code> or <code>GreaterThan</code>
       according to the relative rank of the objects.
     */
    virtual PObject::Comparison Compare(
      const PObject & obj   ///< Object to compare against.
    ) const;

    /** Returns complete SIPURL as one string, including displayname (in
        quotes) and address in angle brackets.
      */
    PString AsQuotedString() const;

    /** Returns display name only
      */
    PString GetDisplayName(PBoolean useDefault = true) const;
    
    void SetDisplayName(const PString & str);

	PStringOptions & GetFieldParameters();
	
    /**Get the host and port as a transport address.
      */
    OpalTransportAddress GetHostAddress() const;

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
      const PString & tag = PString::Empty(),
	  bool force = false
    );
	
 protected:
    void ParseAsAddress(const PString & name, const OpalTransportAddress & _address, WORD listenerPort = 0);

    /** Parses name-addr, like:
        <pre>
        "displayname"<scheme:user:password\@host:port;transport=type>;tag=value
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
        </pre>
        Note that tag parameter outside of <> will be lost,
        but tag in URL without <> will be kept until Sanitise()
     */
    virtual PBoolean InternalParse(
      const char * cstr,
      const char * defaultScheme
    );

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

class SIPMIMEInfo : PMIMEInfo
{
  public:
    SIPMIMEInfo(bool compactForm = false);

    void SetCompactForm(bool form);

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
    unsigned GetAllowBitMask() const;
    void SetAllow(const PString & v);

    PString GetCallID() const;
    void SetCallID(const PString & v);

    PString GetContact() const;
    bool GetContacts(SIPURLList & contacts /Out/) const;
    void SetContact(const PString & v);

    PString GetSubject() const;
    void SetSubject(const PString & v);

    PString GetTo() const;
    void SetTo(const PString & v);

    PString GetVia() const;
    void SetVia(const PString & v);

    bool GetViaList(PStringList & v /Out/) const;
    void SetViaList(const PStringList & v);

    PString GetFirstVia() const;
    OpalTransportAddress GetViaReceivedAddress() const;

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
    void SetDate(); // set to current date

    unsigned GetExpires(unsigned dflt = UINT_MAX) const;// returns default value if not found
    void SetExpires(unsigned v);

    PINDEX GetMaxForwards() const;
    void SetMaxForwards(PINDEX v);

    PINDEX GetMinExpires() const;
    void SetMinExpires(PINDEX v);

    PString GetProxyAuthenticate() const;
    void SetProxyAuthenticate(const PString & v);

    PString GetRoute() const;
    //bool GetRoute(SIPURLList & proxies /Out/) const; // xxx - does not appear to have an implementation.
    void SetRoute(const PString & v);
    void SetRoute(const SIPURLList & proxies);

    PString GetRecordRoute() const;
    bool GetRecordRoute(SIPURLList & proxies /Out/, bool reversed) const;
    void SetRecordRoute(const PString & v);
    void SetRecordRoute(const SIPURLList & proxies);

    unsigned GetCSeqIndex() const;

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

    void GetProductInfo(OpalProductInfo & info /Out/) const;
    void SetProductInfo(const PString & ua, const OpalProductInfo & info);

    PString GetWWWAuthenticate() const;
    void SetWWWAuthenticate(const PString & v);

    PString GetSIPIfMatch() const;
    void SetSIPIfMatch(const PString & v);

    PString GetSIPETag() const;
    void SetSIPETag(const PString & v);

    void GetAlertInfo(PString & info /Out/, int & appearance /Out/);
    void SetAlertInfo(const PString & info, int appearance);

    PString GetCallInfo() const;

    PString GetAllowEvents() const;
    void SetAllowEvents(const PString & v);

    /** return the value of a header field parameter, empty if none
     */
    PString GetFieldParameter(
      const PString & fieldName,    ///< Field name in dictionary
      const PString & paramName,    ///< Field parameter name
      const PString & defaultValue = PString::Empty()  ///< Default value for parameter
    );

    /** set the value for a header field parameter, replace the
     *  current value, or add the parameter and its
     *  value if not already present.
     */
    void SetFieldParameter(
      const PString & fieldName,    ///< Field name in dictionary
      const PString & paramName,    ///< Field parameter name
      const PString & newValue      ///< New value for parameter
    );

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
};



/////////////////////////////////////////////////////////////////////////
// SIPAuthentication

typedef PHTTPClientAuthentication SIPAuthentication;

class SIPAuthenticator : PHTTPClientAuthentication::AuthObject
{
  public:
    SIPAuthenticator(SIP_PDU & pdu);
    virtual PMIMEInfo & GetMIME();
    virtual PString GetURI();
    virtual PString GetEntityBody();
    virtual PString GetMethod();

};



/////////////////////////////////////////////////////////////////////////
// SIP_PDU

/** Session Initiation Protocol message.
	Each message contains a header, MIME lines and possibly SDP.
	Class provides methods for reading from and writing to transport.
 */

class SIP_PDU : PSafeObject
{
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

    SIP_PDU(
      Methods method = SIP_PDU::NumMethods
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
    // SIP_PDU & operator=(const SIP_PDU &);
    ~SIP_PDU();

    void InitialiseHeaders(
      const SIPURL & dest,
      const SIPURL & to,
      const SIPURL & from,
      const PString & callID,
      unsigned cseq,
      const PString & via
    );
    void InitialiseHeaders(
      SIPDialogContext & dialog,
      const PString & via = PString::Empty(),
	  unsigned cseq = 0
    );
    void InitialiseHeaders(
      SIPConnection & connection,
      const OpalTransport & transport,
	  unsigned cseq = 0
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

    /**Update the VIA field following RFC3261, 18.2.1 and RFC3581.
      */
    void AdjustVia(OpalTransport & transport);

    PString CreateVia(
      SIPEndPoint & endpoint,
      const OpalTransport & transport,
      SIPConnection * connection = NULL
    );

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
      SIPEndPoint * endpoint = NULL
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

    Methods GetMethod() const;
    StatusCodes GetStatusCode () const;
    void SetStatusCode (StatusCodes c);
    const SIPURL & GetURI() const;
    unsigned GetVersionMajor() const;
    unsigned GetVersionMinor() const;
    const PString & GetEntityBody() const;
    void SetEntityBody(const PString & body);
	void SetEntityBody();
    const PString & GetInfo() const;
    void SetInfo(const PString & info);
    //const SIPMIMEInfo & GetMIME() const;
    SIPMIMEInfo & GetMIME();
    void SetURI(const SIPURL & newuri);
    SDPSessionDescription * GetSDP(const OpalMediaFormatList & masterList);
    void SetSDP(SDPSessionDescription * sdp);

};


//PQUEUE(SIP_PDU_Queue, SIP_PDU);



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

    const PString & GetCallID() const;
    void SetCallID(const PString & id);

    const SIPURL & GetRequestURI() const;
    void SetRequestURI(const SIPURL & url);
    bool SetRequestURI(const PString & uri);

    const PString & GetLocalTag() const;
    void SetLocalTag(const PString & tag);

    const SIPURL & GetLocalURI() const;
    void SetLocalURI(const SIPURL & url);
    bool SetLocalURI(const PString & uri);

    const PString & GetRemoteTag() const;
    void SetRemoteTag(const PString & tag);

    const SIPURL & GetRemoteURI() const;
    void SetRemoteURI(const SIPURL & url);
    bool SetRemoteURI(const PString & uri);

    const SIPURLList & GetRouteSet() const;
    void SetRouteSet(const PString & str);

    const SIPURL & GetProxy() const;
    void SetProxy(const SIPURL & proxy, bool addToRouteSet);

    void Update(OpalTransport & transport, const SIP_PDU & response);

    unsigned GetNextCSeq();
    bool IsDuplicateCSeq(unsigned sequenceNumber);

    bool IsEstablished() const;

    OpalTransportAddress GetRemoteTransportAddress() const;

    void SetForking(bool f);
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
  PString         m_authID;
  PString         m_password;
  PString         m_realm;
  unsigned        m_expire;
  unsigned        m_restoreTime;
  PTimeInterval   m_minRetryTime;
  PTimeInterval   m_maxRetryTime;
  void          * m_userData;
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

class SIPTransaction : SIP_PDU
{
  public:
    SIPTransaction(
      Methods method,
      SIPEndPoint   & endpoint,
      OpalTransport & transport
    );
    /** Construct a transaction for requests in a dialog.
     *  The transport is used to determine the local address
     */
    SIPTransaction(
      Methods method,
      SIPConnection & connection
    );
    ~SIPTransaction();
	
    virtual SIPTransaction * CreateDuplicate() const = 0;

    PBoolean Start();
    bool IsTrying()     const;
    bool IsProceeding() const;
    bool IsInProgress() const;
    bool IsFailed()     const;
    bool IsCompleted()  const;
    bool IsCanceled()   const;

    void WaitForCompletion();
    PBoolean Cancel();
    void Abort();

    virtual PBoolean OnReceivedResponse(SIP_PDU & response);
    virtual PBoolean OnCompleted(SIP_PDU & response);

    OpalTransport & GetTransport()  const;
    SIPConnection * GetConnection() const;
    PString         GetInterface()  const;
    void            SetInterface(const PString & localIf);

    static PString GenerateCallID();
	
  protected:
    bool SendPDU(SIP_PDU & pdu);
    bool ResendCANCEL();
    void SetParameters(const SIPParameters & params);

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
};




/////////////////////////////////////////////////////////////////////////
// SIPInvite

/** Session Initiation Protocol transaction for INVITE
    INVITE implements a three-way handshake to handle the human input and 
    extended duration of the transaction.
 */

class SIPInvite : SIPTransaction
{

  public:
    SIPInvite(
      SIPConnection & connection,
      const OpalRTPSessionManager & sm
    );
	

    virtual PBoolean OnReceivedResponse(SIP_PDU & response);

    //const OpalRTPSessionManager & GetSessionManager() const;
    OpalRTPSessionManager & GetSessionManager();

};

/* This is the ACK request sent when receiving a response to an outgoing
 * INVITE.
 */
class SIPAck : SIP_PDU
{
  public:
    SIPAck(
      SIPTransaction & invite,
      SIP_PDU & response
    );
};


/////////////////////////////////////////////////////////////////////////

/* This is a BYE request
 */
class SIPBye : SIPTransaction
{
    
  public:
    SIPBye(
      SIPEndPoint & ep,
      OpalTransport & trans,
      SIPDialogContext dialog
    );
    SIPBye(
      SIPConnection & conn
    );

};

/////////////////////////////////////////////////////////////////////////

class SIPRegister : SIPTransaction
{
    
  public:
    enum CompatibilityModes {
      e_FullyCompliant,                 /**< Registrar is fully compliant, we will register
                                             all listeners of all types (e.g. sip, sips etc)
                                             in the Contact field. */
      e_CannotRegisterMultipleContacts, /**< Registrar refuses to register more than one
                                             contact field. Correct behaviour is to return
                                             a contact with the fields it can accept in
                                             the 200 OK */
      e_CannotRegisterPrivateContacts   /**< Registrar refuses to register any RFC
                                             contact field. Correct behaviour is to return
                                             a contact with the fields it can accept in
                                             the 200 OK */
    };

    /// Registrar parameters
    struct Params : SIPParameters {
      Params();
      Params(const SIPRegister::Params & param);

      PCaselessString  & m_registrarAddress; // For backward compatibility
      SIPRegister::CompatibilityModes m_compatibility;
	  
	  private:
		Params & operator=(const Params &);
    };

    SIPRegister(
      SIPEndPoint   & endpoint,
      OpalTransport & transport,
      const PString & callId,
      unsigned cseq,
      const SIPRegister::Params & params
    );

    virtual SIPTransaction * CreateDuplicate() const;

};


/////////////////////////////////////////////////////////////////////////

class SIPSubscribe : SIPTransaction /NoDefaultCtors/
{
  public:
    /** Valid types for an event package
     */
    enum PredefinedPackages {
      MessageSummary,
      Presence,
      Dialog,

      NumPredefinedPackages,

      Watcher = 0x8000,

      MessageSummaryWatcher = 0x8000,
      PresenceWatcher       = 0x8001,
      DialogWatcher         = 0x8002,

      PackageMask = 0x7FFF
    };

	
    class EventPackage
    {
      public:
        EventPackage(SIPSubscribe::PredefinedPackages = SIPSubscribe::NumPredefinedPackages);
        explicit EventPackage(const PString & str);
        explicit EventPackage(const char   *  str);

        //EventPackage & operator=(PredefinedPackages pkg);

        bool operator==(SIPSubscribe::PredefinedPackages pkg) const;

        virtual PObject::Comparison InternalCompare(PINDEX offset, PINDEX length, const char * cstr) const;

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
        SIPEndPoint & ep,
        OpalTransport & trans,
        SIP_PDU & notify,
        SIP_PDU & response);
		
      bool SendResponse(
        SIP_PDU::StatusCodes status,
        const char * extra = NULL
      );
	  

      const SIPEndPoint   & m_endpoint; // should not be initialized outside of the constructor.
      const OpalTransport & m_transport {
			%GetCode
				const char * prefix = sipCpp->m_transport.GetProtoPrefix();

				if (prefix != NULL) {
					if(strcmp(prefix, "tcp$") == 0)
						sipPy = sipConvertFromNewType(reinterpret_cast<OpalTransportTCP*>(&sipCpp->m_transport), sipType_OpalTransportTCP, NULL);
		
					else if (strcmp(prefix, "udp$") == 0)
						sipPy = sipConvertFromNewType(reinterpret_cast<OpalTransportUDP*>(&sipCpp->m_transport), sipType_OpalTransportUDP, NULL);
						
					#if OPAL_PTLIB_SSL		
					else if (strcmp(prefix, "tcps$") == 0)
						sipPy = sipConvertFromNewType(reinterpret_cast<OpalTransportTCPS*>(&sipCpp->m_transport), sipType_OpalTransportTCPS, NULL);
					#endif
					
					else
						sipPy = sipConvertFromNewType(reinterpret_cast<OpalTransportIP*>(&sipCpp->m_transport), sipType_OpalTransportIP, NULL);
				}
				
				if(sipPy == NULL)
					PyErr_SetString( PyExc_RuntimeError, "Failed to cast OpalTransport to subclass type." );	
			%End
		};
				
				
      SIP_PDU       & m_notify;
      SIP_PDU       & m_response;
      bool            m_sendResponse;
    };

    struct Params : SIPParameters
    {
      Params(SIPSubscribe::PredefinedPackages pkg = SIPSubscribe::NumPredefinedPackages);

      Params(const SIPSubscribe::Params & param);

      PCaselessString & 			  m_agentAddress; // For backward compatibility
      SIPSubscribe::EventPackage      m_eventPackage;
      bool              			  m_eventList;    // Enable RFC4662
      PCaselessString   			  m_contentType;  // May be \n separated list of types
	  
	  private:
		Params & operator=(const Params &); // declare assignment operator as private, so SIP does not try to wrap it
    };

    SIPSubscribe(
        SIPEndPoint & ep,
        OpalTransport & trans,
        SIPDialogContext & dialog,
        const SIPSubscribe::Params & params
    );
	
    virtual SIPTransaction * CreateDuplicate() const;
};


typedef SIPSubscribe::EventPackage SIPEventPackage;


/////////////////////////////////////////////////////////////////////////


class SIPEventPackageHandler /Abstract/
{
public:
  virtual ~SIPEventPackageHandler();
  virtual PCaselessString GetContentType() const = 0;
  virtual bool ValidateContentType(const PString & /*type*/, const SIPMIMEInfo & /*mime*/);
  virtual bool OnReceivedNOTIFY(SIPHandler & handler, SIP_PDU & request) = 0;
  virtual PString OnSendNOTIFY(SIPHandler & /*handler*/, const PObject * /*body*/);
};



/////////////////////////////////////////////////////////////////////////

class SIPNotify : SIPTransaction
{
  public:
    SIPNotify(
        SIPEndPoint & ep,
        OpalTransport & trans,
        SIPDialogContext & dialog,
        const SIPEventPackage & eventPackage,
        const PString & state,
        const PString & body
    );
	
    virtual SIPTransaction * CreateDuplicate() const;
};


/////////////////////////////////////////////////////////////////////////

class SIPPublish : SIPTransaction
{
  public:
    SIPPublish(
      SIPEndPoint & ep,
      OpalTransport & trans,
      const PString & id,
      const PString & sipIfMatch,
      const SIPSubscribe::Params & params,
      const PString & body
    );

    virtual SIPTransaction * CreateDuplicate() const;
	
};


/////////////////////////////////////////////////////////////////////////

class SIPRefer : SIPTransaction
{
  public:
    SIPRefer(
      SIPConnection & connection,
      const SIPURL & referTo,
      const SIPURL & referred_by = SIPURL()
    );

    virtual SIPTransaction * CreateDuplicate() const;
};


/////////////////////////////////////////////////////////////////////////

/* This is not a generic NOTIFY PDU, but the minimal one
 * that gets sent when receiving a REFER
 */
class SIPReferNotify : SIPTransaction
{
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
class SIPMessage : SIPTransaction
{
  public:
    struct Params : SIPParameters
    {
      Params();

      PCaselessString m_contentType;
      PString         m_id;
      PString         m_body;
    };

    SIPMessage(
      SIPEndPoint & ep,
      OpalTransport & trans,
      const Params & params
    );
	
    SIPMessage(
      SIPConnection & connection,
      const Params & params
    );

    virtual SIPTransaction * CreateDuplicate() const;

    const SIPURL & GetLocalAddress() const;
};


/////////////////////////////////////////////////////////////////////////


/* This is an OPTIONS request
 */
class SIPOptions : SIPTransaction
{
    
  public:
    struct Params : SIPParameters
    {
      Params();

      PCaselessString m_acceptContent;
      PCaselessString m_contentType;
      PString         m_body;
    };

    SIPOptions(
      SIPEndPoint & ep,
      OpalTransport & trans,
      const PString & id,
      const Params & params
    );
    SIPOptions(
      SIPConnection & conn,
      const Params & params
    );

    virtual SIPTransaction * CreateDuplicate() const;
};


/////////////////////////////////////////////////////////////////////////

/* This is an INFO request
 */
class SIPInfo : SIPTransaction
{
    
  public:
    struct Params
    {
      Params(const PString & contentType = PString::Empty(),
             const PString & body = PString::Empty());

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
class SIPPing : SIPTransaction
{

  public:
    SIPPing(
      SIPEndPoint & ep,
      OpalTransport & trans,
      const SIPURL & address
    );

    virtual SIPTransaction * CreateDuplicate() const;
	
};


/////////////////////////////////////////////////////////////////////////

/* This is a PRACK PDU
 */
class SIPPrack : SIPTransaction
{

  public:
    SIPPrack(
      SIPConnection & conn,
      const PString & rack
    );

    virtual SIPTransaction * CreateDuplicate() const;
};




// End of File ///////////////////////////////////////////////////////////////
