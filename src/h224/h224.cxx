/*
 * h224.cxx
 *
 * H.224 implementation for the OpenH323 Project.
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

/*
  This file implements H.224 as part of H.323, as well as RFC 4573 for H.224 over SIP 
 */

#include <ptlib.h>

#include <opal/buildopts.h>

#ifdef __GNUC__
#pragma implementation "h224.h"
#pragma implementation "h224handler.h"
#endif

#if OPAL_HAS_H224

#include <h224/h224.h>
#include <h224/h224handler.h>

#if OPAL_SIP
#include <sip/sdp.h>
#endif

#define H224_MAX_HEADER_SIZE 6+5

/////////////////////////////////////////////////////////////////////////

OPAL_INSTANTIATE_MEDIATYPE(OpalH224MediaType);

const char * OpalH224MediaType::Name()
{
  return "H.224";
}


OpalH224MediaType::OpalH224MediaType()
  : OpalRTPAVPMediaType(Name())
{
}


const OpalMediaType & OpalH224MediaType::MediaType()
{
  static const OpalMediaType type = Name();
  return type;
}

#if OPAL_SIP

const PCaselessString & OpalH224MediaType::GetSDPMediaType() { static PConstCaselessString const s("application"); return s; }

bool OpalH224MediaType::MatchesSDP(const PCaselessString & sdpMediaType,
                                   const PCaselessString & sdpTransport,
                                   const PStringArray & sdpLines,
                                   PINDEX index)
{
  if (sdpMediaType != GetSDPMediaType() || sdpTransport.NumCompare("RTP/") != PObject::EqualTo)
    return false;

  while (++index < sdpLines.GetSize() && sdpLines[index][0] != 'm') {
    PCaselessString line(sdpLines[index]);
    if (line.NumCompare("a=rtpmap:") == PObject::EqualTo &&
        line.Find(GetOpalH224_HDLCTunneling().GetEncodingName()) != P_MAX_INDEX)
      return true;
  }

  return false;
}


class SDPH224MediaDescription : public SDPRTPAVPMediaDescription
{
  PCLASSINFO(SDPH224MediaDescription, SDPRTPAVPMediaDescription);
  public:
    SDPH224MediaDescription(const OpalTransportAddress & address)
      : SDPRTPAVPMediaDescription(address, OpalH224MediaType::MediaType())
    {
    }

    virtual PString GetSDPMediaType() const
    {
      return OpalH224MediaType::GetSDPMediaType();
    }
};

SDPMediaDescription * OpalH224MediaType::CreateSDPMediaDescription(const OpalTransportAddress & localAddress) const
{
  return new SDPH224MediaDescription(localAddress);
}

#endif

/////////////////////////////////////////////////////////////////////////

const OpalMediaFormat & GetOpalH224_H323AnnexQ()
{
  static class H224_AnnexQ_MediaFormat : public OpalH224MediaFormat { 
    public: 
      H224_AnnexQ_MediaFormat() 
        : OpalH224MediaFormat("H.224/H323AnnexQ", RTP_DataFrame::DynamicBase)
      { 
        OpalMediaOption * option = new OpalMediaOptionBoolean("HDLC Tunneling", true, OpalMediaOption::MinMerge, false);
        AddOption(option);
      } 
  } const h224q; 
  return h224q; 
};

const OpalMediaFormat & GetOpalH224_HDLCTunneling()
{
  static class H224_HDLCTunneling_MediaFormat : public OpalH224MediaFormat { 
    public: 
      H224_HDLCTunneling_MediaFormat() 
        : OpalH224MediaFormat("H.224/HDLCTunneling", RTP_DataFrame::MaxPayloadType)    // HDLC tunnelled is not sent over RTP
      { 
        OpalMediaOption * option = new OpalMediaOptionBoolean("HDLC Tunneling", true, OpalMediaOption::MinMerge, true);
        AddOption(option);
      } 
  } const h224h; 
  return h224h; 
}

OpalH224MediaFormat::OpalH224MediaFormat(
      const char * fullName,                      ///<  Full name of media format
      RTP_DataFrame::PayloadTypes rtpPayloadType  ///<  RTP payload type code
  ) 
  : OpalMediaFormat(fullName,
                    OpalH224MediaType::MediaType(),
                    rtpPayloadType,
                    "h224",
                    false,
                    6400,  // 6.4kbit/s as defined in RFC 4573
                    0,
                    0,
                    4800,  // As defined in RFC 4573
                    0)
{
}

PObject * OpalH224MediaFormat::Clone() const
{
  return new OpalH224MediaFormat(*this);
}

PBoolean OpalH224MediaFormat::IsValidForProtocol(const PString & protocol) const
{
  // HDLC tunnelling only makes sense for H.323. Everything else uses RTP;
  return !GetOptionBoolean("HDLC Tunneling") || (protocol == "h323");
}

/////////////////////////////////////////////////////////////////////////

