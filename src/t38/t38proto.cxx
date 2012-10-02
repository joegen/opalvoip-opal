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
#include <opal/patch.h>
#include <codec/opalpluginmgr.h>


/////////////////////////////////////////////////////////////////////////////

#if OPAL_FAX

#include <asn/t38.h>

OPAL_DEFINE_MEDIA_COMMAND(OpalFaxTerminate, PLUGINCODEC_CONTROL_TERMINATE_CODEC);

#define new PNEW


static const char TIFF_File_FormatName[] = OPAL_FAX_TIFF_FILE;


/////////////////////////////////////////////////////////////////////////////

class OpalFaxMediaStream : public OpalNullMediaStream
{
  public:
    OpalFaxMediaStream(OpalFaxConnection & conn,
                       const OpalMediaFormat & mediaFormat,
                       unsigned sessionID,
                       bool isSource)
      : OpalNullMediaStream(conn, mediaFormat, sessionID, isSource, isSource, true)
      , m_connection(conn)
    {
      m_isAudio = true; // Even though we are not REALLY audio, act like we are
    }

    virtual void InternalClose()
    {
      if (m_connection.m_state == OpalFaxConnection::e_CompletedSwitch &&
          m_connection.m_finalStatistics.m_fax.m_result < 0) {
        // make a referenced copy so can't be deleted out from under us
        PatchPtr patch = m_mediaPatch;
        if (patch != NULL)
          patch->ExecuteCommand(OpalFaxTerminate(), false);
        GetStatistics(m_connection.m_finalStatistics);
        PTRACE(4, "FAX\tGot final statistics: result=" << m_connection.m_finalStatistics.m_fax.m_result);
      }

      OpalNullMediaStream::InternalClose();
    }

  private:
    OpalFaxConnection & m_connection;
};


/////////////////////////////////////////////////////////////////////////////

class T38PseudoRTP_Handler : public RTP_Encoding
{
  public:
    T38PseudoRTP_Handler()
    {
      PStringToString options;

      options.SetAt("T38-UDPTL-Redundancy", "32767:1");           // re-send all ifp packets 1 time
      options.SetAt("T38-UDPTL-Redundancy-Interval", "0");        // re-send redundancy ifp packets only with next ifp
      options.SetAt("T38-UDPTL-Keep-Alive-Interval", "0");        // no send keep-alive packets
      options.SetAt("T38-UDPTL-Optimise-On-Retransmit", "false"); // not optimise udptl packets on retransmit

      ApplyStringOptions(options);
    }


    void OnStart(RTP_Session & _rtpUDP)
    {
      RTP_Encoding::OnStart(_rtpUDP);

      rtpUDP->SetJitterBufferSize(0, 0);
      m_consecutiveBadPackets  = 0;
      m_awaitingGoodPacket     = true;
      m_expectedSequenceNumber = 0;
      m_secondaryPacket        = -1;

      m_sentPacketRedundancy.clear();
      m_sentPacket = T38_UDPTLPacket();
      m_sentPacket.m_error_recovery.SetTag(T38_UDPTLPacket_error_recovery::e_secondary_ifp_packets);
      m_sentPacket.m_seq_number = (unsigned)-1;
      rtpUDP->SetNextSentSequenceNumber(0);
    }


