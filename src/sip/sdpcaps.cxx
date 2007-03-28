/*
 * sdp.cxx
 *
 * Session Description Protocol Capability System implementation.
 *
 * Open Phone Abstraction Library (OPAL)
 *
 * Copyright (c) 2007 Network for Educational Technology, ETH Zurich.
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
 * The Original Code is Open Phone Abstraction Library.
 *
 * The Initial Developer of the Original Code is Equivalence Pty. Ltd.
 *
 * Contributor(s): ______________________________________.
 *
 * $Log: sdpcaps.cxx,v $
 * Revision 1.1.2.3  2007/03/28 06:00:13  hfriederich
 * Fix incorrect memory management when using factory
 *
 * Revision 1.1.2.2  2007/02/16 10:43:41  hfriederich
 * - Extend SDP capability system for merging local / remote format parameters.
 * - Propagate media format options to the media streams
 *
 * Revision 1.1.2.1  2007/02/07 09:31:05  hfriederich
 * Add new SDPCapability class to ease translation between FMTP and media
 * format representation.
 *
 */

#include <ptlib.h>

#ifdef __GNUC__
#pragma implementation "sdpcaps.h"
#endif

#include <sip/sdpcaps.h>

SDP_REGISTER_CAPABILITY(RFC2833_SDPCapability, OpalRFC2833);

/////////////////////////////////////////////////////////

SDPCapability::SDPCapability()
{
}


SDPCapability::SDPCapability(const SDPCapability & cap)
: mediaFormat(cap.mediaFormat)
{
}


SDPCapability * SDPCapability::CreateCapability(const OpalMediaFormat & mediaFormat)
{
    
  SDPCapability * cap = SDPCapabilityFactory::CreateInstance(mediaFormat.GetEncodingName());
  if(cap == NULL) {
    cap = new SDPCapability();
  } else {
    cap = (SDPCapability *)cap->Clone();
  }
  cap->UpdateMediaFormat(mediaFormat);
  return cap;
}


PObject * SDPCapability::Clone() const
{
  return new SDPCapability(*this);
}


/////////////////////////////////////////////////////////

PObject * RFC2833_SDPCapability::Clone() const
{
  return new RFC2833_SDPCapability(*this);
}


BOOL RFC2833_SDPCapability::OnSendingSDP(SDPMediaFormat & sdpMediaFormat) const
{
  sdpMediaFormat.SetFMTP("0-15");
  return TRUE;
}


BOOL RFC2833_SDPCapability::OnReceivedSDP(const SDPMediaFormat & sdpMediaFormat,
                                          const SDPMediaDescription & mediaDescription,
                                          const SDPSessionDescription & sessionDescription)
{
  return TRUE;
}

/////////////////////////////////////////////////////////

SDPCapability * SDPCapabilityList::FindCapability(const OpalMediaFormat & mediaFormat) const
{
  for (PINDEX i = 0; i < GetSize(); i++) {
    SDPCapability & capability = (*this)[i];
    if (capability.GetMediaFormat() == mediaFormat) {
      return &capability;
    }
  }
  return NULL;
}

void SDPCapabilityList::AddCapabilityWithFormat(const OpalMediaFormat & mediaFormat)
{
  if (HasCapability(mediaFormat)) {
    return;
  }
    
  SDPCapability * capability = SDPCapability::CreateCapability(mediaFormat);
  Append(capability);
}

// End of file ////////////////////////////////////////////////////////////////
