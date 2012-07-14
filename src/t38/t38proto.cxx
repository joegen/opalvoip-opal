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

  #ifdef _MSC_VER
    #pragma message("T.38 Fax (spandsp) support enabled")
  #endif

#if OPAL_PTLIB_ASN
#include <asn/t38.h>
#endif

OPAL_DEFINE_MEDIA_COMMAND(OpalFaxTerminate, PLUGINCODEC_CONTROL_TERMINATE_CODEC);

#define new PNEW


static const char TIFF_File_FormatName[] = OPAL_FAX_TIFF_FILE;


/////////////////////////////////////////////////////////////////////////////

#if OPAL_PTLIB_ASN

OpalFaxMediaStream::OpalFaxMediaStream(OpalConnection & conn,
                                       const OpalMediaFormat & mediaFormat,
                                       unsigned sessionID,
                                       bool isSource,
                                       OpalFaxSession & session)
  : OpalMediaStream(conn, mediaFormat, sessionID, isSource)
  , m_session(session)
{
}


PBoolean OpalFaxMediaStream::ReadPacket(RTP_DataFrame & packet)
{
  if (!m_session.ReadData(packet))
    return false;

  timestamp = packet.GetTimestamp();
  return true;
}


PBoolean OpalFaxMediaStream::WritePacket(RTP_DataFrame & packet)
{
  timestamp = packet.GetTimestamp();
  return m_session.WriteData(packet);
}


PBoolean OpalFaxMediaStream::IsSynchronous() const
{
  return false;
}


/////////////////////////////////////////////////////////////////////////////

OpalFaxSession::OpalFaxSession(const Init & init)
  : OpalMediaSession(init)
  , m_dataSocket(NULL)
  , m_consecutiveBadPackets(0)
  , m_oneGoodPacket(false)
  , m_receivedPacket(new T38_UDPTLPacket)
  , m_expectedSequenceNumber(0)
  , m_secondaryPacket(-1)
  , m_optimiseOnRetransmit(false) // not optimise udptl packets on retransmit
  , m_sentPacket(new T38_UDPTLPacket)
{
  m_timerWriteDataIdle.SetNotifier(PCREATE_NOTIFIER(OnWriteDataIdle));

  m_sentPacket->m_error_recovery.SetTag(T38_UDPTLPacket_error_recovery::e_secondary_ifp_packets);
  m_sentPacket->m_seq_number = (unsigned)-1;

  m_redundancy[32767] = 1;  // re-send all ifp packets 1 time
}


OpalFaxSession::~OpalFaxSession()
{
  m_timerWriteDataIdle.Stop();
  delete m_receivedPacket;
  delete m_sentPacket;
}


void OpalFaxSession::ApplyMediaOptions(const OpalMediaFormat & mediaFormat)
{
  for (PINDEX i = 0 ; i < mediaFormat.GetOptionCount() ; i++) {
    const OpalMediaOption & option = mediaFormat.GetOption(i);
    PCaselessString key = option.GetName();

    if (key == "T38-UDPTL-Redundancy") {
      PStringArray value = option.AsString().Tokenise(",", FALSE);
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

        PTRACE(2, "T38_UDPTL\tIgnored redundancy \"" << value[i] << "\"");
      }

#if PTRACING
      if (PTrace::CanTrace(3)) {
        ostream & trace = PTrace::Begin(3, __FILE__, __LINE__, this);
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
      m_redundancyInterval = option.AsString().AsUnsigned();
      PTRACE(3, "T38_UDPTL\tUse redundancy interval " << m_redundancyInterval);
    }
    else
    if (key == "T38-UDPTL-Keep-Alive-Interval") {
      PWaitAndSignal mutex(m_writeMutex);
      m_keepAliveInterval = option.AsString().AsUnsigned();
      PTRACE(3, "T38_UDPTL\tUse keep-alive interval " << m_keepAliveInterval);
    }
    else
    if (key == "T38-UDPTL-Optimise-On-Retransmit") {
      PCaselessString value = option.AsString();
      PWaitAndSignal mutex(m_writeMutex);
      m_optimiseOnRetransmit = (value.IsEmpty() || (value == "true") || (value == "yes") || value.AsInteger() != 0);

      PTRACE(3, "T38_UDPTL\tUse optimise on retransmit - " << (m_optimiseOnRetransmit ? "true" : "false"));
    }
  }
}