    void ApplyStringOptions(const PStringToString & stringOptions)
    {
      for (PINDEX i = 0 ; i < stringOptions.GetSize() ; i++) {
        PCaselessString key = stringOptions.GetKeyAt(i);

        if (key == "T38-UDPTL-Redundancy") {
          PStringArray value = stringOptions.GetDataAt(i).Tokenise(",", FALSE);
          PWaitAndSignal mutex(m_writeMutex);

          m_redundancy.clear();

          for (PINDEX i = 0 ; i < value.GetSize() ; i++) {
            PStringArray pair = value[i].Tokenise(":", FALSE);

            if (pair.GetSize() == 2) {
              long size = pair[0].AsInteger();
              long redundancy = pair[1].AsInteger();

              if (size > INT_MAX)
                size = INT_MAX;

              if (size > 0 && redundancy >= 0) {
                m_redundancy[(int)size] = (int)redundancy;
                continue;
              }
            }

            PTRACE(2, "T38_UDPTL\tIgnored redundancy \"" << value[i] << '"');
          }

#if PTRACING
          if (PTrace::CanTrace(3)) {
            ostream & trace = PTrace::Begin(3, __FILE__, __LINE__);
            trace << "T38_UDPTL\tUse redundancy \"";
            for (std::map<int, int>::iterator it = m_redundancy.begin() ; it != m_redundancy.end() ; it++) {
              if (it != m_redundancy.begin())
                trace << ",";
              trace << it->first << ':' << it->second;
            }
            trace << '"' << PTrace::End;
          }
#endif
        }
        else
        if (key == "T38-UDPTL-Redundancy-Interval") {
          PWaitAndSignal mutex(m_writeMutex);
          m_redundancyInterval = stringOptions.GetDataAt(i).AsUnsigned();
          PTRACE(3, "T38_UDPTL\tUse redundancy interval " << m_redundancyInterval);
        }
        else
        if (key == "T38-UDPTL-Keep-Alive-Interval") {
          PWaitAndSignal mutex(m_writeMutex);
          m_keepAliveInterval = stringOptions.GetDataAt(i).AsUnsigned();
          PTRACE(3, "T38_UDPTL\tUse keep-alive interval " << m_keepAliveInterval);
        }
        else
        if (key == "T38-UDPTL-Optimise-On-Retransmit") {
          PCaselessString value = stringOptions.GetDataAt(i);
          PWaitAndSignal mutex(m_writeMutex);

          m_optimiseOnRetransmit =
            (value.IsEmpty() || (value == "true") || (value == "yes") || value.AsInteger() != 0);

          PTRACE(3, "T38_UDPTL\tUse optimise on retransmit - " << (m_optimiseOnRetransmit ? "true" : "false"));
        }
        else {
          PTRACE(4, "T38_UDPTL\tIgnored option " << key << " = \"" << stringOptions.GetDataAt(i) << '"');
        }
      }
    }


    PBoolean WriteData(RTP_DataFrame & frame, bool oob)
    {
      if (oob)
        return false;

      return RTP_Encoding::WriteData(frame, false);
    }


    RTP_Session::SendReceiveStatus OnSendData(RTP_DataFrame & frame)
    {
      RTP_Session::SendReceiveStatus status = RTP_Encoding::OnSendData(frame);

      if (status == RTP_Session::e_ProcessPacket && frame.GetPayloadSize() == 0)
        status = RTP_Session::e_IgnorePacket;

      return status;
    }


    bool WriteDataPDU(RTP_DataFrame & frame)
    {
      PINDEX plLen = frame.GetPayloadSize();

      if (plLen == 0) {
        PTRACE(2, "T38_UDPTL\tInternal error - empty payload");
        return false;
      }

      PWaitAndSignal mutex(m_writeMutex);

      if (!m_sentPacketRedundancy.empty()) {
        T38_UDPTLPacket_error_recovery &recovery = m_sentPacket.m_error_recovery;

        if (recovery.GetTag() == T38_UDPTLPacket_error_recovery::e_secondary_ifp_packets) {
          // shift old primary ifp packet to secondary list

          T38_UDPTLPacket_error_recovery_secondary_ifp_packets &secondary = recovery;

          if (secondary.SetSize(secondary.GetSize() + 1)) {
            for (int i = secondary.GetSize() - 2 ; i >= 0 ; i--) {
              secondary[i + 1] = secondary[i];
              secondary[i] = T38_UDPTLPacket_error_recovery_secondary_ifp_packets_subtype();
            }

            secondary[0].SetValue(m_sentPacket.m_primary_ifp_packet.GetValue());
            m_sentPacket.m_primary_ifp_packet = T38_UDPTLPacket_primary_ifp_packet();
          }
        } else {
          PTRACE(3, "T38_UDPTL\tNot implemented yet " << recovery.GetTagName());
        }
      }

      // calculate redundancy for new ifp packet

      int redundancy = 0;

      for (std::map<int, int>::iterator it = m_redundancy.begin() ; it != m_redundancy.end() ; it++) {
        if (plLen <= it->first) {
          if (redundancy < it->second)
            redundancy = it->second;

          break;
        }
      }

      if (redundancy > 0 || !m_sentPacketRedundancy.empty())
        m_sentPacketRedundancy.insert(m_sentPacketRedundancy.begin(), redundancy + 1);

      // set new primary ifp packet

      m_sentPacket.m_seq_number = frame.GetSequenceNumber();
      m_sentPacket.m_primary_ifp_packet.SetValue(frame.GetPayloadPtr(), plLen);

      bool ok = WriteUDPTL();

      DecrementSentPacketRedundancy(true);

      return ok;
    }


