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

bool OpalMSRPMediaSession::WritePacket(RTP_DataFrame & frame)
{
  if (m_connectionPtr == NULL) {
    PTRACE(2, "MSRP\tCannot send MSRP message as no connection has been established");
  } 
  else {
    PString messageId;
    T140String text((const BYTE *)frame.GetPayloadPtr(), frame.GetPayloadSize());
    PString str;
    if (!text.AsString(str)) {
      PTRACE(2, "MSRP\tCannot convert T.140 string to text");
    }
    else {
      m_connectionPtr->m_protocol->SendSEND(m_localUrl, m_remoteUrl, str, "text/plain", messageId);
    }
  }
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

void OpalMSRPMediaSession::SetConnection(const PSafePtr<OpalMSRPManager::Connection> & conn)
{
  if (m_connectionPtr == NULL)
    m_connectionPtr = conn;
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
  , m_rfc4103Context(mediaFormat)
{
  PTRACE(3, "MSRP\tOpening MSRP connection from " << m_msrpSession.GetLocalURL() << " to " << m_remoteParty);
  if (isSource) 
    m_msrpSession.GetManager().SetNotifier(m_msrpSession.GetLocalURL(), m_remoteParty,
                                           PCREATE_NOTIFIER2(OnReceiveMSRP, const OpalMSRPManager::IncomingMSRP &));
}

OpalMSRPMediaStream::~OpalMSRPMediaStream()
{
  m_msrpSession.GetManager().RemoveNotifier(m_msrpSession.GetLocalURL(), m_remoteParty);
}

bool OpalMSRPMediaStream::Open()
{
  return m_msrpSession.OpenMSRP(m_remoteParty) && OpalMediaStream::Open();
}

PBoolean OpalMSRPMediaStream::ReadPacket(RTP_DataFrame &)
{
  PAssertAlways("Cannot ReadData from OpalMSRPMediaStream");
  return false;
}

PBoolean OpalMSRPMediaStream::WritePacket(RTP_DataFrame & frame)
{
  if (!IsOpen())
    return false;

  return m_msrpSession.WritePacket(frame);
}

PBoolean OpalMSRPMediaStream::Close()
{
  return OpalIMMediaStream::Close();
}

void OpalMSRPMediaStream::OnReceiveMSRP(OpalMSRPManager &, const OpalMSRPManager::IncomingMSRP & incomingMSRP)
{
  m_msrpSession.SetConnection(incomingMSRP.m_connection);

  if (incomingMSRP.m_command == MSRPProtocol::SEND) {
    PTRACE(3, "MSRP\tMediaStream " << *this << " received SEND");
    T140String t140(incomingMSRP.m_body);
    RTP_DataFrameList frames = m_rfc4103Context.ConvertToFrames(t140);
    OpalMediaFormat fmt(m_rfc4103Context.m_mediaFormat);
    incomingMSRP.m_mime.GetString("Content-Type", "text/plain");
    for (PINDEX i = 0; i < frames.GetSize(); ++i)
      connection.OnReceiveExternalIM(m_rfc4103Context.m_mediaFormat, frames[i]);
  }
  else {
    PTRACE(3, "MSRP\tMediaStream " << *this << " receiving unknown MSRP message");
  }
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
      PTRACE(2, "MSRP\tListener accept failed");
      delete protocol;
      break;
    }
    PTRACE(2, "MSRP\tListener accepted new incoming connection");
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
    PIPSocket::SelectList sockets;
    sockets += *protocol.GetSocket();

    if (PIPSocket::Select(sockets, 1000) != PChannel::NoError) 
      break;

    if (sockets.GetSize() != 0) {

      PTRACE(3, "MSRP\tMSRP message received");

      IncomingMSRP incomingMsg;
      incomingMsg.m_connection = connection;
      if (!protocol.ReadMessage(incomingMsg.m_command, incomingMsg.m_chunkId, incomingMsg.m_mime, incomingMsg.m_body))
        break;

      PString fromUrl(incomingMsg.m_mime("From-Path"));
      PString toUrl  (incomingMsg.m_mime("To-Path"));

      if (incomingMsg.m_command == MSRPProtocol::SEND) {
        protocol.SendResponse(incomingMsg.m_chunkId, 200, "OK", toUrl, fromUrl);
        PTRACE(3, "MSRP\tMSRP SEND received from=" << fromUrl << ",to=" << toUrl);
        if (!toUrl.IsEmpty() && !fromUrl.IsEmpty()) {
          PString key(toUrl + '\t' + fromUrl);
          PWaitAndSignal m(m_callBacksMutex);
          CallBackMap::iterator r = m_callBacks.find(key);
          if (r == m_callBacks.end()) {
            PTRACE(2, "MSRP\tNo registered callbacks with '" << key << "'");
          } else {
            PTRACE(2, "MSRP\tCalling registered callbacks for '" << key << "'");
            r->second(*this, incomingMsg);
          }           
        }
        if (incomingMsg.m_mime("Success-Report") *= "yes") {
          PMIMEInfo mime;
          mime.SetAt("Message-ID", incomingMsg.m_mime("Message-ID"));
          mime.SetAt("Byte-Range", incomingMsg.m_mime("Byte-Range"));
          mime.SetAt("Status",     "000 200 OK");
          protocol.SendREPORT(incomingMsg.m_chunkId, toUrl, fromUrl, mime);
        }
      }
    }
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
#if P_DNS
      PIPSocketAddressAndPortVector addresses;
      if (PDNS::LookupSRV(remoteURL.GetHostName(), "_im._msrp", remoteURL.GetPort(), addresses) && !addresses.empty()) {
        ip   = addresses[0].GetAddress(); // Only use first entry
        port = addresses[0].GetPort();
      } 
      else if (!PIPSocket::GetHostAddress(remoteURL.GetHostName(), ip)) {
        PTRACE(2, "MSRP\tUnable to resolve MSRP URL hostname '" << remoteURL << "' ");
        return false;
      }
#else
      return false;
#endif
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
    PTRACE(2, "MSRP\tUnable to make new connection to " << ip << ":" << port);
    return false;
  }

  PTRACE(2, "MSRP\tConnection established to to " << ip << ":" << port);

  PString key(ip.AsString() + ":" + PString(PString::Unsigned, port));
  m_connectionInfoMap.SetAt(key, connection);

  PString uid;
  connection->m_protocol->SendSEND(localURL, remoteURL, "", "", uid);

  connection->m_handlerThread = new PThreadObj1Arg<OpalMSRPManager, PSafePtr<Connection> >(*this, connection, &OpalMSRPManager::HandlerThread);

  return connection;
}


