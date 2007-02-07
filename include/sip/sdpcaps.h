/*
 * sdpcaps.h
 *
 * Session Description Protocol Capability System
 *
 * Open Phone Abstraction Library (OPAL)
 * Formally known as the Open H323 project.
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
 * $Log: sdpcaps.h,v $
 * Revision 1.1.2.1  2007/02/07 09:31:05  hfriederich
 * Add new SDPCapability class to ease translation between FMTP and media
 * format representation.
 *
 */

#ifndef __OPAL_SDPCAPS_H
#define __OPAL_SDPCAPS_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif

#include <ptlib.h>
#include <sip/sdp.h>


/////////////////////////////////////////////////////////


class SDPCapability : public PObject
{
    PCLASSINFO(SDPCapability, PObject);
public:
    
    SDPCapability();
    
    static const SDPCapability & GetCapability(const OpalMediaFormat & mediaFormat);
    
    virtual BOOL OnSendingSDP(const OpalMediaFormat & mediaFormat, SDPMediaFormat & sdpMediaFormat) const;
    virtual BOOL OnReceivingSDP(OpalMediaFormat & mediaFormat, const SDPMediaFormat & sdpMediaFormat) const;
};


/////////////////////////////////////////////////////////


class RFC2833_SDPCapability : public SDPCapability
{
    PCLASSINFO(RFC2833_SDPCapability, SDPCapability);
public:

    virtual BOOL OnSendingSDP(const OpalMediaFormat & mediaFormat, SDPMediaFormat & sdpMediaFormat) const;
    virtual BOOL OnReceivingSDP(OpalMediaFormat & mediaFormat, const SDPMediaFormat & sdpMediaFormat) const;
};

/////////////////////////////////////////////////////////

/* SDP capability registration macros based on absstract factories
 */
typedef PFactory<SDPCapability, std::string> SDPCapabilityFactory;

#define SDP_REGISTER_CAPABILITY(cls, mediaFormat) static SDPCapabilityFactory::Worker<cls> cls##Factory(mediaFormat.GetEncodingName(), true)


#endif // __OPAL_SDPCAPS_H


// End of File ///////////////////////////////////////////////////////////////
