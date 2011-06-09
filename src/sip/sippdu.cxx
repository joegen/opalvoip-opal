/*
 * sippdu.cxx
 *
 * Session Initiation Protocol PDU support.
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
 * The Initial Developer of the Original Code is Equivalence Pty. Ltd.
 *
 * Contributor(s): ______________________________________.
 *
 * $Revision$
 * $Author$
 * $Date$
 */

#include <ptlib.h>
#include <opal/buildopts.h>

#if OPAL_SIP

#ifdef __GNUC__
#pragma implementation "sippdu.h"
#endif

#include <sip/sippdu.h>

#include <sip/sipep.h>
#include <sip/sipcon.h>
#include <opal/call.h>
#include <opal/manager.h>
#include <opal/connection.h>
#include <opal/transports.h>

#include <ptclib/cypher.h>
#include <ptclib/pdns.h>


#define  SIP_VER_MAJOR  2
#define  SIP_VER_MINOR  0


#define new PNEW

static SIPAuthenticationFactory::Worker<SIPDigestAuthentication> sip_md5Authenticator("digest");

////////////////////////////////////////////////////////////////////////////

static const char * const MethodNames[SIP_PDU::NumMethods] = {
  "INVITE",
  "ACK",
  "OPTIONS",
  "BYE",
  "CANCEL",
  "REGISTER",
  "SUBSCRIBE",
  "NOTIFY",
  "REFER",
  "MESSAGE",
  "INFO",
  "PING",
  "PUBLISH"
};

#if PTRACING
ostream & operator<<(ostream & strm, SIP_PDU::Methods method)
{
  if (method < SIP_PDU::NumMethods)
    strm << MethodNames[method];
  else
    strm << "SIP_PDU_Method<" << (unsigned)method << '>';
  return strm;
}
#endif


const char * SIP_PDU::GetStatusCodeDescription(int code)
{
  static struct {
    int code;
    const char * desc;
  } sipErrorDescriptions[] = {
    { SIP_PDU::Information_Trying,                  "Trying" },
    { SIP_PDU::Information_Ringing,                 "Ringing" },
    { SIP_PDU::Information_CallForwarded,           "Call Forwarded" },
    { SIP_PDU::Information_Queued,                  "Queued" },
    { SIP_PDU::Information_Session_Progress,        "Progress" },

    { SIP_PDU::Successful_OK,                       "OK" },
    { SIP_PDU::Successful_Accepted,                 "Accepted" },

    { SIP_PDU::Redirection_MultipleChoices,         "Multiple Choices" },
    { SIP_PDU::Redirection_MovedPermanently,        "Moved Permanently" },
    { SIP_PDU::Redirection_MovedTemporarily,        "Moved Temporarily" },
    { SIP_PDU::Redirection_UseProxy,                "Use Proxy" },
    { SIP_PDU::Redirection_AlternativeService,      "Alternative Service" },

    { SIP_PDU::Failure_BadRequest,                  "BadRequest" },
    { SIP_PDU::Failure_UnAuthorised,                "Unauthorised" },
    { SIP_PDU::Failure_PaymentRequired,             "Payment Required" },
    { SIP_PDU::Failure_Forbidden,                   "Forbidden" },
    { SIP_PDU::Failure_NotFound,                    "Not Found" },
    { SIP_PDU::Failure_MethodNotAllowed,            "Method Not Allowed" },
    { SIP_PDU::Failure_NotAcceptable,               "Not Acceptable" },
    { SIP_PDU::Failure_ProxyAuthenticationRequired, "Proxy Authentication Required" },
    { SIP_PDU::Failure_RequestTimeout,              "Request Timeout" },
    { SIP_PDU::Failure_Conflict,                    "Conflict" },
    { SIP_PDU::Failure_Gone,                        "Gone" },
    { SIP_PDU::Failure_LengthRequired,              "Length Required" },
    { SIP_PDU::Failure_RequestEntityTooLarge,       "Request Entity Too Large" },
    { SIP_PDU::Failure_RequestURITooLong,           "Request URI Too Long" },
    { SIP_PDU::Failure_UnsupportedMediaType,        "Unsupported Media Type" },
    { SIP_PDU::Failure_UnsupportedURIScheme,        "Unsupported URI Scheme" },
    { SIP_PDU::Failure_BadExtension,                "Bad Extension" },
    { SIP_PDU::Failure_ExtensionRequired,           "Extension Required" },
    { SIP_PDU::Failure_IntervalTooBrief,            "Interval Too Brief" },
    { SIP_PDU::Failure_TemporarilyUnavailable,      "Temporarily Unavailable" },
    { SIP_PDU::Failure_TransactionDoesNotExist,     "Call Leg/Transaction Does Not Exist" },
    { SIP_PDU::Failure_LoopDetected,                "Loop Detected" },
    { SIP_PDU::Failure_TooManyHops,                 "Too Many Hops" },
    { SIP_PDU::Failure_AddressIncomplete,           "Address Incomplete" },
    { SIP_PDU::Failure_Ambiguous,                   "Ambiguous" },
    { SIP_PDU::Failure_BusyHere,                    "Busy Here" },
    { SIP_PDU::Failure_RequestTerminated,           "Request Terminated" },
    { SIP_PDU::Failure_NotAcceptableHere,           "Not Acceptable Here" },
    { SIP_PDU::Failure_BadEvent,                    "Bad Event" },
    { SIP_PDU::Failure_RequestPending,              "Request Pending" },
    { SIP_PDU::Failure_Undecipherable,              "Undecipherable" },

    { SIP_PDU::Failure_InternalServerError,         "Internal Server Error" },
    { SIP_PDU::Failure_NotImplemented,              "Not Implemented" },
    { SIP_PDU::Failure_BadGateway,                  "Bad Gateway" },
    { SIP_PDU::Failure_ServiceUnavailable,          "Service Unavailable" },
    { SIP_PDU::Failure_ServerTimeout,               "Server Time-out" },
    { SIP_PDU::Failure_SIPVersionNotSupported,      "SIP Version Not Supported" },
    { SIP_PDU::Failure_MessageTooLarge,             "Message Too Large" },

    { SIP_PDU::GlobalFailure_BusyEverywhere,        "Busy Everywhere" },
    { SIP_PDU::GlobalFailure_Decline,               "Decline" },
    { SIP_PDU::GlobalFailure_DoesNotExistAnywhere,  "Does Not Exist Anywhere" },
    { SIP_PDU::GlobalFailure_NotAcceptable,         "Not Acceptable" },

    { SIP_PDU::Local_TransportError,                "Transport Error" },
    { SIP_PDU::Local_BadTransportAddress,           "Invalid Address/Hostname" },

    { 0 }
  };

  for (PINDEX i = 0; sipErrorDescriptions[i].code != 0; i++) {
    if (sipErrorDescriptions[i].code == code)
      return sipErrorDescriptions[i].desc;
  }
  return "";
}


ostream & operator<<(ostream & strm, SIP_PDU::StatusCodes status)
{
  strm << (unsigned)status;
  const char * info = SIP_PDU::GetStatusCodeDescription(status);
  if (info != NULL && *info != '\0')
    strm << ' ' << info;
  return strm;
}


static struct {
  char         compact;
  const char * full;
} const CompactForms[] = {
  { 'l', "Content-Length" },
  { 'c', "Content-Type" },
  { 'e', "Content-Encoding" },
  { 'f', "From" },
  { 'i', "Call-ID" },
  { 'm', "Contact" },
  { 's', "Subject" },
  { 't', "To" },
  { 'v', "Via" },
  { 'r', "Refer-To" },
  { 'b', "Referred-By" },
  { 'k', "Supported" },
  { 'o', "Event" }
};


static const char * const AlgorithmNames[SIPDigestAuthentication::NumAlgorithms] = {
  "MD5"
};


/////////////////////////////////////////////////////////////////////////////

SIPURL::SIPURL()
{
}


SIPURL::SIPURL(const char * str, const char * defaultScheme)
{
  Parse(str, defaultScheme);
}


SIPURL::SIPURL(const PString & str, const char * defaultScheme)
{
  Parse(str, defaultScheme);
}


SIPURL::SIPURL(const PString & name,
               const OpalTransportAddress & _address,
               WORD listenerPort)
{
  if (strncmp(name, "sip:", 4) == 0 || strncmp(name, "sips:", 5) == 0)
    Parse(name);
  else {
    OpalTransportAddress address(_address);
    if (address.IsEmpty() && (name.Find('$') != P_MAX_INDEX)) 
      address = name;
    ParseAsAddress(name, address, listenerPort);
  }
}

SIPURL::SIPURL(const OpalTransportAddress & _address, WORD listenerPort)
{
  ParseAsAddress("", _address, listenerPort);
}
  
void SIPURL::ParseAsAddress(const PString & name, const OpalTransportAddress & address, WORD listenerPort)
{
  PIPSocket::Address ip;
  WORD port;
  if (address.GetIpAndPort(ip, port)) {
    PString transProto;
    WORD defaultPort = 5060;

    PStringStream s;
    s << "sip";

    PCaselessString proto = address.GetProto();
    if (proto == "tcps") {
      defaultPort = 5061;
      s << 's';
      // use default tranport=UDP
    }
    else if (proto != "udp")
      transProto = proto; // Typically "tcp"
    // else use default UDP

    s << ':';
    if (!name.IsEmpty())
      s << name << '@';
    if (ip.GetVersion() == 6)
      s << '[' << ip << ']';
    else
      s << ip;

    if (listenerPort == 0)
      listenerPort = port;
    if (listenerPort != 0 && listenerPort != defaultPort)
      s << ':' << listenerPort;

    if (!transProto.IsEmpty())
      s << ";transport=" << transProto;

    Parse(s);
  }
}


PBoolean SIPURL::InternalParse(const char * cstr, const char * p_defaultScheme)
{
  /* This will try to parse an SIP URI according to the RFC3261 EBNF

        Contact        =  ("Contact" / "m" ) HCOLON
                          ( STAR / (contact-param *(COMMA contact-param)))
        contact-param  =  (name-addr / addr-spec) *(SEMI contact-params)

        From           =  ( "From" / "f" ) HCOLON from-spec
        from-spec      =  ( name-addr / addr-spec ) *( SEMI from-param )

        Record-Route   =  "Record-Route" HCOLON rec-route *(COMMA rec-route)
        rec-route      =  name-addr *( SEMI rr-param )

        Reply-To       =  "Reply-To" HCOLON rplyto-spec
        rplyto-spec    =  ( name-addr / addr-spec ) *( SEMI rplyto-param )

        Route          =  "Route" HCOLON route-param *(COMMA route-param)
        route-param    =  name-addr *( SEMI rr-param )

        To             =  ( "To" / "t" ) HCOLON
                          ( name-addr / addr-spec ) *( SEMI to-param )


     For all of the above the field parameters (contact-params, to-param etc)
     all degenerate into special cases of the generic-param production. The
     generic-param is always included as an option for all these fields as
     well, so for general parsing we just need to parse the EBNF:

        generic-param  =  token [ EQUAL gen-value ]
        gen-value      =  token / host / quoted-string

     The URI is then embedded in:

        name-addr      =  [ display-name ] LAQUOT addr-spec RAQUOT
        addr-spec      =  SIP-URI / SIPS-URI / absoluteURI
        display-name   =  *(token LWS) / quoted-string

        token          =  1*(alphanum / "-" / "." / "!" / "%" / "*"
                             / "_" / "+" / "`" / "'" / "~" )

        quoted-string  =  SWS DQUOTE *(qdtext / quoted-pair ) DQUOTE
        qdtext         =  LWS / %x21 / %x23-5B / %x5D-7E
                                / UTF8-NONASCII
        quoted-pair    =  "\" (%x00-09 / %x0B-0C / %x0E-7F)


     So, what all that means is we distinguish between name-addr and addr-spec
     by the '<' character, but we can't just search for it if there is a
     quoted-string before it in the optional display-name field. So check for
     that and remove it first, then look for the the '<' which tells us if
     there COULD be a generic-param later.
   */

  displayName.MakeEmpty();
  fieldParameters.MakeEmpty();

  while (isspace(*cstr))
    cstr++;
  PString str = cstr;

  PINDEX endQuote = 0;
  if (str[0] == '"') {
    do {
      endQuote = str.Find('"', endQuote+1);
      if (endQuote == P_MAX_INDEX) {
        PTRACE(1, "SIP\tNo closing double quote in URI: " << str);
        return false; // No close quote!
      }
    } while (str[endQuote-1] == '\\');

    displayName = str(1, endQuote-1);

    PINDEX backslash;
    while ((backslash = displayName.Find('\\')) != P_MAX_INDEX)
      displayName.Delete(backslash, 1);
  }

  // see if URL is just a URI or it contains a display address as well
  PINDEX startBracket = str.Find('<', endQuote);
  PINDEX endBracket = str.Find('>', startBracket);

  const char * defaultScheme = p_defaultScheme != NULL ? p_defaultScheme : "sip";

  // see if URL is just a URI or it contains a display address as well
  if (startBracket == P_MAX_INDEX || endBracket == P_MAX_INDEX) {
    if (!PURL::InternalParse(cstr, defaultScheme))
      return false;
  }
  else {
    // get the URI from between the angle brackets
    if (!PURL::InternalParse(str(startBracket+1, endBracket-1), defaultScheme))
      return false;

    fieldParameters = str.Mid(endBracket+1).Trim();

    if (endQuote == 0) {
      // There were no double quotes around the display name, take
      // everything before the start angle bracket, sans whitespace
      displayName = str.Left(startBracket).Trim();
    }
  }

  return !IsEmpty();
}


