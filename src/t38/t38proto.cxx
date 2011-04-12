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

#include <t38/t38proto.h>


/////////////////////////////////////////////////////////////////////////////

#if OPAL_FAX

#include <asn/t38.h>

#define new PNEW

namespace PWLibStupidLinkerHacks {
  int t38Loader;
};

#define SPANDSP_AUDIO_SIZE    320

static unsigned MaxConsecutiveBadPackets = 1000;
static PAtomicInteger faxCallIndex;

/////////////////////////////////////////////////////////////////////////////

class T38PseudoRTP_Handler : public RTP_Encoding
{
  public:
    void OnStart(RTP_Session & _rtpUDP)
    {  
      RTP_Encoding::OnStart(_rtpUDP);
      rtpUDP->SetJitterBufferSize(0, 0);
      consecutiveBadPackets = 0;
      oneGoodPacket         = false;

      lastIFP.SetSize(0);
      rtpUDP->SetNextSentSequenceNumber(0);
    }


    bool WriteDataPDU(RTP_DataFrame & frame)
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

      PTRACE(4, "T38_RTP\tSending UDPTL of size " << rawData.GetSize());

      return rtpUDP->WriteDataOrControlPDU(rawData.GetPointer(), rawData.GetSize(), true);
    }


    RTP_Session::SendReceiveStatus OnSendControl(RTP_ControlFrame & /*frame*/, PINDEX & /*len*/)
    {
      return RTP_Session::e_IgnorePacket; // Non fatal error, just ignore
    }


    int WaitForPDU(PUDPSocket & dataSocket, PUDPSocket & controlSocket, const PTimeInterval &)
    {
      // wait for no longer than 20ms so audio gets correctly processed
      return PSocket::Select(dataSocket, controlSocket, 20);
    }


    RTP_Session::SendReceiveStatus OnReadTimeout(RTP_DataFrame & frame)
    {
      // for timeouts, send a "fake" payload of one byte of 0xff to keep the T.38 engine emitting PCM
      frame.SetPayloadSize(1);
      frame.GetPayloadPtr()[0] = 0xff;
      return RTP_Session::e_ProcessPacket;
    }


    RTP_Session::SendReceiveStatus ReadDataPDU(RTP_DataFrame & frame)
    {
      BYTE thisUDPTL[500];
      RTP_Session::SendReceiveStatus status = rtpUDP->ReadDataOrControlPDU(thisUDPTL, sizeof(thisUDPTL), true);
      if (status != RTP_Session::e_ProcessPacket)
        return status;

      PINDEX pduSize = rtpUDP->GetDataSocket().GetLastReadCount();
      
      PTRACE(4, "T38_RTP\tRead UDPTL of size " << pduSize);

      if ((pduSize == 1) && (thisUDPTL[0] == 0xff)) {
        // ignore T.38 timing frames 
        frame.SetPayloadSize(0);
      }
      else {
        PPER_Stream rawData(thisUDPTL, pduSize);

        // Decode the PDU
        T38_UDPTLPacket udptl;
        if (!udptl.Decode(rawData)) {
    #if PTRACING
          if (oneGoodPacket)
            if (PTrace::CanTrace(5))
              PTRACE(5, "RTP_T38\tRaw data decode failure:\n  "
                     << setprecision(2) << rawData << "\n  UDPTL = "
                     << setprecision(2) << udptl);
            else
              PTRACE(2, "RTP_T38\tRaw data decode failure:\n  "
                     << setprecision(2) << rawData);
          else
            PTRACE(2, "RTP_T38\tRaw data decode failure: " << rawData.GetSize() << " bytes.");
    #endif

          consecutiveBadPackets++;
          if (consecutiveBadPackets < MaxConsecutiveBadPackets)
            return RTP_Session::e_IgnorePacket;

          PTRACE(1, "RTP_T38\tRaw data decode failed " << MaxConsecutiveBadPackets << " times,"
                    " remote probably not switched from audio, aborting!");
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

      return RTP_Session::e_ProcessPacket;
    }


  protected:
    int consecutiveBadPackets;
    bool oneGoodPacket;
    PBYTEArray lastIFP;
};


static PFactory<RTP_Encoding>::Worker<T38PseudoRTP_Handler> t38PseudoRTPHandler("udptl");


/////////////////////////////////////////////////////////////////////////////

static PMutex faxMapMutex;
typedef std::map<std::string, OpalFaxCallInfo *> OpalFaxCallInfoMap_T;

static OpalFaxCallInfoMap_T faxCallInfoMap;

OpalFaxCallInfo::OpalFaxCallInfo()
  : stdoutThread(NULL)
  , refCount(1)
  , spanDSPPort(0)
{
}

/////////////////////////////////////////////////////////////////////////////

OpalFaxMediaStream::OpalFaxMediaStream(OpalFaxConnection & conn, 
                                const OpalMediaFormat & mediaFormat, 
                                               unsigned sessionID, 
                                                   bool isSource, 
                                       const PString & token, 
                                       const PString & filename, 
                                                  bool receive,
                                       const PString & stationId)
  : OpalMediaStream(conn, mediaFormat, sessionID, isSource)
  , m_connection(conn)
  , sessionToken(token)
  , m_faxCallInfo(NULL)
  , m_filename(filename)
  , m_receive(receive)
  , m_stationId(stationId)
{
  SetDataSize(RTP_DataFrame::MaxMtuPayloadSize, 1);
}

PBoolean OpalFaxMediaStream::Open()
{
  if (sessionToken.IsEmpty()) {
    PTRACE(1, "Fax\tCannot open unknown media stream");
    return false;
  }

  PWaitAndSignal m(infoMutex);

  if (m_faxCallInfo == NULL) {
    PWaitAndSignal m2(faxMapMutex);
    OpalFaxCallInfoMap_T::iterator r = faxCallInfoMap.find(sessionToken);
    if (r != faxCallInfoMap.end()) {
      m_faxCallInfo = r->second;
      ++m_faxCallInfo->refCount;
    } else {
      m_faxCallInfo = new OpalFaxCallInfo();
      if (!m_faxCallInfo->socket.Listen()) {
        PTRACE(1, "Fax\tCannot open listening socket for SpanDSP");
        return false;
      }
      {
        PIPSocket::Address addr; WORD port;
        m_faxCallInfo->socket.GetLocalAddress(addr, port);
        PTRACE(2, "Fax\tLocal spandsp address set to " << addr << ":" << port);
      }
      m_faxCallInfo->socket.SetReadTimeout(1000);
      faxCallInfoMap.insert(OpalFaxCallInfoMap_T::value_type((const char *)sessionToken, m_faxCallInfo));
    }
  }

  // reset the output buffer
  writeBufferLen = 0;

  // start the spandsp process
  if (!m_faxCallInfo->spanDSP.IsOpen()) {

    // create the command line for spandsp_util
    PString cmdLine = GetSpanDSPCommandLine(*m_faxCallInfo);

#if _WIN32
    cmdLine.Replace("\\", "\\\\", true);
#endif

    PTRACE(1, "Fax\tExecuting '" << cmdLine << "'");

    // open connection to spandsp
    if (!m_faxCallInfo->spanDSP.Open(cmdLine, PPipeChannel::ReadOnly, false, false)) {
      PTRACE(1, "Fax\tCannot open SpanDSP: " << m_faxCallInfo->spanDSP.GetErrorText());
      return false;
    }

    if (!m_faxCallInfo->spanDSP.Execute()) {
      PTRACE(1, "Fax\tCannot execute SpanDSP: return code=" << m_faxCallInfo->spanDSP.GetReturnCode());
      return false;
    }
  }

  if (IsSink() && m_faxCallInfo->stdoutThread == NULL)
    m_faxCallInfo->stdoutThread = PThread::Create(PCREATE_NOTIFIER(ReadStdOut), "SpanDSP Output");

  return OpalMediaStream::Open();
}


PBoolean OpalFaxMediaStream::ReadPacket(RTP_DataFrame & packet)
{
  // it is possible for ReadPacket to be called before the media stream has been opened, so deal with that case
  PWaitAndSignal m(infoMutex);
  if ((m_faxCallInfo == NULL) || !m_faxCallInfo->spanDSP.IsRunning()) {

    // return silence
    packet.SetPayloadSize(0);

  } else {

    packet.SetSize(2048);

    if (m_faxCallInfo->spanDSPPort > 0) {
      if (!m_faxCallInfo->socket.Read(packet.GetPointer()+RTP_DataFrame::MinHeaderSize, packet.GetSize()-RTP_DataFrame::MinHeaderSize)) {
        m_faxCallInfo->socket.Close();
        return false;
      }
    } else{ 
      if (!m_faxCallInfo->socket.ReadFrom(packet.GetPointer()+RTP_DataFrame::MinHeaderSize, packet.GetSize()-RTP_DataFrame::MinHeaderSize, m_faxCallInfo->spanDSPAddr, m_faxCallInfo->spanDSPPort)) {
        m_faxCallInfo->socket.Close();
        return false;
      }
    }

    PINDEX len = m_faxCallInfo->socket.GetLastReadCount();
    packet.SetPayloadType(RTP_DataFrame::MaxPayloadType);
    packet.SetPayloadSize(len);
    packet.SetTimestamp(timestamp);
    timestamp += len/2;

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

  return true;
}

PBoolean OpalFaxMediaStream::WritePacket(RTP_DataFrame & packet)
{
  PWaitAndSignal m(infoMutex);
  if ((m_faxCallInfo == NULL) || !m_faxCallInfo->spanDSP.IsRunning()) {
   
    // return silence
    packet.SetPayloadSize(0);

  } else {

    if (!m_faxCallInfo->spanDSP.IsRunning()) {
      PTRACE(1, "Fax\tspandsp ended");
      return false;
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

    if (m_faxCallInfo->spanDSPPort > 0) {

      PINDEX size = packet.GetPayloadSize();
      BYTE * ptr = packet.GetPayloadPtr();

      // if there is more data than spandsp can accept, break it down
      while ((writeBufferLen + size) >= (PINDEX)sizeof(writeBuffer)) {
        PINDEX len;
        if (writeBufferLen == 0) {
          if (!m_faxCallInfo->socket.WriteTo(ptr, sizeof(writeBuffer), m_faxCallInfo->spanDSPAddr, m_faxCallInfo->spanDSPPort)) {
            PTRACE(1, "Fax\tSocket write error - " << m_faxCallInfo->socket.GetErrorText(PChannel::LastWriteError));
            return false;
          }
          len = sizeof(writeBuffer);
        }
        else {
          len = sizeof(writeBuffer) - writeBufferLen;
          memcpy(writeBuffer + writeBufferLen, ptr, len);
          if (!m_faxCallInfo->socket.WriteTo(writeBuffer, sizeof(writeBuffer), m_faxCallInfo->spanDSPAddr, m_faxCallInfo->spanDSPPort)) {
            PTRACE(1, "Fax\tSocket write error - " << m_faxCallInfo->socket.GetErrorText(PChannel::LastWriteError));
            return false;
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
        if (!m_faxCallInfo->socket.WriteTo(writeBuffer, sizeof(writeBuffer), m_faxCallInfo->spanDSPAddr, m_faxCallInfo->spanDSPPort)) {
          PTRACE(1, "Fax\tSocket write error - " << m_faxCallInfo->socket.GetErrorText(PChannel::LastWriteError));
          return false;
        }
        writeBufferLen = 0;
      }
    }
  }

  return true;
}

PBoolean OpalFaxMediaStream::Close()
{
  if (!OpalMediaStream::Close())
    return false;

  // Give the spandsp sub-process a second to tidy up and exit with
  // some statistics.
  if (m_faxCallInfo->stdoutThread != NULL) {
    PTRACE(4, "Fax\tAwaiting final statistics from SpanDSP");
    if (!m_faxCallInfo->stdoutThread->WaitForTermination(2000)) {
      // OK, force the issue
      m_faxCallInfo->spanDSP.Close();
      m_faxCallInfo->stdoutThread->WaitForTermination(1000);
    }
  }

  OpalFaxCallInfo * faxCallInfo;
  {
    {
      PWaitAndSignal m1(infoMutex);

      if (m_faxCallInfo == NULL || sessionToken.IsEmpty()) {
        PTRACE(1, "Fax\tCannot close unknown media stream");
        return true;
      }

      faxCallInfo = m_faxCallInfo;
      m_faxCallInfo = NULL;
    }

    // shutdown whatever is running
    faxCallInfo->socket.Close();
    faxCallInfo->spanDSP.Close();

    PWaitAndSignal m2(faxMapMutex);

    OpalFaxCallInfoMap_T::iterator r = faxCallInfoMap.find(sessionToken);
    if (r == faxCallInfoMap.end()) {
      PTRACE(1, "Fax\tError: media stream not found in T38 session list");
      return true;
    }

    if (r->second != faxCallInfo) {
      PTRACE(1, "Fax\tError: session list does not match local ptr");
      return true;
    }

    if (faxCallInfo->refCount == 0) {
      PTRACE(1, "Fax\tError: media stream has incorrect reference count");
      return true;
    }

    if (--faxCallInfo->refCount > 0) {
      PTRACE(1, "Fax\tClosed fax media stream (references left)");
      return true;
    }

    // remove info from map
    faxCallInfoMap.erase(sessionToken);
  }

  if (faxCallInfo->stdoutThread != NULL) {
    delete faxCallInfo->stdoutThread;
    faxCallInfo->stdoutThread = NULL;
  }

  // delete the object
  delete faxCallInfo;

  PTRACE(1, "Fax\tClosed fax media stream (deleted info)");
  return true;
}

PBoolean OpalFaxMediaStream::IsSynchronous() const
{
  // Make sure we don't get a jitter buffer by saying we are asynchronous when a sink stream
  return IsSource();
}


#if OPAL_STATISTICS
void OpalFaxMediaStream::GetStatistics(OpalMediaStatistics & statistics, bool fromPatch) const
{
  OpalMediaStream::GetStatistics(statistics, fromPatch);
  statistics.m_fax = m_statistics;
}
#endif


static bool ExtractValue(const PString & msg, PINDEX & position, int & value, char sep = '=')
{
  position = msg.Find(sep, position+1);
  if (position == P_MAX_INDEX)
    return false;

  PINDEX eol = msg.Find('\n', position);
  if (eol == P_MAX_INDEX)
    return false;

  value = msg(position+1, eol-1).AsInteger();
  return true;
}


void OpalFaxMediaStream::ReadStdOut(PThread &, INT)
{
  PTRACE(4, "Fax\tSpanDSP stdout reading thread started");

  PString msg;
  bool notInStats = true;

  for (;;) {
    int c = m_faxCallInfo->spanDSP.ReadChar();
    if (c < 0) {
      PTRACE(2, "Fax\tError reading from " << m_faxCallInfo->spanDSP.GetName()
             << ": " << m_faxCallInfo->spanDSP.GetErrorText(PChannel::LastReadError));
#if OPAL_STATISTICS
      m_connection.OnFaxCompleted(m_statistics.m_result != 0);
#else
      m_connection.OnFaxCompleted(m_result != 0);
#endif
      return;
    }

    if (c != '\r' && c != '\n') {
      msg += (char)c;
      continue;
    }

    if (notInStats) {
      notInStats = msg.Find("statistics") == P_MAX_INDEX;
      if (notInStats) {
        PTRACE_IF(4, !msg.IsEmpty(), "Fax\tSpanDSP Output: \"" << msg << '"');
        msg.MakeEmpty();
        continue;
      }
    }

    if (msg.Find("----------") == P_MAX_INDEX) {
      msg += (char)c;
      continue;
    }

    PINDEX position = 0;
#if OPAL_STATISTICS
    int result, errorCorrection;
    if (ExtractValue(msg, position, result) &&
        ExtractValue(msg, position, m_statistics.m_bitRate) &&
        ExtractValue(msg, position, m_statistics.m_compression) &&
        ExtractValue(msg, position, errorCorrection) &&
        ExtractValue(msg, position, m_statistics.m_txPages) &&
        ExtractValue(msg, position, m_statistics.m_rxPages) &&
        ExtractValue(msg, position, m_statistics.m_totalPages) &&
        ExtractValue(msg, position, m_statistics.m_imageSize) &&
        ExtractValue(msg, position, m_statistics.m_resolutionX) &&
        ExtractValue(msg, position, m_statistics.m_resolutionY, 'x') &&
        ExtractValue(msg, position, m_statistics.m_pageWidth) &&
        ExtractValue(msg, position, m_statistics.m_pageHeight, 'x') &&
        ExtractValue(msg, position, m_statistics.m_badRows) &&
        ExtractValue(msg, position, m_statistics.m_mostBadRows) &&
        ExtractValue(msg, position, m_statistics.m_errorCorrectionRetries))
    {
      m_statistics.m_result = result; // Only set this if everything parsed correctly
      m_statistics.m_errorCorrection = errorCorrection != 0;
    }
#else
    ExtractValue(msg, position, m_result);
#endif
    PTRACE(4, "Fax\tSpanDSP Output:\n" << msg);
    notInStats = true;
    msg.MakeEmpty();
  }
}


PString OpalFaxMediaStream::GetSpanDSPCommandLine(OpalFaxCallInfo & info)
{
  PStringStream cmdline;

  PIPSocket::Address dummy; WORD port;
  info.socket.GetLocalAddress(dummy, port);

  cmdline << ((OpalFaxEndPoint &)connection.GetEndPoint()).GetSpanDSP() << " -m ";
  if (m_receive)
    cmdline << "fax_to_tiff";
  else {
    cmdline << "tiff_to_fax";
    if (!m_stationId.IsEmpty())
      cmdline << " -s '" << m_stationId << "'";
  }
#if PTRACING
  if (PTrace::CanTrace(5))
    cmdline << " -v";
#endif
  cmdline << " -V 0 -n '" << m_filename << "' -f 127.0.0.1:" << port;

  return cmdline;
}

/////////////////////////////////////////////////////////////////////////////

/**This class describes a media stream that transfers data to/from a T.38 session
  */
OpalT38MediaStream::OpalT38MediaStream(OpalFaxConnection     & conn,
                                       const OpalMediaFormat & mediaFormat,
                                       unsigned                sessionID, 
                                       bool                    isSource,
                                       const PString         & token,
                                       const PString         & filename,
                                       bool                    receive,
                                       const PString         & stationId)
  : OpalFaxMediaStream(conn, mediaFormat, sessionID, isSource, token, filename, receive, stationId)
{
}


PString OpalT38MediaStream::GetSpanDSPCommandLine(OpalFaxCallInfo & info)
{
  PStringStream cmdline;

  PIPSocket::Address dummy; WORD port;
  info.socket.GetLocalAddress(dummy, port);

  cmdline << ((OpalFaxEndPoint &)connection.GetEndPoint()).GetSpanDSP() << " -m ";
  if (m_receive)
    cmdline << "t38_to_tiff";
  else {
    cmdline << "tiff_to_t38";
    if (!m_stationId.IsEmpty())
      cmdline << " -s '" << m_stationId << "'";
  }
#if PTRACING
  if (PTrace::CanTrace(5))
    cmdline << " -v";
#endif
  cmdline << " -V 0 -n '" << m_filename << "' -t 127.0.0.1:" << port;

  return cmdline;
}


PBoolean OpalT38MediaStream::ReadPacket(RTP_DataFrame & packet)
{
  // it is possible for ReadPacket to be called before the media stream has been opened, so deal with that case
  PWaitAndSignal m(infoMutex);
  if ((m_faxCallInfo == NULL) || !m_faxCallInfo->spanDSP.IsRunning()) {

    // return silence
    packet.SetPayloadSize(0);
    PThread::Sleep(20);

  } else {

    packet.SetSize(2048);

    bool stat;

    if (m_faxCallInfo->spanDSPPort > 0) 
      stat = m_faxCallInfo->socket.Read(packet.GetPointer(), packet.GetSize());
    else {
      stat = m_faxCallInfo->socket.ReadFrom(packet.GetPointer(), packet.GetSize(), m_faxCallInfo->spanDSPAddr, m_faxCallInfo->spanDSPPort);
      PTRACE_IF(2, stat, "Fax\tRemote spandsp address set to " << m_faxCallInfo->spanDSPAddr << ":" << m_faxCallInfo->spanDSPPort);
    }

    if (!stat) {
      if (m_faxCallInfo->socket.GetErrorCode(PChannel::LastReadError) != PChannel::Timeout)
        return false;
      packet.SetPayloadSize(0);
      return true;
    }

    PINDEX len = m_faxCallInfo->socket.GetLastReadCount();
    if (len < RTP_DataFrame::MinHeaderSize)
      return false;

    packet.SetSize(len);
    packet.SetPayloadSize(len - RTP_DataFrame::MinHeaderSize);
  }

  return true;
}


PBoolean OpalT38MediaStream::WritePacket(RTP_DataFrame & packet)
{
  PWaitAndSignal m(infoMutex);
  if ((packet.GetPayloadSize() == 1) && (packet.GetPayloadPtr()[0] == 0xff)) {
    // ignore T.38 timing frames
  } else if ((m_faxCallInfo == NULL) || !m_faxCallInfo->spanDSP.IsRunning() || m_faxCallInfo->spanDSPPort == 0) {
    // queue frames before we know where to send them
    queuedFrames.Append(new RTP_DataFrame(packet));
  } else {
    PTRACE(5, "Fax\tT.38 Write RTP packet size = " << packet.GetHeaderSize() + packet.GetPayloadSize() <<" to " << m_faxCallInfo->spanDSPAddr << ":" << m_faxCallInfo->spanDSPPort);
    if (queuedFrames.GetSize() > 0) {
      for (PINDEX i = 0; i < queuedFrames.GetSize(); ++i) {
        RTP_DataFrame & frame = queuedFrames[i];
        if (!m_faxCallInfo->socket.WriteTo(frame.GetPointer(), frame.GetHeaderSize() + frame.GetPayloadSize(), m_faxCallInfo->spanDSPAddr, m_faxCallInfo->spanDSPPort)) {
          PTRACE(2, "T38_UDP\tSocket write error - " << m_faxCallInfo->socket.GetErrorText(PChannel::LastWriteError));
          return false;
        }
      }
      queuedFrames.RemoveAll();
    }
    if (!m_faxCallInfo->socket.WriteTo(packet.GetPointer(), packet.GetHeaderSize() + packet.GetPayloadSize(), m_faxCallInfo->spanDSPAddr, m_faxCallInfo->spanDSPPort)) {
      PTRACE(2, "T38_UDP\tSocket write error - " << m_faxCallInfo->socket.GetErrorText(PChannel::LastWriteError));
      return false;
    }
  }

  return true;
}


/////////////////////////////////////////////////////////////////////////////

OpalFaxEndPoint::OpalFaxEndPoint(OpalManager & mgr, const char * g711Prefix, const char * t38Prefix)
  : OpalEndPoint(mgr, g711Prefix, CanTerminateCall)
  , m_t38Prefix(t38Prefix)
#ifdef _WIN32
  , m_spanDSP("./spandsp_util.exe")
#else
  , m_spanDSP("./spandsp_util")
#endif
  , m_defaultDirectory(".")
{
  if (t38Prefix != NULL)
    mgr.AttachEndPoint(this, m_t38Prefix);

  PTRACE(3, "Fax\tCreated Fax endpoint");
}


OpalFaxEndPoint::~OpalFaxEndPoint()
{
  PTRACE(3, "Fax\tDeleted Fax endpoint.");
}


OpalFaxConnection * OpalFaxEndPoint::CreateConnection(OpalCall & call,
                                                      void * /*userData*/,
                                                      OpalConnection::StringOptions * stringOptions,
                                                      const PString & filename,
                                                      bool receive,
                                                      bool t38)
{
  PString token = t38 ? m_t38Prefix : GetPrefixName();
  token.sprintf("_%i", ++faxCallIndex);

  if (t38)
    return new OpalT38Connection(call, *this, filename, receive, token, stringOptions);
  else
    return new OpalFaxConnection(call, *this, filename, receive, token, stringOptions);
}


OpalMediaFormatList OpalFaxEndPoint::GetMediaFormats() const
{
  OpalMediaFormatList formats;
  formats += OpalPCM16;
  formats += OpalT38;
  return formats;
}


void OpalFaxEndPoint::AcceptIncomingConnection(const PString & token)
{
  PSafePtr<OpalFaxConnection> connection = PSafePtrCast<OpalConnection, OpalFaxConnection>(GetConnectionWithLock(token, PSafeReadOnly));
  if (connection != NULL)
    connection->AcceptIncoming();
}


void OpalFaxEndPoint::OnFaxCompleted(OpalFaxConnection & connection, bool failed)
{
  PTRACE(3, "FAX\tFax " << (failed ? "failed" : "completed") << " on connection: " << connection);
  connection.Release(failed ? OpalConnection::EndedByCapabilityExchange : OpalConnection::EndedByLocalUser);
}


PBoolean OpalFaxEndPoint::MakeConnection(OpalCall & call,
                                const PString & remoteParty,
                                         void * userData,
                                 unsigned int /*options*/,
                OpalConnection::StringOptions * stringOptions)
{
  PINDEX prefixLength = remoteParty.Find(':');
  PStringArray tokens = remoteParty.Mid(prefixLength+1).Tokenise(";", true);
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
  if (!PFilePath::IsAbsolutePath(filename))
    filename.Splice(m_defaultDirectory, 0);

  if (!receive && !PFile::Exists(filename)) {
    PTRACE(2, "Fax\tCannot find filename '" << filename << "'");
    return false;
  }

  OpalConnection::StringOptions localOptions;
  if (stringOptions == NULL)
    stringOptions = &localOptions;

  if ((*stringOptions)("stationid").IsEmpty())
    stringOptions->SetAt("stationid", stationId);

  stringOptions->SetAt(OPAL_OPT_DISABLE_JITTER, "1");

  return AddConnection(CreateConnection(call, userData, stringOptions, filename, receive,
                                        remoteParty.Left(prefixLength) *= m_t38Prefix));
}


/////////////////////////////////////////////////////////////////////////////

OpalFaxConnection::OpalFaxConnection(OpalCall        & call,
                                     OpalFaxEndPoint & ep,
                                     const PString   & filename,
                                     bool              receive,
                                     const PString   & token,
                                     OpalConnection::StringOptions * stringOptions)
  : OpalConnection(call, ep, token, 0, stringOptions)
  , m_endpoint(ep)
  , m_filename(filename)
  , m_receive(receive)
{
  synchronousOnRelease = false;
  PTRACE(3, "FAX\tCreated FAX connection with token \"" << callToken << '"');
}


OpalFaxConnection::~OpalFaxConnection()
{
  PTRACE(3, "FAX\tDeleted FAX connection.");
}


void OpalFaxConnection::ApplyStringOptions(OpalConnection::StringOptions & stringOptions)
{
  m_stationId = stringOptions("stationid");
  OpalConnection::ApplyStringOptions(stringOptions);
}


OpalMediaStream * OpalFaxConnection::CreateMediaStream(const OpalMediaFormat & mediaFormat, unsigned sessionID, bool isSource)
{
  return new OpalFaxMediaStream(*this, mediaFormat, sessionID, isSource, GetToken(), m_filename, m_receive, m_stationId);
}


PBoolean OpalFaxConnection::SetUpConnection()
{
  // Check if we are A-Party in this call, so need to do things differently
  if (ownerCall.GetConnection(0) == this) {
    SetPhase(SetUpPhase);

    if (!OnIncomingConnection(0, NULL)) {
      Release(EndedByCallerAbort);
      return false;
    }

    PTRACE(2, "FAX\tOutgoing call routed to " << ownerCall.GetPartyB() << " for " << *this);
    if (!ownerCall.OnSetUp(*this)) {
      Release(EndedByNoAccept);
      return false;
    }

    return true;
  }

  PTRACE(3, "FAX\tSetUpConnection(" << remotePartyName << ')');
  SetPhase(AlertingPhase);
  OnAlerting();

  OnConnectedInternal();

  return true;
}


PBoolean OpalFaxConnection::SetAlerting(const PString & calleeName, PBoolean)
{
  PTRACE(3, "Fax\tSetAlerting(" << calleeName << ')');
  SetPhase(AlertingPhase);
  remotePartyName = calleeName;
  return true;
}


OpalMediaFormatList OpalFaxConnection::GetMediaFormats() const
{
  OpalMediaFormatList formats;
  formats += OpalPCM16;
  return formats;
}


void OpalFaxConnection::AdjustMediaFormats(OpalMediaFormatList & mediaFormats) const
{
  // Remove everything but G.711 or T.38
  OpalMediaFormatList::iterator i = mediaFormats.begin();
  while (i != mediaFormats.end()) {
    if (*i == OpalG711_ULAW_64K || *i == OpalG711_ALAW_64K || *i == OpalT38)
      ++i;
    else
      mediaFormats -= *i++;
  }

  OpalConnection::AdjustMediaFormats(mediaFormats);
}


void OpalFaxConnection::AcceptIncoming()
{
  if (LockReadWrite()) {
    OnConnectedInternal();
    UnlockReadWrite();
  }
}


void OpalFaxConnection::OnFaxCompleted(bool failed)
{
  m_endpoint.OnFaxCompleted(*this, failed);
}


/////////////////////////////////////////////////////////////////////////////

OpalT38Connection::OpalT38Connection(OpalCall        & call,
                                     OpalFaxEndPoint & ep,
                                     const PString   & filename,
                                     bool              receive,
                                     const PString   & token,
                                     OpalConnection::StringOptions * stringOptions)
  : OpalFaxConnection(call, ep, filename, receive, token, stringOptions)
  , m_syncMode(Mode_Timeout)
  , m_faxMode(false)
{
  PTRACE(3, "FAX\tCreated T.38 connection");
}


void OpalT38Connection::ApplyStringOptions(OpalConnection::StringOptions & stringOptions)
{
  PCaselessString opt = stringOptions("Fax-Sync-Mode");
  if (opt == "Wait")
    m_syncMode = Mode_Wait;
  else if (opt == "Timeout")
    m_syncMode = Mode_Timeout;
  else if (opt == "UserInput")
    m_syncMode = Mode_UserInput;
  else if (opt == "InBand")
    m_syncMode = Mode_InBand;

  OpalFaxConnection::ApplyStringOptions(stringOptions);
}


void OpalT38Connection::OnEstablished()
{
  OpalFaxConnection::OnEstablished();

  if (m_faxMode)
    return;

  switch (m_syncMode) {
    case Mode_Timeout :
      m_faxTimer.SetNotifier(PCREATE_NOTIFIER(OnFaxChangeTimeout));
      m_faxTimer = 2000;
      PTRACE(1, "T38\tStarting timer for mode change");
      break;

    case Mode_UserInput :
    case Mode_InBand :
      m_faxTimer.SetNotifier(PCREATE_NOTIFIER(OnSendCNGCED));
      m_faxTimer = 1000;
      PTRACE(1, "T38\tStarting timer for CNG/CED tone");
      break;

    default :
      break;
  }
}


OpalMediaStream * OpalT38Connection::CreateMediaStream(const OpalMediaFormat & mediaFormat, unsigned sessionID, PBoolean isSource)
{
  // if creating an audio session, use a NULL stream
  if (mediaFormat.GetMediaType() == OpalMediaType::Audio())
    return new OpalNullMediaStream(*this, mediaFormat, sessionID, isSource, true);

  // if creating a T.38 stream, see what type it is
  if (mediaFormat.GetMediaType() == OpalMediaType::Fax())
    return new OpalT38MediaStream(*this, mediaFormat, sessionID, isSource, GetToken(), m_filename, m_receive, m_stationId);

  return NULL;
}


void OpalT38Connection::OnMediaPatchStop(unsigned sessionId, bool isSource)
{
  OpalMediaStreamPtr stream = GetMediaStream(sessionId, isSource);
  bool newMode = stream->GetMediaFormat().GetMediaType() != OpalMediaType::Fax();
  if (m_faxMode != newMode) {
    m_faxTimer.Stop();
    m_faxMode = newMode;
  }

  OpalConnection::OnMediaPatchStop(sessionId, isSource);
}


OpalMediaFormatList OpalT38Connection::GetMediaFormats() const
{
  OpalMediaFormatList formats;

  formats += OpalPCM16;
  formats += OpalT38;

  return formats;
}


PBoolean OpalT38Connection::SendUserInputTone(char tone, unsigned /*duration*/)
{
  if (!m_faxMode && m_syncMode != Mode_Wait && toupper(tone) == (m_receive ? 'X' : 'Y'))
    RequestFaxMode(true);
  return true;
}


void OpalT38Connection::OnSendCNGCED(PTimer & timer, INT)
{
  if (LockReadOnly()) {
    if (!m_faxMode && m_syncMode == Mode_UserInput) {
      if (m_receive) {
        // Cadence for CED is single tone, but we repeat just in case
        OnUserInputTone('Y', 3600);
        timer = 5000;
      }
      else {
        // Cadence for CNG is 500ms on 3 seconds off
        OnUserInputTone('X', 500);
        timer = 3000;
      }
    }
    UnlockReadOnly();
  }
}


void OpalT38Connection::OnFaxChangeTimeout(PTimer &, INT)
{
  if (LockReadWrite()) {
    RequestFaxMode(true);
    UnlockReadWrite();
  }
}


void OpalT38Connection::OpenFaxStreams(PThread &, INT)
{
  if (!LockReadWrite())
    return;

  OpalMediaFormat format = m_faxMode ? OpalT38 : OpalG711uLaw;
  OpalMediaType mediaType = format.GetMediaType();

  PSafePtr<OpalConnection> other = ownerCall.GetOtherPartyConnection(*this);
  if ( other == NULL ||
      !ownerCall.OpenSourceMediaStreams(*other, mediaType, 1, format) ||
      !ownerCall.OpenSourceMediaStreams(*this,  mediaType, 1, format)) {
    PTRACE(1, "T38\tMode change request to " << mediaType << " failed");
    OnFaxCompleted(true);
  }

  UnlockReadWrite();
}


void OpalT38Connection::RequestFaxMode(bool toFax)
{
#if PTRACING
  const char * modeStr = toFax ? "fax" : "audio";
#endif

  if (toFax == m_faxMode) {
    PTRACE(1, "T38\tAlready in mode " << modeStr);
    return;
  }

  // definitely changing mode
  PTRACE(3, "T38\tRequesting mode change to " << modeStr);

  m_faxMode = toFax;
  m_faxTimer.Stop();
  if (m_syncMode == Mode_UserInput)
    OnUserInputTone('\0', 0);

  PThread::Create(PCREATE_NOTIFIER(OpenFaxStreams));
}


#endif // OPAL_FAX

