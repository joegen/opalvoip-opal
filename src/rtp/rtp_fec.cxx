/*
 * rtp_fec.cxx
 *
 * RTP protocol session Forward Error Correction
 *
 * Copyright (c) 2014 Vox Lucida Pty. Ltd.
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
 * The Original Code is Open Phone Abstraction Library
 *
 * The Initial Developer of the Original Code is Vox Lucida Pty. Ltd.
 *
 * Contributor(s): ______________________________________.
 *
 */

#include <ptlib.h>

#include <opal_config.h>

#if OPAL_RTP_FEC

#include <rtp/rtp_session.h>


#define PTraceModule() "RTP_FEC"


OpalRTPSession::SendReceiveStatus OpalRTPSession::SyncSource::OnSendRedundantFrame(RTP_DataFrame & frame)
{
  RTP_DataFrameList redundancies;
  OpalRTPSession::SendReceiveStatus status = OnSendRedundantData(frame, redundancies);
  if (status != e_ProcessPacket)
    return status;

  if (redundancies.empty()) {
    PTRACE(m_throttleTxRED, &m_session, m_session << "no redundant blocks added");
    return e_ProcessPacket;
  }

  RTP_DataFrame red;
  red.CopyHeader(frame);
  red.SetPayloadType(m_session.m_redundencyPayloadType);

  PINDEX redPayloadSize = 0;
  for (RTP_DataFrameList::iterator it = redundancies.begin(); it != redundancies.end(); ++it) {
    PINDEX size = it->GetPayloadSize();

    if (!red.SetPayloadSize(redPayloadSize + size + 4))
      return e_AbortTransport;

    BYTE * payload = red.GetPayloadPtr() + redPayloadSize;
    *payload++ = (BYTE)(it->GetPayloadType() | 0x80);
    *payload++ = (BYTE)(it->GetTimestamp() >> 6);
    *payload++ = (BYTE)(((it->GetTimestamp() & 0x3f) << 2) | (size >> 8));
    *payload++ = (BYTE)size;
    memcpy(payload, it->GetPayloadPtr(), size);

    redPayloadSize = red.GetPayloadSize();
  }

  // Set the primary encoding block
  red.SetPayloadSize(redPayloadSize + frame.GetPayloadSize() + 1);
  BYTE * payload = red.GetPayloadPtr() + redPayloadSize;
  *payload++ = (BYTE)frame.GetPayloadType();
  memmove(payload, frame.GetPayloadPtr(), frame.GetPayloadSize());

  PTRACE(m_throttleTxRED, &m_session, m_session << "redundant packet " << red.GetPayloadType()
         << " primary block inserted : " << frame.GetPayloadType() << ", sz=" << frame.GetPayloadSize());
  frame = red;
  return e_ProcessPacket;
}


OpalRTPSession::SendReceiveStatus OpalRTPSession::SyncSource::OnSendRedundantData(RTP_DataFrame & primary, RTP_DataFrameList & redundancies)
{
  if (m_session.m_ulpFecPayloadType == RTP_DataFrame::IllegalPayloadType)
    return e_ProcessPacket; // No redundancies, add primary data and return

  FecData fec;
  fec.m_level.resize(m_session.m_ulpFecSendLevel);

  switch (OnSendFEC(primary, fec)) {
    case e_AbortTransport :
      return e_AbortTransport;
    case e_IgnorePacket :
      return e_ProcessPacket; // No redundancies, add primary data and return
    case e_ProcessPacket :
      break;
  }

  PINDEX size = 10;
  size_t maskSize = 0;
  for (vector<FecLevel>::iterator it = fec.m_level.begin(); it != fec.m_level.end(); ++it) {
    if (!PAssert(!it->m_data.empty(), PLogicError))
      return e_ProcessPacket; // Invalid redundancy, add primary data and return

    if (maskSize == 0) {
      maskSize = it->m_mask.size();
      if (!PAssert(maskSize == 2 || maskSize == 6, PLogicError))
        return e_ProcessPacket; // Invalid redundancy, add primary data and return
    }
    else {
      if (!PAssert(maskSize == it->m_mask.size(), PLogicError))
        return e_ProcessPacket; // Invalid redundancy, add primary data and return
    }

    size += it->m_data.size() + maskSize + 2;
  }

  RTP_DataFrame * red = new RTP_DataFrame(size);
  redundancies.Append(red);
  red->CopyHeader(primary);
  red->SetPayloadType(m_session.m_ulpFecPayloadType);

  BYTE * data = red->GetPayloadPtr();
  if (maskSize == 6)
    *data |= 0x40;
  if (fec.m_pRecovery)
    *data |= 0x20;
  if (fec.m_xRecovery)
    *data |= 0x10;
  *data |= (BYTE)(fec.m_ccRecovery&0xf);
  ++data;
  if (fec.m_mRecovery)
    *data |= 0x80;
  *data |= (BYTE)(fec.m_ptRecovery&0x7f);
  ++data;
  *(PUInt16b *)data = (uint16_t)fec.m_snBase;
  data += 2;
  *(PUInt32b *)data = fec.m_tsRecovery;
  data += 4;
  *(PUInt16b *)data = (uint16_t)fec.m_lenRecovery;
  data += 2;

  for (vector<FecLevel>::iterator it = fec.m_level.begin(); it != fec.m_level.end(); ++it) {
    *(PUInt16b *)data = (uint16_t)it->m_data.size();
    data += 2;
    memcpy(data, it->m_mask, maskSize);
    data += maskSize;
    memcpy(data, it->m_data, it->m_data.size());
    data += it->m_data.size();
  }

  PTRACE(5, &m_session, m_session << "adding (eventually) redundant ULP-FEC");
  return e_ProcessPacket;
}


