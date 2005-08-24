/*
 *
 * Inter Asterisk Exchange 2
 * 
 * Implementation of the class to handle the management of the protocol.
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
 *
 *
 * $Log: processor.cxx,v $
 * Revision 1.9  2005/08/24 04:56:25  dereksmithies
 * Add code from Adrian Sietsma to send FullFrameTexts and FullFrameDtmfs to
 * the remote end.  Many Thanks.
 *
 * Revision 1.8  2005/08/24 03:26:32  dereksmithies
 * Add excellent patch from Adrian Sietsma to set some variables correctly
 * when receiving a call. Many Thanks.
 *
 * Revision 1.7  2005/08/24 02:38:00  dereksmithies
 * Ensure that string information elements in CmdNew contain data.
 *
 * Revision 1.6  2005/08/24 02:09:05  dereksmithies
 * Remove Resume() call from constructor.
 *
 * Revision 1.5  2005/08/24 01:38:38  dereksmithies
 * Add encryption, iax2 style. Numerous tidy ups. Use the label iax2, not iax
 *
 * Revision 1.4  2005/08/20 07:36:08  rjongbloed
 * Fixed 10 second delay when exiting application
 *
 * Revision 1.3  2005/08/12 10:40:22  rjongbloed
 * Fixed compiler warning when notrace
 *
 * Revision 1.2  2005/08/04 08:14:17  rjongbloed
 * Fixed Windows build under DevStudio 2003 of IAX2 code
 *
 * Revision 1.1  2005/07/30 07:01:33  csoutheren
 * Added implementation of IAX2 (Inter Asterisk Exchange 2) protocol
 * Thanks to Derek Smithies of Indranet Technologies Ltd. for
 * writing and contributing this code
 *
 *
 *
 *
 *
 */

#include <ptlib.h>
#include <typeinfo>

#ifdef P_USE_PRAGMA
#pragma implementation "processor.h"
#endif

#include <iax2/causecode.h>
#include <iax2/frame.h>
#include <iax2/iax2con.h>
#include <iax2/iax2ep.h>
#include <iax2/iax2medstrm.h>
#include <iax2/ies.h>
#include <iax2/processor.h>
#include <iax2/sound.h>
#include <iax2/transmit.h>

#define new PNEW

/********************************************************************************
 * A Call has several distinct steps. These steps are defined in OPAL, and they are
 *
 *  UninitialisedPhase,
 *  SetUpPhase,      
 *  AlertingPhase,   
 *  ConnectedPhase,  
 *  EstablishedPhase,
 *  ReleasingPhase, 
 *  ReleasedPhase,   
 *
 ********************************************************************************/

WaitingForAck::WaitingForAck()
{
  ZeroValues();
}

void WaitingForAck::Assign(FullFrame *f, ResponseToAck _response)
{
  timeStamp = f->GetTimeStamp();
  seqNo     = f->GetSequenceInfo().InSeqNo();
  response  = _response;
  PTRACE(3, "MatchingAck\tIs looking for " << timeStamp << " and " << seqNo << " to do " << GetResponseAsString());
}

void WaitingForAck::ZeroValues()
{
  timeStamp = 0;
  seqNo = 0;
  //     response = sendNothing;
}

BOOL WaitingForAck::MatchingAckPacket(FullFrame *f)
{
  PTRACE(3, "MatchingAck\tCompare " << timeStamp << " and " << seqNo);
  if (f->GetTimeStamp() != timeStamp) {
    PTRACE(3, "MatchingAck\tTimstamps differ");
    return FALSE;
  }

  if (f->GetSequenceInfo().OutSeqNo() != seqNo) {
    PTRACE(3, "MatchingAck\tOut seqnos differ");    
    return FALSE;
  }

  return TRUE;
}

void WaitingForAck::PrintOn(ostream & strm) const
{
  strm << "time " << timeStamp << "    seq " << seqNo << "     " << GetResponseAsString();
}

PString WaitingForAck::GetResponseAsString() const
{
  switch(response) {
  case RingingAcked :  return PString("Received acknnowledgement of a Ringing message");
  case AcceptAcked  :  return PString("Received acknnowledgement of a Accept message");
  case AuthRepAcked :  return PString("Received acknnowledgement of a AuthRep message");
  case AnswerAcked  :  return PString("Received acknnowledgement of a Answer message");
  default:;
  }
  
  return PString("Undefined response code of ") + PString((int)response);
}







////////////////////////////////////////////////////////////////////////////////

IAX2Processor::IAX2Processor(IAX2EndPoint &ep)
  : PThread(1000, NoAutoDeleteThread),
    endpoint(ep)
{
  endThread = FALSE;

  remote.SetDestCallNumber(0);
  remote.SetRemoteAddress(0);
  remote.SetRemotePort(0);
  
  specialPackets = FALSE;

  nextTask.ZeroValues();  
  noResponseTimer.SetNotifier(PCREATE_NOTIFIER(OnNoResponseTimeout));
  
  statusCheckTimer.SetNotifier(PCREATE_NOTIFIER(OnStatusCheck));
  statusCheckOtherEnd = FALSE;

  callStatus = 0;
  frameList.Initialise();
  soundWaitingForTransmission.Initialise();
  soundReadFromEthernet.Initialise();
  
  selectedCodec = 0;
  
  audioCanFlow = FALSE;
  audioFramesNotStarted = TRUE;

  con = NULL;

  firstMediaFrame = TRUE;
  answerCallNow = FALSE;
  audioFrameDuration = 0;
  audioCompressedBytes = 0;
}

IAX2Processor::~IAX2Processor()
{
  PTRACE(3, "IAX2Processor DESTRUCTOR");

  StopNoResponseTimer();
  
  Terminate();
  WaitForTermination(10000);

  frameList.AllowDeleteObjects();
}

void IAX2Processor::AssignConnection(IAX2Connection * _con)
{
  con = _con;

  remote.SetSourceCallNumber(con->GetEndPoint().NextSrcCallNumber());
  
  Resume();
}

void IAX2Processor::PrintOn(ostream & strm) const
{
  strm << "In call with " << con->GetRemotePartyName() << "  " << remotePhoneNumber << " " << callToken  << endl
       << "  Call has been up for " << setprecision(0) << setw(5)
       << (PTime() - callStartTime) << endl
       << "  Control frames sent " << controlFramesSent    << endl
       << "  Control frames rcvd " << controlFramesRcvd    << endl
       << "  Audio frames sent   " << audioFramesSent      << endl
       << "  Audio frames rcvd   " << audioFramesRcvd      << endl
       << "  Video frames sent   " << videoFramesSent      << endl
       << "  Video frames rcvd   " << videoFramesRcvd      << endl;
}

void IAX2Processor::Activate()
{
  activate.Signal();
}

void IAX2Processor::Release(OpalConnection::CallEndReason reason)
{
  PTRACE(3, "Processor\tRelease(" << reason << ")");
  PStringStream str;
  str << reason;
  Hangup(str);
}

