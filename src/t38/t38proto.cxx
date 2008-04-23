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

#include <opal/buildopts.h>

#if OPAL_T38FAX

#include <ptlib/pipechan.h>

#include <t38/t38proto.h>
#include <codec/rfc2833.h>

#include <opal/mediastrm.h>
#include <opal/mediatype.h>
#include <opal/patch.h>

#include <h323/transaddr.h>
#include <asn/t38.h>

//#define USE_SEQ

#define new PNEW

namespace PWLibStupidLinkerHacks {
  int t38Loader;
};

#define SPANDSP_AUDIO_SIZE    320

static PAtomicInteger faxCallIndex;

OPAL_INSTANTIATE_MEDIATYPE(fax, OpalFaxMediaType);

/////////////////////////////////////////////////////////////////////////////

OpalFaxMediaType::OpalFaxMediaType()
  : OpalMediaTypeDefinition("fax", "image", 3)
{ }

PString OpalFaxMediaType::GetRTPEncoding() const
{ return "udptl"; }

RTP_UDP * OpalFaxMediaType::CreateRTPSession(OpalRTPConnection &,
#if OPAL_RTP_AGGREGATE
                                             PHandleAggregator * agg,
#endif
                                             unsigned sessionID, bool remoteIsNAT)
{
  return new RTP_UDP(GetRTPEncoding(),
#if OPAL_RTP_AGGREGATE
                     agg,
#endif
                     sessionID, remoteIsNAT);
}

/////////////////////////////////////////////////////////////////////////////

#if OPAL_AUDIO

const OpalFaxAudioFormat & GetOpalPCM16Fax() 
{
  static const OpalFaxAudioFormat opalPCM16Fax(OPAL_PCM16_FAX, RTP_DataFrame::MaxPayloadType, "", 16, 8,  240, 30, 256,  8000);
  return opalPCM16Fax;
}


OpalFaxAudioFormat::OpalFaxAudioFormat(const char * fullName,
                                 RTP_DataFrame::PayloadTypes rtpPayloadType,
                                 const char * encodingName, ///<  RTP encoding name
                                 PINDEX   frameSize,
                                 unsigned frameTime,
                                 unsigned rxFrames,
                                 unsigned txFrames,
                                 unsigned maxFrames,
				                         unsigned clockRate,
                                   time_t timeStamp)
  : OpalMediaFormat(fullName,
                    "audio",
                    rtpPayloadType,
                    encodingName,
                    PTrue,
                    8*frameSize*AudioClockRate/frameTime,  // bits per second = 8*frameSize * framesPerSecond
                    frameSize,
                    frameTime,
                    clockRate,
                    timeStamp)
{
  AddOption(new OpalMediaOptionUnsigned(OpalAudioFormat::RxFramesPerPacketOption(), false, OpalMediaOption::MinMerge, rxFrames, 1, maxFrames));
  AddOption(new OpalMediaOptionUnsigned(OpalAudioFormat::TxFramesPerPacketOption(), false, OpalMediaOption::MinMerge, txFrames, 1, maxFrames));
}

#endif

/////////////////////////////////////////////////////////////////////////////

class T38PseudoRTP_Handler : public RTP_FormatHandler
{
  public:
    void OnStart(RTP_Session & _rtpUDP)
    {  
      RTP_FormatHandler::OnStart(_rtpUDP);
      corrigendumASN        = PTrue;
      consecutiveBadPackets = 0;

      lastIFP.SetSize(0);
      rtpUDP->SetReportTimeInterval(20);
    }

    PBoolean WriteData(RTP_DataFrame & frame);
    PBoolean ReadData(RTP_DataFrame & frame, PBoolean loop);

    RTP_Session::SendReceiveStatus OnSendData(RTP_DataFrame & frame);
    RTP_Session::SendReceiveStatus OnSendControl(RTP_ControlFrame & /*frame*/, PINDEX & /*len*/);
    RTP_Session::SendReceiveStatus ReadDataPDU(RTP_DataFrame & frame);
    RTP_Session::SendReceiveStatus OnReceiveData(RTP_DataFrame & frame);
    int WaitForPDU(PUDPSocket & dataSocket, PUDPSocket & controlSocket, const PTimeInterval &);

  protected:
    PBoolean corrigendumASN;
    int consecutiveBadPackets;
    PBYTEArray lastIFP;
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

