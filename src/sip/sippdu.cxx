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
 * $Log: sippdu.cxx,v $
 * Revision 1.2030  2004/04/25 09:32:15  rjongbloed
 * Fixed incorrect read of zero length SIP body, thanks Nick Hoath
 *
 * Revision 2.28  2004/03/23 09:43:42  rjongbloed
 * Fixed new C++ stream I/O compatibility, thanks Ted Szoczei
 *
 * Revision 2.27  2004/03/20 09:11:52  rjongbloed
 * Fixed probelm if inital read of stream fr SIP PDU fails. Should not then read again using
 *   >> operator as this then blocks the write due to the ios built in mutex.
 * Added timeout for "inter-packet" characters received. Waits forever for the first byte of
 *   a SIP PDU, then only waits a short time for the rest. This helps with getting a deadlock
 *   if a remote fails to send a full PDU.
 *
 * Revision 2.26  2004/03/16 12:06:11  rjongbloed
 * Changed SIP command URI to always be same as "to" address, not sure if this is correct though.
 *
 * Revision 2.25  2004/03/14 10:14:13  rjongbloed
 * Changes to REGISTER to support authentication
 *
 * Revision 2.24  2004/03/14 08:34:10  csoutheren
 * Added ability to set User-Agent string
 *
 * Revision 2.23  2004/03/13 06:32:18  rjongbloed
 * Fixes for removal of SIP and H.323 subsystems.
 * More registration work.
 *
 * Revision 2.22  2004/03/09 12:09:56  rjongbloed
 * More work on SIP register.
 *
 * Revision 2.21  2004/02/24 11:35:25  rjongbloed
 * Bullet proofed reply parsing for if get a command we don't understand.
 *
 * Revision 2.20  2003/12/16 10:22:45  rjongbloed
 * Applied enhancements, thank you very much Ted Szoczei
 *
 * Revision 2.19  2003/12/15 11:56:17  rjongbloed
 * Applied numerous bug fixes, thank you very much Ted Szoczei
 *
 * Revision 2.18  2003/03/19 00:47:06  robertj
 * GNU 3.2 changes
 *
 * Revision 2.17  2002/09/12 06:58:34  robertj
 * Removed protocol prefix strings as static members as has problems with
 *   use in DLL environment.
 *
 * Revision 2.16  2002/07/08 12:48:54  craigs
 * Do not set Record-Route if it is empty.
 *    Thanks to "Babara" <openh323@objectcrafts.org>
 *
 * Revision 2.15  2002/04/18 02:49:20  robertj
 * Fixed checking the correct state when overwriting terminated transactions.
 *
 * Revision 2.14  2002/04/17 07:24:12  robertj
 * Stopped complteion timer if transaction terminated.
 * Fixed multiple terminations so only the first version is used.
 *
 * Revision 2.13  2002/04/16 09:05:39  robertj
 * Fixed correct Route field setting depending on target URI.
 * Fixed some GNU warnings.
 *
 * Revision 2.12  2002/04/16 07:53:15  robertj
 * Changes to support calls through proxies.
 *
 * Revision 2.11  2002/04/15 08:54:46  robertj
 * Fixed setting correct local UDP port on cancelling INVITE.
 *
 * Revision 2.10  2002/04/12 12:22:45  robertj
 * Allowed for endpoint listener that is not on port 5060.
 *
 * Revision 2.9  2002/04/10 08:12:52  robertj
 * Added call back for when transaction completed, used for invite descendant.
 *
 * Revision 2.8  2002/04/10 03:16:23  robertj
 * Major changes to RTP session management when initiating an INVITE.
 * Improvements in error handling and transaction cancelling.
 *
 * Revision 2.7  2002/04/09 01:02:14  robertj
 * Fixed problems with restarting INVITE on  authentication required response.
 *
 * Revision 2.6  2002/04/05 10:42:04  robertj
 * Major changes to support transactions (UDP timeouts and retries).
 *
 * Revision 2.5  2002/03/18 08:09:31  robertj
 * Changed to use new SetXXX functions in PURL in normalisation.
 *
 * Revision 2.4  2002/03/08 06:28:03  craigs
 * Changed to allow Authorisation to be included in other PDUs
 *
 * Revision 2.3  2002/02/13 02:32:00  robertj
 * Fixed use of correct Decode function and error detection on parsing SDP.
 *
 * Revision 2.2  2002/02/11 07:36:23  robertj
 * Changed SDP to use OpalTransport for hosts instead of IP addresses/ports
 * Added media bypass for streams between compatible protocols.
 *
 * Revision 2.1  2002/02/01 04:53:01  robertj
 * Added (very primitive!) SIP support.
 *
 */