void IAX2Processor::ClearCall(OpalConnection::CallEndReason reason)
{
  statusCheckTimer.Stop();
  PTRACE(3, "ListProcesser runs     =====ClearCall(" << reason << ")");
  
  PStringStream str;
  str << reason;
  Hangup(str);

  con->EndCallNow(reason);
}


void IAX2Processor::OnReleased()
{
  PTRACE(3, "OnReleased method in processor has run");
  Terminate();
}

void IAX2Processor::Terminate()
{
  PTRACE(3, "Processor has been directed to end. So end now.");
  if (IsTerminated()) {
    PTRACE(3, "Processor has already ended");
  }
  
  endThread = TRUE;
  Activate();
}

void IAX2Processor::Main()
{
  PString name = GetThreadName();
  if (IsHandlingSpecialPackets())
    SetThreadName("Special Iax packets");
  else
    SetThreadName("Process " + name);

  while(endThread == FALSE) {
    activate.Wait();
    ProcessLists();     
  }
  ProcessLists();
    
  PTRACE(3, "End of iax connection processing");
}

void IAX2Processor::PutSoundPacketToNetwork(PBYTEArray *sound)
{
  /*This thread does not send the audio frame. 
    The IAX2Processor thread sends the audio frame */
  
  soundWaitingForTransmission.AddNewEntry(sound);
  
  CleanPendingLists();
} 

BOOL IAX2Processor::IsStatusQueryEthernetFrame(Frame *frame)
{
  if (!PIsDescendant(frame, FullFrame))
    return FALSE;
   
  FullFrame *f = (FullFrame *)frame;
  if (f->GetFrameType() != FullFrame::iax2ProtocolType)
    return FALSE;
   
  PINDEX subClass = f->GetSubClass();
   
  if (subClass == FullFrameProtocol::cmdLagRq) {
    PTRACE(3, "Special packet of  lagrq to process");
    return TRUE;
  }
   
  if (subClass == FullFrameProtocol::cmdPing) {
    PTRACE(3, "Special packet of Ping to process");
    return TRUE;
  }
   
  PTRACE(3, "This frame  is not a cmdPing or cmdLagRq");
   
  return FALSE;
}


void IAX2Processor::CallStopSounds()
{
  cout << "We have received a call stop sounds packet " << endl;
}

void IAX2Processor::ReceivedHookFlash() 
{ 
} 

void IAX2Processor::RemoteNodeIsBusy() 
{ 
} 

void IAX2Processor::RemoteNodeHasAnswered()
{
  if (IsCallAnswered()) {
    PTRACE(3, "Second accept packet received. Ignore it");
    return;
  }
  
  SetCallAnswered();
  PTRACE(3, " Remote node has answered");
  StopNoResponseTimer();
  PTRACE(3, "IAX\tCALL con->OnConnected");
  con->OnConnected();
}

void IAX2Processor::Hangup(PString dieMessage)
{
  PTRACE(3, "Hangup request " << dieMessage);
  hangList.AppendString(dieMessage);   //send this text to remote endpoint 
  
  activate.Signal();
}

 void IAX2Processor::CheckForHangupMessages()
{
  if (!hangList.IsEmpty()) {
    FullFrameProtocol * f = new FullFrameProtocol(this, FullFrameProtocol::cmdHangup, FullFrame::callIrrelevant);
    PTRACE(3, "Send a hangup frame to the remote endpoint");
	  
    f->AppendIe(new IeCause(hangList.GetFirstDeleteAll()));
    //        f->AppendIe(new IeCauseCode(IeCauseCode::NormalClearing));
    TransmitFrameToRemoteEndpoint(f);
    Terminate();
  }
}

void IAX2Processor::ConnectToRemoteNode(PString & newRemoteNode)
{      // Parse a string like guest@node.name.com/23234
  
  PTRACE(2, "Connect to remote node " << newRemoteNode);
  PStringList res = IAX2EndPoint::DissectRemoteParty(newRemoteNode);

  if (res[IAX2EndPoint::userIndex].IsEmpty()     ||
      res[IAX2EndPoint::addressIndex].IsEmpty()  ||
      res[IAX2EndPoint::extensionIndex].IsEmpty()) {
    PTRACE(3, "Opal\tremote node to call is not specified correctly iax2:" << newRemoteNode);
    PTRACE(3, "Opal\tExample format is iax2:guest@misery.digium.com/s");
    PTRACE(3, "Opal\tYou supplied " <<
	   "iax2:" <<
	   (res[IAX2EndPoint::userIndex].IsEmpty()      ? "" : res[IAX2EndPoint::userIndex])     << "@" <<
	   (res[IAX2EndPoint::addressIndex].IsEmpty()   ? "" : res[IAX2EndPoint::addressIndex])  << "/" <<
	   (res[IAX2EndPoint::extensionIndex].IsEmpty() ? "" : res[IAX2EndPoint::extensionIndex])            );
    return;
  }
    
  PIPSocket::Address ip;
  if (!PIPSocket::GetHostAddress(res[IAX2EndPoint::addressIndex], ip)) {
    PTRACE(0, "Conection\t Failed to make call to " << res[IAX2EndPoint::addressIndex]);
      cout << "Could not make a call to " << res[IAX2EndPoint::addressIndex] 
	   << " as IP resolution failed" << endl;
      return;
    }
  PTRACE(3, "Resolve " << res[IAX2EndPoint::addressIndex]  << " as ip address " << ip);
    
  remote.SetRemotePort(con->GetEndPoint().ListenPortNumber());
  remote.SetRemoteAddress(ip);
    
  callStartTime = PTime();
  FullFrameProtocol * f = new FullFrameProtocol(this, FullFrameProtocol::cmdNew);
  PTRACE(3, "Create full frame protocol to do cmdNew. Just contains data. ");
  f->AppendIe(new IeVersion());
  f->AppendIe(new IeFormat(con->GetPreferredCodec()));
  f->AppendIe(new IeCapability(con->GetSupportedCodecs()));
  
  if (!endpoint.GetLocalNumber().IsEmpty())
    f->AppendIe(new IeCallingNumber(endpoint.GetLocalNumber()));

  if (!endpoint.GetLocalUserName().IsEmpty())
    f->AppendIe(new IeCallingName(endpoint.GetLocalUserName()));
  
  if (!res[IAX2EndPoint::userIndex].IsEmpty())
    f->AppendIe(new IeUserName(res[IAX2EndPoint::userIndex]));

  if (!res[IAX2EndPoint::extensionIndex].IsEmpty())
    f->AppendIe(new IeCalledNumber(res[IAX2EndPoint::extensionIndex] ));

  if (!res[IAX2EndPoint::extensionIndex].IsEmpty())
    f->AppendIe(new IeDnid(res[IAX2EndPoint::extensionIndex]));

  if (!res[IAX2EndPoint::contextIndex].IsEmpty())
    f->AppendIe(new IeCalledContext(res[IAX2EndPoint::contextIndex]));

  f->AppendIe(new IeEncryption());

  PTRACE(3, "Create full frame protocol to do cmdNew. Finished appending Ies. ");
  TransmitFrameToRemoteEndpoint(f);
  StartNoResponseTimer();
  return;
}