PObject::Comparison SIPURL::Compare(const PObject & obj) const
{
  PAssert(PIsDescendant(&obj, SIPURL), PInvalidCast);
  const SIPURL & other = (const SIPURL &)obj;

  // RFC3261 Section 19.1.4 matching rules, hideously complicated!

#define COMPARE_COMPONENT(component) \
  if (component != other.component) \
    return component < other.component ? LessThan : GreaterThan

  COMPARE_COMPONENT(GetScheme());
  COMPARE_COMPONENT(GetUserName());
  COMPARE_COMPONENT(GetPassword());
  COMPARE_COMPONENT(GetHostName());
  COMPARE_COMPONENT(GetPort());
  COMPARE_COMPONENT(GetPortSupplied());

  // If URI parameter exists in both then must be equal
  for (PINDEX i = 0; i < paramVars.GetSize(); i++) {
    PString param = paramVars.GetKeyAt(i);
    if (other.paramVars.Contains(param))
      COMPARE_COMPONENT(paramVars[param]);
  }
  COMPARE_COMPONENT(paramVars("user"));
  COMPARE_COMPONENT(paramVars("ttl"));
  COMPARE_COMPONENT(paramVars("method"));

  return EqualTo;
}


PString SIPURL::AsQuotedString() const
{
  PStringStream s;

  if (!displayName)
    s << '"' << displayName << "\" ";
  s << '<' << AsString() << '>';

  if (!fieldParameters.IsEmpty()) {
    if (fieldParameters[0] != ';')
      s << ';';
    s << fieldParameters;
  }

  return s;
}


PString SIPURL::GetDisplayName (PBoolean useDefault) const
{
  PString s;
  PINDEX tag;
    
  s = displayName;

  if (displayName.IsEmpty () && useDefault) {

    s = AsString ();
    s.Replace ("sip:", "");

    /* There could be a tag if we are using the URL,
     * remove it
     */
    tag = s.Find (';');
    if (tag != P_MAX_INDEX)
      s = s.Left (tag);
  }

  return s;
}


OpalTransportAddress SIPURL::GetHostAddress() const
{
  PString addr;
  if (scheme *= "sips")
    addr = "tcps$";
  else
    addr  = paramVars("transport", "udp") + '$';

  if (paramVars.Contains("maddr"))
    addr += paramVars["maddr"];
  else
    addr += hostname;

  if (port > 0)
    addr.sprintf(":%u", port);

  return addr;
}


void SIPURL::Sanitise(UsageContext context)
{
  // RFC3261, 19.1.1 Table 1
  static struct {
    const char * name;
    unsigned     contexts;
  } const SanitaryFields[] = {
    { "method",    (1<<RequestURI)|(1<<ToURI)|(1<<FromURI)|(1<<ContactURI)|(1<<RouteURI)|(1<<RegisterURI) },
    { "maddr",                     (1<<ToURI)|(1<<FromURI) },
    { "ttl",                       (1<<ToURI)|(1<<FromURI)|(1<<RouteURI)|                (1<<RegisterURI) },
    { "transport",                 (1<<ToURI)|(1<<FromURI) },
    { "lr",                        (1<<ToURI)|(1<<FromURI)|(1<<ContactURI)|              (1<<RegisterURI) },
    { "tag",       (1<<RequestURI)|                        (1<<ContactURI)|(1<<RouteURI)|(1<<RegisterURI)|(1<<ExternalURI) }
  };

  for (PINDEX i = 0; i < PARRAYSIZE(SanitaryFields); i++) {
    if (SanitaryFields[i].contexts&(1<<context))
      paramVars.RemoveAt(PCaselessString(SanitaryFields[i].name));
  }

  if (context != ContactURI && context != ExternalURI)
    queryVars.RemoveAll();

  if (context == ToURI || context == FromURI)
    port = (scheme *= "sips") ? 5061 : 5060;

  if (context == RegisterURI) {
    username.MakeEmpty();
    password.MakeEmpty();
  }

  Recalculate();
}


#if OPAL_PTLIB_DNS
PBoolean SIPURL::AdjustToDNS(PINDEX entry)
{
  // RFC3263 states we do not do lookup if explicit port mentioned
  if (GetPortSupplied()) {
    PTRACE(4, "SIP\tNo SRV lookup as has explicit port number.");
    return true;
  }

  // Or it is a valid IP address, not a domain name
  PIPSocket::Address ip = GetHostName();
  if (ip.IsValid())
    return PTrue;

  // Do the SRV lookup, if fails, then we actually return TRUE so outer loops
  // can use the original host name value.
  PIPSocketAddressAndPortVector addrs;
  if (!PDNS::LookupSRV(GetHostName(), "_sip._" + paramVars("transport", "udp"), GetPort(), addrs)) {
    PTRACE(4, "SIP\tNo SRV record found.");
    return PTrue;
  }

  // Got the SRV list, return FALSE if outer loop has got to the end of it
  if (entry >= (PINDEX)addrs.size()) {
    PTRACE(4, "SIP\tRan out of SRV records at entry " << entry);
    return PFalse;
  }

  PTRACE(4, "SIP\tAttempting SRV record entry " << entry << ": " << addrs[entry].AsString());

  // Adjust our host and port to what the DNS SRV record says
  SetHostName(addrs[entry].GetAddress().AsString());
  SetPort(addrs[entry].GetPort());
  return PTrue;
}
#else
PBoolean SIPURL::AdjustToDNS(PINDEX)
{
  return PTrue;
}
#endif


PString SIPURL::GenerateTag()
{
  return OpalGloballyUniqueID().AsString();
}


void SIPURL::SetTag(const PString & tag)
{
  if (fieldParameters.Find(";tag=") != P_MAX_INDEX)
    return;

  fieldParameters += ";tag=" + tag;
}


/////////////////////////////////////////////////////////////////////////////

SIPMIMEInfo::SIPMIMEInfo(PBoolean _compactForm)
  : compactForm(_compactForm)
{
}


void SIPMIMEInfo::PrintOn(ostream & strm) const
{
  for (PINDEX i = 0; i < GetSize(); i++) {
    PCaselessString name = GetKeyAt(i);
    PString value = GetDataAt(i);

    if (compactForm) {
      for (PINDEX i = 0; i < PARRAYSIZE(CompactForms); ++i) {
        if (name == CompactForms[i].full) {
          name = CompactForms[i].compact;
          break;
        }
      }
    }

    if (value.FindOneOf("\r\n") == P_MAX_INDEX)
      strm << name << ": " << value << "\r\n";
    else {
      PStringArray vals = value.Lines();
      for (PINDEX j = 0; j < vals.GetSize(); j++)
        strm << name << ": " << vals[j] << "\r\n";
    }
  }

  strm << "\r\n";
}


void SIPMIMEInfo::ReadFrom(istream & strm)
{
  PMIMEInfo::ReadFrom(strm);

  for (PINDEX i = 0; i < PARRAYSIZE(CompactForms); ++i) {
    PCaselessString compact(CompactForms[i].compact);
    if (Contains(compact)) {
      SetAt(CompactForms[i].full, *GetAt(compact));
      RemoveAt(compact);
    }
  }
}


PINDEX SIPMIMEInfo::GetContentLength() const
{
  PString len = GetString("Content-Length");
  if (len.IsEmpty())
    return 0; //P_MAX_INDEX;
  return len.AsInteger();
}

PBoolean SIPMIMEInfo::IsContentLengthPresent() const
{
  return Contains("Content-Length");
}


PCaselessString SIPMIMEInfo::GetContentType(bool includeParameters) const
{
  PCaselessString str = GetString("Content-Type");
  return str.Left(includeParameters ? P_MAX_INDEX : str.Find(';')).Trim();
}


void SIPMIMEInfo::SetContentType(const PString & v)
{
  SetAt("Content-Type",  v);
}


PCaselessString SIPMIMEInfo::GetContentEncoding() const
{
  return GetString("Content-Encoding");
}


void SIPMIMEInfo::SetContentEncoding(const PString & v)
{
  SetAt("Content-Encoding",  v);
}


PString SIPMIMEInfo::GetFrom() const
{
  return GetString("From");
}


void SIPMIMEInfo::SetFrom(const PString & v)
{
  SetAt("From",  v);
}

PString SIPMIMEInfo::GetPAssertedIdentity() const
{
  return (*this)["P-Asserted-Identity"];
}

void SIPMIMEInfo::SetPAssertedIdentity(const PString & v)
{
  SetAt("P-Asserted-Identity", v);
}

PString SIPMIMEInfo::GetPPreferredIdentity() const
{
  return (*this)["P-Preferred-Identity"];
}

void SIPMIMEInfo::SetPPreferredIdentity(const PString & v)
{
  SetAt("P-Preferred-Identity", v);
}

PString SIPMIMEInfo::GetCallID() const
{
  return GetString("Call-ID");
}


void SIPMIMEInfo::SetCallID(const PString & v)
{
  SetAt("Call-ID",  v);
}


PString SIPMIMEInfo::GetContact() const
{
  return GetString("Contact");
}


bool SIPMIMEInfo::GetContacts(std::list<SIPURL> & contacts) const
{
  PStringArray lines = GetString("Contact").Lines();
  for (PINDEX i = 0; i < lines.GetSize(); i++) {
    PStringArray items = lines[i].Tokenise(',');
    for (PINDEX j = 0; j < items.GetSize(); j++)
      contacts.push_back(items[j]);
  }

  return !contacts.empty();
}


void SIPMIMEInfo::SetContact(const PString & v)
{
  SetAt("Contact",  v);
}


void SIPMIMEInfo::SetContact(const SIPURL & url)
{
  SetContact(url.AsQuotedString());
}


PString SIPMIMEInfo::GetSubject() const
{
  return GetString("Subject");
}


void SIPMIMEInfo::SetSubject(const PString & v)
{
  SetAt("Subject",  v);
}


PString SIPMIMEInfo::GetTo() const
{
  return GetString("To");
}


void SIPMIMEInfo::SetTo(const PString & v)
{
  SetAt("To",  v);
}


PString SIPMIMEInfo::GetVia() const
{
  return GetString("Via");
}


void SIPMIMEInfo::SetVia(const PString & v)
{
  if (!v.IsEmpty())
    SetAt("Via",  v);
}


PStringList SIPMIMEInfo::GetViaList() const
{
  PStringList viaList;
  PString s = GetVia();
  if (s.FindOneOf("\r\n") != P_MAX_INDEX)
    viaList = s.Lines();
  else
    viaList = s.Tokenise(",", PFalse);

  return viaList;
}


void SIPMIMEInfo::SetViaList(const PStringList & v)
{
  PStringStream fieldValue;
  for (PStringList::const_iterator via = v.begin(); via != v.end(); ++via) {
    if (!fieldValue.IsEmpty())
      fieldValue << '\n';
    fieldValue << *via;
  }
  SetAt("Via", fieldValue);
}


PString SIPMIMEInfo::GetReferTo() const
{
  return GetString("Refer-To");
}


void SIPMIMEInfo::SetReferTo(const PString & r)
{
  SetAt("Refer-To",  r);
}

PString SIPMIMEInfo::GetReferredBy() const
{
  return GetString("Referred-By");
}


void SIPMIMEInfo::SetReferredBy(const PString & r)
{
  SetAt("Referred-By",  r);
}

void SIPMIMEInfo::SetContentLength(PINDEX v)
{
  SetAt("Content-Length", PString(PString::Unsigned, v));
}


PString SIPMIMEInfo::GetCSeq() const
{
  return GetString("CSeq");       // no compact form
}


void SIPMIMEInfo::SetCSeq(const PString & v)
{
  SetAt("CSeq",  v);            // no compact form
}


PStringList SIPMIMEInfo::GetRoute() const
{
  return GetRouteList("Route", false);
}


void SIPMIMEInfo::SetRoute(const PString & v)
{
  if (!v.IsEmpty())
    SetAt("Route",  v);
}


void SIPMIMEInfo::SetRoute(const PStringList & v)
{
  if (!v.IsEmpty())
    SetRouteList("Route",  v);
}


PStringList SIPMIMEInfo::GetRecordRoute(bool reversed) const
{
  return GetRouteList("Record-Route", reversed);
}


void SIPMIMEInfo::SetRecordRoute(const PStringList & v)
{
  if (!v.IsEmpty())
    SetRouteList("Record-Route",  v);
}


PStringList SIPMIMEInfo::GetRouteList(const char * name, bool reversed) const
{
  PStringList routeSet;

  PString s = GetString(name);
  PINDEX left;
  PINDEX right = 0;
  while ((left = s.Find('<', right)) != P_MAX_INDEX &&
         (right = s.Find('>', left)) != P_MAX_INDEX &&
         (right - left) > 5) {
    PString * pstr = new PString(s(left+1, right-1));
    if (reversed)
      routeSet.InsertAt(0, pstr);
    else
      routeSet.Append(pstr);
  }

  return routeSet;
}


void SIPMIMEInfo::SetRouteList(const char * name, const PStringList & v)
{
  PStringStream s;

  for (PStringList::const_iterator via = v.begin(); via != v.end(); ++via) {
    if (!s.IsEmpty())
      s << ',';
    s << '<' << *via << '>';
  }

  SetAt(name,  s);
}


PString SIPMIMEInfo::GetAccept() const
{
  return GetString("Accept");    // no compact form
}