H224_Frame::H224_Frame(PINDEX size)
: Q922_Frame(H224_MAX_HEADER_SIZE + size)
{
  SetHighPriority(false);	
  SetControlFieldOctet(0x03); // UI-Mode
  SetDestinationTerminalAddress(OpalH224Handler::Broadcast);
  SetSourceTerminalAddress(OpalH224Handler::Broadcast);
  
  // setting Client ID to CME
  SetClientID(OpalH224Client::CMEClientID);
  
  // Setting ES / BS / C1 / C0 / Segment number to zero
  SetBS(false);
  SetES(false);
  SetC1(false);
  SetC0(false);
  SetSegmentNumber(0x00);
  
  SetClientDataSize(size);
}

H224_Frame::H224_Frame(const OpalH224Client & h224Client, PINDEX size)
: Q922_Frame(H224_MAX_HEADER_SIZE + size)
{
  SetHighPriority(false);	
  SetControlFieldOctet(0x03); // UI-Mode
  SetDestinationTerminalAddress(OpalH224Handler::Broadcast);
  SetSourceTerminalAddress(OpalH224Handler::Broadcast);
  
  SetClient(h224Client);
  
  // Setting ES / BS / C1 / C0 / Segment number to zero
  SetBS(false);
  SetES(false);
  SetC1(false);
  SetC0(false);
  SetSegmentNumber(0x00);
  
  SetClientDataSize(size);
}

H224_Frame::~H224_Frame()
{
}

void H224_Frame::SetHighPriority(PBoolean flag)
{
  SetHighOrderAddressOctet(0x00);
	
  if (flag) {
    SetLowOrderAddressOctet(0x71);
  } else {
    SetLowOrderAddressOctet(0x061);
  }
}

WORD H224_Frame::GetDestinationTerminalAddress() const
{
  BYTE *data = GetInformationFieldPtr();
  return (WORD)((data[0] << 8) | data[1]);
}

void H224_Frame::SetDestinationTerminalAddress(WORD address)
{
  BYTE *data = GetInformationFieldPtr();
  data[0] = (BYTE)(address >> 8);
  data[1] = (BYTE) address;
}

WORD H224_Frame::GetSourceTerminalAddress() const
{
  BYTE *data = GetInformationFieldPtr();
  return (WORD)((data[2] << 8) | data[3]);
}

void H224_Frame::SetSourceTerminalAddress(WORD address)
{
  BYTE *data = GetInformationFieldPtr();
  data[2] = (BYTE)(address >> 8);
  data[3] = (BYTE) address;
}

void H224_Frame::SetClient(const OpalH224Client & h224Client)
{
  BYTE clientID = h224Client.GetClientID();
  
  SetClientID(clientID);
  
  if (clientID == OpalH224Client::ExtendedClientID) {
    SetExtendedClientID(h224Client.GetExtendedClientID());
    
  } else if (clientID == OpalH224Client::NonStandardClientID) {
    SetNonStandardClientInformation(h224Client.GetCountryCode(),
                                    h224Client.GetCountryCodeExtension(),
                                    h224Client.GetManufacturerCode(),
                                    h224Client.GetManufacturerClientID());
  }
}

BYTE H224_Frame::GetClientID() const
{
  BYTE *data = GetInformationFieldPtr();
  return data[4] & 0x7f;
}

void H224_Frame::SetClientID(BYTE clientID)
{
	BYTE *data = GetInformationFieldPtr();
  data[4] = (clientID & 0x7f);
}

BYTE H224_Frame::GetExtendedClientID() const
{
  if (GetClientID() != OpalH224Client::ExtendedClientID) {
    return 0x00;
  }
	
  BYTE *data = GetInformationFieldPtr();
  return data[5];
}

void H224_Frame::SetExtendedClientID(BYTE extendedClientID)
{
  if (GetClientID() != OpalH224Client::ExtendedClientID) {
    return;
  }
	
  BYTE *data = GetInformationFieldPtr();
  data[5] = extendedClientID;
}

BYTE H224_Frame::GetCountryCode() const
{
  if (GetClientID() != OpalH224Client::NonStandardClientID) {
    return 0x00;
  }
	
  BYTE *data = GetInformationFieldPtr();
  return data[5];
}

BYTE H224_Frame::GetCountryCodeExtension() const
{
  if (GetClientID() != OpalH224Client::NonStandardClientID) {
    return 0x00;
  }
	
  BYTE *data = GetInformationFieldPtr();
  return data[6];
}

WORD H224_Frame::GetManufacturerCode() const
{
  if (GetClientID() != OpalH224Client::NonStandardClientID) {
    return 0x0000;
  }
	
  BYTE *data = GetInformationFieldPtr();
  return (((WORD)data[7] << 8) | (WORD)data[8]);
}

BYTE H224_Frame::GetManufacturerClientID() const
{
  if (GetClientID() != OpalH224Client::NonStandardClientID) {
    return 0x00;
  }
	
  BYTE *data = GetInformationFieldPtr();
  return data[9];
}

