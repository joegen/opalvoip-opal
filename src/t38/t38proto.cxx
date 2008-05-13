/*
 * t38proto.cxx
 *
 * T.38 protocol handler
 *
 * Open Phone Abstraction Library
 *
 * Copyright (c) 1998-2002 Equivalence Pty. Ltd.
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
 * The Original Code is Open H323 Library.
 *
 * The Initial Developer of the Original Code is Equivalence Pty. Ltd.
 *
 * Contributor(s): Vyacheslav Frolov.
 *
 * $Revision$
 * $Author$
 * $Date$
 */

#include <ptlib.h>

#ifdef __GNUC__
#pragma implementation "t38proto.h"
#endif

#include <t38/t38proto.h>


/////////////////////////////////////////////////////////////////////////////

#if OPAL_FAX

#include <asn/t38.h>

#define new PNEW

namespace PWLibStupidLinkerHacks {
  int t38Loader;
};

#define SPANDSP_AUDIO_SIZE    320

static PAtomicInteger faxCallIndex;

/////////////////////////////////////////////////////////////////////////////

class T38PseudoRTP_Handler : public RTP_FormatHandler
{
  public:
    void OnStart(RTP_Session & _rtpUDP)
    {  
      RTP_FormatHandler::OnStart(_rtpUDP);
      rtpUDP->SetJitterBufferSize(0, 0);
      corrigendumASN        = PTrue;
      consecutiveBadPackets = 0;
      oneGoodPacket         = false;

      lastIFP.SetSize(0);
      rtpUDP->SetReportTimeInterval(20);
    }

    RTP_Session::SendReceiveStatus OnSendData(RTP_DataFrame & frame);
    RTP_Session::SendReceiveStatus OnSendControl(RTP_ControlFrame & /*frame*/, PINDEX & /*len*/);
    RTP_Session::SendReceiveStatus ReadDataPDU(RTP_DataFrame & frame);
    int WaitForPDU(PUDPSocket & dataSocket, PUDPSocket & controlSocket, const PTimeInterval &);
    RTP_Session::SendReceiveStatus OnReadTimeout(RTP_DataFrame & frame);
    PBoolean WriteData(RTP_DataFrame & frame);

  protected:
    PBoolean corrigendumASN;
    int consecutiveBadPackets;
    bool oneGoodPacket;
    PBYTEArray lastIFP;
    PBYTEArray thisUDPTL;
};

static PFactory<RTP_FormatHandler>::Worker<T38PseudoRTP_Handler> t38PseudoRTPHandler("udptl");

RTP_Session::SendReceiveStatus T38PseudoRTP_Handler::OnSendData(RTP_DataFrame & frame)
{
  if (frame.GetPayloadSize() == 0)
    return RTP_UDP::e_IgnorePacket;

  PINDEX plLen = frame.GetPayloadSize();

  // reformat the raw T.38 data as an UDPTL packet
  T38_UDPTLPacket udptl;
  udptl.m_seq_number = frame.GetSequenceNumber();
  udptl.m_primary_ifp_packet.SetValue(frame.GetPayloadPtr(), plLen);

  udptl.m_error_recovery.SetTag(T38_UDPTLPacket_error_recovery::e_secondary_ifp_packets);
  T38_UDPTLPacket_error_recovery_secondary_ifp_packets & secondary = udptl.m_error_recovery;
  T38_UDPTLPacket_error_recovery_secondary_ifp_packets & redundantPackets = secondary;
  if (lastIFP.GetSize() == 0)
    redundantPackets.SetSize(0);
  else {
    redundantPackets.SetSize(1);
    T38_UDPTLPacket_error_recovery_secondary_ifp_packets_subtype & redundantPacket = redundantPackets[0];
    redundantPacket.SetValue(lastIFP, lastIFP.GetSize());
  }

  lastIFP = udptl.m_primary_ifp_packet;

  PTRACE(5, "T38_RTP\tEncoded transmitted UDPTL data :\n  " << setprecision(2) << udptl);

  PPER_Stream rawData;
  udptl.Encode(rawData);
  rawData.CompleteEncoding();

#if 0
  // Calculate the level of redundency for this data phase
  PINDEX maxRedundancy;
  if (ifp.m_type_of_msg.GetTag() == T38_Type_of_msg::e_t30_indicator)
    maxRedundancy = indicatorRedundancy;
  else if ((T38_Type_of_msg_data)ifp.m_type_of_msg  == T38_Type_of_msg_data::e_v21)
    maxRedundancy = lowSpeedRedundancy;
  else
    maxRedundancy = highSpeedRedundancy;

  // Push down the current ifp into redundant data
  if (maxRedundancy > 0)
    redundantIFPs.InsertAt(0, new PBYTEArray(udptl.m_primary_ifp_packet.GetValue()));

  // Remove redundant data that are surplus to requirements
  while (redundantIFPs.GetSize() > maxRedundancy)
    redundantIFPs.RemoveAt(maxRedundancy);
#endif

  // copy the UDPTL into the RTP packet
  frame.SetSize(rawData.GetSize());
  memcpy(frame.GetPointer(), rawData.GetPointer(), rawData.GetSize());

  PTRACE(4, "T38_RTP\tSending UDPTL of size " << frame.GetSize());

  return RTP_Session::e_ProcessPacket;
}