void SIPMIMEInfo::SetAccept(const PString & v)
{
  SetAt("Accept", v);  // no compact form
}


PString SIPMIMEInfo::GetAcceptEncoding() const
{
  return GetString("Accept-Encoding");   // no compact form
}


void SIPMIMEInfo::SetAcceptEncoding(const PString & v)
{
  SetAt("Accept-Encoding", v); // no compact form
}


PString SIPMIMEInfo::GetAcceptLanguage() const
{
  return GetString("Accept-Language");   // no compact form
}


void SIPMIMEInfo::SetAcceptLanguage(const PString & v)
{
  SetAt("Accept-Language", v); // no compact form
}


PString SIPMIMEInfo::GetAllow() const
{
  return GetString("Allow");     // no compact form
}


void SIPMIMEInfo::SetAllow(const PString & v)
{
  SetAt("Allow", v);   // no compact form
}



PString SIPMIMEInfo::GetDate() const
{
  return GetString("Date");      // no compact form
}


void SIPMIMEInfo::SetDate(const PString & v)
{
  SetAt("Date", v);    // no compact form
}


void SIPMIMEInfo::SetDate(const PTime & t)
{
  SetDate(t.AsString(PTime::RFC1123, PTime::GMT)) ;
}


void SIPMIMEInfo::SetDate(void) // set to current date
{
  SetDate(PTime()) ;
}

        
unsigned SIPMIMEInfo::GetExpires(unsigned dflt) const
{
  return GetInteger("Expires", dflt);      // no compact form
}


void SIPMIMEInfo::SetExpires(unsigned v)
{
  SetAt("Expires", PString(PString::Unsigned, v));      // no compact form
}


PINDEX SIPMIMEInfo::GetMaxForwards() const
{
  return GetInteger("Max-Forwards", P_MAX_INDEX);       // no compact form
}


void SIPMIMEInfo::SetMaxForwards(PINDEX v)
{
  SetAt("Max-Forwards", PString(PString::Unsigned, v)); // no compact form
}


PINDEX SIPMIMEInfo::GetMinExpires() const
{
  return GetInteger("Min-Expires", P_MAX_INDEX);        // no compact form
}


void SIPMIMEInfo::SetMinExpires(PINDEX v)
{
  SetAt("Min-Expires",  PString(PString::Unsigned, v)); // no compact form
}


PString SIPMIMEInfo::GetProxyAuthenticate() const
{
  return GetString("Proxy-Authenticate");        // no compact form
}


void SIPMIMEInfo::SetProxyAuthenticate(const PString & v)
{
  SetAt("Proxy-Authenticate",  v);      // no compact form
}


PString SIPMIMEInfo::GetSupported() const
{
  return GetString("Supported");
}

void SIPMIMEInfo::SetSupported(const PString & v)
{
  SetAt("Supported",  v);
}


PString SIPMIMEInfo::GetUnsupported() const
{
  return GetString("Unsupported");       // no compact form
}


void SIPMIMEInfo::SetUnsupported(const PString & v)
{
  SetAt("Unsupported",  v);     // no compact form
}


PString SIPMIMEInfo::GetEvent() const
{
  return GetString("Event");
}


void SIPMIMEInfo::SetEvent(const PString & v)
{
  SetAt("Event",  v);
}


PCaselessString SIPMIMEInfo::GetSubscriptionState() const
{
  return GetString("Subscription-State");       // no compact form
}


void SIPMIMEInfo::SetSubscriptionState(const PString & v)
{
  SetAt("Subscription-State",  v);     // no compact form
}


PString SIPMIMEInfo::GetUserAgent() const
{
  return GetString("User-Agent");        // no compact form
}


void SIPMIMEInfo::SetUserAgent(const PString & v)
{
  SetAt("User-Agent",  v);     // no compact form
}


PString SIPMIMEInfo::GetOrganization() const
{
  return GetString("Organization");        // no compact form
}


void SIPMIMEInfo::SetOrganization(const PString & v)
{
  SetAt("Organization",  v);     // no compact form
}


static const char UserAgentTokenChars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-.!%*_+`'~";

void SIPMIMEInfo::GetProductInfo(OpalProductInfo & info)
{
  PCaselessString str = GetUserAgent();
  if (str.IsEmpty()) {
    str = GetString("Server");
    if (str.IsEmpty()) {
      PTRACE(4, "SIP\tNo User-Agent or Server fields, Product Info unknown.");
      return; // Have nothing, change nothing
    }
  }

  // This is not strictly correct according to he BNF, but we cheat
  // and assume that the prod/ver tokens are first and if there is an
  // explicit comment field, it is always last. If any other prod/ver
  // tokens are present, then they will end up as the comments too.
  // All other variations just end up as one big comment

  PINDEX endFirstToken = str.FindSpan(UserAgentTokenChars);
  if (endFirstToken == 0) {
    info.name = str;
    info.vendor = info.version = PString::Empty();
    info.manufacturerCode = info.t35Extension = info.t35CountryCode = 0;
    PTRACE(4, "SIP\tProduct Info: name=\"" << str << '"');
    return;
  }

  PINDEX endSecondToken = endFirstToken;
  if (endFirstToken != P_MAX_INDEX && str[endFirstToken] == '/')
    endSecondToken = str.FindSpan(UserAgentTokenChars, endFirstToken+1);

  info.name = str.Left(endFirstToken);
  info.version = str(endFirstToken+1, endSecondToken);
  info.vendor = GetOrganization();
  info.comments = str.Mid(endSecondToken+1).Trim();
  PTRACE(4, "SIP\tProduct Info: name=\"" << info.name << "\","
                           " version=\"" << info.version << "\","
                            " vendor=\"" << info.vendor << "\","
                          " comments=\"" << info.comments << '"');
}


void SIPMIMEInfo::SetProductInfo(const PString & ua, const OpalProductInfo & info)
{
  PString userAgent = ua;
  if (userAgent.IsEmpty()) {
    PString comments;

    PINDEX pos;
    PCaselessString temp = info.name;
    if ((pos = temp.FindSpan(UserAgentTokenChars)) != P_MAX_INDEX) {
      comments += temp.Mid(pos);
      temp.Delete(pos, P_MAX_INDEX);
    }

    if (!temp.IsEmpty()) {
      userAgent = temp;

      temp = info.version;
      while ((pos = temp.FindSpan(UserAgentTokenChars)) != P_MAX_INDEX)
        temp.Delete(pos, 1);
      if (!temp.IsEmpty())
        userAgent += '/' + temp;
    }

    if (info.comments.IsEmpty() || info.comments[0] == '(')
      comments += info.comments;
    else
      comments += '(' + info.comments + ')';

    userAgent &= comments;
  }

  if (!userAgent.IsEmpty())
    SetUserAgent(userAgent);      // no compact form

  if (!info.vendor.IsEmpty())
    SetOrganization(info.vendor);      // no compact form
}


PString SIPMIMEInfo::GetWWWAuthenticate() const
{
  return GetString("WWW-Authenticate");  // no compact form
}


void SIPMIMEInfo::SetWWWAuthenticate(const PString & v)
{
  SetAt("WWW-Authenticate",  v);        // no compact form
}

PString SIPMIMEInfo::GetSIPIfMatch() const
{
  return GetString("SIP-If-Match");  // no compact form
}

void SIPMIMEInfo::SetSIPIfMatch(const PString & v)
{
  SetAt("SIP-If-Match",  v);        // no compact form
}

PString SIPMIMEInfo::GetSIPETag() const
{
  return GetString("SIP-ETag");  // no compact form
}

void SIPMIMEInfo::SetSIPETag(const PString & v)
{
  SetAt("SIP-ETag",  v);        // no compact form
}


PString SIPMIMEInfo::GetRequire() const
{
  return GetString("Require");  // no compact form
}


void SIPMIMEInfo::SetRequire(const PString & v, bool overwrite)
{
  if (overwrite || !Contains("Require"))
    SetAt("Require",  v);        // no compact form
  else
    SetAt("Require",  GetString("Require") + ", " + v);
}


void SIPMIMEInfo::GetAlertInfo(PString & info, int & appearance)
{
  info.MakeEmpty();
  appearance = -1;

  PString str = GetString("Alert-Info");
  if (str.IsEmpty())
    return;

  PINDEX pos = str.Find('<');
  PINDEX end = str.Find('>', pos);
  if (pos == P_MAX_INDEX || end == P_MAX_INDEX) {
    info = str;
    return;
  }

  info = str(pos+1, end-1);

  static const char appearance1[] = ";appearance=";
  pos = str.Find(appearance1, end);
  if (pos != P_MAX_INDEX) {
    appearance = str.Mid(pos+sizeof(appearance1)).AsUnsigned();
    return;
  }

  static const char appearance2[] = ";x-line-id";
  pos = str.Find(appearance2, end);
  if (pos != P_MAX_INDEX)
    appearance = str.Mid(pos+sizeof(appearance2)).AsUnsigned();
}


void SIPMIMEInfo::SetAlertInfo(const PString & info, int appearance)
{
  if (appearance < 0) {
    if (info.IsEmpty())
      RemoveAt("Alert-Info");
    else
      SetAt("Alert-Info", info);
    return;
  }

  PStringStream str;
  if (info[0] == '<')
    str << info;
  else
    str << '<' << info << '>';
  str << ";appearance=" << appearance;

  SetAt("Alert-Info", str);
}


static bool LocateFieldParameter(const PString & fieldValue, const PString & paramName, PINDEX & start, PINDEX & val, PINDEX & end)
{
  PINDEX semicolon = (PINDEX)-1;
  while ((semicolon = fieldValue.Find(';', semicolon+1)) != P_MAX_INDEX) {
    start = semicolon+1;
    val = fieldValue.FindSpan("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-.!%*_+`'~", semicolon+1);
    if (val == P_MAX_INDEX) {
      end = val;
      if (fieldValue.Mid(start) *= paramName) {
        return true;
      }
      break;
    }
    else if (fieldValue[val] != '=') {
      if (fieldValue(start, val-1) *= paramName) {
        end = val;
        return true;
      }
    }
    else if ((fieldValue(start, val-1) *= paramName)) {
      val++;
      end = fieldValue.FindOneOf("()<>@,;:\\\"/[]?{}= \t", val)-1;
      return true;
    }
  }

  return false;
}


PString SIPMIMEInfo::ExtractFieldParameter(const PString & fieldValue,
                                           const PString & paramName,
                                           const PString & defaultValue)
{
  PINDEX start, val, end;
  if (!LocateFieldParameter(fieldValue, paramName, start, val, end))
    return PString::Empty();
  return (val == end) ? defaultValue : fieldValue(val, end);
}


PString SIPMIMEInfo::InsertFieldParameter(const PString & fieldValue,
                                          const PString & paramName,
                                          const PString & newValue)
{
  PStringStream newField;
  newField << paramName;
  if (!newValue.IsEmpty())
    newField << '=' << newValue;

  PINDEX start, val, end;
  PString str;
  if (!LocateFieldParameter(fieldValue, paramName, start, val, end)) 
    str = fieldValue + ";" + newField;
  else
    str = fieldValue.Left(start) + newField + fieldValue.Mid(end+1);
  return str;
}


////////////////////////////////////////////////////////////////////////////////////

SIPAuthentication::SIPAuthentication()
{
  isProxy = PFalse;
}


PObject::Comparison SIPAuthentication::Compare(const PObject & other) const
{
  const SIPAuthentication * otherAuth = dynamic_cast<const SIPAuthentication *>(&other);
  if (otherAuth == NULL)
    return LessThan;

  Comparison result = GetUsername().Compare(otherAuth->GetUsername());
  if (result != EqualTo)
    return result;

  return GetPassword().Compare(otherAuth->GetPassword());
}


PString SIPAuthentication::GetAuthParam(const PString & auth, const char * name) const
{
  PString value;

  PINDEX pos = auth.Find(name);
  if (pos != P_MAX_INDEX)  {
    pos += strlen(name);
    while (isspace(auth[pos]) || (auth[pos] == ','))
      pos++;
    if (auth[pos] == '=') {
      pos++;
      while (isspace(auth[pos]))
        pos++;
      if (auth[pos] == '"') {
        pos++;
        value = auth(pos, auth.Find('"', pos)-1);
      }
      else {
        PINDEX base = pos;
        while (auth[pos] != '\0' && !isspace(auth[pos]) && (auth[pos] != ','))
          pos++;
        value = auth(base, pos-1);
      }
    }
  }

  return value;
}

PString SIPAuthentication::AsHex(PMessageDigest5::Code & digest) const
{
  PStringStream out;
  out << hex << setfill('0');
  for (PINDEX i = 0; i < 16; i++)
    out << setw(2) << (unsigned)((BYTE *)&digest)[i];
  return out;
}

PString SIPAuthentication::AsHex(const PBYTEArray & data) const
{
  PStringStream out;
  out << hex << setfill('0');
  for (PINDEX i = 0; i < data.GetSize(); i++)
    out << setw(2) << (unsigned)data[i];
  return out;
}

SIPAuthentication * SIPAuthentication::ParseAuthenticationRequired(bool isProxy,
                                                        const PString & line,
                                                              PString & errorMsg)
{
  // determine the authentication scheme
  PINDEX pos = line.Find(' ');
  PString scheme = line.Left(pos).Trim().ToLower();
  SIPAuthentication * newAuth = SIPAuthenticationFactory::CreateInstance(scheme);
  if (newAuth == NULL) {
    errorMsg = "Unknown authentication scheme " + scheme;
    return NULL;
  }

  // parse the new authentication scheme
  if (!newAuth->Parse(line, isProxy)) {
    delete newAuth;
    errorMsg = "Failed to parse authentication for scheme " + scheme;
    return NULL;
  }

  // switch authentication schemes
  return newAuth;
}