void H224_Frame::SetNonStandardClientInformation(BYTE countryCode,
                                                 BYTE countryCodeExtension,
                                                 WORD manufacturerCode,
                                                 BYTE manufacturerClientID)
{
  if (GetClientID() != OpalH224Client::NonStandardClientID) {	
    return;
  }
	
  BYTE *data = GetInformationFieldPtr();
	
  data[5] = countryCode;
  data[6] = countryCodeExtension;
  data[7] = (BYTE)(manufacturerCode << 8);
  data[8] = (BYTE) manufacturerCode;
  data[9] = manufacturerClientID;
}

PBoolean H224_Frame::GetBS() const
{
  BYTE *data = GetInformationFieldPtr();
	
  return (data[5] & 0x80) != 0;
}

void H224_Frame::SetBS(PBoolean flag)
{
  BYTE *data = GetInformationFieldPtr();
	
  if (flag) {
    data[5] |= 0x80;
  }	else {
    data[5] &= 0x7f;
  }
}

PBoolean H224_Frame::GetES() const
{
  BYTE *data = GetInformationFieldPtr();
	
  return (data[5] & 0x40) != 0;
}

void H224_Frame::SetES(PBoolean flag)
{
  BYTE *data = GetInformationFieldPtr();
	
  if (flag) {
    data[5] |= 0x40;
  } else {
    data[5] &= 0xbf;
  }
}

PBoolean H224_Frame::GetC1() const
{
  BYTE *data = GetInformationFieldPtr();
	
  return (data[5] & 0x20) != 0;
}

void H224_Frame::SetC1(PBoolean flag)
{
  BYTE *data = GetInformationFieldPtr();
	
  if (flag) {
    data[5] |= 0x20;
  } else {
    data[5] &= 0xdf;
  }
}

PBoolean H224_Frame::GetC0() const
{
  BYTE *data = GetInformationFieldPtr();
	
  return (data[5] & 0x10) != 0;
}

void H224_Frame::SetC0(PBoolean flag)
{
  BYTE *data = GetInformationFieldPtr();
	
  if (flag) {
    data[5] |= 0x10;
  }	else {
    data[5] &= 0xef;
  }
}

BYTE H224_Frame::GetSegmentNumber() const
{
  BYTE *data = GetInformationFieldPtr();
	
  return (data[5] & 0x0f);
}

void H224_Frame::SetSegmentNumber(BYTE segmentNumber)
{
  BYTE *data = GetInformationFieldPtr();
	
  data[5] &= 0xf0;
  data[5] |= (segmentNumber & 0x0f);
}

BYTE * H224_Frame::GetClientDataPtr() const
{
  BYTE * data = GetInformationFieldPtr();
  return (data + GetHeaderSize());
}

PINDEX H224_Frame::GetClientDataSize() const
{
  PINDEX size = GetInformationFieldSize();
  return (size - GetHeaderSize());
}

void H224_Frame::SetClientDataSize(PINDEX size)
{
  SetInformationFieldSize(size + GetHeaderSize());
}

PBoolean H224_Frame::DecodeAnnexQ(const BYTE *data, PINDEX size)
{
  PBoolean result = Q922_Frame::DecodeAnnexQ(data, size);
	
  if (result == false) {
    return false;
  }
	
  // doing some validity checks for H.224 frames
  BYTE highOrderAddressOctet = GetHighOrderAddressOctet();
  BYTE lowOrderAddressOctet = GetLowOrderAddressOctet();
  BYTE controlFieldOctet = GetControlFieldOctet();
	
  if ((highOrderAddressOctet != 0x00) ||
      (!(lowOrderAddressOctet == 0x61 || lowOrderAddressOctet == 0x71)) ||
      (controlFieldOctet != 0x03)) {		
	  return false;
  }
	
  return true;
  
}

PBoolean H224_Frame::DecodeHDLC(const BYTE *data, PINDEX size)
{
  PBoolean result = Q922_Frame::DecodeHDLC(data, size);
	
  if (result == false) {
    return false;
  }
	
  // doing some validity checks for H.224 frames
  BYTE highOrderAddressOctet = GetHighOrderAddressOctet();
  BYTE lowOrderAddressOctet = GetLowOrderAddressOctet();
  BYTE controlFieldOctet = GetControlFieldOctet();
	
  if ((highOrderAddressOctet != 0x00) ||
     (!(lowOrderAddressOctet == 0x61 || lowOrderAddressOctet == 0x71)) ||
     (controlFieldOctet != 0x03)) {		
	  return false;
  }
	
  return true;
}

PINDEX H224_Frame::GetHeaderSize() const
{
  BYTE clientID = GetClientID();
  
  if (clientID < OpalH224Client::ExtendedClientID) {
    return 6;
  } else if (clientID == OpalH224Client::ExtendedClientID) {
    return 7; // one extra octet
  } else {
    return 11; // 5 extra octets
  }
}

////////////////////////////////////