RTP_Session::SendReceiveStatus T38PseudoRTP_Handler::OnSendControl(RTP_ControlFrame & /*frame*/, PINDEX & /*len*/)
{
  return RTP_Session::e_IgnorePacket; // Non fatal error, just ignore
}

RTP_Session::SendReceiveStatus T38PseudoRTP_Handler::ReadDataPDU(RTP_DataFrame & frame)
{
  thisUDPTL.SetMinSize(500);
  RTP_Session::SendReceiveStatus status = rtpUDP->ReadDataOrControlPDU(rtpUDP->GetDataSocket(), thisUDPTL, PTrue);
  if (status != RTP_Session::e_ProcessPacket)
    return status;

  PINDEX pduSize = rtpUDP->GetDataSocket().GetLastReadCount();
  
  PTRACE(4, "T38_RTP\tRead UDPTL of size " << pduSize);

  if ((pduSize == 1) && (thisUDPTL[0] == 0xff)) {
    // ignore T.38 timing frames 
    frame.SetPayloadSize(0);
  }
  else {
    PPER_Stream rawData(thisUDPTL.GetPointer(), pduSize);

    // Decode the PDU
    T38_UDPTLPacket udptl;
    if (!udptl.Decode(rawData)) {
#if PTRACING
      if (oneGoodPacket)
        PTRACE(2, "RTP_T38\tRaw data decode failure:\n  "
               << setprecision(2) << rawData << "\n  UDPTL = "
               << setprecision(2) << udptl);
      else
        PTRACE(2, "RTP_T38\tRaw data decode failure: " << rawData.GetSize() << " bytes.");
#endif

      consecutiveBadPackets++;
      if (consecutiveBadPackets < 100)
        return RTP_Session::e_IgnorePacket;

      PTRACE(1, "RTP_T38\tRaw data decode failed 100 times, remote probably not switched from audio, aborting!");
      return RTP_Session::e_AbortTransport;
    }

    consecutiveBadPackets = 0;
    PTRACE_IF(3, !oneGoodPacket, "T38_RTP\tFirst decoded UDPTL packet");
    oneGoodPacket = true;

    PASN_OctetString & ifp = udptl.m_primary_ifp_packet;
    frame.SetPayloadSize(ifp.GetDataLength());

    memcpy(frame.GetPayloadPtr(), ifp.GetPointer(), ifp.GetDataLength());
    frame.SetSequenceNumber((WORD)(udptl.m_seq_number & 0xffff));
    PTRACE(5, "T38_RTP\tDecoded UDPTL packet:\n  " << setprecision(2) << udptl);
  }

  return RTP_FormatHandler::OnReceiveData(frame);
}

PBoolean T38PseudoRTP_Handler::WriteData(RTP_DataFrame & frame)
{
  return RTP_FormatHandler::WriteData(frame);
}

int T38PseudoRTP_Handler::WaitForPDU(PUDPSocket & dataSocket, PUDPSocket & controlSocket, const PTimeInterval &)
{
  // wait for no longer than 20ms so audio gets correctly processed
  return PSocket::Select(dataSocket, controlSocket, 20);
}

RTP_Session::SendReceiveStatus T38PseudoRTP_Handler::OnReadTimeout(RTP_DataFrame & frame)
{
  // for timeouts, send a "fake" payload of one byte of 0xff to keep the T.38 engine emitting PCM
  frame.SetPayloadSize(1);
  frame.GetPayloadPtr()[0] = 0xff;
  return RTP_Session::e_ProcessPacket;
}


/////////////////////////////////////////////////////////////////////////////

static PMutex faxMapMutex;
typedef std::map<std::string, OpalFaxCallInfo *> OpalFaxCallInfoMap_T;

static OpalFaxCallInfoMap_T faxCallInfoMap;

OpalFaxCallInfo::OpalFaxCallInfo()
{ 
  refCount = 1; 
  spanDSPPort = 0; 
}

/////////////////////////////////////////////////////////////////////////////

OpalFaxMediaStream::OpalFaxMediaStream(OpalConnection & conn, 
                                const OpalMediaFormat & mediaFormat, 
                                               unsigned sessionID, 
                                               PBoolean isSource, 
                                       const PString & _token, 
                                       const PString & _filename, 
                                              PBoolean _receive,
                                       const PString & _stationId)
  : OpalMediaStream(conn, mediaFormat, sessionID, isSource), sessionToken(_token), filename(_filename), receive(_receive), stationId(_stationId)
{
  faxCallInfo = NULL;
  SetDataSize(RTP_DataFrame::MaxMtuPayloadSize);
}