////////////////////////////////////////////////////////////////////////////////////

SIPDigestAuthentication::SIPDigestAuthentication()
{
  algorithm = NumAlgorithms;
}

SIPDigestAuthentication & SIPDigestAuthentication::operator =(const SIPDigestAuthentication & auth)
{
  isProxy   = auth.isProxy;
  authRealm = auth.authRealm;
  username  = auth.username;
  password  = auth.password;
  nonce     = auth.nonce;
  opaque    = auth.opaque;
          
  qopAuth    = auth.qopAuth;
  qopAuthInt = auth.qopAuthInt;
  cnonce     = auth.cnonce;
  nonceCount.SetValue(auth.nonceCount);

  return *this;
}

PObject::Comparison SIPDigestAuthentication::Compare(const PObject & other) const
{
  const SIPDigestAuthentication * otherAuth = dynamic_cast<const SIPDigestAuthentication *>(&other);
  if (otherAuth == NULL)
    return LessThan;

  Comparison result = GetAuthRealm().Compare(otherAuth->GetAuthRealm());
  if (result != EqualTo)
    return result;

  if (GetAlgorithm() != otherAuth->GetAlgorithm())
    return GetAlgorithm() < otherAuth->GetAlgorithm() ? LessThan : GreaterThan;

  return SIPAuthentication::Compare(other);
}

PBoolean SIPDigestAuthentication::Parse(const PString & _auth, PBoolean proxy)
{
  PCaselessString auth =_auth;

  authRealm.MakeEmpty();
  nonce.MakeEmpty();
  opaque.MakeEmpty();
  algorithm = NumAlgorithms;

  qopAuth = qopAuthInt = PFalse;
  cnonce.MakeEmpty();
  nonceCount.SetValue(1);

  if (auth.Find("digest") == P_MAX_INDEX) {
    PTRACE(1, "SIP\tDigest auth does not contian digest keyword");
    return false;
  }

  algorithm = Algorithm_MD5;  // default
  PCaselessString str = GetAuthParam(auth, "algorithm");
  if (!str.IsEmpty()) {
    while (str != AlgorithmNames[algorithm]) {
      algorithm = (Algorithm)(algorithm+1);
      if (algorithm >= SIPDigestAuthentication::NumAlgorithms) {
        PTRACE(1, "SIP\tUnknown digest algorithm " << str);
        return PFalse;
      }
    }
  }

  authRealm = GetAuthParam(auth, "realm");
  if (authRealm.IsEmpty()) {
    PTRACE(1, "SIP\tNo realm in authentication");
    return PFalse;
  }

  nonce = GetAuthParam(auth, "nonce");
  if (nonce.IsEmpty()) {
    PTRACE(1, "SIP\tNo nonce in authentication");
    return PFalse;
  }

  opaque = GetAuthParam(auth, "opaque");
  if (!opaque.IsEmpty()) {
    PTRACE(2, "SIP\tAuthentication contains opaque data");
  }

  PString qopStr = GetAuthParam(auth, "qop");
  if (!qopStr.IsEmpty()) {
    PTRACE(3, "SIP\tAuthentication contains qop-options " << qopStr);
    PStringList options = qopStr.Tokenise(',', PTrue);
    qopAuth    = options.GetStringsIndex("auth") != P_MAX_INDEX;
    qopAuthInt = options.GetStringsIndex("auth-int") != P_MAX_INDEX;
    cnonce = OpalGloballyUniqueID().AsString();
  }

  isProxy = proxy;
  return PTrue;
}


PBoolean SIPDigestAuthentication::Authorise(SIP_PDU & pdu) const
{
  PTRACE(3, "SIP\tAdding authentication information");

  PMessageDigest5 digestor;
  PMessageDigest5::Code a1, a2, entityBodyCode, response;

  PString uriText = pdu.GetURI().AsString();
  PINDEX pos = uriText.Find(";");
  if (pos != P_MAX_INDEX)
    uriText = uriText.Left(pos);

  digestor.Start();
  digestor.Process(username);
  digestor.Process(":");
  digestor.Process(authRealm);
  digestor.Process(":");
  digestor.Process(password);
  digestor.Complete(a1);

  if (qopAuthInt) {
    digestor.Start();
    digestor.Process(pdu.GetEntityBody());
    digestor.Complete(entityBodyCode);
  }

  digestor.Start();
  digestor.Process(MethodNames[pdu.GetMethod()]);
  digestor.Process(":");
  digestor.Process(uriText);
  if (qopAuthInt) {
    digestor.Process(":");
    digestor.Process(AsHex(entityBodyCode));
  }
  digestor.Complete(a2);

  PStringStream auth;
  auth << "Digest "
          "username=\"" << username << "\", "
          "realm=\"" << authRealm << "\", "
          "nonce=\"" << nonce << "\", "
          "uri=\"" << uriText << "\", "
          "algorithm=" << AlgorithmNames[algorithm];

  digestor.Start();
  digestor.Process(AsHex(a1));
  digestor.Process(":");
  digestor.Process(nonce);
  digestor.Process(":");

  if (qopAuthInt || qopAuth) {
    PString nc(psprintf("%08x", (unsigned int)nonceCount));
    ++nonceCount;
    PString qop;
    if (qopAuthInt)
      qop = "auth-int";
    else
      qop = "auth";
    digestor.Process(nc);
    digestor.Process(":");
    digestor.Process(cnonce);
    digestor.Process(":");
    digestor.Process(qop);
    digestor.Process(":");
    digestor.Process(AsHex(a2));
    digestor.Complete(response);
    auth << ", "
         << "response=\"" << AsHex(response) << "\", "
         << "cnonce=\"" << cnonce << "\", "
         << "nc=" << nc << ", "
         << "qop=" << qop;
  }
  else {
    digestor.Process(AsHex(a2));
    digestor.Complete(response);
    auth << ", response=\"" << AsHex(response) << "\"";
  }

  if (!opaque.IsEmpty())
    auth << ", opaque=\"" << opaque << "\"";

  pdu.GetMIME().SetAt(isProxy ? "Proxy-Authorization" : "Authorization", auth);
  return PTrue;
}

////////////////////////////////////////////////////////////////////////////////////

class SIPNTLMAuthentication : public SIPAuthentication
{
  public: 
    SIPNTLMAuthentication();

    virtual Comparison Compare(
      const PObject & other
    ) const;

    PBoolean Parse(
      const PString & auth,
      PBoolean proxy
    );

    PBoolean Authorise(
      SIP_PDU & pdu
    ) const;

    struct Type1MessageHdr {
       BYTE     protocol[8];     // 'N', 'T', 'L', 'M', 'S', 'S', 'P', '\0'
       BYTE     type;            // 0x01
       BYTE     _zero1[3];
       WORD     flags;           // 0xb203
       BYTE     _zero2[2];

       PUInt16l dom_len;         // domain string length
       PUInt16l dom_len2;        // domain string length
       PUInt16l dom_off;         // domain string offset
       BYTE     _zero3[2];

       PUInt16l host_len;        // host string length
       PUInt16l host_len2;       // host string length
       PUInt16l host_off;        // host string offset (always 0x20)
       BYTE     _zero4[2];

       BYTE     hostAndDomain;   // host string and domain (ASCII)
    };

    void ConstructType1Message(PBYTEArray & message) const;

  public:
    PString domainName;
    PString hostName;
};

SIPNTLMAuthentication::SIPNTLMAuthentication()
{
  hostName   = "Hostname";
  domainName = "Domain";
}

PObject::Comparison SIPNTLMAuthentication::Compare(const PObject & other) const
{
  const SIPNTLMAuthentication * otherAuth = dynamic_cast<const SIPNTLMAuthentication *>(&other);
  if (otherAuth == NULL)
    return LessThan;

  Comparison result = hostName.Compare(otherAuth->hostName);
  if (result != EqualTo)
    return result;

  result = domainName.Compare(otherAuth->domainName);
  if (result != EqualTo)
    return result;

  return SIPAuthentication::Compare(other);
}

PBoolean SIPNTLMAuthentication::Parse(const PString & /*auth*/, PBoolean /*proxy*/)
{
  return false;
}

PBoolean SIPNTLMAuthentication::Authorise(SIP_PDU & pdu) const
{
  PBYTEArray type1;
  ConstructType1Message(type1);
  pdu.GetMIME().SetAt(isProxy ? "Proxy-Authorization" : "Authorization", AsHex(type1));

  return false;
}

void SIPNTLMAuthentication::ConstructType1Message(PBYTEArray & buffer) const
{
  BYTE * ptr = buffer.GetPointer(sizeof(Type1MessageHdr) + hostName.GetLength() + domainName.GetLength());

  Type1MessageHdr * hdr = (Type1MessageHdr *)ptr;
  memset(hdr, 0, sizeof(Type1MessageHdr));
  memcpy(hdr->protocol, "NTLMSSP", 7);
  hdr->flags = 0xb203;

  hdr->host_off = PUInt16l((WORD)(&hdr->hostAndDomain - (BYTE *)hdr));
  PAssert(hdr->host_off == 0x20, "NTLM auth cannot be constructed");
  hdr->host_len = hdr->host_len2 = PUInt16l((WORD)hostName.GetLength());
  memcpy(&hdr->hostAndDomain, (const char *)hostName, hdr->host_len);

  hdr->dom_off = PUInt16l((WORD)(hdr->host_off + hdr->host_len));
  hdr->dom_len = hdr->dom_len2  = PUInt16l((WORD)domainName.GetLength());
  memcpy(&hdr->hostAndDomain + hdr->dom_len - hdr->host_len, (const char *)domainName, hdr->host_len2);
}

////////////////////////////////////////////////////////////////////////////////////

SIP_PDU::SIP_PDU()
{
  Construct(NumMethods);
}


SIP_PDU::SIP_PDU(Methods method,
                 const SIPURL & dest,
                 const PString & to,
                 const PString & from,
                 const PString & callID,
                 unsigned cseq,
                 const OpalTransportAddress & via)
  : m_usePeerTransportAddress(false)
{
  Construct(method, dest, to, from, callID, cseq, via);
}


SIP_PDU::SIP_PDU(Methods method,
                 SIPConnection & connection,
                 const OpalTransport & transport)
  : m_usePeerTransportAddress(false)
{
  Construct(method, connection, transport);
}


SIP_PDU::SIP_PDU(const SIP_PDU & request, 
                 StatusCodes code, 
                 const char * contact,
                 const char * extra,
                 const SDPSessionDescription * sdp)
  : m_usePeerTransportAddress(false)
{
  method       = NumMethods;
  statusCode   = code;
  versionMajor = request.GetVersionMajor();
  versionMinor = request.GetVersionMinor();
  m_SDP = sdp != NULL ? new SDPSessionDescription(*sdp) : NULL;

  // add mandatory fields to response (RFC 2543, 11.2)
  const SIPMIMEInfo & requestMIME = request.GetMIME();
  mime.SetTo(requestMIME.GetTo());
  mime.SetFrom(requestMIME.GetFrom());
  mime.SetCallID(requestMIME.GetCallID());
  mime.SetCSeq(requestMIME.GetCSeq());
  mime.SetVia(requestMIME.GetVia());
  mime.SetRecordRoute(requestMIME.GetRecordRoute(false));

  /* Use extra parameter as redirection URL in case of 302 */
  if (code == SIP_PDU::Redirection_MovedTemporarily) {
    SIPURL contact(extra);
    contact.Sanitise(SIPURL::ContactURI);
    mime.SetContact(contact);
    extra = NULL;
  }
  else if (contact != NULL) {
    mime.SetContact(PString(contact));
  }
    
  // format response
  if (extra != NULL)
    info = extra;
  else
    info = GetStatusCodeDescription(code);
}


SIP_PDU::SIP_PDU(const SIP_PDU & pdu)
  : PSafeObject(pdu)
  , method(pdu.method)
  , statusCode(pdu.statusCode)
  , uri(pdu.uri)
  , versionMajor(pdu.versionMajor)
  , versionMinor(pdu.versionMinor)
  , info(pdu.info)
  , mime(pdu.mime)
  , entityBody(pdu.entityBody)
  , m_SDP(pdu.m_SDP != NULL ? new SDPSessionDescription(*pdu.m_SDP) : NULL)
  , m_usePeerTransportAddress(pdu.m_usePeerTransportAddress)
{
}


SIP_PDU & SIP_PDU::operator=(const SIP_PDU & pdu)
{
  method = pdu.method;
  statusCode = pdu.statusCode;
  uri = pdu.uri;
  m_usePeerTransportAddress = pdu.m_usePeerTransportAddress;

  versionMajor = pdu.versionMajor;
  versionMinor = pdu.versionMinor;
  info = pdu.info;
  mime = pdu.mime;
  entityBody = pdu.entityBody;

  delete m_SDP;
  m_SDP = pdu.m_SDP != NULL ? new SDPSessionDescription(*pdu.m_SDP) : NULL;

  return *this;
}


SIP_PDU::~SIP_PDU()
{
  delete m_SDP;
}