void OpalMSRPManager::SetNotifier(const PURL & localUrl,
                                  const PURL & remoteUrl,
                                  const CallBack & notifier)
{
  PString key(localUrl.AsString() + '\t' + remoteUrl.AsString());
  PTRACE(2, "MSRP\tRegistering callback for incoming MSRP messages with '" << key << "'");
  PWaitAndSignal m(m_callBacksMutex);
  m_callBacks.insert(CallBackMap::value_type(key, CallBack(notifier)));
}


void OpalMSRPManager::RemoveNotifier(const PURL & localUrl, const PURL & remoteUrl)
{
  PString key(localUrl.AsString() + '\t' + remoteUrl.AsString());
  PWaitAndSignal m(m_callBacksMutex);
  m_callBacks.erase(key);
}


////////////////////////////////////////////////////////

OpalMSRPManager::Connection::Connection(MSRPProtocol * protocol)
  : m_protocol(protocol)
  , m_running(false)
  , m_handlerThread(NULL)
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

bool MSRPProtocol::SendSEND(const PURL & from, 
                            const PURL & to,
                            const PString & text,
                            const PString & contentType,
                                  PString & messageId)
{
  // create a message
  Message message;
  message.m_id          = messageId = PGloballyUniqueID().AsString();
  message.m_fromURL     = from;
  message.m_toURL       = to;
  message.m_contentType = contentType;
  message.m_length      = text.GetLength();
  
  // break the text into chunks
  if (message.m_length == 0) {
    Message::Chunk chunk(PGloballyUniqueID().AsString(), 0, 0);
    message.m_chunks.push_back(chunk);
  }
  else {
    unsigned offs = 0;
    while ((message.m_length - offs) > MaximumMessageLength) {
      Message::Chunk chunk(PGloballyUniqueID().AsString(), offs, MaximumMessageLength);
      message.m_chunks.push_back(chunk);
      offs += MaximumMessageLength;
    }
    Message::Chunk chunk(PGloballyUniqueID().AsString(), offs, message.m_length - offs);
    message.m_chunks.push_back(chunk);
  }

  // add message to the message map
  //m_messageMap.insert(MessageMap::value_type(message.m_id, message));

  // send the chunks
  for (Message::ChunkList::const_iterator r = message.m_chunks.begin(); r != message.m_chunks.end(); ++r) {
    PMIMEInfo mime;
    mime.SetAt("Message-ID",   message.m_id);
    bool isLast = ((r+1) == message.m_chunks.end());

    PString body;
    if (message.m_length != 0) {
      mime.SetAt("Success-Report", "yes");
      mime.SetAt("Byte-Range",   psprintf("%u-%u/%u", r->m_rangeFrom, r->m_rangeTo, message.m_length));
      body = (PString("Content-Type: ") & message.m_contentType) + CRLF CRLF +
             text.Mid(r->m_rangeFrom-1, r->m_rangeTo - r->m_rangeFrom + 1) + CRLF;
    }

    body += PString("-------") + r->m_chunkId + (isLast ? '$' : '+') + CRLF;   // note that RFC 4975 mandates a CRLF before the terminator

    if (!SendChunk(r->m_chunkId, 
                   message.m_toURL.AsString(),
                   message.m_fromURL.AsString(),
                   mime, 
                   body))
      return false;
  }
  return true;
}


