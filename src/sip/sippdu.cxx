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
 * Revision 1.2007  2002/04/05 10:42:04  robertj
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
    fragment.Delete(0, P_MAX_INDEX);
    queryVars.RemoveAll();
  }

  if (!paramVars.Contains("transport"))
    paramVars.SetAt("transport", "udp");

  Recalculate();
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


void SIPMIMEInfo::SetContact(const PURL & url, const char * name)
{
  PStringStream s;
  if (name != NULL && *name != '\0')
    s << '"' << name << "\" ";
  s << '<' << url << '>';
  SetContact(s);
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


BOOL SIPAuthentication::Parse(const PString & auth, BOOL proxy)
{
  realm.Empty();
  nonce.Empty();
  algorithm = NumAlgorithms;

  if (auth.Find("digest") != 0) {
    PTRACE(1, "SIP\tUnknown proxy authentication scheme");
    return FALSE;
  }

  PCaselessString str = GetAuthParam(auth, "algorithm");
  if (str.IsEmpty())
    algorithm = Algorithm_MD5;  // default
  else if (str == "md5")
    algorithm = Algorithm_MD5;
  else {
    PTRACE(1, "SIP\tUnknown proxy authentication algorithm");
    return FALSE;
  }

  realm = GetAuthParam(auth, "realm");
  if (realm.IsEmpty()) {
    PTRACE(1, "SIP\tNo realm in proxy authentication");
    return FALSE;
  }

  nonce = GetAuthParam(auth, "nonce");
  if (nonce.IsEmpty()) {
    PTRACE(1, "SIP\tNo nonce in proxy authentication");
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
                 const PString & to,
                 const PString & from,
                 const PString & callID,
                 unsigned cseq,
                 const OpalTransport & via)
{
  Construct(method, to, from, callID, cseq, via);
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
  mime.SetTo(request.GetMIME().GetTo());
  mime.SetFrom(request.GetMIME().GetFrom());
  mime.SetCallID(request.GetMIME().GetCallID());
  mime.SetCSeq(request.GetMIME().GetCSeq());
  mime.SetVia(request.GetMIME().GetVia());

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
                        const PString & to,
                        const PString & from,
                        const PString & callID,
                        unsigned cseq,
                        const OpalTransport & via)
{
  Construct(meth);

  uri = to;
  mime.RemoveAll();
  mime.SetTo(to);
  mime.SetFrom(from);
  mime.SetCallID(callID);
  mime.SetCSeq(PString(cseq) & MethodNames[method]);

  OpalTransportAddress localAddress = via.GetLocalAddress();
  PIPSocket::Address ip;
  WORD port;
  localAddress.GetIpAndPort(ip, port);
  if (!via.IsRunning())
    port = 5060;

  PStringStream str;
  str << "SIP/" << versionMajor << '.' << versionMinor << '/'
      << localAddress.Left(localAddress.Find('$')).ToUpper()
      << ' ' << ip << ':' << port;
  mime.SetVia(str);
}


void SIP_PDU::Construct(Methods meth,
                        SIPConnection & connection,
                        const OpalTransport & transport)
{
  Construct(meth,
            connection.GetRemotePartyAddress(),
            connection.GetLocalPartyAddress(),
            connection.GetToken(),
            connection.GetNextCSeq(),
            transport);

  if (transport.IsRunning()) {
    PIPSocket::Address ip;
    WORD port;
    transport.GetLocalAddress().GetIpAndPort(ip, port);

    OpalTransportAddress localAddress = connection.GetLocalAddress(port);
    if (!localAddress) {
      SIPURL contact(connection.GetLocalPartyName(), localAddress);
      mime.SetContact(contact);
    }
    else
      mime.SetContact(connection.GetLocalPartyAddress(), "");
  }

  if (meth == Method_INVITE) {
    mime.SetContact(connection.GetLocalPartyAddress());
    mime.SetAt("Date", PTime().AsString());
    mime.SetAt("User-Agent", "OPAL/2.0");
    sdp = connection.BuildSDP();
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

    int i = 0;
    while (i < NumMethods && !(cmds[0] *= MethodNames[i]))
      i++;
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


PString SIP_PDU::GetTransactionID() const
{
  return mime.GetFrom() + PString(mime.GetCSeqIndex());
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
  if (connection != NULL && state < Terminated_Success) {
    PTRACE(3, "SIP\tTransaction " << mime.GetCSeq() << " aborted.");
    connection->RemoveTransaction(this);
  }
}


void SIPTransaction::BuildREGISTER(const SIPURL & name, const SIPURL & contact)
{
  PString str = name.AsString();
  SIP_PDU::Construct(Method_REGISTER,
                     str, str,
                     endpoint.GetRegistrationID(),
                     endpoint.GetNextCSeq(),
                     transport);

  mime.SetContact(contact);
  mime.SetAt("Expires", "60");
}


BOOL SIPTransaction::Start()
{
  if (connection != NULL) {
    connection->AddTransaction(this);
    connection->GetAuthentication().Authorise(*this);
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


void SIPTransaction::SendCANCEL()
{
  PWaitAndSignal m(mutex);

  if (state >= Completed)
    return;

  SIP_PDU cancel(Method_CANCEL,
                 mime.GetTo(),
                 mime.GetFrom(),
                 mime.GetCallID(),
                 mime.GetCSeqIndex(),
                 transport);
  if (cancel.Write(transport)) {
    if (state < Cancelling) {
      state = Cancelling;
      retry = 0;
      retryTimer = endpoint.GetRetryTimeoutMin();
    }
  }
  else
    SetTerminated(Terminated_TransportError);
}


void SIPTransaction::OnReceivedResponse(SIP_PDU & response)
{
  PWaitAndSignal m(mutex);

  PString cseq = response.GetMIME().GetCSeq();

  if (cseq.Find(MethodNames[Method_CANCEL]) != P_MAX_INDEX) {
    PTRACE(3, "SIP\tTransaction " << cseq << " acknowledged for " << *this);
    return;
  }

  if (cseq.Find(MethodNames[method]) == P_MAX_INDEX) {
    PTRACE(3, "SIP\tTransaction " << cseq << " response not for " << *this);
    return;
  }

  /* Really need to check if response is actually meant for us. Have a
     temporary cheat in assuming that we are only sending a given CSeq to one
     and one only host, so anything coming back with that CSeq is OK. This has
     "issues" according to the spec but
     */
  if (response.GetStatusCode()/100 == 1) {
    PTRACE(3, "SIP\tTransaction " << cseq << " proceeding.");
    state = Proceeding;
    retry = 0;
    if (method == Method_INVITE) {
      retryTimer.Stop();
      completionTimer = PTimeInterval(0, mime.GetInteger("Expires", 180));
    }
    else {
      retryTimer = endpoint.GetRetryTimeoutMax();
      completionTimer = endpoint.GetNonInviteTimeout();
    }
  }
  else {
    PTRACE(3, "SIP\tTransaction " << cseq << " completed.");
    state = Completed;
    retryTimer.Stop();
    if (method != Method_INVITE || response.GetStatusCode()/100 != 2)
      completionTimer = endpoint.GetPduCleanUpTimeout();
    else {
      completionTimer = endpoint.GetAckTimeout();
      mime.SetTo(response.GetMIME().GetTo()); // Adjust to get added tag
      SIP_PDU ack(Method_ACK,
                  mime.GetTo(),
                  mime.GetFrom(),
                  mime.GetCallID(),
                  mime.GetCSeqIndex(),
                  transport);
      ack.Write(transport);
    }

    finished.Signal();
  }

  if (connection != NULL)
    connection->OnReceivedResponse(*this, response);
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

  if (state == Cancelling)
    SendCANCEL();
  else {
    if (!Write(transport)) {
      SetTerminated(Terminated_TransportError);
      return;
    }
  }

  PTimeInterval t = endpoint.GetRetryTimeoutMin()*(1<<retry);
  if (t > endpoint.GetRetryTimeoutMax())
    retryTimer = endpoint.GetRetryTimeoutMax();
  else
    retryTimer = t;
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
  state = newState;

  if (connection != NULL) {
    if (state != Terminated_Success)
      connection->OnTransactionFailed(*this);

    connection->RemoveTransaction(this);
  }

  finished.Signal();
}


// End of file ////////////////////////////////////////////////////////////////