PBoolean OpalFaxMediaStream::Open()
{
  if (sessionToken.IsEmpty()) {
    PTRACE(1, "T38\tCannot open unknown media stream");
    return PFalse;
  }

  PWaitAndSignal m2(faxMapMutex);
  PWaitAndSignal m(infoMutex);

  if (faxCallInfo == NULL) {
    OpalFaxCallInfoMap_T::iterator r = faxCallInfoMap.find(sessionToken);
    if (r != faxCallInfoMap.end()) {
      faxCallInfo = r->second;
      ++faxCallInfo->refCount;
    } else {
      faxCallInfo = new OpalFaxCallInfo();
      if (!faxCallInfo->socket.Listen()) {
        PTRACE(1, "Fax\tCannot open listening socket for SpanDSP");
        return PFalse;
      }
      {
        PIPSocket::Address addr; WORD port;
        faxCallInfo->socket.GetLocalAddress(addr, port);
        PTRACE(2, "Fax\tLocal spandsp address set to " << addr << ":" << port);
      }
      faxCallInfo->socket.SetReadTimeout(1000);
      faxCallInfoMap.insert(OpalFaxCallInfoMap_T::value_type((const char *)sessionToken, faxCallInfo));
    }
  }

  // reset the output buffer
  writeBufferLen = 0;

  // start the spandsp process
  if (!faxCallInfo->spanDSP.IsOpen()) {

    // create the command line for spandsp_util
    PString cmdLine = GetSpanDSPCommandLine(*faxCallInfo);

#if _WIN32
    cmdLine.Replace("\\", "\\\\", true);
#endif
    
    PTRACE(1, "Fax\tExecuting '" << cmdLine << "'");

    // open connection to spandsp
    if (!faxCallInfo->spanDSP.Open(cmdLine, PPipeChannel::ReadWriteStd)) {
      PTRACE(1, "Fax\tCannot open SpanDSP");
      return PFalse;
    }

    if (!faxCallInfo->spanDSP.Execute()) {
      PTRACE(1, "Fax\tCannot execute SpanDSP");
      return PFalse;
    }
  }

  return OpalMediaStream::Open();
}

PBoolean OpalFaxMediaStream::Start()
{
  return OpalMediaStream::Start();
}

PBoolean OpalFaxMediaStream::ReadPacket(RTP_DataFrame & packet)
{
  // it is possible for ReadPacket to be called before the media stream has been opened, so deal with that case
  PWaitAndSignal m(infoMutex);
  if ((faxCallInfo == NULL) || !faxCallInfo->spanDSP.IsRunning()) {

    // return silence
    packet.SetPayloadSize(0);

  } else {

    packet.SetSize(2048);

    if (faxCallInfo->spanDSPPort > 0) {
      if (!faxCallInfo->socket.Read(packet.GetPointer()+RTP_DataFrame::MinHeaderSize, packet.GetSize()-RTP_DataFrame::MinHeaderSize)) {
        faxCallInfo->socket.Close();
        return PFalse;
      }
    } else{ 
      if (!faxCallInfo->socket.ReadFrom(packet.GetPointer()+RTP_DataFrame::MinHeaderSize, packet.GetSize()-RTP_DataFrame::MinHeaderSize, faxCallInfo->spanDSPAddr, faxCallInfo->spanDSPPort)) {
        faxCallInfo->socket.Close();
        return PFalse;
      }
    }

    PINDEX len = faxCallInfo->socket.GetLastReadCount();
    packet.SetPayloadType(RTP_DataFrame::MaxPayloadType);
    packet.SetPayloadSize(len);

#if WRITE_PCM_FILE
    static int file = _open("t38_audio_in.pcm", _O_BINARY | _O_CREAT | _O_TRUNC | _O_WRONLY, _S_IREAD | _S_IWRITE);
    if (file >= 0) {
      if (_write(file, packet.GetPointer()+RTP_DataFrame::MinHeaderSize, len) < len) {
        cerr << "cannot write output PCM data to file" << endl;
        file = -1;
      }
    }
#endif    
  }

  return PTrue;
}