void OpalFaxSession::AttachTransport(Transport & transport)
{
  m_savedTransport = transport;
  m_dataSocket = dynamic_cast<PUDPSocket *>(&transport.front());
}


OpalMediaSession::Transport OpalFaxSession::DetachTransport()
{
  Transport temp = m_savedTransport;
  m_savedTransport = Transport(); // Detaches reference
  m_dataSocket = NULL;
  return temp;
}


OpalMediaStream * OpalFaxSession::CreateMediaStream(const OpalMediaFormat & mediaFormat, unsigned sessionID, bool isSource)
{
  return new OpalFaxMediaStream(m_connection, mediaFormat, sessionID, isSource, *this);
}


bool OpalFaxSession::WriteData(RTP_DataFrame & frame)
{
  PINDEX plLen = frame.GetPayloadSize();

  if (plLen == 0) {
    PTRACE(2, "T38_UDPTL\tInternal error - empty payload");
    return false;
  }

  PWaitAndSignal mutex(m_writeMutex);

  if (!m_sentPacketRedundancy.empty()) {
    T38_UDPTLPacket_error_recovery &recovery = m_sentPacket->m_error_recovery;

    if (recovery.GetTag() == T38_UDPTLPacket_error_recovery::e_secondary_ifp_packets) {
      // shift old primary ifp packet to secondary list

      T38_UDPTLPacket_error_recovery_secondary_ifp_packets &secondary = recovery;

      if (secondary.SetSize(secondary.GetSize() + 1)) {
        for (int i = secondary.GetSize() - 2 ; i >= 0 ; i--) {
          secondary[i + 1] = secondary[i];
          secondary[i] = T38_UDPTLPacket_error_recovery_secondary_ifp_packets_subtype();
        }

        secondary[0].SetValue(m_sentPacket->m_primary_ifp_packet.GetValue());
        m_sentPacket->m_primary_ifp_packet = T38_UDPTLPacket_primary_ifp_packet();
      }
    } else {
      PTRACE(3, "T38_UDPTL\tNot implemented yet " << recovery.GetTagName());
    }
  }

  // calculate redundancy for new ifp packet

  int redundancy = 0;

  for (std::map<int, int>::iterator i = m_redundancy.begin() ; i != m_redundancy.end() ; i++) {
    if (plLen <= i->first) {
      if (redundancy < i->second)
        redundancy = i->second;

      break;
    }
  }

  if (redundancy > 0 || !m_sentPacketRedundancy.empty())
    m_sentPacketRedundancy.insert(m_sentPacketRedundancy.begin(), redundancy + 1);

  // set new primary ifp packet

  m_sentPacket->m_seq_number = frame.GetSequenceNumber();
  m_sentPacket->m_primary_ifp_packet.SetValue(frame.GetPayloadPtr(), plLen);

  bool ok = WriteUDPTL();

  DecrementSentPacketRedundancy(true);

  if (m_sentPacketRedundancy.empty() || m_redundancyInterval <= 0)
    m_timerWriteDataIdle = m_keepAliveInterval;
  else
    m_timerWriteDataIdle = m_redundancyInterval;

  return ok;
}


void OpalFaxSession::OnWriteDataIdle(PTimer &, INT)
{
  PWaitAndSignal mutex(m_writeMutex);

  WriteUDPTL();

  DecrementSentPacketRedundancy(m_optimiseOnRetransmit);
}


void OpalFaxSession::DecrementSentPacketRedundancy(bool stripRedundancy)
{
  int iMax = (int)m_sentPacketRedundancy.size() - 1;

  for (int i = iMax ; i >= 0 ; i--) {
    m_sentPacketRedundancy[i]--;

    if (i == iMax && m_sentPacketRedundancy[i] <= 0)
      iMax--;
  }

  m_sentPacketRedundancy.resize(iMax + 1);

  if (stripRedundancy) {
    T38_UDPTLPacket_error_recovery &recovery = m_sentPacket->m_error_recovery;

    if (recovery.GetTag() == T38_UDPTLPacket_error_recovery::e_secondary_ifp_packets) {
      T38_UDPTLPacket_error_recovery_secondary_ifp_packets &secondary = recovery;
      secondary.SetSize(iMax > 0 ? iMax : 0);
    } else {
      PTRACE(3, "T38_UDPTL\tNot implemented yet " << recovery.GetTagName());
    }
  }
}