#include <ptlib.h>

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


#define	SIP_VER_MAJOR	2
#define	SIP_VER_MINOR	0


#define new PNEW


////////////////////////////////////////////////////////////////////////////

static const char * const MethodNames[SIP_PDU::NumMethods] = {
  "INVITE",
  "ACK",
  "OPTIONS",
  "BYE",
  "CANCEL",
  "REGISTER",
};

static struct {
  int code;
  const char * desc;
} sipErrorDescriptions[] = {
  { SIP_PDU::Information_Trying,                  "Trying" },
  { SIP_PDU::Information_Ringing,                 "Ringing" },
  { SIP_PDU::Information_CallForwarded,           "CallForwarded" },
  { SIP_PDU::Information_Queued,                  "Queued" },

  { SIP_PDU::Successful_OK,                       "OK" },

  { SIP_PDU::Redirection_MovedTemporarily,        "Moved Temporarily" },

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
  { SIP_PDU::Failure_BadExtension,                "Bad Extension" },
  { SIP_PDU::Failure_TemporarilyUnavailable,      "Temporarily Unavailable" },
  { SIP_PDU::Failure_TransactionDoesNotExist,     "Call Leg/Transaction Does Not Exist" },
  { SIP_PDU::Failure_LoopDetected,                "Loop Detected" },
  { SIP_PDU::Failure_TooManyHops,                 "Too Many Hops" },
  { SIP_PDU::Failure_AddressIncomplete,           "Address Incomplete" },
  { SIP_PDU::Failure_Ambiguous,                   "Ambiguous" },
  { SIP_PDU::Failure_BusyHere,                    "Busy Here" },
  { SIP_PDU::Failure_RequestTerminated,           "Request Terminated" },

  { SIP_PDU::Failure_BadGateway,                  "Bad Gateway" },

  { SIP_PDU::Failure_Decline,                     "Decline" },

  { 0 }
};


