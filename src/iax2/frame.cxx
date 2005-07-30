/*
 *
 * Inter Asterisk Exchange 2
 * 
 * Implementation of the class that specifies frame construction.
 * 
 * Open Phone Abstraction Library (OPAL)
 *
 * Copyright (c) 2005 Indranet Technologies Ltd.
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
 * The Initial Developer of the Original Code is Indranet Technologies Ltd.
 *
 * The author of this code is Derek J Smithies
 *
 *  $Log: frame.cxx,v $
 *  Revision 1.1  2005/07/30 07:01:33  csoutheren
 *  Added implementation of IAX2 (Inter Asterisk Exchange 2) protocol
 *  Thanks to Derek Smithies of Indranet Technologies Ltd. for
 *  writing and contributing this code
 *
 *
 *
 *
 */
#include <ptlib.h>


#ifdef P_USE_PRAGMA
#pragma implementation "frame.h"
#endif

#include <iax2/frame.h>
#include <iax2/iax2con.h>
#include <iax2/iax2ep.h>
#include <iax2/ies.h>
#include <iax2/receiver.h>
#include <iax2/transmit.h>

#define new PNEW

int NextIndex();

int NextIndex() 
{
  static PAtomicInteger counter;

  return ++counter;
}

Frame::Frame(IAX2EndPoint &_endpoint)
  : endpoint(_endpoint)
{
  ZeroAllValues();
  
  frameIndex = NextIndex();
}

void Frame::ZeroAllValues()
{
  data.SetSize(0);
  
  encrypted         = FALSE;
  isFullFrame       = FALSE;
  isVideo           = FALSE;
  isAudio           = FALSE;
  
  currentReadIndex  = 0;
  currentWriteIndex = 0;
  timeStamp = 0;
  
  canRetransmitFrame = FALSE;
  presetTimeStamp = 0;
  
  frameType = undefType;
}

BOOL Frame::IsEncrypted() 
{ 
  return encrypted; 
}

BOOL Frame::IsVideo() const 
{ 
  return isVideo; 
}

BOOL Frame::IsAudio() const
{ 
  return isAudio; 
}

BOOL Frame::IsFullFrame()
{
  return isFullFrame;
}




BOOL Frame::ReadNetworkPacket(PUDPSocket &sock)
{
  data.SetSize(4096);  //Surely no packets > 4096 bytes in length
  
  WORD     portNo;
  PIPSocket::Address addr;
  sock.GetLocalAddress(addr);
  PTRACE(3, "Read process:: wait for  network packet on " << IdString() << " prt:" << sock.GetPort());
  
  BOOL res = sock.ReadFrom(data.GetPointer(), 4096, addr, portNo);
  remote.SetRemoteAddress(addr);
  remote.SetRemotePort(portNo);
  
  if (res == FALSE) {
    PTRACE(3, "Failed in reading from socket");
    return FALSE;
  }
  
  data.SetSize(sock.GetLastReadCount());
  
  if (data.GetSize() < 4) {
    PTRACE(3, "Read a very very small packet from the network - < 4 bytes");
    return FALSE;
  }
  PTRACE(3, "Successfully read a " << data.GetSize() << " byte frame from the network. " << IdString());
  
  return TRUE;
}

BOOL Frame::Read1Byte(BYTE & result)
{
  if (currentReadIndex >= data.GetSize())
    return FALSE;
  
  result = data[currentReadIndex];
  PTRACE(6, "Read byte at " << currentReadIndex << " of " << ::hex << ((int)result) << ::dec);
  currentReadIndex++;
  return TRUE;
}

BOOL Frame::Read2Bytes(PINDEX & res)
{
  BYTE a = 0;
  BYTE b = 0;
  if (Read1Byte(a) && Read1Byte(b)) {
    res = (a << 8) | b;
    return TRUE;
  }
  
  return FALSE;
}

BOOL Frame::Read2Bytes(unsigned int & res)
{
  BYTE a = 0, b = 0;
  if (Read1Byte(a) && Read1Byte(b)) {
    res = (a << 8) | b;
    return TRUE;
  }
  
  return FALSE;
}

BOOL Frame::Read4Bytes(unsigned int & res)
{
  PINDEX a = 0, b = 0;
  if (Read2Bytes(a) && Read2Bytes(b)) {
    res = (a << 16) | b;
    return TRUE;
  }
  
  return FALSE;
}

void Frame::Write1Byte(PINDEX newVal)
{
  Write1Byte((BYTE)(newVal & 0xff));
}

void Frame::Write1Byte(BYTE newVal)
{
  if (currentWriteIndex >= data.GetSize())
    data.SetSize(currentWriteIndex + 1);
  
  data[currentWriteIndex] = newVal;
  currentWriteIndex++;
}

void Frame::Write2Bytes(PINDEX newVal)
{
  BYTE a = (BYTE)(newVal >> 8);
  BYTE b = (BYTE)(newVal & 0xff);
  
  Write1Byte(a);
  Write1Byte(b);
}

void Frame::Write4Bytes(unsigned int newVal)
{
  PINDEX a = newVal >> 16;
  PINDEX b = newVal & 0xffff;
  
  Write2Bytes(a);
  Write2Bytes(b);
}


BOOL Frame::ProcessNetworkPacket()
{
  /*We are guaranteed to have a packet > 4 bytes in size */
  PINDEX a = 0;
  PTRACE(3, "Process Network Packet of " << data.GetSize() << " bytes");
  Read2Bytes(a);
  remote.SetSourceCallNumber(a & 0x7fff);
  PTRACE(6, "Source call number is " << (a & 0x7fff));
  if (a != 0)
    BuildConnectionTokenId();

  if (a & 0x8000) {
    isFullFrame = TRUE;
    if (!LookAheadFullFrameType())
      return FALSE;
    PTRACE(6, "Have a full frame of type " << frameType);
    return TRUE;
  }
  if (a == 0) {    //We have a mini frame here, of video type.
    PTRACE(6, "Have a mini video frame");
    isVideo = TRUE;
    PINDEX b = 0;
    Read2Bytes(b);
    remote.SetSourceCallNumber(b);
    BuildConnectionTokenId();
  }
  
  PTRACE(6, "Have a mini audio frame");
  return TRUE;
}