void IAX2Processor::ReportStatistics()
{
  cout << "  Call has been up for " << setprecision(0) << setw(5)
       << (PTime() - callStartTime) << endl
       << "  Control frames sent " << controlFramesSent  << endl
       << "  Control frames rcvd " << controlFramesRcvd  << endl
       << "  Audio frames sent   " << audioFramesSent      << endl
       << "  Audio frames rcvd   " << audioFramesRcvd      << endl
       << "  Video frames sent   " << videoFramesSent      << endl
       << "  Video frames rcvd   " << videoFramesRcvd      << endl;
}

BOOL IAX2Processor::ProcessOneIncomingEthernetFrame()
{ 
  Frame *frame = frameList.GetLastFrame();
  if (frame == NULL) {
    return FALSE;
  }
  
  PTRACE(3, "IaxConnection\tUnknown  incoming frame " << frame->IdString());
  Frame *af = frame->BuildAppropriateFrameType(encryption);
  delete frame;

  if (af == NULL)
    return TRUE;
  frame = af;
  
  if (PIsDescendant(frame, MiniFrame)) {
    PTRACE(3, "IaxConnection\tIncoming mini frame" << frame->IdString());
    ProcessNetworkFrame((MiniFrame *)frame);
    return TRUE;
  }
  
  FullFrame *f = (FullFrame *) frame;
  PTRACE(3, "IaxConnection\tFullFrame incoming frame " << frame->IdString());
  
  endpoint.transmitter->PurgeMatchingFullFrames(f);

  if (sequence.IncomingMessageIsOk(*f)) {
    PTRACE(3, "sequence numbers are Ok");
  }
  
  IncControlFramesRcvd();
  
  if (remote.DestCallNumber() == 0) {
    PTRACE(3, "Set Destination call number to " << frame->GetRemoteInfo().SourceCallNumber());
    remote.SetDestCallNumber(frame->GetRemoteInfo().SourceCallNumber());
  }
  
  switch(f->GetFrameType()) {
  case FullFrame::dtmfType:        
    PTRACE(3, "Build matching full frame    dtmfType");
    ProcessNetworkFrame(new FullFrameDtmf(*f));
    break;
  case FullFrame::voiceType:       
    PTRACE(3, "Build matching full frame    voiceType");
    ProcessNetworkFrame(new FullFrameVoice(*f));
    break;
  case FullFrame::videoType:       
    PTRACE(3, "Build matching full frame    videoType");
    ProcessNetworkFrame(new FullFrameVideo(*f));
    break;
  case FullFrame::controlType:     
    PTRACE(3, "Build matching full frame    controlType");
    ProcessNetworkFrame(new FullFrameSessionControl(*f));
    break;
  case FullFrame::nullType:        
    PTRACE(3, "Build matching full frame    nullType");
    ProcessNetworkFrame(new FullFrameNull(*f));
    break;
  case FullFrame::iax2ProtocolType: 
    PTRACE(3, "Build matching full frame    iax2ProtocolType");
    ProcessNetworkFrame(new FullFrameProtocol(*f));
    break;
  case FullFrame::textType:        
    PTRACE(3, "Build matching full frame    textType");
    ProcessNetworkFrame(new FullFrameText(*f));
    break;
  case FullFrame::imageType:       
    PTRACE(3, "Build matching full frame    imageType");
    ProcessNetworkFrame(new FullFrameImage(*f));
    break;
  case FullFrame::htmlType:        
    PTRACE(3, "Build matching full frame    htmlType");
    ProcessNetworkFrame(new FullFrameHtml(*f));
    break;
  case FullFrame::cngType:        
    PTRACE(3, "Build matching full frame    cngType");
    ProcessNetworkFrame(new FullFrameCng(*f));
    break;
  default: 
    PTRACE(3, "Build matching full frame, Type not understood");
    f = NULL;
  };
  if (f != NULL)
    delete f;

  return TRUE;   /*There could be more frames to process. */
}


void IAX2Processor::ProcessLists()
{
  while(ProcessOneIncomingEthernetFrame());
    
  PBYTEArray *oneSound;
  do {
    oneSound = soundWaitingForTransmission.GetLastEntry();
    SendSoundMessage(oneSound);
  } while (oneSound != NULL);
  
  
  PString nodeToCall = callList.GetFirstDeleteAll();
  if (!nodeToCall.IsEmpty()) {
    PTRACE(3, "make a call to " << nodeToCall);
    ConnectToRemoteNode(nodeToCall);
  }
  
  PString dtmfs = dtmfText.GetAndDelete();
  
  while(dtmfs.GetLength() > 0) {
    SendDtmfMessage(dtmfs[0]);
    dtmfs = dtmfs.Mid(1);
  }  

  PStringArray sendList; // text messages
  textList.GetAllDeleteAll(sendList);

  PTRACE(3, "Have " << sendList.GetSize() << " text strings to send");
  while(sendList.GetSize() > 0) {
    PString text = *(PString *)sendList.RemoveAt(0);
    SendTextMessage(text);    
  }

  if (answerCallNow)
    SendAnswerMessageToRemoteNode();

  if (statusCheckOtherEnd)
    DoStatusCheck();

  CheckForHangupMessages();
}

void IAX2Processor::ProcessIncomingAudioFrame(Frame *newFrame)
{
  PTRACE(3, "Processor\tPlace audio frame on queue " << newFrame->IdString());
  
  IncAudioFramesRcvd();
  
  soundReadFromEthernet.AddNewFrame(newFrame);
  PTRACE(3, "have " << soundReadFromEthernet.GetSize() << " pending packets on incoming sound queue");
}

void IAX2Processor::ProcessIncomingVideoFrame(Frame *newFrame)
{
  PTRACE(3, "Incoming video frame ignored, cause we don't handle it");
  IncVideoFramesRcvd();
  delete newFrame;
}