    void OnWriteDataIdle()
    {
      PWaitAndSignal mutex(m_writeMutex);

      WriteUDPTL();

      DecrementSentPacketRedundancy(m_optimiseOnRetransmit);
    }


    void SetWriteDataIdleTimer(PTimer & timer)
    {
      PWaitAndSignal mutex(m_writeMutex);

      if (m_sentPacketRedundancy.empty() || m_redundancyInterval <= 0)
        timer = m_keepAliveInterval;
      else
        timer = m_redundancyInterval;
    }


    void DecrementSentPacketRedundancy(bool stripRedundancy)
    {
      int iMax = (int)m_sentPacketRedundancy.size() - 1;

      for (int i = iMax ; i >= 0 ; i--) {
        m_sentPacketRedundancy[i]--;

        if (i == iMax && m_sentPacketRedundancy[i] <= 0)
          iMax--;
      }

      m_sentPacketRedundancy.resize(iMax + 1);

      if (stripRedundancy) {
        T38_UDPTLPacket_error_recovery &recovery = m_sentPacket.m_error_recovery;

        if (recovery.GetTag() == T38_UDPTLPacket_error_recovery::e_secondary_ifp_packets) {
          T38_UDPTLPacket_error_recovery_secondary_ifp_packets &secondary = recovery;
          secondary.SetSize(iMax > 0 ? iMax : 0);
        } else {
          PTRACE(3, "T38_UDPTL\tNot implemented yet " << recovery.GetTagName());
        }
      }
    }


    bool WriteUDPTL()
    {
      PTRACE(5, "T38_UDPTL\tEncoded transmitted UDPTL data :\n  " << setprecision(2) << m_sentPacket);

      PPER_Stream rawData;
      m_sentPacket.Encode(rawData);
      rawData.CompleteEncoding();

      PTRACE(4, "T38_UDPTL\tSending UDPTL of size " << rawData.GetSize());

      return rtpUDP->WriteDataOrControlPDU(rawData.GetPointer(), rawData.GetSize(), true);
    }


    RTP_Session::SendReceiveStatus OnSendControl(RTP_ControlFrame & /*frame*/, PINDEX & /*len*/)
    {
      return RTP_Session::e_IgnorePacket; // Non fatal error, just ignore
    }


    int WaitForPDU(PUDPSocket & dataSocket, PUDPSocket & controlSocket, const PTimeInterval &)
    {
      if (m_secondaryPacket >= 0)
        return -1; // Force immediate call to ReadDataPDU

      // Break out once a second so closes down in orderly fashion
      return PSocket::Select(dataSocket, controlSocket, 1000);
    }


    RTP_Session::SendReceiveStatus OnReadTimeout(RTP_DataFrame & frame)
    {
      // Override so do not do sender reports (RTP only) and push
      // through a zero length packet so checks for orderly shut down
      frame.SetPayloadSize(0);
      return RTP_Session::e_ProcessPacket;
    }


    void SetFrameFromIFP(RTP_DataFrame & frame, const PASN_OctetString & ifp, unsigned sequenceNumber)
    {
      frame.SetPayloadSize(ifp.GetDataLength());
      memcpy(frame.GetPayloadPtr(), (const BYTE *)ifp, ifp.GetDataLength());
      frame.SetSequenceNumber((WORD)(sequenceNumber & 0xffff));
      if (m_secondaryPacket <= 0)
        m_expectedSequenceNumber = sequenceNumber+1;
    }