bool OpalFaxSession::WriteUDPTL()
{
  PTRACE(5, "T38_UDPTL\tEncoded transmitted UDPTL data :\n  " << setprecision(2) << m_sentPacket);

  PPER_Stream rawData;
  m_sentPacket->Encode(rawData);
  rawData.CompleteEncoding();

  PTRACE(4, "T38_UDPTL\tSending UDPTL of size " << rawData.GetSize());

  return m_dataSocket != NULL && m_dataSocket->Write(rawData.GetPointer(), rawData.GetSize());
}


void OpalFaxSession::SetFrameFromIFP(RTP_DataFrame & frame, const PASN_OctetString & ifp, unsigned sequenceNumber)
{
  frame.SetPayloadSize(ifp.GetDataLength());
  memcpy(frame.GetPayloadPtr(), (const BYTE *)ifp, ifp.GetDataLength());
  frame.SetSequenceNumber((WORD)(sequenceNumber & 0xffff));
  if (m_secondaryPacket <= 0)
    m_expectedSequenceNumber = sequenceNumber+1;
}


bool OpalFaxSession::ReadData(RTP_DataFrame & frame)
{
  if (m_secondaryPacket >= 0) {
    if (m_secondaryPacket == 0)
      SetFrameFromIFP(frame, m_receivedPacket->m_primary_ifp_packet, m_receivedPacket->m_seq_number);
    else {
      T38_UDPTLPacket_error_recovery_secondary_ifp_packets & secondaryPackets = m_receivedPacket->m_error_recovery;
      SetFrameFromIFP(frame, secondaryPackets[m_secondaryPacket-1], m_receivedPacket->m_seq_number - m_secondaryPacket);
    }
    --m_secondaryPacket;
    return true;
  }

  BYTE thisUDPTL[500];
  if (m_dataSocket == NULL || !m_dataSocket->Read(thisUDPTL, sizeof(thisUDPTL)))
    return false;

  PINDEX pduSize = m_dataSocket->GetLastReadCount();
      
  PTRACE(4, "T38_UDPTL\tRead UDPTL of size " << pduSize);

  PPER_Stream rawData(thisUDPTL, pduSize);

      // Decode the PDU
  if (!m_receivedPacket->Decode(rawData)) {
#if PTRACING
    if (m_oneGoodPacket)
      PTRACE(2, "T38_UDPTL\tRaw data decode failure:\n  "
             << setprecision(2) << rawData << "\n  UDPTL = "
             << setprecision(2) << m_receivedPacket);
    else
      PTRACE(2, "T38_UDPTL\tRaw data decode failure: " << rawData.GetSize() << " bytes.");
#endif

    m_consecutiveBadPackets++;
    if (m_consecutiveBadPackets < 1000)
      return true;

    PTRACE(1, "T38_UDPTL\tRaw data decode failed 1000 times, remote probably not switched from audio, aborting!");
    return false;
  }

  PTRACE_IF(3, !m_oneGoodPacket, "T38_UDPTL\tFirst decoded UDPTL packet");
  m_oneGoodPacket = true;
  m_consecutiveBadPackets = 0;

  PTRACE(5, "T38_UDPTL\tDecoded UDPTL packet:\n  " << setprecision(2) << m_receivedPacket);

  int missing = m_receivedPacket->m_seq_number - m_expectedSequenceNumber;
  if (missing > 0 && m_receivedPacket->m_error_recovery.GetTag() == T38_UDPTLPacket_error_recovery::e_secondary_ifp_packets) {
    // Packets are missing and we have redundency in the UDPTL packets
    T38_UDPTLPacket_error_recovery_secondary_ifp_packets & secondaryPackets = m_receivedPacket->m_error_recovery;
    if (secondaryPackets.GetSize() > 0) {
      PTRACE(4, "T38_UDPTL\tUsing redundant data to reconstruct missing/out of order packet at SN=" << m_expectedSequenceNumber);
      m_secondaryPacket = missing;
      if (m_secondaryPacket > secondaryPackets.GetSize())
        m_secondaryPacket = secondaryPackets.GetSize();
      SetFrameFromIFP(frame, secondaryPackets[m_secondaryPacket-1], m_receivedPacket->m_seq_number - m_secondaryPacket);
      --m_secondaryPacket;
      return true;
    }
  }

  SetFrameFromIFP(frame, m_receivedPacket->m_primary_ifp_packet, m_receivedPacket->m_seq_number);
  m_expectedSequenceNumber = m_receivedPacket->m_seq_number+1;

  return true;
}