void Frame::BuildConnectionTokenId()
{
  connectionToken = PString("iax2:") + remote.RemoteAddress().AsString() + PString("-") + PString(remote.SourceCallNumber());
  PTRACE(3, "This frame belongs to connection \"" << connectionToken << "\"");
}

BOOL Frame::LookAheadFullFrameType()
{
  if (data.GetSize() < 12)
    return FALSE;
  
  if ((data[10] <= cngType) && (data[1] != undefType)) {
    frameType = (IaxFrameType)data[10];
    return TRUE;
  } else {
    PTRACE(0, "Bogus full frame type " << *this);
    return FALSE;
  }
}


void Frame::PrintOn(ostream & strm) const
{
  strm << IdString() << "      " << data.GetSize() << " bytes in frame" << endl
       << ::hex << data << ::dec;
}

void Frame::BuildTimeStamp(const PTime & callStartTime)
{
  if (presetTimeStamp > 0)
    timeStamp = presetTimeStamp;
  else
    timeStamp = CalcTimeStamp(callStartTime);
}

PINDEX Frame::CalcTimeStamp(const PTime & callStartTime)
{
  PINDEX tVal = (PTime() - callStartTime).GetMilliSeconds();
  PTRACE(3, "Calculate timestamp as " << tVal);
  return tVal;
}

BOOL Frame::TransmitPacket(PUDPSocket &sock)
{
  if (CallMustBeActive()) {
    if (!endpoint.ConnectionForFrameIsAlive(this)) {
      PTRACE(3, "Connection not found, call has been terminated. " << IdString());
      return FALSE;   //This happens because the call has been terminated.
    }
  }
  
  //     if (PIsDescendant(this, FullFrameProtocol))
  //   	    ((FullFrameProtocol *)this)->SetRetransmissionRequired();
  //	    cout << endl;
  
  PTRACE(1, "Now transmit " << endl << *this);
  BOOL transmitResult = sock.WriteTo(data.GetPointer(), DataSize(), remote.RemoteAddress(), 
				     (unsigned short)remote.RemotePort());
  PTRACE(6, "transmission of packet gave a " << transmitResult);
  return transmitResult;
}

////////////////////////////////////////////////////////////////////////////////  

MiniFrame::MiniFrame(Frame & srcFrame)
  : Frame(srcFrame)
{
  ZeroAllValues();
  isAudio = (data[0] != 0) || (data[1] != 0);
  isVideo = !isAudio;
}

MiniFrame::MiniFrame(IAX2EndPoint &_endpoint)
  : Frame(_endpoint)
{
  ZeroAllValues();
}

MiniFrame::MiniFrame(IAX2Processor * iaxProcessor, PBYTEArray &sound, 
		     BOOL _isAudio, PINDEX usersTimeStamp) 
  : Frame(iaxProcessor->GetEndPoint())
{
  isAudio = _isAudio;
  presetTimeStamp = usersTimeStamp;
  InitialiseHeader(iaxProcessor);  
  
  PINDEX headerSize = data.GetSize();
  data.SetSize(sound.GetSize() + headerSize);
  memcpy(data.GetPointer() + headerSize, sound.GetPointer(), sound.GetSize());
}

MiniFrame::~MiniFrame()
{
  PTRACE(1, "Destroy frame " << IdString());
}

void MiniFrame::InitialiseHeader(IAX2Processor *iaxProcessor)
{
  if (iaxProcessor != NULL) {
    remote   = iaxProcessor->GetRemoteInfo();
    BuildTimeStamp(iaxProcessor->GetCallStartTime());          
    SetConnectionToken(iaxProcessor->GetCallToken());
  }
  WriteHeader();
}

BOOL MiniFrame::ProcessNetworkPacket() 
{
  Read2Bytes(timeStamp);
  isAudio = isVideo =  FALSE;
  return TRUE;
}



void MiniFrame::ZeroAllValues()
{
}


void MiniFrame::AlterTimeStamp(PINDEX newValue)
{
  timeStamp = (newValue & (0xffff << 16)) | (timeStamp & 0xffff);
}



BOOL MiniFrame::WriteHeader()
{
  currentWriteIndex = 0;   //Probably not needed, but this makes everything "obvious"
  
  if(IsVideo()) {
    data.SetSize(6);
    Write2Bytes(0);
  } else 
    data.SetSize(4);
  
  Write2Bytes(remote.SourceCallNumber() & 0x7fff);
  Write2Bytes(timeStamp & 0xffff);
  
  return TRUE;
}

void MiniFrame::PrintOn(ostream & strm) const
{
  strm << "MiniFrame of " << PString(IsVideo() ? "video" : "audio") << " " 
       << IdString() << " \"" << GetConnectionToken() << "\"  " << endl;
  
  Frame::PrintOn(strm);
}

BYTE *MiniFrame::GetMediaDataPointer()
{
  if (IsVideo())
    return data.GetPointer() + 6;
  else
    return data.GetPointer() + 4;
}


PINDEX MiniFrame::GetMediaDataSize()
{
  int thisSize;
  if (IsVideo()) 
    thisSize = data.GetSize() - 6;
  else
    thisSize = data.GetSize() - 4;
  
  if (thisSize < 0)
    return 0;
  else
    return thisSize;
}


////////////////////////////////////////////////////////////////////////////////  

FullFrame::FullFrame(IAX2EndPoint &_newEndpoint)
  : Frame(_newEndpoint)
{
  ZeroAllValues();
}

/*This is built from an incoming frame (from the network*/
FullFrame::FullFrame(Frame & srcFrame)
  : Frame(srcFrame)
{
  PTRACE(5, "START Constructor for a full frame");
  ZeroAllValues();
  ProcessNetworkPacket();
  PTRACE(5, "END Constructor for a full frame");
}


