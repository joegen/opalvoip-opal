/*
 * h224.cxx
 *
 * H.224 implementation for the OpenH323 Project.
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
 * $Log: h224.cxx,v $
 * Revision 1.3.6.2  2007/03/20 00:02:13  hfriederich
 * (Backport from HEAD)
 * Add ability to remove H.224
 *
 * Revision 1.3.6.1  2007/02/07 08:51:02  hfriederich
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
#pragma implementation "h224.h"
#pragma implementation "h224handler.h"
#endif

#include <opal/buildopts.h>

#if OPAL_H224

#include <h224/h224mediafmt.h>
#include <h224/h224.h>
#include <h224/h224handler.h>
#include <h323/h323con.h>

#define H224_MAX_HEADER_SIZE 6+5

#define CME_CLIENT_ID 0x00

#define CME_CLIENT_LIST_CODE 0x01
#define CME_EXTRA_CAPABILITIES_CODE 0x02

#define CME_RESPONSE_MESSAGE 0x00
#define CME_RESPONSE_COMMAND 0xff

H224_Frame::H224_Frame(PINDEX size)
: Q922_Frame(H224_MAX_HEADER_SIZE + size)
{
  SetHighPriority(FALSE);	
  SetControlFieldOctet(0x03); // UI-Mode
  SetDestinationTerminalAddress(H224_BROADCAST);
  SetSourceTerminalAddress(H224_BROADCAST);

  // setting Client ID to CME
  SetClientID(CME_CLIENT_ID);
	
  // setting ES / BS / C1 / C0 / Segment number to zero
  SetBS(FALSE);
  SetES(FALSE);
  SetC1(FALSE);
  SetC0(FALSE);
  SetSegmentNumber(0x00);
  
  SetClientDataSize(size);
}

H224_Frame::H224_Frame(const OpalH224Client & h224Client, PINDEX size)
: Q922_Frame(H224_MAX_HEADER_SIZE + size)
{
  SetHighPriority(FALSE); 
  SetControlFieldOctet(0x03); // UI-Mode;
  SetDestinationTerminalAddress(H224_BROADCAST);
  SetSourceTerminalAddress(H224_BROADCAST);
	
  SetClient(h224Client);
	
  SetBS(FALSE);
  SetES(FALSE);
  SetC1(FALSE);
  SetC0(FALSE);
  SetSegmentNumber(0x00);
	
  SetClientDataSize(size);
}

H224_Frame::~H224_Frame()
{
}

void H224_Frame::SetHighPriority(BOOL flag)
{
  SetHighOrderAddressOctet(0x00);
	
  if(flag) {
    SetLowOrderAddressOctet(0x71);
  } else {
    SetLowOrderAddressOctet(0x061);
  }
}

WORD H224_Frame::GetDestinationTerminalAddress() const
{
  BYTE *data = GetInformationFieldPtr();
  return (((WORD)data[0] << 8) | (WORD)data[1]);
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
  return (((WORD)data[2] << 8) | (WORD)data[3]);
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
	
  if(clientID == EXTENDED_CLIENT_ID) {
    SetExtendedClientID(h224Client.GetExtendedClientID());
  } else if(clientID == NON_STANDARD_CLIENT_ID) {
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
  if(GetClientID() != EXTENDED_CLIENT_ID) {
    return 0x00;
  }
	
  BYTE *data = GetInformationFieldPtr();
  return data[5];
}

void H224_Frame::SetExtendedClientID(BYTE extendedClientID)
{
  if(GetClientID() != EXTENDED_CLIENT_ID) {
    return;
  }
	
  BYTE *data = GetInformationFieldPtr();
  data[5] = extendedClientID;
}

BYTE H224_Frame::GetCountryCode() const
{
  if(GetClientID() != NON_STANDARD_CLIENT_ID) {
    return 0x00;
  }
	
  BYTE *data = GetInformationFieldPtr();
  return data[5];
}

BYTE H224_Frame::GetCountryCodeExtension() const
{
  if(GetClientID() != NON_STANDARD_CLIENT_ID) {
    return 0x00;
  }
	
  BYTE *data = GetInformationFieldPtr();
  return data[6];
}

WORD H224_Frame::GetManufacturerCode() const
{
  if(GetClientID() != NON_STANDARD_CLIENT_ID) {
    return 0x0000;
  }
	
  BYTE *data = GetInformationFieldPtr();
  return (((WORD)data[7] << 8) | (WORD)data[8]);
}

BYTE H224_Frame::GetManufacturerClientID() const
{
  if(GetClientID() != NON_STANDARD_CLIENT_ID) {
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
  if(GetClientID() != NON_STANDARD_CLIENT_ID) {	
    return;
  }
	
  BYTE *data = GetInformationFieldPtr();
	
  data[5] = countryCode;
  data[6] = countryCodeExtension;
  data[7] = (BYTE)(manufacturerCode << 8);
  data[8] = (BYTE) manufacturerCode;
  data[9] = manufacturerClientID;
}

BOOL H224_Frame::GetBS() const
{
  BYTE *data = GetInformationFieldPtr();
  PINDEX dataIndex = GetHeaderSize() - 1;
	
  return (data[dataIndex] & 0x80) != 0;
}

void H224_Frame::SetBS(BOOL flag)
{
  BYTE *data = GetInformationFieldPtr();
  PINDEX dataIndex = GetHeaderSize() - 1;
	
  if(flag) {
    data[dataIndex] |= 0x80;
  }	else {
    data[dataIndex] &= 0x7f;
  }
}

BOOL H224_Frame::GetES() const
{
  BYTE *data = GetInformationFieldPtr();
  PINDEX dataIndex = GetHeaderSize() - 1;
	
  return (data[dataIndex] & 0x40) != 0;
}

void H224_Frame::SetES(BOOL flag)
{
  BYTE *data = GetInformationFieldPtr();
  PINDEX dataIndex = GetHeaderSize() - 1;
	
  if(flag) {
    data[dataIndex] |= 0x40;
  } else {
    data[dataIndex] &= 0xbf;
  }
}

BOOL H224_Frame::GetC1() const
{
  BYTE *data = GetInformationFieldPtr();
  PINDEX dataIndex = GetHeaderSize() - 1;
	
  return (data[dataIndex] & 0x20) != 0;
}

void H224_Frame::SetC1(BOOL flag)
{
  BYTE *data = GetInformationFieldPtr();
  PINDEX dataIndex = GetHeaderSize() - 1;
	
  if(flag) {
    data[dataIndex] |= 0x20;
  } else {
    data[dataIndex] &= 0xdf;
  }
}

BOOL H224_Frame::GetC0() const
{
  BYTE *data = GetInformationFieldPtr();
  PINDEX dataIndex = GetHeaderSize() - 1;
	
  return (data[dataIndex] & 0x10) != 0;
}

void H224_Frame::SetC0(BOOL flag)
{
  BYTE *data = GetInformationFieldPtr();
  PINDEX dataIndex = GetHeaderSize() - 1;
	
  if(flag) {
    data[dataIndex] |= 0x10;
  }	else {
    data[dataIndex] &= 0xef;
  }
}

BYTE H224_Frame::GetSegmentNumber() const
{
  BYTE *data = GetInformationFieldPtr();
  PINDEX dataIndex = GetHeaderSize() - 1;
	
  return (data[dataIndex] & 0x0f);
}

void H224_Frame::SetSegmentNumber(BYTE segmentNumber)
{
  BYTE *data = GetInformationFieldPtr();
  PINDEX dataIndex = GetHeaderSize() - 1;
	
  data[dataIndex] &= 0xf0;
  data[dataIndex] |= (segmentNumber & 0x0f);
}

BYTE * H224_Frame::GetClientDataPtr() const
{
  BYTE *data = GetInformationFieldPtr();
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

BOOL H224_Frame::Decode(const BYTE *data, 
						PINDEX size)
{
  BOOL result = Q922_Frame::Decode(data, size);
	
  if(result == FALSE) {
	return FALSE;
  }
	
  // doing some validity check for H.224 frames
  BYTE highOrderAddressOctet = GetHighOrderAddressOctet();
  BYTE lowOrderAddressOctet = GetLowOrderAddressOctet();
  BYTE controlFieldOctet = GetControlFieldOctet();
	
  if((highOrderAddressOctet != 0x00) ||
     (!(lowOrderAddressOctet == 0x61 || lowOrderAddressOctet == 0x71)) ||
     (controlFieldOctet != 0x03)) {
	  
    return FALSE;
  }
	
  return TRUE;
}

PINDEX H224_Frame::GetHeaderSize() const
{
  BYTE clientID = GetClientID();
	
  if(clientID < EXTENDED_CLIENT_ID) {
    return 6;
  } else if(clientID == EXTENDED_CLIENT_ID) {
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
  canTransmit = FALSE;
  transmitBitIndex = 7;
  transmitStartTime = NULL;
  transmitMediaStream = NULL;
	
  clients.DisallowDeleteObjects();
}

OpalH224Handler::~OpalH224Handler()
{
}

BOOL OpalH224Handler::AddClient(OpalH224Client & client)
{
  if(client.GetClientID() == CME_CLIENT_ID)
  {
    return FALSE; // No client may have CME_CLIENT_ID
  }
	
  if(clients.GetObjectsIndex(&client) != P_MAX_INDEX) {
    return FALSE; // Only allow one instance of a client
  }
	
  clients.Append(&client);
  client.SetH224Handler(this);
  return TRUE;
}

BOOL OpalH224Handler::RemoveClient(OpalH224Client & client)
{
  BOOL result = clients.Remove(&client);
  if(result == TRUE) {
    client.SetH224Handler(NULL);
  }
  return result;
}

void OpalH224Handler::SetTransmitMediaStream(OpalH224MediaStream * mediaStream)
{
  PWaitAndSignal m(transmitMutex);
	
  transmitMediaStream = mediaStream;
	
  if(transmitMediaStream != NULL) {
    transmitFrame.SetPayloadType(transmitMediaStream->GetMediaFormat().GetPayloadType());
  }
}

void OpalH224Handler::StartTransmit()
{
  PWaitAndSignal m(transmitMutex);
	
  if(canTransmit == TRUE) {
    return;
  }
	
  canTransmit = TRUE;
  
  transmitBitIndex = 7;
  transmitStartTime = new PTime();
	
  SendClientList();
  SendExtraCapabilities();
}

void OpalH224Handler::StopTransmit()
{
  PWaitAndSignal m(transmitMutex);
	
  if(canTransmit == FALSE) {
    return;
  }
	
  delete transmitStartTime;
  transmitStartTime = NULL;
	
  canTransmit = FALSE;
}

BOOL OpalH224Handler::SendClientList()
{
  PWaitAndSignal m(transmitMutex);
	
  if(canTransmit == FALSE) {
    return FALSE;
  }
	
  // If all clients are non-standard, 5 octets per clients + 3 octets header information
  H224_Frame h224Frame = H224_Frame(5*clients.GetSize() + 3);
  
  h224Frame.SetHighPriority(TRUE);
  h224Frame.SetDestinationTerminalAddress(H224_BROADCAST);
  h224Frame.SetSourceTerminalAddress(H224_BROADCAST);
	
  // CME frame
  h224Frame.SetClientID(CME_CLIENT_ID);
	
  // Begin and end of sequence
  h224Frame.SetBS(TRUE);
  h224Frame.SetES(TRUE);
  h224Frame.SetC1(FALSE);
  h224Frame.SetC0(FALSE);
  h224Frame.SetSegmentNumber(0);
	
  BYTE *ptr = h224Frame.GetClientDataPtr();
	
  ptr[0] = CME_CLIENT_LIST_CODE;
  ptr[1] = CME_RESPONSE_MESSAGE;
  ptr[2] = (BYTE)clients.GetSize();
  
  PINDEX dataIndex = 3;
  for (PINDEX i = 0; i < clients.GetSize(); i++) {
    OpalH224Client & client = clients[i];
	  
    BYTE clientID = client.GetClientID();
	  
    if(client.HasExtraCapabilities()) {
      ptr[dataIndex] = (0x80 | clientID);
    } else {
      ptr[dataIndex] = (0x7f & clientID);
    }
    dataIndex++;
	
	  
    if(clientID == EXTENDED_CLIENT_ID) {
      ptr[dataIndex] = client.GetExtendedClientID();
      dataIndex++;
		  
    } else if(clientID == NON_STANDARD_CLIENT_ID) {
		  
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
	
  return TRUE;
}

BOOL OpalH224Handler::SendExtraCapabilities()
{		
  for(PINDEX i = 0; i < clients.GetSize(); i++) {
    OpalH224Client & client = clients[i];
    client.SendExtraCapabilities();
  }
	
  return TRUE;
}

BOOL OpalH224Handler::SendClientListCommand()
{
  PWaitAndSignal m(transmitMutex);
	
  if(canTransmit == FALSE) {
    return FALSE;
  }
	
  H224_Frame h224Frame = H224_Frame(2);
  h224Frame.SetHighPriority(TRUE);
  h224Frame.SetDestinationTerminalAddress(H224_BROADCAST);
  h224Frame.SetSourceTerminalAddress(H224_BROADCAST);
	
  // CME frame
  h224Frame.SetClientID(CME_CLIENT_ID);
	
  // Begin and end of sequence
  h224Frame.SetBS(TRUE);
  h224Frame.SetES(TRUE);
  h224Frame.SetC1(FALSE);
  h224Frame.SetC0(FALSE);
  h224Frame.SetSegmentNumber(0);
	
  BYTE *ptr = h224Frame.GetClientDataPtr();
	
  ptr[0] = CME_CLIENT_LIST_CODE;
  ptr[1] = CME_RESPONSE_COMMAND;
	
  TransmitFrame(h224Frame);
	
  return TRUE;
}

BOOL OpalH224Handler::SendExtraCapabilitiesCommand(const OpalH224Client & client)
{
  PWaitAndSignal m(transmitMutex);
	
  if(canTransmit == FALSE) {
    return FALSE;
  }
	
  if(clients.GetObjectsIndex(&client) == P_MAX_INDEX) {
    return FALSE; // Only allow if the client is really registered
  }
	
  H224_Frame h224Frame = H224_Frame(8);
  h224Frame.SetHighPriority(TRUE);
  h224Frame.SetDestinationTerminalAddress(H224_BROADCAST);
  h224Frame.SetSourceTerminalAddress(H224_BROADCAST);
	
  // CME frame
  h224Frame.SetClientID(CME_CLIENT_ID);
	
  // Begin and end of sequence
  h224Frame.SetBS(TRUE);
  h224Frame.SetES(TRUE);
  h224Frame.SetC1(FALSE);
  h224Frame.SetC0(FALSE);
  h224Frame.SetSegmentNumber(0);
	
  BYTE *ptr = h224Frame.GetClientDataPtr();
	
  ptr[0] = CME_EXTRA_CAPABILITIES_CODE;
  ptr[1] = CME_RESPONSE_COMMAND;
  
  PINDEX dataSize;
  
  BYTE extendedCapabilitiesFlag = client.HasExtraCapabilities() ? 0x80 : 0x00;
  BYTE clientID = client.GetClientID();
  ptr[2] = (extendedCapabilitiesFlag | (clientID & 0x7f));
  
  if(clientID < EXTENDED_CLIENT_ID) {
    dataSize = 3;
  } else if(clientID == EXTENDED_CLIENT_ID) {
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
	
  return TRUE;
}

BOOL OpalH224Handler::SendExtraCapabilitiesMessage(const OpalH224Client & client,
												   BYTE *data, PINDEX length)
{	
  PWaitAndSignal m(transmitMutex);
	
  if(clients.GetObjectsIndex(&client) == P_MAX_INDEX) {
    return FALSE; // Only allow if the client is really registered
  }
	
  H224_Frame h224Frame = H224_Frame(length+8);
  h224Frame.SetHighPriority(TRUE);
  h224Frame.SetDestinationTerminalAddress(H224_BROADCAST);
  h224Frame.SetSourceTerminalAddress(H224_BROADCAST);
	
  h224Frame.SetClientID(CME_CLIENT_ID);
	
  // Begin and end of sequence, rest is zero
  h224Frame.SetBS(TRUE);
  h224Frame.SetES(TRUE);
  h224Frame.SetC1(FALSE);
  h224Frame.SetC0(FALSE);
  h224Frame.SetSegmentNumber(0);
	
  BYTE *ptr = h224Frame.GetClientDataPtr();
	
  ptr[0] = CME_EXTRA_CAPABILITIES_CODE;
  ptr[1] = CME_RESPONSE_MESSAGE;
  
  PINDEX headerSize;
  BYTE clientID = client.GetClientID();
  BYTE extendedCapabilitiesFlag = client.HasExtraCapabilities() ? 0x80 : 0x00;
  
  ptr[2] = (extendedCapabilitiesFlag | (clientID & 0x7f));
  
  if(clientID < EXTENDED_CLIENT_ID) {
    headerSize = 3;
  } else if(clientID == EXTENDED_CLIENT_ID) {
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
	
  memcpy(ptr+headerSize, data, length);
  
  h224Frame.SetClientDataSize(length+headerSize);
	
  TransmitFrame(h224Frame);
	
  return TRUE;	
}

BOOL OpalH224Handler::TransmitClientFrame(const OpalH224Client & client, H224_Frame & frame)
{
  PWaitAndSignal m(transmitMutex);
	
  if(clients.GetObjectsIndex(&client) == P_MAX_INDEX) {
    return FALSE; // Only allow if the client is really registered
  }
	
  TransmitFrame(frame);
	
  return TRUE;
}

BOOL OpalH224Handler::OnReceivedFrame(H224_Frame & frame)
{
  if(frame.GetDestinationTerminalAddress() != H224_BROADCAST) {
    // only broadcast frames are handled at the moment
	PTRACE(3, "Received H.224 frame with non-broadcast address");
    return TRUE;
  }
  BYTE clientID = frame.GetClientID();
	
  if(clientID == CME_CLIENT_ID) {
    return OnReceivedCMEMessage(frame);
  }
  
  for(PINDEX i = 0; i < clients.GetSize(); i++) {
    OpalH224Client & client = clients[i];
    if(client.GetClientID() == clientID) {
      BOOL found = FALSE;
      if(clientID < EXTENDED_CLIENT_ID) {
        found = TRUE;
      } else if(clientID == EXTENDED_CLIENT_ID) {
        if(client.GetExtendedClientID() == frame.GetExtendedClientID()) {
          found = TRUE;
        }
      } else {
        if(client.GetCountryCode() == frame.GetCountryCode() &&
           client.GetCountryCodeExtension() == frame.GetCountryCodeExtension() &&
           client.GetManufacturerCode() == frame.GetManufacturerCode() &&
           client.GetManufacturerClientID() == frame.GetManufacturerClientID()) {
          found = TRUE;
        }
      }
      if(found == TRUE) {
        client.OnReceivedMessage(frame);
        return TRUE;
      }
    }
  }
  
  // ignore if no corresponding client found
	
  return TRUE;
}

BOOL OpalH224Handler::OnReceivedCMEMessage(H224_Frame & frame)
{
  BYTE *data = frame.GetClientDataPtr();
	
  if(data[0] == CME_CLIENT_LIST_CODE) {
	
    if(data[1] == CME_RESPONSE_MESSAGE) {
      return OnReceivedClientList(frame);
		
    } else if(data[1] == CME_RESPONSE_COMMAND) {
      return OnReceivedClientListCommand();
    }
	  
  } else if(data[0] == CME_EXTRA_CAPABILITIES_CODE) {
	  
    if(data[1] == CME_RESPONSE_MESSAGE) {
      return OnReceivedExtraCapabilities(frame);
		
    } else if(data[1] == CME_RESPONSE_COMMAND) {
      return OnReceivedExtraCapabilitiesCommand();
    }
  }
	
  // ignore incorrect frames
  return TRUE;
}

BOOL OpalH224Handler::OnReceivedClientList(H224_Frame & frame)
{
  // First, reset all clients
  for(PINDEX i = 0; i < clients.GetSize(); i++)
  {
    OpalH224Client & client = clients[i];
    client.SetRemoteClientAvailable(FALSE, FALSE);
  }
	
  BYTE *data = frame.GetClientDataPtr();
	
  BYTE numberOfClients = data[2];
	
  PINDEX dataIndex = 3;
	
  while(numberOfClients > 0) {
	  
	BYTE clientID = (data[dataIndex] & 0x7f);
	BOOL hasExtraCapabilities = (data[dataIndex] & 0x80) != 0 ? TRUE: FALSE;
	dataIndex++;
	BYTE extendedClientID = 0x00;
	BYTE countryCode = COUNTRY_CODE_ESCAPE;
	BYTE countryCodeExtension = 0x00;
	WORD manufacturerCode = 0x0000;
	BYTE manufacturerClientID = 0x00;
	
	if(clientID == EXTENDED_CLIENT_ID) {
      extendedClientID = data[dataIndex];
      dataIndex++;
	} else if(clientID == NON_STANDARD_CLIENT_ID) {
      countryCode = data[dataIndex];
      dataIndex++;
      countryCodeExtension = data[dataIndex];
      dataIndex++;
      manufacturerCode = (((WORD)data[dataIndex] << 8) | (WORD)data[dataIndex+1]);
      dataIndex += 2;
      manufacturerClientID = data[dataIndex];
      dataIndex++;
	}
	
	for(PINDEX i = 0; i < clients.GetSize(); i++) {
      OpalH224Client & client = clients[i];
      BOOL found = FALSE;
      if(client.GetClientID() == clientID) {
        if(clientID < EXTENDED_CLIENT_ID) {
          found = TRUE;
        } else if(clientID == EXTENDED_CLIENT_ID) {
          if(client.GetExtendedClientID() == extendedClientID) {
            found = TRUE;
          }
        } else {
          if(client.GetCountryCode() == countryCode &&
             client.GetCountryCodeExtension() == countryCodeExtension &&
             client.GetManufacturerCode() == manufacturerCode &&
             client.GetManufacturerClientID() == manufacturerClientID) {
            found = TRUE;
          }
        }
      }
      if(found == TRUE) {
        client.SetRemoteClientAvailable(TRUE, hasExtraCapabilities);
        break;
      }
	}
    numberOfClients--;
  }
	
  return TRUE;
}

BOOL OpalH224Handler::OnReceivedClientListCommand()
{
  SendClientList();
  return TRUE;
}

BOOL OpalH224Handler::OnReceivedExtraCapabilities(H224_Frame & frame)
{
  BYTE *data = frame.GetClientDataPtr();
	
  BYTE clientID = (data[2] & 0x7f);
  PINDEX dataIndex;
  BYTE extendedClientID = 0x00;
  BYTE countryCode = COUNTRY_CODE_ESCAPE;
  BYTE countryCodeExtension = 0x00;
  WORD manufacturerCode = 0x0000;
  BYTE manufacturerClientID = 0x00;
  
  if(clientID < EXTENDED_CLIENT_ID) {
    dataIndex = 3;
  } else if(clientID == EXTENDED_CLIENT_ID) {
    extendedClientID = data[3];
    dataIndex = 4;
  } else if(clientID == NON_STANDARD_CLIENT_ID) {
    countryCode = data[3];
    countryCodeExtension = data[4];
    manufacturerCode = (((WORD)data[5] << 8) | (WORD)data[6]);
    manufacturerClientID = data[7];
    dataIndex = 8;
  }
  
  for(PINDEX i = 0; i < clients.GetSize(); i++) {
    OpalH224Client & client = clients[i];
    BOOL found = FALSE;
    if(client.GetClientID() == clientID) {
      if(clientID < EXTENDED_CLIENT_ID) {
        found = TRUE;
      } else if(clientID == EXTENDED_CLIENT_ID) {
        if(client.GetExtendedClientID() == extendedClientID) {
          found = TRUE;
        }
      } else {
        if(client.GetCountryCode() == countryCode &&
           client.GetCountryCodeExtension() == countryCodeExtension &&
           client.GetManufacturerCode() == manufacturerCode &&
           client.GetManufacturerClientID() == manufacturerClientID) {
          found = TRUE;
        }
      }
    }
    if(found == TRUE) {
      PINDEX size = frame.GetClientDataSize() - dataIndex;
      client.SetRemoteClientAvailable(TRUE, TRUE);
      client.OnReceivedExtraCapabilities((data + dataIndex), size);
      return TRUE;
    }
  }
  
  // Simply ignore if no client is available for this clientID
	
  return TRUE;
}

BOOL OpalH224Handler::OnReceivedExtraCapabilitiesCommand()
{
  SendExtraCapabilities();
  return TRUE;
}

BOOL OpalH224Handler::HandleFrame(const RTP_DataFrame & dataFrame)
{		
  if(receiveFrame.Decode(dataFrame.GetPayloadPtr(), dataFrame.GetPayloadSize())) {
    BOOL result = OnReceivedFrame(receiveFrame);
    return result;
  } else {
    PTRACE(3, "Decoding of H.224 frame failed");
    return FALSE;
  }
}

void OpalH224Handler::TransmitFrame(H224_Frame & frame)
{	
  PINDEX size = frame.GetEncodedSize();
	
  if(!frame.Encode(transmitFrame.GetPayloadPtr(), size, transmitBitIndex)) {
    PTRACE(3, "Failed to encode H.224 frame");
    return;
  }
	
  // determining correct timestamp
  PTime currentTime = PTime();
  PTimeInterval timePassed = currentTime - *transmitStartTime;
  transmitFrame.SetTimestamp((DWORD)timePassed.GetMilliSeconds() * 8);
  
  transmitFrame.SetPayloadSize(size);
  transmitFrame.SetMarker(TRUE);
  
  transmitMediaStream->PushPacket(transmitFrame);
}

////////////////////////////////////

OpalH224MediaStream::OpalH224MediaStream(OpalH224Handler & handler,
										 const OpalMediaFormat & mediaFormat,
										 BOOL isSource)
: OpalMediaStream(mediaFormat, isSource),
  h224Handler(handler)
{
  if(isSource == TRUE) {
    h224Handler.SetTransmitMediaStream(this);
  }
}

OpalH224MediaStream::~OpalH224MediaStream()
{
}

void OpalH224MediaStream::OnPatchStart()
{	
  h224Handler.StartTransmit();
}

BOOL OpalH224MediaStream::Close()
{
  if(OpalMediaStream::Close() == FALSE) {
    return FALSE;
  }
	
  if(IsSource()) {
    h224Handler.StopTransmit();
    h224Handler.SetTransmitMediaStream(NULL);
  }
	
  return TRUE;
}

BOOL OpalH224MediaStream::ReadPacket(RTP_DataFrame & packet)
{
  return FALSE;
}

BOOL OpalH224MediaStream::WritePacket(RTP_DataFrame & packet)
{
  return h224Handler.HandleFrame(packet);
}

////////////////////////////////////

OpalH224Client::OpalH224Client()
{
  remoteClientAvailable = FALSE;
  remoteClientHasExtraCapabilities = FALSE;
  h224Handler = NULL;
}

OpalH224Client::~OpalH224Client()
{
}

PObject::Comparison OpalH224Client::Compare(const PObject & obj)
{
  if(!PIsDescendant(&obj, OpalH224Client)) {
    return LessThan;
  }
	
  const OpalH224Client & otherClient = (const OpalH224Client &) obj;
	
  BYTE clientID = GetClientID();
  BYTE otherClientID = otherClient.GetClientID();
	
  if(clientID < otherClientID) {
    return LessThan;
  } else if(clientID > otherClientID) {
    return GreaterThan;
  }
	
  if(clientID < EXTENDED_CLIENT_ID) {
    return EqualTo;
  }
	
  if(clientID == EXTENDED_CLIENT_ID) {
    BYTE extendedClientID = GetExtendedClientID();
    BYTE otherExtendedClientID = otherClient.GetExtendedClientID();
		
    if(extendedClientID < otherExtendedClientID) {
      return LessThan;
    } else if(extendedClientID > otherExtendedClientID) {
      return GreaterThan;
    } else {
      return EqualTo;
    }
  }
	
  // Non-standard client.
  // Compare country code, extended country code, manufacturer code, manufacturer client ID
  BYTE countryCode = GetCountryCode();
  BYTE otherCountryCode = otherClient.GetCountryCode();
  if(countryCode < otherCountryCode) {
    return LessThan;
  } else if(countryCode > otherCountryCode) {
    return GreaterThan;
  }
	
  BYTE countryCodeExtension = GetCountryCodeExtension();
  BYTE otherCountryCodeExtension = otherClient.GetCountryCodeExtension();
  if(countryCodeExtension < otherCountryCodeExtension) {
    return LessThan;
  } else if(countryCodeExtension > otherCountryCodeExtension) {
    return GreaterThan;
  }
	
  WORD manufacturerCode = GetManufacturerCode();
  WORD otherManufacturerCode = otherClient.GetManufacturerCode();
  if(manufacturerCode < otherManufacturerCode) {
    return LessThan;
  } else if(manufacturerCode > otherManufacturerCode) {
    return GreaterThan;
  }
	
  BYTE manufacturerClientID = GetManufacturerClientID();
  BYTE otherManufacturerClientID = otherClient.GetManufacturerClientID();
  
  if(manufacturerClientID < otherManufacturerClientID) {
    return LessThan;
  } else if(manufacturerClientID > otherManufacturerClientID) {
    return GreaterThan;
  }
	
  return EqualTo;
}

void OpalH224Client::SetRemoteClientAvailable(BOOL available, BOOL hasExtraCapabilities)
{
  remoteClientAvailable = available;
  remoteClientHasExtraCapabilities = hasExtraCapabilities;
}

#endif // OPAL_H224