void IAX2Processor::SendSoundMessage(PBYTEArray *sound)
{
  if (sound == NULL)
    return;

  if (sound->GetSize() == 0) {
    delete sound;
    return;
  }

  IncAudioFramesSent();   

  PTRACE(3, "This frame is size " << sound->GetSize());
  PTRACE(3, "This frame is duration " << audioFrameDuration);
  PTRACE(3, "This frame is compresed bytes of " << audioCompressedBytes);

  PINDEX thisDuration = (PINDEX)((sound->GetSize() * audioFrameDuration) / audioCompressedBytes);
  long thisTimeStamp = (PTime() - callStartTime).GetInterval();
  PTRACE(3, "This frame is duration " << thisDuration << " ms   at time " << thisTimeStamp);

  thisTimeStamp = ((thisTimeStamp + (thisDuration > 1))/thisDuration) * thisDuration;
  long lastTimeStamp = thisTimeStamp - thisDuration;

  BOOL sendFullFrame =  ((thisTimeStamp - lastSentAudioFrameTime) > 65536)
                        || ((thisTimeStamp & 0xffff) < (lastTimeStamp & 0xffff))
                        || audioFramesNotStarted;

  if ((thisTimeStamp - lastSentAudioFrameTime) > 65536) {
    PTRACE(3, "RollOver last sent audio frame too large " );
    PTRACE(3, "Thistime stamp is " << thisTimeStamp);
    PTRACE(3, "Thisduration is " << thisDuration);
    PTRACE(3, "This last timestamp is " << lastTimeStamp);
    PTRACE(3, "last sent audio frame is " <<lastSentAudioFrameTime);
  }

  if ((thisTimeStamp & 0xffff) < (lastTimeStamp & 0xffff)) {
    PTRACE(3, "RollOver timestamp past 65535");
    PTRACE(3, "Thistime stamp is " << thisTimeStamp);
    PTRACE(3, "Thisduration is " << thisDuration);
    PTRACE(3, "This last timestamp is " << lastTimeStamp);
    PTRACE(3, "last sent audio frame is " <<lastSentAudioFrameTime);
  }
  lastSentAudioFrameTime = thisTimeStamp;

  if (sendFullFrame) {
    audioFramesNotStarted = FALSE;
      FullFrameVoice *f = new FullFrameVoice(this, *sound, thisTimeStamp);
      PTRACE(3, "Send a full audio frame" << thisDuration << " On " << f->IdString());
      TransmitFrameToRemoteEndpoint(f);
  } else {
      MiniFrame *f = new MiniFrame(this, *sound, TRUE, thisTimeStamp & 0xffff);
      TransmitFrameToRemoteEndpoint(f);
  }

  delete sound;
}

void IAX2Processor::SendDtmf(PString  dtmfs)
{
  dtmfText += dtmfs;
}

void IAX2Processor::SendText(const PString & text)
{
  textList.AppendString(text);
}

void IAX2Processor::SendDtmfMessage(char  message)
{
  FullFrameDtmf *f = new FullFrameDtmf(this, message);
  TransmitFrameToRemoteEndpoint(f);
}

void IAX2Processor::SendTextMessage(PString & message)
{
  FullFrameText *f = new FullFrameText(this, message);
  TransmitFrameToRemoteEndpoint(f);
}

void IAX2Processor::SendAckFrame(FullFrame *inReplyTo)
{
  PTRACE(3, "Processor\tSend an ack frame in reply" );
  PTRACE(3, "Processor\tIn reply to " << *inReplyTo);
  FullFrameProtocol *f = new FullFrameProtocol(this, FullFrameProtocol::cmdAck, inReplyTo); //, FullFrame::callIrrelevant);
  PTRACE(4, "Swquence for sending is (pre) " << sequence.AsString());
  TransmitFrameToRemoteEndpoint(f);
  PTRACE(4, "Sequence for sending is (ppost) " << sequence.AsString());
}

void IAX2Processor::TransmitFrameNow(Frame *src)
{
  if (!src->EncryptContents(encryption)) {
    delete src;
    return;
  }
  endpoint.transmitter->SendFrame(src);
}

void IAX2Processor::TransmitFrameToRemoteEndpoint(FullFrameProtocol *src)
{
  src->WriteIeAsBinaryData();
  TransmitFrameToRemoteEndpoint((Frame *)src);
}

void IAX2Processor::TransmitFrameToRemoteEndpoint(Frame *src)
{
  PTRACE(3, "Send frame " << src->GetClass() << " " << src->IdString() << " to remote endpoint");
  if (src->IsFullFrame()) {
    PTRACE(3, "Send full frame " << src->GetClass() << " with seq increase");
    sequence.MassageSequenceForSending(*(FullFrame*)src);
    IncControlFramesSent();
  }
  TransmitFrameNow(src);
}

void IAX2Processor::TransmitFrameToRemoteEndpoint(FullFrame *src, WaitingForAck::ResponseToAck response)
{
  sequence.MassageSequenceForSending(*(FullFrame*)src);
  IncControlFramesSent();
  nextTask.Assign(src, response);
  TransmitFrameNow(src);
}

void IAX2Processor::ProcessNetworkFrame(MiniFrame * src)
{
  PTRACE(3, "ProcessNetworkFrame(MiniFrame * src)");
  
  src->AlterTimeStamp(lastFullFrameTimeStamp);
  
  if (src->IsVideo()) {
    PTRACE(3, "Incoming mini video frame");
    ProcessIncomingVideoFrame(src);
    return;
  }
  
  if (src->IsAudio()) {
    PTRACE(3, "Incoming mini audio frame");
    ProcessIncomingAudioFrame(src);
    return;
  }
  
  PTRACE(1, "ERROR - mini frame is not marked as audio or video");
  delete src;
  return;
}


void IAX2Processor::ProcessNetworkFrame(FullFrame * src)
{
  PTRACE(3, "ProcessNetworkFrame(FullFrame * src)");
  PStringStream message;
  message << PString("Do not know how to process networks packets of \"Full Frame\" type ") << *src;
  PAssertAlways(message);
  return;
}

void IAX2Processor::ProcessNetworkFrame(Frame * src)
{
  PTRACE(3, "ProcessNetworkFrame(Frame * src)");
  PStringStream message;
  message << PString("Do not know how to process networks packets of \"Frame\" type ") << *src;
  PTRACE(3, message);
  PTRACE(3, message);
  PAssertAlways(message);
  return;
}

void IAX2Processor::ProcessNetworkFrame(FullFrameDtmf * src)
{
  PTRACE(3, "ProcessNetworkFrame(FullFrameDtmf * src)");
  SendAckFrame(src);
  con->OnUserInputTone((char)src->GetSubClass(), 1);

  delete src;
}

void IAX2Processor::ProcessNetworkFrame(FullFrameVoice * src)
{
  if (firstMediaFrame) {
    PTRACE(3, "Processor\tReceived first voice media frame " << src->IdString());
    firstMediaFrame = FALSE;
  }
  PTRACE(3, "ProcessNetworkFrame(FullFrameVoice * src)" << src->IdString());
  SendAckFrame(src);
  ProcessIncomingAudioFrame(src);
  
  return;
}

void IAX2Processor::ProcessNetworkFrame(FullFrameVideo * src)
{
  if (firstMediaFrame) {
    PTRACE(3, "Processor\tReceived first video media frame ");
    firstMediaFrame = FALSE;
  }

  PTRACE(3, "ProcessNetworkFrame(FullFrameVideo * src)");
  SendAckFrame(src);
  ProcessIncomingVideoFrame(src);
  
  return;
}