void SIP_PDU::Construct(Methods meth)
{
  method = meth;
  statusCode = IllegalStatusCode;

  versionMajor = SIP_VER_MAJOR;
  versionMinor = SIP_VER_MINOR;

  m_SDP = NULL;
}


void SIP_PDU::Construct(Methods meth,
                        const SIPURL & dest,
                        const PString & to,
                        const PString & from,
                        const PString & callID,
                        unsigned cseq,
                        const OpalTransportAddress & via)
{
  PString allMethods;
  
  Construct(meth);

  uri = dest;
  uri.Sanitise(SIPURL::RequestURI);

  mime.SetTo(to);
  mime.SetFrom(from);
  mime.SetCallID(callID);
  mime.SetCSeq(PString(cseq) & MethodNames[method]);
  mime.SetMaxForwards(70);  

  // construct Via:
  PINDEX dollar = via.Find('$');

  PStringStream str;
  str << "SIP/" << versionMajor << '.' << versionMinor << '/'
      << via.Left(dollar).ToUpper() << ' ';
  PIPSocket::Address ip;
  WORD port = 5060;
  if (via.GetIpAndPort(ip, port))
    str << ip.AsString(true) << ':' << port;
  else
    str << via.Mid(dollar+1);
  str << ";branch=z9hG4bK" << OpalGloballyUniqueID() << ";rport";

  mime.SetVia(str);
}


void SIP_PDU::Construct(Methods meth,
                        SIPConnection & connection,
                        const OpalTransport & transport,
                        unsigned cseq)
{
  SIPEndPoint & endpoint = connection.GetEndPoint();
  PString localPartyName = connection.GetLocalPartyName();
  PINDEX pos = localPartyName.Find('@');
  if (pos != P_MAX_INDEX)
    localPartyName = localPartyName.Left(pos);

  pos = localPartyName.Find(' ');
  if (pos != P_MAX_INDEX)
    localPartyName.Replace(" ", "_", PTrue);

  PString remotePartyAddress = connection.GetRemotePartyAddress();
  PINDEX prefix = remotePartyAddress.Find("sip:");
  if(prefix != P_MAX_INDEX)
    remotePartyAddress = remotePartyAddress.Mid(prefix + 4);

  SIPURL localPartyURL(localPartyName, remotePartyAddress, endpoint.GetDefaultSignalPort());
  SIPURL contact = endpoint.GetContactURL(transport, localPartyURL);
  contact.Sanitise(meth != Method_INVITE ? SIPURL::ContactURI : SIPURL::RouteURI);
  mime.SetContact(contact);

  SIPURL via = endpoint.GetLocalURL(transport, localPartyName);

  SIPDialogContext & dialog = connection.GetDialog();
  Construct(meth,
            dialog.GetRequestURI(),
            dialog.GetRemoteURI().AsQuotedString(),
            dialog.GetLocalURI().AsQuotedString(),
            dialog.GetCallID(),
            cseq != 0 ? cseq : dialog.GetNextCSeq(),
            via.GetHostAddress());

  SetRoute(dialog.GetRouteSet()); // Possibly adjust the URI and the route

  m_usePeerTransportAddress = dialog.UsePeerTransportAddress();
}


bool SIP_PDU::SetRoute(const PStringList & set)
{
  PStringList routeSet = set;

  if (routeSet.IsEmpty())
    return false;

  SIPURL firstRoute = routeSet.front();
  if (!firstRoute.GetParamVars().Contains("lr")) {
    // this procedure is specified in RFC3261:12.2.1.1 for backwards compatibility with RFC2543
    routeSet.MakeUnique();
    routeSet.RemoveAt(0);
    routeSet.AppendString(uri.AsString());
    uri = firstRoute;
    uri.Sanitise(SIPURL::RouteURI);
  }

  mime.SetRoute(routeSet);
  return true;
}


bool SIP_PDU::SetRoute(const SIPURL & proxy)
{
  if (proxy.IsEmpty())
    return false;

  PStringStream str;
  str << "<sip:" << proxy.GetHostName() << ':'  << proxy.GetPort() << ";lr>";
  mime.SetRoute(str);
  return true;
}


void SIP_PDU::SetAllow(unsigned bitmask)
{
  PStringStream str;
  
  for (Methods method = Method_INVITE ; method < SIP_PDU::NumMethods ; method = (Methods)(method+1)) {
    if ((bitmask&(1<<method))) {
      if (!str.IsEmpty())
        str << ',';
      str << method;
    }
  }
  
  mime.SetAllow(str);
}


void SIP_PDU::AdjustVia(OpalTransport & transport)
{
  // Update the VIA field following RFC3261, 18.2.1 and RFC3581
  PStringList viaList = mime.GetViaList();
  if (viaList.GetSize() == 0)
    return;

  PString viaFront = viaList.front();
  PString via = viaFront;
  PString port, ip;
  PINDEX j = 0;
  
  if ((j = via.FindLast (' ')) != P_MAX_INDEX)
    via = via.Mid(j+1);
  if ((j = via.Find (';')) != P_MAX_INDEX)
    via = via.Left(j);
  if ((j = via.Find (':')) != P_MAX_INDEX) {
    ip = via.Left(j);
    port = via.Mid(j+1);
  }
  else
    ip = via;

  PIPSocket::Address a (ip);
  PIPSocket::Address remoteIp;
  WORD remotePort;
  if (transport.GetLastReceivedAddress().GetIpAndPort(remoteIp, remotePort)) {
    PString rport = SIPMIMEInfo::ExtractFieldParameter(viaFront, "rport");
    if (rport.IsEmpty()) {
      // fill in empty rport and received for RFC 3581
      viaFront = SIPMIMEInfo::InsertFieldParameter(viaFront, "rport", remotePort);
      viaFront = SIPMIMEInfo::InsertFieldParameter(viaFront, "received", remoteIp);
    }
    else if (remoteIp != a ) // set received when remote transport address different from Via address 
    {
      viaFront = SIPMIMEInfo::InsertFieldParameter(viaFront, "received", remoteIp);
    }
  }
  else if (!a.IsValid()) {
    // Via address given has domain name
    viaFront = SIPMIMEInfo::InsertFieldParameter(viaFront, "received", via);
  }

  viaList.front() = viaFront;
  mime.SetViaList(viaList);
}


bool SIP_PDU::SendResponse(OpalTransport & transport, StatusCodes code, SIPEndPoint * endpoint, const char * contact, const char * extra)
{
  SIP_PDU response(*this, code, contact, extra);
  return SendResponse(transport, response, endpoint);
}


bool SIP_PDU::SendResponse(OpalTransport & transport, SIP_PDU & response, SIPEndPoint * endpoint)
{
  OpalTransportAddress newAddress;

  WORD defaultPort = transport.GetEndPoint().GetDefaultSignalPort();

  PStringList viaList = mime.GetViaList();
  if (viaList.GetSize() > 0) {
    PString viaAddress = viaList.front();
    PString proto = viaList.front();
    PString viaPort = defaultPort;

    PINDEX j = 0;
    // get the address specified in the Via
    if ((j = viaAddress.FindLast (' ')) != P_MAX_INDEX)
      viaAddress = viaAddress.Mid(j+1);
    if ((j = viaAddress.Find (';')) != P_MAX_INDEX)
      viaAddress = viaAddress.Left(j);
    if ((j = viaAddress.Find (':')) != P_MAX_INDEX) {
      viaPort = viaAddress.Mid(j+1);
      viaAddress = viaAddress.Left(j);
    }

    // get the protocol type from Via header
    if ((j = proto.FindLast (' ')) != P_MAX_INDEX)
      proto = proto.Left(j);
    if ((j = proto.FindLast('/')) != P_MAX_INDEX)
      proto = proto.Mid(j+1);

    // maddr is present, no support for multicast yet
    PString param = SIPMIMEInfo::ExtractFieldParameter(viaList.front(), "maddr");
    if (!param.IsEmpty()) 
      viaAddress = param;

    // received is present
    bool received = false;
    param = SIPMIMEInfo::ExtractFieldParameter(viaList.front(), "received");
    if (!param.IsEmpty()) {
      viaAddress = param;
      received = true;
    }

    // rport is present. be careful to distinguish between not present and empty
    PIPSocket::Address remoteIp;
    WORD remotePort;
    transport.GetLastReceivedAddress().GetIpAndPort(remoteIp, remotePort);
    {
      PINDEX start, val, end;
      if (LocateFieldParameter(viaList.front(), "rport", start, val, end)) {
        param = viaList.front()(val, end);
        if (!param.IsEmpty()) 
          viaPort = param;
        else
          viaPort = remotePort;
        if (!received) 
          viaAddress = remoteIp.AsString();
      }
    }

    newAddress = OpalTransportAddress(viaAddress+":"+viaPort, defaultPort, (proto *= "TCP") ? "tcp$" : "udp$");
  }
  else {
    // get Via from From field
    PString from = mime.GetFrom();
    if (!from.IsEmpty()) {
      PINDEX j = from.Find (';');
      if (j != P_MAX_INDEX)
        from = from.Left(j); // Remove all parameters
      j = from.Find ('<');
      if (j != P_MAX_INDEX && from.Find ('>') == P_MAX_INDEX)
        from += '>';

      SIPURL url(from);

      newAddress = OpalTransportAddress(url.GetHostName()+ ":" + PString(PString::Unsigned, url.GetPort()), defaultPort, "udp$");
    }
  }

  if (endpoint != NULL && response.GetMIME().GetContact().IsEmpty()) {
    SIPURL to = GetMIME().GetTo();
    PString username = to.GetUserName();
    SIPURL contact = endpoint->GetLocalURL(transport, username);
    contact.Sanitise(SIPURL::ContactURI);
    response.GetMIME().SetContact(contact);
  }

  return response.Write(transport, newAddress);
}


void SIP_PDU::PrintOn(ostream & strm) const
{
  strm << mime.GetCSeq() << ' ';
  if (method != NumMethods)
    strm << uri;
  else if (statusCode != IllegalStatusCode)
    strm << '<' << (unsigned)statusCode << '>';
  else
    strm << "<<Uninitialised>>";
}


PBoolean SIP_PDU::Read(OpalTransport & transport)
{
  if (!transport.IsOpen()) {
    PTRACE(1, "SIP\tAttempt to read PDU from closed transport " << transport);
    return PFalse;
  }

  PStringStream datagram;
  PBYTEArray pdu;

  istream * stream;
  if (transport.IsReliable())
    stream = &transport;
  else {
    stream = &datagram;

    if (!transport.ReadPDU(pdu))
      return false;

    datagram = PString((char *)pdu.GetPointer(), pdu.GetSize());
  }

  // get the message from transport/datagram into cmd and parse MIME
  PString cmd;
  *stream >> cmd;

  if (!stream->good() || cmd.IsEmpty()) {
    if (stream == &datagram) {
      transport.setstate(ios::failbit);
      PTRACE(1, "SIP\tInvalid datagram from " << transport.GetLastReceivedAddress()
                << " - " << pdu.GetSize() << " bytes.\n" << hex << setprecision(2) << pdu << dec);
    }
    return PFalse;
  }

  if (cmd.Left(4) *= "SIP/") {
    // parse Response version, code & reason (ie: "SIP/2.0 200 OK")
    PINDEX space = cmd.Find(' ');
    if (space == P_MAX_INDEX) {
      PTRACE(2, "SIP\tBad Status-Line \"" << cmd << "\" received on " << transport);
      return PFalse;
    }

    versionMajor = cmd.Mid(4).AsUnsigned();
    versionMinor = cmd(cmd.Find('.')+1, space).AsUnsigned();
    statusCode = (StatusCodes)cmd.Mid(++space).AsUnsigned();
    info    = cmd.Mid(cmd.Find(' ', space));
    uri     = PString();
  }
  else {
    // parse the method, URI and version
    PStringArray cmds = cmd.Tokenise( ' ', PFalse);
    if (cmds.GetSize() < 3) {
      PTRACE(2, "SIP\tBad Request-Line \"" << cmd << "\" received on " << transport);
      return PFalse;
    }

    int i = 0;
    while (!(cmds[0] *= MethodNames[i])) {
      i++;
      if (i >= NumMethods) {
        PTRACE(2, "SIP\tUnknown method name " << cmds[0] << " received on " << transport);
        return PFalse;
      }
    }
    method = (Methods)i;

    uri = cmds[1];
    versionMajor = cmds[2].Mid(4).AsUnsigned();
    versionMinor = cmds[2].Mid(cmds[2].Find('.')+1).AsUnsigned();
    info = PString();
  }

  if (versionMajor < 2) {
    PTRACE(2, "SIP\tInvalid version (" << versionMajor << ") received on " << transport);
    return PFalse;
  }

  // Getthe MIME fields
  *stream >> mime;
  if (!stream->good() || mime.IsEmpty()) {
    PTRACE(2, "SIP\tInvalid MIME received on " << transport);
    transport.clear(); // Clear flags so BadRequest response is sent by caller
    return PFalse;
  }


  // get the SDP content body
  // if a content length is specified, read that length
  // if no content length is specified (which is not the same as zero length)
  // then read until end of datagram or stream
  PINDEX contentLength = mime.GetContentLength();
  bool contentLengthPresent = mime.IsContentLengthPresent();

  if (!contentLengthPresent) {
    PTRACE(2, "SIP\tNo Content-Length present from " << transport << ", reading till end of datagram/stream.");
  }
  else if (contentLength < 0) {
    PTRACE(2, "SIP\tImpossible negative Content-Length from " << transport << ", reading till end of datagram/stream.");
    contentLengthPresent = false;
  }
  else if (contentLength > (transport.IsReliable() ? 1000000 : datagram.GetLength())) {
    PTRACE(2, "SIP\tImplausibly long Content-Length " << contentLength << " received from " << transport << ", reading to end of datagram/stream.");
    contentLengthPresent = false;
  }

  if (contentLengthPresent) {
    if (contentLength > 0)
      stream->read(entityBody.GetPointer(contentLength+1), contentLength);
  }
  else {
    contentLength = 0;
    int c;
    while ((c = stream->get()) != EOF) {
      entityBody.SetMinSize((++contentLength/1000+1)*1000);
      entityBody += (char)c;
    }
  }

  ////////////////
  entityBody[contentLength] = '\0';

#if PTRACING
  if (PTrace::CanTrace(3)) {
    ostream & trace = PTrace::Begin(3, __FILE__, __LINE__);

    trace << "SIP\tPDU ";

    if (!PTrace::CanTrace(4)) {
      if (method != NumMethods)
        trace << MethodNames[method] << ' ' << uri;
      else
        trace << (unsigned)statusCode << ' ' << info;
      trace << ' ';
    }

    trace << "received: rem=" << transport.GetLastReceivedAddress()
          << ",local=" << transport.GetLocalAddress()
          << ",if=" << transport.GetLastReceivedInterface();

    if (PTrace::CanTrace(4))
      trace << '\n' << cmd << '\n' << mime << entityBody;

    trace << PTrace::End;
  }
#endif

  return PTrue;
}