PBoolean OpalFaxMediaStream::WritePacket(RTP_DataFrame & packet)
{
  PWaitAndSignal m(infoMutex);
  if ((faxCallInfo == NULL) || !faxCallInfo->spanDSP.IsRunning()) {
   
    // return silence
    packet.SetPayloadSize(0);

  } else {

    if (!faxCallInfo->spanDSP.IsRunning()) {
      PTRACE(1, "Fax\tspandsp ended");
      return PFalse;
    }

#if WRITE_PCM_FILE
    static int file = _open("t38_audio_out.pcm", _O_BINARY | _O_CREAT | _O_TRUNC | _O_WRONLY, _S_IREAD | _S_IWRITE);
    if (file >= 0) {
      PINDEX len = packet.GetPayloadSize();
      if (_write(file, packet.GetPointer()+RTP_DataFrame::MinHeaderSize, len) < len) {
        cerr << "cannot write output PCM data to file" << endl;
        file = -1;
      }
    }
#endif

    if (faxCallInfo->spanDSPPort > 0) {

      PINDEX size = packet.GetPayloadSize();
      BYTE * ptr = packet.GetPayloadPtr();

      // if there is more data than spandsp can accept, break it down
      while ((writeBufferLen + size) >= (PINDEX)sizeof(writeBuffer)) {
        PINDEX len;
        if (writeBufferLen == 0) {
          if (!faxCallInfo->socket.WriteTo(ptr, sizeof(writeBuffer), faxCallInfo->spanDSPAddr, faxCallInfo->spanDSPPort)) {
            PTRACE(1, "Fax\tSocket write error - " << faxCallInfo->socket.GetErrorText(PChannel::LastWriteError));
            return PFalse;
          }
          len = sizeof(writeBuffer);
        }
        else {
          len = sizeof(writeBuffer) - writeBufferLen;
          memcpy(writeBuffer + writeBufferLen, ptr, len);
          if (!faxCallInfo->socket.WriteTo(writeBuffer, sizeof(writeBuffer), faxCallInfo->spanDSPAddr, faxCallInfo->spanDSPPort)) {
            PTRACE(1, "Fax\tSocket write error - " << faxCallInfo->socket.GetErrorText(PChannel::LastWriteError));
            return PFalse;
          }
        }
        ptr += len;
        size -= len;
        writeBufferLen = 0;
      }

      // copy remaining data into buffer
      if (size > 0) {
        memcpy(writeBuffer + writeBufferLen, ptr, size);
        writeBufferLen += size;
      }

      if (writeBufferLen == sizeof(writeBuffer)) {
        if (!faxCallInfo->socket.WriteTo(writeBuffer, sizeof(writeBuffer), faxCallInfo->spanDSPAddr, faxCallInfo->spanDSPPort)) {
          PTRACE(1, "Fax\tSocket write error - " << faxCallInfo->socket.GetErrorText(PChannel::LastWriteError));
          return PFalse;
        }
        writeBufferLen = 0;
      }
    }
  }

  return PTrue;
}

PBoolean OpalFaxMediaStream::Close()
{
  PBoolean stat = OpalMediaStream::Close();

  PWaitAndSignal m2(faxMapMutex);

  {
    if (faxCallInfo == NULL || sessionToken.IsEmpty()) {
      PTRACE(1, "Fax\tCannot close unknown media stream");
      return stat;
    }

    // shutdown whatever is running
    faxCallInfo->socket.Close();
    faxCallInfo->spanDSP.Close();

    OpalFaxCallInfoMap_T::iterator r = faxCallInfoMap.find(sessionToken);
    if (r == faxCallInfoMap.end()) {
      PTRACE(1, "Fax\tError: media stream not found in T38 session list");
      PWaitAndSignal m(infoMutex);
      faxCallInfo = NULL;
      return stat;
    }

    if (r->second != faxCallInfo) {
      PTRACE(1, "Fax\tError: session list does not match local ptr");
      PWaitAndSignal m(infoMutex);
      faxCallInfo = NULL;
      return stat;
    }

    else if (faxCallInfo->refCount == 0) {
      PTRACE(1, "Fax\tError: media stream has incorrect reference count");
      PWaitAndSignal m(infoMutex);
      faxCallInfo = NULL;
      return stat;
    }

    if (--faxCallInfo->refCount > 0) {
      PWaitAndSignal m(infoMutex);
      faxCallInfo = NULL;
      PTRACE(1, "Fax\tClosed fax media stream");
      return stat;
    }
  }

  // remove info from map
  faxCallInfoMap.erase(sessionToken);

  // delete the object
  OpalFaxCallInfo * oldFaxCallInfo = faxCallInfo;
  {
    PWaitAndSignal m(infoMutex);
    faxCallInfo = NULL;
  }
  delete oldFaxCallInfo;

  return stat;
}

PBoolean OpalFaxMediaStream::IsSynchronous() const
{
  return PTrue;
}

PString OpalFaxMediaStream::GetSpanDSPCommandLine(OpalFaxCallInfo & info)
{
  PStringStream cmdline;

  PIPSocket::Address dummy; WORD port;
  info.socket.GetLocalAddress(dummy, port);

  cmdline << ((OpalFaxEndPoint &)connection.GetEndPoint()).GetSpanDSP() << " -m ";
  if (receive)
    cmdline << "fax_to_tiff";
  else
    cmdline << "tiff_to_fax";
  cmdline << "-V 0 -n '" << filename << "' -f 127.0.0.1:" << port;

  return cmdline;
}

/////////////////////////////////////////////////////////////////////////////

/**This class describes a media stream that transfers data to/from a T.38 session
  */