OpalRTPSession::SendReceiveStatus OpalRTPSession::SyncSource::OnSendFEC(RTP_DataFrame & /*primary*/, FecData & /*fec*/)
{
  return e_IgnorePacket;
}


OpalRTPSession::SendReceiveStatus OpalRTPSession::SyncSource::OnReceiveRedundantFrame(RTP_DataFrame & frame)
{
  const BYTE * payload = frame.GetPayloadPtr();
  PINDEX size = frame.GetPayloadSize();

  // Skip over the redundant blocks as want primary block
  while (size >= 4 && (*payload & 0x80) != 0) {
    PINDEX len = (((payload[2] & 3) << 8) | payload[3]) + 4;
    if (len >= size) {
      PTRACE(2, &m_session, m_session << "redundant packet too small: " << size << " bytes, expecting " << len << '\n' << frame);
      return e_IgnorePacket;
    }
    payload += len;
    size -= len;
  }

  if (size <= 0) {
    PTRACE(2, &m_session, m_session << "redundant packet primary block missing:\n" << frame);
    return e_IgnorePacket;
  }

  // Get the primary encoding block
  RTP_DataFrame primary;
  primary.CopyHeader(frame);
  primary.SetPayloadType((RTP_DataFrame::PayloadTypes)(*payload & 0x7f));
  primary.SetPayloadSize(--size);
  memmove(primary.GetPayloadPtr(), ++payload, size);
  PTRACE(m_throttleRxRED, &m_session, m_session << "redundant packet " << frame.GetPayloadType()
         << " primary block extracted: " << primary.GetPayloadType() << ", sz=" << size);

  // Then go through the redundant entries again
  payload = frame.GetPayloadPtr();
  size = frame.GetPayloadSize();
  while (size >= 4 && (*payload & 0x80) != 0) {
    PINDEX len = (((payload[2] & 3) << 8) | payload[3]) + 4;
    switch (OnReceiveRedundantData(primary, (RTP_DataFrame::PayloadTypes)(payload[0] & 0x7f),
                                   frame.GetTimestamp() - (payload[1] << 6) - (payload[2] >> 2),
                                   payload + 4, len - 4)) {
      case e_AbortTransport :
        return e_AbortTransport;
      case e_IgnorePacket :
        size = len; // Force exit of loop, ignore other redundancies
      case e_ProcessPacket :
        break;
    }
    payload += len;
    size -= len;
  }

  frame = primary;
  return e_ProcessPacket;
}


OpalRTPSession::SendReceiveStatus OpalRTPSession::SyncSource::OnReceiveRedundantData(RTP_DataFrame & primary,
                                                                         RTP_DataFrame::PayloadTypes payloadType,
                                                                         unsigned timestamp,
                                                                         const BYTE * data,
                                                                         PINDEX size)
{
  if (payloadType != m_session.m_ulpFecPayloadType) {
    PTRACE(m_throttleRxUnknownFEC, &m_session, m_session << "unknown redundant block: "
           << payloadType << ", ts=" << timestamp << ", sz=" << size << '\n' << hex << setfill('0') << PBYTEArray(data-4, size+4, false));
    return e_ProcessPacket;
  }

  if (size <= 16) {
    PTRACE(2, &m_session, m_session << "redundant ULP-FEC too small: " << size << " bytes");
    return e_IgnorePacket; // This is abort processing redundant data and just return the primary frame
  }

  FecData fec;
  fec.m_timestamp = timestamp;
  bool maskSize = (*data & 0x40) != 0 ? 6 : 2;
  fec.m_pRecovery = (*data & 0x20) != 0;
  fec.m_xRecovery = (*data & 0x10) != 0;
  fec.m_ccRecovery = (*data & 0xf);
  ++data;
  fec.m_mRecovery = (*data & 0x80) != 0;
  fec.m_ptRecovery = (*data & 0x7f);
  ++data;
  fec.m_snBase = *(PUInt16b *)data;
  data += 2;
  fec.m_tsRecovery = *(PUInt32b *)data;
  data += 4;
  fec.m_lenRecovery = *(PUInt16b *)data;
  data += 2;
  size -= 10;

  PINDEX hdrLen = 2 + maskSize;
  while (size >= hdrLen) {
    FecLevel level;
    level.m_mask = PBYTEArray(data+2, maskSize, false);
    level.m_data = PBYTEArray(data+hdrLen, *(PUInt16b *)data, false);
    fec.m_level.push_back(level);

    data += hdrLen;
    size -= hdrLen;
  }

  PTRACE(5, &m_session, m_session << "redundant ULP-FEC:"
            " ts=" << timestamp << ", levels=" << fec.m_level.size());
  return OnReceiveFEC(primary, fec);
}


OpalRTPSession::SendReceiveStatus OpalRTPSession::SyncSource::OnReceiveFEC(RTP_DataFrame & /*primary*/, const FecData & /*fec*/)
{
  /* If there is something in m_pendingPackets, then there was a missing
     packet(s). From the primary data packet and the FEC info, it should be
     possible to calculate the missing packet and insert it into the correct
     position of m_pendingPackets. It will then be fed out as though it was
     just an out of order packet.
   */

  return e_ProcessPacket;
}

#endif // OPAL_RTP_FEC


/////////////////////////////////////////////////////////////////////////////