OpalH224Handler::OpalH224Handler()
: transmitMutex(),
  transmitFrame(300),
  receiveFrame()
{
  canTransmit = false;
  transmitBitIndex = 7;
  transmitStartTime = NULL;
  transmitMediaStream = NULL;
  
  transmitHDLCTunneling = false;
  receiveHDLCTunneling = false;
  
  clients.DisallowDeleteObjects();
}

OpalH224Handler::~OpalH224Handler()
{
}

PBoolean OpalH224Handler::AddClient(OpalH224Client & client)
{
  if (client.GetClientID() == OpalH224Client::CMEClientID) {
    return false; // No client may have CMEClientID
  }
	
  if (clients.GetObjectsIndex(&client) != P_MAX_INDEX) {
    return false; // Only allow one instance of a client
  }
	
  clients.Append(&client);
  client.SetH224Handler(this);
  return true;
}

PBoolean OpalH224Handler::RemoveClient(OpalH224Client & client)
{
  PBoolean result = clients.Remove(&client);
  if (result == true) {
    client.SetH224Handler(NULL);
  }
  return result;
}

void OpalH224Handler::SetTransmitMediaFormat(const OpalMediaFormat & mediaFormat)
{
  PAssert(mediaFormat.GetMediaType() == "h224", "H.224 handler passed incorrect media format");
  transmitHDLCTunneling = mediaFormat.GetOptionBoolean("HDLC Tunneling");
}

void OpalH224Handler::SetReceiveMediaFormat(const OpalMediaFormat & mediaFormat)
{
  PAssert(mediaFormat.GetMediaType() == "h224", "H.224 handler passed incorrect media format");
  receiveHDLCTunneling = mediaFormat.GetOptionBoolean("HDLC Tunneling");
}

void OpalH224Handler::SetTransmitMediaStream(OpalH224MediaStream * mediaStream)
{
  PWaitAndSignal m(transmitMutex);
	
  transmitMediaStream = mediaStream;
	
  if (transmitMediaStream != NULL) {
    transmitFrame.SetPayloadType(transmitMediaStream->GetMediaFormat().GetPayloadType());
  }
}

void OpalH224Handler::StartTransmit()
{
  PWaitAndSignal m(transmitMutex);
	
  if (canTransmit == true) {
    return;
  }
	
  canTransmit = true;
  
  transmitBitIndex = 7;
  transmitStartTime = new PTime();
  
  SendClientList();
  SendExtraCapabilities();
}

void OpalH224Handler::StopTransmit()
{
  PWaitAndSignal m(transmitMutex);
  
  if (canTransmit == false) {
    return;
  }
	
  delete transmitStartTime;
  transmitStartTime = NULL;
	
  canTransmit = false;
}

PBoolean OpalH224Handler::SendClientList()
{
  PWaitAndSignal m(transmitMutex);
	
  if (canTransmit == false) {
    return false;
  }
  
  // If all clients are non-standard, 5 octets per clients + 3 octets header information
  H224_Frame h224Frame = H224_Frame(5*clients.GetSize() + 3);
	
  h224Frame.SetHighPriority(true);
  h224Frame.SetDestinationTerminalAddress(Broadcast);
  h224Frame.SetSourceTerminalAddress(Broadcast);
	
  // CME frame
  h224Frame.SetClientID(OpalH224Client::CMEClientID);
	
  // Begin and end of sequence
  h224Frame.SetBS(true);
  h224Frame.SetES(true);
  h224Frame.SetC1(false);
  h224Frame.SetC0(false);
  h224Frame.SetSegmentNumber(0);
	
  BYTE *ptr = h224Frame.GetClientDataPtr();
	
  ptr[0] = OpalH224Handler::CMEClientListCode;
  ptr[1] = OpalH224Handler::CMEMessage;
  ptr[2] = (BYTE)clients.GetSize();
  
  PINDEX dataIndex = 3;
  for (PINDEX i = 0; i < clients.GetSize(); i++) {
    OpalH224Client & client = clients[i];
    
    BYTE clientID = client.GetClientID();
    
    if (client.HasExtraCapabilities()) {
      ptr[dataIndex] = (0x80 | clientID);
    } else {
      ptr[dataIndex] = (0x7f & clientID);
    }
    dataIndex++;
    
    if (clientID == OpalH224Client::ExtendedClientID) {
      ptr[dataIndex] = client.GetExtendedClientID();
      dataIndex++;
    
    } else if (clientID == OpalH224Client::NonStandardClientID) {
      
      ptr[dataIndex] = client.GetCountryCode();
      dataIndex++;
      ptr[dataIndex] = client.GetCountryCodeExtension();
      dataIndex++;
      
      WORD manufacturerCode = client.GetManufacturerCode();
      ptr[dataIndex] = (BYTE)(manufacturerCode >> 8);
      dataIndex++;
      ptr[dataIndex] = (BYTE) manufacturerCode;
      dataIndex++;
      
      ptr[dataIndex] = client.GetManufacturerClientID();
      dataIndex++;
    }
  }
  
  h224Frame.SetClientDataSize(dataIndex);
	
  TransmitFrame(h224Frame);
	
  return true;
}

