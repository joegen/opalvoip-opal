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

#ifndef __OPAL_SDPCAPS_H
#define __OPAL_SDPCAPS_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif

#include <ptlib.h>
#include <sip/sdp.h>


/////////////////////////////////////////////////////////

/**This class encapsulates the translation between media formats and the
   SDP representation and vice versa.
   It is guaranteed that UpdateMediaFormat() is called before a call to
   either OnSendingSDP() or OnReceivedSDP(). However, OnReceivedSDP() may
   be called multiple times in case of Re-invites, so SDP capabilities should
   store the local settings somehow and apply correct merge rules every time
   OnReceivedSDP() is called.
  */
class SDPCapability : public PObject
{
  PCLASSINFO(SDPCapability, PObject);
    
public:
    
  SDPCapability();
  SDPCapability(const SDPCapability & capability);
    
  /**Creates a new instance of an SDPCapability for the given media format.
     This function uses the SDPCapability factory to look for custom capabilities.
     If no custom capability is found, a default SDPCapability instance is returned.
    */
  static SDPCapability * CreateCapability(const OpalMediaFormat & mediaFormat);
  
  virtual PObject * Clone() const;
  virtual BOOL OnSendingSDP(SDPMediaFormat & sdpMediaFormat) const { return TRUE; }
  virtual BOOL OnReceivedSDP(const SDPMediaFormat & sdpMediaFormat,
                             const SDPMediaDescription & mediaDescription,
                             const SDPSessionDescription & sessionDescription) { return TRUE; }
    
  virtual void UpdateMediaFormat(const OpalMediaFormat & _mediaFormat) { mediaFormat = _mediaFormat; }
  virtual const OpalMediaFormat & GetMediaFormat() const { return mediaFormat; }
  
protected:
  OpalMediaFormat mediaFormat;
};

/////////////////////////////////////////////////////////


class RFC2833_SDPCapability : public SDPCapability
{
  PCLASSINFO(RFC2833_SDPCapability, SDPCapability);
public:

  virtual PObject * Clone() const;
  virtual BOOL OnSendingSDP(SDPMediaFormat & sdpMediaFormat) const;
  virtual BOOL OnReceivedSDP(const SDPMediaFormat & sdpMediaFormat,
                             const SDPMediaDescription & sdpMediaDescription,
                             const SDPSessionDescription & sessionDescription);
};

/////////////////////////////////////////////////////////

PLIST(SDPCapabilityBaseList, SDPCapability);

class SDPCapabilityList : public SDPCapabilityBaseList
{
  PCLASSINFO(SDPCapabilityList, SDPCapabilityBaseList);
    
public:
    
  BOOL HasCapability(const OpalMediaFormat & mediaFormat) const { return FindCapability(mediaFormat) != NULL; }
  SDPCapability * FindCapability(const OpalMediaFormat & mediaFormat) const;
  void AddCapabilityWithFormat(const OpalMediaFormat & mediaFormat);
};

/////////////////////////////////////////////////////////

/* SDP capability registration macros based on abstract factories
 */
typedef PFactory<SDPCapability, std::string> SDPCapabilityFactory;

#define SDP_REGISTER_CAPABILITY(cls, mediaFormat) static SDPCapabilityFactory::Worker<cls> cls##Factory(mediaFormat.GetEncodingName(), true)


#endif // __OPAL_SDPCAPS_H


// End of File ///////////////////////////////////////////////////////////////
