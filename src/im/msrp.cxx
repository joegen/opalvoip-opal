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

#include <opal/transports.h>
#include <opal/mediatype.h>
#include <opal/mediafmt.h>
#include <opal/endpoint.h>

#include <im/im.h>
#include <im/msrp.h>

#if OPAL_HAS_MSRP

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

  SDPMediaFormat * sdpFormat = new SDPMediaFormat(*this, mediaFormat.GetPayloadType(), mediaFormat);
  ProcessMediaOptions(*sdpFormat, mediaFormat);
  AddSDPMediaFormat(sdpFormat);
}


#endif // OPAL_SIP

////////////////////////////////////////////////////////////////////////////////////////////

MSRPSession::MSRPSession(OpalMSRPManager & _manager)
  : manager(_manager)
{
  // sessionIDs are supposed to unguessable
  msrpSessionId = manager.OpenSession();
  url           = manager.SessionIDToPath(msrpSessionId);
}

MSRPSession::~MSRPSession()
{
  manager.CloseSession(msrpSessionId);
}

#if OPAL_SIP

SDPMediaDescription * MSRPSession::CreateSDPMediaDescription(const OpalTransportAddress & sdpContactAddress)
{
  return new SDPMSRPMediaDescription(sdpContactAddress, url);
}

#endif

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
{
  msrpSession = new MSRPSession(MSRPInitialiser::KickStart(_conn.GetEndPoint().GetManager()));
}

OpalMSRPMediaSession::OpalMSRPMediaSession(const OpalMSRPMediaSession & _obj)
  : OpalMediaSession(_obj)
  , msrpSession(_obj.msrpSession)
{
}

void OpalMSRPMediaSession::Close()
{
  if (msrpSession != NULL) {
    delete msrpSession;
    msrpSession = NULL;
  }
}

OpalTransportAddress OpalMSRPMediaSession::GetLocalMediaAddress() const
{
  OpalTransportAddress addr;
  if (msrpSession->GetManager().GetLocalAddress(addr))
    return addr;

  return OpalTransportAddress();
}

#if OPAL_SIP

SDPMediaDescription * OpalMSRPMediaSession::CreateSDPMediaDescription(const OpalTransportAddress & sdpContactAddress)
{
  return msrpSession->CreateSDPMediaDescription(sdpContactAddress);
}

#endif

OpalMediaStream * OpalMSRPMediaSession::CreateMediaStream(const OpalMediaFormat & mediaFormat, 
                                                                         unsigned sessionID, 
                                                                         PBoolean isSource)
{
  PTRACE(2, "MSRP\tCreated " << (isSource ? "source" : "sink") << " media stream in " << (connection.IsOriginating() ? "originator" : "receiver") << " with " << msrpSession->GetURL());
  return new OpalMSRPMediaStream(connection, mediaFormat, sessionID, isSource, *this);
}

void OpalMSRPMediaSession::SetRemoteMediaAddress(const OpalTransportAddress &, const OpalMediaFormatList & )
{
}

////////////////////////////////////////////////////////////////////////////////////////////

OpalMSRPManager::OpalMSRPManager(OpalManager & _opalManager, WORD _port)
  : opalManager(_opalManager), listeningPort(_port), listeningThread(NULL)
{
}

OpalMSRPManager::~OpalMSRPManager()
{
  PWaitAndSignal m(mutex);

  if (listeningThread != NULL) {
    listeningSocket.Close();
    listeningThread->WaitForTermination();
    delete listeningThread;
  }
}

std::string OpalMSRPManager::OpenSession()
{
  PWaitAndSignal m(mutex);

  std::string id = psprintf("%c%08x%u", PRandom::Number('a', 'z'), PRandom::Number(), ++lastID);
  SessionInfo sessionInfo;
  sessionInfoMap.insert(SessionInfoMap::value_type(id, sessionInfo));

  PTRACE(2, "MSRP\tSession opened - " << sessionInfoMap.size() << " sessions now in progress");

  return id;
}

std::string OpalMSRPManager::SessionIDToPath(const std::string & id)
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
      << listeningPort
      << "/"
      << id
      << ";tcp";

  return str;
}

void OpalMSRPManager::CloseSession(const std::string & id)
{
  PWaitAndSignal m(mutex);

  SessionInfoMap::iterator r = sessionInfoMap.find(id);
  if (r != sessionInfoMap.end())
    sessionInfoMap.erase(r);

  PTRACE(2, "MSRP\tSession opened - " << sessionInfoMap.size() << " sessions now in progress");
}

bool OpalMSRPManager::GetLocalAddress(OpalTransportAddress & addr)
{
  PWaitAndSignal m(mutex);

  if (!listeningSocket.IsOpen()) {
    if (!listeningSocket.Listen(5, listeningPort)) {
      PTRACE(2, "MSRP\tCannot start MSRP listener on port " << listeningPort);
      return false;
    }

    listeningThread = new PThreadObj<OpalMSRPManager>(*this, &OpalMSRPManager::ThreadMain);

    PIPSocket::Address ip; WORD port;
    listeningSocket.GetLocalAddress(ip, port);
    if (ip.IsAny()) {
      if (!PIPSocket::GetNetworkInterface(ip)) {
        PTRACE(2, "MSRP\tUnable to get specific IP address for MSRP listener");
        return false;
      }
    }

    listeningAddress = OpalTransportAddress(ip, port);
    PTRACE(2, "MSRP\tListener started on " << listeningAddress);
  }

  addr = listeningAddress;

  return true;
}

bool OpalMSRPManager::GetLocalPort(WORD & port)
{
  port = listeningPort;
  return true;
}

void OpalMSRPManager::ThreadMain()
{
  PTRACE(2, "MSRP\tListener thread started");
  for (;;) {
    PTCPSocket * socket = new PTCPSocket;
    if (!socket->Accept(listeningSocket)) {
      delete socket;
      break;
    }
  }
  PTRACE(2, "MSRP\tListener thread ended");
}

////////////////////////////////////////////////////////

OpalMediaSession * OpalMSRPMediaType::CreateMediaSession(OpalConnection & conn, unsigned sessionID) const
{
  // as this is called in the constructor of an OpalConnection descendant, 
  // we can't use a virtual function on OpalConnection

#if OPAL_SIP
  if (conn.GetPrefixName() *= "sip")
    return new OpalMSRPMediaSession(conn, sessionID);
#endif

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
{
}

OpalMSRPMediaStream::~OpalMSRPMediaStream()
{
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
      const BYTE * /*data*/,   ///<  Data to write
      PINDEX /*length*/,       ///<  Length of data to read.
      PINDEX & /*written*/     ///<  Length of data actually written
    )
{
  if (!isOpen)
    return false;

  // TODO: write MSRP frame via MSRP manager
  // m_msrpSession.xxxxxx

  return true;
}

PBoolean OpalMSRPMediaStream::Close()
{
  return OpalIMMediaStream::Close();
}

////////////////////////////////////////////////////////

#endif //  OPAL_HAS_MSRP
