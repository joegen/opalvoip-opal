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
 * Revision 1.2005  2002/03/08 06:28:03  craigs
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
#include <opal/connection.h>
#include <opal/transports.h>


#define	SIP_VER_MAJOR	2
#define	SIP_VER_MINOR	0


#define new PNEW


////////////////////////////////////////////////////////////////////////////

class SIPErrorDef {
  public:
    int code;
    const char * desc;
};

static SIPErrorDef sipErrorDescriptions[] = {
  { SIP_PDU::Information_Trying,                       "Trying" },
  { SIP_PDU::Information_Ringing,                      "Ringing" },
  { SIP_PDU::Information_CallForwarded,                "CallForwarded" },
  { SIP_PDU::Information_Queued,                       "Queued" },

  { SIP_PDU::Successful_OK,                            "OK" },

  { SIP_PDU::Redirection_MovedTemporarily,             "Moved Temporarily" },

  { SIP_PDU::Failure_BadRequest,                       "BadRequest" },
  { SIP_PDU::Failure_UnAuthorised,                     "Unauthorised" },
  { SIP_PDU::Failure_PaymentRequired,                  "Payment Required" },
  { SIP_PDU::Failure_Forbidden,                        "Forbidden" },
  { SIP_PDU::Failure_NotFound,                         "Not Found" },
  { SIP_PDU::Failure_MethodNotAllowed,                 "Method Not Allowed" },
  { SIP_PDU::Failure_NotAcceptable,                    "Not Acceptable" },
  { SIP_PDU::Failure_ProxyAuthenticationRequired,      "Proxy Authentication Required" },
  { SIP_PDU::Failure_RequestTimeout,                   "Request Timeout" },
  { SIP_PDU::Failure_Conflict,                         "Conflict" },
  { SIP_PDU::Failure_Gone,                             "Gone" },
  { SIP_PDU::Failure_LengthRequired,                   "Length Required" },
  { SIP_PDU::Failure_RequestEntityTooLarge,            "Request Entity Too Large" },
  { SIP_PDU::Failure_RequestURITooLong,                "Request URI Too Long" },
  { SIP_PDU::Failure_UnsupportedMediaType,             "Unsupported Media Type" },
  { SIP_PDU::Failure_BadExtension,                     "Bad Extension" },
  { SIP_PDU::Failure_TemporarilyUnavailable,           "Temporarily Unavailable" },
  { SIP_PDU::Failure_CallLegOrTransactionDoesNotExist, "Call Leg/Transaction Does Not Exist" },
  { SIP_PDU::Failure_LoopDetected,                     "Loop Detected" },
  { SIP_PDU::Failure_TooManyHops,                      "Too Many Hops" },
  { SIP_PDU::Failure_AddressIncomplete,                "Address Incomplete" },
  { SIP_PDU::Failure_Ambiguous,                        "Ambiguous" },
  { SIP_PDU::Failure_BusyHere,                         "Busy Here" },
  { 0 }
};


/////////////////////////////////////////////////////////////////////////////

SIPURL::SIPURL()
{
}


SIPURL::SIPURL(const char * str, BOOL special)
{
  Parse(str, special);
}


SIPURL::SIPURL(const PString & str, BOOL special)
{
  Parse(str, special);
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
      if (strncmp(address, OpalTransportAddress::TcpPrefix, strlen(OpalTransportAddress::TcpPrefix)) == 0)
        s << "tcp";
      else
        s << "udp";
      Parse(s);
    }
  }
}


void SIPURL::Parse(const char * cstr, BOOL special)
{
  displayAddress.Delete(0, P_MAX_INDEX);

  PString str = cstr;

  PINDEX start = str.FindLast('<');
  PINDEX end = str.FindLast('>');

  // see if URL is just a URI or it contains a display address as well
  if (start == P_MAX_INDEX || end == P_MAX_INDEX)
    PURL::Parse(cstr);
  else {
    PURL::Parse(str(start+1, end-1));

    // extract the display address
    end = str.FindLast('"', start);
    start = str.FindLast('"', end-1);
    if (start == P_MAX_INDEX && end == P_MAX_INDEX)
      displayAddress = str.Left(start-1).Trim();
    else if (start != P_MAX_INDEX && end != P_MAX_INDEX) {
      // No trim quotes off
      displayAddress = str(start+1, end-1);
      while ((start = displayAddress.Find('\\')) != P_MAX_INDEX)
        displayAddress.Delete(start, 1);
    }
  }

  if (!(scheme *= "sip")) {
    PURL::Parse("");
    return;
  }

  if (special) {
    PString userParam = paramVars("user", "udp");
    paramVars.RemoveAll();
    paramVars.SetAt("user", userParam);
    parameters = "user=" + userParam;
    fragment.Delete(0, P_MAX_INDEX);
    queryStr.Delete(0, P_MAX_INDEX);
    queryVars.RemoveAll();
  }

  if (!paramVars.Contains("transport")) {
    paramVars.SetAt("transport", "udp");
    if (!parameters)
      parameters += ';';
    parameters += "transport=udp";
  }
}