PBoolean SIP_PDU::Write(OpalTransport & transport, const OpalTransportAddress & remoteAddress, const PString & localInterface)
{
  PWaitAndSignal mutex(transport.GetWriteMutex());

  if (!transport.IsOpen()) {
    PTRACE(1, "SIP\tAttempt to write PDU to closed transport " << transport);
    return PFalse;
  }

  OpalTransportAddress oldRemoteAddress = transport.GetRemoteAddress();
  if (!remoteAddress.IsEmpty() && !oldRemoteAddress.IsEquivalent(remoteAddress)) {
    if (!transport.SetRemoteAddress(remoteAddress)) {
      PTRACE(1, "SIP\tCannot use remote address " << remoteAddress << " for transport " << transport);
      return false;
    }
    PTRACE(4, "SIP\tSet new remote address " << remoteAddress << " for transport " << transport);
  }

  PString oldInterface = transport.GetInterface();
  if (!localInterface.IsEmpty() && oldInterface != localInterface) {
    if (!transport.SetInterface(localInterface)) {
      PTRACE(1, "SIP\tCannot use local interface \"" << localInterface << "\" for transport " << transport);
      return false;
    }
    PTRACE(4, "SIP\tSet new interface " << localInterface << " for transport " << transport);
  }

  mime.SetCompactForm(false);
  PString strPDU = Build();
  if (!transport.IsReliable() && strPDU.GetLength() > 1450) {
    PTRACE(4, "SIP\tPDU is too large (" << strPDU.GetLength() << " bytes) trying compact form.");
    mime.SetCompactForm(true);
    strPDU = Build();
    PTRACE_IF(2, strPDU.GetLength() > 1450,
              "SIP\tPDU is likely too large (" << strPDU.GetLength() << " bytes) for UDP datagram.");
  }

#if PTRACING
  if (PTrace::CanTrace(3)) {
    ostream & trace = PTrace::Begin(3, __FILE__, __LINE__);

    trace << "SIP\tSending PDU ";

    if (!PTrace::CanTrace(4)) {
      if (method != NumMethods)
        trace << MethodNames[method] << ' ' << uri;
      else
        trace << (unsigned)statusCode << ' ' << info;
      trace << ' ';
    }

    trace << '(' << strPDU.GetLength() << " bytes) to: "
             "rem=" << transport.GetRemoteAddress() << ","
             "local=" << transport.GetLocalAddress() << ","
             "if=" << transport.GetInterface();

    if (PTrace::CanTrace(4))
      trace << '\n' << strPDU;

    trace << PTrace::End;
  }
#endif

  bool ok = transport.WriteString(strPDU);
  PTRACE_IF(1, !ok, "SIP\tPDU Write failed: " << transport.GetErrorText(PChannel::LastWriteError));

  transport.SetInterface(oldInterface);
  transport.SetRemoteAddress(oldRemoteAddress);

  return ok;
}


PString SIP_PDU::Build()
{
  PStringStream str;

  if (m_SDP != NULL) {
    entityBody = m_SDP->Encode();
    mime.SetContentType("application/sdp");
  }

  mime.SetContentLength(entityBody.GetLength());

  if (method != NumMethods)
    str << MethodNames[method] << ' ' << uri << ' ';

  str << "SIP/" << versionMajor << '.' << versionMinor;

  if (method == NumMethods)
    str << ' ' << (unsigned)statusCode << ' ' << info;

  str << "\r\n" << mime << entityBody;
  return str;
}


PString SIP_PDU::GetTransactionID() const
{
  if (transactionID.IsEmpty()) {
    /* RFC3261 Sections 8.1.1.7 & 17.1.3 transactions are identified by the
       branch paranmeter in the top most Via and CSeq. We do NOT include the
       CSeq in our id as we want the CANCEL messages directed at out
       transaction structure.
     */
    PStringList vias = mime.GetViaList();
    if (!vias.IsEmpty())
      transactionID = SIPMIMEInfo::ExtractFieldParameter(vias.front(), "branch");
    if (transactionID.IsEmpty()) {
      PTRACE(2, "SIP\tTransaction " << mime.GetCSeq() << " has no branch parameter!");
      transactionID = mime.GetCallID() + mime.GetCSeq(); // Fail safe ...
    }
  }

  return transactionID;
}


SDPSessionDescription * SIP_PDU::GetSDP()
{
  if (m_SDP == NULL && mime.GetContentType() == "application/sdp") {
    m_SDP = new SDPSessionDescription(0, 0, OpalTransportAddress());
    if (!m_SDP->Decode(entityBody)) {
      delete m_SDP;
      m_SDP = NULL;
    }
  }

  return m_SDP;
}


void SIP_PDU::SetSDP(SDPSessionDescription * sdp)
{
  delete m_SDP;
  m_SDP = sdp;
}


////////////////////////////////////////////////////////////////////////////////////

SIPDialogContext::SIPDialogContext()
  : m_callId(SIPTransaction::GenerateCallID())
  , m_lastSentCSeq(0)
  , m_lastReceivedCSeq(0)
  , m_usePeerTransportAddress(false)
{
}


PString SIPDialogContext::AsString() const
{
  PStringStream str;
  SIPURL url = m_requestURI;
  url.SetParamVar("call-id",   m_callId);
  url.SetParamVar("local-uri", m_localURI.AsQuotedString());
  url.SetParamVar("remote-uri",m_remoteURI.AsQuotedString());
  url.SetParamVar("tx-cseq",   m_lastSentCSeq);
  url.SetParamVar("rx-cseq",   m_lastReceivedCSeq);

  unsigned index = 0;
  for (PStringList::const_iterator it = m_routeSet.begin(); it != m_routeSet.end(); ++it)
    url.SetParamVar(psprintf("route-set-%u", ++index), *it);

  return url.AsString();
}


bool SIPDialogContext::FromString(const PString & str)
{
  SIPURL url;
  if (!url.Parse(str))
    return false;

  m_requestURI = url;
  m_requestURI.SetParamVars(PStringToString());

  const PStringToString & params = url.GetParamVars();
  SetCallID(params("call-id"));
  SetLocalURI(params("local-uri"));
  SetRemoteURI(params("remote-uri"));
  m_lastSentCSeq = params("tx-cseq").AsUnsigned();
  m_lastReceivedCSeq = params("rx-cseq").AsUnsigned();

  PString route;
  unsigned index = 0;
  while (!(route = params(psprintf("route-set-%u", ++index))).IsEmpty())
    m_routeSet.AppendString(route);

  return IsEstablished();
}


static void SetWithTag(const SIPURL & url, SIPURL & uri, PString & tag, bool generate)
{
  uri = url;

  PString newTag = url.GetParamVars()("tag");
  if (newTag.IsEmpty())
    newTag = SIPMIMEInfo::ExtractFieldParameter(uri.GetFieldParameters(), "tag");
  else
    uri.SetParamVar("tag", PString::Empty());

  if (!newTag.IsEmpty() && tag != newTag) {
    PTRACE(4, "SIP\tUpdating dialog tag from \"" << tag << "\" to \"" << newTag << '"');
    tag = newTag;
  }

  if (tag.IsEmpty() && generate)
    tag = SIPURL::GenerateTag();

  if (!tag.IsEmpty())
    uri.SetFieldParameters("tag="+tag);
}


static bool SetWithTag(const PString & str, SIPURL & uri, PString & tag, bool generate)
{
  SIPURL url;
  if (!url.Parse(str))
    return false;

  SetWithTag(url, uri, tag, generate);
  return true;
}


void SIPDialogContext::SetLocalURI(const SIPURL & url)
{
  SetWithTag(url, m_localURI, m_localTag, true);
}


bool SIPDialogContext::SetLocalURI(const PString & uri)
{
  return SetWithTag(uri, m_localURI, m_localTag, true);
}


void SIPDialogContext::SetRemoteURI(const SIPURL & url)
{
  SetWithTag(url, m_remoteURI, m_remoteTag, false);
}


bool SIPDialogContext::SetRemoteURI(const PString & uri)
{
  return SetWithTag(uri, m_remoteURI, m_remoteTag, false);
}


void SIPDialogContext::UpdateRouteSet(const SIPURL & proxy)
{
  // Default routeSet if there is a proxy
  if (m_routeSet.IsEmpty() && !proxy.IsEmpty()) {
    PStringStream str;
    str << "sip:" << proxy.GetHostName() << ':'  << proxy.GetPort() << ";lr";
    m_routeSet += str;
  }
}


void SIPDialogContext::Update(const SIP_PDU & pdu)
{
  const SIPMIMEInfo & mime = pdu.GetMIME();

  SetCallID(mime.GetCallID());

  if (m_routeSet.IsEmpty()) {
    // get the route set from the Record-Route response field according to 12.1.2
    // requests in a dialog do not modify the initial route set according to 12.2
    m_routeSet = mime.GetRecordRoute(pdu.GetMethod() == SIP_PDU::NumMethods);
  }

  // Update request URI
  if (pdu.GetMethod() != SIP_PDU::NumMethods || pdu.GetStatusCode()/100 == 2) {
    PString contact = mime.GetContact();
    if (!contact.IsEmpty()) {
      m_requestURI.Parse(contact);
      PTRACE(4, "SIP\tSet Request URI to " << m_requestURI);
    }
  }

  /* Update the local/remote fields */
  if (pdu.GetMethod() == SIP_PDU::NumMethods) {
    SetRemoteURI(mime.GetTo());
    SetLocalURI(mime.GetFrom());
  }
  else {
    SetLocalURI(mime.GetTo()); // Will add a tag
    SetRemoteURI(mime.GetFrom());
  }

  /* Update target address, if required */
  if (pdu.GetMethod() == SIP_PDU::Method_INVITE || pdu.GetMethod() == SIP_PDU::Method_SUBSCRIBE) {
    PINDEX start, val, end;
    PStringList viaList = mime.GetViaList();
    if (viaList.GetSize() > 0)
      m_usePeerTransportAddress = LocateFieldParameter(viaList.front(), "rport", start, val, end);
  }
}


bool SIPDialogContext::IsDuplicateCSeq(unsigned requestCSeq)
{
  /* See if have had a mesage before and this one is older than the last,
     with a check for server bugs where the sequence number changes
     dramatically. if older then is a retransmission so return true and do
     not process it. */
  bool duplicate = m_lastReceivedCSeq != 0 && requestCSeq <= m_lastReceivedCSeq && (m_lastReceivedCSeq - requestCSeq) < 10;

  PTRACE_IF(4, m_lastReceivedCSeq == 0, "SIP\tDialog initial sequence number " << requestCSeq);
  PTRACE_IF(3, duplicate, "SIP\tReceived duplicate sequence number " << requestCSeq);
  PTRACE_IF(2, !duplicate && requestCSeq != m_lastReceivedCSeq+1,
            "SIP\tReceived unexpected sequence number " << requestCSeq << ", expecting " << m_lastReceivedCSeq+1);

  m_lastReceivedCSeq = requestCSeq;

  return duplicate;
}


////////////////////////////////////////////////////////////////////////////////////

SIPTransaction::SIPTransaction(SIPEndPoint & ep,
                               OpalTransport & trans,
                               const PTimeInterval & minRetryTime,
                               const PTimeInterval & maxRetryTime)
  : endpoint(ep)
  , transport(trans)
{
  Construct(minRetryTime, maxRetryTime);
  PTRACE(4, "SIP\tTransaction created.");
}


SIPTransaction::SIPTransaction(SIPConnection & conn,
                               OpalTransport & trans,
                               Methods meth)
  : SIP_PDU(meth, conn, trans),
    endpoint(conn.GetEndPoint()),
    transport(trans)
{
  connection = &conn;
  Construct();
  PTRACE(4, "SIP\t" << meth << " transaction id=" << GetTransactionID() << " created.");
}