void IAX2Processor::ProcessNetworkFrame(FullFrameSessionControl * src)
{ /* these frames are labelled as AST_FRAME_CONTROL in the asterisk souces.
     We could get an Answer message from here., or a hangup., or...congestion control... */
  
  PTRACE(3, "ProcessNetworkFrame(FullFrameSessionControl * src)");
  SendAckFrame(src);  
  
  switch(src->GetSubClass()) {
  case FullFrameSessionControl::hangup:          // Other end has hungup
    cout << "Other end has hungup, so exit" << endl;
    con->EndCallNow();
    break;
    
  case FullFrameSessionControl::ring:            // Local ring
    break;
    
  case FullFrameSessionControl::ringing:         // Remote end is ringing
    RemoteNodeIsRinging();
    break;
    
  case FullFrameSessionControl::answer:          // Remote end has answered
    PTRACE(3, "Have received answer packet from remote endpoint ");    
    RemoteNodeHasAnswered();
    break;
    
  case FullFrameSessionControl::busy:            // Remote end is busy
    RemoteNodeIsBusy();
    break;
    
  case FullFrameSessionControl::tkoffhk:         // Make it go off hook
    break;
    
  case FullFrameSessionControl::offhook:         // Line is off hook
    break;
    
  case FullFrameSessionControl::congestion:      // Congestion (circuits busy)
    break;
    
  case FullFrameSessionControl::flashhook:       // Flash hook
    ReceivedHookFlash();
    break;
    
  case FullFrameSessionControl::wink:            // Wink
    break;
    
  case FullFrameSessionControl::option:          // Set a low-level option
    break;
    
  case FullFrameSessionControl::keyRadio:        // Key Radio
    break;
    
  case FullFrameSessionControl::unkeyRadio:      // Un-Key Radio
    break;
    
  case FullFrameSessionControl::callProgress:    // Indicate PROGRESS
    /**We definately do nothing here */
    break;
    
  case FullFrameSessionControl::callProceeding:  // Indicate CALL PROCEEDING
    /**We definately do nothing here */
    break;
    
  case FullFrameSessionControl::callOnHold:      // Call has been placed on hold
    con->PauseMediaStreams(TRUE);
    break;
    
  case FullFrameSessionControl::callHoldRelease: // Call is no longer on hold
    con->PauseMediaStreams(FALSE);
    break;
    
  case FullFrameSessionControl::stopSounds:      // Moving from call setup to media flowing
    CallStopSounds();
    break;
    
  default:
    break;
  };
  
  delete(src);
  return;
}

BOOL IAX2Processor::SetUpConnection()
{
  PTRACE(2, "IAX\tSet Up Connection to remote node " << con->GetRemotePartyName());
   
  callList.AppendString(con->GetRemotePartyName());
  
  CleanPendingLists();
  return TRUE;
}

void IAX2Processor::ProcessNetworkFrame(FullFrameNull * src)
{
  PTRACE(3, "ProcessNetworkFrame(FullFrameNull * src)");
  delete src;
  return;
}

void IAX2Processor::ProcessNetworkFrame(FullFrameProtocol * src)
{ /* these frames are labelled as AST_FRAME_IAX in the asterisk souces.
     These frames contain Information Elements in the data field.*/
  
  PTRACE(3, "ProcessNetworkFrame(FullFrameProtocol * src)");

  CheckForRemoteCapabilities(src);

  src->CopyDataFromIeListTo(ieData);
  

  switch(src->GetSubClass()) {
  case FullFrameProtocol::cmdNew:
    ProcessIaxCmdNew(src);
    break;
  case FullFrameProtocol::cmdPing:
    ProcessIaxCmdPing(src);
    break;
  case FullFrameProtocol::cmdPong:
    ProcessIaxCmdPong(src);
    break;
  case FullFrameProtocol::cmdAck:
    ProcessIaxCmdAck(src);
    break;
  case FullFrameProtocol::cmdHangup:
    ProcessIaxCmdHangup(src);
    break;
  case FullFrameProtocol::cmdReject:
    ProcessIaxCmdReject(src);
    break;
  case FullFrameProtocol::cmdAccept:
    ProcessIaxCmdAccept(src);
    break;
  case FullFrameProtocol::cmdAuthReq:
    ProcessIaxCmdAuthReq(src);
    break;
  case FullFrameProtocol::cmdAuthRep:
    ProcessIaxCmdAuthRep(src);
    break;
  case FullFrameProtocol::cmdInval:
    ProcessIaxCmdInval(src);
    break;
  case FullFrameProtocol::cmdLagRq:
    ProcessIaxCmdLagRq(src);
    break;
  case FullFrameProtocol::cmdLagRp:
    ProcessIaxCmdLagRp(src);
    break;
  case FullFrameProtocol::cmdRegReq:
    ProcessIaxCmdRegReq(src);
    break;
  case FullFrameProtocol::cmdRegAuth:
    ProcessIaxCmdRegAuth(src);
    break;
  case FullFrameProtocol::cmdRegAck:
    ProcessIaxCmdRegAck(src);
    break;
  case FullFrameProtocol::cmdRegRej:
    ProcessIaxCmdRegRej(src);
    break;
  case FullFrameProtocol::cmdRegRel:
    ProcessIaxCmdRegRel(src);
    break;
  case FullFrameProtocol::cmdVnak:
    ProcessIaxCmdVnak(src);
    break;
  case FullFrameProtocol::cmdDpReq:
    ProcessIaxCmdDpReq(src);
    break;
  case FullFrameProtocol::cmdDpRep:
    ProcessIaxCmdDpRep(src);
    break;
  case FullFrameProtocol::cmdDial:
    ProcessIaxCmdDial(src);
    break;
  case FullFrameProtocol::cmdTxreq:
    ProcessIaxCmdTxreq(src);
    break;
  case FullFrameProtocol::cmdTxcnt:
    ProcessIaxCmdTxcnt(src);
    break;
  case FullFrameProtocol::cmdTxacc:
    ProcessIaxCmdTxacc(src);
    break;
  case FullFrameProtocol::cmdTxready:
    ProcessIaxCmdTxready(src);
    break;
  case FullFrameProtocol::cmdTxrel:
    ProcessIaxCmdTxrel(src);
    break;
  case FullFrameProtocol::cmdTxrej:
    ProcessIaxCmdTxrej(src);
    break;
  case FullFrameProtocol::cmdQuelch:
    ProcessIaxCmdQuelch(src);
    break;
  case FullFrameProtocol::cmdUnquelch:
    ProcessIaxCmdUnquelch(src);
    break;
  case FullFrameProtocol::cmdPoke:
    ProcessIaxCmdPoke(src);
    break;
  case FullFrameProtocol::cmdPage:
    ProcessIaxCmdPage(src);
    break;
  case FullFrameProtocol::cmdMwi:
    ProcessIaxCmdMwi(src);
    break;
  case FullFrameProtocol::cmdUnsupport:
    ProcessIaxCmdUnsupport(src);
    break;
  case FullFrameProtocol::cmdTransfer:
    ProcessIaxCmdTransfer(src);
    break;
  case FullFrameProtocol::cmdProvision:
    ProcessIaxCmdProvision(src);
    break;
  case FullFrameProtocol::cmdFwDownl:
    ProcessIaxCmdFwDownl(src);
    break;
  case FullFrameProtocol::cmdFwData:
    ProcessIaxCmdFwData(src);
    break;
  };
  
  delete src;
  return;
}


void IAX2Processor::ProcessNetworkFrame(FullFrameText * src)
{
  PTRACE(3, "ProcessNetworkFrame(FullFrameText * src)");
  delete src;
  return;
}