    RTP_Session::SendReceiveStatus ReadDataPDU(RTP_DataFrame & frame)
    {
      if (m_secondaryPacket >= 0) {
        if (m_secondaryPacket == 0)
          SetFrameFromIFP(frame, m_receivedPacket.m_primary_ifp_packet, m_receivedPacket.m_seq_number);
        else {
          T38_UDPTLPacket_error_recovery_secondary_ifp_packets & secondaryPackets = m_receivedPacket.m_error_recovery;
          SetFrameFromIFP(frame, secondaryPackets[m_secondaryPacket-1], m_receivedPacket.m_seq_number - m_secondaryPacket);
        }
        --m_secondaryPacket;
        return RTP_Session::e_ProcessPacket;
      }

      BYTE thisUDPTL[500];
      RTP_Session::SendReceiveStatus status = rtpUDP->ReadDataOrControlPDU(thisUDPTL, sizeof(thisUDPTL), true);
      if (status != RTP_Session::e_ProcessPacket)
        return status;

      PINDEX pduSize = rtpUDP->GetDataSocket().GetLastReadCount();
      
      PTRACE(4, "T38_UDPTL\tRead UDPTL of size " << pduSize);

      PPER_Stream rawData(thisUDPTL, pduSize);

      // Decode the PDU, but not if still receiving RTP
      bool decodeBad = !m_receivedPacket.Decode(rawData);
      if (decodeBad || (m_awaitingGoodPacket && m_receivedPacket.m_seq_number >= 32768)) {
        if (++m_consecutiveBadPackets > 1000) {
          PTRACE(1, "T38_UDPTL\tRaw data decode failed 1000 times, remote probably not switched from audio, aborting!");
          return RTP_Session::e_AbortTransport;
        }

 #if PTRACING
        static const unsigned Level = 2;
        if (PTrace::CanTrace(Level)) {
          ostream & trace = PTrace::Begin(Level, __FILE__, __LINE__);
          trace << "T38_UDPTL\t";
          if (m_awaitingGoodPacket)
            trace << "Probable RTP packet: " << rawData.GetSize() << " bytes.";
          else
            trace << "Raw data decode failure:\n  "
                  << setprecision(2) << rawData << "\n  UDPTL = "
                  << setprecision(2) << m_receivedPacket;
          trace << PTrace::End;
        }
  #endif

        return RTP_Session::e_IgnorePacket;
      }

      PTRACE_IF(3, m_awaitingGoodPacket, "T38_UDPTL\tFirst decoded UDPTL packet");
      m_awaitingGoodPacket = false;
      m_consecutiveBadPackets = 0;

      PTRACE(5, "T38_UDPTL\tDecoded UDPTL packet:\n  " << setprecision(2) << m_receivedPacket);

      int missing = m_receivedPacket.m_seq_number - m_expectedSequenceNumber;
      if (missing > 0 && m_receivedPacket.m_error_recovery.GetTag() == T38_UDPTLPacket_error_recovery::e_secondary_ifp_packets) {
        // Packets are missing and we have redundency in the UDPTL packets
        T38_UDPTLPacket_error_recovery_secondary_ifp_packets & secondaryPackets = m_receivedPacket.m_error_recovery;
        if (secondaryPackets.GetSize() > 0) {
          PTRACE(4, "T38_UDPTL\tUsing redundant data to reconstruct missing/out of order packet at SN=" << m_expectedSequenceNumber);
          m_secondaryPacket = missing;
          if (m_secondaryPacket > secondaryPackets.GetSize())
            m_secondaryPacket = secondaryPackets.GetSize();
          SetFrameFromIFP(frame, secondaryPackets[m_secondaryPacket-1], m_receivedPacket.m_seq_number - m_secondaryPacket);
          --m_secondaryPacket;
          return RTP_Session::e_ProcessPacket;
        }
      }

      SetFrameFromIFP(frame, m_receivedPacket.m_primary_ifp_packet, m_receivedPacket.m_seq_number);
      m_expectedSequenceNumber = m_receivedPacket.m_seq_number+1;

      return RTP_Session::e_ProcessPacket;
    }


  protected:
    int             m_consecutiveBadPackets;
    bool            m_awaitingGoodPacket;
    T38_UDPTLPacket m_receivedPacket;
    unsigned        m_expectedSequenceNumber;
    int             m_secondaryPacket;

    std::map<int, int>  m_redundancy;
    PTimeInterval       m_redundancyInterval;
    PTimeInterval       m_keepAliveInterval;
    bool                m_optimiseOnRetransmit;
    std::vector<int>    m_sentPacketRedundancy;
    T38_UDPTLPacket     m_sentPacket;
    PMutex              m_writeMutex;
};


PFACTORY_CREATE(PFactory<RTP_Encoding>, T38PseudoRTP_Handler, "udptl", false);


/////////////////////////////////////////////////////////////////////////////

OpalFaxEndPoint::OpalFaxEndPoint(OpalManager & mgr, const char * g711Prefix, const char * t38Prefix)
  : OpalLocalEndPoint(mgr, g711Prefix)
  , m_t38Prefix(t38Prefix)
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