void SIPTransaction::Construct(const PTimeInterval & minRetryTime, const PTimeInterval & maxRetryTime)
{
  retryTimer.SetNotifier(PCREATE_NOTIFIER(OnRetry));
  completionTimer.SetNotifier(PCREATE_NOTIFIER(OnTimeout));

  retry = 1;
  state = NotStarted;

  retryTimeoutMin = ((minRetryTime != PMaxTimeInterval) && (minRetryTime != 0)) ? minRetryTime : endpoint.GetRetryTimeoutMin(); 
  retryTimeoutMax = ((maxRetryTime != PMaxTimeInterval) && (maxRetryTime != 0)) ? maxRetryTime : endpoint.GetRetryTimeoutMax();
}


void SIPTransaction::Construct(Methods method, SIPDialogContext & dialog)
{
  SIP_PDU::Construct(method,
                     dialog.GetRequestURI(),
                     dialog.GetRemoteURI().AsQuotedString(),
                     dialog.GetLocalURI().AsQuotedString(),
                     dialog.GetCallID(),
                     dialog.GetNextCSeq(),
                     endpoint.GetLocalURL(transport).GetHostAddress());
  SetRoute(dialog.GetRouteSet());
}


SIPTransaction::~SIPTransaction()
{
  PTRACE_IF(1, state < Terminated_Success, "SIP\tDestroying transaction id="
            << GetTransactionID() << " which is not yet terminated.");
  PTRACE(4, "SIP\tTransaction id=" << GetTransactionID() << " destroyed.");
}


PBoolean SIPTransaction::Start()
{
  if (state == Completed)
    return PTrue;

  if (connection != NULL)
    connection->OnStartTransaction(*this);

  endpoint.AddTransaction(this);

  if (state != NotStarted) {
    PAssertAlways(PLogicError);
    return PFalse;
  }

  if (connection != NULL && connection->GetAuthenticator() != NULL)
    connection->GetAuthenticator()->Authorise(*this); 

  PSafeLockReadWrite lock(*this);

  state = Trying;
  retry = 0;

  if (m_localInterface.IsEmpty())
    m_localInterface = transport.GetInterface();

  /* Get the address to which the request PDU should be sent, according to
     the RFC, for a request in a dialog. 
     handle case where address changed using rport
   */
  if (m_usePeerTransportAddress)
    m_remoteAddress = transport.GetLastReceivedAddress();
  else {
    SIPURL destination;
    destination = uri;
    PStringList routeSet = GetMIME().GetRoute();
    if (!routeSet.IsEmpty()) {
      SIPURL firstRoute = routeSet.front();
      if (firstRoute.GetParamVars().Contains("lr"))
        destination = firstRoute;
    }

    // Do a DNS SRV lookup
    destination.AdjustToDNS();
    m_remoteAddress = destination.GetHostAddress();
  }

  PTRACE(3, "SIP\tTransaction remote address is " << m_remoteAddress);

  // Use the connection transport to send the request
  if (!Write(transport, m_remoteAddress, m_localInterface)) {
    SetTerminated(Terminated_TransportError);
    return PFalse;
  }

  retryTimer = retryTimeoutMin;
  if (method == Method_INVITE)
    completionTimer = endpoint.GetInviteTimeout();
  else
    completionTimer = endpoint.GetNonInviteTimeout();

  PTRACE(4, "SIP\tTransaction timers set: retry=" << retryTimer << ", completion=" << completionTimer);
  return true;
}


void SIPTransaction::WaitForCompletion()
{
  if (state >= Completed)
    return;

  if (state == NotStarted)
    Start();

  completed.Wait();
}


PBoolean SIPTransaction::Cancel()
{
  PSafeLockReadWrite lock(*this);

  if (state == NotStarted || state >= Cancelling) {
    PTRACE(3, "SIP\t" << GetMethod() << " transaction id=" << GetTransactionID() << " cannot be cancelled as in state " << state);
    return PFalse;
  }

  PTRACE(4, "SIP\t" << GetMethod() << " transaction id=" << GetTransactionID() << " cancelled.");
  state = Cancelling;
  retry = 0;
  retryTimer = retryTimeoutMin;
  completionTimer = endpoint.GetPduCleanUpTimeout();
  return ResendCANCEL();
}


void SIPTransaction::Abort()
{
  if (LockReadWrite()) {
    PTRACE(4, "SIP\t" << GetMethod() << " transaction id=" << GetTransactionID() << " aborted.");
    if (!IsCompleted())
      SetTerminated(Terminated_Aborted);
    UnlockReadWrite();
  }
}


bool SIPTransaction::SendPDU(SIP_PDU & pdu)
{
  if (pdu.Write(transport, m_remoteAddress, m_localInterface))
    return true;

  SetTerminated(Terminated_TransportError);
  return false;
}


bool SIPTransaction::ResendCANCEL()
{
  SIP_PDU cancel(Method_CANCEL,
                 uri,
                 mime.GetTo(),
                 mime.GetFrom(),
                 mime.GetCallID(),
                 mime.GetCSeqIndex(),
                 m_localInterface);
  // Use the topmost via header from the INVITE we cancel as per 9.1. 
  PStringList viaList = mime.GetViaList();
  cancel.GetMIME().SetVia(viaList.front());

  return SendPDU(cancel);
}


PBoolean SIPTransaction::OnReceivedResponse(SIP_PDU & response)
{
  // Stop the timers with asynchronous flag to avoid deadlock
  retryTimer.Stop(false);

  PString cseq = response.GetMIME().GetCSeq();

  /* If is the response to a CANCEL we sent, then we stop retransmissions
     and wait for the 487 Request Terminated to come in */
  if (cseq.Find(MethodNames[Method_CANCEL]) != P_MAX_INDEX) {
    completionTimer = endpoint.GetPduCleanUpTimeout();
    return PFalse;
  }

  // Something wrong here, response is not for the request we made!
  if (cseq.Find(MethodNames[method]) == P_MAX_INDEX) {
    PTRACE(2, "SIP\tTransaction " << cseq << " response not for " << *this);
    // Restart timer as haven't finished yet
    retryTimer = retryTimer.GetResetTime();
    completionTimer = completionTimer.GetResetTime();
    return PFalse;
  }

  PSafeLockReadWrite lock(*this);
  if (!lock.IsLocked())
    return PFalse;

  /* Really need to check if response is actually meant for us. Have a
     temporary cheat in assuming that we are only sending a given CSeq to one
     and one only host, so anything coming back with that CSeq is OK. This has
     "issues" according to the spec but
     */
  if (IsInProgress()) {
    if (response.GetStatusCode()/100 == 1) {
      PTRACE(3, "SIP\t" << GetMethod() << " transaction id=" << GetTransactionID() << " proceeding.");

      if (state == Trying)
        state = Proceeding;

      retry = 0;
      retryTimer = retryTimeoutMax;

      int expiry = mime.GetExpires();
      if (expiry > 0)
        completionTimer.SetInterval(0, expiry);
      else if (method == Method_INVITE)
        completionTimer = endpoint.GetInviteTimeout();
      else
        completionTimer = endpoint.GetNonInviteTimeout();
    }
    else {
      PTRACE(3, "SIP\t" << GetMethod() << " transaction id=" << GetTransactionID() << " completed.");
      state = Completed;
      statusCode = response.GetStatusCode();
    }

    if (connection != NULL)
      connection->OnReceivedResponse(*this, response);
    else
      endpoint.OnReceivedResponse(*this, response);

    if (state == Completed)
      OnCompleted(response);
  }

  if (response.GetStatusCode() >= 200) {
    completionTimer = endpoint.GetPduCleanUpTimeout();
    completed.Signal();
  }

  return PTrue;
}


PBoolean SIPTransaction::OnCompleted(SIP_PDU & /*response*/)
{
  return PTrue;
}


void SIPTransaction::OnRetry(PTimer &, INT)
{
  PSafeLockReadWrite lock(*this);

  if (!lock.IsLocked() || state > Cancelling || (state == Proceeding && method == Method_INVITE))
    return;

  retry++;

  if (retry >= endpoint.GetMaxRetries()) {
    SetTerminated(Terminated_RetriesExceeded);
    return;
  }

  if (state > Trying)
    retryTimer = retryTimeoutMax;
  else {
    PTimeInterval timeout = retryTimeoutMin*(1<<retry);
    if (timeout > retryTimeoutMax)
      timeout = retryTimeoutMax;
    retryTimer = timeout;
  }

  PTRACE(3, "SIP\t" << GetMethod() << " transaction id=" << GetTransactionID()
         << " timeout, making retry " << retry << ", timeout " << retryTimer);

  if (state == Cancelling)
    ResendCANCEL();
  else
    SendPDU(*this);
}


void SIPTransaction::OnTimeout(PTimer &, INT)
{
  PSafeLockReadWrite lock(*this);

  if (lock.IsLocked()) {
    switch (state) {
      case Trying :
        // Sent initial command and got nothin'
        SetTerminated(Terminated_Timeout);
        break;

      case Proceeding :
        /* Got a 100 response and then nothing, give up with a CANCEL
           just in case the other side is still there, and in particular
           in the case of an INVITE where nobody answers */
        Cancel();
        break;

      case Cancelling :
        // We cancelled and finished waiting for retries
        SetTerminated(Terminated_Cancelled);
        break;

      case Completed :
        // We completed and finished waiting for retries
        SetTerminated(Terminated_Success);
        break;

      default :
        // Already terminated in some way
        break;
    }
  }
}


void SIPTransaction::SetTerminated(States newState)
{
#if PTRACING
  static const char * const StateNames[NumStates] = {
    "NotStarted",
    "Trying",
    "Proceeding",
    "Cancelling",
    "Completed",
    "Terminated_Success",
    "Terminated_Timeout",
    "Terminated_RetriesExceeded",
    "Terminated_TransportError",
    "Terminated_Cancelled",
    "Terminated_Aborted"
  };
#endif

  // Terminated, so finished with timers
  retryTimer.Stop(false);
  completionTimer.Stop(false);

  if (state >= Terminated_Success) {
    PTRACE_IF(3, newState != Terminated_Success, "SIP\tTried to set state " << StateNames[newState] 
              << " for " << GetMethod() << " transaction id=" << GetTransactionID()
              << " but already terminated ( " << StateNames[state] << ')');
    return;
  }
  
  States oldState = state;
  
  state = newState;
  PTRACE(3, "SIP\tSet state " << StateNames[newState] << " for "
         << GetMethod() << " transaction id=" << GetTransactionID());

  // Transaction failed, tell the endpoint
  if (state > Terminated_Success) {
    switch (state) {
      case Terminated_Timeout :
      case Terminated_RetriesExceeded:
        statusCode = SIP_PDU::Local_Timeout;
        break;

      case Terminated_TransportError :
        statusCode = SIP_PDU::Local_TransportError;
        break;

      case Terminated_Aborted :
      case Terminated_Cancelled :
        statusCode = SIP_PDU::Failure_RequestTerminated;
        break;

      default :// Prevent GNU warning
        break;
    }

    endpoint.OnTransactionFailed(*this);
    if (connection != NULL)
      connection->OnTransactionFailed(*this);
  }

  if (oldState != Completed)
    completed.Signal();
}


PString SIPTransaction::GenerateCallID()
{
  return PGloballyUniqueID().AsString() + '@' + PIPSocket::GetHostName();
}


////////////////////////////////////////////////////////////////////////////////////

SIPInvite::SIPInvite(SIPConnection & connection, OpalTransport & transport, const OpalRTPSessionManager & sm)
  : SIPTransaction(connection, transport, Method_INVITE)
  , rtpSessions(sm)
{
  mime.SetDate(); // now
  SetAllow(connection.GetEndPoint().GetAllowedMethods());
  mime.SetProductInfo(connection.GetEndPoint().GetUserAgent(), connection.GetProductInfo());

  connection.OnCreatingINVITE(*this);
  
  if (m_SDP != NULL)
    m_SDP->SetSessionName(mime.GetUserAgent());
}


PBoolean SIPInvite::OnReceivedResponse(SIP_PDU & response)
{
  if (response.GetMIME().GetCSeq().Find(MethodNames[Method_INVITE]) != P_MAX_INDEX) {
    if (IsInProgress())
      connection->OnReceivedResponseToINVITE(*this, response);

    if (response.GetStatusCode() >= 200) {
      PSafeLockReadWrite lock(*this);
      if (!lock.IsLocked())
        return false;

      if (response.GetStatusCode() < 300) {
        // Need to update where the ACK goes to when have 2xx response as per 13.2.2.4
        if (!connection->LockReadOnly())
          return false;

        SIPDialogContext & dialog = connection->GetDialog();
        const PStringList & routeSet = dialog.GetRouteSet();
        SIPURL dest;
        if (routeSet.IsEmpty())
          dest = dialog.GetRequestURI();
        else
          dest = routeSet.front();
        connection->UnlockReadOnly();

        m_remoteAddress = dest.GetHostAddress();
        PTRACE(4, "SIP\tTransaction remote address changed to " << m_remoteAddress);
      }

      // ACK constructed following 13.2.2.4 or 17.1.1.3
      SIPAck ack(*this, response);
      if (!SendPDU(ack))
        return false;
    }
  }

  return SIPTransaction::OnReceivedResponse(response);
}


////////////////////////////////////////////////////////////////////////////////////

SIPRegister::Params::Params()
  : m_expire(0)
  , m_restoreTime(30)
  , m_minRetryTime(PMaxTimeInterval)
  , m_maxRetryTime(PMaxTimeInterval)
  , m_userData(NULL)
{
}


