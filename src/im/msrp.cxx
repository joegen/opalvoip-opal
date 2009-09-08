/*
 * msrp.cxx
 *
 * Support for RFC 4975 Message Session Relay Protocol (MSRP)
 *
 * Open Phone Abstraction Library (OPAL)
 *
 * Copyright (c) 2008 Post Increment
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
 * The Initial Developer of the Original Code is Post Increment
 *
 * Contributor(s): ______________________________________.
 *
 * $Revision$
 * $Author$
 * $Date$
 */

#include <ptlib.h>
#include <opal/buildopts.h>

#ifdef __GNUC__
#pragma implementation "msrp.h"
#endif

#include <ptlib/socket.h>
#include <ptclib/random.h>
#include <ptclib/pdns.h>
#include <ptclib/mime.h>

#include <opal/transports.h>
#include <opal/mediatype.h>
#include <opal/mediafmt.h>
#include <opal/endpoint.h>

#include <im/im.h>
#include <im/msrp.h>

#if OPAL_HAS_MSRP

#define CRLF "\r\n"

OpalMSRPMediaType::OpalMSRPMediaType()
  : OpalIMMediaType("msrp", "message|tcp/msrp", 5)
{
}

#if OPAL_SIP

/////////////////////////////////////////////////////////
//
//  SDP media description for the MSRP type
//
//  A new class is needed for "message" due to the following differences
//
//  - the SDP type is "message"
//  - the transport is "tcp/msrp"
//  - the format list is always "*". The actual supported formats are defined by the a=accept-types attribute
//  - the OpalMediaFormats for the IM types have no RTP encoding names
//

class SDPMSRPMediaDescription : public SDPMediaDescription
{
  PCLASSINFO(SDPMSRPMediaDescription, SDPMediaDescription);
  public:
    SDPMSRPMediaDescription(const OpalTransportAddress & address);
    SDPMSRPMediaDescription(const OpalTransportAddress & address, const PString & url);

    PCaselessString GetSDPTransportType() const
    { return "tcp/msrp"; }

    virtual PString GetSDPMediaType() const 
    { return "message"; }

    virtual PString GetSDPPortList() const
    { return " *"; }

    virtual void CreateSDPMediaFormats(const PStringArray &);
    virtual bool PrintOn(ostream & str, const PString & connectString) const;
    virtual void SetAttribute(const PString & attr, const PString & value);
    virtual void ProcessMediaOptions(SDPMediaFormat & sdpFormat, const OpalMediaFormat & mediaFormat);
    virtual void AddMediaFormat(const OpalMediaFormat & mediaFormat);

    virtual OpalMediaFormatList GetMediaFormats() const;

    // CreateSDPMediaFormat is used for processing format lists. MSRP always contains only "*"
    virtual SDPMediaFormat * CreateSDPMediaFormat(const PString & ) { return NULL; }

    // FindFormat is used only for rtpmap and fmtp, neither of which are used for MSRP
    virtual SDPMediaFormat * FindFormat(PString &) const { return NULL; }

  protected:
    PString path;
    PString types;
};

////////////////////////////////////////////////////////////////////////////////////////////

SDPMediaDescription * OpalMSRPMediaType::CreateSDPMediaDescription(const OpalTransportAddress & localAddress)
{
  return new SDPMSRPMediaDescription(localAddress);
}

///////////////////////////////////////////////////////////////////////////////////////////

SDPMSRPMediaDescription::SDPMSRPMediaDescription(const OpalTransportAddress & address)
  : SDPMediaDescription(address)
{
  SetDirection(SDPMediaDescription::SendRecv);
}

SDPMSRPMediaDescription::SDPMSRPMediaDescription(const OpalTransportAddress & address, const PString & _path)
  : SDPMediaDescription(address), path(_path)
{
  SetDirection(SDPMediaDescription::SendRecv);
}

void SDPMSRPMediaDescription::CreateSDPMediaFormats(const PStringArray &)
{
  formats.Append(new SDPMediaFormat(*this, RTP_DataFrame::MaxPayloadType, OpalMSRP));
}


bool SDPMSRPMediaDescription::PrintOn(ostream & str, const PString & /*connectString*/) const
{
  // call ancestor. Never output the connect string, as the listening TCP sockets 
  // for the MSRP manager will always give an address of 0.0.0.0
  if (!SDPMediaDescription::PrintOn(str, ""))
    return false;

  str << "a=accept-types:" << types << "\r\n";
  str << "a=path:" << path << "\r\n";

  return true;
}

