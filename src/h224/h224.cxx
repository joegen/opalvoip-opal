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

#ifdef __GNUC__
#pragma implementation "h224.h"
#pragma implementation "h224handler.h"
#endif

#include <opal/buildopts.h>

#if OPAL_H224FECC

#include <h224/h224.h>
#include <h224/h224handler.h>
#include <h323/h323con.h>

/////////////////////////////////////////////////////////////////////////

OpalH224MediaType::OpalH224MediaType()
  : OpalRTPAVPMediaType("h224", "application", 5)
{
}

#if OPAL_SIP

SDPMediaDescription * OpalH224MediaType::CreateSDPMediaDescription(const OpalTransportAddress & localAddress)
{
  return NULL;
}

#endif

/////////////////////////////////////////////////////////////////////////

H224_Frame::H224_Frame(PINDEX size)
: Q922_Frame(H224_HEADER_SIZE + size)
{
  SetHighPriority(PFalse);
	
  SetControlFieldOctet(0x03);
	
  BYTE *data = GetInformationFieldPtr();
	
  // setting destination & source terminal address to BROADCAST
  data[0] = 0;
  data[1] = 0;
  data[2] = 0;
  data[3] = 0;
	
  // setting Client ID to CME
  data[4] = 0;
	
  // setting ES / BS / C1 / C0 / Segment number to zero
  data[5] = 0;
}

H224_Frame::~H224_Frame()
{
}

void H224_Frame::SetHighPriority(PBoolean flag)
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

BYTE H224_Frame::GetClientID() const
{
  BYTE *data = GetInformationFieldPtr();
	
  return data[4] & 0x7f;
}

void H224_Frame::SetClientID(BYTE clientID)
{
  // At the moment, only H.281 (client ID 0x01)
  // is supported
  PAssert(clientID <= 0x01, "Invalid client ID");
	
  BYTE *data = GetInformationFieldPtr();
	
  data[4] = clientID;
}

PBoolean H224_Frame::GetBS() const
{
  BYTE *data = GetInformationFieldPtr();
	
  return (data[5] & 0x80) != 0;
}