bool MSRPProtocol::SendChunk(const PString & chunkId, 
                             const PString toUrl,
                             const PString fromUrl,
                             const PMIMEInfo & mime, 
                             const PString & body)
{
  // Note that RFC 4975 mandates the order and position of of To-Path and From-Path
  *this << "MSRP " << chunkId << " " << MSRPCommands[SEND] << CRLF
        << "To-Path: " << toUrl << CRLF
        << "From-Path: "<< fromUrl << CRLF
        << ::setfill('\r');
  mime.PrintContents(*this);
  *this << body;
  flush();

  {
    PStringStream str; str << ::setfill('\r');
    mime.PrintContents(str);
    PTRACE(4, "Sending MSRP message\n" << "MSRP " << chunkId << " " << MSRPCommands[SEND] << CRLF 
                                       << "To-Path: " << toUrl << CRLF 
                                       << "From-Path: "<< fromUrl << CRLF 
                                       << str 
                                       << body);
  }

  return true;
}

bool MSRPProtocol::SendREPORT(const PString & chunkId, 
                              const PString & toUrl,
                              const PString & fromUrl,
                            const PMIMEInfo & mime)
{
  // Note that RFC 4975 mandates the order and position of of To-Path and From-Path
  *this << "MSRP " << chunkId << " " << MSRPCommands[REPORT] << CRLF
        << "To-Path: " << toUrl << CRLF
        << "From-Path: "<< fromUrl << CRLF
        << ::setfill('\r');
  mime.PrintContents(*this);
  *this << "-------" << chunkId << "$" << CRLF;
  flush();

  {
    PStringStream str; str << ::setfill('\r') << mime.PrintContents(str);
    PTRACE(4, "Sending MSRP REPORT\n" << "MSRP " << chunkId << " " << MSRPCommands[REPORT] << CRLF 
                                                 << "To-Path: " << toUrl << CRLF 
                                                 << "From-Path: "<< fromUrl << CRLF 
                                                 << str 
                                                 << "-------" << chunkId << "$");
  }

  return true;
}

bool MSRPProtocol::SendResponse(const PString & chunkId, 
                                unsigned response,
                                const PString & text,
                                const PString & toUrl,
                                const PString & fromUrl)
{
  // Note that RFC 4975 mandates the order and position of of To-Path and From-Path
  *this << "MSRP " << chunkId << " " << response << (text.IsEmpty() ? "" : " ") << text << CRLF
        << "To-Path: " << toUrl << CRLF
        << "From-Path: "<< fromUrl << CRLF
        << "-------" << chunkId << "$" << CRLF;
  flush();

  PTRACE(4, "Sending MSRP response\n" << "MSRP " << chunkId << " " << response << (text.IsEmpty() ? "" : " ") << CRLF 
                                                 << "To-Path: " << toUrl << CRLF
                                                 << "From-Path: "<< fromUrl << CRLF
                                                 << "-------" << chunkId << "$");

  return true;
}

bool MSRPProtocol::ReadMessage(int & command, 
                           PString & chunkId,
                         PMIMEInfo & mime, 
                           PString & body)
{
  // get the MSRP start line
  PString line;
  do {
    if (!ReadLine(line, false)) {
      PTRACE(2, "MSRP\tError while reading MSRP command");
      return PFalse;
    }
  } while (line.IsEmpty());

  // get tokens
  PStringArray tokens = line.Tokenise(' ', false);
  if (tokens.GetSize() < 3) {
    PTRACE(2, "MSRP\tReceived malformed MSRP command line with " << tokens.GetSize() << " tokens");
    return false;
  }

  if (!(tokens[0] *= "MSRP")) {
    PTRACE(2, "MSRP\tFirst token on MSRP command line is not MSRP");
    return false;
  }

  chunkId = tokens[1];
  PString terminator = "-------" + chunkId;
  body.MakeEmpty();

  // read MIME until empty line or terminator
  bool terminated = false;
  {
    mime.RemoveAll();
    PString line;
    while (ReadLine(line, false)) {
      if (line.IsEmpty())
        break;
      if (line.Find(terminator) == 0) {
        terminated = true;
        break;
      }
      mime.AddMIME(line);
    }
  }

  // determine what command was given
  command = NumCommands;
  for (PINDEX i = 0; i < NumCommands; ++i) {
    if (tokens[2] *= MSRPCommands[i]) {
      command = i; 
      break;
    }
  }
  if (command == NumCommands) {
    unsigned code = tokens[2].AsUnsigned();
    if (code > NumCommands)
      command = code;
  }

  // handle SEND bodies
  if ((command == SEND) && mime.Contains("Content-Type")) {
    for (;;) {
      PString line;
      if (!ReadLine(line)) {
        PTRACE(2, "MSRP\tError while reading MSRP command body");
        return false;
      }
      if (line.Find(terminator) == 0) {
        break;
      }
      if ((body.GetSize() + line.GetLength()) > 10240) {
        PTRACE(2, "MSRP\tMaximum body size exceeded");
        return false;
      }
      body += line;
    }
  }

  {
    PStringStream str; str << ::setfill('\r');
    mime.PrintContents(str);
    PTRACE(4, "Received MSRP message\n" << line << "\n" << str << body << terminator);
  }

  return true;
}




////////////////////////////////////////////////////////

#endif //  OPAL_HAS_MSRP
