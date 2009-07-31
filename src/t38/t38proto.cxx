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


static const char TIFF_File_FormatName[] = "TIFF-File";


/////////////////////////////////////////////////////////////////////////////

class OpalFaxMediaStream : public OpalNullMediaStream
{
  public:
    OpalFaxMediaStream(OpalFaxConnection & conn,
                       const OpalMediaFormat & mediaFormat,
                       unsigned sessionID,
                       bool isSource)
      : OpalNullMediaStream(conn, mediaFormat, sessionID, isSource, true)
      , m_connection(conn)
    {
    }

    virtual PBoolean WriteData(const BYTE * data, PINDEX length, PINDEX & written)
    {
      return OpalNullMediaStream::WriteData(data, length, written);
    }

#if OPAL_STATISTICS
    virtual void GetStatistics(OpalMediaStatistics & statistics, bool fromPatch) const
    {
      OpalMediaStream::GetStatistics(statistics, fromPatch);
      statistics.m_fax = m_statistics;
    }
#endif

  private:
    OpalFaxConnection      & m_connection;
    OpalMediaStatistics::Fax m_statistics;
};


/////////////////////////////////////////////////////////////////////////////

class T38PseudoRTP_Handler : public RTP_Encoding
{
  public:
    void OnStart(RTP_Session & _rtpUDP)
    {  
      RTP_Encoding::OnStart(_rtpUDP);
      rtpUDP->SetJitterBufferSize(0, 0);
      m_consecutiveBadPackets  = 0;
      m_oneGoodPacket          = false;
      m_expectedSequenceNumber = 0;
      m_secondaryPacket        = -1;

      m_lastSentIFP.SetSize(0);
      rtpUDP->SetReportTimeInterval(20);
      rtpUDP->SetNextSentSequenceNumber(0);
    }