void H224_Frame::SetBS(PBoolean flag)
{
  BYTE *data = GetInformationFieldPtr();
	
  if(flag) {
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
	
  if(flag) {
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
	
  if(flag) {
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
	
  if(flag) {
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

PBoolean H224_Frame::Decode(const BYTE *data, 
						PINDEX size)
{
  PBoolean result = Q922_Frame::Decode(data, size);
	
  if(result == PFalse) {
	return PFalse;
  }
	
  // doing some validity check for H.224 frames
  BYTE highOrderAddressOctet = GetHighOrderAddressOctet();
  BYTE lowOrderAddressOctet = GetLowOrderAddressOctet();
  BYTE controlFieldOctet = GetControlFieldOctet();
	
  if((highOrderAddressOctet != 0x00) ||
     (!(lowOrderAddressOctet == 0x61 || lowOrderAddressOctet == 0x71)) ||
     (controlFieldOctet != 0x03) ||
     (GetClientID() > 0x02))
  {		
	  return PFalse;
  }
	
  return PTrue;
}

////////////////////////////////////

#if 0

OpalH224Handler::OpalH224Handler(OpalRTPConnection & connection, unsigned sessionID)
: transmitMutex()
{
  session = connection.UseSession(connection.GetTransport(), sessionID, "h224");
	
  h281Handler = connection.CreateH281ProtocolHandler(*this);
  receiverThread = NULL;
	
}

OpalH224Handler::~OpalH224Handler()
{
  delete h281Handler;
}

void OpalH224Handler::StartTransmit()
{
  PWaitAndSignal m(transmitMutex);
	
  if(canTransmit == PTrue) {
    return;
  }
	
  canTransmit = PTrue;
	
  transmitFrame = new RTP_DataFrame(300);
  
  // Use payload code 100 as this seems to be common to other implementations
  transmitFrame->SetPayloadType((RTP_DataFrame::PayloadTypes)100);
  transmitBitIndex = 7;
  transmitStartTime = new PTime();
	
  SendClientList();
  SendExtraCapabilities();
}

void OpalH224Handler::StopTransmit()
{
  PWaitAndSignal m(transmitMutex);
	
  delete transmitStartTime;
  transmitStartTime = NULL;
	
  canTransmit = PFalse;
}

void OpalH224Handler::StartReceive()
{
  if(receiverThread != NULL) {
    PTRACE(2, "H.224\tHandler is already receiving");
    return;
  }
	
  receiverThread = CreateH224ReceiverThread();
  receiverThread->Resume();
}

void OpalH224Handler::StopReceive()
{
  if(receiverThread != NULL) {
    receiverThread->Close();
  }
}

PBoolean OpalH224Handler::SendClientList()
{
  PWaitAndSignal m(transmitMutex);
	
  if(canTransmit == PFalse) {
    return PFalse;
  }
	
  H224_Frame h224Frame = H224_Frame(4);
  h224Frame.SetHighPriority(PTrue);
  h224Frame.SetDestinationTerminalAddress(H224_BROADCAST);
  h224Frame.SetSourceTerminalAddress(H224_BROADCAST);
	
  // CME frame
  h224Frame.SetClientID(0x00);
	
  // Begin and end of sequence
  h224Frame.SetBS(PTrue);
  h224Frame.SetES(PTrue);
  h224Frame.SetC1(PFalse);
  h224Frame.SetC0(PFalse);
  h224Frame.SetSegmentNumber(0);
	
  BYTE *ptr = h224Frame.GetClientDataPtr();
	
  ptr[0] = 0x01; // Client list code
  ptr[1] = 0x00; // Message code
  ptr[2] = 0x01; // one client
  ptr[3] = (0x80 | H281_CLIENT_ID); // H.281 with etra capabilities
	
  TransmitFrame(h224Frame);
	
  return PTrue;
}

PBoolean OpalH224Handler::SendExtraCapabilities()
{
  PWaitAndSignal m(transmitMutex);
	
  if(canTransmit == PFalse) {
    return PFalse;
  }
	
  h281Handler->SendExtraCapabilities();
	
  return PTrue;
}

PBoolean OpalH224Handler::SendClientListCommand()
{
  PWaitAndSignal m(transmitMutex);
	
  if(canTransmit == PFalse) {
    return PFalse;
  }
	
  H224_Frame h224Frame = H224_Frame(2);
  h224Frame.SetHighPriority(PTrue);
  h224Frame.SetDestinationTerminalAddress(H224_BROADCAST);
  h224Frame.SetSourceTerminalAddress(H224_BROADCAST);
	
  // CME frame
  h224Frame.SetClientID(0x00);
	
  // Begin and end of sequence
  h224Frame.SetBS(PTrue);
  h224Frame.SetES(PTrue);
  h224Frame.SetC1(PFalse);
  h224Frame.SetC0(PFalse);
  h224Frame.SetSegmentNumber(0);
	
  BYTE *ptr = h224Frame.GetClientDataPtr();
	
  ptr[0] = 0x01; // Client list code
  ptr[1] = 0xff; // Command code
	
  TransmitFrame(h224Frame);
	
  return PTrue;
}

PBoolean OpalH224Handler::SendExtraCapabilitiesCommand(BYTE clientID)
{
  PWaitAndSignal m(transmitMutex);
	
  if(canTransmit == PFalse) {
    return PFalse;
  }
	
  if(clientID != H281_CLIENT_ID) {
    return PFalse;
  }
	
  H224_Frame h224Frame = H224_Frame(4);
  h224Frame.SetHighPriority(PTrue);
  h224Frame.SetDestinationTerminalAddress(H224_BROADCAST);
  h224Frame.SetSourceTerminalAddress(H224_BROADCAST);
	
  // CME frame
  h224Frame.SetClientID(0x00);
	
  // Begin and end of sequence
  h224Frame.SetBS(PTrue);
  h224Frame.SetES(PTrue);
  h224Frame.SetC1(PFalse);
  h224Frame.SetC0(PFalse);
  h224Frame.SetSegmentNumber(0);
	
  BYTE *ptr = h224Frame.GetClientDataPtr();
	
  ptr[0] = 0x01; // Client list code
  ptr[1] = 0xFF; // Response code
  ptr[2] = (0x80 | clientID); // clientID with extra capabilities
	
  TransmitFrame(h224Frame);
	
  return PTrue;
}

PBoolean OpalH224Handler::SendExtraCapabilitiesMessage(BYTE clientID, 
												   BYTE *data, PINDEX length)
{	
  PWaitAndSignal m(transmitMutex);
	
  // only H.281 supported at the moment
  if(clientID != H281_CLIENT_ID) {
	
    return PFalse;
  }
	
  if(canTransmit == PFalse) {
    return PFalse;
  }
	
  H224_Frame h224Frame = H224_Frame(length+3);
  h224Frame.SetHighPriority(PTrue);
  h224Frame.SetDestinationTerminalAddress(H224_BROADCAST);
  h224Frame.SetSourceTerminalAddress(H224_BROADCAST);
	
  // use clientID zero to indicate a CME frame
  h224Frame.SetClientID(0x00);
	
  // Begin and end of sequence, rest is zero
  h224Frame.SetBS(PTrue);
  h224Frame.SetES(PTrue);
  h224Frame.SetC1(PFalse);
  h224Frame.SetC0(PFalse);
  h224Frame.SetSegmentNumber(0);
	
  BYTE *ptr = h224Frame.GetClientDataPtr();
	
  ptr[0] = 0x02; // Extra Capabilities code
  ptr[1] = 0x00; // Response Code
  ptr[2] = (0x80 | clientID); // EX CAPS and ClientID
	
  memcpy(ptr+3, data, length);
	
  TransmitFrame(h224Frame);
	
  return PTrue;	
}

PBoolean OpalH224Handler::TransmitClientFrame(BYTE clientID, H224_Frame & frame)
{
  PWaitAndSignal m(transmitMutex);
	
  // only H.281 is supported at the moment
  if(clientID != H281_CLIENT_ID) {
    return PFalse;
  }
	
  frame.SetClientID(clientID);
	
  TransmitFrame(frame);
	
  return PTrue;
}

PBoolean OpalH224Handler::OnReceivedFrame(H224_Frame & frame)
{
  if(frame.GetDestinationTerminalAddress() != H224_BROADCAST) {
    // only broadcast frames are handled at the moment
    PTRACE(3, "H.224\tReceived frame with non-broadcast address");
    return PTrue;
  }
  BYTE clientID = frame.GetClientID();
	
  if(clientID == 0x00) {
    return OnReceivedCMEMessage(frame);
  }
	
  if(clientID == H281_CLIENT_ID)	{
    h281Handler->OnReceivedMessage((const H281_Frame &)frame);
  }
	
  return PTrue;
}

PBoolean OpalH224Handler::OnReceivedCMEMessage(H224_Frame & frame)
{
  BYTE *data = frame.GetClientDataPtr();
	
  if(data[0] == 0x01) { // Client list code
	
    if(data[1] == 0x00) { // Message
      return OnReceivedClientList(frame);
		
    } else if(data[1] == 0xff) { // Command
      return OnReceivedClientListCommand();
    }
	  
  } else if(data[0] == 0x02) { // Extra Capabilities code
	  
    if(data[1] == 0x00) { // Message
      return OnReceivedExtraCapabilities(frame);
		
    } else if(data[1] == 0xff) {// Command
      return OnReceivedExtraCapabilitiesCommand();
    }
  }
	
  // incorrect frames are simply ignored
  return PTrue;
}

PBoolean OpalH224Handler::OnReceivedClientList(H224_Frame & frame)
{
  BYTE *data = frame.GetClientDataPtr();
	
  BYTE numberOfClients = data[2];
	
  PINDEX i = 3;
	
  PBoolean remoteHasH281 = PFalse;
	
  while(numberOfClients > 0) {
	  
	BYTE clientID = (data[i] & 0x7f);
		
	if(clientID == H281_CLIENT_ID) {
	  remoteHasH281 = PTrue;
	  i++;
	} else if(clientID == 0x7e) { // extended client ID
      i += 2;
    } else if(clientID == 0x7f) { // non-standard client ID
      i += 6;
    } else { // other standard client ID such as T.140
      i++;
    }
    numberOfClients--;
  }
	
  h281Handler->SetRemoteHasH281(remoteHasH281);
	
  return PTrue;
}

PBoolean OpalH224Handler::OnReceivedClientListCommand()
{
  SendClientList();
  return PTrue;
}

PBoolean OpalH224Handler::OnReceivedExtraCapabilities(H224_Frame & frame)
{
  BYTE *data = frame.GetClientDataPtr();
	
  BYTE clientID = (data[2] & 0x7f);
	
  if(clientID == H281_CLIENT_ID) {
    PINDEX size = frame.GetClientDataSize() - 3;
    h281Handler->OnReceivedExtraCapabilities((data + 3), size);
  }
	
  return PTrue;
}

PBoolean OpalH224Handler::OnReceivedExtraCapabilitiesCommand()
{
  SendExtraCapabilities();
  return PTrue;
}

OpalH224ReceiverThread * OpalH224Handler::CreateH224ReceiverThread()
{
  return new OpalH224ReceiverThread(this, *session);
}

void OpalH224Handler::TransmitFrame(H224_Frame & frame)
{	
  PINDEX size = frame.GetEncodedSize();
	
  if(!frame.Encode(transmitFrame->GetPayloadPtr(), size, transmitBitIndex)) {
    PTRACE(1, "H.224\tFailed to encode frame");
    return;
  }
	
  // determining correct timestamp
  PTime currentTime = PTime();
  PTimeInterval timePassed = currentTime - *transmitStartTime;
  transmitFrame->SetTimestamp((DWORD)timePassed.GetMilliSeconds() * 8);
  
  transmitFrame->SetPayloadSize(size);
  transmitFrame->SetMarker(PTrue);
	
  if(!session->WriteData(*transmitFrame)) {
    PTRACE(1, "H.224\tFailed to write encoded frame");
  }
}

////////////////////////////////////

OpalH224ReceiverThread::OpalH224ReceiverThread(OpalH224Handler *theH224Handler, RTP_Session & session)
: PThread(10000, NoAutoDeleteThread, HighestPriority, "H.224 Receiver Thread"),
  rtpSession(session)
{
  h224Handler = theH224Handler;
  timestamp = 0;
  terminate = PFalse;
}

OpalH224ReceiverThread::~OpalH224ReceiverThread()
{
}

void OpalH224ReceiverThread::Main()
{	
  RTP_DataFrame packet = RTP_DataFrame(300);
  H224_Frame h224Frame = H224_Frame();
	
  for (;;) {
	  
    inUse.Wait();
		
    if(!rtpSession.ReadBufferedData(packet)) {
      inUse.Signal();
      return;
    }
	
    timestamp = packet.GetTimestamp();
		
    if(h224Frame.Decode(packet.GetPayloadPtr(), packet.GetPayloadSize())) {
      PBoolean result = h224Handler->OnReceivedFrame(h224Frame);

      if(result == PFalse) {
        // PFalse indicates a serious problem, therefore the thread is closed
        return;
      }
    } else {
      PTRACE(1, "H.224\tDecoding of frame failed");
    }
		
    inUse.Signal();
		
    if(terminate == PTrue) {
      return;
    }
  }
}

void OpalH224ReceiverThread::Close()
{
  rtpSession.Close(PTrue);
	
  inUse.Wait();
	
  terminate = PTrue;
	
  inUse.Signal();
	
  PAssert(WaitForTermination(10000), "H224 receiver thread not terminated");
}

#endif

#endif // OPAL_H224FECC
