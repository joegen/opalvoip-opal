/*
 * sockep.cxx
 *
 * Media from socket endpoint.
 *
 * Open Phone Abstraction Library (OPAL)
 *
 * Copyright (c) 2018 Vox Lucida Pty. Ltd.
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
 * The Initial Developer of the Original Code is Vox Lucdia Pty. Ltd.
 *
 * Contributor(s): Robert Jongbleod robertj@voxlucida.com.au.
 */

#include <ptlib.h>

#include <ep/sockep.h>
#include <codec/vidcodec.h>


#define PTraceModule() "sock-ep"
#define new PNEW


OpalSockEndPoint::OpalSockEndPoint(OpalManager & manager, const char * prefix)
  : OpalLocalEndPoint(manager, prefix)
{
  m_defaultAudioSynchronicity = e_Asynchronous;
  m_defaultVideoSourceSynchronicity = e_Asynchronous;
}


OpalSockEndPoint::~OpalSockEndPoint()
{
}


PSafePtr<OpalConnection> OpalSockEndPoint::MakeConnection(OpalCall & call,
                                                     const PString & remoteParty,
                                                              void * userData,
                                                        unsigned int options,
                                     OpalConnection::StringOptions * stringOptions)
{
  OpalConnection::StringOptions localStringOptions;

  // First strip of the prefix if present
  if (remoteParty.Find(GetPrefixName() + ":") == 0)
    PURL::SplitVars(remoteParty.Mid(GetPrefixName().GetLength() + 1), localStringOptions, ',', '=');
  else
    PURL::SplitVars(remoteParty, localStringOptions, ',', '=');

  if (stringOptions != NULL)
    localStringOptions.Merge(*stringOptions, PStringToString::e_MergeAppend);

  return AddConnection(CreateConnection(call, userData, options, &localStringOptions));
}


OpalLocalConnection * OpalSockEndPoint::CreateConnection(OpalCall & call,
                                                        void * userData,
                                                        unsigned options,
                                                        OpalConnection::StringOptions * stringOptions)
{
  return new OpalSockConnection(call, *this, userData, options, stringOptions);
}


///////////////////////////////////////////////////////////////////////////////

OpalSockConnection::OpalSockConnection(OpalCall & call,
                                       OpalSockEndPoint & endpoint,
                                       void * userData,
                                       unsigned options,
                                       OpalConnection::StringOptions * stringOptions)
  : OpalLocalConnection(call, endpoint, userData, options, stringOptions, 'I')
  , m_endpoint(endpoint)
  , m_audioSocket(NULL)
#if OPAL_VIDEO
  , m_videoSocket(NULL)
#endif
{
}


OpalSockConnection::~OpalSockConnection()
{
  delete m_audioSocket;
#if OPAL_VIDEO
  delete m_videoSocket;
#endif
}


void OpalSockConnection::OnReleased()
{
  if (m_audioSocket != NULL)
    m_audioSocket->Close();

#if OPAL_VIDEO
  if (m_videoSocket != NULL)
    m_videoSocket->Close();
#endif

  OpalLocalConnection::OnReleased();
}


OpalMediaFormatList OpalSockConnection::GetMediaFormats() const
{
  OpalMediaFormatList fmts;
  fmts += m_stringOptions.GetString(OPAL_OPT_SOCK_EP_AUDIO_CODEC, OPAL_OPUS48);
#if OPAL_VIDEO
  OpalMediaFormat video = m_stringOptions.GetString(OPAL_OPT_SOCK_EP_VIDEO_CODEC, OPAL_H264_MODE1);
  if (video.IsValid()) {
    double fps = m_stringOptions.GetReal(OPAL_OPT_SOCK_EP_FRAME_RATE);
    if (fps > 0)
      video.SetOptionInteger(OpalMediaFormat::FrameTimeOption(), static_cast<int>(video.GetClockRate() / fps));
    fmts += video;
  }
#endif // OPAL_VIDEO
  return fmts;
}


bool OpalSockConnection::TransferConnection(const PString & remoteParty)
{
  return OpalLocalConnection::TransferConnection(remoteParty);
}


bool OpalSockConnection::OnOutgoing()
{
  return OpenMediaSockets() && OpalLocalConnection::OnOutgoing();
}


bool OpalSockConnection::OnIncoming()
{
  // Returning false is EndedByLocalBusy
  return OpenMediaSockets() && OpalLocalConnection::OnIncoming();
}


bool OpalSockConnection::OpenMediaSockets()
{
  if (!OpenMediaSocket(m_audioSocket, OPAL_OPT_SOCK_EP_AUDIO_IP, OPAL_OPT_SOCK_EP_AUDIO_PORT, OPAL_OPT_SOCK_EP_AUDIO_PROTO))
    return false;

#if OPAL_VIDEO
  if (!OpenMediaSocket(m_videoSocket, OPAL_OPT_SOCK_EP_VIDEO_IP, OPAL_OPT_SOCK_EP_VIDEO_PORT, OPAL_OPT_SOCK_EP_VIDEO_PROTO))
    return false;
#endif

  PTRACE(3, "Opened media sockets.");
  return true;
}


