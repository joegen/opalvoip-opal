/*
 * h323h224.h
 *
 * H.323 H.224 logical channel establishment implementation for the
 * OpenH323 Project.
 *
 * Copyright (c) 2006 Network for Educational Technology, ETH Zurich.
 * Written by Hannes Friederich.
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
 * Contributor(s): ______________________________________.
 *
 */

#include <ptlib.h>

#include <opal_config.h>

#ifdef __GNUC__
#pragma implementation "h323h224.h"
#endif

#include <h224/h323h224.h>


#if OPAL_HAS_H281 && OPAL_H323

#include <h323/h323ep.h>
#include <h323/h323con.h>
#include <h323/channels.h>
#include <h323/h323rtp.h>

#include <asn/h245.h>


H323_H224_AnnexQCapability::H323_H224_AnnexQCapability(const OpalMediaFormat & mediaFormat)
  : H323GenericDataCapability("0.0.8.224.1.0", mediaFormat.GetMaxBandwidth())
{
  m_mediaFormat = mediaFormat;
}


PString H323_H224_AnnexQCapability::GetFormatName() const
{
  return m_mediaFormat.GetName();
}


H323Channel * H323_H224_AnnexQCapability::CreateChannel(H323Connection & connection,
                                                        H323Channel::Directions direction,
                                                        unsigned int sessionID,
                                                        const H245_H2250LogicalChannelParameters * params) const
{
  return connection.CreateRealTimeLogicalChannel(*this, direction, sessionID, params);
}


//////////////////////////////////////////////////////////////////////

H323_H224_HDLCTunnelingCapability::H323_H224_HDLCTunnelingCapability(const OpalMediaFormat & mediaFormat)
  : H323DataCapability(mediaFormat.GetMaxBandwidth())
{
  m_mediaFormat = mediaFormat;
}


unsigned H323_H224_HDLCTunnelingCapability::GetSubType() const
{
  return H245_DataApplicationCapability_application::e_h224;
}


PString H323_H224_HDLCTunnelingCapability::GetFormatName() const
{
  return m_mediaFormat.GetName();
}


H323Channel * H323_H224_HDLCTunnelingCapability::CreateChannel(H323Connection & connection,
                                                               H323Channel::Directions direction,
                                                               unsigned int sessionID,
                                                               const H245_H2250LogicalChannelParameters * params) const
{
  return connection.CreateRealTimeLogicalChannel(*this, direction, sessionID, params);
}


PBoolean H323_H224_HDLCTunnelingCapability::OnSendingPDU(H245_DataApplicationCapability & pdu) const
{
  m_maxBitRate.SetH245(pdu.m_maxBitRate);
  pdu.m_application.SetTag(H245_DataApplicationCapability_application::e_h224);

  H245_DataProtocolCapability & dataProtocolCapability = pdu.m_application;
  dataProtocolCapability.SetTag(H245_DataProtocolCapability::e_hdlcFrameTunnelling);

  return true;
}


PBoolean H323_H224_HDLCTunnelingCapability::OnSendingPDU(H245_DataMode & pdu) const
{
  m_maxBitRate.SetH245(pdu.m_bitRate);
  pdu.m_application.SetTag(H245_DataMode_application::e_h224);

  H245_DataProtocolCapability & dataProtocolCapability = pdu.m_application;
  dataProtocolCapability.SetTag(H245_DataProtocolCapability::e_hdlcFrameTunnelling);

  return true;
}


PBoolean H323_H224_HDLCTunnelingCapability::OnReceivedPDU(const H245_DataApplicationCapability & /*pdu*/)
{
  //m_maxBitRate.FromH245(pdu.m_bitRate);
  return true;
}


#endif // OPAL_HAS_H281 && OPAL_H323