OpalTransportAddress SIPURL::GetHostAddress() const
{
  PString addr;
  if (paramVars("transport") == "tcp")
    addr = OpalTransportAddress::TcpPrefix;
  else
    addr = OpalTransportAddress::UdpPrefix;

  addr += hostname;
  addr.sprintf(":%u", port);
  return addr;
}


/////////////////////////////////////////////////////////////////////////////

SIPMIMEInfo::SIPMIMEInfo(BOOL _compactForm)
  : compactForm(_compactForm)
{
}


PINDEX SIPMIMEInfo::GetContentLength() const
{
  PString len = GetSIPString("Content-Length", "l");
  if (len.IsEmpty())
    return P_MAX_INDEX;
  return len.AsInteger();
}


PString SIPMIMEInfo::GetSIPString(const char * fullForm, const char * compactForm) const
{
  if (Contains(PCaselessString(fullForm)))
    return (*this)[fullForm];
  return (*this)(compactForm);
}


void SIPMIMEInfo::SetContentType(const PString & v)
{
  this->SetAt(compactForm ? "c" : "Content-Type",  v);
}


void SIPMIMEInfo::SetContentEncoding(const PString & v)
{
  this->SetAt(compactForm ? "e" : "Content-Encoding",  v);
}


void SIPMIMEInfo::SetFrom(const PString & v)
{
  this->SetAt(compactForm ? "f" : "From",  v);
}


void SIPMIMEInfo::SetCallID(const PString & v)
{
  this->SetAt(compactForm ? "i" : "Call-ID",  v);
}


void SIPMIMEInfo::SetContact(const PString & v)
{
  this->SetAt(compactForm ? "m" : "Contact",  v);
}


void SIPMIMEInfo::SetSubject(const PString & v)
{
  this->SetAt(compactForm ? "s" : "Subject",  v);
}


void SIPMIMEInfo::SetTo(const PString & v)
{
  this->SetAt(compactForm ? "t" : "To",  v);
}


void SIPMIMEInfo::SetVia(const PString & v)
{
  this->SetAt(compactForm ? "v" : "Via",  v);
}


void SIPMIMEInfo::SetContentLength(PINDEX v)
{
  this->SetAt(compactForm ? "l" : "Content-Length", PString(PString::Unsigned, v));
}


void SIPMIMEInfo::SetCSeq(const PString & v)
{
  this->SetAt("CSeq",  v);
}


PINDEX SIPMIMEInfo::GetCSeqIndex() const
{
  PString str = GetCSeq().Trim();
  PINDEX pos = str.Find(' ');
  if (pos != P_MAX_INDEX)
    str = str.Left(pos);

  return str.AsInteger();
}


////////////////////////////////////////////////////////////////////////////////////

SIP_PDU::SIP_PDU(OpalTransport & trans, SIPConnection * conn)
  : transport(trans)
{
  versionMajor = SIP_VER_MAJOR;
  versionMinor = SIP_VER_MINOR;

  sdp = NULL;
  connection = conn;

////////  mime.SetForm(_ep.GetMIMEForm());
}


