/*
 * h224handler.h
 *
 * H.224 protocol handler implementation for the OpenH323 Project.
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
 */

#ifndef OPAL_H224_H224HANDLER_H
#define OPAL_H224_H224HANDLER_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif

#include <opal_config.h>

#if OPAL_HAS_H224

#include <opal/connection.h>
#include <opal/transports.h>
#include <opal/mediastrm.h>
#include <rtp/rtp.h>
#include <h224/h224.h>

class OpalH224Handler;

class OpalH224Client : public PObject
{
  PCLASSINFO(OpalH224Client, PObject);

public:

  OpalH224Client();
  ~OpalH224Client();

  enum {
    CMEClientID         = 0x00,
    H281ClientID        = 0x01,
    ExtendedClientID    = 0x7e,
    NonStandardClientID = 0x7f,
  };

  /**Return the client ID if this is a standard client.
     Else, return either ExtendedClientId or NonStandardClientID
    */
  virtual BYTE GetClientID() const = 0;

  /**Return the extended client ID if given. The default returns 0x00
    */
  virtual BYTE GetExtendedClientID() const { return 0x00; }

  /**Return the T.35 country code octet for the non-standard client.
     Default returns CountryCodeEscape
    */
  virtual BYTE GetCountryCode() const { return 0xff; /* CountryCodeEscape */ }

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
  virtual bool HasExtraCapabilities() const { return false; }

  /**Called if the CME client received an Extra Capabilities PDU for this client.
     Default does nothing.
    */
  virtual void OnReceivedExtraCapabilities(const BYTE * /*capabilities*/, PINDEX /*size*/) { }

  /**Called if a PDU for this client was received.
     Default does nothing.
    */
  virtual void OnReceivedMessage(const H224_Frame & /*message*/) { }

  /**Called to indicate that the extra capabilities pdu should be sent.
     Default does nothing
    */
  virtual void SendExtraCapabilities() const { }

  virtual Comparison Compare(const PObject & obj);

  /**Connection to the H.224 protocol handler */
  void SetH224Handler(OpalH224Handler * handler) { m_h224Handler = handler; }

  /**Called by the H.224 handler to indicate if the remote party has such a client or not */
  void SetRemoteClientAvailable(bool remoteClientAvailable, bool remoteClientHasExtraCapabilities);

  bool GetRemoteClientAvailable() const { return m_remoteClientAvailable; }
  bool GetRemoteClientHasExtraCapabilities() const { return m_remoteClientHasExtraCapabilities; }

protected:
  bool              m_remoteClientAvailable;
  bool              m_remoteClientHasExtraCapabilities;
  OpalH224Handler * m_h224Handler;
};

PSORTED_LIST(OpalH224ClientList, OpalH224Client);


///////////////////////////////////////////////////////////////////////////////

class OpalH224MediaStream;

class OpalH224Handler : public PObject
{
  PCLASSINFO(OpalH224Handler, PObject);

public:

  OpalH224Handler();
  ~OpalH224Handler();

  enum {
    Broadcast = 0x0000,

    CMEClientListCode        = 0x01,
    CMEExtraCapabilitiesCode = 0x02,
    CMEMessage               = 0x00,
    CMECommand               = 0xff,

    CountryCodeEscape   = 0xff,
  };

  /**Adds / removes the client from the client list */
  bool AddClient(OpalH224Client & client);
  bool RemoveClient(OpalH224Client & client);

  /** Sets the transmit H224 media stream */
  void SetTransmitMediaStream(OpalH224MediaStream * transmitMediaStream);

  virtual void StartTransmit();
  virtual void StopTransmit();

  /**Sends the complete client list with all clients registered */
  bool SendClientList();

  /**Sends the extra capabilities for all clients that indicate to have extra capabilities. */
  bool SendExtraCapabilities();

  /**Requests the remote side to send it's client list */
  bool SendClientListCommand();

  /**Request the remote side to send the extra capabilities for the given client */
  bool SendExtraCapabilitiesCommand(const OpalH224Client & client);

  /**Callback for H.224 clients to send their extra capabilities */
  bool SendExtraCapabilitiesMessage(const OpalH224Client & client, BYTE *data, PINDEX length);

  /**Callback for H.224 clients to send a client frame */
  bool TransmitClientFrame(const OpalH224Client & client, H224_Frame & frame);

  virtual bool OnReceivedFrame(H224_Frame & frame);
  virtual bool OnReceivedCMEMessage(H224_Frame & frame);
  virtual bool OnReceivedClientList(H224_Frame & frame);
  virtual bool OnReceivedClientListCommand();
  virtual bool OnReceivedExtraCapabilities(H224_Frame & frame);
  virtual bool OnReceivedExtraCapabilitiesCommand();

protected:
  bool TransmitFrame(H224_Frame & frame);
  OpalH224Client * FindClient(
    BYTE clientID,
    BYTE extendedClientID,
    BYTE countryCode,
    BYTE countryCodeExtension,
    WORD manufacturerCode,
    BYTE manufacturerClientID
  ) const;

  PDECLARE_MUTEX(       m_transmitMutex);
  bool                  m_canTransmit;
  bool                  m_transmitHDLCTunneling;
  PINDEX                m_transmitBitIndex;
  PTime                 m_transmitStartTime;
  OpalH224MediaStream * m_transmitMediaStream;

  OpalH224ClientList    m_clients;
};

///////////////////////////////////////////////////////////////////////////////

class OpalH224MediaStream : public OpalMediaStream
{
    PCLASSINFO(OpalH224MediaStream, OpalMediaStream);

  public:
    OpalH224MediaStream(OpalConnection & connection,
                        OpalH224Handler & h224Handler,
                        const OpalMediaFormat & mediaFormat,
                        unsigned sessionID,
                        bool isSource);
    ~OpalH224MediaStream();

    virtual void OnStartMediaPatch();
    virtual PBoolean ReadPacket(RTP_DataFrame & packet);
    virtual PBoolean WritePacket(RTP_DataFrame & packet);
    virtual PBoolean IsSynchronous() const { return false; }
    virtual PBoolean RequiresPatchThread() const { return IsSink(); }

  private:
    virtual void InternalClose();

    OpalH224Handler & m_h224Handler;
    unsigned          m_consecutiveErrors;
};


#endif // OPAL_HAS_H224

#endif // OPAL_H224_H224HANDLER_H
