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
 */

#ifdef __GNUC__
#pragma implementation "msrp.h"
#endif

#include <ptlib.h>

#include <im/msrp.h>

#if OPAL_HAS_MSRP

#include <im/im.h>

#include <ptclib/pdns.h>
#include <ptclib/http.h>

#include <opal/connection.h>
#include <opal/endpoint.h>
#include <im/t140.h>
#include <sdp/sdp.h>


#define CRLF "\r\n"

const PCaselessString & OpalMSRPMediaSession::TCP_MSRP() { static const PConstCaselessString s("TCP/MSRP"); return s; }

PFACTORY_CREATE(OpalMediaSessionFactory, OpalMSRPMediaSession, OpalMSRPMediaSession::TCP_MSRP());

class OpalMSRPMediaDefinition : public OpalMediaTypeDefinition 
{
  public:
    static const char * Name() { return OPAL_IM_MEDIA_TYPE_PREFIX"msrp"; }

    OpalMSRPMediaDefinition()
      : OpalMediaTypeDefinition(Name(), OpalMSRPMediaSession::TCP_MSRP())
    {
    }

#if OPAL_SDP
    virtual PString GetSDPMediaType() const { static PConstCaselessString const s("message"); return s; }
    virtual PString GetSDPTransportType() const { return OpalMSRPMediaSession::TCP_MSRP(); }

    virtual SDPMediaDescription * CreateSDPMediaDescription(
      const OpalTransportAddress & localAddress
    ) const;
#endif // OPAL_SDP
};

OPAL_MEDIATYPE(OpalMSRPMedia);


/////////////////////////////////////////////////////////////////////////////

#define DECLARE_MSRP_ENCODING(title, encoding) \
class IM##title##OpalMSRPEncoding : public OpalMSRPEncoding \
{ \
}; \
PFACTORY_CREATE(PFactory<OpalMSRPEncoding>, IM##title##OpalMSRPEncoding, encoding, true)


DECLARE_MSRP_ENCODING(Text, "text/plain");
DECLARE_MSRP_ENCODING(CPIM, "message/cpim");
DECLARE_MSRP_ENCODING(HTML, "message/html");

const OpalMediaFormat & GetOpalMSRP() 
{ 
  class OpalIMMSRPMediaFormatInternal : public OpalMediaFormatInternal {
    public: 
      OpalIMMSRPMediaFormatInternal() 
        : OpalMediaFormatInternal(OPAL_MSRP, 
                                  OpalMSRPMediaType(),
                                  RTP_DataFrame::MaxPayloadType, 
                                  "+", 
                                  false,  
                                  1440, 
                                  512, 
                                  0, 
                                  1000)
      { 
        SetOptionString(OpalMediaFormat::DescriptionOption(), "RFC 4975 (MSRP) Instant Message Relay");

        PFactory<OpalMSRPEncoding>::KeyList_T types = PFactory<OpalMSRPEncoding>::GetKeyList();
        PFactory<OpalMSRPEncoding>::KeyList_T::iterator r;

        PString acceptTypes;
        for (r = types.begin(); r != types.end(); ++r) {
          if (!acceptTypes.IsEmpty())
            acceptTypes += " ";
          acceptTypes += *r;
        }

        OpalMediaOptionString * option = new OpalMediaOptionString("Accept Types", false, acceptTypes);
        option->SetMerge(OpalMediaOption::AlwaysMerge);
        AddOption(option);

        option = new OpalMediaOptionString("Path", false, "");
        option->SetMerge(OpalMediaOption::MaxMerge);
        AddOption(option);
      } 
  };
  static OpalMediaFormatStatic<OpalMediaFormat> MSRP(new OpalIMMSRPMediaFormatInternal);
  return MSRP;
} 


#if OPAL_SDP

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

    virtual void CreateSDPMediaFormats(const PStringArray &);
    virtual void OutputAttributes(ostream & strm) const;
    virtual void SetAttribute(const PString & attr, const PString & value);
    virtual void ProcessMediaOptions(SDPMediaFormat & sdpFormat, const OpalMediaFormat & mediaFormat);
    virtual void AddMediaFormat(const OpalMediaFormat & mediaFormat);

    virtual OpalMediaFormatList GetMediaFormats() const;

    // CreateSDPMediaFormat is used for processing format lists. MSRP always contains only "*"
    virtual SDPMediaFormat * CreateSDPMediaFormat();

    // FindFormat is used only for rtpmap and fmtp, neither of which are used for MSRP
    virtual SDPMediaFormat * FindFormat(PString &) const { return NULL; }

  protected:
    class Format : public SDPMediaFormat
    {
      public:
        Format(SDPMSRPMediaDescription & parent, const OpalMediaFormat & mediaFormat)
          : SDPMediaFormat(parent)
        {
          FromMediaFormat(mediaFormat);
        }
    };

    PString path;
    PString types;
};