  PPER_Stream rawData;
  udptl.Encode(rawData);
  rawData.CompleteEncoding();

#if PTRACING
  if (PTrace::CanTrace(4)) {
    PTRACE(4, "RTP_T38\tSending PDU:\n  "
           << setprecision(2) << udptl << "\n "
           << setprecision(2) << rawData);
  }
#endif

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

PTRACE(1, "T38_RTP\tWriting RTP T.38 seq " << udptl.m_seq_number << " of size " << plLen << " as T.38 UDPTL size " << frame.GetSize());

  return RTP_Session::e_ProcessPacket;
}

PBoolean T38PseudoRTP_Handler::WriteData(RTP_DataFrame & frame)
{
  if (rtpUDP->shutdownWrite) {
    PTRACE(3, "RTP_T38\tSession " << rtpUDP->GetSessionID() << ", Write shutdown.");
    rtpUDP->shutdownWrite = PFalse;
    return PFalse;
  }

  // Trying to send a PDU before we are set up!
  if (!rtpUDP->GetRemoteAddress().IsValid() || rtpUDP->GetRemoteDataPort() == 0)
    return PTrue;

  switch (OnSendData(frame)) {
    case RTP_Session::e_ProcessPacket :
      break;
    case RTP_Session::e_IgnorePacket :
      return PTrue;
    case RTP_Session::e_AbortTransport :
      return PFalse;
  }

  PUDPSocket & dataSocket = rtpUDP->GetDataSocket();

  while (!dataSocket.WriteTo(frame.GetPointer(),
                                  frame.GetSize(),
                                  rtpUDP->GetRemoteAddress(), rtpUDP->GetRemoteDataPort())) {
    switch (dataSocket.GetErrorNumber()) {
      case ECONNRESET :
      case ECONNREFUSED :
        PTRACE(2, "RTP_T38\tSession " << rtpUDP->GetSessionID() << ", data port on remote not ready.");
        break;

      default:
        PTRACE(1, "RTP_T38\tSession " << rtpUDP->GetSessionID()
               << ", Write error on data port ("
               << dataSocket.GetErrorNumber(PChannel::LastWriteError) << "): "
               << dataSocket.GetErrorText(PChannel::LastWriteError));
        return PFalse;
    }
  }

  return PTrue;
}


RTP_Session::SendReceiveStatus T38PseudoRTP_Handler::OnSendControl(RTP_ControlFrame & /*frame*/, PINDEX & /*len*/)
{
  return RTP_Session::e_IgnorePacket; // Non fatal error, just ignore
}

RTP_Session::SendReceiveStatus T38PseudoRTP_Handler::ReadDataPDU(RTP_DataFrame & frame)
{
  frame.SetPayloadSize(500);
  RTP_Session::SendReceiveStatus status = rtpUDP->ReadDataOrControlPDU(rtpUDP->GetDataSocket(), frame, PTrue);
  if (status != RTP_Session::e_ProcessPacket)
    return status;

  // Check received PDU is big enough
  PINDEX pduSize = rtpUDP->GetDataSocket().GetLastReadCount();
  frame.SetSize(pduSize);

  return OnReceiveData(frame);
}

RTP_Session::SendReceiveStatus T38PseudoRTP_Handler::OnReceiveData(RTP_DataFrame & frame)
{
  PTRACE(4, "T38_RTP\tReading raw T.38 of size " << frame.GetSize());

  if ((frame.GetPayloadSize() == 1) && (frame.GetPayloadPtr()[0] == 0xff)) {
    // allow fake timing frames to pass through
  }
  else {
    PPER_Stream rawData(frame.GetPointer(), frame.GetSize());

    // Decode the PDU
    T38_UDPTLPacket udptl;
    if (udptl.Decode(rawData))
      consecutiveBadPackets = 0;
    else {
      consecutiveBadPackets++;
      PTRACE(2, "RTP_T38\tRaw data decode failure:\n  "
             << setprecision(2) << rawData << "\n  UDPTL = "
             << setprecision(2) << udptl);
      if (consecutiveBadPackets > 100) {
        PTRACE(1, "RTP_T38\tRaw data decode failed multiple times, aborting!");
        return RTP_Session::e_AbortTransport;
      }
      return RTP_Session::e_IgnorePacket;
    }

    PASN_OctetString & ifp = udptl.m_primary_ifp_packet;
    frame.SetPayloadSize(ifp.GetDataLength());

    memcpy(frame.GetPayloadPtr(), ifp.GetPointer(), ifp.GetDataLength());
    frame.SetSequenceNumber((WORD)(udptl.m_seq_number & 0xffff));
    PTRACE(4, "T38_RTP\tT38 decode :\n  " << setprecision(2) << udptl);
  }

  frame[0] = 0x80;
  frame.SetPayloadType((RTP_DataFrame::PayloadTypes)96);
  frame.SetSyncSource(rtpUDP->GetSyncSourceIn());

  PTRACE(3, "T38_RTP\tReading RTP payload size " << frame.GetPayloadSize());

  return RTP_FormatHandler::OnReceiveData(frame);
}