SIP_PDU::SIP_PDU(const SIP_PDU & request, StatusCodes code, const char * extra)
  : transport(request.GetTransport())
{
  statusCode = code;
  versionMajor = request.GetVersionMajor();
  versionMinor = request.GetVersionMinor();

  sdp = NULL;
  connection = request.GetConnection();

  // add mandatory fields to response (RFC 2543, 11.2)
  mime.SetTo(request.GetMIME().GetTo());
  mime.SetFrom(request.GetMIME().GetFrom());
  mime.SetCallID(request.GetMIME().GetCallID());
  mime.SetCSeq(request.GetMIME().GetCSeq());
  mime.SetVia(request.GetMIME().GetVia());

  PIPSocket::Address address;
  WORD port;
  PAssert(transport.GetLocalAddress().GetIpAndPort(address, port), "Only IPv4 supported!");
  PStringStream myAddr;
  myAddr << "<sip:" << address << ':' << port << '>';
  mime.SetContact(myAddr);

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


SIP_PDU::~SIP_PDU()
{
  delete sdp;
}


BOOL SIP_PDU::Read()
{
  // Do this to force a Read() by the PChannelBuffer outside of the
  // ios::lock() mutex which would prevent simultaneous reads and writes.
  transport.rdbuf()->underflow();

  PString cmd;
  transport >> cmd >> mime;

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

    method  = cmds[0];
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


BOOL SIP_PDU::Write()
{
  if (sdp != NULL) {
    entityBody = sdp->Encode();
    mime.SetContentType("application/sdp");
  }

  mime.SetContentLength(entityBody.GetLength());

  PStringStream str;

  if (!method)
    str << method << ' ' << uri << ' ';

  str << "SIP/" << versionMajor << '.' << versionMinor;

  if (method.IsEmpty())
    str << ' ' << statusCode << ' ' << info;

  str << "\r\n"
      << setfill('\r') << mime << setfill(' ')
      << entityBody;

#if PTRACING
  if (PTrace::CanTrace(4))
    PTRACE(4, "Sending SIP PDU on " << transport << '\n' << str);
  else if (!method)
    PTRACE(3, "Sending SIP PDU: " << method << ' ' << uri << " on " << transport);
  else
    PTRACE(3, "Sending SIP PDU: " << statusCode << ' ' << info << " on " << transport);
#endif

  if (transport.WriteString(str))
    return TRUE;

  PTRACE(1, "Write SIP PDU failed: " << transport.GetErrorText(PChannel::LastWriteError));
  return FALSE;
}


void SIP_PDU::BuildVia()
{
  OpalTransportAddress localAddress = transport.GetLocalAddress();
  PIPSocket::Address ip;
  WORD port;
  localAddress.GetIpAndPort(ip, port);
  PStringStream via;
  via << "SIP/" << versionMajor << '.' << versionMinor << '/'
      << localAddress.Left(localAddress.Find('$')).ToUpper()
      << ' ' << ip << ':' << port;
  mime.SetVia(via);
}


void SIP_PDU::BuildCommon()
{
  uri = connection->GetRemotePartyAddress();

  mime.RemoveAll();
  mime.SetTo(connection->GetRemotePartyAddress());
  mime.SetFrom(connection->GetLocalPartyAddress());
  mime.SetCallID(connection->GetToken());
  mime.SetCSeq(PString(connection->GetLastSentCSeq()) & method);

  BuildVia();

  PIPSocket::Address ip;
  WORD port;
  transport.GetLocalAddress().GetIpAndPort(ip, port);

  OpalTransportAddress localAddress = connection->GetLocalAddress(port);
  if (!localAddress) {
    SIPURL contact(connection->GetLocalPartyName(), localAddress);
    mime.SetContact(contact.AsString());
  }
  else
    mime.SetContact(connection->GetLocalPartyAddress());

  mime.SetAt("Date", PTime().AsString());
}


void SIP_PDU::BuildINVITE()
{
  method = "INVITE";
  BuildCommon();

  mime.SetAt("User-Agent", "OPAL/2.0");
  mime.SetContentType("application/sdp");
  sdp = connection->BuildSDP();
}


void SIP_PDU::BuildBYE()
{
  method = "BYE";
  BuildCommon();
}


void SIP_PDU::BuildACK()
{
  method = "ACK";
  BuildCommon();
}


void SIP_PDU::BuildREGISTER(const SIPEndPoint & endpoint,
                            const SIPURL & name,
                            const SIPURL & contact)
{
  method = "REGISTER";
  uri = name;

  PString str = name.AsString();

  mime.RemoveAll();
  mime.SetTo(str);
  mime.SetFrom(str);
  mime.SetContact('<'+contact.AsString()+'>');
  mime.SetCallID(endpoint.GetRegistrationID());
  mime.SetCSeq(PString(endpoint.GetLastSentCSeq()) & method);
  mime.SetAt("Expires", "60");

  BuildVia();
}


void SIP_PDU::SendResponse(StatusCodes code, const char * extra)
{
  SIP_PDU response(*this, code, extra);
  response.Write();
}


// End of file ////////////////////////////////////////////////////////////////