SDPMediaDescription * OpalMSRPMediaDefinition::CreateSDPMediaDescription(const OpalTransportAddress & localAddress) const
{
  return new SDPMSRPMediaDescription(localAddress, PURL());
}

SDPMediaDescription * OpalMSRPMediaSession::CreateSDPMediaDescription()
{
  return new SDPMSRPMediaDescription(GetLocalAddress(), GetLocalURL());
}


///////////////////////////////////////////////////////////////////////////////////////////

SDPMSRPMediaDescription::SDPMSRPMediaDescription(const OpalTransportAddress & address)
  : SDPMediaDescription(address, OpalMSRPMediaType())
{
  SetDirection(SDPMediaDescription::SendRecv);
}


SDPMSRPMediaDescription::SDPMSRPMediaDescription(const OpalTransportAddress & address, const PString & _path)
  : SDPMediaDescription(address, OpalMSRPMediaType())
  , path(_path)
{
  SetDirection(SDPMediaDescription::SendRecv);
}


void SDPMSRPMediaDescription::CreateSDPMediaFormats(const PStringArray &)
{
  m_formats.Append(CreateSDPMediaFormat());
}


SDPMediaFormat * SDPMSRPMediaDescription::CreateSDPMediaFormat()
{
  return new Format(*this, OpalMSRP);
}


void SDPMSRPMediaDescription::OutputAttributes(ostream & strm) const
{
  // call ancestor. Never output the connect string, as the listening TCP sockets 
  // for the MSRP manager will always give an address of 0.0.0.0
  SDPMediaDescription::OutputAttributes(strm);

  strm << "a=accept-types:" << types << "\r\n"
          "a=path:" << path << "\r\n";
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
  if (mediaFormat.GetMediaType() == OpalMSRPMediaType()) 
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
  if (!mediaFormat.IsTransportable() || !mediaFormat.IsValidForProtocol("sip") || mediaFormat.GetMediaType() != OpalMSRPMediaType()) {
    PTRACE(4, "MSRP\tSDP not including " << mediaFormat << " as it is not a valid MSRP format");
    return;
  }

  AddSDPMediaFormat(new Format(*this, mediaFormat));
}


#endif // OPAL_SDP


////////////////////////////////////////////////////////////////////////////////////////////

static OpalMSRPManager & GetOpalMSRPManager(OpalManager & opal)
{
  static PCriticalSection mutex;
  PWaitAndSignal lock(mutex);
  static OpalMSRPManager mgr(opal);
  return mgr;
}


////////////////////////////////////////////////////////////////////////////////////////////

OpalMSRPMediaSession::OpalMSRPMediaSession(const Init & init)
  : OpalMediaSession(init)
  , m_manager(GetOpalMSRPManager(init.m_connection.GetEndPoint().GetManager()))
{
}


OpalMSRPMediaSession::~OpalMSRPMediaSession()
{
  CloseMSRP();
}


