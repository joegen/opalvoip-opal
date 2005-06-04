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
 * Revision 1.2056  2005/06/04 12:44:36  dsandras
 * Applied patch from Ted Szoczei to fix leaks and problems on cancelling a call and to improve the Allow PDU field handling.
 *
 * Revision 2.54  2005/05/23 20:14:04  dsandras
 * Added preliminary support for basic instant messenging.
 *
 * Revision 2.53  2005/05/06 07:37:06  csoutheren
 * Various changed while working with SIP carrier
 *   - remove assumption that authentication realm is a domain name.
 *   - stopped rewrite of "To" field when proxy being used
 *   - fix Contact field in REGISTER to match actual port used when Symmetric NATin use
 *   - lots of formatting changes and cleanups
 *
 * Revision 2.52  2005/05/02 20:12:32  dsandras
 * Use the first listener port as signaling port in the Contact field for REGISTER PDU's.
 *
 * Revision 2.51  2005/04/28 20:22:55  dsandras
 * Applied big sanity patch for SIP thanks to Ted Szoczei <tszoczei@microtronix.ca>.
 * Thanks a lot!
 *
 * Revision 2.50  2005/04/28 07:59:37  dsandras
 * Applied patch from Ted Szoczei to fix problem when answering to PDUs containing
 * multiple Via fields in the message header. Thanks!
 *
 * Revision 2.49  2005/04/15 14:01:39  dsandras
 * Added User Agent string in REGISTER and SUBSCRIBE PDUs.
 *
 * Revision 2.48  2005/04/15 10:48:34  dsandras
 * Allow reading on the transport until there is an EOF or it becomes bad. Fixes interoperability problem with QSC.DE which is sending keep-alive messages, leading to a timeout (transport.good() fails, but the stream is still usable).
 *
 * Revision 2.47  2005/04/11 10:38:14  dsandras
 * Added support for INVITE done with the same RTP Session for call HOLD.
 *
 * Revision 2.46  2005/04/11 10:37:14  dsandras
 * Added support for the MESSAGE PDU.
 *
 * Revision 2.45  2005/04/11 10:36:34  dsandras
 * Added support for REFER and its associated NOTIFY for blind transfer.
 *
 * Revision 2.44  2005/03/11 18:12:09  dsandras
 * Added support to specify the realm when registering. That way softphones already know what authentication information to use when required. The realm/domain can also be used in the From field.
 *
 * Revision 2.43  2005/02/19 22:48:48  dsandras
 * Added the possibility to register to several registrars and be able to do authenticated calls to each of them. Added SUBSCRIBE/NOTIFY support for Message Waiting Indications.
 *
 * Revision 2.42  2005/02/19 22:36:25  dsandras
 * Always send PDU's to the proxy when there is one.
 *
 * Revision 2.41  2005/01/16 11:28:06  csoutheren
 * Added GetIdentifier virtual function to OpalConnection, and changed H323
 * and SIP descendants to use this function. This allows an application to
 * obtain a GUID for any connection regardless of the protocol used
 *
 * Revision 2.40  2004/12/27 22:19:27  dsandras
 * Added Allow field to PDUs.
 *
 * Revision 2.39  2004/12/22 18:57:50  dsandras
 * Added support for Call Forwarding via the "302 Moved Temporarily" SIP response.
 *
 * Revision 2.38  2004/12/17 12:06:53  dsandras
 * Added error code to OnRegistrationFailed. Made Register/Unregister wait until the transaction is over. Fixed Unregister so that the SIPRegister is used as a pointer or the object is deleted at the end of the function and make Opal crash when transactions are cleaned. Reverted part of the patch that was sending authentication again when it had already been done on a Register.
 *
 * Revision 2.37  2004/12/12 13:44:38  dsandras
 * - Modified InternalParse so that the remote displayName defaults to the sip url when none is provided.
 * - Changed GetDisplayName accordingly.
 * - Added call to OnRegistrationFailed when the REGISTER fails for any reason.
 *
 * Revision 2.36  2004/11/29 06:53:25  csoutheren
 * Prevent attempt to read infinite size
 * buffer if no ContentLength specified in MIME
 *
 * Revision 2.35  2004/11/08 10:17:51  rjongbloed
 * Tidied some trace logs
 *
 * Revision 2.34  2004/10/25 23:28:28  csoutheren
 * Fixed problems with systems that use commas between authentication parameters
 *
 * Revision 2.33  2004/09/27 12:51:20  rjongbloed
 * Fixed deadlock in SIP transaction timeout
 *
 * Revision 2.32  2004/08/22 12:27:46  rjongbloed
 * More work on SIP registration, time to live refresh and deregistration on exit.
 *
 * Revision 2.31  2004/08/18 13:05:49  rjongbloed
 * Fixed indicating transaction finished before it really is. Possible crash if then delete object.
 *
 * Revision 2.30  2004/08/14 07:56:43  rjongbloed
 * Major revision to utilise the PSafeCollection classes for the connections and calls.
 *
 * Revision 2.29  2004/04/25 09:32:15  rjongbloed
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
  "SUBSCRIBE",
  "NOTIFY",
  "REFER",
  "MESSAGE"
};

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