FullFrame::~FullFrame()
{
  PTRACE(3, "Delete this frame  " << IdString());
  MarkDeleteNow();
}

BOOL FullFrame::operator*=(FullFrame & /*other*/)
{
  PAssertAlways("Sorry, FullFrame comparison operator is Not implemented");
  return TRUE;
}


void FullFrame::MarkAsResent()
{
  if (data.GetSize() > 2)
    data[2] |= 0x80;
}

void FullFrame::ZeroAllValues()
{
  subClass = 0;     
  timeStamp = 0;
  sequence.ZeroAllValues();
  canRetransmitFrame = TRUE;
  
  transmissionTimer.SetNotifier(PCREATE_NOTIFIER(OnTransmissionTimeout));
  
  retryDelta = PTimeInterval(minRetryTime);
  retries = maxRetries;
  
  callMustBeActive = TRUE;
  packetResent = FALSE;
  ClearListFlags();
  
  isFullFrame = TRUE;
  isAckFrame = FALSE;
}

void FullFrame::ClearListFlags()
{
  deleteFrameNow = FALSE;
  sendFrameNow   = FALSE;
}

BOOL FullFrame::TransmitPacket(PUDPSocket &sock)
{
  PTRACE(6, "Send network packet on " << IdString() << " " << connectionToken);
  if (packetResent) {
    MarkDataResent();      /* Set Retry flag.*/
  }
  
  if (retries < 0) {
    PTRACE(3, "Retries count is now negative on. " << IdString());
    return FALSE;    //Give up on this packet, it has exceeded the allowed number of retries.
  }
  
  PTRACE(6, "Start timer running for " << IdString() << connectionToken);
  transmissionTimer.SetInterval(retryDelta.GetMilliSeconds());
  transmissionTimer.Reset();   //*This causes the timer to start running.
  ClearListFlags();
  
  return Frame::TransmitPacket(sock);
}


BOOL FullFrame::ProcessNetworkPacket()
{
  PTRACE(1, "ProcessNetworkPacket - read the frame header");
  if (data.GetSize() < 12) {
    PTRACE(1, "Incoming full frame is undersize - should have 12 bytes, but only read " << data.GetSize());
    return FALSE;
  }
  
  PINDEX destCallNumber = 0;
  Read2Bytes(destCallNumber);
  packetResent = destCallNumber & 0x8000;
  destCallNumber &= 0x7fff;
  remote.SetDestCallNumber(destCallNumber);
  PTRACE(1, "Dest call number is " << destCallNumber);
  
  Read4Bytes(timeStamp);
  PTRACE(1, "Remote timestamp is " << timeStamp  << " milliseconds");
  
  BYTE a = 0;
  Read1Byte(a);
  sequence.SetOutSeqNo(a);
  Read1Byte(a);
  sequence.SetInSeqNo(a);
  PTRACE(1, "Sequence is " << sequence.AsString());
  
  Read1Byte(a);
  if ((a >= numFrameTypes) || (a == undefType)) {
    PTRACE(1, "Incoming packet has invalid frame type of " << a);
    return FALSE;
  }
  
  frameType = (IaxFrameType)a;
  isAudio = frameType == voiceType;
  isVideo = frameType == videoType;
  
  /*Do subclass value (which might be compressed) */
  Read1Byte(a);
  
  UnCompressSubClass(a);
  PTRACE(1, "Process network frame");
  PTRACE(1, "subClass is " << subClass);
  PTRACE(1, "frameType is " << frameType);
  isAckFrame = (subClass == FullFrameProtocol::cmdAck) && (frameType == iaxProtocolType);     
  return TRUE;
}

BOOL FullFrame::IsPingFrame()
{
  return (subClass == FullFrameProtocol::cmdPing) && (frameType == iaxProtocolType);     
}

BOOL FullFrame::IsNewFrame()
{
  return (subClass == FullFrameProtocol::cmdNew) && (frameType == iaxProtocolType);     
}

BOOL FullFrame::IsLagRqFrame()
{
  return (subClass == FullFrameProtocol::cmdLagRq) && (frameType == iaxProtocolType);     
}

BOOL FullFrame::IsLagRpFrame()
{
  return (subClass == FullFrameProtocol::cmdLagRp) && (frameType == iaxProtocolType);     
}

BOOL FullFrame::IsPongFrame()
{
  return (subClass == FullFrameProtocol::cmdPong) && (frameType == iaxProtocolType);     
}

BOOL FullFrame::IsAuthReqFrame()
{
  return (subClass == FullFrameProtocol::cmdAuthReq) && (frameType == iaxProtocolType);     
}

BOOL FullFrame::IsVNakFrame()
{
  return (subClass == FullFrameProtocol::cmdVnak) && (frameType == iaxProtocolType);     
}

BOOL FullFrame::FrameIncrementsInSeqNo()
{
  if (frameType != iaxProtocolType) {
    PTRACE(3, "SeqNos\tFrameType is not iaxProtocol, so we do increment inseqno. FrameType is " << frameType);
    return TRUE;
  }

  FullFrameProtocol::ProtocolSc cmdType = (FullFrameProtocol::ProtocolSc)subClass;
  PTRACE(3, "SeqNos\tThe cmd type (or subclass of FullFrameProtocol) is " << cmdType);
  if ((cmdType == FullFrameProtocol::cmdAck)     ||
      //  (cmdType == FullFrameProtocol::cmdLagRq)   ||
      //  (cmdType == FullFrameProtocol::cmdLagRp)   ||
      //  (cmdType == FullFrameProtocol::cmdAuthReq) ||
      //  (cmdType == FullFrameProtocol::cmdAuthRep) ||
      (cmdType == FullFrameProtocol::cmdVnak))   {
    PTRACE(3, "SeqNos\tThis is a iaxProtocol cmd type that does not increment inseqno");
    return FALSE;
  } else {
    PTRACE(3, "SeqNos\tThis is a iaxProtocol cmd type that increments inseqno");
  }
  return TRUE;
}

