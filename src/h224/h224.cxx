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
 */

/*
  This file implements H.224 as part of H.323, as well as RFC 4573 for H.224 over SIP
 */

#include <ptlib.h>

#ifdef __GNUC__
#pragma implementation "h224.h"
#pragma implementation "h224handler.h"
#endif

#include <h224/h224.h>
#include <h224/h224handler.h>

#if OPAL_HAS_H224

#include <sdp/sdp.h>


#define H224_MAX_HEADER_SIZE 6+5

#define PTraceModule() "H.224"


/////////////////////////////////////////////////////////////////////////

class OpalH224MediaDefinition : public OpalRTPAVPMediaDefinition
{
  public:
    static const char * Name() { return "H.224"; }

    OpalH224MediaDefinition() : OpalRTPAVPMediaDefinition(Name(), 0, OpalMediaType::ReceiveTransmit) { }

#if OPAL_SDP
    virtual PString GetSDPMediaType() const { return SDPApplicationMediaDescription::TypeName(); }

    virtual bool MatchesSDP(const PCaselessString & sdpMediaType,
                            const PCaselessString & sdpTransport,
                            const PStringArray & sdpLines,
                            PINDEX index)
    {
      if (!OpalRTPAVPMediaDefinition::MatchesSDP(sdpMediaType, sdpTransport, sdpLines, index))
        return false;

      while (++index < sdpLines.GetSize() && sdpLines[index][0] != 'm') {
        static PRegularExpression regex("a=rtpmap:[0-9]+ +h224/4800", PRegularExpression::IgnoreCase|PRegularExpression::Extended);
        if (sdpLines[index].FindRegEx(regex) != P_MAX_INDEX)
          return true;
      }

      return false;
    }

    virtual SDPMediaDescription * CreateSDPMediaDescription(const OpalTransportAddress & localAddress) const
    {
      return new SDPRTPAVPMediaDescription(localAddress, OpalH224MediaType());
    }
#endif // OPAL_SDP
};

OPAL_MEDIATYPE(OpalH224Media);


/////////////////////////////////////////////////////////////////////////

const PString & OpalH224MediaFormat::HDLCTunnelingOption() { static const PConstString s("HDLC Tunneling"); return s; }

OpalH224MediaFormat::Internal::Internal(const char * fullName, const char * description, bool hdlcTunneling)
  : OpalMediaFormatInternal(fullName,
                            OpalH224MediaType(),
                            RTP_DataFrame::DynamicBase,
                            "H224",
                            false,
                            6400,  // 6.4kbit/s as defined in RFC 4573
                            0,
                            0,
                            4800,  // Clcok rate as defined in RFC 4573
                            0)
{
  SetOptionString(OpalMediaFormat::DescriptionOption(), description);
  AddOption(new OpalMediaOptionBoolean(OpalH224MediaFormat::HDLCTunnelingOption(), true, OpalMediaOption::NoMerge, hdlcTunneling));
}