SIPRegister::SIPRegister(SIPEndPoint & ep,
                         OpalTransport & trans,
                         const SIPURL & proxy,
                         const PString & id,
                         unsigned cseq,
                         const Params & params)
  : SIPTransaction(ep, trans, params.m_minRetryTime, params.m_maxRetryTime)
{
  SIPURL uri = params.m_registrarAddress;
  PString to = uri.GetUserName().IsEmpty() ? params.m_addressOfRecord : params.m_registrarAddress;
  uri.Sanitise(SIPURL::RegisterURI);
  SIP_PDU::Construct(Method_REGISTER,
                     uri.AsString(),
                     to,
                     params.m_addressOfRecord,
                     id,
                     cseq,
                     ep.GetLocalURL(transport).GetHostAddress());

  mime.SetProductInfo(ep.GetUserAgent(), ep.GetProductInfo());
  mime.SetContact(params.m_contactAddress);
  mime.SetExpires(params.m_expire);

  SetAllow(ep.GetAllowedMethods());
  SetRoute(proxy);
}


////////////////////////////////////////////////////////////////////////////////////

static const char * const KnownEventPackage[SIPSubscribe::NumPredefinedPackages] = {
  "message-summary",
  "presence",
  "dialog;sla;ma", // sla is the old version ma is the new for Line Appearance extension
};

SIPSubscribe::EventPackage::EventPackage(unsigned pkg)
 : PCaselessString((pkg & PackageMask) < NumPredefinedPackages ? KnownEventPackage[(pkg & PackageMask)] : "")
  , m_isWatcher((pkg & Watcher) != 0)
{
  if (m_isWatcher)
    *this += ".winfo";
}


SIPSubscribe::EventPackage::EventPackage(const PString & str)
  : PCaselessString(str)
{ 
  m_isWatcher = (Right(6) == ".winfo");
}


SIPSubscribe::EventPackage::EventPackage(const char * cstr)
  : PCaselessString(cstr)
{ 
  m_isWatcher = (Right(6) == ".winfo");
}

bool SIPSubscribe::EventPackage::operator==(PredefinedPackages pkg) const
{
  return (m_isWatcher == ((pkg & Watcher) != 0)) &&
          (InternalCompare(0, P_MAX_INDEX, (pkg & PackageMask) < NumPredefinedPackages ? KnownEventPackage[(pkg & PackageMask)] : "") == EqualTo);
}


PObject::Comparison SIPSubscribe::EventPackage::InternalCompare(PINDEX offset, PINDEX length, const char * cstr) const
{
  // Special rules for comparing event package strings, only up to the ';', if present

  for (;;) {
    if (length-- == 0)
      return EqualTo;
    if (theArray[offset] == '\0' && cstr[offset] == '\0')
      return EqualTo;
    if (theArray[offset] == ';' || cstr[offset] == ';')
      break;
    Comparison c = PCaselessString::InternalCompare(offset, cstr[offset]);
    if (c != EqualTo)
      return c;
    offset++;
  }

  const char * myIdPtr = strstr(theArray+offset, "id");
  const char * theirIdPtr = strstr(cstr+offset, "id");
  if (myIdPtr == NULL && theirIdPtr == NULL)
    return EqualTo;

  const char * myIdEnd = strchr(myIdPtr, ';');
  PINDEX myIdLen = myIdEnd != NULL ? myIdEnd - myIdPtr : strlen(myIdPtr);

  const char * theirIdEnd = strchr(theirIdPtr, ';');
  PINDEX theirIdLen = theirIdEnd != NULL ? theirIdEnd - theirIdPtr : strlen(theirIdPtr);

  if (myIdLen < theirIdLen)
    return LessThan;

  if (myIdLen > theirIdLen)
    return GreaterThan;

  return (Comparison)strncmp(myIdPtr, theirIdPtr, theirIdLen);
}


SIPSubscribe::Params::Params(unsigned pkg)
  : m_eventPackage(pkg)
  , m_expire(0)
  , m_restoreTime(30)
  , m_minRetryTime(PMaxTimeInterval)
  , m_maxRetryTime(PMaxTimeInterval)
  , m_userData(NULL)
{
}


SIPSubscribe::SIPSubscribe(SIPEndPoint & ep,
                           OpalTransport & trans,
                           SIPDialogContext & dialog,
                           const Params & params)
  : SIPTransaction(ep, trans)
{
  Construct(Method_SUBSCRIBE, dialog);

  SIPURL contact;
  if (params.m_contactAddress.IsEmpty()) {
    // I have no idea why this is necessary, but it is the way OpenSIPS works ....
    const SIPURL & userURI = params.m_eventPackage == SIPSubscribe::Dialog ? dialog.GetRemoteURI() : dialog.GetLocalURI();
    contact = endpoint.GetLocalURL(trans, userURI.GetUserName());
  }
  else
    contact = params.m_contactAddress;
  contact.Sanitise(SIPURL::ContactURI);
  mime.SetContact(contact);

  mime.SetProductInfo(ep.GetUserAgent(), ep.GetProductInfo());
  mime.SetEvent(params.m_eventPackage);
  mime.SetExpires(params.m_expire);

  SIPEventPackageHandler * packageHandler = SIPEventPackageFactory::CreateInstance(params.m_eventPackage);
  if (packageHandler != NULL) {
    mime.SetAccept(packageHandler->GetContentType());
    delete packageHandler;
  }

  SetAllow(ep.GetAllowedMethods());
}


////////////////////////////////////////////////////////////////////////////////////

SIPNotify::SIPNotify(SIPEndPoint & ep,
                     OpalTransport & trans,
                     SIPDialogContext & dialog,
                     const SIPEventPackage & eventPackage,
                     const PString & state,
                     const PString & body)
  : SIPTransaction(ep, trans)
{
  Construct(Method_NOTIFY, dialog);

  SIPURL contact = endpoint.GetLocalURL(trans, dialog.GetLocalURI().GetUserName());
  contact.Sanitise(SIPURL::ContactURI);
  mime.SetContact(contact);
  mime.SetProductInfo(ep.GetUserAgent(), ep.GetProductInfo());
  mime.SetEvent(eventPackage);
  mime.SetSubscriptionState(state);

  SIPEventPackageHandler * packageHandler = SIPEventPackageFactory::CreateInstance(eventPackage);
  if (packageHandler != NULL) {
    mime.SetContentType(packageHandler->GetContentType());
    delete packageHandler;
  }

  entityBody = body;
}


////////////////////////////////////////////////////////////////////////////////////

SIPPublish::SIPPublish(SIPEndPoint & ep,
                       OpalTransport & trans,
                       const PString & id,
                       const PString & sipIfMatch,
                       SIPSubscribe::Params & params,
                       const PString & body)
  : SIPTransaction(ep, trans)
{
  SIPURL toAddress = params.m_addressOfRecord;
  SIPURL fromAddress = toAddress;
  fromAddress.SetTag();

  SIP_PDU::Construct(Method_PUBLISH,
                     toAddress,
                     toAddress.AsQuotedString(),
                     fromAddress.AsQuotedString(),
                     id,
                     endpoint.GetNextCSeq(),
                     ep.GetLocalURL(transport).GetHostAddress());

  mime.SetProductInfo(ep.GetUserAgent(), ep.GetProductInfo());
  SIPURL contact = endpoint.GetLocalURL(trans, toAddress.GetUserName());
  contact.Sanitise(SIPURL::ContactURI);
  mime.SetContact(contact);
  mime.SetExpires(params.m_expire);

  if (!sipIfMatch.IsEmpty())
    mime.SetSIPIfMatch(sipIfMatch);
  
  mime.SetEvent(params.m_eventPackage);
  SIPEventPackageHandler * packageHandler = SIPEventPackageFactory::CreateInstance(params.m_eventPackage);
  if (packageHandler != NULL) {
    mime.SetContentType(packageHandler->GetContentType());
    delete packageHandler;
  }

  SetRoute(SIPURL(params.m_agentAddress));

  if (!body.IsEmpty())
    entityBody = body;
}


/////////////////////////////////////////////////////////////////////////

SIPRefer::SIPRefer(SIPConnection & connection, OpalTransport & transport, const SIPURL & refer, const SIPURL & referred_by)
  : SIPTransaction(connection, transport, Method_REFER)
{
  Construct(connection, transport, refer, referred_by);
}

SIPRefer::SIPRefer(SIPConnection & connection, OpalTransport & transport, const SIPURL & refer)
  : SIPTransaction(connection, transport, Method_REFER)
{
  Construct(connection, transport, refer, SIPURL());
}


void SIPRefer::Construct(SIPConnection & connection, OpalTransport & /*transport*/, const SIPURL & refer, const SIPURL & _referred_by)
{
  SIPURL referred_by = _referred_by;

  mime.SetProductInfo(connection.GetEndPoint().GetUserAgent(), connection.GetProductInfo());
  mime.SetReferTo(refer.AsQuotedString());
  if(!referred_by.IsEmpty()) {
    referred_by.SetDisplayName(PString::Empty());
    mime.SetReferredBy(referred_by.AsQuotedString());
  }
}


/////////////////////////////////////////////////////////////////////////

SIPReferNotify::SIPReferNotify(SIPConnection & connection, OpalTransport & transport, StatusCodes code)
  : SIPTransaction(connection, transport, Method_NOTIFY)
{
  mime.SetProductInfo(connection.GetEndPoint().GetUserAgent(), connection.GetProductInfo());
  mime.SetSubscriptionState(code < Successful_OK ? "active" : "terminated;reason=noresource");
  mime.SetEvent("refer");
  mime.SetContentType("message/sipfrag");

  PStringStream str;
  str << "SIP/" << versionMajor << '.' << versionMinor << " " << code << " " << GetStatusCodeDescription(code);
  entityBody = str;
}


/////////////////////////////////////////////////////////////////////////

SIPMessage::SIPMessage(SIPEndPoint & ep,
                       OpalTransport & trans,
                       const SIPURL & proxy,
                       const SIPURL & address,
                       const PString & id,
                       const PString & body,
                       SIPURL & localAddress)
  : SIPTransaction(ep, trans)
{
  SIPURL myAddress = localAddress = endpoint.GetRegisteredPartyName(address.GetHostName(), transport);
  myAddress.SetTag();

  SIP_PDU::Construct(Method_MESSAGE,
                     address.AsQuotedString(),
                     address.AsQuotedString(),
                     myAddress.AsQuotedString(),
                     id,
                     endpoint.GetNextCSeq(),
                     ep.GetLocalURL(transport).GetHostAddress());
  mime.SetContentType("text/plain;charset=UTF-8");
  SetRoute(proxy);

  entityBody = body;
}

/////////////////////////////////////////////////////////////////////////

SIPPing::SIPPing(SIPEndPoint & ep,
                 OpalTransport & trans,
                 const SIPURL & address,
                 const PString & body)
  : SIPTransaction(ep, trans)
{
  SIP_PDU::Construct(Method_PING,
                     address.AsQuotedString(),
                     address.AsQuotedString(),
                     "sip:"+address.GetUserName()+"@"+address.GetHostName(),
                     GenerateCallID(),
                     endpoint.GetNextCSeq(),
                     ep.GetLocalURL(transport).GetHostAddress());
  mime.SetContentType("text/plain;charset=UTF-8");

  entityBody = body;
}


/////////////////////////////////////////////////////////////////////////

SIPAck::SIPAck(SIPTransaction & invite, SIP_PDU & response)
{
  if (response.GetStatusCode() < 300)
    Construct(Method_ACK, *invite.GetConnection(), invite.GetTransport(), invite.GetMIME().GetCSeqIndex());
  else {
    Construct(Method_ACK,
              invite.GetURI(),
              response.GetMIME().GetTo(),
              invite.GetMIME().GetFrom(),
              invite.GetMIME().GetCallID(),
              invite.GetMIME().GetCSeqIndex(),
              invite.GetConnection()->GetEndPoint().GetLocalURL(invite.GetTransport()).GetHostAddress());

    // Use the topmost via header from the INVITE we ACK as per 17.1.1.3
    // as well as the initial Route
    PStringList viaList = invite.GetMIME().GetViaList();
    if (viaList.GetSize() > 0)
      mime.SetVia(viaList.front());

    if (invite.GetMIME().GetRoute().GetSize() > 0)
      mime.SetRoute(invite.GetMIME().GetRoute());
  }

  // Add authentication if had any on INVITE
  if (invite.GetMIME().Contains("Proxy-Authorization") || invite.GetMIME().Contains("Authorization"))
    invite.GetConnection()->GetAuthenticator()->Authorise(*this);
}


/////////////////////////////////////////////////////////////////////////

SIPOptions::SIPOptions(SIPEndPoint & ep,
                       OpalTransport & trans,
                       const SIPURL & address)
  : SIPTransaction(ep, trans)
{
  // Build the correct From field
  SIPURL myAddress = endpoint.GetRegisteredPartyName(address.GetHostName(), transport);
  myAddress.SetTag();

  SIP_PDU::Construct(Method_OPTIONS,
                     address,
                     address.AsQuotedString(),
                     myAddress.AsQuotedString(),
                     GenerateCallID(),
                     endpoint.GetNextCSeq(),
                     ep.GetLocalURL(transport).GetHostAddress());
  mime.SetAccept("application/sdp, application/media_control+xml, application/dtmf, application/dtmf-relay");

  SetAllow(ep.GetAllowedMethods());
}


#endif // OPAL_SIP

// End of file ////////////////////////////////////////////////////////////////