bool OpalMSRPMediaSession::Open(const PString & localInterface, const OpalTransportAddress & remoteAddress)
{
  m_isOriginating = m_connection.IsOriginating();
  m_localMSRPSessionId = m_manager.CreateSessionID();
  m_localUrl = m_manager.SessionIDToURL(localInterface, m_localMSRPSessionId);
  m_remoteAddress = remoteAddress;
  return true;
}


bool OpalMSRPMediaSession::Close()
{
  CloseMSRP();
  return true;
}


OpalTransportAddress OpalMSRPMediaSession::GetLocalAddress(bool) const
{
  return OpalTransportAddress(m_localUrl.GetHostName(), m_localUrl.GetPort());
}


OpalTransportAddress OpalMSRPMediaSession::GetRemoteAddress(bool) const
{
  return m_remoteAddress;
}


bool OpalMSRPMediaSession::SetRemoteAddress(const OpalTransportAddress & transportAddress, bool)
{
  PTRACE(2, "MSRP\tSetting remote media address to " << transportAddress);
  m_remoteAddress = transportAddress;
  return true;
}


OpalMediaStream * OpalMSRPMediaSession::CreateMediaStream(const OpalMediaFormat & mediaFormat, 
                                                                         unsigned sessionID, 
                                                                         PBoolean isSource)
{
  PTRACE(2, "MSRP\tCreated " << (isSource ? "source" : "sink") << " media stream in "
         << (m_connection.IsOriginating() ? "originator" : "receiver") << " with " << m_localUrl);
  return new OpalMSRPMediaStream(m_connection, mediaFormat, sessionID, isSource, *this);
}