OpalT38MediaStream::OpalT38MediaStream(
      OpalConnection & conn,
      const OpalMediaFormat & mediaFormat, ///<  Media format for stream
      unsigned sessionID, 
      PBoolean isSource,                       ///<  Is a source stream
      const PString & token,               ///<  token used to match incoming/outgoing streams
      const PString & _filename,
      PBoolean _receive,
      const PString & _stationId
    )
  : OpalFaxMediaStream(conn, mediaFormat, sessionID, isSource, token, _filename, _receive, _stationId)
{
}

PString OpalT38MediaStream::GetSpanDSPCommandLine(OpalFaxCallInfo & info)
{
  PStringStream cmdline;

  PIPSocket::Address dummy; WORD port;
  info.socket.GetLocalAddress(dummy, port);

  cmdline << ((OpalFaxEndPoint &)connection.GetEndPoint()).GetSpanDSP() << " -V 0 -m ";
  if (receive)
    cmdline << "t38_to_tiff";
  else {
    cmdline << "tiff_to_t38";
    if (!stationId.IsEmpty())
      cmdline << " -s '" << stationId << "'";
  }
  cmdline << " -v -n '" << filename << "' -t 127.0.0.1:" << port;

  return cmdline;
}

PBoolean OpalT38MediaStream::ReadPacket(RTP_DataFrame & packet)
{
  // it is possible for ReadPacket to be called before the media stream has been opened, so deal with that case
  PWaitAndSignal m(infoMutex);
  if ((faxCallInfo == NULL) || !faxCallInfo->spanDSP.IsRunning()) {

    // return silence
    packet.SetPayloadSize(0);
    PThread::Sleep(20);

  } else {

    packet.SetSize(2048);

    bool stat;

    if (faxCallInfo->spanDSPPort > 0) 
      stat = faxCallInfo->socket.Read(packet.GetPointer(), packet.GetSize());
    else {
      stat = faxCallInfo->socket.ReadFrom(packet.GetPointer(), packet.GetSize(), faxCallInfo->spanDSPAddr, faxCallInfo->spanDSPPort);
      PTRACE(2, "Fax\tRemote spandsp address set to " << faxCallInfo->spanDSPAddr << ":" << faxCallInfo->spanDSPPort);
    }

    if (!stat) {
      if (faxCallInfo->socket.GetErrorCode(PChannel::LastReadError) == PChannel::Timeout) {
        packet.SetPayloadSize(0);
        return true;
      }
      return false;
    }

    PINDEX len = faxCallInfo->socket.GetLastReadCount();
    if (len < RTP_DataFrame::MinHeaderSize)
      return PFalse;

    packet.SetSize(len);
    packet.SetPayloadSize(len - RTP_DataFrame::MinHeaderSize);
  }

  return PTrue;
}

PBoolean OpalT38MediaStream::WritePacket(RTP_DataFrame & packet)
{
  PWaitAndSignal m(infoMutex);
  if ((packet.GetPayloadSize() == 1) && (packet.GetPayloadPtr()[0] == 0xff)) {
    // ignore T.38 timing frames
  } else if ((faxCallInfo == NULL) || !faxCallInfo->spanDSP.IsRunning() || faxCallInfo->spanDSPPort == 0) {
    // queue frames before we know where to send them
    queuedFrames.Append(new RTP_DataFrame(packet));
  } else {
    PTRACE(5, "Fax\tT.38 Write RTP packet size = " << packet.GetHeaderSize() + packet.GetPayloadSize() <<" to " << faxCallInfo->spanDSPAddr << ":" << faxCallInfo->spanDSPPort);
    if (queuedFrames.GetSize() > 0) {
      for (PINDEX i = 0; i < queuedFrames.GetSize(); ++i) {
        RTP_DataFrame & frame = queuedFrames[i];
        if (!faxCallInfo->socket.WriteTo(frame.GetPointer(), frame.GetHeaderSize() + frame.GetPayloadSize(), faxCallInfo->spanDSPAddr, faxCallInfo->spanDSPPort)) {
          PTRACE(2, "T38_UDP\tSocket write error - " << faxCallInfo->socket.GetErrorText(PChannel::LastWriteError));
          return PFalse;
        }
      }
      queuedFrames.RemoveAll();
    }
    if (!faxCallInfo->socket.WriteTo(packet.GetPointer(), packet.GetHeaderSize() + packet.GetPayloadSize(), faxCallInfo->spanDSPAddr, faxCallInfo->spanDSPPort)) {
      PTRACE(2, "T38_UDP\tSocket write error - " << faxCallInfo->socket.GetErrorText(PChannel::LastWriteError));
      return PFalse;
    }
  }

  return PTrue;
}

/////////////////////////////////////////////////////////////////////////////

OpalFaxEndPoint::OpalFaxEndPoint(OpalManager & mgr, const char * prefix)
  : OpalEndPoint(mgr, prefix, CanTerminateCall)
#ifdef _WIN32
  , m_spanDSP("./spandsp_util.exe")
#else
  , m_spanDSP("./spandsp_util")
