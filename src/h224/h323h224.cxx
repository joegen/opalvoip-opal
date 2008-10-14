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
 * $Revision$
 * $Author$
 * $Date$
 */

#include <ptlib.h>

#include <opal/buildopts.h>

#ifdef __GNUC__
#pragma implementation "h323h224.h"
#endif

#if OPAL_H224FECC

#include <h224/h323h224.h>

#include <h323/h323ep.h>
#include <h323/h323con.h>
#include <h323/channels.h>
#include <h323/h323rtp.h>

#include <asn/h245.h>

//////////////////////////////////////////////////////////////////////

H323_H224Capability::H323_H224Capability()
: H323DataCapability(640)
{
  SetPayloadType((RTP_DataFrame::PayloadTypes)100);
}

H323_H224Capability::~H323_H224Capability()
{
}

PObject::Comparison H323_H224Capability::Compare(const PObject & obj) const
{
  Comparison result = H323DataCapability::Compare(obj);

  if(result != EqualTo)	{
    return result;
  }
	
  PAssert(PIsDescendant(&obj, H323_H224Capability), PInvalidCast);
	
  return EqualTo;
}

PObject * H323_H224Capability::Clone() const
{
  return new H323_H224Capability(*this);
}

unsigned H323_H224Capability::GetSubType() const
{
  return H245_DataApplicationCapability_application::e_h224;
}

PString H323_H224Capability::GetFormatName() const
{
  return OPAL_H224;
}

H323Channel * H323_H224Capability::CreateChannel(H323Connection & connection,
                                                 H323Channel::Directions direction,
                                                 unsigned int sessionID,
                                                 const H245_H2250LogicalChannelParameters * params) const
{
  return connection.CreateRealTimeLogicalChannel(*this, direction, sessionID, params);
}

PBoolean H323_H224Capability::OnSendingPDU(H245_DataApplicationCapability & pdu) const
{
  pdu.m_maxBitRate = maxBitRate;
  pdu.m_application.SetTag(H245_DataApplicationCapability_application::e_h224);
	
  H245_DataProtocolCapability & dataProtocolCapability = (H245_DataProtocolCapability &)pdu.m_application;
  dataProtocolCapability.SetTag(H245_DataProtocolCapability::e_hdlcFrameTunnelling);
	
  return PTrue;
}

PBoolean H323_H224Capability::OnSendingPDU(H245_DataMode & pdu) const
{
  pdu.m_bitRate = maxBitRate;
  pdu.m_application.SetTag(H245_DataMode_application::e_h224);
	
  return PTrue;
}

PBoolean H323_H224Capability::OnReceivedPDU(const H245_DataApplicationCapability & /*pdu*/)
{
  return PTrue;
}

#endif // OPAL_H224FECC