void FullFrame::UnCompressSubClass(BYTE a)
{
  if (a & 0x80) {
    if (a == 0xff)
      subClass = -1;
    else
      subClass = 1 << (a & 0x1f);
  } else
    subClass = a;
}

int FullFrame::CompressSubClass()
{
  if (subClass < 0x80)
    return subClass;
  
  for(PINDEX i = 0; i <  0x1f; i++) {
    if (subClass & (1 << i)) 
      return i | 0x80;
  }
  
  return -1;
}

void FullFrame::MarkDataResent()
{
  if (data.GetSize() >= 2) 
    data[2] |= 0x80;
}

BOOL FullFrame::WriteHeader()
{
  data.SetSize(12);
  PTRACE(6, "Write a source call number of " << remote.SourceCallNumber());
  Write2Bytes(remote.SourceCallNumber() + 0x8000);
  PTRACE(6, "Write a dest call number of " << remote.DestCallNumber());
  Write2Bytes(remote.DestCallNumber() + (packetResent ? 0x8000 : 0));
  
  PTRACE(6, "Write a timestamp of " << timeStamp);
  Write4Bytes(timeStamp);
  
  PTRACE(6, "Write in seq no " << sequence.InSeqNo() << " and out seq no of " << sequence.OutSeqNo());
  Write1Byte(sequence.OutSeqNo());
  Write1Byte(sequence.InSeqNo());
  
  PTRACE(6, "FrameType is " << GetFullFrameType());
  Write1Byte(GetFullFrameType());
  
  int a = CompressSubClass();
  if (a < 0)
    Write1Byte(0xff);
  else
    Write1Byte((BYTE)a);     
  PTRACE(6, "Comppressed sub class is " << a << " from " << subClass);
  
  return TRUE;
}

void FullFrame::MarkDeleteNow()
{
  transmissionTimer.Stop();
  deleteFrameNow = TRUE;
  retries = -1;
}

void FullFrame::OnTransmissionTimeout(PTimer &, INT)
{
  PTRACE(3, "Has had a timeout " << IdString() << " " << connectionToken);
  retryDelta = 4 * retryDelta.GetMilliSeconds();
  if (retryDelta > maxRetryTime)
    retryDelta = maxRetryTime;
  
  packetResent = TRUE;
  retries--;
  if (retries < 0) {
    deleteFrameNow = TRUE;
    PTRACE(3, "Mark as delete now " << IdString());
  } else {
    sendFrameNow = TRUE;
    PTRACE(3, "Mark as Send now " << IdString() << " " << connectionToken);
  }
  
  endpoint.transmitter->ProcessLists();
}


PString FullFrame::GetFullFrameName() const
{
  switch(frameType)  {
      case undefType      : return PString("(0?)      ");
      case dtmfType       : return PString("Dtmf      ");
      case voiceType      : return PString("Voice     ");
      case videoType      : return PString("Video     ");
      case controlType    : return PString("Session   ");
      case nullType       : return PString("Null      ");
      case iaxProtocolType: return PString("Protocol  ");
      case textType       : return PString("Text      ");
      case imageType      : return PString("Image     ");
      case htmlType       : return PString("Html      ");
      case cngType        : return PString("Cng       ");
      case numFrameTypes  : return PString("# F types ");
  }
  
  return PString("Frame name is undefined for value of ") + PString(frameType);
}

BYTE *FullFrame::GetMediaDataPointer()
{
  return data.GetPointer() + 12;
}


PINDEX FullFrame::GetMediaDataSize()
{
  return data.GetSize() - 12;
}

void FullFrame::InitialiseHeader(IAX2Processor *iaxProcessor)
{
  if (iaxProcessor != NULL) {
    SetConnectionToken(iaxProcessor->GetCallToken());
    BuildTimeStamp(iaxProcessor->GetCallStartTime());    
    remote   = iaxProcessor->GetRemoteInfo();
  }
  PTRACE(3, "source timestamp is " << timeStamp);
  frameType = (IaxFrameType)GetFullFrameType();  
  WriteHeader();
}

void FullFrame::PrintOn(ostream & strm) const
{
  strm << IdString() << " ++  " << GetFullFrameName() << " -- " 
       << GetSubClassName() << " \"" << connectionToken << "\"" << endl
       << remote << endl
       << ::hex << data << ::dec;
}

void FullFrame::ModifyFrameHeaderSequenceNumbers(PINDEX inNo, PINDEX outNo)
{
  data[8] = (BYTE) (outNo & 0xff);
  data[9] = (BYTE) (inNo & 0xff);
  GetSequenceInfo().SetOutSeqNo(outNo);
  GetSequenceInfo().SetInSeqNo(inNo);
}

void FullFrame::ModifyFrameTimeStamp(PINDEX newTimeStamp)
{
  timeStamp = newTimeStamp;
  PINDEX oldWriteIndex = currentWriteIndex;
  currentWriteIndex = 4;
  Write4Bytes(timeStamp);
  currentWriteIndex = oldWriteIndex;
}
////////////////////////////////////////////////////////////////////////////////  

FullFrameDtmf::FullFrameDtmf(Frame & srcFrame)
  : FullFrame(srcFrame)
{
}

FullFrameDtmf::FullFrameDtmf(FullFrame & srcFrame)
  : FullFrame(srcFrame)
{
}

FullFrameDtmf::FullFrameDtmf(IAX2Processor *iaxProcessor, PString  subClassValue)
  : FullFrame(iaxProcessor->GetEndPoint())
{
  SetSubClass(subClassValue.ToUpper()[0]);
  InitialiseHeader(iaxProcessor);
}