void IAX2Processor::ProcessNetworkFrame(FullFrameImage * src)
{
  PTRACE(3, "ProcessNetworkFrame(FullFrameImage * src)");
  delete src;
  return;
}

void IAX2Processor::ProcessNetworkFrame(FullFrameHtml * src)
{
  PTRACE(3, "ProcessNetworkFrame(FullFrameHtml * src)");
  delete src;
  return;
}

void IAX2Processor::ProcessNetworkFrame(FullFrameCng * src)
{
  PTRACE(3, "ProcessNetworkFrame(FullFrameCng * src)");
  delete src;
  return;
}

void IAX2Processor::CheckForRemoteCapabilities(FullFrameProtocol *src)
{
  unsigned int remoteCapability, format;
  
  src->GetRemoteCapability(remoteCapability, format);

  PTRACE(3, "Connection\t Remote capabilities are " << remoteCapability << "   codec preferred " << format);
  if ((remoteCapability == 0) && (format == 0))
    return;

  con->BuildRemoteCapabilityTable(remoteCapability, format);
}


BOOL IAX2Processor::RemoteSelectedCodecOk()
{
  selectedCodec = con->ChooseCodec();
  PTRACE(3, "Sound have decided on the codec " << ::hex << selectedCodec << ::dec);
  
  if (selectedCodec == 0) {
    FullFrameProtocol * reply;
    reply = new FullFrameProtocol(this, FullFrameProtocol::cmdReject, FullFrame::callIrrelevant);
    reply->AppendIe(new IeCause("Unable to negotiate codec"));
    reply->AppendIe(new IeCauseCode(IeCauseCode::BearerCapabilityNotAvail));
    TransmitFrameToRemoteEndpoint(reply);
    con->ClearCall(OpalConnection::EndedByCapabilityExchange);
    return FALSE;
  }
  
  return TRUE;
}

void IAX2Processor::ProcessIaxCmdNew(FullFrameProtocol *src)
{ /*That we are here indicates this connection is already in place */
  
  PTRACE(3, "ProcessIaxCmdNew(FullFrameProtocol *src)");
  remote.SetRemoteAddress(src->GetRemoteInfo().RemoteAddress());
  remote.SetRemotePort(src->GetRemoteInfo().RemotePort());

  con->OnSetUp();   //ONLY the  callee (person receiving call) does ownerCall.OnSetUp();

  FullFrameProtocol * reply;
  
  if (IsCallHappening()) {
    PTRACE(3, "Remote node has sent us a eecond new message. ignore");
    return;
  }
  
  if (!RemoteSelectedCodecOk()) {
    PTRACE(3, "Remote node sected a bad codec, hangup call ");
    reply = new  FullFrameProtocol(this, FullFrameProtocol::cmdInval, src, FullFrame::callIrrelevant);
    TransmitFrameToRemoteEndpoint(reply);
    
    reply= new FullFrameProtocol(this, FullFrameProtocol::cmdHangup, FullFrame::callIrrelevant);
    PTRACE(3, "Send a hangup frame to the remote endpoint as there is no codec available");		 
    reply->AppendIe(new IeCause("No matching codec"));
    //              f->AppendIe(new IeCauseCode(IeCauseCode::NormalClearing));
    TransmitFrameToRemoteEndpoint(reply);
    
    con->EndCallNow(OpalConnection::EndedByCapabilityExchange);
    return;
  }
  
  SetCallNewed();

  con->GetEndPoint().GetCodecLengths(selectedCodec, audioCompressedBytes, audioFrameDuration);
  PTRACE(3, "codec frame play duration is " << audioFrameDuration << " ms, which compressed to "
         << audioCompressedBytes << " bytes of data");


  /*At this point, we have selected a codec to use. */
  reply = new  FullFrameProtocol(this, FullFrameProtocol::cmdAccept);
  reply->AppendIe(new IeFormat(selectedCodec));
  TransmitFrameToRemoteEndpoint(reply);
  SetCallAccepted();
  
  /*We could send an AuthReq frame at this point */

  FullFrameSessionControl *r;
  r = new FullFrameSessionControl(this, FullFrameSessionControl::ringing);
  TransmitFrameToRemoteEndpoint(r, WaitingForAck::RingingAcked);
}

void IAX2Processor::ProcessIaxCmdPing(FullFrameProtocol *src)
{
  PTRACE(3, "ProcessIaxCmdPing(FullFrameProtocol *src)");
  FullFrameProtocol *f = new FullFrameProtocol(this, FullFrameProtocol::cmdPong, src, FullFrame::callIrrelevant);
  TransmitFrameToRemoteEndpoint(f);
}

void IAX2Processor::ProcessIaxCmdPong(FullFrameProtocol *src)
{
  SendAckFrame(src);  
  PTRACE(3, "ProcessIaxCmdPong(FullFrameProtocol *src)");
}

void IAX2Processor::ProcessIaxCmdAck(FullFrameProtocol *src)
{
  PTRACE(3, "ProcessIaxCmdAck(FullFrameProtocol * /*src*/)");
  /* The corresponding FullFrame has already been marked as acknowledged */
  
  
  if (!nextTask.MatchingAckPacket(src)) {
    PTRACE(3, "ack packet does not match a pending response");
    return;
  }
  
  WaitingForAck::ResponseToAck action = nextTask.GetResponse();
  nextTask.ZeroValues();
  switch(action) {
  case WaitingForAck::RingingAcked : 
    RingingWasAcked();
    break;
  case WaitingForAck::AcceptAcked  : 
    break;;
  case WaitingForAck::AuthRepAcked : 
    break;
  case WaitingForAck::AnswerAcked  : 
    AnswerWasAcked();
    break;
  }

  
}

void IAX2Processor::RingingWasAcked()
{
  PTRACE(3, "Processor\t Remote node " << con->GetRemotePartyName() << " knows our phone is ringing");
}

void IAX2Processor::AnswerWasAcked()
{
  PTRACE(3, "Answer was acked");
}

void IAX2Processor::AcceptIncomingCall()
{
  PTRACE(3, "AcceptIncomingCall()");

}

void IAX2Processor::SetEstablished(BOOL PTRACE_PARAM(originator))
{
  PTRACE(3, "Processor\tStatusCheck timer set to 10 seconds");
  StartStatusCheckTimer();

  PTRACE(3, "Processor\tOnEstablished,   Originator = " << originator);
  return;
}

void IAX2Processor::SendAnswerMessageToRemoteNode()
{
  answerCallNow = FALSE;
  StopNoResponseTimer();
  PTRACE(3, "Processor\tSend Answer message");
  FullFrameSessionControl * reply;
  reply = new FullFrameSessionControl(this, FullFrameSessionControl::answer);
  TransmitFrameToRemoteEndpoint(reply, WaitingForAck::AnswerAcked);
}

void IAX2Processor::SetConnected()
{
  PTRACE(3, "SetConnected");

  answerCallNow = TRUE;
  CleanPendingLists();
}  