PBoolean T38PseudoRTP_Handler::ReadData(RTP_DataFrame & frame, PBoolean loop)
{
  PUDPSocket * dataSocket    = &rtpUDP->GetDataSocket();
  PUDPSocket * controlSocket = &rtpUDP->GetControlSocket();

  do {
    int selectStatus = WaitForPDU(*dataSocket, *controlSocket, rtpUDP->GetReportTimer());

    if (rtpUDP->shutdownRead) {
      PTRACE(3, "T38_RTP\tSession " << rtpUDP->GetSessionID() << ", Read shutdown.");
      rtpUDP->shutdownRead = PFalse;
      return PFalse;
    }

    switch (selectStatus) {
      case -2 :
        if (rtpUDP->ReadControlPDU() == RTP_Session::e_AbortTransport)
          return PFalse;
        break;

      case -3 :
        if (rtpUDP->ReadControlPDU() == RTP_Session::e_AbortTransport)
          return PFalse;
        // Then do -1 case

      case -1 :
        switch (rtpUDP->ReadDataPDU(frame)) {
          case RTP_Session::e_ProcessPacket :
            if (!rtpUDP->shutdownRead)
              return PTrue;
          case RTP_Session::e_IgnorePacket :
            break;
          case RTP_Session::e_AbortTransport :
            return PFalse;
        }
        break;

      case 0 :
        // for timeouts, send a "fake" payload of one byte of 0xff to keep the T.38 engine emitting PCM
        frame.SetPayloadSize(1);
        frame.GetPayloadPtr()[0] = 0xff;
        return PTrue;

      case PSocket::Interrupted:
        PTRACE(3, "T38_RTP\tSession " << rtpUDP->GetSessionID() << ", Interrupted.");
        return PFalse;

      default :
        PTRACE(1, "T38_RTP\tSession " << rtpUDP->GetSessionID() << ", Select error: "
                << PChannel::GetErrorText((PChannel::Errors)selectStatus));
        return PFalse;
    }
  } while (loop);

  frame.SetSize(0);
  return PTrue;
}

int T38PseudoRTP_Handler::WaitForPDU(PUDPSocket & dataSocket, PUDPSocket & controlSocket, const PTimeInterval &)
{
  // wait for no longer than 20ms so audio gets correctly processed
  return PSocket::Select(dataSocket, controlSocket, 20);
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
  cmdline << " -n '" << filename << "' -f 127.0.0.1:" << port;

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
  cmdline << " -n '" << filename << "' -t 127.0.0.1:" << port;

  return cmdline;
}