PString FullFrameDtmf::GetSubClassName() const {
  switch (GetSubClass()) {    
  case dtmf0:    return PString("0"); 
  case dtmf1:    return PString("1"); 
  case dtmf2:    return PString("2"); 
  case dtmf3:    return PString("3"); 
  case dtmf4:    return PString("4"); 
  case dtmf5:    return PString("5"); 
  case dtmf6:    return PString("6"); 
  case dtmf7:    return PString("7"); 
  case dtmf8:    return PString("8"); 
  case dtmf9:    return PString("9"); 
  case dtmfA:    return PString("A"); 
  case dtmfB:    return PString("B"); 
  case dtmfC:    return PString("C"); 
  case dtmfD:    return PString("D"); 
  case dtmfStar: return PString("*"); 
  case dtmfHash: return PString("#"); 
  };
  return PString("Undefined dtmf subclass value of ") + PString(GetSubClass());
}

////////////////////////////////////////////////////////////////////////////////
FullFrameVoice::FullFrameVoice(Frame & srcFrame)
  : FullFrame(srcFrame)
{
  PTRACE(3, "Construct a full frame voice from a Frame" << IdString());
}

FullFrameVoice::FullFrameVoice(FullFrame & srcFrame)
  : FullFrame(srcFrame)
{
  PTRACE(3, "Construct a full frame voice from a FullFrame" << IdString());
}

FullFrameVoice::FullFrameVoice(IAX2Processor *iaxProcessor, PBYTEArray & sound, PINDEX usersTimeStamp)
  : FullFrame(iaxProcessor->GetEndPoint())
{
  if (iaxProcessor != NULL) {
    SetSubClass((PINDEX)iaxProcessor->GetSelectedCodec());
  }

  presetTimeStamp = usersTimeStamp;
  InitialiseHeader(iaxProcessor);  

  PINDEX headerSize = data.GetSize();
  data.SetSize(sound.GetSize() + headerSize);
  memcpy(data.GetPointer() + headerSize, sound.GetPointer(), sound.GetSize());
  PTRACE(3, "Construct a full frame voice from a processor, sound, and codec" << IdString());
}


FullFrameVoice::~FullFrameVoice()
{
  PTRACE(3, "DE Construct a full frame voice" << IdString());
}

PString FullFrameVoice::GetSubClassName(unsigned int testValue) 
{
  switch (testValue) {
  case g7231:     return PString("G.723.1");
  case gsm:       return PString("GSM-06.10");
  case g711ulaw:  return PString("G.711-uLaw-64k");
  case g711alaw:  return PString("G.711-ALaw-64k");
  case mp3:       return PString("mp3");
  case adpcm:     return PString("adpcm");
  case pcm:       return PString("pcm");
  case lpc10:     return PString("LPC-10");
  case g729:      return PString("G.729");
  case speex:     return PString("speex");
  case ilbc:      return PString("iLBC-13k3");
  default: ;
  };
  
  PStringStream res;
  res << "The value 0x" << ::hex << testValue << ::dec << " could not be identified as a codec";
  return res;
}

unsigned short FullFrameVoice::OpalNameToIax2Value(const PString opalName)
{
  if (opalName.Find("uLaw") != P_MAX_INDEX) {
    PTRACE(3, "Codec supported "<< opalName);
    return g711ulaw;
  }
  
  if (opalName.Find("ALaw") != P_MAX_INDEX) {
    PTRACE(3, "Codec supported " << opalName);
    return  g711alaw;
  }
  
  if (opalName.Find("GSM-06.10") != P_MAX_INDEX) {
    PTRACE(3, "Codec supported " << opalName);
    return gsm;
  }

  if (opalName.Find("iLBC-13k3") != P_MAX_INDEX) {
    PTRACE(3, "Codec supported " << opalName);
    return ilbc; 
    }
  PTRACE(3, "Codec " << opalName << " is not supported in IAX2");
  return 0;
}


PString FullFrameVoice::GetSubClassName() const 
{
  return GetSubClassName(GetSubClass());
}

PString FullFrameVoice::GetOpalNameOfCodec(unsigned short testValue)
{
  switch (testValue) {
  case g7231:     return PString("G.723.1");
  case gsm:       return PString("GSM-06.10");
  case g711ulaw:  return PString("G.711-uLaw-64k");
  case g711alaw:  return PString("G.711-ALaw-64k");
  case mp3:       return PString("mp3");
  case adpcm:     return PString("adpcm");
  case pcm:       return PString("Linear-16-Mono-8kHz");
  case lpc10:     return PString("LPC10");
  case g729:      return PString("G.729");
  case speex:     return PString("speex");
  case ilbc:      return PString("ilbc");
  default: ;
  };
  
  PStringStream res;
  res << "The value 0x" << ::hex << testValue << ::dec << " could not be identified as a codec";
  return res;
}

////////////////////////////////////////////////////////////////////////////////

FullFrameVideo::FullFrameVideo(Frame & srcFrame)
  : FullFrame(srcFrame)
{
}

FullFrameVideo::FullFrameVideo(FullFrame & srcFrame)
  : FullFrame(srcFrame)
{
}

PString FullFrameVideo::GetSubClassName() const
{
  switch (GetSubClass()) {
  case jpeg:  return PString("jpeg");
  case png:  return PString("png");
  case h261:  return PString("H.261");
  case h263:  return PString("H.263");
  };
  return PString("Undefined FullFrameVideo subclass value of ") + PString(GetSubClass());
}
////////////////////////////////////////////////////////////////////////////////

FullFrameSessionControl::FullFrameSessionControl(Frame & srcFrame)
  : FullFrame(srcFrame)
{
}

FullFrameSessionControl::FullFrameSessionControl(FullFrame & srcFrame)
  : FullFrame(srcFrame)
{
}

FullFrameSessionControl::FullFrameSessionControl(IAX2Processor *iaxProcessor,
						 SessionSc session)  
  :  FullFrame(iaxProcessor->GetEndPoint())          
{
  SetSubClass((PINDEX)session);
  isAckFrame = FALSE;
  InitialiseHeader(iaxProcessor);
  callMustBeActive = TRUE;
}