BOOL IAX2Processor::SetAlerting(const PString & PTRACE_PARAM(calleeName), BOOL /*withMedia*/)
{
  PTRACE(3, "Processor\tSetAlerting from " << calleeName);
  return TRUE;
}

/*The remote node has told us to hangup and go away */
void IAX2Processor::ProcessIaxCmdHangup(FullFrameProtocol *src)
{ 
  PTRACE(3, "ProcessIaxCmdHangup(FullFrameProtocol *src)");
  SendAckFrame(src);
  cerr << "The remote node (" << con->GetRemotePartyName()  << ") has closed the call" << endl;
  con->EndCallNow(OpalConnection::EndedByRemoteUser);
}

void IAX2Processor::ProcessIaxCmdReject(FullFrameProtocol *src)
{
  PTRACE(3, "ProcessIaxCmdReject(FullFrameProtocol *src)");
  cout << "Remote endpoint has rejected our call " << endl;
  cout << "Cause \"" << ieData.cause << "\"" << endl;
  SendAckFrame(src);
  con->EndCallNow(OpalConnection::EndedByRefusal);
}

void IAX2Processor::ProcessIaxCmdAccept(FullFrameProtocol *src)
{
  con->OnAlerting();

  PTRACE(3, "ProcessIaxCmdAccept(FullFrameProtocol *src)");
  StopNoResponseTimer();
    
  if (IsCallAccepted()) {
    PTRACE(3, "Second accept packet received. Ignore it");
    return;
  }
  
  SendAckFrame(src);
  SetCallAccepted();
  
  PTRACE(3, "Now check codecs");
  
  if (!RemoteSelectedCodecOk()) {
    PTRACE(3, "Remote node sected a bad codec, hangup call ");
    Release();
    return;
  }
  PString codecName = FullFrameVoice::GetOpalNameOfCodec((unsigned short)selectedCodec);
  PTRACE(3, "The remote endpoint has accepted our call on codec " << codecName);
  con->GetEndPoint().GetCodecLengths(selectedCodec, audioCompressedBytes, audioFrameDuration);
  PTRACE(3, "codec frame play duration is " << audioFrameDuration << " ms, which compressed to " 
	 << audioCompressedBytes << " bytes of data");
}

void IAX2Processor::ProcessIaxCmdAuthReq(FullFrameProtocol * /*src*/)
{
  PTRACE(3, "ProcessIaxCmdAuthReq(FullFrameProtocol *src)");
  StopNoResponseTimer();
  FullFrameProtocol *f = new FullFrameProtocol(this, FullFrameProtocol::cmdAuthRep);
  
  Authenticate(f);
  
  TransmitFrameToRemoteEndpoint(f);
  StartNoResponseTimer();
}

void IAX2Processor::ProcessIaxCmdAuthRep(FullFrameProtocol * /*src*/)
{
  PTRACE(3, "ProcessIaxCmdAuthRep(FullFrameProtocol *src)");
  /** When this packet has been acked, we send an accept */     
}

void IAX2Processor::ProcessIaxCmdInval(FullFrameProtocol * src)
{
  PTRACE(3, "ProcessIaxCmdInval(FullFrameProtocol *src) " << src->IdString());
  PTRACE(3, "ProcessIaxCmdInval(FullFrameProtocol *src) " << src->GetSequenceInfo().AsString());
  PTRACE(3, "ProcessIaxCmdInval(FullFrameProtocol *src) " << src->GetTimeStamp());

  if (src->GetSequenceInfo().IsSequenceNosZero() && (src->GetTimeStamp() == 0)) {
    PTRACE(3, "ProcessIaxCmdInval - remote end does not like us, and nuked the call");
    con->ClearCall(OpalConnection::EndedByRemoteUser);
  }
}

void IAX2Processor::ProcessIaxCmdLagRq(FullFrameProtocol *src)
{
  PTRACE(3, "ProcessIaxCmdLagRq(FullFrameProtocol *src)");
  FullFrameProtocol *f = new FullFrameProtocol(this, FullFrameProtocol::cmdLagRp, src, FullFrame::callIrrelevant);
  TransmitFrameToRemoteEndpoint(f);
}

void IAX2Processor::ProcessIaxCmdLagRp(FullFrameProtocol *src)
{
  PTRACE(3, "ProcessIaxCmdLagRp(FullFrameProtocol *src)");
  SendAckFrame(src);
  PTRACE(3, "Process\tRound trip lag time is " << (Frame::CalcTimeStamp(callStartTime) - src->GetTimeStamp()));
}

void IAX2Processor::ProcessIaxCmdRegReq(FullFrameProtocol *src)
{
  PTRACE(3, "ProcessIaxCmdRegReq(FullFrameProtocol *src)");
  SendAckFrame(src);
}

void IAX2Processor::ProcessIaxCmdRegAuth(FullFrameProtocol * /*src*/) /* The regauth contains the challenge, which we have to solve*/
{                                                                  /* We put our reply into the cmdRegReq frame */
  PTRACE(3, "ProcessIaxCmdRegAuth(FullFrameProtocol *src)");
  
  FullFrameProtocol *f = new FullFrameProtocol(this, FullFrameProtocol::cmdRegReq);
  
  Authenticate(f);
  
  TransmitFrameToRemoteEndpoint(f); 
  StartNoResponseTimer();
  
}

void IAX2Processor::ProcessIaxCmdRegAck(FullFrameProtocol *src)
{
  PTRACE(3, "ProcessIaxCmdRegAck(FullFrameProtocol *src)");
  StopNoResponseTimer();
  SendAckFrame(src);
}

void IAX2Processor::ProcessIaxCmdRegRej(FullFrameProtocol * /*src*/)
{
  PTRACE(3, "ProcessIaxCmdRegRej(FullFrameProtocol *src)");
}

void IAX2Processor::ProcessIaxCmdRegRel(FullFrameProtocol * /*src*/)
{
  PTRACE(3, "ProcessIaxCmdRegRel(FullFrameProtocol *src)");
}

void IAX2Processor::ProcessIaxCmdVnak(FullFrameProtocol * /*src*/)
{
  PTRACE(3, "ProcessIaxCmdVnak(FullFrameProtocol *src)");
}

void IAX2Processor::ProcessIaxCmdDpReq(FullFrameProtocol * /*src*/)
{
  PTRACE(3, "ProcessIaxCmdDpReq(FullFrameProtocol *src)");
}

void IAX2Processor::ProcessIaxCmdDpRep(FullFrameProtocol * /*src*/)
{
  PTRACE(3, "ProcessIaxCmdDpRep(FullFrameProtocol *src)");
}

void IAX2Processor::ProcessIaxCmdDial(FullFrameProtocol * /*src*/)
{
  PTRACE(3, "ProcessIaxCmdDial(FullFrameProtocol *src)");
}

void IAX2Processor::ProcessIaxCmdTxreq(FullFrameProtocol * /*src*/)
{
  PTRACE(3, "ProcessIaxCmdTxreq(FullFrameProtocol */*src*/)");
  //Not implemented.
}