PBoolean OpalT38MediaStream::ReadPacket(RTP_DataFrame & packet)
{
  // it is possible for ReadPacket to be called before the media stream has been opened, so deal with that case
  PWaitAndSignal m(infoMutex);
  if ((faxCallInfo == NULL) || !faxCallInfo->spanDSP.IsRunning()) {

    // return silence
    packet.SetPayloadSize(0);

  } else {

    packet.SetSize(2048);

    bool stat;

    if (faxCallInfo->spanDSPPort > 0) 
      stat = faxCallInfo->socket.Read(packet.GetPointer(), packet.GetSize());
    else
      stat = faxCallInfo->socket.ReadFrom(packet.GetPointer(), packet.GetSize(), faxCallInfo->spanDSPAddr, faxCallInfo->spanDSPPort);

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
  if ((faxCallInfo == NULL) || !faxCallInfo->spanDSP.IsRunning() || faxCallInfo->spanDSPPort == 0) {
   
    // return silence
    packet.SetPayloadSize(0);

  } else {

    PTRACE(5, "Fax\tT.38 Write RTP packet size = " << packet.GetHeaderSize() + packet.GetPayloadSize() <<" to " << faxCallInfo->spanDSPAddr << ":" << faxCallInfo->spanDSPPort);
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
  if (sessionID == OpalMediaFormat::DefaultAudioSessionID) {
    if (forceFaxAudio && (mediaFormat == OpalPCM16))
      return new OpalFaxMediaStream(*this, mediaFormat, sessionID, isSource, GetToken(), filename, receive, stationId);
    else
      return new OpalNullMediaStream(*this, mediaFormat, sessionID, isSource);
  }

  // if creating a data stream, see what type it is
  else if (!forceFaxAudio && (sessionID == OpalMediaFormat::DefaultDataSessionID)) {
    if (mediaFormat == OpalPCM16Fax)
      return new OpalFaxMediaStream(*this, mediaFormat, sessionID, isSource, GetToken(), filename, receive, stationId);
  }

  // else call ancestor
  return NULL;
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

  {
    PSafePtr<OpalConnection> otherConn = ownerCall.GetOtherPartyConnection(*this);
    if (otherConn == NULL)
      return PFalse;

    remotePartyName    = otherConn->GetRemotePartyName();
    remotePartyAddress = otherConn->GetRemotePartyAddress();
    remoteProductInfo  = otherConn->GetRemoteProductInfo();
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

  if (forceFaxAudio)
    formats += OpalPCM16;        
  else
    formats += OpalPCM16Fax;        

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
  forceFaxAudio = false;
  inT38Mode     = false;

  faxTimer.SetNotifier(PCREATE_NOTIFIER(OnFaxChangeTimeout));
}

OpalT38Connection::~OpalT38Connection()
{
}

OpalMediaStream * OpalT38Connection::CreateMediaStream(const OpalMediaFormat & mediaFormat, unsigned sessionID, PBoolean isSource)
{

  // if creating an audio session, use a NULL stream
  if (mediaFormat.GetMediaType() == OpalMediaType::Audio()) {
    if (isSource && !inT38Mode && ((t38WaitMode & T38Mode_Timeout) != 0))
      faxTimer = 5000;
    return new OpalSinkMediaStream(*this, mediaFormat, sessionID, isSource);
  }

  // if creating a T.38 stream, see what type it is
  else if (mediaFormat.GetMediaType() == OpalMediaType::Fax()) 
    return new OpalT38MediaStream(*this, mediaFormat, sessionID, isSource, GetToken(), filename, receive, stationId);

  return NULL;
}

void OpalT38Connection::AdjustMediaFormats(OpalMediaFormatList & mediaFormats) const
{
  endpoint.AdjustMediaFormats(*this, mediaFormats);
  mediaFormats.Remove(PStringArray(OpalPCM16Fax));
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
    SwitchToT38();

  return true;
}

void OpalT38Connection::OnFaxChangeTimeout(PTimer &, INT)
{
  SwitchToT38();
}

static void ReinviteFunction(OpalManager & manager, const PString & callToken, const PString & connectionToken)
{
  PSafePtr<OpalCall> call = manager.FindCallWithLock(callToken);
  unsigned otherIndex = (call->GetConnection(0)->GetToken() == connectionToken) ? 1 : 0;

  PSafePtr<OpalConnection> otherParty = call->GetConnection(otherIndex, PSafeReadWrite);
  if (otherParty == NULL) {
    PTRACE(1, "T38\tCannot get other party for fax trigger");
  }
  else 
  {
    // for now, assume fax is always session 1
    OpalMediaFormatList formats;
    formats += OpalT38;

    if (!call->OpenSourceMediaStreams(*otherParty, OpalMediaType::Fax(), 1, formats)) {
      PTRACE(1, "T38\tReInvite trigger failed");
    }
    else
    {
      PTRACE(3, "T38\tTriggered ReInvite into fax mode");
    }
  }
}


void OpalT38Connection::SwitchToT38()
{
  if (!inT38Mode) {
    PTRACE(1, "T38\tTriggering ReInvite into fax mode");
    OpalCall & call = GetCall();
    inT38Mode = true;
    faxTimer.Stop();
    new PThread3Arg<OpalManager &, const PString &, const PString &>(call.GetManager(), call.GetToken(), GetToken(), &ReinviteFunction, true);
  }
}

#endif // OPAL_T38FAX