FullFrameSessionControl::FullFrameSessionControl(IAX2Processor *iaxProcessor,
						 PINDEX subClassValue)   
  :  FullFrame(iaxProcessor->GetEndPoint())          
{
  SetSubClass(subClassValue);
  isAckFrame = FALSE;
  InitialiseHeader(iaxProcessor);
  callMustBeActive = TRUE;
}


PString FullFrameSessionControl::GetSubClassName() const {
  switch (GetSubClass()) {
  case hangup:          return PString("hangup");
  case ring:            return PString("ring");
  case ringing:         return PString("ringing");
  case answer:          return PString("answer");
  case busy:            return PString("busy");
  case tkoffhk:         return PString("tkoffhk");
  case offhook:         return PString("offhook");
  case congestion:      return PString("congestion");
  case flashhook:       return PString("flashhook");
  case wink:            return PString("wink");
  case option:          return PString("option");
  case keyRadio:        return PString("keyRadio");
  case unkeyRadio:      return PString("unkeyRadio");
  case callProgress:    return PString("callProgress");
  case callProceeding:  return PString("callProceeding");
  };
  
  return PString("Undefined FullFrameSessionControl subclass value of ") 
    + PString(GetSubClass());
}
////////////////////////////////////////////////////////////////////////////////

FullFrameNull::FullFrameNull(Frame & srcFrame)
  : FullFrame(srcFrame)
{
}

FullFrameNull::FullFrameNull(FullFrame & srcFrame)
  : FullFrame(srcFrame)
{
}

////////////////////////////////////////////////////////////////////////////////

FullFrameProtocol::FullFrameProtocol(IAX2Processor *iaxProcessor, PINDEX subClassValue, ConnectionRequired needCon)
  :  FullFrame(iaxProcessor->GetEndPoint())
{
  SetSubClass(subClassValue);
  isAckFrame = (subClassValue == cmdAck);
  if (isAckFrame) {
    PTRACE(1, "Sending an ack frame now");
  }
  InitialiseHeader(iaxProcessor);
  callMustBeActive = (needCon == callActive);
  PTRACE(3, "Contstruct a fullframeprotocol from a processor, subclass value    and a connectionrequired. " << IdString());
}

FullFrameProtocol::FullFrameProtocol(IAX2Processor *iaxProcessor,  ProtocolSc  subClassValue, ConnectionRequired needCon)
  : FullFrame(iaxProcessor->GetEndPoint())
{
  SetSubClass(subClassValue);
  isAckFrame = (subClassValue == cmdAck);
  InitialiseHeader(iaxProcessor);
  callMustBeActive = (needCon == callActive);

  PTRACE(3, "Contstruct a fullframeprotocol from a processor subclass value and connection required " << IdString());
}

FullFrameProtocol::FullFrameProtocol(IAX2Processor *iaxProcessor,  ProtocolSc  subClassValue, FullFrame *inReplyTo, ConnectionRequired needCon)
  : FullFrame(iaxProcessor->GetEndPoint())
{
  SetSubClass(subClassValue);     
  timeStamp = inReplyTo->GetTimeStamp();     
  isAckFrame = (subClassValue == cmdAck);
  if (isAckFrame) {
    sequence.SetAckSequenceInfo(inReplyTo->GetSequenceInfo());
  }
  if (iaxProcessor == NULL) {
    Remote rem = inReplyTo->GetRemoteInfo();
    remote = rem;
    ///	  remote.SetSourceCallNumber(rem.GetDestCallNumber());
    ///       remote.SetDestCallNumber(rem.GetSourceCallNumber());	  
  } else {
    remote = iaxProcessor->GetRemoteInfo();
    SetConnectionToken(iaxProcessor->GetCallToken());
  }
  //     processor->MassageSequenceForReply(sequence, inReplyTo->GetSequenceInfo());
  frameType = iaxProtocolType;       
  callMustBeActive = (needCon == callActive);
  WriteHeader();
  PTRACE(3, "Contstruct a fullframeprotocol from a  processor, subcalss value and a connection required" << IdString());
}

FullFrameProtocol::FullFrameProtocol(Frame & srcFrame)
  : FullFrame(srcFrame)
{
  ReadInformationElements();
  PTRACE(3, "Contstruct a fullframeprotocol from a Frame" << IdString());
}

FullFrameProtocol::FullFrameProtocol(FullFrame & srcFrame)
  : FullFrame(srcFrame)
{
  ReadInformationElements();
  PTRACE(3, "Contstruct a fullframeprotocol from a Full Frame" << IdString());
}

FullFrameProtocol::~FullFrameProtocol()
{
  ieElements.AllowDeleteObjects(TRUE);
  PTRACE(3, "DE Contstruct a fullframeprotocol" << IdString());
}



void FullFrameProtocol::SetRetransmissionRequired()
{
  if (GetSubClass() == FullFrameProtocol::cmdAck)
    canRetransmitFrame = FALSE;
  
  if (GetSubClass() == FullFrameProtocol::cmdLagRp)
    canRetransmitFrame = FALSE;
  
  if (GetSubClass() == FullFrameProtocol::cmdPong)
    canRetransmitFrame = FALSE;
}


void FullFrameProtocol::WriteIeAsBinaryData()
{
  PTRACE(6, "Write the IE data (" << ieElements.GetSize() 
	 << " elements) as binary data to frame");
  PINDEX headerSize = data.GetSize();
  data.SetSize(headerSize + ieElements.GetBinaryDataSize());
  
  PINDEX writeIndex = headerSize;
  for(PINDEX i = 0; i < ieElements.GetSize(); i++) {
    PTRACE(5, "Append to outgoing frame " << *ieElements.GetIeAt(i));
    ieElements.GetIeAt(i)->WriteBinary(data.GetPointer(), writeIndex);
  }    
}