PBoolean OpalH224Handler::SendExtraCapabilities()
{
  for (PINDEX i = 0; i < clients.GetSize(); i++) {
    OpalH224Client & client = clients[i];
    client.SendExtraCapabilities();
  }
	
  return true;
}

PBoolean OpalH224Handler::SendClientListCommand()
{
  PWaitAndSignal m(transmitMutex);
	
  if (canTransmit == false) {
    return false;
  }
	
  H224_Frame h224Frame = H224_Frame(2);
  h224Frame.SetHighPriority(true);
  h224Frame.SetDestinationTerminalAddress(OpalH224Handler::Broadcast);
  h224Frame.SetSourceTerminalAddress(OpalH224Handler::Broadcast);
	
  // CME frame
  h224Frame.SetClientID(OpalH224Client::CMEClientID);
	
  // Begin and end of sequence
  h224Frame.SetBS(true);
  h224Frame.SetES(true);
  h224Frame.SetC1(false);
  h224Frame.SetC0(false);
  h224Frame.SetSegmentNumber(0);
	
  BYTE *ptr = h224Frame.GetClientDataPtr();
	
  ptr[0] = OpalH224Handler::CMEClientListCode;
  ptr[1] = OpalH224Handler::CMECommand;
	
  TransmitFrame(h224Frame);
	
  return true;
}

PBoolean OpalH224Handler::SendExtraCapabilitiesCommand(const OpalH224Client & client)
{
  PWaitAndSignal m(transmitMutex);
	
  if (canTransmit == false) {
    return false;
  }
	
  if (clients.GetObjectsIndex(&client) == P_MAX_INDEX) {
    return false; // only allow if the client is really registered
  }
	
  H224_Frame h224Frame = H224_Frame(8);
  h224Frame.SetHighPriority(true);
  h224Frame.SetDestinationTerminalAddress(OpalH224Handler::Broadcast);
  h224Frame.SetSourceTerminalAddress(OpalH224Handler::Broadcast);
	
  // CME frame
  h224Frame.SetClientID(OpalH224Client::CMEClientID);
	
  // Begin and end of sequence
  h224Frame.SetBS(true);
  h224Frame.SetES(true);
  h224Frame.SetC1(false);
  h224Frame.SetC0(false);
  h224Frame.SetSegmentNumber(0);
	
  BYTE *ptr = h224Frame.GetClientDataPtr();
	
  ptr[0] = OpalH224Handler::CMEExtraCapabilitiesCode;
  ptr[1] = OpalH224Handler::CMECommand;

  PINDEX dataSize;
  
  BYTE extendedCapabilitiesFlag = client.HasExtraCapabilities() ? 0x80 : 0x00;
  BYTE clientID = client.GetClientID();
  ptr[2] = (extendedCapabilitiesFlag | (clientID & 0x7f));
  
  if (clientID < OpalH224Client::ExtendedClientID) {
    dataSize = 3;
  } else if (clientID == OpalH224Client::ExtendedClientID) {
    ptr[3] = client.GetExtendedClientID();
    dataSize = 4;
  } else {
    ptr[3] = client.GetCountryCode();
    ptr[4] = client.GetCountryCodeExtension();
	  
    WORD manufacturerCode = client.GetManufacturerCode();
    ptr[5] = (BYTE)(manufacturerCode >> 8);
    ptr[6] = (BYTE) manufacturerCode;
	  
    ptr[7] = client.GetManufacturerClientID();
    dataSize = 8;
  }
  h224Frame.SetClientDataSize(dataSize);
	
  TransmitFrame(h224Frame);
	
  return true;
}