void SDPMSRPMediaDescription::SetAttribute(const PString & attr, const PString & value)
{
  if (attr *= "path") 
    path = value;
  else if (attr *= "accept-types")
    types = value.Trim();
}

void SDPMSRPMediaDescription::ProcessMediaOptions(SDPMediaFormat & /*sdpFormat*/, const OpalMediaFormat & mediaFormat)
{
  if (mediaFormat.GetMediaType() == "msrp") 
    types = mediaFormat.GetOptionString("Accept Types").Trim();
}

OpalMediaFormatList SDPMSRPMediaDescription::GetMediaFormats() const
{
  OpalMediaFormat msrp(OpalMSRP);
  msrp.SetOptionString("Accept Types", types);
  msrp.SetOptionString("Path",         path);

  PTRACE(4, "MSRP\tNew format is\n" << setw(-1) << msrp);

  OpalMediaFormatList fmts;
  fmts += msrp;
  return fmts;
}

void SDPMSRPMediaDescription::AddMediaFormat(const OpalMediaFormat & mediaFormat)
{
  if (!mediaFormat.IsTransportable() || !mediaFormat.IsValidForProtocol("sip") || mediaFormat.GetMediaType() != "msrp") {
    PTRACE(4, "MSRP\tSDP not including " << mediaFormat << " as it is not a valid MSRP format");
    return;
  }

  SDPMediaFormat * sdpFormat = new SDPMediaFormat(*this, mediaFormat);
  ProcessMediaOptions(*sdpFormat, mediaFormat);
  AddSDPMediaFormat(sdpFormat);
}


#endif // OPAL_SIP

////////////////////////////////////////////////////////////////////////////////////////////

class MSRPInitialiser : public PProcessStartup
{
  PCLASSINFO(MSRPInitialiser, PProcessStartup)
  public:
    virtual void OnShutdown()
    {
      PWaitAndSignal m(mutex);
      delete manager;
      manager = NULL;
    }

    static OpalMSRPManager & KickStart(OpalManager & opalManager)
    {
      PWaitAndSignal m(mutex);
      if (manager == NULL) 
        manager = new OpalMSRPManager(opalManager, OpalMSRPManager::DefaultPort);

      return * manager;
    }

protected:
    static PMutex mutex;
    static OpalMSRPManager * manager;
};

PMutex MSRPInitialiser::mutex;
OpalMSRPManager * MSRPInitialiser::manager = NULL;

static PFactory<PProcessStartup>::Worker<MSRPInitialiser> opalpluginStartupFactory("MSRP", true);

////////////////////////////////////////////////////////////////////////////////////////////

OpalMSRPMediaSession::OpalMSRPMediaSession(OpalConnection & _conn, unsigned _sessionId)
  : OpalMediaSession(_conn, "msrp", _sessionId)
  , m_manager(MSRPInitialiser::KickStart(_conn.GetEndPoint().GetManager()))
{
  // sessionIDs are supposed to unguessable
  m_localMSRPSessionId = m_manager.CreateSession();
  m_localUrl           = m_manager.SessionIDToURL(m_localMSRPSessionId);
}

OpalMSRPMediaSession::OpalMSRPMediaSession(const OpalMSRPMediaSession & _obj)
  : OpalMediaSession(_obj)
  , m_manager(_obj.m_manager)
  , m_connectionPtr(_obj.m_connectionPtr)
{
}

void OpalMSRPMediaSession::Close()
{
  m_manager.DestroySession(m_localMSRPSessionId);
}

OpalTransportAddress OpalMSRPMediaSession::GetLocalMediaAddress() const
{
  OpalTransportAddress addr;
  if (m_manager.GetLocalAddress(addr))
    return addr;

  return OpalTransportAddress();
}

#if OPAL_SIP

SDPMediaDescription * OpalMSRPMediaSession::CreateSDPMediaDescription(const OpalTransportAddress & sdpContactAddress)
{
  return new SDPMSRPMediaDescription(sdpContactAddress, m_localUrl.AsString());
}

#endif

