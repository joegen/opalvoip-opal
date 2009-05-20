/*
 * sipim.cxx
 *
 * Support for SIP session mode IM
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
#pragma implementation "sipim.h"
#endif

#include <ptlib/socket.h>
#include <ptclib/random.h>

#include <opal/transports.h>
#include <opal/mediatype.h>
#include <opal/mediafmt.h>
#include <opal/endpoint.h>

#include <im/im.h>
#include <im/sipim.h>
#include <sip/sipep.h>

#if OPAL_HAS_SIPIM

////////////////////////////////////////////////////////////////////////////

OpalSIPIMMediaType::OpalSIPIMMediaType()
  : OpalIMMediaType("sip-im", "message|sip", 6)
{
}


/////////////////////////////////////////////////////////
//
//  SDP media description for the SIPIM type
//
//  A new class is needed for "message" due to the following differences
//
//  - the SDP type is "message"
//  - the transport is "sip"
//  - the format list is a SIP URL
//

class SDPSIPIMMediaDescription : public SDPMediaDescription
{
  PCLASSINFO(SDPSIPIMMediaDescription, SDPMediaDescription);
  public:
    SDPSIPIMMediaDescription(const OpalTransportAddress & address);
    SDPSIPIMMediaDescription(const OpalTransportAddress & address, const OpalTransportAddress & _transportAddr, const PString & fromURL);

    PCaselessString GetSDPTransportType() const
    { return "sip"; }

    virtual PString GetSDPMediaType() const 
    { return "message"; }

    virtual PString GetSDPPortList() const;

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
    OpalTransportAddress transportAddress;
    PString fromURL;
};

////////////////////////////////////////////////////////////////////////////////////////////

SDPMediaDescription * OpalSIPIMMediaType::CreateSDPMediaDescription(const OpalTransportAddress & localAddress)
{
  return new SDPSIPIMMediaDescription(localAddress);
}

///////////////////////////////////////////////////////////////////////////////////////////

SDPSIPIMMediaDescription::SDPSIPIMMediaDescription(const OpalTransportAddress & address)
  : SDPMediaDescription(address)
{
  SetDirection(SDPMediaDescription::SendRecv);
}

SDPSIPIMMediaDescription::SDPSIPIMMediaDescription(const OpalTransportAddress & address, const OpalTransportAddress & _transportAddr, const PString & _fromURL)
  : SDPMediaDescription(address), transportAddress(_transportAddr), fromURL(_fromURL)
{
  SetDirection(SDPMediaDescription::SendRecv);
}

void SDPSIPIMMediaDescription::CreateSDPMediaFormats(const PStringArray &)
{
  formats.Append(new SDPMediaFormat(*this, OpalSIPIM));
}


bool SDPSIPIMMediaDescription::PrintOn(ostream & str, const PString & /*connectString*/) const
{
  // call ancestor
  if (!SDPMediaDescription::PrintOn(str, ""))
    return false;

  return true;
}

PString SDPSIPIMMediaDescription::GetSDPPortList() const
{ 
  PIPSocket::Address addr; WORD port;
  transportAddress.GetIpAndPort(addr, port);

  PStringStream str;
  str << " " << fromURL << "@" << addr << ":" << port;

  return str;
}


void SDPSIPIMMediaDescription::SetAttribute(const PString & /*attr*/, const PString & /*value*/)
{
}

void SDPSIPIMMediaDescription::ProcessMediaOptions(SDPMediaFormat & /*sdpFormat*/, const OpalMediaFormat & /*mediaFormat*/)
{
}

OpalMediaFormatList SDPSIPIMMediaDescription::GetMediaFormats() const
{
  OpalMediaFormat sipim(OpalSIPIM);
  sipim.SetOptionString("URL", fromURL);

  PTRACE(4, "SIPIM\tNew format is " << setw(-1) << sipim);

  OpalMediaFormatList fmts;
  fmts += sipim;
  return fmts;
}

void SDPSIPIMMediaDescription::AddMediaFormat(const OpalMediaFormat & mediaFormat)
{
  if (!mediaFormat.IsTransportable() || !mediaFormat.IsValidForProtocol("sip") || mediaFormat.GetMediaType() != "sip-im") {
    PTRACE(4, "SIPIM\tSDP not including " << mediaFormat << " as it is not a valid SIPIM format");
    return;
  }

  SDPMediaFormat * sdpFormat = new SDPMediaFormat(*this, mediaFormat);
  ProcessMediaOptions(*sdpFormat, mediaFormat);
  AddSDPMediaFormat(sdpFormat);
}


////////////////////////////////////////////////////////////////////////////////////////////

OpalMediaSession * OpalSIPIMMediaType::CreateMediaSession(OpalConnection & conn, unsigned sessionID) const
{
  // as this is called in the constructor of an OpalConnection descendant, 
  // we can't use a virtual function on OpalConnection

  if (conn.GetPrefixName() *= "sip")
    return new OpalSIPIMMediaSession(conn, sessionID);

  return NULL;
}

////////////////////////////////////////////////////////////////////////////////////////////

OpalSIPIMMediaSession::OpalSIPIMMediaSession(OpalConnection & _conn, unsigned _sessionId)
: OpalMediaSession(_conn, "sip-im", _sessionId)
{
  transportAddress = connection.GetTransport().GetLocalAddress();
  localURL         = connection.GetLocalPartyURL();
  remoteURL        = connection.GetRemotePartyURL();
  callId           = connection.GetToken();
}