static const char * const AlgorithmNames[SIPAuthentication::NumAlgorithms] = {
  "md5"
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
  if (strncmp(name, "sip", 3) == 0)
    Parse(name);
  else {
    PIPSocket::Address ip;
    WORD port;
    if (address.GetIpAndPort(ip, port)) {
      PStringStream s;
      s << "sip:" << name << '@' << ip << ':';
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


BOOL SIPURL::InternalParse(const char * cstr, const char * defaultScheme)
{
  if (defaultScheme == NULL)
    defaultScheme = "sip";

  displayName = PString::Empty();

  PString str = cstr;

  // see if URL is just a URI or it contains a display address as well
  PINDEX start = str.FindLast('<');
  PINDEX end = str.FindLast('>');

  // see if URL is just a URI or it contains a display address as well
  if (start == P_MAX_INDEX || end == P_MAX_INDEX) {
    if (!PURL::InternalParse(cstr, defaultScheme))
      return FALSE;
  }
  else {
    // get the URI from between the angle brackets
    if (!PURL::InternalParse(str(start+1, end-1), defaultScheme))
      return FALSE;

    // extract the display address
    end = str.FindLast('"', start);
    start = str.FindLast('"', end-1);
    if (start == P_MAX_INDEX && end == P_MAX_INDEX)
      displayName = str.Left(start-1).Trim();
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


OpalTransportAddress SIPURL::GetHostAddress() const
{
  PString addr = paramVars("transport", "udp") + '$';

  if (paramVars.Contains("maddr"))
    addr += paramVars["maddr"];
  else
    addr += hostname;

  if (port != 0)
    addr.sprintf(":%u", port);

  return addr;
}


void SIPURL::AdjustForRequestURI()
{
  paramVars.RemoveAt("tag");
  queryVars.RemoveAll();
  Recalculate();
}


/////////////////////////////////////////////////////////////////////////////

SIPMIMEInfo::SIPMIMEInfo(BOOL _compactForm)
  : compactForm(_compactForm)
{
}


PINDEX SIPMIMEInfo::GetContentLength() const
{
  PString len = GetFullOrCompact("Content-Length", 'l');
  if (len.IsEmpty())
    return P_MAX_INDEX;
  return len.AsInteger();
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
  SetAt(compactForm ? "v" : "Via",  v);
}


void SIPMIMEInfo::SetContentLength(PINDEX v)
{
  SetAt(compactForm ? "l" : "Content-Length", PString(PString::Unsigned, v));
}


PString SIPMIMEInfo::GetCSeq() const
{
  return (*this)("CSeq");
}


void SIPMIMEInfo::SetCSeq(const PString & v)
{
  SetAt("CSeq",  v);
}


PStringList SIPMIMEInfo::GetRoute() const
{
  return GetRouteList("Route");
}


void SIPMIMEInfo::SetRoute(const PStringList & v)
{
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

  for (PINDEX i = 0; i < v.GetSize(); i++) {
    if (i > 0)
      s << ',';
    s << '<' << v[i] << '>';
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


PString SIPMIMEInfo::GetUserAgent() const
{
  return (*this)(PCaselessString("User-Agent"));        // no compact form
}


void SIPMIMEInfo::SetUserAgent(const SIPEndPoint & sipep)
{
  SetAt("User-Agent", sipep.GetUserAgent());      // no compact form
}


PString SIPMIMEInfo::GetWWWAuthenticate() const
{
  return (*this)(PCaselessString("WWW-Authenticate"));  // no compact form
}


void SIPMIMEInfo::SetWWWAuthenticate(const PString & v)
{
  SetAt("WWW-Authenticate",  v);        // no compact form
}


PString SIPMIMEInfo::GetFullOrCompact(const char * fullForm, char compactForm) const
{
  if (Contains(PCaselessString(fullForm)))
    return (*this)[fullForm];
  return (*this)(PCaselessString(compactForm));
}


////////////////////////////////////////////////////////////////////////////////////

SIPAuthentication::SIPAuthentication(const PString & user, const PString & pwd)
  : username(user),
    password(pwd)
{
  algorithm = NumAlgorithms;
  isProxy = FALSE;
}


static PString GetAuthParam(const PString & auth, const char * name)
{
  PString value;

  PINDEX pos = auth.Find(name);
  if (pos != P_MAX_INDEX)  {
    pos += strlen(name);
    while (isspace(auth[pos]))
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
        while (auth[pos] != '\0' && !isspace(auth[pos]))
          pos++;
        value = auth(base, pos-1);
      }
    }
  }

  return value;
}


BOOL SIPAuthentication::Parse(const PCaselessString & auth, BOOL proxy)
{
  realm.Empty();
  nonce.Empty();
  algorithm = NumAlgorithms;

  if (auth.Find("digest") != 0) {
    PTRACE(1, "SIP\tUnknown authentication type");
    return FALSE;
  }

  PCaselessString str = GetAuthParam(auth, "algorithm");
  if (str.IsEmpty())
    algorithm = Algorithm_MD5;  // default
  else if (str == "md5")
    algorithm = Algorithm_MD5;
  else {
    PTRACE(1, "SIP\tUnknown authentication algorithm");
    return FALSE;
  }

  realm = GetAuthParam(auth, "realm");
  if (realm.IsEmpty()) {
    PTRACE(1, "SIP\tNo realm in authentication");
    return FALSE;
  }

  nonce = GetAuthParam(auth, "nonce");
  if (nonce.IsEmpty()) {
    PTRACE(1, "SIP\tNo nonce in authentication");
    return FALSE;
  }

  isProxy = proxy;
  return TRUE;
}


BOOL SIPAuthentication::IsValid() const
{
  return !realm && !username && !nonce && algorithm < NumAlgorithms;
}


static PString AsHex(PMessageDigest5::Code & digest)
{
  PStringStream out;
  out << hex << setfill('0');
  for (PINDEX i = 0; i < 16; i++)
    out << setw(2) << (unsigned)((BYTE *)&digest)[i];
  return out;
}


BOOL SIPAuthentication::Authorise(SIP_PDU & pdu) const
{
  if (!IsValid()) {
    PTRACE(2, "SIP\tNo authentication information available");
    return FALSE;
  }

  PTRACE(2, "SIP\tAdding authentication information");

  PMessageDigest5 digestor;
  PMessageDigest5::Code a1, a2, response;

  PString uriText = pdu.GetURI().AsString();
  PINDEX pos = uriText.Find(";");
  if (pos != P_MAX_INDEX)
    uriText = uriText.Left(pos);

  digestor.Start();
  digestor.Process(username);
  digestor.Process(":");
  digestor.Process(realm);
  digestor.Process(":");
  digestor.Process(password);
  digestor.Complete(a1);

  digestor.Start();
  digestor.Process(MethodNames[pdu.GetMethod()]);
  digestor.Process(":");
  digestor.Process(uriText);
  digestor.Complete(a2);

  digestor.Start();
  digestor.Process(AsHex(a1));
  digestor.Process(":");
  digestor.Process(nonce);
  digestor.Process(":");
  digestor.Process(AsHex(a2));
  digestor.Complete(response);

  PStringStream auth;
  auth << "Digest "
          "username=\"" << username << "\", "
          "realm=\"" << realm << "\", "
          "nonce=\"" << nonce << "\", "
          "uri=\"" << uriText << "\", "
          "response=\"" << AsHex(response) << "\", "
          "algorithm=" << AlgorithmNames[algorithm];

  pdu.GetMIME().SetAt(isProxy ? "Proxy-Authorization" : "Authorization", auth);
  return TRUE;
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


SIP_PDU::SIP_PDU(const SIP_PDU & request, StatusCodes code, const char * extra)
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

  // format response
  if (extra != NULL)
    info = extra;
  else {
    PINDEX i;
    for (i = 0; (extra == NULL) && (sipErrorDescriptions[i].code != 0); i++) {
      if (sipErrorDescriptions[i].code == code) {
        info = sipErrorDescriptions[i].desc;
        break;
      }
    }
  }
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
  Construct(meth);

  uri = dest;
  uri.AdjustForRequestURI();

  mime.SetTo(to);
  mime.SetFrom(from);
  mime.SetCallID(callID);
  mime.SetCSeq(PString(cseq) & MethodNames[method]);

  PINDEX dollar = via.Find('$');

  PStringStream str;
  str << "SIP/" << versionMajor << '.' << versionMinor << '/'
      << via.Left(dollar).ToUpper() << ' ';
  PIPSocket::Address ip;
  WORD port;
  if (via.GetIpAndPort(ip, port))
    str << ip << ':' << port;
  else
    str << via.Mid(dollar+1);

  mime.SetVia(str);
}


void SIP_PDU::Construct(Methods meth,
                        SIPConnection & connection,
                        const OpalTransport & transport)
{
  PIPSocket::Address ip;
  WORD port;
  if (transport.IsRunning())
    transport.GetLocalAddress().GetIpAndPort(ip, port);
  else
    connection.GetEndPoint().GetListeners()[0].GetLocalAddress().GetIpAndPort(ip, port);

  OpalTransportAddress localAddress = connection.GetLocalAddress(port);
  SIPURL contact(connection.GetLocalPartyName(), localAddress);
  mime.SetContact(contact);

  Construct(meth,
            connection.GetRemotePartyAddress(),
            connection.GetRemotePartyAddress(),
            connection.GetLocalPartyAddress(),
            connection.GetToken(),
            connection.GetNextCSeq(),
            localAddress);

  PStringList routeSet = connection.GetRouteSet();
  if (!routeSet.IsEmpty()) {
    SIPURL firstRoute = routeSet[0];
    if (!firstRoute.GetParamVars().Contains("lr")) {
      routeSet.MakeUnique();
      routeSet.RemoveAt(0);
      routeSet.AppendString(uri.AsString());
      uri = firstRoute;
      uri.AdjustForRequestURI();
    }
    mime.SetRoute(routeSet);
  }
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


BOOL SIP_PDU::Read(OpalTransport & transport)
{
  // Do this to force a Read() by the PChannelBuffer outside of the
  // ios::lock() mutex which would prevent simultaneous reads and writes.
  transport.SetReadTimeout(PMaxTimeInterval);
#if defined(__MWERKS__) || (__GNUC__ >= 3) || (_MSC_VER >= 1300)
  if (transport.rdbuf()->pubseekoff(0, ios_base::cur) == streampos(_BADOFF))
#else
  if (transport.rdbuf()->seekoff(0, ios::cur, ios::in) == EOF)
#endif                                                   
    transport.clear(ios::badbit);

  PString cmd;
  if (transport.good()) {
    transport.SetReadTimeout(3000);
    transport >> cmd >> mime;
  }

  if (!transport.good()) {
    PTRACE_IF(1, transport.GetErrorCode(PChannel::LastReadError) != PChannel::NoError,
              "Read SIP PDU failed: " << transport.GetErrorText(PChannel::LastReadError));
    return FALSE;
  }

  if (cmd.IsEmpty()) {
    PTRACE(1, "SIP\tNo Request-Line or Status-Line received on " << transport);
    return FALSE;
  }

  if (cmd.Left(4) *= "SIP/") {
    PINDEX space = cmd.Find(' ');
    if (space == P_MAX_INDEX) {
      PTRACE(1, "SIP\tBad Status-Line received on " << transport);
      return FALSE;
    }

    versionMajor = cmd.Mid(4).AsUnsigned();
    versionMinor = cmd(cmd.Find('.')+1, space).AsUnsigned();
    statusCode = (StatusCodes)cmd.Mid(++space).AsUnsigned();
    info    = cmd.Mid(cmd.Find(' ', space));
    uri     = PString();
  }
  else {
    // parse the method, URI and version
    PStringArray cmds = cmd.Tokenise( ' ', FALSE);
    if (cmds.GetSize() < 3) {
      PTRACE(1, "SIP\tBad Request-Line received on " << transport);
      return FALSE;
    }

    int i = 0;
    while (!(cmds[0] *= MethodNames[i])) {
      i++;
      if (i >= NumMethods) {
        PTRACE(1, "SIP\tUnknown method name " << cmds[0] << " received on " << transport);
        return FALSE;
      }
    }
    method = (Methods)i;

    uri = cmds[1];
    versionMajor = cmds[2].Mid(4).AsUnsigned();
    versionMinor = cmds[2].Mid(cmds[2].Find('.')+1).AsUnsigned();
    info = PString();
  }

  if (versionMajor < 2) {
    PTRACE(1, "SIP\tInvalid version received on " << transport);
    return FALSE;
  }

  PINDEX contentLength = mime.GetContentLength();
  if (contentLength > 0)
    transport.read(entityBody.GetPointer(contentLength+1), contentLength);
  entityBody[contentLength] = '\0';

  BOOL removeSDP = TRUE;

  if (mime.GetContentType() == "application/sdp") {
    sdp = new SDPSessionDescription();
    removeSDP = !sdp->Decode(entityBody);
  }

  if (removeSDP) {
    delete sdp;
    sdp = NULL;
  }

#if PTRACING
  if (PTrace::CanTrace(4)) {
    PTRACE(4, "Received SIP PDU on " << transport << "\n"
           << cmd << '\n' << mime << entityBody);
  }
  else {
    PTRACE(3, "Received SIP PDU: " << cmd << " on " << transport);
  }
#endif

  return TRUE;
}


BOOL SIP_PDU::Write(OpalTransport & transport)
{
  if (sdp != NULL) {
    entityBody = sdp->Encode();
    mime.SetContentType("application/sdp");
  }

  mime.SetContentLength(entityBody.GetLength());

  PStringStream str;

  if (method != NumMethods)
    str << MethodNames[method] << ' ' << uri << ' ';

  str << "SIP/" << versionMajor << '.' << versionMinor;

  if (method == NumMethods)
    str << ' ' << (unsigned)statusCode << ' ' << info;

  str << "\r\n"
      << setfill('\r') << mime << setfill(' ')
      << entityBody;

#if PTRACING
  if (PTrace::CanTrace(4))
    PTRACE(4, "Sending SIP PDU on " << transport << '\n' << str);
  else if (!method)
    PTRACE(3, "Sending SIP PDU: " << MethodNames[method] << ' ' << uri << " on " << transport);
  else
    PTRACE(3, "Sending SIP PDU: " << (unsigned)statusCode << ' ' << info << " on " << transport);
#endif

  if (transport.WriteString(str))
    return TRUE;

  PTRACE(1, "Write SIP PDU failed: " << transport.GetErrorText(PChannel::LastWriteError));
  return FALSE;
}


PString SIP_PDU::GetTransactionID() const
{
  // sometimes peers put <> around address, use GetHostAddress on GetFrom to handle all cases
  SIPURL fromURL(mime.GetFrom());
  return fromURL.GetHostAddress().ToLower() + PString(mime.GetCSeqIndex());
}


////////////////////////////////////////////////////////////////////////////////////

SIPTransaction::SIPTransaction(SIPEndPoint & ep,
                               OpalTransport & trans)
  : endpoint(ep),
    transport(trans)
{
  connection = NULL;
  Construct();
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
}


void SIPTransaction::Construct()
{
  retryTimer.SetNotifier(PCREATE_NOTIFIER(OnRetry));
  completionTimer.SetNotifier(PCREATE_NOTIFIER(OnTimeout));

  retry = 1;
  state = NotStarted;
}


SIPTransaction::~SIPTransaction()
{
  if (connection != NULL && state > NotStarted && state < Terminated_Success) {
    PTRACE(3, "SIP\tTransaction " << mime.GetCSeq() << " aborted.");
    connection->RemoveTransaction(this);
  }
}


BOOL SIPTransaction::Start()
{
  if (state != NotStarted) {
    PAssertAlways(PLogicError);
    return FALSE;
  }

  if (connection != NULL) {
    connection->AddTransaction(this);
    connection->GetAuthentication().Authorise(*this);
  }
  else {
    endpoint.AddTransaction(this);
    endpoint.GetAuthentication().Authorise(*this);
  }

  PWaitAndSignal m(mutex);

  state = Trying;
  retry = 0;
  retryTimer = endpoint.GetRetryTimeoutMin();
  completionTimer = endpoint.GetNonInviteTimeout();
  localAddress = transport.GetLocalAddress();

  if (method == Method_REGISTER || transport.SetRemoteAddress(uri.GetHostAddress())) {
    if (Write(transport))
      return TRUE;
  }

  SetTerminated(Terminated_TransportError);
  return FALSE;
}


void SIPTransaction::Wait()
{
  if (state == NotStarted)
    Start();

  finished.Wait();
}


BOOL SIPTransaction::SendCANCEL()
{
  PWaitAndSignal m(mutex);

  if (state == NotStarted || state >= Cancelling)
    return FALSE;

  return ResendCANCEL();
}


BOOL SIPTransaction::ResendCANCEL()
{
  SIP_PDU cancel(Method_CANCEL,
                 uri,
                 mime.GetTo(),
                 mime.GetFrom(),
                 mime.GetCallID(),
                 mime.GetCSeqIndex(),
                 localAddress);

  if (!transport.SetLocalAddress(localAddress) || !cancel.Write(transport)) {
    SetTerminated(Terminated_TransportError);
    return FALSE;
  }

  if (state < Cancelling) {
    state = Cancelling;
    retry = 0;
    retryTimer = endpoint.GetRetryTimeoutMin();
  }

  return TRUE;
}


BOOL SIPTransaction::OnReceivedResponse(SIP_PDU & response)
{
  PWaitAndSignal m(mutex);

  PString cseq = response.GetMIME().GetCSeq();

  // If is the response to a CANCEl we sent, then just ignore it
  if (cseq.Find(MethodNames[Method_CANCEL]) != P_MAX_INDEX) {
    SetTerminated(Terminated_Cancelled);
    return FALSE;
  }

  // Something wrong here, response is not for the request we made!
  if (cseq.Find(MethodNames[method]) == P_MAX_INDEX) {
    PTRACE(3, "SIP\tTransaction " << cseq << " response not for " << *this);
    return FALSE;
  }

  /* Really need to check if response is actually meant for us. Have a
     temporary cheat in assuming that we are only sending a given CSeq to one
     and one only host, so anything coming back with that CSeq is OK. This has
     "issues" according to the spec but
     */
  if (response.GetStatusCode()/100 == 1) {
    PTRACE(3, "SIP\tTransaction " << cseq << " proceeding.");

    if (connection != NULL)
      connection->OnReceivedResponse(*this, response);
    else
      endpoint.OnReceivedResponse(*this, response);

    state = Proceeding;
    retry = 0;
    retryTimer = endpoint.GetRetryTimeoutMax();
    completionTimer = endpoint.GetNonInviteTimeout();
  }
  else {
    PTRACE(3, "SIP\tTransaction " << cseq << " completed.");

    if (!OnCompleted(response))
      return FALSE;

    if (state < Completed && connection != NULL)
      connection->OnReceivedResponse(*this, response);
    else
      endpoint.OnReceivedResponse(*this, response);

    state = Completed;
    retryTimer.Stop();
    completionTimer = endpoint.GetPduCleanUpTimeout();
    finished.Signal();  // keep this at the end to prevent deletion before actual completion
  }

  return TRUE;
}


BOOL SIPTransaction::OnCompleted(SIP_PDU & /*response*/)
{
  return TRUE;
}


void SIPTransaction::OnRetry(PTimer &, INT)
{
  PWaitAndSignal m(mutex);

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
  else if (!transport.SetLocalAddress(localAddress) || !Write(transport)) {
    SetTerminated(Terminated_TransportError);
    return;
  }

  PTimeInterval timeout = endpoint.GetRetryTimeoutMin()*(1<<retry);
  if (timeout > endpoint.GetRetryTimeoutMax())
    retryTimer = endpoint.GetRetryTimeoutMax();
  else
    retryTimer = timeout;
}


void SIPTransaction::OnTimeout(PTimer &, INT)
{
  mutex.Wait();
  SetTerminated(state != Completed ? Terminated_Timeout : Terminated_Success);
  mutex.Signal();
}


void SIPTransaction::SetTerminated(States newState)
{
  retryTimer.Stop();
  completionTimer.Stop();

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
    "Terminated_Cancelled"
  };
#endif

  if (state >= Terminated_Success) {
    PTRACE(3, "SIP\tTried to set " << StateNames[newState] << " for " << mime.GetCSeq()
           << " but already terminated ( " << StateNames[state] << ')');
    return;
  }

  state = newState;
  PTRACE(3, "SIP\tSet " << StateNames[newState] << " for " << mime.GetCSeq());

  if (connection != NULL) {
    if (state != Terminated_Success)
      connection->OnTransactionFailed(*this);

    connection->RemoveTransaction(this);
  }

  finished.Signal();
}


////////////////////////////////////////////////////////////////////////////////////

SIPInvite::SIPInvite(SIPConnection & connection, OpalTransport & transport)
  : SIPTransaction(connection, transport, Method_INVITE)
{
  mime.SetDate() ;                             // now
  mime.SetUserAgent(connection.GetEndPoint()); // normally 'OPAL/2.0'
  mime.SetMaxForwards(70);                     // default

  sdp = connection.BuildSDP(rtpSessions, OpalMediaFormat::DefaultAudioSessionID);
}


BOOL SIPInvite::OnReceivedResponse(SIP_PDU & response)
{
  if (!SIPTransaction::OnReceivedResponse(response))
    return FALSE;

  if (response.GetStatusCode()/100 == 1) {
    retryTimer.Stop();
    completionTimer = PTimeInterval(0, mime.GetExpires(180));
  }
  else
    completionTimer = endpoint.GetAckTimeout();

  return TRUE;
}


BOOL SIPInvite::OnCompleted(SIP_PDU & response)
{
  // Adjust "to" field for possible added tag in response
  mime.SetTo(response.GetMIME().GetTo());

  // Build an ACK
  SIP_PDU ack(Method_ACK,
              uri,
              mime.GetTo(),
              mime.GetFrom(),
              mime.GetCallID(),
              mime.GetCSeqIndex(),
              localAddress);

  // Add authentication if had any on INVITE
  if (connection != NULL && 
          (mime.Contains("Proxy-Authorization") || mime.Contains("Authorization")))
    connection->GetAuthentication().Authorise(ack);

  // Send the ACK
  if (ack.Write(transport))
    return TRUE;

  SetTerminated(Terminated_TransportError);
  return FALSE;
}


SIPRegister::SIPRegister(SIPEndPoint & ep,
                         OpalTransport & trans,
                         const SIPURL & address,
                         const PString & id)
  : SIPTransaction(ep, trans)
{
  // translate contact address
  OpalTransportAddress contactAddress = transport.GetLocalAddress();
  WORD contactPort = endpoint.GetDefaultSignalPort();

  PIPSocket::Address localIP;
  if (transport.GetLocalAddress().GetIpAddress(localIP)) {
    PIPSocket::Address remoteIP;
    if (transport.GetRemoteAddress().GetIpAddress(remoteIP)) {
      endpoint.GetManager().TranslateIPAddress(localIP, remoteIP);
      contactAddress = OpalTransportAddress(localIP, contactPort, "udp");
    }
  }

  SIPURL contact(address.GetUserName(), contactAddress, contactPort);

  PString addrStr = address.AsQuotedString();
  SIP_PDU::Construct(Method_REGISTER,
                     "sip:"+address.GetHostName(),
                     addrStr,
                     addrStr,
                     id,
                     endpoint.GetNextCSeq(),
                     transport.GetLocalAddress());

  mime.SetContact(contact);
//  mime.SetExpires(60);
}


// End of file ////////////////////////////////////////////////////////////////
