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
#ifdef OPAL_SIP

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


const char * SIP_PDU::GetStatusCodeDescription (int code)
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

    { 0 }
  };

  for (PINDEX i = 0; sipErrorDescriptions[i].code != 0; i++) {
    if (sipErrorDescriptions[i].code == code)
      return sipErrorDescriptions[i].desc;
  }
  return "";
}


static const char * const AlgorithmNames[SIPAuthentication::NumAlgorithms] = {
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


void SIPURL::AdjustForRequestURI()
{
  paramVars.RemoveAt("tag");
  paramVars.RemoveAt("transport");
  queryVars.RemoveAll();
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


PINDEX SIPMIMEInfo::GetContentLength() const
{
  PString len = GetFullOrCompact("Content-Length", 'l');
  if (len.IsEmpty())
    return 0; //P_MAX_INDEX;
  return len.AsInteger();
}

PBoolean SIPMIMEInfo::IsContentLengthPresent() const
{
  return !GetFullOrCompact("Content-Length", 'l').IsEmpty();
}


PString SIPMIMEInfo::GetContentType() const
{
  return GetFullOrCompact("Content-Type", 'c');
}


void SIPMIMEInfo::SetContentType(const PString & v)
{
  SetAt(compactForm ? "c" : "Content-Type",  v);
}


PString SIPMIMEInfo::GetContentEncoding() const
{
  return GetFullOrCompact("Content-Encoding", 'e');
}


void SIPMIMEInfo::SetContentEncoding(const PString & v)
{
  SetAt(compactForm ? "e" : "Content-Encoding",  v);
}


PString SIPMIMEInfo::GetFrom() const
{
  return GetFullOrCompact("From", 'f');
}


void SIPMIMEInfo::SetFrom(const PString & v)
{
  SetAt(compactForm ? "f" : "From",  v);
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
  return GetFullOrCompact("Call-ID", 'i');
}


void SIPMIMEInfo::SetCallID(const PString & v)
{
  SetAt(compactForm ? "i" : "Call-ID",  v);
}


PString SIPMIMEInfo::GetContact() const
{
  return GetFullOrCompact("Contact", 'm');
}


void SIPMIMEInfo::SetContact(const PString & v)
{
  SetAt(compactForm ? "m" : "Contact",  v);
}


void SIPMIMEInfo::SetContact(const SIPURL & url)
{
  SetContact(url.AsQuotedString());
}


PString SIPMIMEInfo::GetSubject() const
{
  return GetFullOrCompact("Subject", 's');
}


void SIPMIMEInfo::SetSubject(const PString & v)
{
  SetAt(compactForm ? "s" : "Subject",  v);
}


PString SIPMIMEInfo::GetTo() const
{
  return GetFullOrCompact("To", 't');
}


void SIPMIMEInfo::SetTo(const PString & v)
{
  SetAt(compactForm ? "t" : "To",  v);
}


PString SIPMIMEInfo::GetVia() const
{
  return GetFullOrCompact("Via", 'v');
}


void SIPMIMEInfo::SetVia(const PString & v)
{
  if (!v.IsEmpty())
    SetAt(compactForm ? "v" : "Via",  v);
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
  SetAt(compactForm ? "v" : "Via", fieldValue);
}


PString SIPMIMEInfo::GetReferTo() const
{
  return GetFullOrCompact("Refer-To", 'r');
}


void SIPMIMEInfo::SetReferTo(const PString & r)
{
  SetAt(compactForm ? "r" : "Refer-To",  r);
}

PString SIPMIMEInfo::GetReferredBy() const
{
  return GetFullOrCompact("Referred-By", 'b');
}


void SIPMIMEInfo::SetReferredBy(const PString & r)
{
  SetAt(compactForm ? "b" : "Referred-By",  r);
}

void SIPMIMEInfo::SetContentLength(PINDEX v)
{
  SetAt(compactForm ? "l" : "Content-Length", PString(PString::Unsigned, v));
}


PString SIPMIMEInfo::GetCSeq() const
{
  return (*this)("CSeq");       // no compact form
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

  PString s = (*this)(name);
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
  return (*this)(PCaselessString("Accept"));    // no compact form
}


void SIPMIMEInfo::SetAccept(const PString & v)
{
  SetAt("Accept", v);  // no compact form
}


PString SIPMIMEInfo::GetAcceptEncoding() const
{
  return (*this)(PCaselessString("Accept-Encoding"));   // no compact form
}


void SIPMIMEInfo::SetAcceptEncoding(const PString & v)
{
  SetAt("Accept-Encoding", v); // no compact form
}


PString SIPMIMEInfo::GetAcceptLanguage() const
{
  return (*this)(PCaselessString("Accept-Language"));   // no compact form
}


void SIPMIMEInfo::SetAcceptLanguage(const PString & v)
{
  SetAt("Accept-Language", v); // no compact form
}


PString SIPMIMEInfo::GetAllow() const
{
  return (*this)(PCaselessString("Allow"));     // no compact form
}


void SIPMIMEInfo::SetAllow(const PString & v)
{
  SetAt("Allow", v);   // no compact form
}



PString SIPMIMEInfo::GetDate() const
{
  return (*this)(PCaselessString("Date"));      // no compact form
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
  PString v = (*this)(PCaselessString("Expires"));      // no compact form
  if (v.IsEmpty())
    return dflt;

  return v.AsUnsigned();
}


void SIPMIMEInfo::SetExpires(unsigned v)
{
  SetAt("Expires", PString(PString::Unsigned, v));      // no compact form
}


PINDEX SIPMIMEInfo::GetMaxForwards() const
{
  PString len = (*this)(PCaselessString("Max-Forwards"));       // no compact form
  if (len.IsEmpty())
    return P_MAX_INDEX;
  return len.AsInteger();
}


void SIPMIMEInfo::SetMaxForwards(PINDEX v)
{
  SetAt("Max-Forwards", PString(PString::Unsigned, v)); // no compact form
}


PINDEX SIPMIMEInfo::GetMinExpires() const
{
  PString len = (*this)(PCaselessString("Min-Expires"));        // no compact form
  if (len.IsEmpty())
    return P_MAX_INDEX;
  return len.AsInteger();
}


void SIPMIMEInfo::SetMinExpires(PINDEX v)
{
  SetAt("Min-Expires",  PString(PString::Unsigned, v)); // no compact form
}


PString SIPMIMEInfo::GetProxyAuthenticate() const
{
  return (*this)(PCaselessString("Proxy-Authenticate"));        // no compact form
}


void SIPMIMEInfo::SetProxyAuthenticate(const PString & v)
{
  SetAt("Proxy-Authenticate",  v);      // no compact form
}


PString SIPMIMEInfo::GetSupported() const
{
  return GetFullOrCompact("Supported", 'k');
}

void SIPMIMEInfo::SetSupported(const PString & v)
{
  SetAt(compactForm ? "k" : "Supported",  v);
}


PString SIPMIMEInfo::GetUnsupported() const
{
  return (*this)(PCaselessString("Unsupported"));       // no compact form
}


void SIPMIMEInfo::SetUnsupported(const PString & v)
{
  SetAt("Unsupported",  v);     // no compact form
}


PString SIPMIMEInfo::GetEvent() const
{
  return GetFullOrCompact("Event", 'o');
}


void SIPMIMEInfo::SetEvent(const PString & v)
{
  SetAt(compactForm ? "o" : "Event",  v);
}


PString SIPMIMEInfo::GetSubscriptionState() const
{
  return (*this)(PCaselessString("Subscription-State"));       // no compact form
}


void SIPMIMEInfo::SetSubscriptionState(const PString & v)
{
  SetAt("Subscription-State",  v);     // no compact form
}


PString SIPMIMEInfo::GetUserAgent() const
{
  return (*this)(PCaselessString("User-Agent"));        // no compact form
}


void SIPMIMEInfo::SetUserAgent(const PString & v)
{
  SetAt("User-Agent",  v);     // no compact form
}


PString SIPMIMEInfo::GetOrganization() const
{
  return (*this)(PCaselessString("Organization"));        // no compact form
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
    str = (*this)("Server");
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
  return (*this)(PCaselessString("WWW-Authenticate"));  // no compact form
}


void SIPMIMEInfo::SetWWWAuthenticate(const PString & v)
{
  SetAt("WWW-Authenticate",  v);        // no compact form
}

PString SIPMIMEInfo::GetSIPIfMatch() const
{
  return (*this)(PCaselessString("SIP-If-Match"));  // no compact form
}

void SIPMIMEInfo::SetSIPIfMatch(const PString & v)
{
  SetAt("SIP-If-Match",  v);        // no compact form
}

PString SIPMIMEInfo::GetSIPETag() const
{
  return (*this)(PCaselessString("SIP-ETag"));  // no compact form
}

void SIPMIMEInfo::SetSIPETag(const PString & v)
{
  SetAt("SIP-ETag",  v);        // no compact form
}

void SIPMIMEInfo::SetFieldParameter(const PString & param,
                                          PString & field,
                                    const PString & value)
{
  PStringStream s;
  
  PCaselessString val = field;
  
  if (HasFieldParameter(param, field)) {

    val = GetFieldParameter(param, field);
    
    if (!val.IsEmpty()) // The parameter already has a value, replace it.
      field.Replace(val, value);
    else { // The parameter has no value
     
      s << param << "=" << value;
      field.Replace(param, s);
    }
  }
  else { // There is no such parameter

    s << field << ";" << param << "=" << value;
    field = s;
  }
}


PString SIPMIMEInfo::GetFieldParameter(const PString & param,
                                       const PString & field)
{
  PINDEX j = 0;
  
  PCaselessString val = field;
  if ((j = val.FindLast (param)) != P_MAX_INDEX) {

    val = val.Mid(j+param.GetLength());
    if ((j = val.Find (';')) != P_MAX_INDEX)
      val = val.Left(j);

    if ((j = val.Find (' ')) != P_MAX_INDEX)
      val = val.Left(j);
    
    if ((j = val.Find (',')) != P_MAX_INDEX)
      val = val.Left(j);
    
    if ((j = val.Find ('=')) != P_MAX_INDEX) 
      val = val.Mid(j+1);
    else
      val = "";
  }
  else
    val = "";

  return val;
}


PBoolean SIPMIMEInfo::HasFieldParameter(const PString & param, const PString & field)
{
  PCaselessString val = field;
  
  return (val.Find(param) != P_MAX_INDEX);
}


PString SIPMIMEInfo::GetFullOrCompact(const char * fullForm, char compactForm) const
{
  if (Contains(PCaselessString(fullForm)))
    return (*this)[fullForm];
  return (*this)(PCaselessString(compactForm));
}


////////////////////////////////////////////////////////////////////////////////////

SIPAuthentication::SIPAuthentication(const PString & user, const PString & pwd)
  : username(user), password(pwd)
{
  algorithm = NumAlgorithms;
  isProxy = PFalse;
}


static PString GetAuthParam(const PString & auth, const char * name)
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


PBoolean SIPAuthentication::Parse(const PCaselessString & auth, PBoolean proxy)
{
  authRealm.MakeEmpty();
  nonce.MakeEmpty();
  opaque.MakeEmpty();
  algorithm = NumAlgorithms;

  qopAuth = qopAuthInt = PFalse;
  cnonce.MakeEmpty();
  nonceCount.SetValue(1);

  if (auth.Find("digest") != 0) {
    PTRACE(1, "SIP\tUnknown authentication type");
    return PFalse;
  }

  PCaselessString str = GetAuthParam(auth, "algorithm");
  if (str.IsEmpty())
    algorithm = Algorithm_MD5;  // default
  else if (str == "md5")
    algorithm = Algorithm_MD5;
  else {
    PTRACE(1, "SIP\tUnknown authentication algorithm");
    return PFalse;
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


PBoolean SIPAuthentication::IsValid() const
{
  return /*!authRealm && */ !username && !nonce && algorithm < NumAlgorithms;
}


static PString AsHex(PMessageDigest5::Code & digest)
{
  PStringStream out;
  out << hex << setfill('0');
  for (PINDEX i = 0; i < 16; i++)
    out << setw(2) << (unsigned)((BYTE *)&digest)[i];
  return out;
}


PBoolean SIPAuthentication::Authorise(SIP_PDU & pdu) const
{
  if (!IsValid()) {
    PTRACE(3, "SIP\tNo authentication information present");
    return PFalse;
  }

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
                 const char * extra)
{
  method       = NumMethods;
  statusCode   = code;
  versionMajor = request.GetVersionMajor();
  versionMinor = request.GetVersionMinor();
  sdp = NULL;

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
    mime.SetContact(contact.AsQuotedString());
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
  : method(pdu.method),
    statusCode(pdu.statusCode),
    uri(pdu.uri),
    versionMajor(pdu.versionMajor),
    versionMinor(pdu.versionMinor),
    info(pdu.info),
    mime(pdu.mime),
    entityBody(pdu.entityBody)
{
  if (pdu.sdp != NULL)
    sdp = new SDPSessionDescription(*pdu.sdp);
  else
    sdp = NULL;
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

  delete sdp;
  if (pdu.sdp != NULL)
    sdp = new SDPSessionDescription(*pdu.sdp);
  else
    sdp = NULL;

  return *this;
}


SIP_PDU::~SIP_PDU()
{
  delete sdp;
}


void SIP_PDU::Construct(Methods meth)
{
  method = meth;
  statusCode = IllegalStatusCode;

  versionMajor = SIP_VER_MAJOR;
  versionMinor = SIP_VER_MINOR;

  sdp = NULL;
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
  uri.AdjustForRequestURI();

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
  mime.SetContact(contact);

  SIPURL targetAddress = connection.GetTargetAddress();
  targetAddress.AdjustForRequestURI(),

  Construct(meth,
            targetAddress,
            connection.GetRemotePartyAddress(),
            connection.GetExplicitFrom(),
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
    uri.AdjustForRequestURI();
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

    if (mime.HasFieldParameter("rport", viaList.front()) && mime.GetFieldParameter("rport", viaList.front()).IsEmpty()) {
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


OpalTransportAddress SIP_PDU::GetViaAddress(OpalEndPoint &ep)
{
  PStringList viaList = mime.GetViaList();

  if (viaList.GetSize() > 0) {

    PString viaAddress = viaList.front();
    PString proto = viaList.front();
    PString viaPort = ep.GetDefaultSignalPort();

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
    if (mime.HasFieldParameter("maddr", viaList.front())) 
      viaAddress = mime.GetFieldParameter("maddr", viaList.front());
    // received and rport are present
    else if (mime.HasFieldParameter("received", viaList.front()) && mime.HasFieldParameter("rport", viaList.front())) {
      viaAddress = mime.GetFieldParameter("received", viaList.front());
      viaPort = mime.GetFieldParameter("rport", viaList.front());
    }
    // received is present
    else if (mime.HasFieldParameter("received", viaList.front()))
      viaAddress = mime.GetFieldParameter("received", viaList.front());

    OpalTransportAddress address(viaAddress+":"+viaPort, ep.GetDefaultSignalPort(), (proto *= "TCP") ? "$tcp" : "udp$");
    return address;
  }

  // get Via from From field
  PString from = mime.GetFrom();
  PINDEX j = from.Find (';');
  if (j != P_MAX_INDEX)
    from = from.Left(j); // Remove all parameters
  j = from.Find ('<');
  if (j != P_MAX_INDEX && from.Find ('>') == P_MAX_INDEX)
    from += '>';

  SIPURL url(from);

  OpalTransportAddress address(url.GetHostName()+ ":" + PString(PString::Unsigned, url.GetPort()), ep.GetDefaultSignalPort(), "udp$");
  return address;
}


OpalTransportAddress SIP_PDU::GetSendAddress(const PStringList & routeSet)
{
  if (!routeSet.IsEmpty()) {

    SIPURL firstRoute = routeSet.front();
    if (firstRoute.GetParamVars().Contains("lr")) {
      return OpalTransportAddress(firstRoute.GetHostAddress());
    }
  }

  return OpalTransportAddress();
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
  if (!transport.IsReliable())
    transport.SetReadsPerPDU(1);

  // Do this to force a Read() by the PChannelBuffer outside of the
  // ios::lock() mutex which would prevent simultaneous reads and writes.
  transport.SetReadTimeout(PMaxTimeInterval);
#if defined(__MWERKS__) || (__GNUC__ >= 3) || (_MSC_VER >= 1300) || defined(SOLARIS)
  if (transport.rdbuf()->pubseekoff(0, ios_base::cur) == streampos(-1))
#else
  if (transport.rdbuf()->seekoff(0, ios::cur, ios::in) == EOF)
#endif                  
    transport.clear(ios::badbit);

  if (!transport.IsOpen()) {
    PTRACE(1, "SIP\tAttempt to read PDU from closed tansport " << transport);
    return PFalse;
  }

  // get the message from transport into cmd and parse MIME
  transport.clear();
  PString cmd(512);
  if (!transport.bad() && !transport.eof()) {
    if (transport.IsReliable())
      transport.SetReadTimeout(3000);
    transport >> cmd >> mime;
  }

  if (transport.bad()) {
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
  // then read until plausible end of header marker
  PINDEX contentLength = mime.GetContentLength();

  // assume entity bodies can't be longer than a UDP packet
  if (contentLength > 1500) {
    PTRACE(2, "SIP\tImplausibly long Content-Length " << contentLength << " received on " << transport);
    return PFalse;
  }
  else if (contentLength < 0) {
    PTRACE(2, "SIP\tImpossible negative Content-Length on " << transport);
    return PFalse;
  }

  if (contentLength > 0)
    transport.read(entityBody.GetPointer(contentLength+1), contentLength);
  else if (!mime.IsContentLengthPresent()) {
    PBYTEArray pp;

#if defined(__MWERKS__) || (__GNUC__ >= 3) || (_MSC_VER >= 1300) || defined(SOLARIS)
    transport.rdbuf()->pubseekoff(0, ios_base::cur);
#else
    transport.rdbuf()->seekoff(0, ios::cur, ios::in);
#endif 

    PINDEX lrc = transport.GetLastReadCount();
    if (lrc == 0) 
      contentLength = 0;
    else {

      //store in pp ALL the PDU (from beginning)
      transport.read((char*)pp.GetPointer(lrc), lrc);
      PINDEX pos = 3;
      while (++pos < pp.GetSize() && !(pp[pos]=='\n' && pp[pos-1]=='\r' && pp[pos-2]=='\n' && pp[pos-3]=='\r'))
        ; //end of header is marked by "\r\n\r\n"

      if (pos<pp.GetSize())
        pos++;
      contentLength = PMAX(0,pp.GetSize() - pos);
      if (contentLength > 0)
        memcpy(entityBody.GetPointer(contentLength+1),pp.GetPointer()+pos,  contentLength);
      else
        contentLength = 0;
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
    }

    trace << " received: rem=" << transport.GetLastReceivedAddress()
          << ",local=" << transport.GetLocalAddress()
          << ",if=" << transport.GetLastReceivedInterface();

    if (PTrace::CanTrace(4))
      trace << '\n' << cmd << '\n' << mime << entityBody;

    trace << PTrace::End;
  }
#endif

  PBoolean removeSDP = PTrue;

  // 'application/' is case sensitive, 'sdp' is not
  PString ContentType = mime.GetContentType();
  if ((ContentType.Left(12) == "application/") && (ContentType.Mid(12) *= "sdp")) {
    sdp = new SDPSessionDescription();
    removeSDP = !sdp->Decode(entityBody);
  }

  if (removeSDP) {
    delete sdp;
    sdp = NULL;
  }

  return PTrue;
}


PBoolean SIP_PDU::Write(OpalTransport & transport, const OpalTransportAddress & remoteAddress)
{
  if (!transport.IsOpen()) {
    PTRACE(1, "SIP\tAttempt to write PDU to closed tansport " << transport);
    return PFalse;
  }

  if (!remoteAddress.IsEmpty() && !transport.GetRemoteAddress().IsEquivalent(remoteAddress)) {
    // skip transport identifier
    SIPURL hosturl = remoteAddress.Mid(remoteAddress.Find('$')+1);

    // Do a DNS SRV lookup
    hosturl.AdjustToDNS();

    OpalTransportAddress actualRemoteAddress = hosturl.GetHostAddress();
    PTRACE(3, "SIP\tAdjusting transport remote address to " << actualRemoteAddress);
    transport.SetRemoteAddress(actualRemoteAddress);
  }


  PString strPDU = Build();

#if PTRACING
  if (PTrace::CanTrace(3)) {
    ostream & trace = PTrace::Begin(3, __FILE__, __LINE__);

    trace << "SIP\tSending PDU ";

    if (!PTrace::CanTrace(4)) {
      if (method != NumMethods)
        trace << MethodNames[method] << ' ' << uri;
      else
        trace << (unsigned)statusCode << ' ' << info;
    }

    trace << " to: rem=" << transport.GetRemoteAddress()
          << ",local=" << transport.GetLocalAddress()
          << ",if=" << transport.GetInterface();

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

  if (sdp != NULL) {
    entityBody = sdp->Encode();
    mime.SetContentType("application/sdp");
  }

  mime.SetContentLength(entityBody.GetLength());

  if (method != NumMethods)
    str << MethodNames[method] << ' ' << uri << ' ';

  str << "SIP/" << versionMajor << '.' << versionMinor;

  if (method == NumMethods)
    str << ' ' << (unsigned)statusCode << ' ' << info;

  str << "\r\n"
      << setfill('\r') << mime << setfill(' ')
      << entityBody;
  return str;
}


PString SIP_PDU::GetTransactionID() const
{
  // sometimes peers put <> around address, use GetHostAddress on GetFrom to handle all cases
  SIPURL fromURL(mime.GetFrom());
  return mime.GetCallID() + fromURL.GetHostAddress().ToLower() + PString(mime.GetCSeq());
}


////////////////////////////////////////////////////////////////////////////////////

SIPTransaction::SIPTransaction(SIPEndPoint & ep,
                               OpalTransport & trans,
                               const PTimeInterval & minRetryTime,
                               const PTimeInterval & maxRetryTime)
  : endpoint(ep),
    transport(trans)
{
  connection = (SIPConnection *)NULL;
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

  if (connection != NULL)
    connection->GetAuthenticator().Authorise(*this); 

  PSafeLockReadWrite lock(*this);

  state = Trying;
  retry = 0;
  localInterface = transport.GetInterface();


  bool stat = false;

  PStringList routeSet = this->GetMIME().GetRoute(); // Get the route set from the PDU

  // Use the connection transport to send the request
  if (connection != NULL) 
    stat = connection->SendPDU(*this, GetSendAddress(routeSet));
  else 
    stat = Write(transport, GetSendAddress(routeSet));

  if (!stat) {
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
      pdu.Write(transport) &&
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

  // If is the response to a CANCEl we sent, then just ignore it
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

    completed.Signal();
    completionTimer = endpoint.GetPduCleanUpTimeout();
  }

  if (notCompletedFlag) {
    if (connection != NULL)
      connection->OnReceivedResponse(*this, response);
    else
      endpoint.OnReceivedResponse(*this, response);

    if (state == Completed)
      return OnCompleted(response);
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

  if (!lock.IsLocked() || state != Trying)
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

  sdp = new SDPSessionDescription();
  if (!connection.OnSendSDP(false, rtpSessions, *sdp)) {
    delete sdp;
    sdp = NULL;
  }

  connection.OnCreatingINVITE(*this);
}


SIPInvite::SIPInvite(SIPConnection & connection, OpalTransport & transport, RTP_SessionManager & sm)
  : SIPTransaction(connection, transport, Method_INVITE)
{
  mime.SetDate() ;                             // now
  mime.SetProductInfo(connection.GetEndPoint().GetUserAgent(), connection.GetProductInfo());

  rtpSessions = sm;
  sdp = new SDPSessionDescription();
  if (!connection.OnSendSDP(false, rtpSessions, *sdp)) {
    delete sdp;
    sdp = NULL;
  }

  connection.OnCreatingINVITE(*this);
}


PBoolean SIPInvite::OnReceivedResponse(SIP_PDU & response)
{
  unsigned statusClass = response.GetStatusCode()/100;

  {
    PSafeLockReadWrite lock(*this);
    if (!lock.IsLocked())
      return false;

    // ACK Constructed following 17.1.1.3
    if (statusClass == 2) {
      SIPAck ack(*this);
      if (!SendPDU(ack))
        return false;
    }
    else if (statusClass > 2) {
      SIPAck ack(endpoint, *this, response);
      if (!SendPDU(ack))
        return false;
    }
  }

  if (!SIPTransaction::OnReceivedResponse(response))
    return false;

  PSafeLockReadWrite lock(*this);
  if (!lock.IsLocked())
    return false;

  if (statusClass == 1)
    completionTimer = PTimeInterval(0, mime.GetExpires(180));

  /* Handle response to outgoing call cancellation */
  if (response.GetStatusCode() == Failure_RequestTerminated)
    SetTerminated(Terminated_Success);

  return true;
}


SIPRegister::Params::Params()
  : m_expire(0)
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
  mime.SetContact(contact);

  mime.SetExpires(params.m_expire);

  SetRoute(routeSet);
}


SIPSubscribe::SIPSubscribe(SIPEndPoint & ep,
                           OpalTransport & trans,
                           SIPSubscribe::SubscribeType & type,
                           const PStringList & routeSet,
                           const SIPURL & targetAddress,
                           const PString & remotePartyAddress,
                           const PString & localPartyAddress,
                           const PString & id,
                           const unsigned & cseq,
                           unsigned expires)
  : SIPTransaction(ep, trans)
{
  PString acceptField;
  PString eventField;
  
  switch (type) {
  case MessageSummary:
    eventField = "message-summary";
    acceptField = "application/simple-message-summary";
    break;
    
  default:
  case Presence:
    eventField = "presence";
    acceptField = "application/pidf+xml";
    break;
  }

  SIPURL address = targetAddress;
  address.AdjustForRequestURI();

  OpalTransportAddress viaAddress = ep.GetLocalURL(transport).GetHostAddress();
  SIP_PDU::Construct(Method_SUBSCRIBE,
                     address,
                     remotePartyAddress,
                     localPartyAddress,
                     id,
                     cseq,
                     viaAddress);

  SIPURL contact = 
    endpoint.GetLocalURL(trans, SIPURL(localPartyAddress).GetUserName());
  mime.SetProductInfo(ep.GetUserAgent(), ep.GetProductInfo());
  mime.SetContact(contact);
  mime.SetAccept(acceptField);
  mime.SetEvent(eventField);
  mime.SetExpires(expires);

  SetRoute(routeSet);
}

SIPPublish::SIPPublish(SIPEndPoint & ep,
                       OpalTransport & trans,
                       const PStringList & /*routeSet*/,
                       const SIPURL & targetAddress,
                       const PString & sipIfMatch,
                       const PString & body,
                       unsigned expires)
  : SIPTransaction(ep, trans)
{
  PString addrStr = targetAddress.AsQuotedString();
  PString id = OpalGloballyUniqueID().AsString();
  id += "@" + PIPSocket::GetHostName();
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
                       const PString & body)
  : SIPTransaction(ep, trans)
{
  PString id = OpalGloballyUniqueID().AsString() + "@" + PIPSocket::GetHostName();
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

SIPAck::SIPAck(SIPEndPoint & ep,
               SIPTransaction & invite,
               SIP_PDU & response)
  : SIP_PDU (SIP_PDU::Method_ACK,
             invite.GetURI(),
             response.GetMIME().GetTo(),
             invite.GetMIME().GetFrom(),
             invite.GetMIME().GetCallID(),
             invite.GetMIME().GetCSeqIndex(),
             ep.GetLocalURL(invite.GetTransport()).GetHostAddress()),
  transaction(invite)
{
  Construct();
  // Use the topmost via header from the INVITE we ACK as per 17.1.1.3
  // as well as the initial Route
  PStringList viaList = invite.GetMIME().GetViaList();
  if (viaList.GetSize() > 0)
    mime.SetVia(viaList.front());

  if (transaction.GetMIME().GetRoute().GetSize() > 0)
    mime.SetRoute(transaction.GetMIME().GetRoute());
}


SIPAck::SIPAck(SIPTransaction & invite)
  : SIP_PDU (SIP_PDU::Method_ACK,
             *invite.GetConnection(),
             invite.GetTransport()),
  transaction(invite)
{
  mime.SetCSeq(PString(invite.GetMIME().GetCSeqIndex()) & MethodNames[Method_ACK]);
  Construct();
}


void SIPAck::Construct()
{
  // Add authentication if had any on INVITE
  if (transaction.GetMIME().Contains("Proxy-Authorization") || transaction.GetMIME().Contains("Authorization"))
    transaction.GetConnection()->GetAuthenticator().Authorise(*this);
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