PBoolean OpalH224Handler::SendExtraCapabilitiesMessage(const OpalH224Client & client, 
                                                       BYTE *data, PINDEX length)
{	
  PWaitAndSignal m(transmitMutex);
	
  if (clients.GetObjectsIndex(&client) == P_MAX_INDEX) {
    return false; // Only allow if the client is really registered
  }
	
  H224_Frame h224Frame = H224_Frame(length+3);
  h224Frame.SetHighPriority(true);
  h224Frame.SetDestinationTerminalAddress(OpalH224Handler::Broadcast);
  h224Frame.SetSourceTerminalAddress(OpalH224Handler::Broadcast);
	
  // use clientID zero to indicate a CME frame
  h224Frame.SetClientID(OpalH224Client::CMEClientID);
	
  // Begin and end of sequence, rest is zero
  h224Frame.SetBS(true);
  h224Frame.SetES(true);
  h224Frame.SetC1(false);
  h224Frame.SetC0(false);
  h224Frame.SetSegmentNumber(0);
	
  BYTE *ptr = h224Frame.GetClientDataPtr();
	
  ptr[0] = CMEExtraCapabilitiesCode;
  ptr[1] = CMEMessage;
  
  PINDEX headerSize;
  BYTE clientID = client.GetClientID();
  BYTE extendedCapabilitiesFlag = client.HasExtraCapabilities() ? 0x80 : 0x00;
  
  ptr[2] = (extendedCapabilitiesFlag | (clientID & 0x7f));
  
  if (clientID < OpalH224Client::ExtendedClientID) {
    headerSize = 3;
  } else if (clientID == OpalH224Client::ExtendedClientID) {
    ptr[3] = client.GetExtendedClientID();
    headerSize = 4;
  } else {
    ptr[3] = client.GetCountryCode();
    ptr[4] = client.GetCountryCodeExtension();
	  
    WORD manufacturerCode = client.GetManufacturerCode();
    ptr[5] = (BYTE) (manufacturerCode >> 8);
    ptr[6] = (BYTE) manufacturerCode;
	  
    ptr[7] = client.GetManufacturerClientID();
    headerSize = 8;
  }
	
  h224Frame.SetClientDataSize(length+headerSize);
  memcpy(ptr+headerSize, data, length);
	
  TransmitFrame(h224Frame);
	
  return true;	
}

PBoolean OpalH224Handler::TransmitClientFrame(const OpalH224Client & client, H224_Frame & frame)
{
  PWaitAndSignal m(transmitMutex);
  
  if (canTransmit == false) {
    return false;
  }
	
  if (clients.GetObjectsIndex(&client) == P_MAX_INDEX) {
    return false; // Only allow if the client is really registered
  }
  
  TransmitFrame(frame);
	
  return true;
}

PBoolean OpalH224Handler::OnReceivedFrame(H224_Frame & frame)
{
  if (frame.GetDestinationTerminalAddress() != OpalH224Handler::Broadcast) {
    // only broadcast frames are handled at the moment
    PTRACE(3, "H.224\tReceived frame with non-broadcast address");
    return true;
  }
  BYTE clientID = frame.GetClientID();
	
  if (clientID == OpalH224Client::CMEClientID) {
    return OnReceivedCMEMessage(frame);
  }
	
  for (PINDEX i = 0; i < clients.GetSize(); i++) {
    OpalH224Client & client = clients[i];
    if (client.GetClientID() == clientID) {
      PBoolean found = false;
      if (clientID < OpalH224Client::ExtendedClientID) {
        found = true;
      } else if (clientID == OpalH224Client::ExtendedClientID) {
        if (client.GetExtendedClientID() == frame.GetExtendedClientID()) {
          found = true;
        }
      } else {
        if (client.GetCountryCode() == frame.GetCountryCode() &&
           client.GetCountryCodeExtension() == frame.GetCountryCodeExtension() &&
           client.GetManufacturerCode() == frame.GetManufacturerCode() &&
           client.GetManufacturerClientID() == frame.GetManufacturerClientID()) {
          found = true;
        }
      }
      if (found == true) {
        client.OnReceivedMessage(frame);
        return true;
      }
    }
  }
  
  // ignore if no client found
	
  return true;
}

PBoolean OpalH224Handler::OnReceivedCMEMessage(H224_Frame & frame)
{
  BYTE *data = frame.GetClientDataPtr();
	
  if (data[0] == CMEClientListCode) {
	
    if (data[1] == CMEMessage) {
      return OnReceivedClientList(frame);
		
    } else if (data[1] == CMECommand) {
      return OnReceivedClientListCommand();
    }
	  
  } else if (data[0] == CMEExtraCapabilitiesCode) {
	  
    if (data[1] == CMEMessage) {
      return OnReceivedExtraCapabilities(frame);
		
    } else if (data[1] == CMECommand) {
      return OnReceivedExtraCapabilitiesCommand();
    }
  }
	
  // incorrect frames are simply ignored
  return true;
}