bool OpalMSRPMediaSession::WritePacket(RTP_DataFrame & frame)
{
  if (m_connectionPtr == NULL) {
    PTRACE(2, "MSRP\tCannot send MSRP message as no connection has been established");
  } 
  else {
    OpalT140RTPFrame * imFrame = dynamic_cast<OpalT140RTPFrame *>(&frame);
    if (imFrame != NULL) {
      PString messageId;
      T140String content;
      PString str;
      if (imFrame->GetContent(content) && content.AsString(str))
        m_connectionPtr->m_protocol->SendSEND(m_localUrl, m_remoteUrl, str, imFrame->GetContentType(), messageId);
      else {
        PTRACE(1, "MSRP\tCannot convert IM message to string");
      }
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

  m_connectionPtr.SetSafetyMode(PSafeReference);

  return true;
}

void OpalMSRPMediaSession::CloseMSRP()
{
  if (m_connectionPtr != NULL)
    m_manager.CloseConnection(m_connectionPtr);
}

void OpalMSRPMediaSession::SetConnection(PSafePtr<OpalMSRPManager::Connection> & conn)
{
  if (m_connectionPtr == NULL) {
    m_connectionPtr = conn;
    m_connectionPtr.SetSafetyMode(PSafeReference);
  }
}

////////////////////////////////////////////////////////

OpalMSRPMediaStream::OpalMSRPMediaStream(OpalConnection & connection,
                                         const OpalMediaFormat & mediaFormat,
                                         unsigned sessionID,
                                         bool isSource,
                                         OpalMSRPMediaSession & msrpSession)
  : OpalMediaStream(connection, mediaFormat, sessionID, isSource)
  , m_msrpSession(msrpSession)
  , m_remoteParty(mediaFormat.GetOptionString("Path"))
{
  PTRACE(3, "MSRP\tOpening MSRP connection from " << m_msrpSession.GetLocalURL() << " to " << m_remoteParty);
  if (isSource) 
    m_msrpSession.GetManager().SetNotifier(m_msrpSession.GetLocalURL(), m_remoteParty,
                                           PCREATE_NOTIFIER2(OnReceiveMSRP, OpalMSRPManager::IncomingMSRP &));
}


OpalMSRPMediaStream::~OpalMSRPMediaStream()
{
  m_msrpSession.GetManager().RemoveNotifier(m_msrpSession.GetLocalURL(), m_remoteParty);
}


bool OpalMSRPMediaStream::Open()
{
  if (m_isOpen)
    return false;

  P_INSTRUMENTED_LOCK_READ_WRITE(return false);

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


void OpalMSRPMediaStream::OnReceiveMSRP(OpalMSRPManager &, OpalMSRPManager::IncomingMSRP & incomingMSRP)
{
  m_msrpSession.SetConnection(incomingMSRP.m_connection);

  if (m_connection.GetPhase() != OpalConnection::EstablishedPhase) {
    PTRACE(3, "MSRP\tMediaStream " << *this << " receiving MSRP message in non-Established phase");
  } 
  else if (incomingMSRP.m_command == MSRPProtocol::SEND) {
    PTRACE(3, "MSRP\tMediaStream " << *this << " received SEND");
    OpalIM im;
    im.m_from = m_connection.GetRemotePartyURL();
    im.m_to = m_connection.GetLocalPartyURL();

    T140String t140(incomingMSRP.m_body);
    t140.AsString(im.m_bodies[PMIMEInfo::TextPlain()]);

    PString error;
//    connection.GetEndPoint().GetManager().GetIMManager().OnMessageReceived(im, &connection, error);
  }
  else {
    PTRACE(3, "MSRP\tMediaStream " << *this << " receiving unknown MSRP message");
  }
}


////////////////////////////////////////////////////////////////////////////////////////////

OpalMSRPManager::OpalMSRPManager(OpalManager & _opalManager, WORD _port)
  : opalManager(_opalManager)
  , m_listenerPort(_port)
  , m_listenerThread(NULL)
{
  if (m_listenerSocket.Listen(5, m_listenerPort, PSocket::CanReuseAddress)) 
    m_listenerThread = new PThreadObj<OpalMSRPManager>(*this, &OpalMSRPManager::ListenerThread);
  else {
    PTRACE(2, "MSRP\tCannot start MSRP listener on port " << m_listenerPort);
  }
}

OpalMSRPManager::~OpalMSRPManager()
{
  PWaitAndSignal m(mutex);

  m_listenerSocket.Close();
  PThread::WaitAndDelete(m_listenerThread);
}


std::string OpalMSRPManager::CreateSessionID()
{
  PString str = PGloballyUniqueID().AsString();
  return (const char *)str;
}


PURL OpalMSRPManager::SessionIDToURL(const OpalTransportAddress & taddr, const std::string & id)
{
  PIPSocket::Address addr;
  taddr.GetIpAddress(addr);

  PStringStream str;
  str << "msrp://"
      << addr.AsString()
      << ":"
      << m_listenerPort
      << "/"
      << id
      << ";tcp";

  return PURL(str);
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

    PIPSocket * socket = protocol->GetSocket();
    PIPSocketAddressAndPort remoteAddr;
    socket->GetPeerAddress(remoteAddr);

    PTRACE(2, "MSRP\tListener accepted new incoming connection");
    PSafePtr<Connection> connection = new Connection(*this, remoteAddr.AsString(), protocol);
    {
      PWaitAndSignal m(m_connectionInfoMapAddMutex);
      connection.SetSafetyMode(PSafeReference);
      m_connectionInfoMap[remoteAddr.AsString()] = connection;
      connection.SetSafetyMode(PSafeReadWrite);
    }
    connection->StartHandler();
  }
  PTRACE(2, "MSRP\tListener thread ended");
}


PSafePtr<OpalMSRPManager::Connection> OpalMSRPManager::OpenConnection(const PURL & localURL, const PURL & remoteURL)
{
  PIPSocket::AddressAndPort ap;

  // get hostname of remote 
#if P_DNS_RESOLVER
  PIPSocketAddressAndPortVector addresses;
  if (PDNS::LookupSRV(remoteURL.GetHostName(), "_im._msrp", remoteURL.GetPort(), addresses) && !addresses.empty())
    ap = addresses[0];
  else
#endif // P_DNS_RESOLVER
    ap.Parse(remoteURL.GetHostPort());

  PString connectionKey(ap.AsString());

  PSafePtr<Connection> connectionPtr = NULL;

  // see if we already have a connection to that remote host
  // if not, create one and add to the connection map
  {
    PWaitAndSignal m(m_connectionInfoMapAddMutex);
    ConnectionInfoMapType::iterator r = m_connectionInfoMap.find(connectionKey);
    if (r != m_connectionInfoMap.end()) {
      PTRACE(2, "MSRP\tReusing existing connection to " << ap);
      connectionPtr = r->second;
      connectionPtr.SetSafetyMode(PSafeReadWrite);
      ++connectionPtr->m_refCount;
      return connectionPtr;
    }

    connectionPtr = PSafePtr<Connection>(new Connection(*this, connectionKey));
    m_connectionInfoMap[connectionKey] = connectionPtr;
  }

  connectionPtr.SetSafetyMode(PSafeReadWrite);
  
  // create a connection to the remote
  // if cannot, remove it from connection map
  connectionPtr->m_protocol->SetReadTimeout(2000);
  if (!connectionPtr->m_protocol->Connect(ap.GetAddress(), ap.GetPort())) {
    PTRACE(2, "MSRP\tUnable to make new connection to " << ap);
    PWaitAndSignal m(m_connectionInfoMapAddMutex);
    m_connectionInfoMap.erase(connectionKey);
    connectionPtr.SetNULL();
    return NULL;
  }

  PTRACE(2, "MSRP\tConnection established to to " << ap);


  PString uid;
  connectionPtr->m_protocol->SendSEND(localURL, remoteURL, "", "", uid);
  connectionPtr->StartHandler();

  return connectionPtr;
}


bool OpalMSRPManager::CloseConnection(PSafePtr<OpalMSRPManager::Connection> & connection)
{
  PWaitAndSignal m(m_connectionInfoMapAddMutex);
  if (--connection->m_refCount == 0) {
    m_connectionInfoMap.erase(connection->m_key);
    connection.SetNULL();
  }
  return true;
}

void OpalMSRPManager::SetNotifier(const PURL & localUrl,
                                  const PURL & remoteUrl,
                                  const CallBack & notifier)
{
  PString key(localUrl.AsString() + '\t' + remoteUrl.AsString());
  PTRACE(2, "MSRP\tRegistering callback for incoming MSRP messages with '" << key << "'");
  PWaitAndSignal m(m_callBacksMutex);
  m_callBacks[key] = notifier;
}


void OpalMSRPManager::RemoveNotifier(const PURL & localUrl, const PURL & remoteUrl)
{
  PString key(localUrl.AsString() + '\t' + remoteUrl.AsString());
  PWaitAndSignal m(m_callBacksMutex);
  m_callBacks.erase(key);
}

void OpalMSRPManager::DispatchMessage(IncomingMSRP & incomingMsg)
{
  PString fromUrl(incomingMsg.m_mime("From-Path"));
  PString toUrl  (incomingMsg.m_mime("To-Path"));

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
}

////////////////////////////////////////////////////////

OpalMSRPManager::Connection::Connection(OpalMSRPManager & manager, const std::string & key, MSRPProtocol * protocol)
  : m_manager(manager)
  , m_key(key)
  , m_protocol(protocol)
  , m_running(true)
  , m_handlerThread(NULL)
  , m_refCount(1)

{
  PTRACE(3, "MSRP\tCreating connection");
  if (m_protocol == NULL)
    m_protocol = new MSRPProtocol();
}

void OpalMSRPManager::Connection::StartHandler()
{
  m_handlerThread = new PThreadObj<OpalMSRPManager::Connection>(*this, &OpalMSRPManager::Connection::HandlerThread);
}

OpalMSRPManager::Connection::~Connection()
{
  m_running = false;
  PThread::WaitAndDelete(m_handlerThread);

  delete m_protocol;
  m_protocol = NULL;
  PTRACE(3, "MSRP\tDestroying connection");
}


void OpalMSRPManager::Connection::HandlerThread()
{
  PTRACE(2, "MSRP\tMSRP connection thread started");

  m_protocol->SetReadTimeout(1000);

  while (m_running) {

    PIPSocket::SelectList sockets;
    sockets += *m_protocol->GetSocket();

    if (PIPSocket::Select(sockets, 1000) != PChannel::NoError) 
      break;

    if (sockets.GetSize() != 0) {

      PTRACE(3, "MSRP\tMSRP message received");

      OpalMSRPManager::IncomingMSRP incomingMsg;
      if (!m_protocol->ReadMessage(incomingMsg.m_command, incomingMsg.m_chunkId, incomingMsg.m_mime, incomingMsg.m_body))
        break;

      PString fromPath(incomingMsg.m_mime("From-Path"));
      PString toPath (incomingMsg.m_mime("To-Path"));

      if (incomingMsg.m_command == MSRPProtocol::SEND) {
        m_protocol->SendResponse(incomingMsg.m_chunkId, 200, "OK", toPath, fromPath);
        PTRACE(3, "MSRP\tMSRP SEND received from=" << fromPath << ",to=" << toPath);
        if (incomingMsg.m_mime.Contains(PHTTP::ContentTypeTag)) {
          incomingMsg.m_connection = PSafePtr<Connection>(this);
          m_manager.DispatchMessage(incomingMsg);
        }
        if (incomingMsg.m_mime("Success-Report") *= "yes") {
          PMIMEInfo mime;
          PString fromUrl(incomingMsg.m_mime("From-Path"));
          PString toUrl  (incomingMsg.m_mime("To-Path"));
          mime.SetAt("Message-ID", incomingMsg.m_mime("Message-ID"));
          mime.SetAt("Byte-Range", incomingMsg.m_mime("Byte-Range"));
          mime.SetAt("Status",     "000 200 OK");
          m_protocol->SendREPORT(incomingMsg.m_chunkId, toUrl, fromUrl, mime);
        }
      }
    }
  }

  PTRACE(2, "MSRP\tMSRP protocol thread finished");
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
      body = (PHTTP::ContentTypeTag() & message.m_contentType) + CRLF CRLF +
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
    PTRACE(4, "Sending MSRP chunk\n" << "MSRP " << chunkId << " " << MSRPCommands[SEND] << CRLF 
                                       << "To-Path: " << toUrl << CRLF 
                                       << "From-Path: "<< fromUrl << CRLF 
                                       << str << CRLF
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

#if PTRACING
  static const unsigned Level = 4;
  if (PTrace::CanTrace(Level)) {
    ostream & trace = PTRACE_BEGIN(Level, "MSRP");
    trace << "Sending MSRP REPORT\n"
             "MSRP " << chunkId << " " << MSRPCommands[REPORT] << "\n"
             "To-Path: " << toUrl << "\n"
             "From-Path: " << fromUrl << '\n'
             << ::setfill('\r');
    mime.PrintContents(trace);
    trace << "-------" << chunkId << '$'
          << PTrace::End;
  }
#endif

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
  PString startLine;
  do {
    if (!ReadLine(startLine, false)) {
      PTRACE(2, "MSRP\tError while reading MSRP command");
      return false;
    }
  } while (startLine.IsEmpty());

  // get tokens
  PStringArray tokens = startLine.Tokenise(' ', false);
  if (tokens.GetSize() < 3) {
    PTRACE(2, "MSRP\tReceived malformed MSRP command line \"" << startLine << "\" with " << tokens.GetSize() << " tokens");
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
  {
    mime.RemoveAll();
    PString line;
    while (ReadLine(line, false)) {
      if (line.IsEmpty())
        break;
      if (line.Find(terminator) == 0)
        break;
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
  if ((command == SEND) && mime.Contains(PHTTP::ContentTypeTag)) {
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
    PTRACE(4, "Received MSRP message\n" << startLine << "\n" << str << body << terminator);
  }

  return true;
}




////////////////////////////////////////////////////////

#endif //  OPAL_HAS_MSRP
