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
               const OpalTransportAddress & address,
               WORD listenerPort)
{
  if (strncmp(name, "sip:", 4) == 0 || strncmp(name, "sips:", 5) == 0)
    Parse(name);
  else {
    PIPSocket::Address ip;
    WORD port;
    if (address.GetIpAndPort(ip, port)) {
      PStringStream s;
      s << "sip:" << name << '@';
      if (ip.GetVersion() == 6)
        s << '[' << ip << ']';
      else
        s << ip;
      s << ':';
      if (listenerPort != 0)
        s << listenerPort;
      else
        s << port;
      s << ";transport=";
      if (strncmp(address, "tcp", 3) == 0)
        s << "tcp";
      else
        s << "udp";
      Parse(s);
    }
  }
}


PBoolean SIPURL::InternalParse(const char * cstr, const char * _defaultScheme)
{
  PString defaultScheme;
  if (_defaultScheme != NULL) 
    defaultScheme = _defaultScheme;
  else if (strncmp(cstr, "sips:", 5) == 0)
    defaultScheme = "sips";
  else 
    defaultScheme = "sip";

  displayName = PString::Empty();
  fieldParameters = PString::Empty();

  PString str = cstr;

  // see if URL is just a URI or it contains a display address as well
  PINDEX start = str.FindLast('<');
  PINDEX end = str.FindLast('>');

  // see if URL is just a URI or it contains a display address as well
  if (start == P_MAX_INDEX || end == P_MAX_INDEX) {
    if (!PURL::InternalParse(cstr, defaultScheme)) {
      return PFalse;
    }
  }
  else {
    // get the URI from between the angle brackets
    if (!PURL::InternalParse(str(start+1, end-1), defaultScheme))
      return PFalse;

    fieldParameters = str.Mid(end+1).Trim();

    // extract the display address
    end = str.FindLast('"', start);
    start = str.FindLast('"', end-1);
    // There are no double quotes around the display name
    if (start == P_MAX_INDEX && end == P_MAX_INDEX) {
      
      displayName = str.Left(start-1).Trim();
      start = str.FindLast ('<');
      
      // See if there is something before the <
      if (start != P_MAX_INDEX && start > 0)
        displayName = str.Left(start).Trim();
      else { // Use the url as display name
        end = str.FindLast('>');
        if (end != P_MAX_INDEX)
          str = displayName.Mid ((start == P_MAX_INDEX) ? 0:start+1, end-1);

        /* Remove the tag from the display name, if any */
        end = str.Find (';');
        if (end != P_MAX_INDEX)
          str = str.Left (end);

        displayName = str;
        displayName.Replace  ("sip:", "");
      }
    }
    else if (start != P_MAX_INDEX && end != P_MAX_INDEX) {
      // No trim quotes off
      displayName = str(start+1, end-1);
      while ((start = displayName.Find('\\')) != P_MAX_INDEX)
        displayName.Delete(start, 1);
    }
  }

  if (!(scheme *= defaultScheme))
    return PURL::Parse("");

//  if (!paramVars.Contains("transport"))
//    SetParamVar("transport", "udp");

  Recalculate();
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
    { "method",    (1<<RequestURI)|(1<<ToURI)|(1<<FromURI)|(1<<ContactURI)|(1<<RouteURI) },
    { "maddr",     (1<<ToURI)|(1<<FromURI) },
    { "ttl",       (1<<ToURI)|(1<<FromURI)|(1<<RouteURI) },
    { "transport", (1<<ToURI)|(1<<FromURI) },
    { "lr",        (1<<ToURI)|(1<<FromURI)|(1<<ContactURI) },
    { "tag",       (1<<RequestURI)|(1<<ContactURI)|(1<<RouteURI)|(1<<ExternalURI) }
  };

  for (PINDEX i = 0; i < PARRAYSIZE(SanitaryFields); i++) {
    if (SanitaryFields[i].contexts&(1<<context))
      paramVars.RemoveAt(PCaselessString(SanitaryFields[i].name));
  }

  if (context != ContactURI && context != ExternalURI)
    queryVars.RemoveAll();

  if (context == ToURI || context == FromURI)
    port = (scheme *= "sips") ? 5061 : 5060;

  Recalculate();
}


#if P_DNS
PBoolean SIPURL::AdjustToDNS(PINDEX entry)
{
  // RFC3263 states we do not do lookup if explicit port mentioned
  if (GetPortSupplied())
    return PTrue;

  // Or it is a valid IP address, not a domain name
  PIPSocket::Address ip = GetHostName();
  if (ip.IsValid())
    return PTrue;

  // Do the SRV lookup, if fails, then we actually return TRUE so outer loops
  // can use the original host name value.
  PIPSocketAddressAndPortVector addrs;
  if (!PDNS::LookupSRV(GetHostName(), "_sip._" + paramVars("transport", "udp"), GetPort(), addrs))
    return PTrue;

  // Got the SRV list, return FALSE if outer loop has got to the end of it
  if (entry >= (PINDEX)addrs.size())
    return PFalse;

  // Adjust our host and port to what the DNS SRV record says
  SetHostName(addrs[entry].address.AsString());
  SetPort(addrs[entry].port);
  return PTrue;
}
#else
PBoolean SIPURL::AdjustToDNS(PINDEX)
{
  return PTrue;
}
#endif


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