OpalSIPIMMediaSession::OpalSIPIMMediaSession(const OpalSIPIMMediaSession & obj)
  : OpalMediaSession(obj)
{
  transportAddress = obj.transportAddress;
  localURL         = obj.localURL;        
  remoteURL        = obj.remoteURL;       
  callId           = obj.callId;          
}

OpalTransportAddress OpalSIPIMMediaSession::GetLocalMediaAddress() const
{
  return transportAddress;
}


SDPMediaDescription * OpalSIPIMMediaSession::CreateSDPMediaDescription(const OpalTransportAddress & sdpContactAddress)
{
  return new SDPSIPIMMediaDescription(sdpContactAddress, transportAddress, localURL);
}


OpalMediaStream * OpalSIPIMMediaSession::CreateMediaStream(const OpalMediaFormat & mediaFormat, 
                                                                         unsigned sessionID, 
                                                                         PBoolean isSource)
{
  PTRACE(2, "SIPIM\tCreated " << (isSource ? "source" : "sink") << " media stream in " << (connection.IsOriginating() ? "originator" : "receiver") << " with local " << localURL << " and remote " << remoteURL);
  return new OpalSIPIMMediaStream(connection, mediaFormat, sessionID, isSource, *this);
}

void OpalSIPIMMediaSession::SetRemoteMediaAddress(const OpalTransportAddress &, const OpalMediaFormatList &)
{
}

bool OpalSIPIMMediaSession::SendMessage(const PString & /*contentType*/, const PString & body)
{
  SIPEndPoint * ep = dynamic_cast<SIPEndPoint *>(&connection.GetEndPoint());
  if (ep == NULL)
    return false;

  return ep->Message(remoteURL, body, localURL, callId);
}

bool OpalSIPIMMediaSession::SendIM(const PString & /*contentType*/, const PString & body)
{
  return connection.SendIM(OpalSIPIM, body);
}


////////////////////////////////////////////////////////////////////////////////////////////

OpalSIPIMMediaStream::OpalSIPIMMediaStream(
      OpalConnection & conn,
      const OpalMediaFormat & mediaFormat, ///<  Media format for stream
      unsigned sessionID,                  ///<  Session number for stream
      bool isSource,                        ///<  Is a source stream
      OpalSIPIMMediaSession & mediaSession
)
  : OpalIMMediaStream(conn, mediaFormat, sessionID, isSource)
  , m_imSession(mediaSession)
{
}

OpalSIPIMMediaStream::~OpalSIPIMMediaStream()
{
}

bool OpalSIPIMMediaStream::Open()
{
  if (!OpalIMMediaStream::Open())
    return false;

  SIPEndPoint * ep = dynamic_cast<SIPEndPoint *>(&connection.GetEndPoint());
  if (ep == NULL) 
    return false;

  ep->GetSIPIMManager().StartSession(&m_imSession);

  return true;
}


PBoolean OpalSIPIMMediaStream::Close()
{
  if (!OpalIMMediaStream::Close())
    return false;

  SIPEndPoint * ep = dynamic_cast<SIPEndPoint *>(&connection.GetEndPoint());
  if (ep == NULL) 
    return false;

  ep->GetSIPIMManager().EndSession(&m_imSession);

  return true;
}

PBoolean OpalSIPIMMediaStream::ReadData(BYTE *,PINDEX,PINDEX &)
{
  PAssertAlways("Cannot ReadData from OpalSIPIMMediaStream");
  return false;
}

PBoolean OpalSIPIMMediaStream::WriteData(
      const BYTE * data,   ///<  Data to write
            PINDEX length,       ///<  Length of data to read.
          PINDEX & written     ///<  Length of data actually written
    )
{
  if (!IsOpen())
    return false;

  bool stat = true;

  if (length != 0 && data != NULL) {
    // T.140 data has 3 bytes at the start of the data
    if (length > 4) {
      PString body((const char *)data + 3, length-3);
      stat = m_imSession.SendMessage("", body);
    }
    written = length;
  }

  return stat;
}

////////////////////////////////////////////////////////////////////////////////////////////

OpalSIPIMManager::OpalSIPIMManager(SIPEndPoint & endpoint)
  : m_endpoint(endpoint)
{
}

void OpalSIPIMManager::OnReceivedMessage(const SIP_PDU & pdu)
{
  PString callId = pdu.GetMIME().GetCallID();
  if (!callId.IsEmpty()) {
    PWaitAndSignal m(m_mutex);
    IMSessionMapType::iterator r = m_imSessionMap.find((const char *)callId);
    if (r != m_imSessionMap.end()) {
      r->second->SendIM(pdu.GetMIME().GetContentEncoding(), pdu.GetEntityBody());
    }
  }
}

bool OpalSIPIMManager::StartSession(OpalSIPIMMediaSession * mediaSession)
{ 
  PWaitAndSignal m(m_mutex);
  PString callId(mediaSession->GetCallID());
  m_imSessionMap.insert(IMSessionMapType::value_type((const char *)callId, mediaSession));
  return true;
}

bool OpalSIPIMManager::EndSession(OpalSIPIMMediaSession * mediaSession)
{ 
  PWaitAndSignal m(m_mutex);
  PString callId(mediaSession->GetCallID());
  IMSessionMapType::iterator r = m_imSessionMap.find((const char *)callId);
  if (r != m_imSessionMap.end())
    m_imSessionMap.erase(r);
  return true;
}

#endif // OPAL_HAS_SIPIM