void IAX2Processor::ProcessIaxCmdTxcnt(FullFrameProtocol * /*src*/)
{
  PTRACE(3, "ProcessIaxCmdTxcnt(FullFrameProtocol * /*src*/)");
  //Not implemented.
}

void IAX2Processor::ProcessIaxCmdTxacc(FullFrameProtocol * /*src*/)
{ /*Transfer has been accepted */
  PTRACE(3, "ProcessIaxCmdTxacc(FullFrameProtocol * /*src*/)");
  //Not implemented.
}

void IAX2Processor::ProcessIaxCmdTxready(FullFrameProtocol * /*src*/)
{
  PTRACE(3, "ProcessIaxCmdTxready(FullFrameProtocol */*src*/)");
  //Not implemented.
}

void IAX2Processor::ProcessIaxCmdTxrel(FullFrameProtocol * /*src*/)
{
  PTRACE(3, "ProcessIaxCmdTxrel(FullFrameProtocol */*src*/)");
  //Not implemented.
}

void IAX2Processor::ProcessIaxCmdTxrej(FullFrameProtocol * /*src*/)
{
  PTRACE(3, "ProcessIaxCmdTxrej(FullFrameProtocol */*src*/)");
  //Not implemented. (transfer rejected)
}

void IAX2Processor::ProcessIaxCmdQuelch(FullFrameProtocol * /*src*/)
{
  cerr << "Quelch message received" << endl;
  PTRACE(3, "ProcessIaxCmdQuelch(FullFrameProtocol */*src*/)");
  audioCanFlow = FALSE;
}

void IAX2Processor::ProcessIaxCmdUnquelch(FullFrameProtocol * /*src*/)
{
  PTRACE(3, "ProcessIaxCmdUnquelch(FullFrameProtocol */*src*/)");
  audioCanFlow = TRUE;
}

void IAX2Processor::ProcessIaxCmdPoke(FullFrameProtocol * /*src*/)
{
  PTRACE(3, "ProcessIaxCmdPoke(FullFrameProtocol *src)");
}

void IAX2Processor::ProcessIaxCmdPage(FullFrameProtocol * /*src*/)
{
  PTRACE(3, "ProcessIaxCmdPage(FullFrameProtocol *src)");
}

void IAX2Processor::ProcessIaxCmdMwi(FullFrameProtocol * /*src*/)
{
  PTRACE(3, "ProcessIaxCmdMwi(FullFrameProtocol *src)");
}

void IAX2Processor::ProcessIaxCmdUnsupport(FullFrameProtocol * /*src*/)
{
  PTRACE(3, "ProcessIaxCmdUnsupport(FullFrameProtocol *src)");
}

void IAX2Processor::ProcessIaxCmdTransfer(FullFrameProtocol * /*src*/)
{
  PTRACE(3, "ProcessIaxCmdTransfer(FullFrameProtocol *src)");
}

void IAX2Processor::ProcessIaxCmdProvision(FullFrameProtocol * /*src*/)
{
  PTRACE(3, "ProcessIaxCmdProvision(FullFrameProtocol *src)");
}

void IAX2Processor::ProcessIaxCmdFwDownl(FullFrameProtocol * /*src*/)
{
  PTRACE(3, "ProcessIaxCmdFwDownl(FullFrameProtocol *src)");
}

void IAX2Processor::ProcessIaxCmdFwData(FullFrameProtocol * /*src*/)
{
  PTRACE(3, "ProcessIaxCmdFwData(FullFrameProtocol *src)");
}

BOOL IAX2Processor::Authenticate(FullFrameProtocol *reply)
{
  BOOL processed = FALSE;
  IeAuthMethods ie(ieData.authMethods);
  
  if (ie.IsRsaAuthentication()) {
    PTRACE(3, "DO NOT handle RSA authentication ");
    reply->SetSubClass(FullFrameProtocol::cmdInval);
    processed = TRUE;
  }
  
  if (ie.IsMd5Authentication()) {
    PTRACE(3, "Processor\tMD5 Authentiction yes, make reply up");
    IeMd5Result *res = new IeMd5Result(ieData.challenge, con->GetEndPoint().GetPassword());
    reply->AppendIe(res);
    processed = TRUE;
    encryption.SetChallengeKey(ieData.challenge);
    encryption.SetEncryptionKey(con->GetEndPoint().GetPassword());
  }
  
  if (ie.IsPlainTextAuthentication() && (!processed)) {
    reply->AppendIe(new IePassword(con->GetEndPoint().GetPassword()));
    processed = TRUE;
  }
  
  if (ieData.encryptionMethods == IeEncryption::encryptAes128) {
    PTRACE(3, "Processor\tEnable AES 128 encryption");
    encryption.SetEncryptionOn();
    reply->AppendIe(new IeEncryption);
  }

  return processed;
}

void IAX2Processor::StartStatusCheckTimer(PINDEX msToWait)
{
  PTRACE(3, "Processor\tStatusCheck time. Now set flag to  send a ping and a lagrq packet");

  statusCheckTimer = PTimeInterval(msToWait); 
  statusCheckOtherEnd = TRUE;
  CleanPendingLists();
}

void IAX2Processor::OnStatusCheck(PTimer &, INT)
{
  StartStatusCheckTimer();
}

void IAX2Processor::OnNoResponseTimeout(PTimer &, INT)
{
  PTRACE(3, "hangup now, as we have had no response from the remote node in the specified time ");
  cout << "no answer in specified time period. End this call " << endl;
  
  con->ClearCall(OpalConnection::EndedByNoAnswer);
}

void IAX2Processor::StartNoResponseTimer(PINDEX msToWait) 
{
  if (msToWait == 0)
    msToWait = NoResponseTimePeriod;
  
  noResponseTimer = PTimeInterval(msToWait); 
}

void IAX2Processor::RemoteNodeIsRinging()
{
  StopNoResponseTimer();
  
  StartNoResponseTimer(60 * 1000); /* The remote end has one minute to answer their phone */
}


void IAX2Processor::IncomingEthernetFrame(Frame *frame)
{
  frameList.AddNewFrame(frame);
  
  CleanPendingLists();
} 

Frame * IAX2Processor::GetSoundPacketFromNetwork()
{
  Frame * newSound = soundReadFromEthernet.GetLastFrame();
  if (newSound == NULL) {
    PTRACE(3, "OpalMediaStream\t NULL sound packet on stack ");
    return NULL;
  }
     
  PTRACE(3, "OpalMediaStream\tSend frame to media stream " << newSound->IdString());
  return newSound;
} 

void IAX2Processor::SetCallToken(PString newToken) 
{
  callToken = newToken;
} 

PString IAX2Processor::GetCallToken()
{ 
  return callToken;
}

void IAX2Processor::DoStatusCheck()
{
  statusCheckOtherEnd = FALSE;
  FullFrame *p = new FullFrameProtocol(this, FullFrameProtocol::cmdPing);
  TransmitFrameToRemoteEndpoint(p);

  p = new FullFrameProtocol(this, FullFrameProtocol::cmdLagRq);
  TransmitFrameToRemoteEndpoint(p);
}