#endif
  , m_defaultDirectory(".")
{
  PTRACE(3, "Fax\tCreated Fax endpoint");
}

OpalFaxEndPoint::~OpalFaxEndPoint()
{
  PTRACE(3, "Fax\tDeleted Fax endpoint.");
}

OpalFaxConnection * OpalFaxEndPoint::CreateConnection(OpalCall & call, const PString & filename, PBoolean receive, void * /*userData*/, OpalConnection::StringOptions * stringOptions)
{
  return new OpalFaxConnection(call, *this, filename, receive, MakeToken(), stringOptions);
}

OpalMediaFormatList OpalFaxEndPoint::GetMediaFormats() const
{
  OpalMediaFormatList formats;

  //formats += OpalPCM16;        

  return formats;
}

PString OpalFaxEndPoint::MakeToken()
{
  return psprintf("FaxConnection_%i", ++faxCallIndex);
}

void OpalFaxEndPoint::AcceptIncomingConnection(const PString & token)
{
  PSafePtr<OpalFaxConnection> connection = PSafePtrCast<OpalConnection, OpalFaxConnection>(GetConnectionWithLock(token, PSafeReadOnly));
  if (connection != NULL)
    connection->AcceptIncoming();
}

PBoolean OpalFaxEndPoint::MakeConnection(OpalCall & call,
                                const PString & remoteParty,
                                         void * userData,
                                 unsigned int /*options*/,
                OpalConnection::StringOptions * stringOptions)
{
  // First strip of the prefix if present
  PINDEX prefixLength = 0;
  if (remoteParty.Find(GetPrefixName()+":") == 0)
    prefixLength = GetPrefixName().GetLength()+1;

  PStringArray tokens = remoteParty.Mid(prefixLength).Tokenise(";", true);
  if (tokens.IsEmpty()) {
    PTRACE(2, "Fax\tNo filename specified!");
    return false;
  }

  bool receive = false;
  PString stationId = GetDefaultDisplayName();

  for (PINDEX i = 1; i < tokens.GetSize(); ++i) {
    if (tokens[i] *= "receive")
      receive = true;
    else if (tokens[i].Left(10) *= "stationid=")
      stationId = tokens[i].Mid(10);
  }

  PString filename = tokens[0];
  if (!receive && !PFile::Exists(filename)) {
    PTRACE(2, "Fax\tCannot find filename '" << filename << "'");
    return PFalse;
  }

  if (stringOptions == NULL)
    stringOptions = new OpalConnection::StringOptions;
  if ((*stringOptions)("stationid").IsEmpty())
    stringOptions->SetAt("stationid", stationId);

  PSafePtr<OpalFaxConnection> connection = PSafePtrCast<OpalConnection, OpalFaxConnection>(GetConnectionWithLock(MakeToken()));
  if (connection != NULL)
    return PFalse;

  connection = CreateConnection(call, filename, receive, userData, stringOptions);
  if (connection == NULL)
    return PFalse;

  connectionsActive.SetAt(connection->GetToken(), connection);

  return PTrue;
}

void OpalFaxEndPoint::OnPatchMediaStream(const OpalFaxConnection & /*connection*/, PBoolean /*isSource*/, OpalMediaPatch & /*patch*/)
{
}

/////////////////////////////////////////////////////////////////////////////

OpalFaxConnection::OpalFaxConnection(OpalCall & call, OpalFaxEndPoint & ep, const PString & _filename, PBoolean _receive, const PString & _token, OpalConnection::StringOptions * _stringOptions)
  : OpalConnection(call, ep, _token, 0, _stringOptions), endpoint(ep), filename(_filename), receive(_receive)
{
  PTRACE(3, "FAX\tCreated FAX connection with token '" << callToken << "'");
  phase = SetUpPhase;

  if (stringOptions != NULL) {
    forceFaxAudio = stringOptions->Contains("Force-Fax-Audio");
    stationId     = (*stringOptions)("stationid");
  }

  detectInBandDTMF = true;
}

OpalFaxConnection::~OpalFaxConnection()
{
  PTRACE(3, "FAX\tDeleted FAX connection.");
}

OpalMediaStream * OpalFaxConnection::CreateMediaStream(const OpalMediaFormat & mediaFormat, unsigned sessionID, PBoolean isSource)
{
  // if creating an audio session, use a NULL stream
  if (forceFaxAudio && (mediaFormat == OpalPCM16))
    return new OpalFaxMediaStream(*this, mediaFormat, sessionID, isSource, GetToken(), filename, receive, stationId);
  else
    return new OpalNullMediaStream(*this, mediaFormat, sessionID, isSource, true);
}

