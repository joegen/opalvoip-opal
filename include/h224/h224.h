/*
 * h224.h
 *
 * H.224 PDU implementation for the OpenH323 Project.
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
 * $Log: h224.h,v $
 * Revision 1.2.6.1  2007/02/07 08:51:00  hfriederich
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
 * Revision 1.2  2006/04/23 18:52:19  dsandras
 * Removed warnings when compiling with gcc on Linux.
 *
 * Revision 1.1  2006/04/20 16:48:17  hfriederich
 * Initial version of H.224/H.281 implementation.
 *
 */

#ifndef __OPAL_H224_H
#define __OPAL_H224_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif

#include <ptlib.h>
#include <opal/mediafmt.h>
#include <opal/transcoders.h>
#include <h224/h224mediafmt.h>
#include <h224/q922.h>

#define H224_BROADCAST 0x0000

#define EXTENDED_CLIENT_ID 0x7e
#define NON_STANDARD_CLIENT_ID 0x7f

#define COUNTRY_CODE_ESCAPE 0xff

class OpalH224Client;

class H224_Frame : public Q922_Frame
{
  PCLASSINFO(H224_Frame, Q922_Frame);
	
public:

  H224_Frame(PINDEX clientDataSize = 254);
  H224_Frame(const OpalH224Client & h224Client, PINDEX clientDataSize = 254);
  ~H224_Frame();
	
  BOOL IsHighPriority() const { return (GetLowOrderAddressOctet() == 0x71); }
  void SetHighPriority(BOOL flag);
	
  WORD GetDestinationTerminalAddress() const;
  void SetDestinationTerminalAddress(WORD destination);
	
  WORD GetSourceTerminalAddress() const;
  void SetSourceTerminalAddress(WORD source);
  
  /**Convenience function to set the H.224 header values */
  void SetClient(const OpalH224Client & h224Client);
	
  BYTE GetClientID() const;
  void SetClientID(BYTE clientID);
  
  /**Returns 0 in case clientID isn't set to EXTENDED_CLIENT_ID */
  BYTE GetExtendedClientID() const;
  /**Does nothing in case clientID isn't set to EXTENDED_CLIENT_ID */
  void SetExtendedClientID(BYTE extendedClientID);
  
  /**Returns 0 in case clientID isn't set to NON_STANDARD_CLIENT_ID */
  BYTE GetCountryCode() const;
  BYTE GetCountryCodeExtension() const;
  WORD GetManufacturerCode() const;
  BYTE GetManufacturerClientID() const;
  
  /**Does nothing in case clientID isn't set to NON_STANDARD_CLIENT_ID */
  void SetNonStandardClientInformation(BYTE countryCode,
                                       BYTE countryCodeExtension,
                                       WORD manufacturerCode,
                                       BYTE manufacturerClientID);

  /**Note: The following methods depend on the value of clientID as to where put the value.
	 Always set clientID first before altering these values */
  BOOL GetBS() const;
  void SetBS(BOOL bs);
	
  BOOL GetES() const;
  void SetES(BOOL es);
	
  BOOL GetC1() const;
  void SetC1(BOOL c1);
	
  BOOL GetC0() const;
  void SetC0(BOOL c0);
	
  BYTE GetSegmentNumber() const;
  void SetSegmentNumber(BYTE segmentNumber);
	
  BYTE *GetClientDataPtr() const;
	
  PINDEX GetClientDataSize() const;
  void SetClientDataSize(PINDEX size);
	
  BOOL Decode(const BYTE *data, PINDEX size);
  
private:
	  
  PINDEX GetHeaderSize() const;
};

#endif // __OPAL_H224_H