PBoolean OpalH224Handler::OnReceivedClientList(H224_Frame & frame)
{
  // First, reset all clients
  for (PINDEX i = 0; i < clients.GetSize(); i++)
  {
    OpalH224Client & client = clients[i];
    client.SetRemoteClientAvailable(false, false);
  }
  
  BYTE *data = frame.GetClientDataPtr();
	
  BYTE numberOfClients = data[2];
	
  PINDEX dataIndex = 3;
	
  while (numberOfClients > 0) {
	  
    BYTE clientID = (data[dataIndex] & 0x7f);
    PBoolean hasExtraCapabilities = (data[dataIndex] & 0x80) != 0 ? true: false;
    dataIndex++;
    BYTE extendedClientID = 0x00;
    BYTE countryCode = CountryCodeEscape;
    BYTE countryCodeExtension = 0x00;
    WORD manufacturerCode = 0x0000;
    BYTE manufacturerClientID = 0x00;
    
    if (clientID == OpalH224Client::ExtendedClientID) {
      extendedClientID = data[dataIndex];
      dataIndex++;
    } else if (clientID == OpalH224Client::NonStandardClientID) {
      countryCode = data[dataIndex];
      dataIndex++;
      countryCodeExtension = data[dataIndex];
      dataIndex++;
      manufacturerCode = (((WORD)data[dataIndex] << 8) | (WORD)data[dataIndex+1]);
      dataIndex += 2;
      manufacturerClientID = data[dataIndex];
      dataIndex++;
    }
    
    for (PINDEX i = 0; i < clients.GetSize(); i++) {
      OpalH224Client & client = clients[i];
      PBoolean found = false;
      if (client.GetClientID() == clientID) {
        if (clientID < OpalH224Client::ExtendedClientID) {
          found = true;
        } else if (clientID == OpalH224Client::ExtendedClientID) {
          if (client.GetExtendedClientID() == extendedClientID) {
            found = true;
          }
        } else {
          if (client.GetCountryCode() == countryCode &&
             client.GetCountryCodeExtension() == countryCodeExtension &&
             client.GetManufacturerCode() == manufacturerCode &&
             client.GetManufacturerClientID() == manufacturerClientID) {
            found = true;
          }
        }
      }
      if (found == true) {
        client.SetRemoteClientAvailable(true, hasExtraCapabilities);
        break;
      }
    }
    numberOfClients--;
  }
  
	
  return true;
}

PBoolean OpalH224Handler::OnReceivedClientListCommand()
{
  SendClientList();
  return true;
}

PBoolean OpalH224Handler::OnReceivedExtraCapabilities(H224_Frame & frame)
{
  BYTE *data = frame.GetClientDataPtr();
	
  BYTE clientID = (data[2] & 0x7f);
  PINDEX dataIndex = 0;
  BYTE extendedClientID = 0x00;
  BYTE countryCode = CountryCodeEscape;
  BYTE countryCodeExtension = 0x00;
  WORD manufacturerCode = 0x0000;
  BYTE manufacturerClientID = 0x00;
  
  if (clientID < OpalH224Client::ExtendedClientID) {
    dataIndex = 3;
  } else if (clientID == OpalH224Client::ExtendedClientID) {
    extendedClientID = data[3];
    dataIndex = 4;
  } else if (clientID == OpalH224Client::NonStandardClientID) {
    countryCode = data[3];
    countryCodeExtension = data[4];
    manufacturerCode = (((WORD)data[5] << 8) | (WORD)data[6]);
    manufacturerClientID = data[7];
    dataIndex = 8;
  }
  
  for (PINDEX i = 0; i < clients.GetSize(); i++) {
    OpalH224Client & client = clients[i];
    PBoolean found = false;
    if (client.GetClientID() == clientID) {
      if (clientID < OpalH224Client::ExtendedClientID) {
        found = true;
      } else if (clientID == OpalH224Client::ExtendedClientID) {
        if (client.GetExtendedClientID() == extendedClientID) {
          found = true;
        }
      } else {
        if (client.GetCountryCode() == countryCode &&
           client.GetCountryCodeExtension() == countryCodeExtension &&
           client.GetManufacturerCode() == manufacturerCode &&
           client.GetManufacturerClientID() == manufacturerClientID) {
          found = true;
        }
      }
    }
    if (found) {
      PINDEX size = frame.GetClientDataSize() - dataIndex;
      client.SetRemoteClientAvailable(true, true);
      client.OnReceivedExtraCapabilities((data + dataIndex), size);
      return true;
    }
  }
  
  // Simply ignore if no client is available for this clientID
	
  return true;
}

PBoolean OpalH224Handler::OnReceivedExtraCapabilitiesCommand()
{
  SendExtraCapabilities();
  return true;
}

PBoolean OpalH224Handler::HandleFrame(const RTP_DataFrame & dataFrame)
{
  if (receiveHDLCTunneling) {
    if (receiveFrame.DecodeHDLC(dataFrame.GetPayloadPtr(), dataFrame.GetPayloadSize())) {
      PBoolean result = OnReceivedFrame(receiveFrame);
      return result;
    } else {
      PTRACE(1, "H224\tDecoding of the frame failed");
      return false;
    }
  } else {
    if (receiveFrame.DecodeAnnexQ(dataFrame.GetPayloadPtr(), dataFrame.GetPayloadSize())) {
      PBoolean result = OnReceivedFrame(receiveFrame);
      return result;
    } else {
      PTRACE(1, "H224\tDecoding of the frame failed");
      return false;
    }
  }
}