PBoolean OpalFaxConnection::SetUpConnection()
{
  // Check if we are A-Party in thsi call, so need to do things differently
  if (ownerCall.GetConnection(0) == this) {
    phase = SetUpPhase;

    {
      StringOptions * otherStringOptions = new StringOptions;
      otherStringOptions->SetAt("enableinbanddtmf", "true");
      otherStringOptions->SetAt("dtmfmult",         "4");
      bool stat = OnIncomingConnection(0, otherStringOptions);
      delete otherStringOptions;
      if (!stat) {
        Release(EndedByCallerAbort);
        return PFalse;
      }
    }

    PTRACE(2, "FAX\tOutgoing call routed to " << ownerCall.GetPartyB() << " for " << *this);
    if (!ownerCall.OnSetUp(*this)) {
      Release(EndedByNoAccept);
      return PFalse;
    }

    return PTrue;
  }

  PTRACE(3, "FAX\tSetUpConnection(" << remotePartyName << ')');
  phase = AlertingPhase;
  //endpoint.OnShowIncoming(*this);
  OnAlerting();

  SetPhase(ConnectedPhase);
  OnConnected();

  if (!mediaStreams.IsEmpty()) {
    phase = EstablishedPhase;
    OnEstablished();
  }

  return PTrue;
}


PBoolean OpalFaxConnection::SetAlerting(const PString & calleeName, PBoolean)
{
  PTRACE(3, "Fax\tSetAlerting(" << calleeName << ')');
  phase = AlertingPhase;
  remotePartyName = calleeName;
  //return endpoint.OnShowOutgoing(*this);
  return PTrue;
}


PBoolean OpalFaxConnection::SetConnected()
{
  PTRACE(3, "Fax\tSetConnected()");

  {
    PSafeLockReadWrite safeLock(*this);
    if (!safeLock.IsLocked())
      return PFalse;

    if (mediaStreams.IsEmpty())
      return PTrue;

    phase = EstablishedPhase;
  }

  OnEstablished();

  return PTrue;
}


OpalMediaFormatList OpalFaxConnection::GetMediaFormats() const
{
  OpalMediaFormatList formats;
  formats += OpalPCM16;       
  return formats;
}

void OpalFaxConnection::AdjustMediaFormats(OpalMediaFormatList & mediaFormats) const
{
  endpoint.AdjustMediaFormats(*this, mediaFormats);
  if (forceFaxAudio)
    mediaFormats.Remove(PStringArray(OpalT38));
}

void OpalFaxConnection::OnPatchMediaStream(PBoolean isSource, OpalMediaPatch & patch)
{
  endpoint.OnPatchMediaStream(*this, isSource, patch);
}


void OpalFaxConnection::AcceptIncoming()
{
  if (!LockReadOnly())
    return;

  if (phase != AlertingPhase) {
    UnlockReadOnly();
    return;
  }

  LockReadWrite();
  phase = ConnectedPhase;
  UnlockReadWrite();
  UnlockReadOnly();

  OnConnected();

  if (!LockReadOnly())
    return;

  if (mediaStreams.IsEmpty()) {
    UnlockReadOnly();
    return;
  }

  LockReadWrite();
  phase = EstablishedPhase;
  UnlockReadWrite();
  UnlockReadOnly();

  OnEstablished();
}

/////////////////////////////////////////////////////////////////////////////

OpalT38EndPoint::OpalT38EndPoint(OpalManager & manager, const char * prefix)
  : OpalFaxEndPoint(manager, prefix)
{
}

OpalMediaFormatList OpalT38EndPoint::GetMediaFormats() const
{
  OpalMediaFormatList formats;

  formats += OpalPCM16;        // need this so we get lead-in to T.38 sessions
  formats += OpalT38;        

  return formats;
}

OpalFaxConnection * OpalT38EndPoint::CreateConnection(OpalCall & call, const PString & filename, PBoolean receive, void * /*userData*/, OpalConnection::StringOptions * stringOptions)
{
  return new OpalT38Connection(call, *this, filename, receive, MakeToken(), stringOptions);
}

PString OpalT38EndPoint::MakeToken()
{
  return psprintf("T38Connection_%i", ++faxCallIndex);
}

/////////////////////////////////////////////////////////////////////////////

OpalT38Connection::OpalT38Connection(OpalCall & call, OpalT38EndPoint & ep, const PString & _filename, PBoolean _receive, const PString & _token, OpalConnection::StringOptions * stringOptions)
  : OpalFaxConnection(call, ep, _filename, _receive, _token, stringOptions), t38WaitMode(T38Mode_Auto)
{
  PTRACE(3, "FAX\tCreated T.38 connection with token '" << callToken << "'");
  forceFaxAudio         = false;
  currentMode = newMode = false;
  modeChangeTriggered   = false;
  faxStartup            = true;

  faxTimer.SetNotifier(PCREATE_NOTIFIER(OnFaxChangeTimeout));
}

OpalT38Connection::~OpalT38Connection()
{
}