#endif // OPAL_PTLIB_ASN

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

  if ((*stringOptions)(OPAL_OPT_STATION_ID).IsEmpty())
    stringOptions->SetAt(OPAL_OPT_STATION_ID, stationId);

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
#if OPAL_PTLIB_ASN
  formats += OpalT38;
#endif
  formats += TIFF_File_FormatName;
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

  PTRACE(3, "FAX\tCreated FAX connection with token \"" << callToken << "\","
            " receiving=" << receiving << ","
            " disabledT38=" << disableT38 << ","
            " filename=\"" << filename << '"'
            << "\n" << setw(-1) << m_tiffFileFormat);
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
    else if (it->GetMediaType() != OpalMediaType::Fax()
             || (m_disableT38
#ifdef OPAL_PTLIB_ASN
                              && *it == OpalT38
#endif
                                                )
        )
      mediaFormats -= *it++;
    else
      SetFaxMediaFormatOptions(*it++);
  }

  OpalConnection::AdjustMediaFormats(local, otherConnection, mediaFormats);
}


void OpalFaxConnection::OnApplyStringOptions()
{
  OpalLocalConnection::OnApplyStringOptions();
  SetFaxMediaFormatOptions(m_tiffFileFormat);
}


void OpalFaxConnection::SetFaxMediaFormatOptions(OpalMediaFormat & mediaFormat) const
{
  PTRACE(4, "FAX\tSetting fax media format options from string options " << m_stringOptions);

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
#if OPAL_PTLIB_ASN
  // If switched and we don't need to do CNG/CED any more, or T.38 is disabled
  // in which case the SpanDSP will deal with CNG/CED stuff.
  if (m_state == e_AwaitingSwitchToT38) {
    PString str = m_stringOptions(OPAL_T38_SWITCH_TIME);
    if (!str.IsEmpty()) {
      m_switchTimer.SetInterval(0, str.AsUnsigned());
      PTRACE(3, "FAX\tStarting timer for auto-switch to T.38");
    }
  }
#endif
}


void OpalFaxConnection::OnReleased()
{
  m_switchTimer.Stop(false);
  OpalConnection::OnReleased();
}


OpalMediaStream * OpalFaxConnection::CreateMediaStream(const OpalMediaFormat & mediaFormat, unsigned sessionID, bool isSource)
{
  return new OpalNullMediaStream(*this, mediaFormat, sessionID, isSource, isSource, true);
}


void OpalFaxConnection::OnStartMediaPatch(OpalMediaPatch & patch)
{
#if OPAL_PTLIB_ASN
  // Have switched to T.38 mode
  if (patch.GetSink()->GetMediaFormat() == OpalT38) {
    m_switchTimer.Stop(false);
    m_state = e_CompletedSwitch;
    m_finalStatistics.m_fax.m_result = OpalMediaStatistics::FaxNotStarted;
    PTRACE(4, "FAX\tStarted fax media stream for " << m_tiffFileFormat
           << " state=" << m_state << " switch=" << m_faxMediaStreamsSwitchState);
  }
#endif

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
    GetEndPoint().GetManager().QueueDecoupledEvent(
          new PSafeWorkNoArg<OpalFaxConnection, OpalConnection>(this, &OpalFaxConnection::OpenFaxStreams));
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
    GetEndPoint().GetManager().QueueDecoupledEvent(
          new PSafeWorkNoArg<OpalFaxConnection, OpalConnection>(this, &OpalFaxConnection::OpenFaxStreams));
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


void OpalFaxConnection::OpenFaxStreams()
{
  if (LockReadWrite()) {
    m_state = e_SwitchingToT38;
    if (!SwitchFaxMediaStreams(true))
      m_state = e_CompletedSwitch; // Couldn't, don't try again.
    UnlockReadWrite();
  }
}

#else

  #ifdef _MSC_VER
    #pragma message("T.38 Fax (spandsp) support DISABLED")
  #endif

#endif // OPAL_FAX