PString SIPMIMEInfo::GetContentType() const
{
  return GetString("Content-Type");
}


void SIPMIMEInfo::SetContentType(const PString & v)
{
  SetAt("Content-Type",  v);
}


PString SIPMIMEInfo::GetContentEncoding() const
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


bool SIPMIMEInfo::GetContacts(std::vector<SIPURL> & contacts) const
{
  PStringArray lines = GetString("Contact").Lines();
  contacts.resize(lines.GetSize());
  for (PINDEX i = 0; i < lines.GetSize(); i++)
    contacts[i].Parse(lines[i]);

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
  return GetRouteList("Route");
}


void SIPMIMEInfo::SetRoute(const PStringList & v)
{
  if (!v.IsEmpty())
    SetRouteList("Route",  v);
}


PStringList SIPMIMEInfo::GetRecordRoute() const
{
  return GetRouteList("Record-Route");
}


void SIPMIMEInfo::SetRecordRoute(const PStringList & v)
{
  if (!v.IsEmpty())
    SetRouteList("Record-Route",  v);
}


PStringList SIPMIMEInfo::GetRouteList(const char * name) const
{
  PStringList routeSet;

  PString s = GetString(name);
  PINDEX left;
  PINDEX right = 0;
  while ((left = s.Find('<', right)) != P_MAX_INDEX &&
         (right = s.Find('>', left)) != P_MAX_INDEX &&
         (right - left) > 5)
    routeSet.AppendString(s(left+1, right-1));

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


PString SIPMIMEInfo::GetSubscriptionState() const
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
    if (str.IsEmpty())
      return; // Have nothing, change nothing
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
    return;
  }

  PINDEX endSecondToken = endFirstToken;
  if (endFirstToken != P_MAX_INDEX && str[endFirstToken] == '/')
    endSecondToken = str.FindSpan(UserAgentTokenChars, endFirstToken+1);

  info.name = str.Left(endFirstToken);
  info.version = str(endFirstToken+1, endSecondToken);
  info.vendor = GetOrganization();
}