OpalMediaStream * OpalMSRPMediaSession::CreateMediaStream(const OpalMediaFormat & mediaFormat, 
                                                                         unsigned sessionID, 
                                                                         PBoolean isSource)
{
  PTRACE(2, "MSRP\tCreated " << (isSource ? "source" : "sink") << " media stream in " << (connection.IsOriginating() ? "originator" : "receiver") << " with " << m_localUrl);
  return new OpalMSRPMediaStream(connection, mediaFormat, sessionID, isSource, *this);
}

void OpalMSRPMediaSession::SetRemoteMediaAddress(const OpalTransportAddress & transportAddress, const OpalMediaFormatList & )
{
  PTRACE(2, "MSRP\tSetting remote media address to " << transportAddress);
}

bool OpalMSRPMediaSession::WriteData(      
  const BYTE * /*data*/,   ///<  Data to write
  PINDEX length,       ///<  Length of data to read.
  PINDEX & written     ///<  Length of data actually written
)
{
  PTRACE(2, "MSRP\tWriting " << length << " bytes");
  written = length;
  return true;
}


bool OpalMSRPMediaSession::OpenMSRP(const PURL & remoteUrl)
{
  if (m_connectionPtr != NULL)
    return true;

  if (remoteUrl.IsEmpty())
    return false;

  m_remoteUrl = remoteUrl;

  // only create connections when originating the call
  if (!m_isOriginating)
    return true;

  // create connection to remote 
  m_connectionPtr = m_manager.OpenConnection(m_localUrl, m_remoteUrl);
  if (m_connectionPtr == NULL) {
    PTRACE(3, "MSRP\tCannot create connection to remote URL '" << m_remoteUrl << "'");
    return false;
  }

  return true;
}


////////////////////////////////////////////////////////

OpalMediaSession * OpalMSRPMediaType::CreateMediaSession(OpalConnection & conn, unsigned sessionID) const
{
  // as this is called in the constructor of an OpalConnection descendant, 
  // we can't use a virtual function on OpalConnection
#if OPAL_SIP
  if (conn.GetPrefixName() *= "sip") {
    PTRACE(2, "MSRP\tCreating MSRP media session for SIP connection");
    return new OpalMSRPMediaSession(conn, sessionID);
  }
#endif

  PTRACE(2, "MSRP\tCannot create MSRP media session for unknown connection type " << conn.GetPrefixName());
  
  return NULL;
}

////////////////////////////////////////////////////////

OpalMSRPMediaStream::OpalMSRPMediaStream(
      OpalConnection & conn,
      const OpalMediaFormat & mediaFormat, ///<  Media format for stream
      unsigned sessionID,                  ///<  Session number for stream
      bool isSource,                       ///<  Is a source stream
      OpalMSRPMediaSession & msrpSession
)
  : OpalIMMediaStream(conn, mediaFormat, sessionID, isSource)
  , m_msrpSession(msrpSession)
  , m_remoteParty(mediaFormat.GetOptionString("Path"))
{
  PTRACE(3, "MSRP\tOpening MSRP connection from " << m_msrpSession.GetLocalURL() << " to " << m_remoteParty);
}

OpalMSRPMediaStream::~OpalMSRPMediaStream()
{
}

bool OpalMSRPMediaStream::Open()
{
  return m_msrpSession.OpenMSRP(m_remoteParty) && OpalMediaStream::Open();
}

PBoolean OpalMSRPMediaStream::ReadData(
      BYTE *,
      PINDEX,
      PINDEX &
    )
{
  PAssertAlways("Cannot ReadData from OpalMSRPMediaStream");
  return false;
}

PBoolean OpalMSRPMediaStream::WriteData(
      const BYTE * data,   ///<  Data to write
      PINDEX length,       ///<  Length of data to read.
      PINDEX & written     ///<  Length of data actually written
    )
{
  if (!isOpen)
    return false;

  return m_msrpSession.WriteData(data, length, written);
}

PBoolean OpalMSRPMediaStream::Close()
{
  return OpalIMMediaStream::Close();
}

////////////////////////////////////////////////////////////////////////////////////////////

OpalMSRPManager::OpalMSRPManager(OpalManager & _opalManager, WORD _port)
  : opalManager(_opalManager), m_listenerPort(_port), m_listenerThread(NULL)
{
}