bool OpalH224MediaFormat::Internal::IsValidForProtocol(const PString & protocol) const
{
  return (protocol *= "H.323") || !GetOptionBoolean(OpalH224MediaFormat::HDLCTunnelingOption(), false);
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


void H224_Frame::SetHighPriority(bool flag)
{
  SetHighOrderAddressOctet(0x00);
  SetLowOrderAddressOctet(flag ? 0x71 : 0x061);
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

  if (clientID == OpalH224Client::ExtendedClientID)
    SetExtendedClientID(h224Client.GetExtendedClientID());
  else if (clientID == OpalH224Client::NonStandardClientID)
    SetNonStandardClientInformation(h224Client.GetCountryCode(),
                                    h224Client.GetCountryCodeExtension(),
                                    h224Client.GetManufacturerCode(),
                                    h224Client.GetManufacturerClientID());
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
  if (GetClientID() != OpalH224Client::ExtendedClientID)
    return 0x00;

  BYTE *data = GetInformationFieldPtr();
  return data[5];
}


void H224_Frame::SetExtendedClientID(BYTE extendedClientID)
{
  if (GetClientID() != OpalH224Client::ExtendedClientID)
    return;

  BYTE *data = GetInformationFieldPtr();
  data[5] = extendedClientID;
}


BYTE H224_Frame::GetCountryCode() const
{
  if (GetClientID() != OpalH224Client::NonStandardClientID)
    return 0x00;

  BYTE *data = GetInformationFieldPtr();
  return data[5];
}


BYTE H224_Frame::GetCountryCodeExtension() const
{
  if (GetClientID() != OpalH224Client::NonStandardClientID)
    return 0x00;

  BYTE *data = GetInformationFieldPtr();
  return data[6];
}


WORD H224_Frame::GetManufacturerCode() const
{
  if (GetClientID() != OpalH224Client::NonStandardClientID)
    return 0x0000;

  BYTE *data = GetInformationFieldPtr();
  return (((WORD)data[7] << 8) | (WORD)data[8]);
}


BYTE H224_Frame::GetManufacturerClientID() const
{
  if (GetClientID() != OpalH224Client::NonStandardClientID)
    return 0x00;

  BYTE *data = GetInformationFieldPtr();
  return data[9];
}


void H224_Frame::SetNonStandardClientInformation(BYTE countryCode,
                                                 BYTE countryCodeExtension,
                                                 WORD manufacturerCode,
                                                 BYTE manufacturerClientID)
{
  if (GetClientID() != OpalH224Client::NonStandardClientID)
    return;

  BYTE *data = GetInformationFieldPtr();

  data[5] = countryCode;
  data[6] = countryCodeExtension;
  data[7] = (BYTE)(manufacturerCode << 8);
  data[8] = (BYTE) manufacturerCode;
  data[9] = manufacturerClientID;
}


bool H224_Frame::GetBS() const
{
  BYTE *data = GetInformationFieldPtr();
  return (data[5] & 0x80) != 0;
}


void H224_Frame::SetBS(bool flag)
{
  BYTE *data = GetInformationFieldPtr();

  if (flag)
    data[5] |= 0x80;
  else
    data[5] &= 0x7f;
}

bool H224_Frame::GetES() const
{
  BYTE *data = GetInformationFieldPtr();
  return (data[5] & 0x40) != 0;
}

void H224_Frame::SetES(bool flag)
{
  BYTE *data = GetInformationFieldPtr();

  if (flag)
    data[5] |= 0x40;
  else
    data[5] &= 0xbf;
}


bool H224_Frame::GetC1() const
{
  BYTE *data = GetInformationFieldPtr();
  return (data[5] & 0x20) != 0;
}


void H224_Frame::SetC1(bool flag)
{
  BYTE *data = GetInformationFieldPtr();

  if (flag)
    data[5] |= 0x20;
  else
    data[5] &= 0xdf;
}


bool H224_Frame::GetC0() const
{
  BYTE *data = GetInformationFieldPtr();
  return (data[5] & 0x10) != 0;
}


void H224_Frame::SetC0(bool flag)
{
  BYTE *data = GetInformationFieldPtr();

  if (flag)
    data[5] |= 0x10;
  else
    data[5] &= 0xef;
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


bool H224_Frame::DecodeAnnexQ(const BYTE *data, PINDEX size)
{
  if (!Q922_Frame::DecodeAnnexQ(data, size))
    return false;

  // doing some validity checks for H.224 frames
  BYTE highOrderAddressOctet = GetHighOrderAddressOctet();
  BYTE lowOrderAddressOctet = GetLowOrderAddressOctet();
  BYTE controlFieldOctet = GetControlFieldOctet();

  if (  highOrderAddressOctet != 0x00 ||
      !(lowOrderAddressOctet == 0x61 || lowOrderAddressOctet == 0x71) ||
        controlFieldOctet != 0x03)
    return false;

  return true;

}


bool H224_Frame::DecodeHDLC(const BYTE *data, PINDEX size)
{
  if (!Q922_Frame::DecodeHDLC(data, size))
    return false;

  // doing some validity checks for H.224 frames
  BYTE highOrderAddressOctet = GetHighOrderAddressOctet();
  BYTE lowOrderAddressOctet = GetLowOrderAddressOctet();
  BYTE controlFieldOctet = GetControlFieldOctet();

  if (  highOrderAddressOctet != 0x00 ||
      !(lowOrderAddressOctet == 0x61 || lowOrderAddressOctet == 0x71) ||
        controlFieldOctet != 0x03)
    return false;

  return true;
}


PINDEX H224_Frame::GetHeaderSize() const
{
  BYTE clientID = GetClientID();

  if (clientID < OpalH224Client::ExtendedClientID)
    return 6;

  if (clientID == OpalH224Client::ExtendedClientID)
    return 7; // one extra octet

  return 11; // 5 extra octets
}


////////////////////////////////////

OpalH224Handler::OpalH224Handler()
  : m_canTransmit(false)
  , m_transmitHDLCTunneling(false)
  , m_transmitBitIndex(7)
  , m_transmitStartTime(0)
  , m_transmitMediaStream(NULL)
{
  m_clients.DisallowDeleteObjects();
}


OpalH224Handler::~OpalH224Handler()
{
}


bool OpalH224Handler::AddClient(OpalH224Client & client)
{
  if (client.GetClientID() == OpalH224Client::CMEClientID)
    return false; // No client may have CMEClientID

  if (m_clients.GetObjectsIndex(&client) != P_MAX_INDEX)
    return false; // Only allow one instance of a client

  m_clients.Append(&client);
  client.SetH224Handler(this);
  return true;
}


bool OpalH224Handler::RemoveClient(OpalH224Client & client)
{
  if (!m_clients.Remove(&client))
    return false;

  client.SetH224Handler(NULL);
  return true;
}


void OpalH224Handler::SetTransmitMediaStream(OpalH224MediaStream * mediaStream)
{
  PWaitAndSignal m(m_transmitMutex);

  m_transmitMediaStream = mediaStream;
  if (mediaStream != NULL)
    m_transmitHDLCTunneling = mediaStream->GetMediaFormat().GetOptionBoolean(OpalH224MediaFormat::HDLCTunnelingOption());
}


void OpalH224Handler::StartTransmit()
{
  PWaitAndSignal m(m_transmitMutex);

  if (m_canTransmit)
    return;

  PTRACE(4, "Starting transmission");
  m_canTransmit = true;

  m_transmitBitIndex = 7;
  m_transmitStartTime.SetCurrentTime();

  SendClientList();
  SendExtraCapabilities();
}


void OpalH224Handler::StopTransmit()
{
  PWaitAndSignal m(m_transmitMutex);
  m_canTransmit = false;
  PTRACE(4, "Stopping transmission");
}


bool OpalH224Handler::SendClientList()
{
  PWaitAndSignal m(m_transmitMutex);

  // If all clients are non-standard, 5 octets per clients + 3 octets header information
  H224_Frame h224Frame = H224_Frame(5*m_clients.GetSize() + 3);

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
  ptr[2] = (BYTE)m_clients.GetSize();

  PINDEX dataIndex = 3;
  for (PINDEX i = 0; i < m_clients.GetSize(); i++) {
    OpalH224Client & client = m_clients[i];

    BYTE clientID = client.GetClientID();

    if (client.HasExtraCapabilities())
      ptr[dataIndex] = (0x80 | clientID);
    else
      ptr[dataIndex] = (0x7f & clientID);
    dataIndex++;

    if (clientID == OpalH224Client::ExtendedClientID) {
      ptr[dataIndex] = client.GetExtendedClientID();
      dataIndex++;

    }
    else if (clientID == OpalH224Client::NonStandardClientID) {

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

  return TransmitFrame(h224Frame);
}


bool OpalH224Handler::SendExtraCapabilities()
{
  for (PINDEX i = 0; i < m_clients.GetSize(); i++)
    m_clients[i].SendExtraCapabilities();

  return true;
}


bool OpalH224Handler::SendClientListCommand()
{
  PWaitAndSignal m(m_transmitMutex);

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

  return TransmitFrame(h224Frame);
}


bool OpalH224Handler::SendExtraCapabilitiesCommand(const OpalH224Client & client)
{
  PWaitAndSignal m(m_transmitMutex);

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

  return TransmitClientFrame(client, h224Frame);
}


bool OpalH224Handler::SendExtraCapabilitiesMessage(const OpalH224Client & client,
                                                       BYTE *data, PINDEX length)
{
  PWaitAndSignal m(m_transmitMutex);

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

  if (clientID < OpalH224Client::ExtendedClientID)
    headerSize = 3;
  else if (clientID == OpalH224Client::ExtendedClientID) {
    ptr[3] = client.GetExtendedClientID();
    headerSize = 4;
  }
  else {
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

  return TransmitClientFrame(client, h224Frame);
}


bool OpalH224Handler::TransmitClientFrame(const OpalH224Client & client, H224_Frame & frame)
{
  PWaitAndSignal m(m_transmitMutex);

  if (m_clients.GetObjectsIndex(&client) == P_MAX_INDEX) {
    PTRACE(4, "Client not registered");
    return false;
  }

  return TransmitFrame(frame);
}


bool OpalH224Handler::OnReceivedFrame(H224_Frame & frame)
{
  if (frame.GetDestinationTerminalAddress() != OpalH224Handler::Broadcast) {
    // only broadcast frames are handled at the moment
    PTRACE(3, "Received frame with non-broadcast address");
    return true;
  }

  if (frame.GetClientID() == OpalH224Client::CMEClientID)
    return OnReceivedCMEMessage(frame);

  OpalH224Client * client = FindClient(frame.GetClientID(),
                                       frame.GetExtendedClientID(),
                                       frame.GetCountryCode(),
                                       frame.GetCountryCodeExtension(),
                                       frame.GetManufacturerCode(),
                                       frame.GetManufacturerClientID());
  if (client != NULL) {
    PTRACE(4, "Received message for client " << (unsigned)client->GetClientID());
    client->OnReceivedMessage(frame);
    return true;
  }

  // ignore if no client found
  PTRACE(4, "Ignored message for client " << (unsigned)frame.GetClientID());
  return true;
}


OpalH224Client * OpalH224Handler::FindClient(BYTE clientID,
                                             BYTE extendedClientID,
                                             BYTE countryCode,
                                             BYTE countryCodeExtension,
                                             WORD manufacturerCode,
                                             BYTE manufacturerClientID) const
{
  for (PINDEX i = 0; i < m_clients.GetSize(); i++) {
    OpalH224Client & client = m_clients[i];
    if (client.GetClientID() == clientID) {
      if (clientID < OpalH224Client::ExtendedClientID)
        return &client;

      if (clientID == OpalH224Client::ExtendedClientID && client.GetExtendedClientID() == extendedClientID)
        return &client;

      if (client.GetCountryCode() == countryCode &&
          client.GetCountryCodeExtension() == countryCodeExtension &&
          client.GetManufacturerCode() == manufacturerCode &&
          client.GetManufacturerClientID() == manufacturerClientID)
        return &client;
    }
  }

  return NULL;
}


bool OpalH224Handler::OnReceivedCMEMessage(H224_Frame & frame)
{
  BYTE *data = frame.GetClientDataPtr();

  switch (data[0]) {
    case CMEClientListCode :
      switch (data[1]) {
        case CMEMessage :
          return OnReceivedClientList(frame);
        case CMECommand :
          return OnReceivedClientListCommand();
      }
      break;

    case CMEExtraCapabilitiesCode :
      switch (data[1]) {
        case CMEMessage:
          return OnReceivedExtraCapabilities(frame);
        case CMECommand:
          return OnReceivedExtraCapabilitiesCommand();
      }
  }

  // incorrect frames are simply ignored
  PTRACE(4, "Ignored unknown CME message.");
  return true;
}


bool OpalH224Handler::OnReceivedClientList(H224_Frame & frame)
{
  // First, reset all clients
  for (PINDEX i = 0; i < m_clients.GetSize(); i++)
    m_clients[i].SetRemoteClientAvailable(false, false);

  BYTE *data = frame.GetClientDataPtr();

  BYTE numberOfClients = data[2];
  PTRACE(4, "Received client list: " << (unsigned)numberOfClients);

  PINDEX dataIndex = 3;

  while (numberOfClients > 0) {

    BYTE clientID = (data[dataIndex] & 0x7f);
    bool hasExtraCapabilities = (data[dataIndex] & 0x80) != 0 ? true: false;
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

    OpalH224Client * client = FindClient(clientID,
                                         extendedClientID,
                                         countryCode,
                                         countryCodeExtension,
                                         manufacturerCode,
                                         manufacturerClientID);
    if (client != NULL)
      client->SetRemoteClientAvailable(true, hasExtraCapabilities);

    numberOfClients--;
  }

  return true;
}


bool OpalH224Handler::OnReceivedClientListCommand()
{
  PTRACE(4, "Handling request to send client list");
  SendClientList();
  return true;
}


bool OpalH224Handler::OnReceivedExtraCapabilities(H224_Frame & frame)
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

  OpalH224Client * client = FindClient(clientID,
                                       extendedClientID,
                                       countryCode,
                                       countryCodeExtension,
                                       manufacturerCode,
                                       manufacturerClientID);
  if (client != NULL) {
    PINDEX size = frame.GetClientDataSize() - dataIndex;
    client->SetRemoteClientAvailable(true, true);
    PTRACE(4, "Extra capabilities for client " << (unsigned)clientID << ", size=" << size);
    client->OnReceivedExtraCapabilities((data + dataIndex), size);
    return true;
  }

  // Simply ignore if no client is available for this clientID
  PTRACE(4, "Ignoring extra capabilities for client " << (unsigned)clientID);
  return true;
}


bool OpalH224Handler::OnReceivedExtraCapabilitiesCommand()
{
  PTRACE(4, "Handling request to send extra capabilities");
  SendExtraCapabilities();
  return true;
}


bool OpalH224Handler::TransmitFrame(H224_Frame & frame)
{
  if (m_transmitMediaStream == NULL) {
    PTRACE(4, "Transmit stream not attached");
    return false;
  }

  if (!m_canTransmit) {
    PTRACE(4, "Transmitter not started");
    return false;
  }

  PINDEX size = m_transmitHDLCTunneling ? frame.GetHDLCEncodedSize() : frame.GetAnnexQEncodedSize();
  RTP_DataFrame transmitFrame(size);
  transmitFrame.SetPayloadType(m_transmitMediaStream->GetMediaFormat().GetPayloadType());

  if (!(m_transmitHDLCTunneling ? frame.EncodeHDLC(transmitFrame.GetPayloadPtr(), size, m_transmitBitIndex)
                                : frame.EncodeAnnexQ(transmitFrame.GetPayloadPtr(), size))) {
    PTRACE(1, "Failed to encode the frame");
    return false;
  }

  // determining correct timestamp
  PTimeInterval timePassed = PTime() - m_transmitStartTime;
  transmitFrame.SetTimestamp((DWORD)timePassed.GetMilliSeconds() * 8);

  transmitFrame.SetPayloadSize(size);
  transmitFrame.SetMarker(true);

  PTRACE(4, "Sending frame: " << transmitFrame);
  m_transmitMediaStream->PushPacket(transmitFrame);
  return true;
}


////////////////////////////////////

OpalH224MediaStream::OpalH224MediaStream(OpalConnection & connection,
                                         OpalH224Handler & handler,
                                         const OpalMediaFormat & mediaFormat,
                                         unsigned sessionID,
                                         bool isSource)
  : OpalMediaStream(connection, mediaFormat, sessionID, isSource)
  , m_h224Handler(handler)
  , m_consecutiveErrors(0)
{
  if (isSource)
    m_h224Handler.SetTransmitMediaStream(this);
}


OpalH224MediaStream::~OpalH224MediaStream()
{
  Close();
}


void OpalH224MediaStream::OnStartMediaPatch()
{
  if (IsSource())
    m_h224Handler.StartTransmit();
  OpalMediaStream::OnStartMediaPatch();
}


void OpalH224MediaStream::InternalClose()
{
  if (IsSource()) {
    m_h224Handler.StopTransmit();
    m_h224Handler.SetTransmitMediaStream(NULL);
  }
}


PBoolean OpalH224MediaStream::ReadPacket(RTP_DataFrame & /*packet*/)
{
  return false;
}


PBoolean OpalH224MediaStream::WritePacket(RTP_DataFrame & packet)
{
  PTRACE(4, "Received frame: " << packet);

  H224_Frame receiveFrame;
  if (m_mediaFormat.GetOptionBoolean(OpalH224MediaFormat::HDLCTunnelingOption())
            ? receiveFrame.DecodeHDLC(packet.GetPayloadPtr(), packet.GetPayloadSize())
            : receiveFrame.DecodeAnnexQ(packet.GetPayloadPtr(), packet.GetPayloadSize())) {
    m_consecutiveErrors = 0;
    return m_h224Handler.OnReceivedFrame(receiveFrame);
  }

  PTRACE(m_consecutiveErrors > 0 ? 4 : 2, "Decoding of frame failed");
  if (++m_consecutiveErrors < 10)
    return true;

  PTRACE(2, "Too many consecutive decode errors, shutting down channel");
  return false;
}


////////////////////////////////////

OpalH224Client::OpalH224Client()
  : m_remoteClientAvailable(false)
  , m_remoteClientHasExtraCapabilities(false)
  , m_h224Handler(NULL)
{
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


void OpalH224Client::SetRemoteClientAvailable(bool available, bool hasExtraCapabilities)
{
  m_remoteClientAvailable = available;
  m_remoteClientHasExtraCapabilities = hasExtraCapabilities;
}


#endif // OPAL_HAS_H224