PSafePtr<OpalConnection> OpalFaxEndPoint::MakeConnection(OpalCall & call,
                                                    const PString & remoteParty,
                                                             void * userData,
                                                       unsigned int /*options*/,
                                    OpalConnection::StringOptions * stringOptions)
{
  if (!OpalMediaFormat(TIFF_File_FormatName).IsValid()) {
    PTRACE(1, "TIFF File format not valid! Missing plugin?");
    return NULL;
  }

  PINDEX prefixLength = remoteParty.Find(':');
  PStringArray tokens = remoteParty.Mid(prefixLength+1).Tokenise(";", true);
  if (tokens.IsEmpty()) {
    PTRACE(2, "Fax\tNo filename specified!");
    return NULL;
  }

  bool receiving = false;
  PString stationId = GetDefaultDisplayName();

  for (PINDEX i = 1; i < tokens.GetSize(); ++i) {
    if (tokens[i] *= "receive")
      receiving = true;
    else if (tokens[i].Left(10) *= "stationid=")
      stationId = tokens[i].Mid(10);
  }

  PString filename = tokens[0];
  if (!PFilePath::IsAbsolutePath(filename))
    filename.Splice(m_defaultDirectory, 0);

  if (!receiving && !PFile::Exists(filename)) {
    PTRACE(2, "Fax\tCannot find filename '" << filename << "'");
    return NULL;
  }

  OpalConnection::StringOptions localOptions;
  if (stringOptions == NULL)
    stringOptions = &localOptions;

  if ((*stringOptions)("stationid").IsEmpty())
    stringOptions->SetAt("stationid", stationId);

  stringOptions->SetAt(OPAL_OPT_DISABLE_JITTER, "1");

  return AddConnection(CreateConnection(call, userData, stringOptions, filename, receiving,
                                        remoteParty.Left(prefixLength) *= GetPrefixName()));
}


bool OpalFaxEndPoint::IsAvailable() const
{
  return OpalMediaFormat(OPAL_FAX_TIFF_FILE).IsValid();
}


OpalFaxConnection * OpalFaxEndPoint::CreateConnection(OpalCall & call,
                                                      void * /*userData*/,
                                                      OpalConnection::StringOptions * stringOptions,
                                                      const PString & filename,
                                                      bool receiving,
                                                      bool disableT38)
{
  return new OpalFaxConnection(call, *this, filename, receiving, disableT38, stringOptions);
}


OpalMediaFormatList OpalFaxEndPoint::GetMediaFormats() const
{
  OpalMediaFormatList formats;
  formats += OpalT38;
  formats += TIFF_File_FormatName;
PTRACE(4, "OpalFaxEndPoint\tGetMediaFormats for " << *this << "\n    " << setfill(',') << formats << setfill(' '));
  return formats;
}


void OpalFaxEndPoint::OnFaxCompleted(OpalFaxConnection & connection, bool PTRACE_PARAM(failed))
{
  PTRACE(3, "FAX\tFax " << (failed ? "failed" : "completed") << " on connection: " << connection);
  connection.Release();
}


/////////////////////////////////////////////////////////////////////////////

OpalFaxConnection::OpalFaxConnection(OpalCall        & call,
                                     OpalFaxEndPoint & ep,
                                     const PString   & filename,
                                     bool              receiving,
                                     bool              disableT38,
                                     OpalConnection::StringOptions * stringOptions)
  : OpalLocalConnection(call, ep, NULL, 0, stringOptions, 'F')
  , m_endpoint(ep)
  , m_filename(filename)
  , m_receiving(receiving)
  , m_disableT38(disableT38)
  , m_tiffFileFormat(TIFF_File_FormatName)
  , m_state(disableT38 ? e_CompletedSwitch : e_AwaitingSwitchToT38)
{
  SetFaxMediaFormatOptions(m_tiffFileFormat);

  m_switchTimer.SetNotifier(PCREATE_NOTIFIER(OnSwitchTimeout));

  PTRACE(3, "FAX\tCreated fax connection with token \"" << callToken << "\","
            " receiving=" << receiving << ","
            " disabledT38=" << disableT38 << ","
            " filename=\"" << filename << '"');
}


OpalFaxConnection::~OpalFaxConnection()
{
  PTRACE(3, "FAX\tDeleted FAX connection.");
}


PString OpalFaxConnection::GetPrefixName() const
{
  return m_disableT38 ? m_endpoint.GetPrefixName() : m_endpoint.GetT38Prefix();
}


