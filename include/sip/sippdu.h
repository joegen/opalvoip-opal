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
 * Revision 1.2003  2002/03/08 06:28:19  craigs
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


/////////////////////////////////////////////////////////////////////////

class SIPURL : public PURL
{
  PCLASSINFO(SIPURL, PURL);
  public:
    SIPURL();
    SIPURL(
      const char * str,
      BOOL special = FALSE
    );
    SIPURL(
      const PString & str,
      BOOL special = FALSE
    );
    SIPURL(
      const PString & name,
      const OpalTransportAddress & address,
      WORD listenerPort = 0
    );

    void Parse(
      const char * cstr,
      BOOL special = FALSE
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
    PString GetSIPString(const char * fullForm, const char * compactForm) const;

    void SetForm(BOOL v) { compactForm = v; }

    PString GetContentType() const         { return GetSIPString("Content-Type",     "c"); }
    void SetContentType(const PString & v);

    PString GetContentEncoding() const         { return GetSIPString("Content-Encoding", "e"); }
    void SetContentEncoding(const PString & v);

    PString GetFrom() const            { return GetSIPString("From",             "f"); }
    void SetFrom(const PString & v);

    PString GetCallID() const          { return GetSIPString("Call-ID",          "i"); }
    void SetCallID(const PString & v);

    PString GetContact() const         { return GetSIPString("Contact",          "m"); }
    void SetContact(const PString & v);

    PString GetSubject() const         { return GetSIPString("Subject",          "s"); }
    void SetSubject(const PString & v);

    PString GetTo() const              { return GetSIPString("To",               "t"); }
    void SetTo(const PString & v);

    PString GetVia() const             { return GetSIPString("Via",              "v"); }
    void SetVia(const PString & v);

    PINDEX  GetContentLength() const;
    void SetContentLength(PINDEX v);

    PString GetCSeq() const            { return (*this)("CSeq"); }
    void SetCSeq(const PString & v);

    PINDEX GetCSeqIndex() const;

  protected:
    BOOL compactForm;
};

/////////////////////////////////////////////////////////////////////////

class SIP_PDU : public PObject
{
  PCLASSINFO(SIP_PDU, PObject);
  public:
    enum StatusCodes {
      Information_Trying                       = 100,
      Information_Ringing                      = 180,
      Information_CallForwarded                = 181,
      Information_Queued                       = 182,
      Information_Session_Progress             = 183,

      Successful_OK                            = 200,

      Redirection_MovedTemporarily             = 302,

      Failure_BadRequest                       = 400,
      Failure_UnAuthorised                     = 401,
      Failure_PaymentRequired                  = 402,
      Failure_Forbidden                        = 403,
      Failure_NotFound                         = 404,
      Failure_MethodNotAllowed                 = 405,
      Failure_NotAcceptable                    = 406,
      Failure_ProxyAuthenticationRequired      = 407,
      Failure_RequestTimeout                   = 408,
      Failure_Conflict                         = 409,
      Failure_Gone                             = 410,
      Failure_LengthRequired                   = 411,
      Failure_RequestEntityTooLarge            = 412,
      Failure_RequestURITooLong                = 413,
      Failure_UnsupportedMediaType             = 414,
      Failure_BadExtension                     = 420,
      Failure_TemporarilyUnavailable           = 480,
      Failure_CallLegOrTransactionDoesNotExist = 481,
      Failure_LoopDetected                     = 482,
      Failure_TooManyHops                      = 483,
      Failure_AddressIncomplete                = 484,
      Failure_Ambiguous                        = 485,
      Failure_BusyHere                         = 486
    };

    SIP_PDU(
      OpalTransport & transport,
      SIPConnection * connection = NULL
    );
    SIP_PDU(
      const SIP_PDU & request,
      StatusCodes code,
      const char * extra = NULL
    );
    ~SIP_PDU();

    /**Read PDU from the specified transport.
      */
    BOOL Read();

    /**Write the PDU to the transport.
      */
    BOOL Write();

    const PString & GetMethod() const        { return method; }
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
    void SetSDP(const SDPSessionDescription & s) { sdp = new SDPSessionDescription(s); }
    OpalTransport & GetTransport() const     { return transport; }
    SIPConnection * GetConnection() const    { return connection; }
    void SetConnection(SIPConnection * conn) { connection = conn; }

    void BuildVia();
    void BuildCommon();
    void BuildINVITE();
    void BuildBYE();
    void BuildACK();
    void BuildREGISTER(
      const SIPEndPoint & endpoint,
      const SIPURL & name,
      const SIPURL & contact
    );

    void SendResponse(
      StatusCodes code,
      const char * extra = NULL
    );

  protected:
    PString     method;
    StatusCodes statusCode;
    SIPURL      uri;
    unsigned    versionMajor;
    unsigned    versionMinor;
    PString     info;
    SIPMIMEInfo mime;
    PString     entityBody;

    SDPSessionDescription * sdp;

    OpalTransport & transport;
    SIPConnection * connection;
};

typedef void (SIPConnection::* SIPMethodFunction)(SIP_PDU & pdu);


#define MAX_SIP_UDP_PDU_SIZE (8*1024)


#endif // __OPAL_SIPPDU_H


// End of File ///////////////////////////////////////////////////////////////