void SIPMIMEInfo::SetProductInfo(const PString & ua, const OpalProductInfo & info)
{
  PString userAgent = ua;
  if (userAgent.IsEmpty()) {
    PINDEX pos;
    PCaselessString temp = info.name;
    temp.Replace(' ', '-', PTrue);
    while ((pos = temp.FindSpan(UserAgentTokenChars)) != P_MAX_INDEX)
      temp.Delete(pos, 1);
    if (!temp.IsEmpty()) {
      userAgent = temp;

      temp = info.version;
      temp.Replace(' ', '-', PTrue);
      while ((pos = temp.FindSpan(UserAgentTokenChars)) != P_MAX_INDEX)
        temp.Delete(pos, 1);
      if (!temp.IsEmpty())
        userAgent += '/' + temp;
    }
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


static bool LocateFieldParameter(const PString & fieldValue, const PString & paramName, PINDEX & start, PINDEX & end)
{
  PINDEX semicolon = (PINDEX)-1;
  while ((semicolon = fieldValue.Find(';', semicolon+1)) != P_MAX_INDEX) {
    start = fieldValue.FindSpan("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-.!%*_+`'~", semicolon+1);
    if (start != P_MAX_INDEX && fieldValue[start] == '=' && (fieldValue(semicolon+1, start-1) *= paramName)) {
      start++;
      end = fieldValue.FindOneOf("()<>@,;:\\\"/[]?{}= \t", start)-1;
      return true;
    }
  }

  return false;
}


PString SIPMIMEInfo::ExtractFieldParameter(const PString & fieldValue,
                                           const PString & paramName,
                                           const PString & defaultValue)
{
  PINDEX start, end;
  return LocateFieldParameter(fieldValue, paramName, start, end) ? fieldValue(start, end) : defaultValue;
}


PString SIPMIMEInfo::InsertFieldParameter(const PString & fieldValue,
                                          const PString & paramName,
                                          const PString & newValue)
{

  PINDEX start, end;
  if (LocateFieldParameter(fieldValue, paramName, start, end))
    return fieldValue.Left(start) + newValue + fieldValue.Mid(end+1);

  PStringStream newField;
  newField << fieldValue << ';' << paramName << '=' << newValue;
  return newField;
}


////////////////////////////////////////////////////////////////////////////////////

SIPAuthentication::SIPAuthentication()
{
  isProxy = PFalse;
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

bool SIPDigestAuthentication::EquivalentTo(const SIPAuthentication & _oldAuth)
{
  const SIPDigestAuthentication * oldAuth = dynamic_cast<const SIPDigestAuthentication *>(&_oldAuth);
  PAssert(oldAuth != NULL, "Cannot compare auths of different classes");

  return GetUsername() == oldAuth->GetUsername() &&
         GetNonce()    == oldAuth->GetNonce();
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

bool SIPNTLMAuthentication::EquivalentTo(const SIPAuthentication & /*_oldAuth*/)
{
  return false;
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
{
  Construct(method, dest, to, from, callID, cseq, via);
}


SIP_PDU::SIP_PDU(Methods method,
                 SIPConnection & connection,
                 const OpalTransport & transport)
{
  Construct(method, connection, transport);
}


SIP_PDU::SIP_PDU(const SIP_PDU & request, 
                 StatusCodes code, 
                 const char * contact,
                 const char * extra,
                 const SDPSessionDescription * sdp)
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
  mime.SetRecordRoute(requestMIME.GetRecordRoute());
  SetAllow();

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
  : method(pdu.method)
  , statusCode(pdu.statusCode)
  , uri(pdu.uri)
  , versionMajor(pdu.versionMajor)
  , versionMinor(pdu.versionMinor)
  , info(pdu.info)
  , mime(pdu.mime)
  , entityBody(pdu.entityBody)
  , m_SDP(pdu.m_SDP != NULL ? new SDPSessionDescription(*pdu.m_SDP) : NULL)
{
}


SIP_PDU & SIP_PDU::operator=(const SIP_PDU & pdu)
{
  method = pdu.method;
  statusCode = pdu.statusCode;
  uri = pdu.uri;
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

  OpalGloballyUniqueID branch;
  PStringStream str;
  str << "SIP/" << versionMajor << '.' << versionMinor << '/'
      << via.Left(dollar).ToUpper() << ' ';
  PIPSocket::Address ip;
  WORD port;
  if (via.GetIpAndPort(ip, port))
    str << ip << ':' << port;
  else
    str << via.Mid(dollar+1);
  str << ";branch=z9hG4bK" << branch << ";rport";

  mime.SetVia(str);

  SetAllow();
}


void SIP_PDU::Construct(Methods meth,
                        SIPConnection & connection,
                        const OpalTransport & transport)
{
  PStringList routeSet = connection.GetRouteSet();
  SIPEndPoint & endpoint = connection.GetEndPoint();
  PString localPartyName = connection.GetLocalPartyName();
  SIPURL contact = endpoint.GetContactURL(transport, localPartyName, SIPURL(connection.GetRemotePartyAddress()).GetHostName());
  SIPURL via = endpoint.GetLocalURL(transport, localPartyName);
  contact.Sanitise(meth != Method_INVITE ? SIPURL::ContactURI : SIPURL::RouteURI);
  mime.SetContact(contact);

  Construct(meth,
            connection.GetRequestURI(),
            connection.GetDialogTo(),
            connection.GetDialogFrom(),
            connection.GetToken(),
            connection.GetNextCSeq(),
            via.GetHostAddress());

  SetRoute(routeSet); // Possibly adjust the URI and the route
}


PBoolean SIP_PDU::SetRoute(const PStringList & set)
{
  PStringList routeSet = set;

  if (routeSet.IsEmpty())
    return PFalse;

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
  return PTrue;
}


void SIP_PDU::SetAllow(void)
{
  PStringStream str;
  PStringList methods;
  
  for (PINDEX i = 0 ; i < SIP_PDU::NumMethods ; i++) {
    PString method(MethodNames[i]);
    if (method.Find("SUBSCRIBE") == P_MAX_INDEX && method.Find("REGISTER") == P_MAX_INDEX)
      methods += method;
  }
  
  str << setfill(',') << methods << setfill(' ');

  mime.SetAllow(str);
}


void SIP_PDU::AdjustVia(OpalTransport & transport)
{
  // Update the VIA field following RFC3261, 18.2.1 and RFC3581
  PStringList viaList = mime.GetViaList();
  if (viaList.GetSize() == 0)
    return;

  PString via = viaList.front();
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
    PString rport = SIPMIMEInfo::ExtractFieldParameter(viaList.front(), "rport");
    if (rport.IsEmpty()) {
      // fill in empty rport and received for RFC 3581
      mime.SetFieldParameter("rport", viaList.front(), remotePort);
      mime.SetFieldParameter("received", viaList.front(), remoteIp);
    }
    else if (remoteIp != a ) // set received when remote transport address different from Via address
      mime.SetFieldParameter("received", viaList.front(), remoteIp);
  }
  else if (!a.IsValid()) {
    // Via address given has domain name
    mime.SetFieldParameter("received", viaList.front(), via);
  }

  mime.SetViaList(viaList);
}


bool SIP_PDU::SendResponse(OpalTransport & transport, StatusCodes code, const char * contact, const char * extra)
{
  SIP_PDU response(*this, code, contact, extra);
  return SendResponse(transport, response);
}


bool SIP_PDU::SendResponse(OpalTransport & transport, SIP_PDU & response)
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
    param = SIPMIMEInfo::ExtractFieldParameter(viaList.front(), "received");
    if (!param.IsEmpty()) 
      viaAddress = param;

    // rport is present
    param = SIPMIMEInfo::ExtractFieldParameter(viaList.front(), "rport");
    if (!param.IsEmpty()) 
      viaPort = param;

    newAddress = OpalTransportAddress(viaAddress+":"+viaPort, defaultPort, (proto *= "TCP") ? "tcp$" : "udp$");
  }
  else {
    // get Via from From field
    PString from = mime.GetFrom();
    PINDEX j = from.Find (';');
    if (j != P_MAX_INDEX)
      from = from.Left(j); // Remove all parameters
    j = from.Find ('<');
    if (j != P_MAX_INDEX && from.Find ('>') == P_MAX_INDEX)
      from += '>';

    SIPURL url(from);

    newAddress = OpalTransportAddress(url.GetHostName()+ ":" + PString(PString::Unsigned, url.GetPort()), defaultPort, "udp$");
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
  PStringStream datagram;

  istream * stream;
  if (transport.IsReliable())
    stream = &transport;
  else {
    stream = &datagram;

    PBYTEArray pdu;
    if (transport.ReadPDU(pdu))
      datagram = PString((char *)pdu.GetPointer(), pdu.GetSize());
  }

  if (!transport.IsOpen()) {
    PTRACE(1, "SIP\tAttempt to read PDU from closed tansport " << transport);
    return PFalse;
  }

  // get the message from transport/datagram into cmd and parse MIME
  PString cmd;
  *stream >> cmd >> mime;

  if (!stream->good()) {
    PTRACE_IF(1, transport.GetErrorCode(PChannel::LastReadError) != PChannel::NoError,
              "SIP\tPDU Read failed: " << transport.GetErrorText(PChannel::LastReadError));
    return PFalse;
  }

  if (cmd.IsEmpty()) {
    PTRACE(2, "SIP\tNo Request-Line or Status-Line received on " << transport);
    return PFalse;
  }

  if (cmd.Left(4) *= "SIP/") {
    // parse Response version, code & reason (ie: "SIP/2.0 200 OK")
    PINDEX space = cmd.Find(' ');
    if (space == P_MAX_INDEX) {
      PTRACE(2, "SIP\tBad Status-Line received on " << transport);
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
      PTRACE(2, "SIP\tBad Request-Line received on " << transport);
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
    PTRACE(2, "SIP\tInvalid version received on " << transport);
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


PBoolean SIP_PDU::Write(OpalTransport & transport, const OpalTransportAddress & remoteAddress)
{
  if (!transport.IsOpen()) {
    PTRACE(1, "SIP\tAttempt to write PDU to closed tansport " << transport);
    return PFalse;
  }

  if (!remoteAddress.IsEmpty() && !transport.GetRemoteAddress().IsEquivalent(remoteAddress)) {
    if (!transport.SetRemoteAddress(remoteAddress)) {
      PTRACE(1, "SIP\tCannot use remote address " << remoteAddress << " for tansport " << transport);
      return false;
    }
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

  if (transport.WriteString(strPDU))
    return PTrue;

  PTRACE(1, "SIP\tPDU Write failed: " << transport.GetErrorText(PChannel::LastWriteError));
  return PFalse;
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
  if (m_SDP == NULL && (mime.GetContentType() *= "application/sdp")) {
    m_SDP = new SDPSessionDescription();
    if (!m_SDP->Decode(entityBody)) {
      delete m_SDP;
      m_SDP = NULL;
    }
  }

  return m_SDP;
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
  PTRACE(4, "SIP\tTransaction " << mime.GetCSeq() << " created.");
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
  PTRACE(4, "SIP\tTransaction " << mime.GetCSeq() << " created.");
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


SIPTransaction::~SIPTransaction()
{
  PTRACE_IF(1, state < Terminated_Success, "SIP\tDestroying transaction " << mime.GetCSeq() << " which is not yet terminated.");
  PTRACE(4, "SIP\tTransaction " << mime.GetCSeq() << " destroyed.");
}


PBoolean SIPTransaction::Start()
{
  if (state == Completed)
    return PTrue;

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
  localInterface = transport.GetInterface();

  /* Get the address to which the request PDU should be sent, according to
     the RFC, for a request in a dialog. */
  SIPURL destination = uri;

  PStringList routeSet = GetMIME().GetRoute();
  if (!routeSet.IsEmpty()) {
    SIPURL firstRoute = routeSet.front();
    if (firstRoute.GetParamVars().Contains("lr"))
      destination = firstRoute;
  }

  // Do a DNS SRV lookup
  destination.AdjustToDNS();

  m_remoteAddress = destination.GetHostAddress();
  PTRACE(3, "SIP\tTransaction remote address is " << m_remoteAddress);

  // Use the connection transport to send the request
  if (!Write(transport, m_remoteAddress)) {
    SetTerminated(Terminated_TransportError);
    return PFalse;
  }

  retryTimer = retryTimeoutMin;
  if (method == Method_INVITE)
    completionTimer = endpoint.GetInviteTimeout();
  else
    completionTimer = endpoint.GetNonInviteTimeout();

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
    PTRACE(3, "SIP\tTransaction " << mime.GetCSeq() << " cannot be cancelled.");
    return PFalse;
  }

  completionTimer = endpoint.GetPduCleanUpTimeout();
  return ResendCANCEL();
}


void SIPTransaction::Abort()
{
  if (LockReadWrite()) {
    SetTerminated(Terminated_Aborted);
    UnlockReadWrite();
  }
}


bool SIPTransaction::SendPDU(SIP_PDU & pdu)
{
  PString oldInterface = transport.GetInterface();
  if (transport.SetInterface(localInterface) &&
      pdu.Write(transport, m_remoteAddress) &&
      transport.SetInterface(oldInterface))
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
                 localInterface);
  // Use the topmost via header from the INVITE we cancel as per 9.1. 
  PStringList viaList = mime.GetViaList();
  cancel.GetMIME().SetVia(viaList.front());

  if (!SendPDU(cancel))
    return false;

  if (state < Cancelling) {
    state = Cancelling;
    retry = 0;
    retryTimer = retryTimeoutMin;
  }

  return true;
}


PBoolean SIPTransaction::OnReceivedResponse(SIP_PDU & response)
{
  // Stop the timers outside of the mutex to avoid deadlock
  retryTimer.Stop(false);
  completionTimer.Stop(false);

  PString cseq = response.GetMIME().GetCSeq();

  // If is the response to a CANCEL we sent, then just ignore it
  if (cseq.Find(MethodNames[Method_CANCEL]) != P_MAX_INDEX) {
    // Lock only if we have not already locked it in SIPInvite::OnReceivedResponse
    if (LockReadWrite()) {
      SetTerminated(Terminated_Cancelled);
      UnlockReadWrite();
    }
    return PFalse;
  }

  // Something wrong here, response is not for the request we made!
  if (cseq.Find(MethodNames[method]) == P_MAX_INDEX) {
    PTRACE(2, "SIP\tTransaction " << cseq << " response not for " << *this);
    return PFalse;
  }

  PSafeLockReadWrite lock(*this);
  if (!lock.IsLocked())
    return PFalse;

  PBoolean notCompletedFlag = state < Completed;
  bool signalCompletionFlag = false;

  /* Really need to check if response is actually meant for us. Have a
     temporary cheat in assuming that we are only sending a given CSeq to one
     and one only host, so anything coming back with that CSeq is OK. This has
     "issues" according to the spec but
     */
  if (notCompletedFlag && response.GetStatusCode()/100 == 1) {
    PTRACE(3, "SIP\tTransaction " << cseq << " proceeding.");

    if (state == Trying)
      state = Proceeding;

    retry = 0;
    retryTimer = retryTimeoutMax;
    completionTimer = endpoint.GetNonInviteTimeout();
  }
  else {
    if (notCompletedFlag) {
      PTRACE(3, "SIP\tTransaction " << cseq << " completed.");
      state = Completed;
      statusCode = response.GetStatusCode();
    }

    signalCompletionFlag = true;
    completionTimer = endpoint.GetPduCleanUpTimeout();
  }

  if (notCompletedFlag) {
    if (connection != NULL)
      connection->OnReceivedResponse(*this, response);
    else
      endpoint.OnReceivedResponse(*this, response);

    if (state == Completed)
      OnCompleted(response);
  }

  if (signalCompletionFlag)
    completed.Signal();

  return PTrue;
}


PBoolean SIPTransaction::OnCompleted(SIP_PDU & /*response*/)
{
  return PTrue;
}


void SIPTransaction::OnRetry(PTimer &, INT)
{
  PSafeLockReadWrite lock(*this);

  if (!lock.IsLocked() || (state != Trying && state != Cancelling))
    return;

  retry++;

  PTRACE(3, "SIP\tTransaction " << mime.GetCSeq() << " timeout, making retry " << retry);

  if (retry >= endpoint.GetMaxRetries()) {
    SetTerminated(Terminated_RetriesExceeded);
    return;
  }

  if (state == Cancelling) {
    if (!ResendCANCEL())
      return;
  }
  else {
    if (!SendPDU(*this))
      return;
  }

  PTimeInterval timeout = retryTimeoutMin*(1<<retry);
  if (timeout > retryTimeoutMax)
    retryTimer = retryTimeoutMax;
  else
    retryTimer = timeout;
}


void SIPTransaction::OnTimeout(PTimer &, INT)
{
  PSafeLockReadWrite lock(*this);

  if (lock.IsLocked() && state <= Completed)
    SetTerminated(state != Completed ? Terminated_Timeout : Terminated_Success);
}


void SIPTransaction::SetTerminated(States newState)
{
#if PTRACING
  static const char * const StateNames[NumStates] = {
    "NotStarted",
    "Trying",
    "Aborting",
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
  
  if (state >= Terminated_Success) {
    PTRACE_IF(3, newState != Terminated_Success, "SIP\tTried to set state " << StateNames[newState] 
              << " for transaction " << mime.GetCSeq()
              << " but already terminated ( " << StateNames[state] << ')');
    return;
  }
  
  States oldState = state;
  
  state = newState;
  PTRACE(3, "SIP\tSet state " << StateNames[newState] << " for transaction " << mime.GetCSeq());

  if (oldState != Completed)
    completed.Signal();

  // Transaction failed, tell the endpoint
  switch (state) {
    case Terminated_Success :
      break;

    case Terminated_Timeout :
    case Terminated_RetriesExceeded:
      statusCode = SIP_PDU::Failure_RequestTimeout;
      // Do default case

    default :
      endpoint.OnTransactionFailed(*this);
      if (connection != NULL)
        connection->OnTransactionFailed(*this);
  }
}


////////////////////////////////////////////////////////////////////////////////////

SIPInvite::SIPInvite(SIPConnection & connection, OpalTransport & transport)
  : SIPTransaction(connection, transport, Method_INVITE)
{
  mime.SetDate() ;                             // now
  mime.SetProductInfo(connection.GetEndPoint().GetUserAgent(), connection.GetProductInfo());

  m_SDP = new SDPSessionDescription();
  if (!connection.OnSendSDP(false, rtpSessions, *m_SDP)) {
    delete m_SDP;
    m_SDP = NULL;
  }

  connection.OnCreatingINVITE(*this);
}


SIPInvite::SIPInvite(SIPConnection & connection, OpalTransport & transport, OpalRTPSessionManager & sm)
  : SIPTransaction(connection, transport, Method_INVITE)
{
  mime.SetDate() ;                             // now
  mime.SetProductInfo(connection.GetEndPoint().GetUserAgent(), connection.GetProductInfo());

  rtpSessions.CopyFromMaster(sm);
  m_SDP = new SDPSessionDescription();
  if (!connection.OnSendSDP(false, rtpSessions, *m_SDP)) {
    delete m_SDP;
    m_SDP = NULL;
  }

  connection.OnCreatingINVITE(*this);
}


PBoolean SIPInvite::OnReceivedResponse(SIP_PDU & response)
{
  if (response.GetMIME().GetCSeq().Find(MethodNames[Method_INVITE]) != P_MAX_INDEX) {
    connection->OnReceivedResponseToINVITE(*this, response);

    if (response.GetStatusCode() >= 200) {
      PSafeLockReadWrite lock(*this);
      if (!lock.IsLocked())
        return false;

      // ACK constructed following 17.1.1.3
      SIPAck ack(*this, response);
      if (!SendPDU(ack))
        return false;
    }
  }

  if (!SIPTransaction::OnReceivedResponse(response))
    return false;

  if (!LockReadWrite())
    return false;

  /* Handle response to outgoing call cancellation */
  if (response.GetStatusCode() == Failure_RequestTerminated)
    SetTerminated(Terminated_Success);

  if (response.GetStatusCode()/100 == 1)
    completionTimer = PTimeInterval(0, mime.GetExpires(180));
    
  UnlockReadWrite();

  return true;
}


////////////////////////////////////////////////////////////////////////////////////

SIPRegister::Params::Params()
  : m_expire(0)
  , m_restoreTime(30)
  , m_minRetryTime(PMaxTimeInterval)
  , m_maxRetryTime(PMaxTimeInterval)
{
}


SIPRegister::SIPRegister(SIPEndPoint & ep,
                         OpalTransport & trans,
                         const PStringList & routeSet,
                         const PString & id,
                         const Params & params)
  : SIPTransaction(ep, trans, params.m_minRetryTime, params.m_maxRetryTime)
{
  SIPURL aor = params.m_addressOfRecord;
  PString addrStr = aor.AsQuotedString();
  OpalTransportAddress viaAddress = ep.GetLocalURL(transport).GetHostAddress();
  SIP_PDU::Construct(Method_REGISTER,
                     "sip:"+aor.GetHostName(),
                     addrStr,
                     addrStr+";tag="+OpalGloballyUniqueID().AsString(),
                     id,
                     endpoint.GetNextCSeq(),
                     viaAddress);

  mime.SetProductInfo(ep.GetUserAgent(), ep.GetProductInfo());
  SIPURL contact = params.m_contactAddress;
  if (contact.IsEmpty())
    contact = endpoint.GetLocalURL(trans, aor.GetUserName());
  contact.Sanitise(SIPURL::ContactURI);
  mime.SetContact(contact);

  mime.SetExpires(params.m_expire);

  SetRoute(routeSet);
}


////////////////////////////////////////////////////////////////////////////////////

SIPSubscribe::Params::Params(PredefinedPackages pkg)
  : m_eventPackage(SIPSubscribe::GetEventPackageName(pkg))
  , m_expire(0)
  , m_restoreTime(30)
  , m_minRetryTime(PMaxTimeInterval)
  , m_maxRetryTime(PMaxTimeInterval)
{
  switch (pkg) {
    case SIPSubscribe::MessageSummary :
      m_mimeType = "application/simple-message-summary";
      break;

    case SIPSubscribe::Presence :
      m_mimeType = "application/pidf+xml";
      break;

    default :
      break;
  }
}


PString SIPSubscribe::GetEventPackageName(PredefinedPackages pkg)
{
  static const char * const names[NumPredefinedPackages] = {
    "message-summary",
    "presence"
  };
  if (pkg < NumPredefinedPackages)
    return names[pkg];
  return PString::Empty();
}


SIPSubscribe::SIPSubscribe(SIPEndPoint & ep,
                           OpalTransport & trans,
                           const PStringList & routeSet,
                           const PString & to,
                           const PString & id,
                           unsigned cseq,
                           const Params & params)
  : SIPTransaction(ep, trans)
{
  SIPURL targetAddress = params.m_targetAddress;

  PString localPartyAddress;
  if (params.m_eventPackage == GetEventPackageName(Presence))
    localPartyAddress = endpoint.GetRegisteredPartyName(params.m_targetAddress).AsQuotedString();
  else
    localPartyAddress = targetAddress.AsQuotedString();
  localPartyAddress += ";tag=" + OpalGloballyUniqueID().AsString();

  targetAddress.Sanitise(SIPURL::RequestURI);

  OpalTransportAddress viaAddress = ep.GetLocalURL(transport).GetHostAddress();
  SIP_PDU::Construct(Method_SUBSCRIBE,
                     targetAddress,
                     to,
                     localPartyAddress,
                     id,
                     cseq,
                     viaAddress);

  mime.SetProductInfo(ep.GetUserAgent(), ep.GetProductInfo());
  SIPURL contact = endpoint.GetLocalURL(trans, SIPURL(localPartyAddress).GetUserName());
  contact.Sanitise(SIPURL::ContactURI);
  mime.SetContact(contact);
  mime.SetAccept(params.m_mimeType);
  mime.SetEvent(params.m_eventPackage);
  mime.SetExpires(params.m_expire);

  SetRoute(routeSet);
}


////////////////////////////////////////////////////////////////////////////////////

SIPPublish::SIPPublish(SIPEndPoint & ep,
                       OpalTransport & trans,
                       const PStringList & /*routeSet*/,
                       const SIPURL & targetAddress,
                       const PString & id,
                       const PString & sipIfMatch,
                       const PString & body,
                       unsigned expires)
  : SIPTransaction(ep, trans)
{
  PString addrStr = targetAddress.AsQuotedString();
  OpalTransportAddress viaAddress = ep.GetLocalURL(transport).GetHostAddress();

  SIP_PDU::Construct(Method_PUBLISH,
                     targetAddress,
                     addrStr,
                     addrStr+";tag="+OpalGloballyUniqueID().AsString(),
                     id,
                     endpoint.GetNextCSeq(),
                     viaAddress);

  mime.SetProductInfo(ep.GetUserAgent(), ep.GetProductInfo());
  SIPURL contact = endpoint.GetLocalURL(trans, targetAddress.GetUserName());
  contact.Sanitise(SIPURL::ContactURI);
  mime.SetContact(contact);
  mime.SetExpires(expires);

  if (!sipIfMatch.IsEmpty())
    mime.SetSIPIfMatch(sipIfMatch);
  
  mime.SetEvent("presence");
  mime.SetContentType("application/pidf+xml");

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


void SIPRefer::Construct(SIPConnection & connection, OpalTransport & /*transport*/, const SIPURL & refer, const SIPURL & referred_by)
{
  mime.SetProductInfo(connection.GetEndPoint().GetUserAgent(), connection.GetProductInfo());
  mime.SetReferTo(refer.AsQuotedString());
  if(!referred_by.IsEmpty())
    mime.SetReferredBy(referred_by.AsQuotedString());
}


/////////////////////////////////////////////////////////////////////////

SIPReferNotify::SIPReferNotify(SIPConnection & connection, OpalTransport & transport, StatusCodes code)
  : SIPTransaction(connection, transport, Method_NOTIFY)
{
  PStringStream str;
  
  mime.SetProductInfo(connection.GetEndPoint().GetUserAgent(), connection.GetProductInfo());
  mime.SetSubscriptionState("terminated;reason=noresource"); // Do not keep an internal state
  mime.SetEvent("refer");
  mime.SetContentType("message/sipfrag;version=2.0");


  str << "SIP/" << versionMajor << '.' << versionMinor << " " << code << " " << GetStatusCodeDescription(code);
  entityBody = str;
}


/////////////////////////////////////////////////////////////////////////

SIPMessage::SIPMessage(SIPEndPoint & ep,
                       OpalTransport & trans,
                       const SIPURL & address,
                       const PStringList & routeSet,
                       const PString & id,
                       const PString & body)
  : SIPTransaction(ep, trans)
{
  OpalTransportAddress viaAddress = ep.GetLocalURL(transport).GetHostAddress();
    
  // Build the correct From field
  PString displayName = ep.GetDefaultDisplayName();
  PString partyName = endpoint.GetRegisteredPartyName(address).AsString(); 

  SIPURL myAddress("\"" + displayName + "\" <" + partyName + ">"); 

  SIP_PDU::Construct(Method_MESSAGE,
                     "sip:"+address.GetUserName()+"@"+address.GetHostName(),
                     address.AsQuotedString(),
                     myAddress.AsQuotedString()+";tag="+OpalGloballyUniqueID().AsString(),
                     id,
                     endpoint.GetNextCSeq(),
                     viaAddress);
  mime.SetContentType("text/plain;charset=UTF-8");
  mime.SetRoute(routeSet);

  entityBody = body;
}

/////////////////////////////////////////////////////////////////////////

SIPPing::SIPPing(SIPEndPoint & ep,
                 OpalTransport & trans,
                 const SIPURL & address,
                 const PString & body)
  : SIPTransaction(ep, trans)
{
  PString id = OpalGloballyUniqueID().AsString() + "@" + PIPSocket::GetHostName();
  OpalTransportAddress viaAddress = ep.GetLocalURL(transport).GetHostAddress();
    
  // Build the correct From field
  PString displayName = ep.GetDefaultDisplayName();
  PString partyName = endpoint.GetRegisteredPartyName(address).AsString(); 

  SIPURL myAddress("\"" + displayName + "\" <" + partyName + ">"); 
  
  SIP_PDU::Construct(Method_PING,
                     "sip:"+address.GetUserName()+"@"+address.GetHostName(),
                     address.AsQuotedString(),
                     // myAddress.AsQuotedString()+";tag="+OpalGloballyUniqueID().AsString(),
                     "sip:"+address.GetUserName()+"@"+address.GetHostName(),
                     id,
                     endpoint.GetNextCSeq(),
                     viaAddress);
  mime.SetContentType("text/plain;charset=UTF-8");

  entityBody = body;
}


/////////////////////////////////////////////////////////////////////////

SIPAck::SIPAck(SIPTransaction & invite, SIP_PDU & response)
{
  if (response.GetStatusCode() < 300) {
    Construct(Method_ACK, *invite.GetConnection(), invite.GetTransport());
    mime.SetCSeq(PString(invite.GetMIME().GetCSeqIndex()) & MethodNames[Method_ACK]);
  }
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
  PString requestURI;
  PString hosturl;
  PString id = OpalGloballyUniqueID().AsString() + "@" + PIPSocket::GetHostName();
  OpalTransportAddress viaAddress = ep.GetLocalURL(transport).GetHostAddress();
    
  // Build the correct From field
  PString displayName = ep.GetDefaultDisplayName();
  PString partyName = endpoint.GetRegisteredPartyName(SIPURL(address).GetHostName()).AsString();

  SIPURL myAddress("\"" + displayName + "\" <" + partyName + ">"); 

  requestURI = "sip:" + address.AsQuotedString();
  
  SIP_PDU::Construct(Method_OPTIONS,
                     requestURI,
                     address.AsQuotedString(),
                     myAddress.AsQuotedString()+";tag="+OpalGloballyUniqueID().AsString(),
                     id,
                     endpoint.GetNextCSeq(),
                     viaAddress);
  mime.SetAccept("application/sdp");
}


#endif // OPAL_SIP

// End of file ////////////////////////////////////////////////////////////////
