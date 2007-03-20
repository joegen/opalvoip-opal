/*
 * h323h224.h
 *
 * H.323 H.224 logical channel establishment implementation for the 
 * OpenH323 Project.
 *
 * Copyright (c) 2006-2007 Network for Educational Technology, ETH Zurich.
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
 * $Log: h323h224.cxx,v $
 * Revision 1.6.4.2  2007/03/20 00:02:14  hfriederich
 * (Backport from HEAD)
 * Add ability to remove H.224
 *
 * Revision 1.6.4.1  2007/02/07 08:51:02  hfriederich
 * New branch with major revision of the core Opal media format handling system.
 *
 * - Session IDs have been replaced by new OpalMediaType class.
 * - The creation of H.245 TCS and SDP media descriptions have been extended
 *   to dynamically handle all available media types
 * - The H.224 code has been rewritten for better integration into the Opal
 *   system. It takes advantage of the new media type system and removes
 *   all hooks found in the core Opal classes.
 *
 * More work will follow as the current version breaks lots of important code.
 *
 * Revision 1.6  2006/08/21 05:29:25  csoutheren
 * Messy but relatively simple change to add support for secure (SSL/TLS) TCP transport
 * and secure H.323 signalling via the sh323 URL scheme
 *
 * Revision 1.5  2006/08/10 05:10:30  csoutheren
 * Various H.323 stability patches merged in from DeimosPrePLuginBranch
 *
 * Revision 1.4.2.1  2006/08/09 12:49:21  csoutheren
 * Improve stablity under heavy H.323 load
 *
 * Revision 1.4  2006/06/07 08:02:22  hfriederich
 * Fixing crashes when creating the RTP session failed
 *
 * Revision 1.3  2006/05/01 10:29:50  csoutheren
 * Added pragams for gcc < 4
 *
 * Revision 1.2  2006/04/24 12:53:50  rjongbloed
 * Port of H.224 Far End Camera Control to DevStudio/Windows
 *
 * Revision 1.1  2006/04/20 16:48:17  hfriederich
 * Initial version of H.224/H.281 implementation.
 *
 */

#include <ptlib.h>

#ifdef __GNUC__
#pragma implementation "h323h224.h"
#endif

#include <opal/buildopts.h>

#if OPAL_H224

#include <h224/h323h224.h>

#include <h323/h323ep.h>
#include <h323/h323con.h>
#include <h323/channels.h>
#include <h323/h323rtp.h>

#include <asn/h245.h>

OPAL_REGISTER_H224_CAPABILITY();

H323_H224Capability::H323_H224Capability()
: H323DataCapability(OpalH224.GetBandwidth() / 10)
{
  SetPayloadType(OpalH224.GetPayloadType());
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
  return "H.224";
}

H323Channel * H323_H224Capability::CreateChannel(H323Connection & connection,
                                                 H323Channel::Directions direction,
                                                 unsigned int sessionID,
                                                 const H245_H2250LogicalChannelParameters * params) const
{
  return connection.CreateRealTimeLogicalChannel(*this, direction, sessionID, params);
}

BOOL H323_H224Capability::OnSendingPDU(H245_DataApplicationCapability & pdu) const
{
  pdu.m_maxBitRate = maxBitRate;
  pdu.m_application.SetTag(H245_DataApplicationCapability_application::e_h224);
	
  H245_DataProtocolCapability & dataProtocolCapability = (H245_DataProtocolCapability &)pdu.m_application;
  dataProtocolCapability.SetTag(H245_DataProtocolCapability::e_hdlcFrameTunnelling);
	
  return TRUE;
}

BOOL H323_H224Capability::OnSendingPDU(H245_DataMode & pdu) const
{
  pdu.m_bitRate = maxBitRate;
  pdu.m_application.SetTag(H245_DataMode_application::e_h224);
	
  return TRUE;
}

BOOL H323_H224Capability::OnReceivedPDU(const H245_DataApplicationCapability & /*pdu*/)
{
  return TRUE;
}

#endif // OPAL_H224