OpalMediaFormatList OpalFaxConnection::GetMediaFormats() const
{
  OpalMediaFormatList formats;

  if (m_filename.IsEmpty())
    formats += OpalPCM16;
  else
    formats += m_tiffFileFormat;

  if (!m_disableT38) {
    formats += OpalRFC2833;
    formats += OpalCiscoNSE;
  }

  return formats;
}


void OpalFaxConnection::AdjustMediaFormats(bool   local,
                           const OpalConnection * otherConnection,
                            OpalMediaFormatList & mediaFormats) const
{
  // Remove everything but G.711 or fax stuff
  OpalMediaFormatList::iterator it = mediaFormats.begin();
  while (it != mediaFormats.end()) {
    if ((m_state != e_SwitchingToT38 && it->GetMediaType() == OpalMediaType::Audio()) ||
        (*it == OpalG711_ULAW_64K || *it == OpalG711_ALAW_64K || *it == OpalRFC2833 || *it == OpalCiscoNSE))
      ++it;
    else if (it->GetMediaType() != OpalMediaType::Fax() || (m_disableT38 && *it == OpalT38))
      mediaFormats -= *it++;
    else
      SetFaxMediaFormatOptions(*it++);
  }

  OpalConnection::AdjustMediaFormats(local, otherConnection, mediaFormats);
}


void OpalFaxConnection::SetFaxMediaFormatOptions(OpalMediaFormat & mediaFormat) const
{
  mediaFormat.SetOptionString("TIFF-File-Name", m_filename);
  mediaFormat.SetOptionBoolean("Receiving", m_receiving);

  PString str = m_stringOptions(OPAL_OPT_STATION_ID);
  if (!str.IsEmpty()) {
    mediaFormat.SetOptionString("Station-Identifier", str);
    PTRACE(4, "FAX\tSet Station-Identifier: \"" << str << '"');
  }

  str = m_stringOptions(OPAL_OPT_HEADER_INFO);
  if (!str.IsEmpty()) {
    mediaFormat.SetOptionString("Header-Info", str);
    PTRACE(4, "FAX\tSet Header-Info: \"" << str << '"');
  }
}


void OpalFaxConnection::OnEstablished()
{
  OpalConnection::OnEstablished();

  // If switched and we don't need to do CNG/CED any more, or T.38 is disabled
  // in which case the SpanDSP will deal with CNG/CED stuff.
  if (m_state == e_AwaitingSwitchToT38) {
    PString str = m_stringOptions(OPAL_T38_SWITCH_TIME);
    if (!str.IsEmpty()) {
      m_switchTimer.SetInterval(0, str.AsUnsigned());
      PTRACE(3, "FAX\tStarting timer for auto-switch to T.38");
    }
  }
}


void OpalFaxConnection::OnReleased()
{
  m_switchTimer.Stop(false);
  OpalConnection::OnReleased();
}


OpalMediaStream * OpalFaxConnection::CreateMediaStream(const OpalMediaFormat & mediaFormat, unsigned sessionID, bool isSource)
{
  return new OpalFaxMediaStream(*this, mediaFormat, sessionID, isSource);
}


void OpalFaxConnection::OnStartMediaPatch(OpalMediaPatch & patch)
{
  // Have switched to T.38 mode
  if (patch.GetSink()->GetMediaFormat() == OpalT38) {
    m_switchTimer.Stop(false);
    m_state = e_CompletedSwitch;
    m_finalStatistics.m_fax.m_result = OpalMediaStatistics::FaxNotStarted;
    PTRACE(4, "FAX\tStarted fax media stream for " << m_tiffFileFormat
           << " state=" << m_state << " switch=" << m_faxMediaStreamsSwitchState);
  }

  OpalConnection::OnStartMediaPatch(patch);
}


void OpalFaxConnection::OnStopMediaPatch(OpalMediaPatch & patch)
{
  // Finished the fax transmission, look for TIFF
  OpalMediaStream & source = patch.GetSource();
  if (source.GetMediaFormat() == m_tiffFileFormat) {
    m_switchTimer.Stop(false);

    PTRACE(4, "FAX\tStopped fax media stream for " << m_tiffFileFormat
           << " state=" << m_state << " switch=" << m_faxMediaStreamsSwitchState);

    // Not an explicit switch, so fax plug in indicated end of fax
    if (m_state == e_CompletedSwitch && m_faxMediaStreamsSwitchState == e_NotSwitchingFaxMediaStreams) {
#if OPAL_STATISTICS
      InternalGetStatistics(m_finalStatistics, true);
      PTRACE(3, "FAX\tGot final statistics: result=" << m_finalStatistics.m_fax.m_result);
      OnFaxCompleted(m_finalStatistics.m_fax.m_result != OpalMediaStatistics::FaxSuccessful);
#else
      OnFaxCompleted(true);
#endif
    }
  }

  OpalConnection::OnStopMediaPatch(patch);
}