OpalMediaStream * OpalT38Connection::CreateMediaStream(const OpalMediaFormat & mediaFormat, unsigned sessionID, PBoolean isSource)
{
  // if creating an audio session, use a NULL stream
  if (mediaFormat.GetMediaType() == OpalMediaType::Audio()) {
    if (isSource)
      InFaxMode(false);
    return new OpalNullMediaStream(*this, mediaFormat, sessionID, isSource, true);
  }

  // if creating a T.38 stream, see what type it is
  else if (mediaFormat.GetMediaType() == OpalMediaType::Fax()) {
    if (isSource)
      InFaxMode(true);
    return new OpalT38MediaStream(*this, mediaFormat, sessionID, isSource, GetToken(), filename, receive, stationId);
  }

  return NULL;
}


OpalMediaFormatList OpalT38Connection::GetMediaFormats() const
{
  OpalMediaFormatList formats;

  formats += OpalPCM16;        
  formats += OpalT38;        

  return formats;
}


void OpalT38Connection::OnPatchMediaStream(PBoolean isSource, OpalMediaPatch & patch)
{
  OpalConnection::OnPatchMediaStream(isSource, patch);
}

PBoolean OpalT38Connection::SendUserInputTone(char tone, unsigned /*duration*/)
{
  if (((t38WaitMode & T38Mode_NSECED) != 0) && (tolower(tone) == 'y'))
    RequestFaxMode(true);

  return true;
}

void OpalT38Connection::OnFaxChangeTimeout(PTimer &, INT)
{
  RequestFaxMode(true);
}

struct ModeChangeRequestStruct {
  OpalManager * manager;
  PString callToken;
  PString connectionToken;
  bool newMode;
};

static void ReinviteFunction(ModeChangeRequestStruct request)
{
  PSafePtr<OpalCall> call = request.manager->FindCallWithLock(request.callToken);
  unsigned otherIndex = (call->GetConnection(0)->GetToken() == request.connectionToken) ? 1 : 0;

#if PTRACING
  const char * modeStr = request.newMode ? "fax" : "audio";
#endif

  PSafePtr<OpalConnection> otherParty = call->GetConnection(otherIndex, PSafeReadWrite);
  if (otherParty == NULL) {
    PTRACE(1, "T38\tCannot get other party for " << modeStr << " trigger");
  }
  else 
  {
    // for now, assume fax is always session 1
    OpalMediaFormatList formats;
    OpalMediaType newType;

    if (request.newMode) {
      formats += OpalT38;
      newType = OpalMediaType::Fax();
    } else {
      formats += OpalG711uLaw;
      newType = OpalMediaType::Audio();
    }

    if (!call->OpenSourceMediaStreams(*otherParty, newType, 1, formats)) {
      PTRACE(1, "T38\tMode change request to " << modeStr << " failed");
    } else {
      PTRACE(1, "T38\tMode change request to " << modeStr << " succeeded");
    }
  }
}

void OpalT38Connection::RequestFaxMode(bool toFax)
{
#if PTRACING
  const char * modeStr = toFax ? "fax" : "audio";
#endif

  PWaitAndSignal m(modeMutex);

  if (!modeChangeTriggered) {
    if (toFax == currentMode) {
      PTRACE(1, "T38\tIgnoring request for mode " << modeStr);
      return;
    }
  }
  else
  {
    if (toFax == newMode) {
      PTRACE(1, "T38\tIgnoring duplicate request for mode " << modeStr);
    } else {
      PTRACE(1, "T38\tCan't change in-progress mode request for mode " << modeStr);
    }
    return;
  }

  // definitely changing mode
  faxTimer.Stop();
  modeChangeTriggered = true;
  newMode = toFax;
  PTRACE(1, "T38\tRequesting mode change to " << modeStr);

  OpalCall & call = GetCall();
  ModeChangeRequestStruct request;
  request.manager         = &call.GetManager();
  request.callToken       = call.GetToken();
  request.connectionToken = GetToken();
  request.newMode         = newMode;

  new PThread1Arg<ModeChangeRequestStruct>(request, &ReinviteFunction, true);
}

void OpalT38Connection::InFaxMode(bool toFax)
{
#if PTRACING
  const char * modeStr = toFax ? "fax" : "audio";
#endif

  PWaitAndSignal m(modeMutex);

  modeChangeTriggered = false;
  faxTimer.Stop();

  if (!modeChangeTriggered) {
    if (toFax == currentMode) {
      if (!faxStartup) {
        PTRACE(1, "T38\tUntriggered mode to same mode " << modeStr);
      }
      else {
        if (faxStartup && !currentMode && (t38WaitMode & T38Mode_Timeout) != 0) {
          faxTimer = receive ? 2000 : 8000;
          PTRACE(1, "T38\tStarting timer for mode change");
        }
      }
    }
    else {
      PTRACE(1, "T38\tMode changed to different mode " << modeStr);
    }
  }

  else {
    if (toFax == newMode) {
      PTRACE(1, "T38\tMode changed to correct mode " << modeStr);
    }
    else {
      PTRACE(1, "T38\tMode changed to wrong mode " << modeStr);
    }
  }

  faxStartup = false;
  currentMode = toFax;
}

#endif // OPAL_FAX