BOOL FullFrameProtocol::ReadInformationElements()
{
  Ie *elem = NULL;
  
  while (GetUnReadBytes() >= 2) {
    BYTE thisType = 0, thisLength = 0;
    Read1Byte(thisType);
    Read1Byte(thisLength);
    if (thisLength <= GetUnReadBytes()){
      elem = Ie::BuildInformationElement(thisType, thisLength, data.GetPointer() + currentReadIndex);
      currentReadIndex += thisLength;
      if (elem != NULL)
	if (elem->IsValid()) {
	  ieElements.Append(elem);
	  //	  PTRACE(3, "Read information element " << *elem);
	}
    } else {
      PTRACE(3, "Unread bytes is " << GetUnReadBytes() << " This length is " << thisLength);
      break;
    }	  
  }
  
  if (elem == NULL)
    return FALSE;
  
  if (!elem->IsValid())
    return FALSE;
  
  return GetUnReadBytes() == 0;
}

void FullFrameProtocol::GetRemoteCapability(unsigned int & capability, unsigned int & preferred)
{
  capability = 0;
  preferred = 0;
  Ie * p;
  PINDEX i = 0;
  for(;;) {
    p = GetIeAt(i);
    if (p == NULL)
      break;
    
    i++;
    if (p->IsValid()) {
      if (PIsDescendant(p, IeCapability)) {
	capability = ((IeCapability *)p)->ReadData();
	PTRACE(3, "FullFrameProtocol\tCapability codecs are " << capability);
      }
      if (PIsDescendant(p, IeFormat)) {
	preferred = ((IeFormat *)p)->ReadData();
	PTRACE(3, "FullFrameProtocol\tPreferred codec is " << preferred);
      }
    } else {
      PTRACE(3, "Invalid data in IE. ");
    }
  }
}

void FullFrameProtocol::CopyDataFromIeListTo(IeData &res)
{
  Ie * p;
  PINDEX i = 0;
  for(;;) {
    p = GetIeAt(i);
    if (p == NULL)
      break;
    
    i++;
    PTRACE(3, "From FullFrameProtocol, handle Ie of type " << *p);
    if (p->IsValid()) {
      p->StoreDataIn(res);
      //             ieData.StoreValue(*p);// *(dynamic_cast <typeid(*p).name() *>(p)));
    } else {
      PTRACE(3, "Invalid data in IE. ");
    }
  }
}

PString FullFrameProtocol::GetSubClassName() const{
  switch (GetSubClass()) {
  case cmdNew:        return PString("new");
  case cmdPing:       return PString("ping");
  case cmdPong:       return PString("pong");
  case cmdAck:        return PString("ack");
  case cmdHangup:     return PString("hangup");
  case cmdReject:     return PString("reject");
  case cmdAccept:     return PString("accept");
  case cmdAuthReq:    return PString("authreq");
  case cmdAuthRep:    return PString("authrep");
  case cmdInval:      return PString("inval");
  case cmdLagRq:      return PString("lagrq");
  case cmdLagRp:      return PString("lagrp");
  case cmdRegReq:     return PString("regreq");
  case cmdRegAuth:    return PString("regauth");
  case cmdRegAck:     return PString("regack");
  case cmdRegRej:     return PString("regrej");
  case cmdRegRel:     return PString("regrel");
  case cmdVnak:       return PString("vnak");
  case cmdDpReq:      return PString("dpreq");
  case cmdDpRep:      return PString("dprep");
  case cmdDial:       return PString("dial");
  case cmdTxreq:      return PString("txreq");
  case cmdTxcnt:      return PString("txcnt");
  case cmdTxacc:      return PString("txacc");
  case cmdTxready:    return PString("txready");
  case cmdTxrel:      return PString("txrel");
  case cmdTxrej:      return PString("txrej");
  case cmdQuelch:     return PString("quelch");
  case cmdUnquelch:   return PString("unquelch");
  case cmdPoke:       return PString("poke");
  case cmdPage:       return PString("page");
  case cmdMwi:        return PString("mwi");
  case cmdUnsupport:  return PString("unsupport");
  case cmdTransfer:   return PString("transfer");
  case cmdProvision:  return PString("provision");
  case cmdFwDownl:    return PString("fwDownl");
  case cmdFwData:     return PString("fwData");
  };
  return PString("Undefined FullFRameProtocol subclass value of ") + PString(GetSubClass());
}
////////////////////////////////////////////////////////////////////////////////

FullFrameText::FullFrameText(Frame & srcFrame)
  : FullFrame(srcFrame)
{
}

FullFrameText::FullFrameText(FullFrame & srcFrame)
  : FullFrame(srcFrame)
{
}

PString FullFrameText::GetSubClassName() const 
{
  return PString("FullFrameText never has a valid sub class");
}
////////////////////////////////////////////////////////////////////////////////

FullFrameImage::FullFrameImage(Frame & srcFrame)
  : FullFrame(srcFrame)
{
}

FullFrameImage::FullFrameImage(FullFrame & srcFrame)
  : FullFrame(srcFrame)
{
}

PString FullFrameImage::GetSubClassName() const 
{
  return PString("FullFrameImage never has a valid sub class");
}
////////////////////////////////////////////////////////////////////////////////

FullFrameHtml::FullFrameHtml(Frame & srcFrame)
  : FullFrame(srcFrame)
{
}

FullFrameHtml::FullFrameHtml(FullFrame & srcFrame)
  : FullFrame(srcFrame)
{
}

PString FullFrameHtml::GetSubClassName() const 
{
  return PString("FullFrameHtml has a sub class of ") + PString(GetSubClass());
}

////////////////////////////////////////////////////////////////////////////////

FullFrameCng::FullFrameCng(Frame & srcFrame)
  : FullFrame(srcFrame)
{
}

FullFrameCng::FullFrameCng(FullFrame & srcFrame)
  : FullFrame(srcFrame)
{
}

PString FullFrameCng::GetSubClassName() const 
{
  return PString("FullFrameCng has a sub class of ") + PString(GetSubClass());
}


////////////////////////////////////////////////////////////////////////////////

FrameList::~FrameList()
{   
  AllowDeleteObjects(); 
}