void OpalH224Handler::TransmitFrame(H224_Frame & frame)
{
  PINDEX size;
  if (transmitHDLCTunneling) {
    size = frame.GetHDLCEncodedSize();
    transmitFrame.SetMinSize(size);
    if (!frame.EncodeHDLC(transmitFrame.GetPayloadPtr(), size, transmitBitIndex)) {
      PTRACE(1, "H224\tFailed to encode the frame");
      return;
    }
  } else {
    size = frame.GetAnnexQEncodedSize();
    transmitFrame.SetMinSize(size);
    if (!frame.EncodeAnnexQ(transmitFrame.GetPayloadPtr(), size)) {
      PTRACE(1, "H224\tFailed to encode the frame");
      return;
    }
  }
  
  // determining correct timestamp
  PTime currentTime = PTime();
  PTimeInterval timePassed = currentTime - *transmitStartTime;
  transmitFrame.SetTimestamp((DWORD)timePassed.GetMilliSeconds() * 8);
  
  transmitFrame.SetPayloadSize(size);
  transmitFrame.SetMarker(true);
  
  if (transmitMediaStream != NULL) {
    transmitMediaStream->PushPacket(transmitFrame);
  }
}

////////////////////////////////////

OpalH224MediaStream::OpalH224MediaStream(OpalConnection & connection, 
                                         OpalH224Handler & handler,
                                         const OpalMediaFormat & mediaFormat,
                                         unsigned sessionID,
                                         PBoolean isSource)
: OpalMediaStream(connection, mediaFormat, sessionID, isSource),
  h224Handler(handler)
{
  if (isSource == true) {
    h224Handler.SetTransmitMediaFormat(mediaFormat);
    h224Handler.SetTransmitMediaStream(this);
  } else {
    h224Handler.SetReceiveMediaFormat(mediaFormat);
  }
}

OpalH224MediaStream::~OpalH224MediaStream()
{
  Close();
}

void OpalH224MediaStream::OnStartMediaPatch()
{	
  h224Handler.StartTransmit();
  OpalMediaStream::OnStartMediaPatch();
}


void OpalH224MediaStream::InternalClose()
{
  if (IsSource()) {
    h224Handler.StopTransmit();
    h224Handler.SetTransmitMediaStream(NULL);
  }
}


PBoolean OpalH224MediaStream::ReadPacket(RTP_DataFrame & /*packet*/)
{
  return false;
}

PBoolean OpalH224MediaStream::WritePacket(RTP_DataFrame & packet)
{
  return h224Handler.HandleFrame(packet);
}

////////////////////////////////////

OpalH224Client::OpalH224Client()
{
  remoteClientAvailable = false;
  remoteClientHasExtraCapabilities = false;
  h224Handler = NULL;
}

OpalH224Client::~OpalH224Client()
{
}

PObject::Comparison OpalH224Client::Compare(const PObject & obj)
{
  if (!PIsDescendant(&obj, OpalH224Client)) {
    return LessThan;
  }
	
  const OpalH224Client & otherClient = (const OpalH224Client &) obj;
	
  BYTE clientID = GetClientID();
  BYTE otherClientID = otherClient.GetClientID();
	
  if (clientID < otherClientID) {
    return LessThan;
  } else if (clientID > otherClientID) {
    return GreaterThan;
  }
	
  if (clientID < ExtendedClientID) {
    return EqualTo;
  }
	
  if (clientID == ExtendedClientID) {
    BYTE extendedClientID = GetExtendedClientID();
    BYTE otherExtendedClientID = otherClient.GetExtendedClientID();
		
    if (extendedClientID < otherExtendedClientID) {
      return LessThan;
    } else if (extendedClientID > otherExtendedClientID) {
      return GreaterThan;
    } else {
      return EqualTo;
    }
  }
	
  // Non-standard client.
  // Compare country code, extended country code, manufacturer code, manufacturer client ID
  BYTE countryCode = GetCountryCode();
  BYTE otherCountryCode = otherClient.GetCountryCode();
  if (countryCode < otherCountryCode) {
    return LessThan;
  } else if (countryCode > otherCountryCode) {
    return GreaterThan;
  }
	
  BYTE countryCodeExtension = GetCountryCodeExtension();
  BYTE otherCountryCodeExtension = otherClient.GetCountryCodeExtension();
  if (countryCodeExtension < otherCountryCodeExtension) {
    return LessThan;
  } else if (countryCodeExtension > otherCountryCodeExtension) {
    return GreaterThan;
  }
	
  WORD manufacturerCode = GetManufacturerCode();
  WORD otherManufacturerCode = otherClient.GetManufacturerCode();
  if (manufacturerCode < otherManufacturerCode) {
    return LessThan;
  } else if (manufacturerCode > otherManufacturerCode) {
    return GreaterThan;
  }
	
  BYTE manufacturerClientID = GetManufacturerClientID();
  BYTE otherManufacturerClientID = otherClient.GetManufacturerClientID();
  
  if (manufacturerClientID < otherManufacturerClientID) {
    return LessThan;
  } else if (manufacturerClientID > otherManufacturerClientID) {
    return GreaterThan;
  }
	
  return EqualTo;
}

void OpalH224Client::SetRemoteClientAvailable(PBoolean available, PBoolean hasExtraCapabilities)
{
  remoteClientAvailable = available;
  remoteClientHasExtraCapabilities = hasExtraCapabilities;
}

#endif // OPAL_HAS_H224