OpalMSRPManager::~OpalMSRPManager()
{
  PWaitAndSignal m(mutex);

  if (m_listenerThread != NULL) {
    m_listenerSocket.Close();
    m_listenerThread->WaitForTermination();
    delete m_listenerThread;
  }
}

std::string OpalMSRPManager::CreateSession()
{
  std::string id;
  for (;;) {
    mutex.Wait();
    PString s = psprintf("%c%08x%u", PRandom::Number('a', 'z'), PRandom::Number(), ++lastID);
    id = (const char *)s;
    //if (m_sessionInfoMap.find(id) != m_sessionInfoMap.end()) {
    //  mutex.Signal();
    //  continue;
    //}
    //SessionInfo sessionInfo;
    //m_sessionInfoMap.insert(SessionInfoMap::value_type(id, sessionInfo));
    //PTRACE(2, "MSRP\tSession opened - " << m_sessionInfoMap.size() << " sessions now in progress");
    mutex.Signal();
    break;
  }

  return id;
}

PURL OpalMSRPManager::SessionIDToURL(const std::string & id)
{
  PIPSocket::Address addr;
  PString hostname;
  if (!PIPSocket::GetHostAddress(addr))
    hostname = PIPSocket::GetHostName();
  else
    hostname = addr.AsString();

  PStringStream str;
  str << "msrp://"
      << hostname
      << ":"
      << m_listenerPort
      << "/"
      << id
      << ";tcp";

  return PURL(str);
}

void OpalMSRPManager::DestroySession(const std::string & /*id*/)
{
  //PWaitAndSignal m(mutex);

  //SessionInfoMap::iterator r = m_sessionInfoMap.find(id);
  //if (r != m_sessionInfoMap.end())
  //  m_sessionInfoMap.erase(r);

  //PTRACE(2, "MSRP\tSession " << id << " closed - " << m_sessionInfoMap.size() << " sessions now in progress");
}

bool OpalMSRPManager::GetLocalAddress(OpalTransportAddress & addr)
{
  PWaitAndSignal m(mutex);

  if (!m_listenerSocket.IsOpen()) {
    if (!m_listenerSocket.Listen(5, m_listenerPort)) {
      PTRACE(2, "MSRP\tCannot start MSRP listener on port " << m_listenerPort);
      return false;
    }

    m_listenerThread = new PThreadObj<OpalMSRPManager>(*this, &OpalMSRPManager::ListenerThread);

    PIPSocket::Address ip; WORD port;
    m_listenerSocket.GetLocalAddress(ip, port);
    if (ip.IsAny()) {
      if (!PIPSocket::GetNetworkInterface(ip)) {
        PTRACE(2, "MSRP\tUnable to get specific IP address for MSRP listener");
        return false;
      }
    }

    m_listenerAddress = OpalTransportAddress(ip, port);
    PTRACE(2, "MSRP\tListener started on " << m_listenerAddress);
  }

  addr = m_listenerAddress;

  return true;
}

bool OpalMSRPManager::GetLocalPort(WORD & port)
{
  port = m_listenerPort;
  return true;
}

void OpalMSRPManager::ListenerThread()
{
  PTRACE(2, "MSRP\tListener thread started");

  for (;;) {
    MSRPProtocol * protocol = new MSRPProtocol;
    if (!protocol->Accept(m_listenerSocket)) {
      delete protocol;
      break;
    }
    PSafePtr<Connection> connection = new Connection(protocol);
    connection->m_handlerThread = new PThreadObj1Arg<OpalMSRPManager, PSafePtr<Connection> >(*this, connection, &OpalMSRPManager::HandlerThread);
  }
  PTRACE(2, "MSRP\tListener thread ended");
}

void OpalMSRPManager::HandlerThread(PSafePtr<Connection> connection)
{
  PTRACE(2, "MSRP\tMSRP protocol thread started");

  MSRPProtocol & protocol = *(connection->m_protocol);
  protocol.SetReadTimeout(1000);

  for (;;) {
    connection.SetSafetyMode(PSafeReadOnly);

    PString line;
    if (!protocol.ReadLine(line, false)) {
      if (protocol.GetErrorCode() == PChannel::Timeout)
        continue;
      PTRACE(2, "MSRP\tMSRP protocol socket returned error " << protocol.GetErrorText());
      break;
    }

    PMIMEInfo mime(protocol);

    connection.SetSafetyMode(PSafeReadWrite);

    PTRACE(2, "MSRP\tMSRP protocol socket returned " << line);

/*
    switch (cmd) {
      case MSRPProtocol::SEND:
        PTRACE(2, "MSRP\tMSRP protocol socket returned SEND");
        break;

      case MSRPProtocol::REPORT:
        PTRACE(2, "MSRP\tMSRP protocol socket returned REPORT");
        break;
    }
*/
  }

  PTRACE(2, "MSRP\tMSRP protocol thread finished");
}

