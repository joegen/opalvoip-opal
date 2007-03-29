/*
 * h224handler.h
 *
 * H.224 protocol handler implementation for the OpenH323 Project.
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
 * $Log: h224handler.h,v $
 * Revision 1.2.6.2  2007/03/29 22:14:58  hfriederich
 * Pass OpalConnection to OpalMediaStream constructor
 * Add ID to OpalMediaStreams so that transcoders can match incoming and
 *   outgoing codecs
 *
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

#ifndef __OPAL_H224HANDLER_H
#define __OPAL_H224HANDLER_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif

#include <ptlib.h>
#include <opal/connection.h>
#include <opal/transports.h>
#include <opal/mediastrm.h>
#include <rtp/rtp.h>
#include <h224/h224.h>

#define H281_CLIENT_ID 0x01

class OpalH224Handler;

class OpalH224Client : public PObject
{
  PCLASSINFO(OpalH224Client, PObject);
	
public:
	
  OpalH224Client();
  ~OpalH224Client();
	
  /**Return the client ID if this is a standard client.
     Else, return either EXTENDED_CLIENT_ID or
     NON_STANDARD_CLIENT_ID
    */
  virtual BYTE GetClientID() const = 0;
	
  /**Return the extended client ID if given. The default
     returns 0x00
    */
  virtual BYTE GetExtendedClientID() const { return 0x00; }
	
  /**Return the T.35 country code octet for the non-standard client.
     Default returns COUNTRY_CODE_ESCAPE
    */
  virtual BYTE GetCountryCode() const { return COUNTRY_CODE_ESCAPE; }
	
  /**Return the T.35 extension code octet for the non-standard client.
     Default returns 0x00
    */
  virtual BYTE GetCountryCodeExtension() const { return 0x00; }
	
  /**Return the manufacturer code word for the non-standard client.
     Default returns 0x0000
    */
  virtual WORD GetManufacturerCode() const { return 0x0000; }
	
  /**Return the Manufacturer Client ID for the non-standard client.
     Default returns 0x00;
    */
  virtual BYTE GetManufacturerClientID() const { return 0x00; }
	
  /**Return whether this client has extra capabilities.
     Default returns FALSE.
    */
  virtual BOOL HasExtraCapabilities() const { return FALSE; }
	
  /**Called if the CME client received an Extra Capabilities PDU for this client.
     Default does nothing.
    */
  virtual void OnReceivedExtraCapabilities(const BYTE *capabilities, PINDEX size) {}
  
  /**Called if a PDU for this client was received.
     Default does nothing.
    */
  virtual void OnReceivedMessage(const H224_Frame & message) {};
	
  /**Called to indicate that the extra capabilities pdu should be sent.
     Default does nothing
    */
  virtual void SendExtraCapabilities() const {};
	
  virtual Comparison Compare(const PObject & obj);
	
  /**Connection to the H.224 protocol handler */
  void SetH224Handler(OpalH224Handler * handler) { h224Handler = handler; };
	
  /**Called by the H.224 handler to indicate if the remote party has such a client or not */
  void SetRemoteClientAvailable(BOOL remoteClientAvailable, BOOL remoteClientHasExtraCapabilities);
	
  BOOL GetRemoteClientAvailable() const { return remoteClientAvailable; };
  BOOL GetRemoteClientHasExtraCapabilities() const { return remoteClientHasExtraCapabilities; };
	
protected:
	
  BOOL remoteClientAvailable;
  BOOL remoteClientHasExtraCapabilities;
  OpalH224Handler * h224Handler;
	
};

PSORTED_LIST(OpalH224ClientList, OpalH224Client);

class OpalH224MediaStream;

class OpalH224Handler : public PObject
{
  PCLASSINFO(OpalH224Handler, PObject);
	
public:
	
  OpalH224Handler();
  ~OpalH224Handler();
  
  /**Adds / removes the client from the client list */
  BOOL AddClient(OpalH224Client & client);
  BOOL RemoveClient(OpalH224Client & client);
  
  /**Sets / Unsets the transmit H224 Media Stream */
  void SetTransmitMediaStream(OpalH224MediaStream * transmitMediaStream);
	
  virtual void StartTransmit();
  virtual void StopTransmit();

  /**Sends the complete client list with all clients registered so far
    */
  BOOL SendClientList();
  
  /**Sends the extra capabilities for all clients that indicate to have
     extra capabilities.
    */
  BOOL SendExtraCapabilities();
  
  /**Requests the remote side to send it's client list */
  BOOL SendClientListCommand();
  
  /**Request the remote side to send the extra capabilities for the given client */
  BOOL SendExtraCapabilitiesCommand(const OpalH224Client & client);

  /**Callback for H.224 clients to send their extra capabilities */
  BOOL SendExtraCapabilitiesMessage(const OpalH224Client & client, BYTE *data, PINDEX length);

  /**Callback for H.224 clients to send a client frame */
  BOOL TransmitClientFrame(const OpalH224Client & client, H224_Frame & frame);
	
  BOOL HandleFrame(const RTP_DataFrame & rtpFrame);
  virtual BOOL OnReceivedFrame(H224_Frame & frame);
  virtual BOOL OnReceivedCMEMessage(H224_Frame & frame);
  virtual BOOL OnReceivedClientList(H224_Frame & frame);
  virtual BOOL OnReceivedClientListCommand();
  virtual BOOL OnReceivedExtraCapabilities(H224_Frame & frame);
  virtual BOOL OnReceivedExtraCapabilitiesCommand();
  
  /**Public to allow clients to use this mutex for synchronization as well */
  PMutex & GetTransmitMutex() { return transmitMutex; }
	
protected:

  PMutex transmitMutex;
  BOOL canTransmit;
  RTP_DataFrame transmitFrame;
  BYTE transmitBitIndex;
  PTime *transmitStartTime;
  OpalH224MediaStream * transmitMediaStream;
  
  H224_Frame receiveFrame;
  
  OpalH224ClientList clients;
	
private:
	  
  void TransmitFrame(H224_Frame & frame);
	
};

class OpalH224MediaStream : public OpalMediaStream
{
  PCLASSINFO(OpalH224MediaStream, OpalMediaStream);
    
public:
	
  OpalH224MediaStream(OpalConnection & connection,
                      OpalH224Handler & h224Handler,
                      const OpalMediaFormat & mediaFormat,
                      BOOL isSource);
  ~OpalH224MediaStream();
	
  virtual void OnPatchStart();
  virtual BOOL Close();
  virtual BOOL ReadPacket(RTP_DataFrame & packet);
  virtual BOOL WritePacket(RTP_DataFrame & packet);
  virtual BOOL IsSynchronous() const { return FALSE; }
  virtual BOOL RequiresPatchThread() const { return FALSE; }
	
private:
  OpalH224Handler & h224Handler;
};

#endif // __OPAL_H224HANDLER_H