    PBoolean WriteData(RTP_DataFrame & frame, bool oob)
    {
      if (oob)
        return false;

      return RTP_Encoding::WriteData(frame, false);
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
      if (m_lastSentIFP.GetSize() == 0)
        redundantPackets.SetSize(0);
      else {
        redundantPackets.SetSize(1);
        T38_UDPTLPacket_error_recovery_secondary_ifp_packets_subtype & redundantPacket = redundantPackets[0];
        redundantPacket.SetValue(m_lastSentIFP, m_lastSentIFP.GetSize());
      }

      m_lastSentIFP = udptl.m_primary_ifp_packet;

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
      
      PTRACE(4, "T38_RTP\tRead UDPTL of size " << pduSize);

      PPER_Stream rawData(thisUDPTL, pduSize);

      // Decode the PDU
      if (!m_receivedPacket.Decode(rawData)) {
  #if PTRACING
        if (m_oneGoodPacket)
          PTRACE(2, "RTP_T38\tRaw data decode failure:\n  "
                 << setprecision(2) << rawData << "\n  UDPTL = "
                 << setprecision(2) << m_receivedPacket);
        else
          PTRACE(2, "RTP_T38\tRaw data decode failure: " << rawData.GetSize() << " bytes.");
  #endif

        m_consecutiveBadPackets++;
        if (m_consecutiveBadPackets < 100)
          return RTP_Session::e_IgnorePacket;

        PTRACE(1, "RTP_T38\tRaw data decode failed 100 times, remote probably not switched from audio, aborting!");
        return RTP_Session::e_AbortTransport;
      }

      PTRACE_IF(3, !m_oneGoodPacket, "T38_RTP\tFirst decoded UDPTL packet");
      m_oneGoodPacket = true;
      m_consecutiveBadPackets = 0;

      PTRACE(5, "T38_RTP\tDecoded UDPTL packet:\n  " << setprecision(2) << m_receivedPacket);

      int missing = m_receivedPacket.m_seq_number - m_expectedSequenceNumber;
      if (missing > 0 && m_receivedPacket.m_error_recovery.GetTag() == T38_UDPTLPacket_error_recovery::e_secondary_ifp_packets) {
        // Packets are missing and we have redundency in the UDPTL packets
        T38_UDPTLPacket_error_recovery_secondary_ifp_packets & secondaryPackets = m_receivedPacket.m_error_recovery;
        if (secondaryPackets.GetSize() > 0) {
          PTRACE(4, "T38_RTP\tUsing redundant data to reconstruct missing/out of order packet at SN=" << m_expectedSequenceNumber);
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
    bool            m_oneGoodPacket;
    PBYTEArray      m_lastSentIFP;
    T38_UDPTLPacket m_receivedPacket;
    unsigned        m_expectedSequenceNumber;
    int             m_secondaryPacket;
};


static PFactory<RTP_Encoding>::Worker<T38PseudoRTP_Handler> t38PseudoRTPHandler("udptl");


/////////////////////////////////////////////////////////////////////////////

OpalFaxEndPoint::OpalFaxEndPoint(OpalManager & mgr, const char * g711Prefix, const char * t38Prefix)
  : OpalEndPoint(mgr, g711Prefix, CanTerminateCall)
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
  if (!OpalMediaFormat(TIFF_File_FormatName).IsValid())
    return false;

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


/////////////////////////////////////////////////////////////////////////////

OpalFaxConnection::OpalFaxConnection(OpalCall        & call,
                                     OpalFaxEndPoint & ep,
                                     const PString   & filename,
                                     bool              receiving,
                                     bool              disableT38,
                                     OpalConnection::StringOptions * stringOptions)
  : OpalConnection(call, ep, ep.GetManager().GetNextToken('F'), 0, stringOptions)
  , m_endpoint(ep)
  , m_filename(filename)
  , m_receiving(receiving)
  , m_disableT38(disableT38)
  , m_releaseTimeout(0, 0, 1)
  , m_tiffFileFormat(TIFF_File_FormatName)
  , m_faxMode(false)
{
  m_tiffFileFormat.SetOptionString("TIFF-File-Name", filename);
  m_tiffFileFormat.SetOptionBoolean("Receiving", receiving);

  PTRACE(3, "FAX\tCreated FAX connection with token \"" << callToken << '"');
}


OpalFaxConnection::~OpalFaxConnection()
{
  PTRACE(3, "FAX\tDeleted FAX connection.");
}


PString OpalFaxConnection::GetPrefixName() const
{
  return m_disableT38 ? m_endpoint.GetPrefixName() : m_endpoint.GetT38Prefix();
}


void OpalFaxConnection::ApplyStringOptions(OpalConnection::StringOptions & stringOptions)
{
  m_stationId = stringOptions("stationid");
  OpalConnection::ApplyStringOptions(stringOptions);
}


OpalMediaFormatList OpalFaxConnection::GetMediaFormats() const
{
  OpalMediaFormatList formats;
  if (m_faxMode)
    formats += m_tiffFileFormat;
  else
    formats += OpalPCM16;
  return formats;
}


void OpalFaxConnection::AdjustMediaFormats(OpalMediaFormatList & mediaFormats) const
{
  // Remove everything but G.711 or fax stuff
  OpalMediaFormatList::iterator i = mediaFormats.begin();
  while (i != mediaFormats.end()) {
    if (*i == OpalG711_ULAW_64K || *i == OpalG711_ALAW_64K)
      ++i;
    else if (i->GetMediaType() != OpalMediaType::Fax())
      mediaFormats -= *i++;
    else {
      i->SetOptionString("TIFF-File-Name", m_filename);
      i->SetOptionBoolean("Receiving", m_receiving);
      ++i;
    }
  }

  OpalConnection::AdjustMediaFormats(mediaFormats);
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
  if (GetMediaStream(OpalMediaType::Audio(), true) == NULL)
    ownerCall.OpenSourceMediaStreams(*this, OpalMediaType::Audio());

  return true;
}


PBoolean OpalFaxConnection::SetAlerting(const PString & calleeName, PBoolean)
{
  PTRACE(3, "Fax\tSetAlerting(" << calleeName << ')');
  SetPhase(AlertingPhase);
  remotePartyName = calleeName;
  return true;
}


PBoolean OpalFaxConnection::SetConnected()
{
  if (GetMediaStream(OpalMediaType::Audio(), true) == NULL)
    ownerCall.OpenSourceMediaStreams(*this, OpalMediaType::Audio());
  return OpalConnection::SetConnected();
}


void OpalFaxConnection::OnEstablished()
{
  OpalConnection::OnEstablished();

  if (m_faxMode)
    return;

  if (m_disableT38)
    return;

  m_faxTimer.SetNotifier(PCREATE_NOTIFIER(OnSendCNGCED));
  m_faxTimer.SetInterval(0, 1);
  PTRACE(1, "T38\tStarting timer for CNG/CED tone");
}


void OpalFaxConnection::OnReleased()
{
  m_faxTimer.Stop(false);
  OpalConnection::OnReleased();
}


OpalMediaStream * OpalFaxConnection::CreateMediaStream(const OpalMediaFormat & mediaFormat, unsigned sessionID, bool isSource)
{
  return new OpalFaxMediaStream(*this, mediaFormat, sessionID, isSource);
}


void OpalFaxConnection::OnMediaPatchStop(unsigned sessionId, bool isSource)
{
  OpalMediaStreamPtr stream = GetMediaStream(sessionId, isSource);
  bool newMode = stream->GetMediaFormat().GetMediaType() != OpalMediaType::Fax();
  if (m_faxMode != newMode) {
    m_faxTimer.Stop();
    m_faxMode = newMode;

    // If was fax media, must have finished sending/receiving
    if (!m_faxMode) {
      synchronousOnRelease = false; // Get deadlock if OnRelease() from patch thread.
      OnFaxCompleted(false);
    }
  }

  OpalConnection::OnMediaPatchStop(sessionId, isSource);
}


PBoolean OpalFaxConnection::SendUserInputTone(char tone, unsigned /*duration*/)
{
  if (!m_faxMode && toupper(tone) == (m_receiving ? 'X' : 'Y'))
    RequestFax(true);
  return true;
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


void OpalFaxConnection::OnSendCNGCED(PTimer & timer, INT)
{
  if (LockReadOnly() && !m_faxMode) {
    PTimeInterval elapsed = PTime() - connectedTime;
    if (m_releaseTimeout > 0 && elapsed > m_releaseTimeout) {
      PTRACE(2, "T38\tDid not switch to T.38 mode, releasing connection");
      Release(OpalConnection::EndedByCapabilityExchange);
    }
    if (m_switchTimeout > 0 && elapsed > m_switchTimeout) {
      PTRACE(2, "T38\tDid not switch to T.38 mode, forcing switch");
      RequestFax(true);
    }
    else if (m_receiving) {
      // Cadence for CED is single tone, but we repeat just in case
      OnUserInputTone('Y', 3600);
      timer = 5000;
    }
    else {
      // Cadence for CNG is 500ms on 3 seconds off
      OnUserInputTone('X', 500);
      timer = 3500;
    }
    UnlockReadOnly();
  }
}


void OpalFaxConnection::OpenFaxStreams(PThread &, INT)
{
  if (!LockReadWrite())
    return;

  PSafePtr<OpalConnection> other = GetOtherPartyConnection();
  if (other == NULL || !other->SwitchFaxMediaStreams(m_faxMode)) {
    PTRACE(1, "T38\tMode change request to " << (m_faxMode ? "fax" : "audio") << " failed");
    OnFaxCompleted(true);
  }

  UnlockReadWrite();
}


void OpalFaxConnection::RequestFax(bool toFax)
{
#if PTRACING
  const char * modeStr = toFax ? "Fax/T.38" : "audio";
#endif

  if (toFax == m_faxMode) {
    PTRACE(1, "T38\tAlready in mode " << modeStr);
    return;
  }

  // definitely changing mode
  PTRACE(1, "T38\tRequesting mode change to " << modeStr);

  m_faxMode = toFax;

  PThread::Create(PCREATE_NOTIFIER(OpenFaxStreams));
}


#endif // OPAL_FAX