const char * SIP_PDU::GetStatusCodeDescription (int code)
{
  unsigned i;
  for (i = 0; sipErrorDescriptions[i].code != 0; i++) {
    if (sipErrorDescriptions[i].code == code)
	  return sipErrorDescriptions[i].desc;
  }
  return 0;
}


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
    if (!PURL::InternalParse(cstr, defaultScheme)) {
      return FALSE;
    }
  }
  else {
    // get the URI from between the angle brackets
    if (!PURL::InternalParse(str(start+1, end-1), defaultScheme))
      return FALSE;

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


PString SIPURL::GetDisplayName () const
{
  PString s;
  PINDEX tag;
    
  s = displayName;
  
  if (displayName.IsEmpty ()) {
    
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
    return 0; //P_MAX_INDEX;
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


PStringList SIPMIMEInfo::GetViaList() const
{
  PStringList viaList;
  PString s = GetVia();
  if (s.FindOneOf("\r\n") != P_MAX_INDEX)
    viaList = s.Lines();
  else
    viaList.AppendString(s);

  return viaList;
}


void SIPMIMEInfo::SetViaList(const PStringList & v)
{
  PString fieldValue;
  if (v.GetSize() > 0)
  {
    fieldValue = v[0];
    for (PINDEX i = 1; i < v.GetSize(); i++)
      fieldValue += '\n' + v[i];
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
  : username(user), password(pwd)
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


BOOL SIPAuthentication::Parse(const PCaselessString & auth, BOOL proxy)
{
  authRealm.Empty();
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

  authRealm = GetAuthParam(auth, "realm");
  if (authRealm.IsEmpty()) {
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


BOOL SIPAuthentication::Authorise(SIP_PDU & pdu) const
{
  if (!IsValid()) {
    PTRACE(2, "SIP\tNo authentication information present");
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
  digestor.Process(authRealm);
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
          "realm=\"" << authRealm << "\", "
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


SIP_PDU::SIP_PDU(const SIP_PDU & request, 
		 StatusCodes code, 
		 const char * contact,
		 const char * extra)
{
  char *extraInfo = NULL;
 
  method       = NumMethods;
  statusCode   = code;
  versionMajor = request.GetVersionMajor();
  versionMinor = request.GetVersionMinor();

  extraInfo = (char *) extra;
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
    SIPURL contact(extraInfo);
    mime.SetContact(contact.AsQuotedString ());
    extraInfo = NULL;
  }
  else if (contact != NULL) {
    mime.SetContact(PString(contact));
  }
    
  // format response
  if (extraInfo != NULL) {
    info = extraInfo;
  }
  else {
    PINDEX i;
    for (i = 0; (extraInfo == NULL) && (sipErrorDescriptions[i].code != 0); i++) {
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
  PString allMethods;
  
  Construct(meth);

  uri = dest;
  uri.AdjustForRequestURI();

  mime.SetTo(to);
  mime.SetFrom(from);
  mime.SetCallID(callID);
  mime.SetCSeq(PString(cseq) & MethodNames[method]);

  // construct Via:
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

  SetAllow();
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

  SetRoute(connection);
}


BOOL SIP_PDU::SetRoute(SIPConnection & connection)
{
  PStringList routeSet = connection.GetRouteSet();
  if (!routeSet.IsEmpty()) {
    SIPURL firstRoute = routeSet[0];
    if (!firstRoute.GetParamVars().Contains("lr")) {
      // this procedure is specified in RFC3261:12.2.1.1 for backwards compatibility with RFC2543
      routeSet.MakeUnique();
      routeSet.RemoveAt(0);
      routeSet.AppendString(uri.AsString());
      uri = firstRoute;
      uri.AdjustForRequestURI();
    }
    mime.SetRoute(routeSet);
	return TRUE;
  }
  return FALSE;
}


void SIP_PDU::SetAllow(void)
{
  PString methods;
  methods = MethodNames [0];
  for (PINDEX i = 1 ; i < SIP_PDU::NumMethods ; i++)
    methods = methods + ", " + MethodNames[i];

  mime.SetAllow(methods);
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

  // get the message from transport into cmd and parse MIME
  transport.clear();
  PString cmd;
  if (!transport.bad() && !transport.eof()) {
    transport.SetReadTimeout(3000);
    transport >> cmd >> mime;
  }

  if (transport.bad()) {
    PTRACE_IF(1, transport.GetErrorCode(PChannel::LastReadError) != PChannel::NoError,
              "SIP\tPDU Read failed: " << transport.GetErrorText(PChannel::LastReadError));
    return FALSE;
  }

  if (cmd.IsEmpty()) {
    PTRACE(1, "SIP\tNo Request-Line or Status-Line received on " << transport);
    return FALSE;
  }

  if (cmd.Left(4) *= "SIP/") {
    // parse Response version, code & reason (ie: "SIP/2.0 200 OK")
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

  // get the SDP content
  PINDEX contentLength = mime.GetContentLength();
  if (contentLength > 0)
    transport.read(entityBody.GetPointer(contentLength+1), contentLength);
  entityBody[contentLength] = '\0';

  BOOL removeSDP = TRUE;

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

#if PTRACING
  if (PTrace::CanTrace(4))
    PTRACE(4, "SIP\tPDU Received on " << transport << "\n"
           << cmd << '\n' << mime << entityBody);
  else
    PTRACE(3, "SIP\tPDU Received " << cmd << " on " << transport);
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
    PTRACE(4, "SIP\tSending PDU on " << transport << '\n' << str);
  else if (method != NumMethods)
    PTRACE(3, "SIP\tSending PDU " << MethodNames[method] << ' ' << uri << " on " << transport);
  else
    PTRACE(3, "SIP\tSending PDU " << (unsigned)statusCode << ' ' << info << " on " << transport);
#endif

  if (transport.WriteString(str))
    return TRUE;

  PTRACE(1, "SIP\tPDU Write failed: " << transport.GetErrorText(PChannel::LastWriteError));
  return FALSE;
}


PString SIP_PDU::GetTransactionID() const
{
  // sometimes peers put <> around address, use GetHostAddress on GetFrom to handle all cases
  SIPURL fromURL(mime.GetFrom());
  return fromURL.GetHostAddress().ToLower() + PString(mime.GetCSeq());
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
  PTRACE(3, "SIP\tTransaction " << mime.GetCSeq() << " created.");
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
  retryTimer.Stop();
  completionTimer.Stop();

  if (connection != NULL && state > NotStarted && state < Terminated_Success) {
    PTRACE(3, "SIP\tTransaction " << mime.GetCSeq() << " aborted.");
    connection->RemoveTransaction(this);
  }
  PTRACE(3, "SIP\tTransaction " << mime.GetCSeq() << " destroyed.");
}


BOOL SIPTransaction::Start()
{
  if (state != NotStarted) {
    PAssertAlways(PLogicError);
    return FALSE;
  }

  if (connection != NULL) {
    connection->AddTransaction(this);
    connection->GetAuthenticator().Authorise(*this); 
  }
  else {
    endpoint.AddTransaction(this);
    //We authorise the PDU when authentication is required
  }

  PWaitAndSignal m(mutex);

  state = Trying;
  retry = 0;
  retryTimer = endpoint.GetRetryTimeoutMin();
  completionTimer = endpoint.GetNonInviteTimeout();
  localAddress = transport.GetLocalAddress();

  if (Write(transport))
    return TRUE;

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
  }

  return TRUE;
}


BOOL SIPTransaction::OnCompleted(SIP_PDU & /*response*/)
{
  return TRUE;
}


void SIPTransaction::OnRetry(PTimer &, INT)
{
  /* There is a potential deadlock if a reply packet comes in at the exact
     same time as the timeout. So, put a timeout on the mutex grab, if it
     fails then we had the deadlock condition, in which case the retry
     timeout can be safely ignored as the PDU states are already handled.
     */
  if (!mutex.Wait(100)) {
    PTRACE(3, "SIP\tTransaction " << mime.GetCSeq() << " timeout ignored.");
    return;
  }

  PWaitAndSignal m(mutex, false);

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
  /* There is a potential deadlock if a reply packet comes in at the exact
     same time as the timeout. So, put a timeout on the mutex grab, if it
     fails then we had the deadlock condition, in which case the retry
     timeout can be safely ignored as the PDU states are already handled.
   */
  if (mutex.Wait(100)) {
    SetTerminated(state != Completed ? Terminated_Timeout : Terminated_Success);
    mutex.Signal();
  }
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
    PTRACE(3, "SIP\tTried to set state " << StateNames[newState] << " for transaction " << mime.GetCSeq()
           << " but already terminated ( " << StateNames[state] << ')');
    return;
  }

  state = newState;
  PTRACE(3, "SIP\tSet state " << StateNames[newState] << " for transaction " << mime.GetCSeq());

  if (connection != NULL) {
    if (state != Terminated_Success)
      connection->OnTransactionFailed(*this);

    connection->RemoveTransaction(this);
  }
  else {
    endpoint.RemoveTransaction(this);
  }
    

  // REGISTER or MESSAGE Failed, tell the endpoint
  if (state != Terminated_Success) {
    
    if (GetMethod() == SIP_PDU::Method_REGISTER) {
      
      SIPURL url (GetMIME().GetFrom ());

      endpoint.OnRegistrationFailed(url.GetHostName(), 
				    url.GetUserName(),
				    SIP_PDU::Failure_RequestTimeout,
				    (GetMIME().GetExpires(0) > 0));
    }
    else if (GetMethod() == SIP_PDU::Method_MESSAGE) {

      SIPURL url (GetMIME().GetTo ());

      endpoint.OnMessageFailed(url,
			       SIP_PDU::Failure_RequestTimeout);
    }
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


SIPInvite::SIPInvite(SIPConnection & connection, OpalTransport & transport, RTP_SessionManager & sm)
  : SIPTransaction(connection, transport, Method_INVITE)
{
  mime.SetDate() ;                             // now
  mime.SetUserAgent(connection.GetEndPoint()); // normally 'OPAL/2.0'
  mime.SetMaxForwards(70);                     // default

  rtpSessions = sm;
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

  /* Handle response to outgoing call cancellation */
  if (response.GetStatusCode() == Failure_RequestTerminated)
    SetTerminated(Terminated_Success);

  return TRUE;
}


BOOL SIPInvite::OnCompleted(SIP_PDU & response)
{
  // Adjust "to" field for possible added tag in response
  mime.SetTo(response.GetMIME().GetTo());

  // Use Contact for request URI as specified in RFC 3261:12.1.2, 12.2.1.1
  PString contact = response.GetMIME().GetContact();
  SIPURL targetAddress = contact;

  // Build an ACK
  SIP_PDU ack(Method_ACK,
              targetAddress,
              mime.GetTo(),
              mime.GetFrom(),
              mime.GetCallID(),
              mime.GetCSeqIndex(),
              localAddress);

  BOOL targetChange = FALSE;
  if (connection != NULL) {
    // Add authentication if had any on INVITE
    if (mime.Contains("Proxy-Authorization") || mime.Contains("Authorization"))
      connection->GetAuthenticator().Authorise(ack);

    if (ack.SetRoute(*connection) == FALSE)
      targetChange = TRUE;
  }
  // In peer-to-peer the transport will have been sending to the called party.
  // When calling thru proxy the transport will have been sending to the proxy.
  // If the proxy does not want to handle signalling any more, there will
  // be no Record-Route - to indicate that signalling should now be sent 
  // directly to the called party. (RFC 3261:4)
  if (targetChange) {
    transport.SetRemoteAddress(targetAddress.GetHostAddress());
    PTRACE(4, "SIP\tNon-recording proxy changed remote address of transport " << transport);
  }
  // Send the ACK
  if (ack.Write(transport))
    return TRUE;

  SetTerminated(Terminated_TransportError);
  return FALSE;
}


SIPRegister::SIPRegister(SIPEndPoint & ep,
                         OpalTransport & trans,
                         const SIPURL & address,
                         const PString & id,
                         unsigned expires)
  : SIPTransaction(ep, trans)
{
  // translate contact address
  PIPSocket::Address ip(PIPSocket::GetDefaultIpAny());
  OpalTransportAddress contactAddress = transport.GetLocalAddress();
  WORD contactPort = endpoint.GetDefaultSignalPort();
  if (!ep.GetListeners().IsEmpty())
    ep.GetListeners()[0].GetLocalAddress().GetIpAndPort(ip, contactPort);

  PIPSocket::Address localIP;
  WORD localPort;
  if (transport.GetLocalAddress().GetIpAndPort(localIP, localPort)) {
    PIPSocket::Address remoteIP;
    if (transport.GetRemoteAddress().GetIpAddress(remoteIP)) {
      PIPSocket::Address _localIP(localIP);
      endpoint.GetManager().TranslateIPAddress(localIP, remoteIP);
      if (localIP != _localIP)
        contactPort = localPort;
      contactAddress = OpalTransportAddress(localIP, contactPort, "udp");
    }
  }

  // Find the correct From/To fields
  // changed CRS 6/5/05
  //PString addrStr = ep.GetRegisteredPartyName(address.GetHostName()).AsQuotedString();
  //if (addrStr.IsEmpty())
  PString addrStr = address.AsQuotedString();
  SIP_PDU::Construct(Method_REGISTER,
                     "sip:"+address.GetHostName(),
                     addrStr,
                     addrStr,
                     id,
                     endpoint.GetNextCSeq(),
                     transport.GetLocalAddress());

  mime.SetUserAgent(ep); // normally 'OPAL/2.0'
  SIPURL contact(address.GetUserName(), contactAddress, contactPort);
  mime.SetContact(contact);
  mime.SetExpires(expires);
}


SIPMWISubscribe::SIPMWISubscribe(SIPEndPoint & ep,
				 OpalTransport & trans,
				 const SIPURL & address,
				 const PString & id,
				 unsigned expires)
  : SIPTransaction(ep, trans)
{
  // translate contact address
  PIPSocket::Address ip(PIPSocket::GetDefaultIpAny());
  OpalTransportAddress contactAddress = transport.GetLocalAddress();
  WORD contactPort = endpoint.GetDefaultSignalPort();
  if (!ep.GetListeners().IsEmpty())
    ep.GetListeners()[0].GetLocalAddress().GetIpAndPort(ip, contactPort);

  PIPSocket::Address localIP;
  WORD localPort;
  if (transport.GetLocalAddress().GetIpAndPort(localIP, localPort)) {
    PIPSocket::Address remoteIP;
    if (transport.GetRemoteAddress().GetIpAddress(remoteIP)) {
      PIPSocket::Address _localIP(localIP);
      endpoint.GetManager().TranslateIPAddress(localIP, remoteIP);
      if (localIP != localIP)
        contactPort = localPort;
      contactAddress = OpalTransportAddress(localIP, contactPort, "udp");
    }
  }

  // Find the correct From/To fields
  // changed CRS 6/5/05
  //PString addrStr = 
  //  ep.GetRegisteredPartyName(address.GetHostName()).AsQuotedString();
  //if (addrStr.IsEmpty())

  PString addrStr = address.AsQuotedString();
  SIP_PDU::Construct(Method_SUBSCRIBE,
                     "sip:"+address.GetUserName()+"@"+address.GetHostName(),
                     addrStr,
                     addrStr,
                     id,
                     endpoint.GetNextCSeq(),
                     transport.GetLocalAddress());

  SIPURL contact(address.GetUserName(), contactAddress, contactPort);
  mime.SetUserAgent(ep); // normally 'OPAL/2.0'
  mime.SetContact(contact);
  mime.SetAccept("application/simple-message-summary");
  mime.SetEvent("message-summary");
  mime.SetExpires(expires);
}


/////////////////////////////////////////////////////////////////////////

SIPRefer::SIPRefer(SIPConnection & connection, OpalTransport & transport, const PString & refer)
  : SIPTransaction(connection, transport, Method_REFER)
{
  mime.SetUserAgent(connection.GetEndPoint()); // normally 'OPAL/2.0'
  mime.SetReferTo(refer);
}


/////////////////////////////////////////////////////////////////////////

SIPReferNotify::SIPReferNotify(SIPConnection & connection, OpalTransport & transport, StatusCodes code)
  : SIPTransaction(connection, transport, Method_NOTIFY)
{
  PStringStream str;
  
  mime.SetUserAgent(connection.GetEndPoint()); // normally 'OPAL/2.0'
  mime.SetSubscriptionState("terminated;reason=noresource"); // Do not keep
  							     // an internal state
  mime.SetEvent("refer");
  mime.SetContentType("message/sipfrag;version=2.0");


  str << "SIP/" << versionMajor << '.' << versionMinor << " " << code << " " << sipErrorDescriptions[code].desc;
  entityBody = str;
}


/////////////////////////////////////////////////////////////////////////

SIPMessage::SIPMessage(SIPEndPoint & ep,
		       OpalTransport & trans,
		       const SIPURL & address,
		       const PString & body)
  : SIPTransaction(ep, trans)
{
  PString id = OpalGloballyUniqueID().AsString() + "@" + PIPSocket::GetHostName();
    
  // Build the correct From field
  int port = 0;
  PString displayName = ep.GetDefaultDisplayName();
  PString localName = endpoint.GetRegisteredPartyName(SIPURL(address).GetHostName()).GetUserName(); 
  PString domain = endpoint.GetRegisteredPartyName(SIPURL(address).GetHostName()).GetHostName();

  // If no domain, use the local domain as default
  if (domain.IsEmpty()) {
    domain = trans.GetLocalAddress().GetHostName();
    if (port != endpoint.GetDefaultSignalPort())
      domain += psprintf(":%d", port);
  }
  if (localName.IsEmpty())
    localName = ep.GetDefaultLocalPartyName ();

  SIPURL myAddress("\"" + displayName + "\" <" + localName + "@" + domain + ">"); 
  
  SIP_PDU::Construct(Method_MESSAGE,
                     "sip:"+address.GetUserName()+"@"+address.GetHostName(),
                     address.AsQuotedString(),
                     myAddress.AsQuotedString(),
                     id,
                     endpoint.GetNextCSeq(),
                     transport.GetLocalAddress());
  mime.SetContentType("text/plain;charset=UTF-8");

  entityBody = body;
}

// End of file ////////////////////////////////////////////////////////////////
