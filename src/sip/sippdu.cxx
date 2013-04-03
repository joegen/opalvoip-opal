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

#include <ptclib/pdns.h>
#include <sip/sipep.h>
#include <sip/sipcon.h>
#include <sip/sdp.h>
#include <codec/opalplugin.h>


#define  SIP_VER_MAJOR  2
#define  SIP_VER_MINOR  0


#define new PNEW

static bool LocateFieldParameter(const PString & fieldValue, const PString & paramName, PINDEX & start, PINDEX & val, PINDEX & end);

static PConstCaselessString const TagStr("tag");
static PConstString const TransactionPrefix("z9hG4bK");

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
  "PUBLISH",
  "PRACK"
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


PString SIP_PDU::GetStatusCodeDescription(int code)
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
    { SIP_PDU::Local_Timeout,                       "Timeout or retries exceeded" },

    { 0 }
  };

  for (PINDEX i = 0; sipErrorDescriptions[i].code != 0; i++) {
    if (sipErrorDescriptions[i].code == code)
      return sipErrorDescriptions[i].desc;
  }
  return PString::Empty();
}


ostream & operator<<(ostream & strm, SIP_PDU::StatusCodes status)
{
  strm << (unsigned)status;
  PString info = SIP_PDU::GetStatusCodeDescription(status);
  if (!info.IsEmpty())
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

/////////////////////////////////////////////////////////////////////////////

const WORD SIPURL::DefaultPort = 5060;
const WORD SIPURL::DefaultSecurePort = 5061;

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


SIPURL::SIPURL(const SIPMIMEInfo & mime, const char * name)
{
  ReallyInternalParse(true, mime.GetString(name), "sip");
}


SIPURL::SIPURL(const PString & name,
               const OpalTransportAddress & address,
               WORD listenerPort,
               const char * scheme)
{
  if (strncmp(name, "sip:", 4) == 0 || strncmp(name, "sips:", 5) == 0)
    Parse(name);
  else if (address.IsEmpty() && (name.Find('$') != P_MAX_INDEX)) 
    ParseAsAddress(PString::Empty(), name, listenerPort, scheme);
  else
    ParseAsAddress(name, address, listenerPort, scheme);
}

SIPURL::SIPURL(const OpalTransportAddress & address, WORD listenerPort, const char * scheme)
{
  ParseAsAddress(PString::Empty(), address, listenerPort, scheme);
}

SIPURL & SIPURL::operator=(const OpalTransportAddress & address)
{
  ParseAsAddress(PString::Empty(), address, 0, NULL);
  return *this;
}


void SIPURL::ParseAsAddress(const PString & name,
                            const OpalTransportAddress & address,
                            WORD listenerPort,
                            const char * scheme)
{
  PIPSocket::Address ip;
  WORD port;
  if (!address.GetIpAndPort(ip, port))
    return;

  PCaselessString proto = address.GetProtoPrefix();

  PCaselessString actualScheme = scheme;
  if (actualScheme.IsEmpty()) {
#if OPAL_PTLIB_SSL
    if (proto == OpalTransportAddress::TlsPrefix())
      actualScheme = "sips";
    else
#endif
      actualScheme = "sip";
  }

  WORD defaultPort;
  PString defaultProto;

  if (actualScheme != "sips") {
    defaultPort = SIPURL::DefaultPort;
    defaultProto = OpalTransportAddress::UdpPrefix();
  }
  else {
#if OPAL_PTLIB_SSL
    defaultPort = SIPURL::DefaultSecurePort;
    defaultProto = OpalTransportAddress::TlsPrefix();
#else
    PTRACE(1, "sip\tCannot use sips if do not have TLS");
    return;
#endif
  }

  PStringStream uri;
  uri << actualScheme << ':';
  if (!name.IsEmpty())
    uri << name << '@';
  uri << ip.AsString(true, true);

  if (listenerPort == 0)
    listenerPort = port;
  if (listenerPort != 0 && listenerPort != defaultPort)
    uri << ':' << listenerPort;

  if (proto != defaultProto) {
    proto.Delete(proto.Find('$'), 1); // Remove dollar
    uri << ";transport=" << proto;
  }

  Parse(uri);
}


PBoolean SIPURL::ReallyInternalParse(bool fromHeader, const char * cstr, const char * p_defaultScheme)
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

  m_displayName.MakeEmpty();
  m_fieldParameters.RemoveAll();

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

    m_displayName = str(1, endQuote-1);

    PINDEX backslash;
    while ((backslash = m_displayName.Find('\\')) != P_MAX_INDEX)
      m_displayName.Delete(backslash, 1);
  }

  // see if URL is just a URI or it contains a display address as well
  PINDEX startBracket = str.Find('<', endQuote);
  PINDEX endBracket = str.Find('>', startBracket);

  const char * defaultScheme = p_defaultScheme != NULL ? p_defaultScheme : "sip";

  // see if URL is just a URI or it contains a display address as well
  if (startBracket == P_MAX_INDEX || endBracket == P_MAX_INDEX) {
    if (!PURL::InternalParse(cstr, defaultScheme))
      return false;

    if (fromHeader) {
      // RFC says that if no <> then ; parameters belong to field, not URI.
      m_fieldParameters = paramVars;
      paramVars = PStringToString(); // Do not use RemoveAll()
    }
  }
  else {
    // get the URI from between the angle brackets
    if (!PURL::InternalParse(str(startBracket+1, endBracket-1), defaultScheme))
      return false;

    PURL::SplitVars(str.Mid(endBracket+1).Trim(), m_fieldParameters, ';', '=', QuotedParameterTranslation);

    if (endQuote == 0) {
      // There were no double quotes around the display name, take
      // everything before the start angle bracket, sans whitespace
      m_displayName = str.Left(startBracket).Trim();
    }
  }

  if (!portSupplied)
    port = GetDefaultPort();

  // Do sanity check of some parameters that cannot be doubled up
  for (PStringToString::iterator it = paramVars.begin(); it != paramVars.end(); ++it) {
    PINDEX pos = it->second.FindLast('\n');
    if (pos != P_MAX_INDEX &&
          (it->first == "user" || it->first == "ttl" || it->first == "method" || it->first == "transport")) {
      it->second.Delete(0, pos+1);
      Recalculate();
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
  for (PStringToString::const_iterator it = paramVars.begin(); it != paramVars.end(); ++it) {
    PString param = it->first;
    if (other.paramVars.Contains(param))
      COMPARE_COMPONENT(paramVars[param]);
  }
  COMPARE_COMPONENT(paramVars("user"));
  COMPARE_COMPONENT(paramVars("ttl"));
  COMPARE_COMPONENT(paramVars("method"));

  /* While RFC3261/19.1.4 does not mention transport explicitly in the same
     sectionas the above, the "not equivalent" examples do state that the
     absence of a transport is not the same as using the default. While this
     is not normative, the logic is impeccable so we add it in. */
  COMPARE_COMPONENT(paramVars("transport"));

  return EqualTo;
}


PString SIPURL::AsQuotedString() const
{
  PStringStream s;

  if (!m_displayName)
    s << '"' << m_displayName << "\" ";
  s << '<' << AsString() << '>';

  OutputVars(s, m_fieldParameters, ';', ';', '=', QuotedParameterTranslation);

  return s;
}


PString SIPURL::GetDisplayName(PBoolean useDefault) const
{
  if (m_displayName.IsEmpty() && useDefault)
    return AsString();
  return m_displayName;
}


OpalTransportAddress SIPURL::GetTransportAddress(PINDEX entry) const
{
  if (IsEmpty())
    return OpalTransportAddress();

  PString maddr = paramVars("maddr");
  if (!maddr.IsEmpty())
    return OpalTransportAddress(maddr, GetPort(), OpalTransportAddress::UdpPrefix());

  PCaselessString proto = GetTransportProto();

  // Or it is a valid IP address, not a domain name
  PIPSocket::Address ip = GetHostName();
  if (ip.IsValid())
    return OpalTransportAddress(ip, port, proto);

  if (entry == P_MAX_INDEX)
    return OpalTransportAddress(GetHostName(), port, proto);

#if OPAL_PTLIB_DNS_RESOLVER
  // RFC3263 states we do not do lookup if explicit port mentioned
  if (GetPortSupplied()) {
    PTRACE(4, "SIP\tNo SRV lookup as has explicit port number.");
  }
  else {
    // Do the SRV lookup, if fails, then we actually return TRUE so outer loops
    // can use the original host name value.
    PIPSocketAddressAndPortVector addrs;
    if (!PDNS::LookupSRV(GetHostName(), "_sip._" + proto, GetPort(), addrs)) {
      PTRACE(4, "SIP\tNo SRV record found.");
    }
    else {
      // Got the SRV list, return FALSE if outer loop has got to the end of it
      if (entry >= (PINDEX)addrs.size()) {
        PTRACE(4, "SIP\tRan out of SRV records at entry " << entry);
        return OpalTransportAddress();
      }

      // Adjust our host and port to what the DNS SRV record says
      PTRACE(4, "SIP\tAttempting SRV record entry " << entry << ": " << addrs[entry]);
      return OpalTransportAddress(addrs[entry].GetAddress(), addrs[entry].GetPort(), proto);
    }
  }
#endif // OPAL_PTLIB_DNS_RESOLVER

  if (PIPSocket::GetHostAddress(GetHostName(), ip))
    return OpalTransportAddress(ip, port, proto);

  return OpalTransportAddress(GetHostName(), port, proto);
}


void SIPURL::SetHostAddress(const OpalTransportAddress & addr)
{
  PIPSocket::Address ip;
  WORD port = GetPort();
  if (!addr.GetIpAndPort(ip, port))
    return;
  SetHostName(ip.AsString(true));
  SetPort(port);
}


PCaselessString SIPURL::GetTransportProto() const
{
  return paramVars("transport", scheme == "sips" ? "tls" : "udp").ToLower();
}


WORD SIPURL::GetDefaultPort() const
{
  return GetTransportProto() == "tls" ? SIPURL::DefaultSecurePort : SIPURL::DefaultPort;
}


void SIPURL::Sanitise(UsageContext context)
{
  PINDEX i;

  // RFC3261, 19.1.1 Table 1
  static struct {
    const char * name;
    unsigned     contexts;
  } const SanitaryFields[] = {
    { "method",    (1<<RequestURI)|(1<<ToURI)|(1<<FromURI)|(1<<RedirectURI)|(1<<ContactURI)|(1<<RegContactURI)|(1<<RouteURI)|(1<<RegisterURI)                  },
    { "maddr",                     (1<<ToURI)|(1<<FromURI)                                                                                                     },
    { "ttl",                       (1<<ToURI)|(1<<FromURI)                 |(1<<ContactURI)                   |(1<<RouteURI)|(1<<RegisterURI)                  },
    { "transport",                 (1<<ToURI)|(1<<FromURI)                                                                                                     },
    { "lr",                        (1<<ToURI)|(1<<FromURI)|(1<<RedirectURI)                |(1<<RegContactURI)              |(1<<RegisterURI)                  },
    { "expires",   (1<<RequestURI)|(1<<ToURI)|(1<<FromURI)                 |(1<<ContactURI)                   |(1<<RouteURI)|(1<<RegisterURI)|(1<<ExternalURI) },
    { "q",         (1<<RequestURI)|(1<<ToURI)|(1<<FromURI)                 |(1<<ContactURI)                   |(1<<RouteURI)|(1<<RegisterURI)|(1<<ExternalURI) },
    { TagStr,      (1<<RequestURI)                        |(1<<RedirectURI)|(1<<ContactURI)|(1<<RegContactURI)|(1<<RouteURI)|(1<<RegisterURI)|(1<<ExternalURI) }
  };

  for (i = 0; i < PARRAYSIZE(SanitaryFields); i++) {
    if (SanitaryFields[i].contexts&(1<<context)) {
      PCaselessString name = SanitaryFields[i].name;
      paramVars.RemoveAt(name);
      m_fieldParameters.RemoveAt(name);
    }
  }

  for (PStringToString::iterator it = paramVars.begin(); it != paramVars.end(); ) {
    PCaselessString key = it->first;
    if (key.NumCompare("OPAL-") != EqualTo)
      ++it;
    else if (paramVars.MakeUnique())
      paramVars.erase(it++);
    else {
      paramVars.RemoveAt(key);
      it = paramVars.begin();
    }
  }

  if (context != RedirectURI && context != ExternalURI) {
    queryVars.MakeUnique();
    queryVars.RemoveAll();
  }

  WORD defPort = GetDefaultPort();

  switch (context) {
    case RequestURI :
      SetDisplayName(PString::Empty());
      break;

    case ToURI :
    case FromURI :
      // Port not allowed for To or From, RFC3261, 19.1.1
      portSupplied = false;
      break;

    case RegContactURI :
      /* Strict application of RFC3261, 19.1.1 does not require this, but
         there are some interop problems with registrars where the ports
         presence/absence is not handled correctly. There is a distinction
         between absence and presence with the default value, that is
         "sip:fred" is not the same as "sip:fred:5060". Some registrars
         remove the default port if present, others insert it. Sheesh!

         Safest thing to do is always assume an explicit port number for
         REGISTER contact field and never send an empty port. This should be
         generally OK, though not perfect. It prevents using an SRV record
         as the contact, but as the thing doing the registering is nearly
         always the thing listening on a port, it is the 99.9% solution.
       */
      if (!portSupplied) {
        portSupplied = true;
        port = defPort;
      }
      break;

    case RegisterURI :
      username.MakeEmpty();
      password.MakeEmpty();

    default:
      break;
  }

  if (!portSupplied)
    port = defPort;

  Recalculate();
}


PString SIPURL::GenerateTag()
{
  return OpalGloballyUniqueID().AsString();
}


void SIPURL::SetTag(const PString & tag, bool force)
{
  if (force || !m_fieldParameters.Contains(TagStr))
    m_fieldParameters.SetAt(TagStr, tag.IsEmpty() ? GenerateTag() : tag);
}


PString SIPURL::GetTag() const
{
  return m_fieldParameters.Get(TagStr);
}


/////////////////////////////////////////////////////////////////////////////

bool SIPURLList::FromString(const PString & str, SIPURL::UsageContext context, bool reversed)
{
  clear();

  PStringArray lines = str.Lines();
  for (PINDEX i = 0; i < lines.GetSize(); i++) {
    PString line = lines[i];

    PINDEX previousPos = (PINDEX)-1;
    PINDEX comma = previousPos;
    do {
      PINDEX pos = line.FindOneOf(",\"<", previousPos+1);
      if (pos != P_MAX_INDEX && line[pos] != ',') {
        if (line[pos] == '<')
          previousPos = line.Find('>', pos);
        else {
          PINDEX lastQuote = pos;
          do { 
            lastQuote = line.Find('"', lastQuote+1);
          } while (lastQuote != P_MAX_INDEX && line[lastQuote-1] == '\\');
          previousPos = lastQuote;
        }
        if (previousPos != P_MAX_INDEX)
          continue;
        pos = previousPos;
      }

      SIPURL uri = line(comma+1, pos-1);
      uri.Sanitise(context);

      if (context == SIPURL::RegContactURI) {
        double q = uri.GetFieldParameters().GetReal("q");
        SIPURLList::iterator it = begin();
        while (it != end() && it->GetFieldParameters().GetReal("q") >= q)
          ++it;
        insert(it, uri);
      }
      else if (reversed)
        push_front(uri);
      else
        push_back(uri);

      comma = previousPos = pos;
    } while (comma != P_MAX_INDEX);
  }

  return !empty();
}


PString SIPURLList::ToString() const
{
  PStringStream strm;
  strm << *this;
  return strm;
}


ostream & operator<<(ostream & strm, const SIPURLList & urls)
{
  bool outputCommas = false;
  for (SIPURLList::const_iterator it = urls.begin(); it != urls.end(); ++it) {
    if (it->IsEmpty())
      continue;

    if (outputCommas)
      strm << ", ";
    else
      outputCommas = true;

    strm << it->AsQuotedString();
  }
  return strm;
}


/////////////////////////////////////////////////////////////////////////////

SIPMIMEInfo::SIPMIMEInfo(PBoolean _compactForm)
  : compactForm(_compactForm)
{
  SetContentLength(0);
}


void SIPMIMEInfo::PrintOn(ostream & strm) const
{
  const char * eol = strm.fill() == '\r' ? "\r\n" : "\n";

  for (PStringToString::const_iterator it = begin(); it != end(); ++it) {
    PCaselessString name = it->first;
    PString value = it->second;

    if (compactForm) {
      for (PINDEX i = 0; i < PARRAYSIZE(CompactForms); ++i) {
        if (name == CompactForms[i].full) {
          name = CompactForms[i].compact;
          break;
        }
      }
    }

    if (value.FindOneOf("\r\n") == P_MAX_INDEX)
      strm << name << ": " << value << eol;
    else {
      PStringArray vals = value.Lines();
      for (PINDEX j = 0; j < vals.GetSize(); j++)
        strm << name << ": " << vals[j] << eol;
    }
  }

  strm << eol;
}


bool SIPMIMEInfo::InternalAddMIME(const PString & fieldName, const PString & fieldValue)
{
  if (fieldName.GetLength() == 1) {
    char compact = (char)tolower(fieldName[0] & 0x7f);
    for (PINDEX i = 0; i < PARRAYSIZE(CompactForms); ++i) {
      if (compact == CompactForms[i].compact)
        return PMIMEInfo::InternalAddMIME(CompactForms[i].full, fieldValue);
    }
  }

  return PMIMEInfo::InternalAddMIME(fieldName, fieldValue);
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
  PCaselessString str = GetString(PMIMEInfo::ContentTypeTag);
  return str.Left(includeParameters ? P_MAX_INDEX : str.Find(';')).Trim();
}


void SIPMIMEInfo::SetContentType(const PString & v)
{
  SetAt(PMIMEInfo::ContentTypeTag,  v);
}


PCaselessString SIPMIMEInfo::GetContentEncoding() const
{
  return GetString("Content-Encoding");
}


void SIPMIMEInfo::SetContentEncoding(const PString & v)
{
  SetAt("Content-Encoding",  v);
}


SIPURL SIPMIMEInfo::GetFrom() const
{
  return SIPURL(*this, "From");
}


void SIPMIMEInfo::SetFrom(const SIPURL & uri)
{
  SetAt("From", uri.AsQuotedString());
}

SIPURL SIPMIMEInfo::GetPAssertedIdentity() const
{
  return SIPURL(*this, "P-Asserted-Identity");
}

void SIPMIMEInfo::SetPAssertedIdentity(const PString & v)
{
  SetAt("P-Asserted-Identity", v);
}

SIPURL SIPMIMEInfo::GetPPreferredIdentity() const
{
  return SIPURL(*this, "P-Preferred-Identity");
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


SIPURL SIPMIMEInfo::GetContact() const
{
  return SIPURL(*this, "Contact");
}


bool SIPMIMEInfo::GetContacts(SIPURLList & contacts) const
{
  return contacts.FromString(GetString("Contact"), SIPURL::RegContactURI);
}


void SIPMIMEInfo::SetContact(const PString & v)
{
  SetAt("Contact",  v);
}


PString SIPMIMEInfo::GetSubject() const
{
  return GetString("Subject");
}


void SIPMIMEInfo::SetSubject(const PString & v)
{
  SetAt("Subject",  v);
}


SIPURL SIPMIMEInfo::GetTo() const
{
  return SIPURL(*this, "To");
}


void SIPMIMEInfo::SetTo(const SIPURL & uri)
{
  SetAt("To", uri.AsQuotedString());
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


bool SIPMIMEInfo::GetViaList(PStringList & viaList) const
{
  PString s = GetVia();
  if (s.FindOneOf("\r\n") != P_MAX_INDEX)
    viaList = s.Lines();
  else
    viaList = s.Tokenise(",", false);

  return !viaList.IsEmpty();
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


PString SIPMIMEInfo::GetFirstVia() const
{
  // Get first via
  PString via = GetVia();
  via.Delete(via.FindOneOf("\r\n"), P_MAX_INDEX);
  return via;
}

OpalTransportAddress SIPMIMEInfo::GetViaReceivedAddress() const
{
  // Get first via
  PCaselessString via = GetFirstVia();

  if (via.Find("/UDP") == P_MAX_INDEX)
    return OpalTransportAddress();

  PINDEX start, val, end;
  if (!LocateFieldParameter(via, "rport", start, val, end) || val >= end)
    return OpalTransportAddress();

  WORD port = (WORD)via(val, end).AsUnsigned();
  if (port == 0)
    return OpalTransportAddress();
  
  if (LocateFieldParameter(via, "received", start, val, end) && val < end)
    return OpalTransportAddress(via(val, end), port, OpalTransportAddress::UdpPrefix());

  return OpalTransportAddress(via(via.Find(' ')+1, via.FindOneOf(";:")-1), port, OpalTransportAddress::UdpPrefix());
}


SIPURL SIPMIMEInfo::GetReferTo() const
{
  return SIPURL(*this, "Refer-To");
}


void SIPMIMEInfo::SetReferTo(const PString & r)
{
  SetAt("Refer-To",  r);
}

SIPURL SIPMIMEInfo::GetReferredBy() const
{
  // If no RFC 3892 header, try Cisco custom header
  return SIPURL(*this, Contains("Referred-By") ? "Referred-By" : "Diversion");
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


PString SIPMIMEInfo::GetRoute() const
{
  return Get("Route");
}


bool SIPMIMEInfo::GetRoute(SIPURLList & proxies) const
{
  return proxies.FromString(GetRoute());
}


void SIPMIMEInfo::SetRoute(const PString & v)
{
  if (!v.IsEmpty())
    SetAt("Route",  v);
}


void SIPMIMEInfo::SetRoute(const SIPURLList & proxies)
{
  if (!proxies.empty())
    SetRoute(proxies.ToString());
}


PString SIPMIMEInfo::GetRecordRoute() const
{
  return Get("Record-Route");
}


bool SIPMIMEInfo::GetRecordRoute(SIPURLList & proxies, bool reversed) const
{
  return proxies.FromString(GetRecordRoute(), SIPURL::RouteURI, reversed);
}


void SIPMIMEInfo::SetRecordRoute(const PString & v)
{
  if (!v.IsEmpty())
    SetAt("Record-Route",  v);
}


void SIPMIMEInfo::SetRecordRoute(const SIPURLList & proxies)
{
  if (!proxies.empty())
    SetRecordRoute(proxies.ToString());
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


unsigned SIPMIMEInfo::GetAllowBitMask() const
{
  unsigned bits = 0;

  PCaselessString allowedMethods = GetAllow();
  for (unsigned i = 0; i < SIP_PDU::NumMethods; ++i) {
    if (allowedMethods.Find(MethodNames[i]) != P_MAX_INDEX)
      bits |= (1 << i);
  }

  return bits;
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


unsigned SIPMIMEInfo::GetMaxForwards() const
{
  return GetInteger("Max-Forwards", INT_MAX);       // no compact form
}


void SIPMIMEInfo::SetMaxForwards(PINDEX v)
{
  SetAt("Max-Forwards", PString(PString::Unsigned, v)); // no compact form
}


unsigned SIPMIMEInfo::GetMinExpires() const
{
  return GetInteger("Min-Expires", INT_MAX);        // no compact form
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


PStringSet SIPMIMEInfo::GetTokenSet(const char * field) const
{
  PStringSet set;
  PStringArray tokens = GetString(field).Tokenise(',');
  for (PINDEX i = 0; i < tokens.GetSize(); ++i) {
    PString token = tokens[i].Trim();
    if (!token.IsEmpty())
      set += token;
  }
  return set;
}


void SIPMIMEInfo::AddTokenSet(const char * field, const PString & token)
{
  if (token.IsEmpty())
    RemoveAt(field);
  else {
    PString existing = GetString(field);
    if (existing.IsEmpty())
      SetAt(field,  token);
    else {
      existing += ',';
      existing += token;
      SetAt(field,  existing);
    }
  }
}


void SIPMIMEInfo::SetTokenSet(const char * field, const PStringSet & tokens)
{
  if (tokens.IsEmpty())
    RemoveAt(field);
  else {
    PStringStream strm;
    bool outputSeparator = false;
    for (PStringSet::const_iterator it = tokens.begin(); it != tokens.end(); ++it) {
      if (outputSeparator)
        strm << ',';
      else
        outputSeparator = true;
      strm << *it;
    }
    SetAt(field,  strm);
  }
}


PStringSet SIPMIMEInfo::GetRequire() const
{
  return GetTokenSet("Require");
}


void SIPMIMEInfo::SetRequire(const PStringSet & v)
{
  SetTokenSet("Require", v);
}


void SIPMIMEInfo::AddRequire(const PString & v)
{
  SetTokenSet("Require", v);
}


PStringSet SIPMIMEInfo::GetSupported() const
{
  return GetTokenSet("Supported");
}


void SIPMIMEInfo::SetSupported(const PStringSet & v)
{
  SetTokenSet("Supported", v);
}


void SIPMIMEInfo::AddSupported(const PString & v)
{
  AddTokenSet("Supported", v);
}


PStringSet SIPMIMEInfo::GetUnsupported() const
{
  return GetTokenSet("Unsupported");
}


void SIPMIMEInfo::SetUnsupported(const PStringSet & v)
{
  SetTokenSet("Unsupported",  v);
}


void SIPMIMEInfo::AddUnsupported(const PString & v)
{
  SetTokenSet("Unsupported",  v);
}


PString SIPMIMEInfo::GetEvent() const
{
  return GetString("Event");
}


void SIPMIMEInfo::SetEvent(const PString & v)
{
  SetAt("Event",  v);
}


PCaselessString SIPMIMEInfo::GetSubscriptionState(PStringToString & info) const
{
  return GetComplex("Subscription-State", info) ? info[PString::Empty()] : PString::Empty();
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


PString SIPMIMEInfo::GetAllowEvents() const
{
  return GetString("Allow-Events");     // no compact form
}


void SIPMIMEInfo::SetAllowEvents(const PString & v)
{
  if (!v.IsEmpty())
    SetAt("Allow-Events", v);   // no compact form
}


void SIPMIMEInfo::SetAllowEvents(const PStringSet & set)
{
  if (set.IsEmpty())
    return;

  PStringStream strm;
  strm << setfill(',') << set;
  SetAllowEvents(strm);
}


static const char UserAgentTokenChars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-.!%*_+`'~";

void SIPMIMEInfo::GetProductInfo(OpalProductInfo & info) const
{
  PCaselessString str = GetUserAgent();
  if (str.IsEmpty()) {
    str = GetString("Server");
    if (str.IsEmpty()) {
      PTRACE_IF(4, info.name.IsEmpty(), "SIP\tNo User-Agent or Server fields, Product Info unknown.");
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


static PConstCaselessString const AppearanceFieldParamName("appearance");
static PConstCaselessString const x_line_id("x-line-id");

void SIPMIMEInfo::GetAlertInfo(PString & info, int & appearance) const
{
  info.MakeEmpty();
  appearance = -1;

  SIPURL uri = GetString("Alert-Info");
  if (uri.IsEmpty())
    return;

  PStringOptions & params = uri.GetFieldParameters();

  if (params.Has(AppearanceFieldParamName)) {
    appearance = params[AppearanceFieldParamName].AsUnsigned();
    params.Remove(AppearanceFieldParamName);
  }
  else if (params.Has(x_line_id)) {
    appearance = params[x_line_id].AsUnsigned();
    params.Remove(x_line_id);
  }

  info = uri.AsQuotedString();
}


void SIPMIMEInfo::SetAlertInfo(const PString & info, int appearance)
{
  if (appearance < 0 && info.IsEmpty()) {
    RemoveAt("Alert-Info");
    return;
  }

  if (info.IsEmpty())
    SetAt("Alert-Info", PSTRSTRM("<>;" << AppearanceFieldParamName << '=' << appearance));
  else {
    SIPURL uri = info;
    if (appearance >= 0)
      uri.GetFieldParameters().Set(AppearanceFieldParamName, appearance);
    SetAt("Alert-Info", uri.AsQuotedString());
  }
}


PString SIPMIMEInfo::GetCallInfo() const
{
  return GetString("Call-Info");
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
        end = val-1;
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
  PString str = fieldValue;
  if (LocateFieldParameter(fieldValue, paramName, start, val, end))
    str.Splice(newField, start, end-start+1);
  else
    str += ';' + newField;
  return str;
}


////////////////////////////////////////////////////////////////////////////////////

SIPAuthenticator::SIPAuthenticator(SIP_PDU & pdu)
  : m_pdu(pdu)
{
}


PMIMEInfo & SIPAuthenticator::GetMIME()
{
  return m_pdu.GetMIME();
}


PString SIPAuthenticator::GetURI()
{
  return m_pdu.GetURI().AsString();
}


PString SIPAuthenticator::GetEntityBody()
{
  m_pdu.SetEntityBody();
  return m_pdu.GetEntityBody();
}


PString SIPAuthenticator::GetMethod()
{
  return MethodNames[m_pdu.GetMethod()];
}


////////////////////////////////////////////////////////////////////////////////////

class SIPNTLMAuthentication : public PHTTPClientAuthentication
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

  return PHTTPClientAuthentication::Compare(other);
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

SIP_PDU::SIP_PDU(Methods meth, const OpalTransportPtr & transport)
  : m_method(meth)
  , m_statusCode(IllegalStatusCode)
  , m_versionMajor(SIP_VER_MAJOR)
  , m_versionMinor(SIP_VER_MINOR)
  , m_transactionID(TransactionPrefix + OpalGloballyUniqueID().AsString())
  , m_SDP(NULL)
  , m_transport(transport)
{
  PTRACE_CONTEXT_ID_TO(m_mime);
}


SIP_PDU::SIP_PDU(const SIP_PDU & request, 
                 StatusCodes code, 
                 const SDPSessionDescription * sdp)
  : m_method(NumMethods)
  , m_statusCode(code)
  , m_SDP(sdp != NULL ? new SDPSessionDescription(*sdp) : NULL)
  , m_transport(request.GetTransport())
{
  PTRACE_CONTEXT_ID_TO(m_mime);
  InitialiseHeaders(request);
}


SIP_PDU::SIP_PDU(const SIP_PDU & pdu)
  : PSafeObject(pdu)
  , m_method(pdu.m_method)
  , m_statusCode(pdu.m_statusCode)
  , m_uri(pdu.m_uri)
  , m_versionMajor(pdu.m_versionMajor)
  , m_versionMinor(pdu.m_versionMinor)
  , m_info(pdu.m_info)
  , m_mime(pdu.m_mime)
  , m_entityBody(pdu.m_entityBody)
  , m_transactionID(pdu.m_transactionID)
  , m_SDP(pdu.m_SDP != NULL ? new SDPSessionDescription(*pdu.m_SDP) : NULL)
  , m_transport(pdu.m_transport)
{
  PTRACE_CONTEXT_ID_TO(m_mime);
}


SIP_PDU & SIP_PDU::operator=(const SIP_PDU & pdu)
{
  m_method = pdu.m_method;
  m_statusCode = pdu.m_statusCode;
  m_uri = pdu.m_uri;

  m_versionMajor = pdu.m_versionMajor;
  m_versionMinor = pdu.m_versionMinor;
  m_info = pdu.m_info;
  m_mime = pdu.m_mime;
  m_entityBody = pdu.m_entityBody;
  m_transactionID = pdu.m_transactionID;
  m_transport = pdu.m_transport;

  delete m_SDP;
  m_SDP = pdu.m_SDP != NULL ? new SDPSessionDescription(*pdu.m_SDP) : NULL;

  return *this;
}


SIP_PDU::~SIP_PDU()
{
  delete m_SDP;
}


void SIP_PDU::InitialiseHeaders(const SIPURL & dest,
                                const SIPURL & to,
                                const SIPURL & from,
                                const PString & callID,
                                unsigned cseq)
{
  m_uri = dest;
  m_uri.Sanitise(m_method == Method_REGISTER ? SIPURL::RegisterURI : SIPURL::RequestURI);

  SIPURL tmp = to;
  tmp.Sanitise(SIPURL::ToURI);
  m_mime.SetTo(tmp);

  tmp = from;
  tmp.Sanitise(SIPURL::FromURI);
  m_mime.SetFrom(tmp);

  /* This is as per EBNF in RFC 3261
       Call-ID  =  ( "Call-ID" / "i" ) HCOLON callid
       callid   =  word [ "@" word ]
     And "word" is the character set below.

     This is necessary as sometimes the code uses the Call-ID for internal
     references and we need to be able to disinguish between then, even though
     they use the same Call-ID over the wire.
  */
  static const char ValidCallID[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-.!%*_+`'~()<>:\\\"/[]?{}";
  PINDEX valid = callID.FindSpan(ValidCallID);
  if (valid != P_MAX_INDEX && callID[valid] == '@')
    valid = callID.FindSpan(ValidCallID, valid+1);
  m_mime.SetCallID(callID.Left(valid));
  m_mime.SetMaxForwards(70);
  CalculateVia();

  SetCSeq(cseq);
}


void SIP_PDU::CalculateVia()
{
  if (m_transport == NULL)
    return;

  OpalTransportAddress via = m_transport->GetLocalAddress();

  PCaselessString proto = via.GetProtoPrefix();
  PINDEX protoLen = proto.GetLength();
  PStringStream str;
  str << "SIP/" << m_versionMajor << '.' << m_versionMinor << '/' << proto.Left(protoLen-1).ToUpper() << ' ';
  PIPSocketAddressAndPort addrport(SIPURL::DefaultPort);
  if (via.GetIpAndPort(addrport))
    str << addrport;
  else
    str << via.Mid(protoLen);
  str << ";branch=" << m_transactionID << ";rport";
  m_mime.SetVia(str);
}


void SIP_PDU::InitialiseHeaders(SIPDialogContext & dialog, unsigned cseq)
{
  // Assume the dialog URI's are already sanitised.
  m_uri = dialog.GetRequestURI();
  m_mime.SetTo(dialog.GetRemoteURI());
  m_mime.SetFrom(dialog.GetLocalURI());
  m_mime.SetCallID(dialog.GetCallID());
  m_mime.SetCSeq(PString(cseq != 0 ? cseq : dialog.GetNextCSeq()) & MethodNames[m_method]);
  m_mime.SetMaxForwards(70);
  CalculateVia();
  SetRoute(dialog.GetRouteSet());
}


void SIP_PDU::InitialiseHeaders(SIPConnection & connection, unsigned cseq)
{
  InitialiseHeaders(connection.GetDialog(), cseq);
  connection.GetEndPoint().AdjustToRegistration(*this, &connection);
}


void SIP_PDU::InitialiseHeaders(const SIP_PDU & request)
{
  m_versionMajor = request.GetVersionMajor();
  m_versionMinor = request.GetVersionMinor();
  m_viaAddress = request.m_viaAddress;

  // add mandatory fields to response (RFC 2543, 11.2)
  const SIPMIMEInfo & requestMIME = request.GetMIME();
  static const char * FieldsToCopy[] = { "To", "From", "Call-ID", "CSeq", "Via", "Record-Route" };
  for (PINDEX i = 0; i < PARRAYSIZE(FieldsToCopy); ++i) {
    PString value = requestMIME.Get(FieldsToCopy[i]);
    if (!value.IsEmpty())
      m_mime.Set(FieldsToCopy[i], value);
  }

  // Update the VIA field following RFC3261, 18.2.1 and RFC3581
  PStringList viaList;
  if (!requestMIME.GetViaList(viaList))
    return;

  PString & via = viaList.front();
  PINDEX space = via.Find(' ');
  if (space == P_MAX_INDEX) {
    PTRACE(2, "SIP\tIllegal via string \"" << via << '"');
    return;
  }

  PIPSocketAddressAndPort addrport(via(space+1, via.Find(';', space)-1), SIPURL::DefaultPort);
  if (!addrport.IsValid())
    return;

  PIPSocket::Address remoteIp;
  WORD remotePort;
  if (!m_viaAddress.GetIpAndPort(remoteIp, remotePort))
    return;

  PINDEX start, val, end;
  if (LocateFieldParameter(via, "rport", start, val, end)) {
    // fill in empty rport and received for RFC 3581
    via = SIPMIMEInfo::InsertFieldParameter(via, "rport", remotePort);
    via = SIPMIMEInfo::InsertFieldParameter(via, "received", remoteIp);
  }
  else if (remoteIp != addrport.GetAddress()) // set received when remote transport address different from Via address 
    via = SIPMIMEInfo::InsertFieldParameter(via, "received", remoteIp);

  m_mime.SetViaList(viaList);
}


void SIP_PDU::SetCSeq(unsigned cseq)
{
  m_mime.SetCSeq(PString(cseq) & MethodNames[m_method]);
}


bool SIP_PDU::SetRoute(const SIPURLList & set)
{
  if (set.empty())
    return false;

  SIPURL firstRoute = set.front();
  if (firstRoute.GetParamVars().Contains("lr"))
    m_mime.SetRoute(set);
  else {
    // this procedure is specified in RFC3261:12.2.1.1 for backwards compatibility with RFC2543
    SIPURLList routeSet = set;
    routeSet.erase(routeSet.begin());
    routeSet.push_back(m_uri.AsString());
    m_uri = firstRoute;
    m_uri.Sanitise(SIPURL::RouteURI);
    m_mime.SetRoute(routeSet);
  }

  return true;
}


bool SIP_PDU::SetRoute(const SIPURL & proxy)
{
  if (proxy.IsEmpty())
    return false;

  PStringStream str;
  str << "<sip:" << proxy.GetHostName() << ':'  << proxy.GetPort() << ";lr>";
  m_mime.SetRoute(str);
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
  
  m_mime.SetAllow(str);
}


bool SIP_PDU::SendResponse(StatusCodes code)
{
  SIP_PDU response(*this, code);
  return response.Send();
}


void SIP_PDU::PrintOn(ostream & strm) const
{
  strm << m_mime.GetCSeq() << ' ';
  if (m_method != NumMethods)
    strm << m_uri;
 else if (m_statusCode != IllegalStatusCode)
    strm << '<' << (unsigned)m_statusCode << '>';
  else
    strm << "<<Uninitialised>>";
}


SIP_PDU::StatusCodes SIP_PDU::Read()
{
  if (PAssertNULL(m_transport) == NULL)
    return Local_TransportLost;

  if (!m_transport->IsOpen()) {
    PTRACE(1, "SIP\tAttempt to read PDU from closed transport " << *m_transport);
    return Local_TransportLost;
  }

  StatusCodes status;

  if (m_transport->IsReliable()) {
    status = Parse(*m_transport->GetChannel(), false);
    PTRACE_IF(2, status == SIP_PDU::Local_TransportLost,
              "SIP\tReliable transport lost to " << *m_transport <<
              " - " << m_transport->GetErrorText(PChannel::LastReadError));
    return status;
  }

  PBYTEArray pdu;
  bool truncated = false;
  if (!m_transport->ReadPDU(pdu)) {
    if (pdu.IsEmpty()) {
      PTRACE(1, "SIP\tPDU Read failed: " << m_transport->GetErrorText(PChannel::LastReadError));
      return Local_TransportError;
    }
    truncated = true;
  }

  PStringStream datagram;
  datagram = PString(pdu);
  status = Parse(datagram, truncated);

  PTRACE_IF(2, status == Local_TransportLost,
            "SIP\tInvalid datagram from " << m_transport->GetLastReceivedAddress() <<
            " - " << pdu.GetSize() << " bytes:\n" << hex << setprecision(2) << pdu << dec);

  if (m_method == NumMethods)
    return status;

  // If a command coming in, figure out where reply goes. Sometimes via, sometimes where it came from.
  OpalTransportAddress receivedAddress = m_transport->GetLastReceivedAddress();

  // Set the default return address for the response, must send back to Via address
  PString via = m_mime.GetFirstVia();
  PINDEX space = via.Find(' ');
  if (via.IsEmpty() || space == P_MAX_INDEX) {
    // Illegal via, send reply back from whence it came
    m_viaAddress = receivedAddress;
    return status;
  }

  // get the protocol type from Via header
  PINDEX pos;
  PString proto;
  if ((pos = via.FindLast('/', space)) != P_MAX_INDEX)
    proto = via(pos+1, space-1).ToLower();

  // Parse the via address
  PIPSocketAddressAndPort addrport(SIPURL::DefaultPort);
  addrport.Parse(via(space+1, via.Find(';', space)-1));

  // Make OpalTransportAddress out the bits in Via
  OpalTransportAddress viaAddress(addrport.AsString(), 5060, proto);

  // Now see if we are UDP, have rport, and received addr is different to Via,
  // which means use actual received address not Via, NAT!
  PINDEX start, end;
  if (receivedAddress.GetProtoPrefix() != OpalTransportAddress::UdpPrefix() ||
        !LocateFieldParameter(via, "rport", start, pos, end) || receivedAddress == viaAddress) {
    m_viaAddress = viaAddress;
    return status;
  }

  // Is UDP and has rport, send reply back from whence it came, fill in received elsewhere
  m_externalTransportAddress = m_viaAddress = receivedAddress;
  PTRACE(4, "SIP", "Detected rport, using " << receivedAddress << " as reply address");
  return status;
}


SIP_PDU::StatusCodes SIP_PDU::Parse(istream & stream, bool truncated)
{
#if PTRACING
  PStringStream transportName;
  if (m_transport != NULL)
    transportName << " from " << m_transport->GetLastReceivedAddress() << " on " << *m_transport;
#endif

  // get the message from transport/datagram into cmd and parse MIME
  PString cmd;
  stream >> cmd >> m_mime;

  if (stream.bad() || stream.fail() || cmd.IsEmpty() || m_mime.IsEmpty()) {
    if (cmd.IsEmpty() && m_mime.IsEmpty()) {
      if (!stream.good())
        return Local_TransportLost;

      PTRACE(5, "SIP\tProbable keep-alive" << transportName);
      return Local_KeepAlive;
    }

    PTRACE(1, "SIP\tInvalid message from" << transportName
           << ", request \"" << cmd << "\", mime:\n" << m_mime);
    return SIP_PDU::Failure_BadRequest;
  }

  if (cmd.Left(4) *= "SIP/") {
    // parse Response version, code & reason (ie: "SIP/2.0 200 OK")
    PINDEX space = cmd.Find(' ');
    if (space == P_MAX_INDEX) {
      PTRACE(2, "SIP\tBad Status-Line \"" << cmd << "\" received" << transportName);
      return SIP_PDU::Failure_BadRequest;
    }

    m_versionMajor = cmd.Mid(4).AsUnsigned();
    m_versionMinor = cmd(cmd.Find('.')+1, space).AsUnsigned();
    m_statusCode = (StatusCodes)cmd.Mid(++space).AsUnsigned();
    m_info = cmd.Mid(cmd.Find(' ', space));
    m_uri = PString::Empty();
  }
  else {
    // parse the method, URI and version
    PStringArray cmds = cmd.Tokenise( ' ', false);
    if (cmds.GetSize() < 3) {
      PTRACE(2, "SIP\tBad Request-Line \"" << cmd << "\" received" << transportName);
      return SIP_PDU::Failure_BadRequest;
    }

    int i = 0;
    while (!(cmds[0] *= MethodNames[i])) {
      i++;
      if (i >= NumMethods) {
        PTRACE(2, "SIP\tUnknown method name " << cmds[0] << " received" << transportName);
        return SIP_PDU::Failure_BadRequest;
      }
    }
    m_method = (Methods)i;

    m_uri = cmds[1];
    m_versionMajor = cmds[2].Mid(4).AsUnsigned();
    m_versionMinor = cmds[2].Mid(cmds[2].Find('.')+1).AsUnsigned();
    m_info.MakeEmpty();
  }

  if (m_versionMajor < 2) {
    PTRACE(2, "SIP\tInvalid version (" << m_versionMajor << ") received" << transportName);
    return SIP_PDU::Failure_BadRequest;
  }

  // get the SDP content body
  // if a content length is specified, read that length
  // if no content length is specified (which is not the same as zero length)
  // then read until end of datagram or stream
  int contentLength = m_mime.GetContentLength();
  bool contentLengthPresent = m_mime.IsContentLengthPresent();

  if (!contentLengthPresent) {
    PTRACE(2, "SIP\tNo Content-Length present" << transportName << ", reading till end of datagram/stream.");
  }
  else if (contentLength < 0) {
    PTRACE(2, "SIP\tImpossible negative Content-Length" << transportName << ", reading till end of datagram/stream.");
    contentLengthPresent = false;
  }
  else if (contentLength > 65535) {
    PTRACE(2, "SIP\tImplausibly long Content-Length " << contentLength << " received" << transportName << ", reading to end of datagram/stream.");
    contentLengthPresent = false;
  }

  // Don't worry about body if was truncated packet
  if (!truncated) {
    if (contentLengthPresent) {
      if (contentLength > 0) {
        stream.read(m_entityBody.GetPointerAndSetLength(contentLength), contentLength);
        if (stream.gcount() != (std::streamsize)contentLength)
          truncated = true;
      }
    }
    else {
      contentLength = 0;
      int c;
      while ((c = stream.get()) != EOF) {
        m_entityBody.SetMinSize((++contentLength/1000+1)*1000);
        m_entityBody += (char)c;
      }
    }

    m_entityBody[contentLength] = '\0';
  }

#if PTRACING
  if (PTrace::CanTrace(3)) {
    ostream & trace = PTrace::Begin(3, __FILE__, __LINE__, this);

    trace << "SIP\t";
    if (truncated)
      trace << "Truncated (EMSGSIZE) ";
    trace << "PDU ";

    if (!PTrace::CanTrace(4)) {
      if (m_method != NumMethods)
        trace << MethodNames[m_method] << ' ' << m_uri;
      else
        trace << (unsigned)m_statusCode << ' ' << m_info;
      trace << ' ';
    }

    if (m_transport != NULL)
      trace << "received: rem=" << m_transport->GetLastReceivedAddress()
            << ",local=" << m_transport->GetLocalAddress()
            << ",if=" << m_transport->GetLastReceivedInterface();

    if (PTrace::CanTrace(4)) {
      trace << '\n' << cmd << '\n' << setfill('\n') << m_mime << setfill(' ');
      for (const char * ptr = m_entityBody; *ptr != '\0'; ++ptr) {
        if (*ptr != '\r')
          trace << *ptr;
      }
    }
    if (truncated && contentLength > 0)
      trace << "... truncated";

    trace << PTrace::End;
  }
#endif

  if (truncated)
    return SIP_PDU::Failure_MessageTooLarge;
  
  /* RFC3261 Sections 8.1.1.7 & 17.1.3 transactions are identified by the
      branch paranmeter in the top most Via and CSeq. We do NOT include the
      CSeq in our id as we want the CANCEL messages directed at out
      transaction structure.
    */
  m_transactionID = SIPMIMEInfo::ExtractFieldParameter(m_mime.GetFirstVia(), "branch");
  if (m_transactionID.NumCompare(TransactionPrefix) != EqualTo) {
    PTRACE(2, "SIP\tTransaction " << m_mime.GetCSeq() << " has "
            << (m_transactionID.IsEmpty() ? "no" : "RFC2543") << " branch parameter!");

    SIPURL to(m_mime.GetTo());
    to.Sanitise(SIPURL::ToURI);

    SIPURL from(m_mime.GetFrom());
    from.Sanitise(SIPURL::FromURI);

    PStringStream strm;
    strm << to << from << m_mime.GetCallID() << m_mime.GetCSeq();
    m_transactionID += strm;
  }

  return SIP_PDU::Successful_OK;
}


bool SIP_PDU::Send()
{
  return InternalSend(false) == Successful_OK;
}


SIP_PDU::StatusCodes SIP_PDU::InternalSend(bool canDoTCP)
{
  if (PAssertNULL(m_transport) == NULL)
    return Local_TransportError;

  PSafeLockReadWrite mutex(*m_transport);

  if (!m_transport->IsOpen()) {
    PTRACE(1, "SIP\tAttempt to write PDU to closed transport " << *m_transport);
    return Local_TransportError;
  }

  SIPEndPoint & endpoint = dynamic_cast<SIPEndPoint &>(m_transport->GetEndPoint());

  if (m_method == NumMethods)
    endpoint.AdjustToRegistration(*this, NULL, m_transport);

  PString pduStr;
  PINDEX pduLen;

  m_mime.SetCompactForm(false);
  Build(pduStr, pduLen);

  // RFC3261 18.1.1 specifies maximum PDU size of 1300 bytes.
  if (!m_transport->IsReliable()) {
    if (pduLen > endpoint.GetMaxPacketSizeUDP()) {
      m_mime.SetCompactForm(true);
      Build(pduStr, pduLen);
      if (canDoTCP && pduLen > endpoint.GetMaxPacketSizeUDP()) {
        PTRACE(2, "SIP\tPDU is too large (" << pduLen << " bytes) for UDP datagram.");
        return Failure_MessageTooLarge;
      }
      PTRACE(4, "SIP\tPDU is too large (" << pduLen << " bytes) using compact form.");
    }

    if (!m_viaAddress.IsEmpty())
      m_transport->SetRemoteAddress(m_viaAddress);
  }

#if PTRACING
  if (PTrace::CanTrace(3)) {
    ostream & trace = PTrace::Begin(3, __FILE__, __LINE__, this);

    trace << "SIP\tSending PDU ";

    if (!PTrace::CanTrace(4)) {
      if (m_method != NumMethods)
        trace << MethodNames[m_method] << ' ' << m_uri;
      else
        trace << (unsigned)m_statusCode << ' ' << m_info;
      trace << ' ';
    }

    trace << '(' << pduLen << " bytes) to: "
             "rem=" << m_transport->GetRemoteAddress() << ","
             "local=" << m_transport->GetLocalAddress() << ","
             "if=" << m_transport->GetInterface();

    if (PTrace::CanTrace(4)) {
      trace << '\n';
      for (const char * ptr = pduStr; *ptr != '\0'; ++ptr) {
        if (*ptr != '\r')
          trace << *ptr;
      }
    }

    trace << PTrace::End;
  }
#endif

  if (m_transport->Write((const char *)pduStr, pduLen))
    return Successful_OK;

  PTRACE(1, "SIP\tPDU Write failed: " << m_transport->GetErrorText(PChannel::LastWriteError));
  return Local_TransportError;
}


void  SIP_PDU::SetEntityBody()
{
  if (m_SDP != NULL && m_entityBody.IsEmpty()) {
    m_entityBody = m_SDP->Encode();
    m_mime.SetContentType("application/sdp");
  }

  m_mime.SetContentLength(m_entityBody.GetLength());
}


void SIP_PDU::Build(PString & pduStr, PINDEX & pduLen)
{
  PStringStream strm;

  SetEntityBody();

  if (m_method != NumMethods)
    strm << MethodNames[m_method] << ' ' << m_uri << ' ';

  strm << "SIP/" << m_versionMajor << '.' << m_versionMinor;

  if (m_method == NumMethods) {
    if (m_info.IsEmpty())
      m_info = GetStatusCodeDescription(m_statusCode);
    strm << ' ' << (unsigned)m_statusCode << ' ' << m_info;
  }

  strm << "\r\n" << setfill('\r') << m_mime << m_entityBody;

  pduStr = strm;
  pduLen = strm.GetLength();
}


bool SIP_PDU::DecodeSDP(const OpalMediaFormatList & localList)
{
  if (m_SDP != NULL)
    return true;

  if (m_entityBody.IsEmpty())
    return false;

  if (m_mime.GetContentType() != "application/sdp")
    return false;

  m_SDP = new SDPSessionDescription(0, 0, OpalTransportAddress());
  PTRACE_CONTEXT_ID_TO(m_SDP);
  if (m_SDP->Decode(m_entityBody, localList.IsEmpty() ? OpalMediaFormat::GetAllRegisteredMediaFormats() : localList))
    return true;

  delete m_SDP;
  m_SDP = NULL;
  return false;
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
  , m_forking(false)
{
}


static void SetWithTag(const SIPURL & url, SIPURL & uri, PString & tag, bool local)
{
  uri = url;

  PString newTag = url.GetParamVars()(TagStr);
  if (newTag.IsEmpty())
    newTag = uri.GetFieldParameters().Get(TagStr);
  else
    uri.SetParamVar(TagStr, PString::Empty());

  if (!newTag.IsEmpty() && tag != newTag) {
    PTRACE(4, "SIP\tUpdating dialog tag from \"" << tag << "\" to \"" << newTag << '"');
    tag = newTag;
  }

  if (tag.IsEmpty() && local)
    tag = SIPURL::GenerateTag();

  if (!tag.IsEmpty())
    uri.GetFieldParameters().Set(TagStr, tag);

  uri.Sanitise(local ? SIPURL::FromURI : SIPURL::ToURI);
}


SIPDialogContext::SIPDialogContext(const SIPMIMEInfo & mime)
  : m_callId(mime.GetCallID())
  , m_requestURI(mime.GetContact())
  , m_lastSentCSeq(0)
  , m_lastReceivedCSeq(mime.GetCSeqIndex())
  , m_forking(false)
{
  SetWithTag(mime.GetTo(), m_localURI, m_localTag, false);
  SetWithTag(mime.GetFrom(), m_remoteURI, m_remoteTag, false);

  mime.GetRecordRoute(m_routeSet, true);
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
  url.SetParamVar("interface", m_interface);

  unsigned index = 0;
  for (SIPURLList::const_iterator it = m_routeSet.begin(); it != m_routeSet.end(); ++it)
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
  m_interface = params("interface");

  PString route;
  unsigned index = 0;
  while (!(route = params(psprintf("route-set-%u", ++index))).IsEmpty())
    m_routeSet.push_back(route);

  return IsEstablished();
}


void SIPDialogContext::SetRequestURI(const SIPURL & url)
{
  m_requestURI = url;
  m_requestURI.Sanitise(SIPURL::RequestURI);
}


void SIPDialogContext::SetLocalURI(const SIPURL & url)
{
  SetWithTag(url, m_localURI, m_localTag, true);
}


void SIPDialogContext::SetRemoteURI(const SIPURL & url)
{
  SetWithTag(url, m_remoteURI, m_remoteTag, false);
}


void SIPDialogContext::SetProxy(const SIPURL & proxy, bool addToRouteSet)
{
  if (!proxy.IsEmpty()) {
    PTRACE(3, "SIP\tOutbound proxy for dialog set to " << proxy);

    m_proxy = proxy;  

    // Default routeSet if there is a proxy
    if (addToRouteSet && m_routeSet.empty()) {
      SIPURL route = proxy;
      route.SetParamVar("lr", PString::Empty(), false); // Always be loose
      route.Sanitise(SIPURL::RouteURI);
      m_routeSet.push_back(route);
    }
  }
}


void SIPDialogContext::Update(const SIP_PDU & pdu)
{
  const SIPMIMEInfo & mime = pdu.GetMIME();

  SetCallID(mime.GetCallID());

  if (m_routeSet.empty() && mime.Has("Record-Route")) {
    // get the route set from the Record-Route response field according to 12.1.2
    // requests in a dialog do not modify the initial route set according to 12.2
    m_routeSet.FromString(mime.GetRecordRoute(), SIPURL::RouteURI, pdu.GetMethod() == SIP_PDU::NumMethods);
    PTRACE(4, "SIP\tRoute set is " << m_routeSet.ToString());
  }

  /* Update request URI
     Do this only if no Route and no Record-Route header is set.
     Otherwise we will send SIP Messages to the proxy.
     Problems found if Contact contains transport=tcp.
     OPAL tries to switch to tcp even if there is an upp proxy between.
     TCP/UDP Problems conversions of a proxy are not solved with this work around.
     See RFC 5658 for a description of these problems.
  */
  if (m_requestURI.IsEmpty() || pdu.GetMethod() != SIP_PDU::NumMethods || pdu.GetStatusCode()/100 == 2) {
    SIPURL contact = mime.GetContact();
    if (!contact.IsEmpty()) {
      m_requestURI = contact;
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

  m_externalTransportAddress = pdu.GetExternalTransportAddress();
  m_interface = pdu.GetTransport()->GetInterface();
}


unsigned SIPDialogContext::GetNextCSeq()
{
  if (m_forking && m_lastSentCSeq > 0)
    return m_lastSentCSeq;

  return ++m_lastSentCSeq;
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
  PTRACE_IF(2, !duplicate && m_lastReceivedCSeq != 0 && requestCSeq != m_lastReceivedCSeq+1,
            "SIP\tReceived unexpected sequence number " << requestCSeq << ", expecting " << m_lastReceivedCSeq+1);

  m_lastReceivedCSeq = requestCSeq;

  return duplicate;
}


OpalTransportAddress SIPDialogContext::GetRemoteTransportAddress(PINDEX dnsEntry) const
{
  // In order of priority ...

  if (!m_externalTransportAddress.IsEmpty()) {
    PTRACE(4, "SIP\tRemote dialog address external (NAT): " << m_externalTransportAddress);
    return m_externalTransportAddress;
  }

  OpalTransportAddress addr;
  if (!m_proxy.IsEmpty()) {
    addr = m_proxy.GetTransportAddress(dnsEntry);
    PTRACE(4, "SIP\tRemote dialog address proxied: " << addr);
  }
  else if (m_routeSet.empty()) {
    addr = m_requestURI.GetTransportAddress(dnsEntry);
    PTRACE(4, "SIP\tRemote dialog address from target: " << addr);
  }
  else {
    addr = m_routeSet.front().GetTransportAddress(dnsEntry);
    PTRACE(4, "SIP\tRemote dialog address from route set: " << addr);
  }

  return addr;
}


////////////////////////////////////////////////////////////////////////////////////

SIPTransactionOwner::SIPTransactionOwner(PSafeObject & object, SIPEndPoint & endpoint)
  : m_object(object)
  , m_endpoint(endpoint)
  , m_dnsEntry(0)
  , m_authentication(NULL)
  , m_authenticateErrors(0)
{
  m_transactions.DisallowDeleteObjects();
}


SIPTransactionOwner::~SIPTransactionOwner()
{
  delete m_authentication;
}


SIP_PDU::StatusCodes SIPTransactionOwner::SwitchTransportProto(const char * proto, OpalTransportPtr * transport)
{
  SIPURL newTransportAddress(m_dialog.GetRequestURI());
  newTransportAddress.SetParamVar("transport", proto);
  m_dialog.SetRequestURI(newTransportAddress);

  PTRACE(4, "SIP\tSetting new transport for destination \"" << newTransportAddress << '"');

  SIP_PDU::StatusCodes reason;
  OpalTransport * newTransport = GetEndPoint().GetTransport(*this, reason);
  if (newTransport == NULL)
    return reason;

  PTRACE_CONTEXT_ID_SET(*newTransport, m_object);

  if (transport != NULL)
    *transport = newTransport;

  return SIP_PDU::Successful_OK;
}


unsigned SIPTransactionOwner::GetAllowedMethods() const
{
  return m_endpoint.GetAllowedMethods();
}


void SIPTransactionOwner::FinaliseForking(SIPTransaction & transaction, SIP_PDU & response)
{
  if (response.GetStatusCode() == SIP_PDU::Information_Trying)
    return;

  // Take this transaction out of list
  if (!m_transactions.Remove(&transaction))
    return;

  OpalTransportPtr transport = transaction.GetTransport();
  if (PAssertNULL(transport) == NULL)
    return;

  m_forkMutex.Wait();

  // Finally end connect mode on the transport
  PString localInterface = transaction.GetInterface();
  if (!localInterface.IsEmpty()) {
    transport->SetInterface(localInterface);
    m_dialog.SetInterface(localInterface);
    PTRACE(4, "SIP\tSet local interface to " << localInterface);
  }

  m_forkMutex.Signal();

  // About any transactions on a different interface, they are forks that have
  // been resolved.
  AbortPendingTransactions(false);
}


void SIPTransactionOwner::OnReceivedResponse(SIPTransaction & transaction, SIP_PDU & response)
{
  FinaliseForking(transaction, response);

  unsigned responseClass = response.GetStatusCode()/100;

  // If 2xx response, kill any forked transactions, have the definitive answer.
  if (responseClass == 2)
    AbortPendingTransactions();

  // Then tell endpoint - backward compatibility API
  m_endpoint.OnReceivedResponse(transaction, response);

  // Ignore the pending responses
  if (responseClass == 1)
    return;

  // Handling of specific responses
  switch (response.GetStatusCode()) {
    case SIP_PDU::Failure_UnAuthorised :
    case SIP_PDU::Failure_ProxyAuthenticationRequired :
      break;

    default :
      m_authenticateErrors = 0;
  }
}


void SIPTransactionOwner::OnTransactionFailed(SIPTransaction & transaction)
{
  PTRACE(4, "SIP\tOnTransactionFailed for transaction id=" << transaction.GetTransactionID());

  m_endpoint.OnTransactionFailed(transaction);
}


bool SIPTransactionOwner::CleanPendingTransactions()
{
  /* Note we wait for various transactions to complete as the transport they
     rely on may be owned by the connection, and would be deleted once we exit
     from OnRelease() causing a crash in the transaction processing. */
  PSafePtr<SIPTransaction> transaction;
  while ((transaction = m_transactions.GetAt(0, PSafeReference)) != NULL) {
    if (!transaction->IsTerminated())
      return false;

    PTRACE(4, "SIP\tRemoved terminated transaction, id=" << transaction->GetTransactionID());
    m_transactions.Remove(transaction);
  }

  return true;
}


void SIPTransactionOwner::AbortPendingTransactions(bool all)
{
  PTRACE_IF(3, !m_transactions.IsEmpty(), "SIP\tCancelling/Aborting " << m_transactions.GetSize() << " transactions.");

  PSafePtr<SIPTransaction> transaction;
  while ((transaction = m_transactions.GetAt(0)) != NULL) {
    if (all || transaction->GetInterface() != GetInterface()) {
      if (transaction->IsTrying())
        transaction->Abort();
      else
        transaction->Cancel();
    }
    m_transactions.Remove(transaction);
  }
}


SIP_PDU::StatusCodes SIPTransactionOwner::StartTransaction(const OpalTransport::WriteConnectCallback & function)
{
  SIP_PDU::StatusCodes reason = SIP_PDU::Local_BadTransportAddress;
  OpalTransportPtr transport = GetEndPoint().GetTransport(*this, reason);
  if (transport == NULL)
    return reason;

  PTRACE_CONTEXT_ID_SET(transport, function);

  reason = SIP_PDU::Local_TransportError;

  if (GetInterface().IsEmpty()) {
    // Restoring or first time, try every interface
    if (transport->WriteConnect(function))
      reason = SIP_PDU::Successful_OK;
  }
  else {
    m_forkMutex.Wait();

    // We contacted the server on an interface last time, assume it still works!
    bool succeeded = false;
    function(*transport, succeeded);
    if (succeeded)
      reason = SIP_PDU::Successful_OK;

    m_forkMutex.Signal();
  }

  return reason;
}


SIP_PDU::StatusCodes SIPTransactionOwner::HandleAuthentication(const SIP_PDU & response)
{
  bool isProxy = response.GetStatusCode() == SIP_PDU::Failure_ProxyAuthenticationRequired;

#if PTRACING
  const char * proxyTrace = isProxy ? "Proxy " : "";
#endif

  PTRACE(3, "SIP\tReceived " << proxyTrace << "Authentication Required response for " << response.GetTransactionID());

  // authenticate 
  PString errorMsg;
  SIPAuthentication * newAuthentication = PHTTPClientAuthentication::ParseAuthenticationRequired(isProxy, response.GetMIME(), errorMsg);
  if (newAuthentication == NULL) {
    PTRACE(2, "SIP\t" << proxyTrace << " Authentication error: " << errorMsg);
    return SIP_PDU::Failure_Forbidden;
  }

  PString realm = newAuthentication->GetAuthRealm();
  PString authId = GetAuthID();
  PString password = GetPassword();

  if (authId.IsEmpty() || password.IsEmpty())
    m_endpoint.GetAuthentication(realm, authId, password);

  // Still need stuff, check proxy
  if (authId.IsEmpty() || password.IsEmpty()) {
    SIPURL proxy = GetProxy();
    if (proxy.IsEmpty())
      proxy = GetProxy();
    if (!proxy.IsEmpty()) {
      PTRACE (3, "SIP\tNo auth info for realm \"" << realm << "\", using proxy auth");
      authId = proxy.GetUserName();
      password = proxy.GetPassword();
    }
  }

  if (authId.IsEmpty() || password.IsEmpty()) {
    PTRACE(2, "SIP\tAuthentication not possible yet, no credentials available.");
    return SIP_PDU::Failure_UnAuthorised;
  }

  newAuthentication->SetUsername(authId);
  newAuthentication->SetPassword(password);

  // Only check for same credentials if same request (via CSeq)
  if (m_authenticateErrors > 1 && m_authentication != NULL && *newAuthentication == *m_authentication) {
    PTRACE(1, "SIP\tAuthentication already performed using current credentials, not trying again.");
    return SIP_PDU::Failure_UnAuthorised;
  }

  PTRACE(4, "SIP\t" << (m_authentication != NULL ? "Upd" : "Cre") << "ating"
            " authentication credentials of user \"" << authId << "\" for realm \"" << realm << '"');

  // switch authentication schemes
  delete m_authentication;
  m_authentication = newAuthentication;
  ++m_authenticateErrors;
  return SIP_PDU::Successful_OK;
}


////////////////////////////////////////////////////////////////////////////////////

PObject::Comparison SIPTransactionBase::Compare(const PObject & other) const
{
  return GetTransactionID().Compare(dynamic_cast<const SIPTransactionBase&>(other).GetTransactionID());
}


////////////////////////////////////////////////////////////////////////////////////

SIPTransaction::SIPTransaction(Methods method, SIPTransactionOwner * owner, OpalTransport * transport, bool deleteOwner)
  : SIPTransactionBase(method)
  , m_owner(PAssertNULL(owner))
  , m_deleteOwner(deleteOwner)
  , m_retryTimeoutMin(GetEndPoint().GetRetryTimeoutMin())
  , m_retryTimeoutMax(GetEndPoint().GetRetryTimeoutMax())
  , m_state(NotStarted)
  , m_retry(1)
  , m_retryTimer(GetEndPoint().GetThreadPool(), GetEndPoint(), GetTransactionID(), &SIPTransaction::OnRetry)
  , m_completionTimer(GetEndPoint().GetThreadPool(), GetEndPoint(), GetTransactionID(), &SIPTransaction::OnTimeout)
  , m_pduSizeOK(true)
{
  PTRACE_CONTEXT_ID_FROM(m_owner->m_object);

  PAssert(m_owner->m_object.SafeReference(), "Transaction created on owner pending deletion.");

  if (transport != NULL)
    m_transport = transport;
  else {
    SIP_PDU::StatusCodes reason;
    m_transport = GetEndPoint().GetTransport(*m_owner, reason);
  }

  SIPConnection * conn = GetConnection();
  if (conn == NULL)
    m_mime.SetProductInfo(GetEndPoint().GetUserAgent(), GetEndPoint().GetProductInfo());
  else {
    m_mime.SetProductInfo(GetEndPoint().GetUserAgent(), conn->GetProductInfo());
    InitialiseHeaders(*conn);
  }

  PTRACE(4, "SIP\t" << method << " transaction id=" << GetTransactionID() << " created.");
}


SIPTransaction::~SIPTransaction()
{
  if (m_state < Terminated_Success) {
    PTRACE(1, "SIP\tDestroying transaction id="
           << GetTransactionID() << " which is not yet terminated.");
    m_state = Terminated_Aborted;
  }

  // Stop timers here so happens before the below trace log,
  // and not after it, if we wait for ~PTimer()
  m_retryTimer.Stop();
  m_completionTimer.Stop();

  m_owner->m_object.SafeDereference();
  if (m_deleteOwner)
    delete m_owner;

  PTRACE(4, "SIP\tTransaction id=" << GetTransactionID() << " destroyed.");
}


SIPConnection * SIPTransaction::GetConnection() const
{
  return dynamic_cast<SIPConnection *>(m_owner);
}


void SIPTransaction::SetParameters(const SIPParameters & params)
{
  if (params.m_minRetryTime != PMaxTimeInterval)
    m_retryTimeoutMin = params.m_minRetryTime;
  if (params.m_maxRetryTime != PMaxTimeInterval)
    m_retryTimeoutMax = params.m_maxRetryTime;

  m_mime.SetExpires(params.m_expire);

  if (!params.m_contactAddress.IsEmpty())
    m_mime.SetContact(params.m_contactAddress);

  if (!params.m_proxyAddress.IsEmpty())
    SetRoute(params.m_proxyAddress);

  m_mime.AddMIME(params.m_mime);
}


bool SIPTransaction::Start()
{
  PSafeLockReadWrite lock(*this);

  if (!PAssert(m_state == NotStarted, PLogicError))
    return false;

  GetEndPoint().AddTransaction(this);

  m_state = Trying;
  m_retry = 0;

  m_owner->m_transactions.Append(this);
  m_owner->OnStartTransaction(*this);

  if (m_owner->GetAuthenticator() != NULL) {
    SIPAuthenticator auth(*this);
    m_owner->GetAuthenticator()->Authorise(auth);
  }

  if (m_transport == NULL) {
    SetTerminated(Terminated_TransportError);
    return false;
  }

  // Remember this for when we are forking on UDP
  m_localInterface = m_transport->GetInterface();

  m_pduSizeOK = true;
  for (;;) {
    // Use the transactions transport to send the request
    switch (InternalSend(m_pduSizeOK)) {
      case Successful_OK :
        if (!m_transport->IsReliable())
          m_retryTimer = m_retryTimeoutMin;
        if (m_method == Method_INVITE)
          m_completionTimer = GetEndPoint().GetInviteTimeout();
        else
          m_completionTimer = GetEndPoint().GetNonInviteTimeout();

        PTRACE(4, "SIP\tTransaction timers set: retry=" << m_retryTimer << ", completion=" << m_completionTimer);
        return true;

      case Failure_MessageTooLarge : // Can only occur if UDP && canDoTCP
        // switch to TCP as per RFC3261 18.1.1
        if (m_owner->SwitchTransportProto("tcp", &m_transport) != Successful_OK) {
          // Could not connect using TCP, go back to UDP and send it regardless of size
          if (m_owner->SwitchTransportProto("udp", &m_transport) != Successful_OK)
            return false; // Huh? We where able to a moment ago!
        }

        CalculateVia();
        m_pduSizeOK = false;
        break;

      default :
        SetTerminated(Terminated_TransportError);
        return false;
    }
  }
}


void SIPTransaction::WaitForCompletion()
{
  if (IsCompleted())
    return;

  if (m_state == NotStarted) {
    if (!Start())
      return;
  }

  PTRACE(4, "SIP\tAwaiting completion of " << GetMethod() << " transaction id=" << GetTransactionID());
  m_completed.Wait();
}


PBoolean SIPTransaction::Cancel()
{
  PSafeLockReadWrite lock(*this);

  if (m_state == NotStarted || m_state >= Cancelling) {
    PTRACE(3, "SIP\t" << GetMethod() << " transaction id=" << GetTransactionID() << " cannot be cancelled as in state " << m_state);
    return false;
  }

  PTRACE(4, "SIP\t" << GetMethod() << " transaction id=" << GetTransactionID() << " cancelled.");
  m_state = Cancelling;
  m_retry = 0;
  if (!m_transport->IsReliable())
    m_retryTimer = m_retryTimeoutMin;
  m_completionTimer = GetEndPoint().GetPduCleanUpTimeout();
  return ResendCANCEL();
}


void SIPTransaction::Abort()
{
  PTRACE(4, "SIP\tAttempting to abort " << GetMethod() << " transaction id=" << GetTransactionID());

  if (LockReadWrite()) {
    if (!IsCompleted()) {
      SetTerminated(Terminated_Aborted);
    }
    UnlockReadWrite();
  }
}


bool SIPTransaction::ResendCANCEL()
{
  SIP_PDU cancel(Method_CANCEL, m_transport);
  PTRACE_CONTEXT_ID_TO(cancel);
  cancel.InitialiseHeaders(m_uri,
                           m_mime.GetTo(),
                           m_mime.GetFrom(),
                           m_mime.GetCallID(),
                           m_mime.GetCSeqIndex());
  cancel.GetMIME().SetVia(m_mime.GetFirstVia()); // Use the topmost via header from the INVITE we cancel as per 9.1. 

  return cancel.Send();
}


PBoolean SIPTransaction::OnReceivedResponse(SIP_PDU & response)
{
  m_retryTimer.Stop();

  PString cseq = response.GetMIME().GetCSeq();

  /* If is the response to a CANCEL we sent, then we stop retransmissions
     and wait for the 487 Request Terminated to come in */
  if (cseq.Find(MethodNames[Method_CANCEL]) != P_MAX_INDEX) {
    m_completionTimer = GetEndPoint().GetPduCleanUpTimeout();
    return false;
  }

  // Something wrong here, response is not for the request we made!
  if (cseq.Find(MethodNames[m_method]) == P_MAX_INDEX) {
    PTRACE(2, "SIP\tTransaction " << cseq << " response not for " << *this);
    // Restart timer as haven't finished yet
    m_retryTimer = m_retryTimer.GetResetTime();
    m_completionTimer = m_completionTimer.GetResetTime();
    return false;
  }

  PSafeLockReadWrite lock(*this);
  if (!lock.IsLocked())
    return false;

  /* Really need to check if response is actually meant for us. Have a
     temporary cheat in assuming that we are only sending a given CSeq to one
     and one only host, so anything coming back with that CSeq is OK. This has
     "issues" according to the spec but
     */
  if (IsInProgress()) {
    if (response.GetStatusCode()/100 == 1) {
      PTRACE(3, "SIP\t" << GetMethod() << " transaction id=" << GetTransactionID() << " proceeding.");

      if (m_state == Trying)
        m_state = Proceeding;

      m_retry = 0;
      if (m_method != Method_INVITE && !m_transport->IsReliable())
        m_retryTimer = m_retryTimeoutMax;
      else
        m_retryTimer = 0;

      int expiry = m_mime.GetExpires();
      if (expiry > 0)
        m_completionTimer.SetInterval(0, expiry);
      else if (m_method == Method_INVITE)
        m_completionTimer = GetEndPoint().GetProgressTimeout();
      else
        m_completionTimer = GetEndPoint().GetNonInviteTimeout();
      PTRACE(4, "SIP\tTransaction timers set: retry=" << m_retryTimer << ", completion=" << m_completionTimer);
    }
    else {
      PTRACE(4, "SIP\t" << GetMethod() << " transaction id=" << GetTransactionID() << " completing.");
      m_state = Completed;
      m_statusCode = response.GetStatusCode();
    }

    if (m_owner->m_object.LockReadWrite()) {
      m_owner->OnReceivedResponse(*this, response);
      m_owner->m_object.UnlockReadWrite();
    }

    if (m_state == Completed) {
      OnCompleted(response);
      m_completed.Signal();
      PTRACE(3, "SIP\t" << GetMethod() << " transaction id=" << GetTransactionID() << " completed.");
    }
  }
  else {
    PTRACE(4, "SIP\tIgnoring duplicate response to " << GetMethod() << " transaction id=" << GetTransactionID());
  }

  if (response.GetStatusCode() >= 200)
    m_completionTimer = GetEndPoint().GetPduCleanUpTimeout();

  return true;
}


PBoolean SIPTransaction::OnCompleted(SIP_PDU & /*response*/)
{
  return true;
}


void SIPTransaction::OnRetry()
{
  // Can do this outside mutex as never changes from this state
  if (IsTerminated())
    return;

  if (m_state > Cancelling || (m_state == Proceeding && m_method == Method_INVITE))
    return;

  m_retry++;

  if (m_retry >= GetEndPoint().GetMaxRetries()) {
    SetTerminated(Terminated_RetriesExceeded);
    return;
  }

  if (m_state > Trying)
    m_retryTimer = m_retryTimeoutMax;
  else {
    PTimeInterval timeout = m_retryTimeoutMin*(1<<m_retry);
    if (timeout > m_retryTimeoutMax)
      timeout = m_retryTimeoutMax;
    m_retryTimer = timeout;
  }

  PTRACE(3, "SIP\t" << GetMethod() << " transaction id=" << GetTransactionID()
         << " timeout, making retry " << m_retry << ", timeout " << m_retryTimer << ", state " << m_state);

  if (m_state == Cancelling)
    ResendCANCEL();
  else
    Send();
}


void SIPTransaction::OnTimeout()
{
  // Can do this outside mutex as never changes from this state
  if (IsTerminated())
    return;

  switch (m_state) {
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


void SIPTransaction::SetTerminated(States newState)
{
  if (!PAssert(newState >= Terminated_Success, PInvalidParameter))
    return;

  // Terminated, so finished with timers
  m_retryTimer.Stop();
  m_completionTimer.Stop();

  m_owner->m_transactions.Remove(this);

  if (m_state >= Terminated_Success) {
    PTRACE_IF(3, newState != Terminated_Success, "SIP\tTried to set state " << newState 
              << " for " << GetMethod() << " transaction id=" << GetTransactionID()
              << " but already terminated ( " << m_state << ')');
    return;
  }

  m_state = newState;
  PTRACE(3, "SIP\tSet state " << newState << " for "
         << GetMethod() << " transaction id=" << GetTransactionID());

  // Transaction failed, tell the endpoint
  if (m_state > Terminated_Success) {
    switch (m_state) {
      case Terminated_Timeout :
      case Terminated_RetriesExceeded:
        m_statusCode = m_pduSizeOK ? SIP_PDU::Local_Timeout : SIP_PDU::Failure_MessageTooLarge;
        break;

      case Terminated_TransportError :
        m_statusCode = SIP_PDU::Local_TransportError;
        break;

      case Terminated_Aborted :
      case Terminated_Cancelled :
        m_statusCode = SIP_PDU::Failure_RequestTerminated;
        break;

      default :// Prevent GNU warning
        break;
    }

    m_owner->OnTransactionFailed(*this);
  }

  m_completed.Signal();
  PTRACE(4, "SIP\tCompleted state for transaction id=" << GetTransactionID());
}


PString SIPTransaction::GenerateCallID()
{
  return PGloballyUniqueID().AsString() + '@' + PIPSocket::GetHostName();
}


////////////////////////////////////////////////////////////////////////////////////

SIPParameters::SIPParameters(const PString & aor, const PString & remote)
  : m_remoteAddress(remote)
  , m_addressOfRecord(aor)
  , m_expire(0)
  , m_restoreTime(30)
  , m_minRetryTime(PMaxTimeInterval)
  , m_maxRetryTime(PMaxTimeInterval)
  , m_userData(NULL)
{
}


void SIPParameters::Normalise(const PString & defaultUser, const PTimeInterval & defaultExpire)
{
  /* Try to be intelligent about what we got in the two fields target/remote,
     we have several scenarios depending on which is a partial or full URL.
   */

  SIPURL aor, server;
  PString possibleProxy;

  if (m_addressOfRecord.IsEmpty()) {
    if (m_remoteAddress.IsEmpty())
      aor = server = defaultUser + '@' + PIPSocket::GetHostName();
    else if (m_remoteAddress.Find('@') == P_MAX_INDEX)
      aor = server = defaultUser + '@' + m_remoteAddress;
    else
      aor = server = m_remoteAddress;
  }
  else if (m_addressOfRecord.Find('@') == P_MAX_INDEX) {
    if (m_remoteAddress.IsEmpty())
      aor = server = defaultUser + '@' + m_addressOfRecord;
    else if (m_remoteAddress.Find('@') == P_MAX_INDEX)
      aor = server = m_addressOfRecord + '@' + m_remoteAddress;
    else {
      server = m_remoteAddress;
      aor = m_addressOfRecord + '@' + server.GetHostName();
    }
  }
  else {
    aor = m_addressOfRecord;
    if (m_remoteAddress.IsEmpty())
      server = aor;
    else if (m_remoteAddress.Find('@') != P_MAX_INDEX)
      server = m_remoteAddress; // For third party registrations
    else {
      SIPURL remoteURL = m_remoteAddress;
      server = aor;
      if (aor.GetHostName() != remoteURL.GetHostName()) {
        /* Note this sets the proxy field because the user has given a full AOR
           with a domain for "user" and then specified a specific host name
           which as far as we are concered is the host to talk to. Setting the
           proxy will prevent SRV lookups or other things that might stop us
           from going to that very specific host.
         */
        possibleProxy = m_remoteAddress;
      }
    }
  }

  if (m_proxyAddress.IsEmpty())
    m_proxyAddress = server.GetParamVars()(OPAL_PROXY_PARAM, possibleProxy);
  if (!m_proxyAddress.IsEmpty())
    server.SetParamVar(OPAL_PROXY_PARAM, m_remoteAddress);

  if (!m_localAddress.IsEmpty()) {
    SIPURL local(m_localAddress);
    m_localAddress = local.AsString();
    aor.SetParamVar(OPAL_LOCAL_ID_PARAM, m_localAddress);
  }

  if (!m_interface.IsEmpty())
    server.SetParamVar(OPAL_INTERFACE_PARAM, m_interface);

  m_remoteAddress = server.AsString();
  m_addressOfRecord = aor.AsString();

  if (m_authID.IsEmpty())
    m_authID = aor.GetUserName();

  if (m_expire == 0)
    m_expire = defaultExpire.GetSeconds();
}


#if PTRACING
ostream & operator<<(ostream & strm, const SIPParameters & params)
{
  strm << "          aor=" << params.m_addressOfRecord << "\n"
          "       remote=" << params.m_remoteAddress << "\n"
          "        local=" << params.m_localAddress << "\n"
          "      contact=" << params.m_contactAddress << "\n"
          "        proxy=" << params.m_proxyAddress << "\n"
          "       authID=" << params.m_authID << "\n"
          "        realm=" << params.m_realm << "\n"
          "       expire=" << params.m_expire << "\n"
          "      restore=" << params.m_restoreTime << "\n"
          "     minRetry=";
  if (params.m_minRetryTime != PMaxTimeInterval)
    strm << params.m_minRetryTime;
  else
    strm << "default";
  strm << "\n"
          "     maxRetry=";
  if (params.m_maxRetryTime != PMaxTimeInterval)
    strm << params.m_maxRetryTime;
  else
    strm << "default";
  return strm;
}
#endif


////////////////////////////////////////////////////////////////////////////////////

#ifdef _MSC_VER
#define P_DISABLE_MSVC_WARNING_4355(member) __pragma(warning(disable:4355)) member __pragma(warning(default:4355))
#else
#define P_DISABLE_MSVC_WARNING_4355(member) member
#endif

class SIPTransactionOwnerDummy : public PSafeObject, public SIPTransactionOwner
{
  public:
    SIPTransactionOwnerDummy(SIPEndPoint & endpoint, const SIPURL & targetURI)
      : P_DISABLE_MSVC_WARNING_4355(SIPTransactionOwner(*this, endpoint))
      , m_targetURI(targetURI)
    {
    }

    virtual SIPURL GetTargetURI() const { return m_targetURI; }
    virtual PString GetAuthID() const { return PString::Empty(); }

  protected:
    SIPURL m_targetURI;
};


SIPResponse::SIPResponse(SIPEndPoint & endpoint, const SIP_PDU & request, StatusCodes code)
  : SIPTransaction(NumMethods, new SIPTransactionOwnerDummy(endpoint, request.GetURI()), request.GetTransport(), true)
{
  m_statusCode = code;
  InitialiseHeaders(request);
}


SIPTransaction * SIPResponse::CreateDuplicate() const
{
  return new SIPResponse(*this);
}


bool SIPResponse::Resend(SIP_PDU & command)
{
  if (m_state == NotStarted) {
    InitialiseHeaders(command);

    GetEndPoint().AddTransaction(this);

    m_state = Completed;
  }

  m_completionTimer = GetEndPoint().GetPduCleanUpTimeout();
  PTRACE(4, "SIP\tResponse transaction timer set " << m_completionTimer);

  // Do not use internal m_transport as is dummy
  m_transport = command.GetTransport();
  return Send();
}


////////////////////////////////////////////////////////////////////////////////////

SIPInvite::SIPInvite(SIPConnection & connection, OpalTransport * transport)
  : SIPTransaction(Method_INVITE, &connection, transport)
{
  connection.OnCreatingINVITE(*this);

  if (m_SDP != NULL)
    m_SDP->SetSessionName(m_mime.GetUserAgent());
}


SIPTransaction * SIPInvite::CreateDuplicate() const
{
  GetConnection()->m_sessions.Assign(m_sessions);
  SIPTransaction * newTransaction = new SIPInvite(*GetConnection(), m_transport);

  // Section 8.1.3.5 of RFC3261 tells that the authenticated
  // request SHOULD have the same value of the Call-ID, To and From.
  // For Asterisk this is not merely SHOULD, but SHALL ....
  newTransaction->GetMIME().Set("From", m_mime.Get("From"));
  return newTransaction;
}


PBoolean SIPInvite::OnReceivedResponse(SIP_PDU & response)
{
  if (IsTerminated())
    return false;

  m_owner->FinaliseForking(*this, response); // Need this before sending ACK

  if (response.GetMIME().GetCSeq().Find(MethodNames[Method_INVITE]) != P_MAX_INDEX &&
                            GetConnection()->OnReceivedResponseToINVITE(*this, response)) {
    // ACK constructed following 13.2.2.4 or 17.1.1.3
    SIPAck ack(*this, response);
    if (!ack.Send())
      return false;
  }

  return SIPTransaction::OnReceivedResponse(response);
}


/////////////////////////////////////////////////////////////////////////

SIPAck::SIPAck(const SIPTransaction & invite, const SIP_PDU & response)
  : SIP_PDU(Method_ACK, invite.GetTransport())
{
  if (response.GetStatusCode() < 300)
    InitialiseHeaders(*invite.GetConnection(), invite.GetMIME().GetCSeqIndex());
  else {
    InitialiseHeaders(invite.GetURI(),
                      response.GetMIME().GetTo(),
                      invite.GetMIME().GetFrom(),
                      invite.GetMIME().GetCallID(),
                      invite.GetMIME().GetCSeqIndex());

    // Use the topmost via header from the INVITE we ACK as per 17.1.1.3
    // as well as the initial Route
    m_mime.SetVia(invite.GetMIME().GetFirstVia());
    m_mime.SetRoute(invite.GetMIME().GetRoute());
  }

  // Add authentication if had any on INVITE
  if (invite.GetMIME().Contains("Proxy-Authorization") || invite.GetMIME().Contains("Authorization")) {
    SIPAuthenticator auth(*this);
    invite.GetConnection()->GetAuthenticator()->Authorise(auth);
  }
}


SIPTransaction * SIPAck::CreateDuplicate() const
{
  return NULL;
}


/////////////////////////////////////////////////////////////////////////

SIPBye::SIPBye(SIPEndPoint & endpoint, SIPDialogContext & dialog)
  : SIPTransaction(Method_BYE, new SIPTransactionOwnerDummy(endpoint, dialog.GetRemoteURI()), NULL, true)
{
  InitialiseHeaders(dialog);
}


SIPBye::SIPBye(SIPConnection & connection)
  : SIPTransaction(Method_BYE, &connection, NULL)
{
}


SIPTransaction * SIPBye::CreateDuplicate() const
{
  return new SIPBye(*GetConnection());
}


////////////////////////////////////////////////////////////////////////////////////

SIPRegister::SIPRegister(SIPTransactionOwner & owner,
                         OpalTransport & transport,
                         const PString & id,
                         unsigned cseq,
                         const Params & params)
  : SIPTransaction(Method_REGISTER, &owner, &transport)
{
  InitialiseHeaders(params.m_registrarAddress,
                    params.m_addressOfRecord,
                    params.m_localAddress,
                    id,
                    cseq);

  SetAllow(m_owner->GetAllowedMethods());

  SetParameters(params);

  if (params.m_compatibility == e_RFC5626) {
    m_mime.AddSupported("path");
    m_mime.AddSupported("outbound");
  }
}


SIPTransaction * SIPRegister::CreateDuplicate() const
{
  return new SIPRegister(*this);
}


#if PTRACING
ostream & operator<<(ostream & strm, const SIPRegister::Params & params)
{
  return strm << (const SIPParameters &)params << "\n"
                 "compatibility=" << params.m_compatibility;
}
#endif


////////////////////////////////////////////////////////////////////////////////////

static const char * const KnownEventPackage[SIPSubscribe::NumPredefinedPackages] = {
  "message-summary",
  "presence",
  "dialog;sla;ma", // sla is the old version ma is the new for Line Appearance extension
  "reg",
  "conference"
};

SIPSubscribe::EventPackage::EventPackage(PredefinedPackages pkg)
  : PCaselessString((pkg & PackageMask) < NumPredefinedPackages ? KnownEventPackage[(pkg & PackageMask)] : "")
{
  if ((pkg & Watcher) != 0)
    *this += ".winfo";
}


SIPSubscribe::EventPackage & SIPSubscribe::EventPackage::operator=(PredefinedPackages pkg)
{
  if ((pkg & PackageMask) < NumPredefinedPackages) {
    PCaselessString::operator=(KnownEventPackage[(pkg & PackageMask)]);
    if ((pkg & Watcher) != 0)
      *this += ".winfo";
  }
  return *this;
}


PObject::Comparison SIPSubscribe::EventPackage::InternalCompare(PINDEX offset, PINDEX length, const char * cstr) const
{
  // Special rules for comparing event package strings, only up to the ';', if present

  PINDEX idx = 0;
  for (;;) {
    if (length-- == 0)
      return EqualTo;
    if (theArray[idx+offset] == '\0' && cstr[idx] == '\0')
      return EqualTo;
    if (theArray[idx+offset] == ';' || cstr[idx] == ';')
      break;
    Comparison c = PCaselessString::InternalCompare(idx+offset, cstr[idx]);
    if (c != EqualTo)
      return c;
    idx++;
  }

  const char * myIdPtr = strstr(theArray+idx+offset, "id");
  const char * theirIdPtr = strstr(cstr+idx, "id");
  if (myIdPtr == NULL && theirIdPtr == NULL)
    return EqualTo;

  if (myIdPtr == NULL)
    return LessThan;

  if (theirIdPtr == NULL)
    return GreaterThan;

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


bool SIPSubscribe::EventPackage::IsWatcher() const
{
  static const char Suffix[] = ".winfo";
  return NumCompare(Suffix, sizeof(Suffix)-1, GetLength()-(sizeof(Suffix)-1)) == EqualTo;
}


SIPSubscribe::NotifyCallbackInfo::NotifyCallbackInfo(SIPSubscribeHandler & handler,
                                                     SIPEndPoint & ep,
                                                     SIP_PDU & request,
                                                     SIP_PDU & response)
  : m_handler(handler)
  , m_endpoint(ep)
  , m_request(request)
  , m_response(response)
  , m_sendResponse(true)
{
}


#if P_EXPAT
bool SIPSubscribe::NotifyCallbackInfo::LoadAndValidate(PXML & xml,
                                                       const PXML::ValidationInfo * validator,
                                                       PXML::Options options)
{
  PString error;
  if (xml.LoadAndValidate(m_request.GetEntityBody(), validator, error, options))
    return true;

  m_response.SetEntityBody(error);
  PTRACE2(2, &xml, "SIP\tError parsing XML in NOTIFY: " << error);
  return false;
}
#endif


bool SIPSubscribe::NotifyCallbackInfo::SendResponse(SIP_PDU::StatusCodes status, const char * extra)
{
  // See if already sent response, don't send another.
  if (!m_sendResponse)
    return true;

  m_response.SetStatusCode(status);

  if (extra != NULL)
    m_response.SetInfo(extra);

  if (!m_response.Send())
    return false;

  m_sendResponse = false;
  return true;
}


SIPSubscribe::SIPSubscribe(SIPTransactionOwner & owner,
                           OpalTransport & transport,
                           SIPDialogContext & dialog,
                           const Params & params)
  : SIPTransaction(Method_SUBSCRIBE, &owner, &transport)
{
  InitialiseHeaders(dialog);

  // I have no idea why this is necessary, but it is the way OpenSIPS works ....
  if (params.m_eventPackage == SIPSubscribe::Dialog && params.m_contactAddress.IsEmpty())
    m_mime.SetContact(dialog.GetRemoteURI());

  m_mime.SetEvent(params.m_eventPackage);

  PString acceptableContentTypes = params.m_contentType;
  if (acceptableContentTypes.IsEmpty()) {
    SIPEventPackageHandler * packageHandler = SIPEventPackageFactory::CreateInstance(params.m_eventPackage);
    if (packageHandler != NULL) {
      acceptableContentTypes = packageHandler->GetContentType();
      delete packageHandler;
    }
  }

  if (params.m_eventList) {
    if (!acceptableContentTypes.IsEmpty())
      acceptableContentTypes += '\n';
    acceptableContentTypes += "multipart/related\napplication/rlmi+xml";
    m_mime.AddSupported("eventlist");
  }

  if (!acceptableContentTypes.IsEmpty())
    m_mime.SetAccept(acceptableContentTypes);

  SetAllow(m_owner->GetAllowedMethods());

  SetParameters(params);

  GetEndPoint().AdjustToRegistration(*this);
}


SIPTransaction * SIPSubscribe::CreateDuplicate() const
{
  return new SIPSubscribe(*this);
}


#if PTRACING
ostream & operator<<(ostream & strm, const SIPSubscribe::Params & params)
{
  return strm << " eventPackage=" << params.m_eventPackage << '\n' << (const SIPParameters &)params;
}
#endif


////////////////////////////////////////////////////////////////////////////////////

SIPNotify::SIPNotify(SIPTransactionOwner & owner,
                     OpalTransport & transport,
                     SIPDialogContext & dialog,
                     const SIPEventPackage & eventPackage,
                     const PString & state,
                     const PString & body)
  : SIPTransaction(Method_NOTIFY, &owner, &transport)
{
  InitialiseHeaders(dialog);

  m_mime.SetEvent(eventPackage);
  m_mime.SetSubscriptionState(state);

  SIPEventPackageHandler * packageHandler = SIPEventPackageFactory::CreateInstance(eventPackage);
  if (packageHandler != NULL) {
    m_mime.SetContentType(packageHandler->GetContentType());
    delete packageHandler;
  }

  m_entityBody = body;

  GetEndPoint().AdjustToRegistration(*this);
}


SIPTransaction * SIPNotify::CreateDuplicate() const
{
  return new SIPNotify(*this);
}


////////////////////////////////////////////////////////////////////////////////////

SIPPublish::SIPPublish(SIPTransactionOwner & owner,
                       OpalTransport & transport,
                       const PString & id,
                       const PString & sipIfMatch,
                       const SIPSubscribe::Params & params,
                       const PString & body)
  : SIPTransaction(Method_PUBLISH, &owner, &transport)
{
  SIPURL addr = params.m_addressOfRecord;
  InitialiseHeaders(addr, addr, addr, id, GetEndPoint().GetNextCSeq());

  if (!sipIfMatch.IsEmpty())
    m_mime.SetSIPIfMatch(sipIfMatch);

  m_mime.SetEvent(params.m_eventPackage);

  if (!body.IsEmpty()) {
    m_entityBody = body;

    if (!params.m_contentType.IsEmpty())
      m_mime.SetContentType(params.m_contentType);
    else {
      SIPEventPackageHandler * packageHandler = SIPEventPackageFactory::CreateInstance(params.m_eventPackage);
      if (packageHandler == NULL)
        m_mime.SetContentType(PMIMEInfo::TextPlain());
      else {
        m_mime.SetContentType(packageHandler->GetContentType());
        delete packageHandler;
      }
    }
  }

  SetParameters(params);

  GetEndPoint().AdjustToRegistration(*this);
}


SIPTransaction * SIPPublish::CreateDuplicate() const
{
  return new SIPPublish(*this);
}


/////////////////////////////////////////////////////////////////////////

SIPRefer::SIPRefer(SIPConnection & connection,
                   const SIPURL & referTo,
                   const SIPURL & referredBy,
                   bool referSub)
  : SIPTransaction(Method_REFER, &connection, NULL)
{
  m_mime.SetProductInfo(connection.GetEndPoint().GetUserAgent(), connection.GetProductInfo());

  m_mime.SetReferTo(referTo.AsQuotedString());

  if(!referredBy.IsEmpty()) {
    SIPURL adjustedReferredBy = referredBy;
    adjustedReferredBy.Sanitise(SIPURL::RequestURI);
    m_mime.SetReferredBy(adjustedReferredBy.AsQuotedString());
  }

  m_mime.SetBoolean("Refer-Sub", referSub); // Use RFC4488 to indicate we doing NOTIFYs or not ...
  m_mime.AddSupported("norefersub");
}


SIPTransaction * SIPRefer::CreateDuplicate() const
{
  return new SIPRefer(*GetConnection(),
                      m_mime.GetReferTo(),
                      m_mime.GetReferredBy(),
                      m_mime.GetBoolean("Refer-Sub"));
}


/////////////////////////////////////////////////////////////////////////

SIPReferNotify::SIPReferNotify(SIPConnection & connection, StatusCodes code)
  : SIPTransaction(Method_NOTIFY, &connection, NULL)
{
  m_mime.SetSubscriptionState(code < Successful_OK ? "active" : "terminated;reason=noresource");
  m_mime.SetEvent("refer");
  m_mime.SetContentType("message/sipfrag");

  PStringStream str;
  str << "SIP/" << m_versionMajor << '.' << m_versionMinor << ' ' << code;
  m_entityBody = str;
}


SIPTransaction * SIPReferNotify::CreateDuplicate() const
{
  return new SIPReferNotify(*this);
}


/////////////////////////////////////////////////////////////////////////

SIPMessage::SIPMessage(SIPTransactionOwner & owner,
                       OpalTransport & transport,
                       const Params & params)
  : SIPTransaction(Method_MESSAGE, &owner, &transport)
  , m_parameters(params)
{
  SIPURL addr(params.m_remoteAddress);

  m_localAddress = params.m_localAddress;
  if (m_localAddress.IsEmpty()) {
    m_localAddress = params.m_addressOfRecord;
    if (m_localAddress.IsEmpty())
      m_localAddress = GetEndPoint().GetDefaultLocalURL(*GetTransport());
  }

  InitialiseHeaders(addr, addr, m_localAddress, params.m_id, GetEndPoint().GetNextCSeq());

  Construct(params);

  GetEndPoint().AdjustToRegistration(*this);
}


SIPMessage::SIPMessage(SIPConnection & conn, const Params & params)
  : SIPTransaction(Method_MESSAGE, &conn, NULL)
  , m_parameters(params)
{
  Construct(params);
}


void SIPMessage::Construct(const Params & params)
{
  if (!params.m_contentType.IsEmpty()) {
    m_mime.SetContentType(params.m_contentType);
    m_entityBody = params.m_body;
  }

  SetParameters(params);
}


SIPTransaction * SIPMessage::CreateDuplicate() const
{
  return new SIPMessage(*this);
}


/////////////////////////////////////////////////////////////////////////

SIPOptions::SIPOptions(SIPEndPoint & endpoint, const Params & params)
  : SIPTransaction(Method_OPTIONS, new SIPTransactionOwnerDummy(endpoint, params.m_remoteAddress), NULL, true)
{
  // Build the correct From field
  SIPURL remoteAddress = params.m_remoteAddress;
  SIPURL localAddress = params.m_localAddress;
  if (localAddress.IsEmpty())
    localAddress = GetEndPoint().GetDefaultLocalURL(*GetTransport());
  localAddress.SetTag();

  InitialiseHeaders(remoteAddress,
                    remoteAddress,
                    localAddress,
                    SIPTransaction::GenerateCallID(),
                    GetEndPoint().GetNextCSeq());

  Construct(params);

  GetEndPoint().AdjustToRegistration(*this);
}


SIPOptions::SIPOptions(SIPConnection & connection, const Params & params)
  : SIPTransaction(Method_OPTIONS, &connection, NULL)
{
  Construct(params);
}


void SIPOptions::Construct(const Params & params)
{
  SetAllow(m_owner->GetAllowedMethods());
  m_mime.SetAccept(params.m_acceptContent);

  if (!params.m_contentType.IsEmpty()) {
    m_mime.SetContentType(params.m_contentType);
    m_entityBody = params.m_body;
  }

  SetParameters(params);
}


SIPTransaction * SIPOptions::CreateDuplicate() const
{
  return new SIPOptions(*this);
}


/////////////////////////////////////////////////////////////////////////

SIPInfo::SIPInfo(SIPConnection & connection, const Params & params)
  : SIPTransaction(Method_INFO, &connection, NULL)
{
  if (!params.m_contentType.IsEmpty()) {
    m_mime.SetContentType(params.m_contentType);
    m_entityBody = params.m_body;
  }
}


SIPTransaction * SIPInfo::CreateDuplicate() const
{
  return new SIPInfo(*this);
}


/////////////////////////////////////////////////////////////////////////

SIPPing::SIPPing(SIPTransactionOwner & owner, OpalTransport & transport,const SIPURL & address)
  : SIPTransaction(Method_PING, &owner, &transport)
{
  InitialiseHeaders(address,
                    address,
                    SIPURL(address.GetUserName(), address.GetTransportAddress()),
                    GenerateCallID(),
                    GetEndPoint().GetNextCSeq());
  // PING must not have a body
}


SIPTransaction * SIPPing::CreateDuplicate() const
{
  return new SIPPing(*this);
}


/////////////////////////////////////////////////////////////////////////

SIPPrack::SIPPrack(SIPConnection & connection, OpalTransport & transport, const PString & rack)
  : SIPTransaction(Method_PRACK, &connection, &transport)
{
  m_mime.SetAt("RAck", rack);
}


SIPTransaction * SIPPrack::CreateDuplicate() const
{
  return new SIPPrack(*this);
}


////////////////////////////////////////////////////////////////////////////

SIPWorkItem::SIPWorkItem(SIPEndPoint & ep, const PString & token)
  : m_endpoint(ep)
  , m_token(token)
{
  PAssert(!token, PInvalidParameter);
}


bool SIPWorkItem::GetTarget(PSafePtr<SIPTransaction> & transaction)
{
  if (m_token.IsEmpty())
    return false;

  transaction = m_endpoint.GetTransaction(m_token, PSafeReadWrite);
  if (transaction == NULL) {
    PTRACE(2, "SIP\tCannot find transaction for timeout using id=" << m_token);
    return false;
  }

  PTRACE(3, transaction, "SIP\tHandling timeout for transaction using id=" << m_token);
  return true;
}


bool SIPWorkItem::GetTarget(PSafePtr<SIPConnection> & connection)
{
  if (m_token.IsEmpty())
    return false;

  connection = m_endpoint.GetSIPConnectionWithLock(m_token, PSafeReadWrite);
  if (connection == NULL) {
    PTRACE(2, "SIP\tCannot find connection for timeout using token=" << m_token);
    return false;
  }

  PTRACE(3, connection, "SIP\tHandling timeout for connection using token=" << m_token);
  return true;
}


bool SIPWorkItem::GetTarget(PSafePtr<SIPHandler> & handler)
{
  if (m_token.IsEmpty())
    return false;

  handler = m_endpoint.FindSIPHandlerByCallID(m_token, PSafeReadWrite);
  if (handler == NULL) {
    PTRACE(2, "SIP\tCannot find handler for timeout using token=" << m_token);
    return false;
  }

  PTRACE(3, handler, "SIP\tHandling timeout for handler using token=" << m_token);
  return true;
}


#endif // OPAL_SIP

// End of file ////////////////////////////////////////////////////////////////
