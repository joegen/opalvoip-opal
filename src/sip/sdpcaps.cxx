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


const SDPCapability & SDPCapability::GetCapability(const OpalMediaFormat & mediaFormat)
{
    static SDPCapability defaultCapability;
    SDPCapability * cap = SDPCapabilityFactory::CreateInstance(mediaFormat.GetEncodingName());
    if(cap == NULL) {
        return defaultCapability;
    }
    return *cap;
}


BOOL SDPCapability::OnSendingSDP(const OpalMediaFormat & mediaFormat, SDPMediaFormat & sdpMediaFormat) const
{
    return TRUE;
}


BOOL SDPCapability::OnReceivingSDP(OpalMediaFormat & mediaFormat, const SDPMediaFormat & sdpMediaFormat) const
{
    return TRUE;
}


/////////////////////////////////////////////////////////

BOOL RFC2833_SDPCapability::OnSendingSDP(const OpalMediaFormat & mediaFormat, SDPMediaFormat & sdpMediaFormat) const
{
    sdpMediaFormat.SetFMTP("0-15");
    return TRUE;
}


BOOL RFC2833_SDPCapability::OnReceivingSDP(OpalMediaFormat & mediaFormat, const SDPMediaFormat & sdpMediaFormat) const
{
    return TRUE;
}

// End of file ////////////////////////////////////////////////////////////////