PBoolean OpalFaxConnection::SendUserInputTone(char tone, unsigned duration)
{
  OnUserInputTone(tone, duration);
  return true;
}


void OpalFaxConnection::OnUserInputTone(char tone, unsigned /*duration*/)
{
  // Not yet switched and got a CNG/CED from the remote system, start switch
  if (m_state == e_AwaitingSwitchToT38 &&
         (m_receiving ? (tone == 'X')
                      : (tone == 'Y' && m_stringOptions.GetBoolean(OPAL_SWITCH_ON_CED)))) {
    PTRACE(3, "FAX\tRequesting mode change in response to " << (tone == 'X' ? "CNG" : "CED"));
    PThread::Create(PCREATE_NOTIFIER(OpenFaxStreams));
  }
}


void OpalFaxConnection::OnFaxCompleted(bool failed)
{
  m_endpoint.OnFaxCompleted(*this, failed);

  // Prevent to reuse filename
  m_filename.MakeEmpty();
}


#if OPAL_STATISTICS

void OpalFaxConnection::GetStatistics(OpalMediaStatistics & statistics) const
{
  InternalGetStatistics(statistics, false);
}


void OpalFaxConnection::InternalGetStatistics(OpalMediaStatistics & statistics, bool terminate) const
{
  if (m_finalStatistics.m_fax.m_result >= 0) {
    statistics = m_finalStatistics;
    return;
  }

  OpalMediaStreamPtr stream;
  if ((stream = GetMediaStream(OpalMediaType::Fax(), false)) == NULL &&
      (stream = GetMediaStream(OpalMediaType::Fax(), true )) == NULL) {

    PSafePtr<OpalConnection> other = GetOtherPartyConnection();
    if (other == NULL) {
      PTRACE(2, "FAX\tNo connection to get statistics.");
      return;
    }

    if ((stream = other->GetMediaStream(OpalMediaType::Fax(), false)) == NULL &&
        (stream = other->GetMediaStream(OpalMediaType::Fax(), true )) == NULL) {
      PTRACE(2, "FAX\tNo stream to get statistics.");
      return;
    }
  }

  if (terminate)
    stream->ExecuteCommand(OpalFaxTerminate());

  stream->GetStatistics(statistics);
}

#endif


void OpalFaxConnection::OnSwitchTimeout(PTimer &, INT)
{
  if (m_state == e_AwaitingSwitchToT38) {
    PTRACE(2, "FAX\tDid not switch to T.38 mode, forcing switch");
    PThread::Create(PCREATE_NOTIFIER(OpenFaxStreams));
  }
}


bool OpalFaxConnection::SwitchFaxMediaStreams(bool enableFax)
{
  PSafePtr<OpalConnection> other = GetOtherPartyConnection();
  if (other != NULL && other->SwitchFaxMediaStreams(enableFax))
    return true;

  PTRACE(1, "FAX\tMode change request to " << (enableFax ? "fax" : "audio") << " failed");
  return false;
}


void OpalFaxConnection::OnSwitchedFaxMediaStreams(bool enabledFax)
{
  if (enabledFax) {
    PTRACE(3, "FAX\tMode change request to fax succeeded");
  }
  else {
    PTRACE(4, "FAX\tMode change request to fax failed, falling back to G.711");
    if (m_stringOptions.GetBoolean(OPAL_NO_G111_FAX))
      OnFaxCompleted(true);
    else {
      m_disableT38 = true;
      SwitchFaxMediaStreams(false);
    }
  }

  m_state = e_CompletedSwitch;
}


void OpalFaxConnection::OpenFaxStreams(PThread &, INT)
{
  if (LockReadWrite()) {
    m_state = e_SwitchingToT38;
    if (!SwitchFaxMediaStreams(true))
      m_state = e_CompletedSwitch; // Couldn't, don't try again.
    UnlockReadWrite();
  }
}


#endif // OPAL_FAX