bool OpalSockConnection::OpenMediaSocket(PIPSocket * & socket,
                                         const PString & addrKey,
                                         const PString & portKey,
                                         const PCaselessString & protoKey)
{
  delete socket;
  socket = NULL;

  PString addr = m_stringOptions.GetString(addrKey);
  if (!PIPAddress(addr).IsValid()) {
    PTRACE(2, "Invalid IP address provided: \"" << addr << '"');
    return false;
  }

  long port = m_stringOptions.GetInteger(portKey);
  if (port < 1024 || port > 65535) {
    PTRACE(2, "Invalid port provided: " << port);
    return false;
  }

  PCaselessString proto = m_stringOptions.GetString(protoKey, "tcp");
  if (proto == "tcp")
    socket = new PTCPSocket;
  else if (proto == "udp")
    socket = new PUDPSocket;
  else {
    PTRACE(2, "Invalid protocol provided: \"" << proto << '"');
    return false;
  }

  socket->SetPort((uint16_t)port);
  if (socket->Connect(addr)) {
    PTRACE(3, "Connected to media socket: \"" << socket->GetPeerAddress() << '"');
    return true;
  }

  PTRACE(2, "Could not connect to " << addr << ':' << port);
  delete socket;
  socket = NULL;
  return false;
}


#pragma pack(1)
struct MediaSockHeader {
  uint8_t  m_headerSize;
  uint8_t  m_flags;    // bit 0 is marker bit (last packet in video frame)
  PUInt16b m_length;
};
#pragma pack()


bool OpalSockConnection::OnReadMediaData(OpalMediaStream & mediaStream,
                                         void * data,
                                         PINDEX size,
                                         PINDEX & length)
{
  OpalMediaType mediaType = mediaStream.GetMediaFormat().GetMediaType();

  PIPSocket * socket;
  if (mediaType == OpalMediaType::Audio())
    socket = m_audioSocket;
#if OPAL_VIDEO
  else if (mediaType == OpalMediaType::Video())
    socket = m_videoSocket;
#endif
  else {
    PTRACE(2, "Unsupported media type: " << mediaType);
    return false;
  }

  if (socket == NULL || !socket->IsOpen()) {
    PTRACE(2, "Socket not open for " << mediaType);
    return false;
  }

  MediaSockHeader hdr;
  if (!socket->ReadBlock(&hdr, sizeof(hdr))) {
    PTRACE(2, "Socket read error for " << mediaType << " - " << socket->GetErrorText());
    return false;
  }
  length = hdr.m_length;
  mediaStream.SetMarker((hdr.m_flags & 1) != 0);

  if (length > size) {
    PTRACE(2, "Unexpectedly large data for " << mediaType << ": " << length << " > " << size);
    return false;
  }

  if (socket->ReadBlock(data, length))
    return true;

  PTRACE(2, "Socket read error for " << mediaType << " - " << socket->GetErrorText());
  return false;
}


bool OpalSockConnection::OnWriteMediaData(const OpalMediaStream & mediaStream,
                                          const void * data,
                                          PINDEX length,
                                          PINDEX & written)
{
  OpalMediaType mediaType = mediaStream.GetMediaFormat().GetMediaType();

  PIPSocket * socket;
  if (mediaType == OpalMediaType::Audio())
    socket = m_audioSocket;
#if OPAL_VIDEO
  else if (mediaType == OpalMediaType::Video())
    socket = m_videoSocket;
#endif
  else {
    PTRACE(2, "Unsupported media type: " << mediaType);
    return false;
  }

  if (socket == NULL || !socket->IsOpen()) {
    PTRACE(2, "Socket not open for " << mediaType);
    return false;
  }

  MediaSockHeader hdr;
  hdr.m_headerSize = sizeof(hdr);
  hdr.m_flags = mediaStream.GetMarker() ? 1 : 0;
  hdr.m_length = (uint16_t)length;
  if (!socket->Write(&hdr, sizeof(hdr)) || !socket->Write(data, length)) {
    PTRACE(2, "Socket write error for " << mediaType << " - " << socket->GetErrorText());
    return false;
  }

  written = socket->GetLastWriteCount();
  return true;
}


bool OpalSockConnection::OnMediaCommand(OpalMediaStream & stream, const OpalMediaCommand & command)
{
#if OPAL_VIDEO
  if (stream.IsSource() == (&stream.GetConnection() == this)) {
    const OpalVideoUpdatePicture * vup = dynamic_cast<const OpalVideoUpdatePicture *>(&command);
    if (vup != NULL) {
      MediaSockHeader hdr;
      hdr.m_headerSize = sizeof(hdr);
      hdr.m_flags = 2;
      hdr.m_length = 0;
      if (!m_videoSocket->Write(&hdr, sizeof(hdr))) {
        PTRACE(2, "Socket write error for video - " << m_videoSocket->GetErrorText());
      }
      return true;
    }

    const OpalMediaFlowControl * flow = dynamic_cast<const OpalMediaFlowControl *>(&command);
    if (flow != NULL) {
      struct {
        MediaSockHeader hdr;
        PUInt32b rate;
      } cmd;
      cmd.hdr.m_headerSize = sizeof(cmd.hdr);
      cmd.hdr.m_flags = 4;
      cmd.hdr.m_length = 4;
      cmd.rate = flow->GetMaxBitRate();
      if (!m_videoSocket->Write(&cmd, sizeof(cmd))) {
        PTRACE(2, "Socket write error for video - " << m_videoSocket->GetErrorText());
      }
      return true;
    }
  }
#endif // OPAL_VIDEO

  return OpalLocalConnection::OnMediaCommand(stream, command);
}