void FrameList::ReportList()
{
  PWaitAndSignal m(mutex);

  for(PINDEX i = 0; i < PAbstractList::GetSize(); i++){
    PTRACE(3, "#" << (i + 1) << " of " << PAbstractList::GetSize() << "     " << ((Frame *)(GetAt(i)))->GetClass() << "  " << ((Frame *)(GetAt(i)))->IdString());
  }
}

void FrameList::AddNewFrame(Frame *newFrame)
{
  if (newFrame == NULL)
    return;

  PWaitAndSignal m(mutex);
  PAbstractList::Append(newFrame);
}


void FrameList::GrabContents(FrameList &src)
{
  Frame *current;
  do {
    current = src.GetLastFrame();
    AddNewFrame(current);
  } while (current != NULL);
}


Frame *FrameList::GetLastFrame()
{
  PWaitAndSignal m(mutex);
  PINDEX elems = GetEntries();
  if (elems ==  0) {
    return NULL;
  }
  
  PObject *p = PAbstractList::RemoveAt(0);
  return (Frame *)p;
}



void FrameList::DeleteMatchingSendFrame(FullFrame *reply)
{
  PWaitAndSignal m(mutex);
  PTRACE(3, "Look for a frame that has been sent, which is waiting for a reply/ack");

  for (PINDEX i = 0; i < GetEntries(); i++) {
    Frame *frame = (Frame *)GetAt(i);
    if (frame == NULL)
      continue;
    
    if (!frame->IsFullFrame())
      continue;

    FullFrame *sent = (FullFrame *)frame;

    if (sent->DeleteFrameNow()) {
      PTRACE(3, "Skip this frame, as it is marked, delete now" << sent->IdString());
      continue;
    }
    
    if (!(sent->GetRemoteInfo() *= reply->GetRemoteInfo())) {
      PTRACE(3, "mismatch in remote info");
      continue;
    } else {
      PTRACE(3, "Remotes are the same.. Keep looking for comparison");
      PTRACE(3, "Compare sent " << sent->IdString() << PString(" and  reply") << reply->IdString());
    }

    if (sent->GetSequenceInfo().IsSequenceNosZero() && sent->IsNewFrame()) {
      PTRACE(3,"Outgoing frame has a zero sequence nos, - delete this one" << sent->IdString());
      sent->MarkDeleteNow();
      return;
    } else {
      PTRACE(3, "Non zero sequence nos in the sequence number - look for exact match");
    }
   
    if (sent->GetTimeStamp() != reply->GetTimeStamp()) {
      PTRACE(3, "Time stamps differ, so give up on the test" << sent->IdString());
      continue;
    } else {
      PTRACE(3, "Time stamps are the same, so check in seqno vs oseqno " << sent->IdString());
    }

    PTRACE(3, "SeqNos\tSent is " << sent->GetSequenceInfo().OutSeqNo() << " " << sent->GetSequenceInfo().InSeqNo());
    PTRACE(3, "SeqNos\tRepl is " << reply->GetSequenceInfo().OutSeqNo() << " " << reply->GetSequenceInfo().InSeqNo());

    if (reply->IsLagRpFrame() && sent->IsLagRqFrame()) {
      PTRACE(3, "have read a LagRp packet for a LagRq frame  we have sent, delete this LagRq " 
	     << sent->IdString());
      sent->MarkDeleteNow();
      return;
    }

    if (reply->IsPongFrame() && sent->IsPingFrame()) {
      PTRACE(3, "have read a Pong packet for a PING frame  we have sent: delete the Pong " 
	     << sent->IdString());
      sent->MarkDeleteNow();
      return;
    }

    if (sent->GetSequenceInfo().InSeqNo() == reply->GetSequenceInfo().OutSeqNo()) {
        PTRACE(3, "Timestamp, and inseqno matches oseqno " << sent->IdString());
	if (reply->IsAckFrame()) {
	  PTRACE(3, "have read an ack packet for one we have sent, so delete this one " << sent->IdString());
	  sent->MarkDeleteNow();
	  return;
	}
    
    } else {
      PTRACE(3, "No match:: sent=" << sent->IdString() << " and reply=" << reply->IdString() 
	     << PString(reply->IsAckFrame() ? "reply is ack frame " : "reply is not ack frame ")
	     << PString("Sequence numbers are:: sentIn" ) 
	     << sent->GetSequenceInfo().InSeqNo() << "  rcvdOut" << reply->GetSequenceInfo().OutSeqNo());
    }	
	  
    PTRACE(3, " sequence " << sent->GetSequenceInfo().OutSeqNo() 
	   << " and " << reply->GetSequenceInfo().InSeqNo() << " are different");
    
  }
  PTRACE(3, "No match found, so no sent frame will be deleted ");
  return;
}


void FrameList::GetResendFramesDeleteOldFrames(FrameList &framesToSend)
{
  PWaitAndSignal m(mutex);
  
  if (GetSize() == 0) {
    PTRACE(3, "No frames available on the resend list");
    return;
  }
  
  for (PINDEX i = GetEntries(); i > 0; i--) {
    FullFrame *active = (FullFrame *)PAbstractList::GetAt(i - 1);
    if (active == NULL)
      continue;
    
    if (active->DeleteFrameNow() || active->SendFrameNow()) {
      if (active->DeleteFrameNow()) {
	PAbstractList::RemoveAt(i - 1);
	delete active;
      } else {
	PAbstractList::RemoveAt(i - 1);
	framesToSend.AddNewFrame(active);
      }
    }
  }
  PTRACE(3, "Have collected " << framesToSend.GetSize() << " frames to onsend");
  return;
}


void FrameList::MarkAllAsResent()
{
  for (PINDEX i = 0; i < GetEntries(); i++) {
    FullFrame *active = (FullFrame *)PAbstractList::GetAt(i);
    active->MarkAsResent();
  }
}


////////////////////////////////////////////////////////////////////////////////
/* The comment below is magic for those who use emacs to edit this file. */
/* With the comment below, the tab key does auto indent to 4 spaces.     */

/*
 * Local Variables:
 * mode:c
 * c-file-style:linux
 * c-basic-offset:2
 * End:
 */