PSafePtr<OpalMSRPManager::Connection> OpalMSRPManager::OpenConnection(const PURL & localURL, const PURL & remoteURL)
{
  // get hostname of remote 
  PIPSocket::Address ip = remoteURL.GetHostName();
  WORD port             = remoteURL.GetPort();
  if (!ip.IsValid()) {
    if (remoteURL.GetPortSupplied()) {
      if (!PIPSocket::GetHostAddress(remoteURL.GetHostName(), ip)) {
        PTRACE(2, "MSRP\tUnable to resolve MSRP URL '" << remoteURL << "' with explicit port");
        return NULL;
      }
    }
    else {
      PIPSocketAddressAndPortVector addresses;
      if (PDNS::LookupSRV(remoteURL.GetHostName(), "_im._msrp", remoteURL.GetPort(), addresses) && !addresses.empty()) {
        ip   = addresses[0].GetAddress(); // Only use first entry
        port = addresses[0].GetPort();
      } 
      else if (!PIPSocket::GetHostAddress(remoteURL.GetHostName(), ip)) {
        PTRACE(2, "MSRP\tUnable to resolve MSRP URL hostname '" << remoteURL << "' ");
        return false;
      }
    }
  }

  PString connectionKey(ip.AsString() + ":" + PString(PString::Unsigned, port));

  PWaitAndSignal m(m_connectionInfoMapAddMutex);

  // see if we already have a connection to that remote
  PSafePtr<Connection> connection = NULL;
  if (m_connectionInfoMap.FindWithLock(connectionKey) != NULL) {
    PTRACE(2, "MSRP\tReusing existing connection to " << ip << ":" << port);
    return connection;
  }
  
  // create a connection to the remote
  connection = new Connection();
  if (!connection->m_protocol->Connect(ip, port)) {
    PTRACE(2, "MSRP\tUnable to make new connection connection to " << ip << ":" << port);
    return false;
  }

  PString key(ip.AsString() + ":" + PString(PString::Unsigned, port));
  m_connectionInfoMap.SetAt(key, connection);

  connection->m_protocol->SendCommand(MSRPProtocol::SEND, localURL, remoteURL);

  return connection;
}

////////////////////////////////////////////////////////

OpalMSRPManager::Connection::Connection(MSRPProtocol * protocol)
  : m_handlerThread(NULL)
  , m_running(false)
  , m_protocol(protocol)
{
  PTRACE(3, "MSRP\tCreating connection");
  if (m_protocol == NULL)
    m_protocol = new MSRPProtocol();
}

OpalMSRPManager::Connection::~Connection()
{
  if (m_handlerThread != NULL) {
    m_running = false;
    m_handlerThread->WaitForTermination();
    delete m_handlerThread;
    m_handlerThread = NULL;
  }
  delete m_protocol;
  m_protocol = NULL;
  PTRACE(3, "MSRP\tDestroying connection");
}

////////////////////////////////////////////////////////

static char const * const MSRPCommands[MSRPProtocol::NumCommands] = {
  "SEND", "REPORT"
};

MSRPProtocol::MSRPProtocol()
: PInternetProtocol("msrp 2855", NumCommands, MSRPCommands)
{ }

bool MSRPProtocol::SendCommand(Commands cmd, const PURL & from, const PURL & to)
{
  PString tag("xxxxx");

  PMIMEInfo mime;
  mime.SetAt("To-Path", to.AsString());
  mime.SetAt("From-Path", from.AsString());
  mime.SetAt("Content-Type", "text/plain");

  PStringStream strm;
  strm << "MSRP " << tag << " " << MSRPCommands[cmd] << CRLF;
  if (!WriteLine(strm) || !mime.Write(*this))
    return false;

  return true;
}


////////////////////////////////////////////////////////

#endif //  OPAL_HAS_MSRP
