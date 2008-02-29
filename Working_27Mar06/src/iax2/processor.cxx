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
 * Revision 1.14  2005/09/05 01:19:44  dereksmithies
 * add patches from Adrian Sietsma to avoid multiple hangup packets at call end,
 * and stop the sending of ping/lagrq packets at call end. Many thanks.
 *
 * Revision 1.13  2005/08/26 03:26:51  dereksmithies
 * Add some tidyups from Adrian Sietsma.  Many thanks..
 *
 * Revision 1.12  2005/08/26 03:07:38  dereksmithies
 * Change naming convention, so all class names contain the string "IAX2"
 *
 * Revision 1.11  2005/08/25 03:26:06  dereksmithies
 * Add patch from Adrian Sietsma to correctly set the packet timestamps under windows.
 * Many thanks.
 *
 * Revision 1.10  2005/08/25 00:46:08  dereksmithies
 * Thanks to Adrian Sietsma for his code to better dissect the remote party name
 * Add  PTRACE statements, and more P_SSL_AES tests
 *
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

IAX2WaitingForAck::IAX2WaitingForAck()
{
  ZeroValues();
}

void IAX2WaitingForAck::Assign(IAX2FullFrame *f, ResponseToAck _response)
{
  timeStamp = f->GetTimeStamp();
  seqNo     = f->GetSequenceInfo().InSeqNo();
  response  = _response;
  PTRACE(3, "MatchingAck\tIs looking for " << timeStamp << " and " << seqNo << " to do " << GetResponseAsString());
}

void IAX2WaitingForAck::ZeroValues()
{
  timeStamp = 0;
  seqNo = 0;
  //     response = sendNothing;
}

BOOL IAX2WaitingForAck::MatchingAckPacket(IAX2FullFrame *f)
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

void IAX2WaitingForAck::PrintOn(ostream & strm) const
{
  strm << "time " << timeStamp << "    seq " << seqNo << "     " << GetResponseAsString();
}

PString IAX2WaitingForAck::GetResponseAsString() const
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
       << "  Call has been up for " << setprecision(0) << setw(8)
       << (PTimer::Tick() - callStartTick) << " milliseconds" << endl
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

BOOL IAX2Processor::IsStatusQueryEthernetFrame(IAX2Frame *frame)
{
  if (!PIsDescendant(frame, IAX2FullFrame))
    return FALSE;
   
  IAX2FullFrame *f = (IAX2FullFrame *)frame;
  if (f->GetFrameType() != IAX2FullFrame::iax2ProtocolType)
    return FALSE;
   
  PINDEX subClass = f->GetSubClass();
   
  if (subClass == IAX2FullFrameProtocol::cmdLagRq) {
    PTRACE(3, "Special packet of  lagrq to process");
    return TRUE;
  }
   
  if (subClass == IAX2FullFrameProtocol::cmdPing) {
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
  if (hangList.IsEmpty()) 
    return;

  if (!IsCallTerminating()) {
    IAX2FullFrameProtocol * f = 
      new IAX2FullFrameProtocol(this, IAX2FullFrameProtocol::cmdHangup, IAX2FullFrame::callIrrelevant);
    PTRACE(3, "Send a hangup frame to the remote endpoint");
    
    f->AppendIe(new IAX2IeCause(hangList.GetFirstDeleteAll()));
    //        f->AppendIe(new IeCauseCode(IAX2IeCauseCode::NormalClearing));
    TransmitFrameToRemoteEndpoint(f);
  } else {
    PTRACE(3, "hangup message required. Not sending, cause already have a hangup message in queue");
  }
  
  Terminate();
}

void IAX2Processor::ConnectToRemoteNode(PString & newRemoteNode)
{      // Parse a string like guest@node.name.com/23234
  
  PTRACE(2, "Connect to remote node " << newRemoteNode);
  PStringList res = IAX2EndPoint::DissectRemoteParty(newRemoteNode);

  if (res[IAX2EndPoint::addressIndex].IsEmpty()) {
    PTRACE(3, "Opal\tremote node to call is not specified correctly iax2:" << newRemoteNode);
    PTRACE(3, "Opal\tExample format is iax2:guest@misery.digium.com/s");
    PTRACE(3, "Opal\tYou must supply (as a minimum iax2:address)");
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
    
  callStartTick = PTimer::Tick();
  IAX2FullFrameProtocol * f = new IAX2FullFrameProtocol(this, IAX2FullFrameProtocol::cmdNew);
  PTRACE(3, "Create full frame protocol to do cmdNew. Just contains data. ");
  f->AppendIe(new IAX2IeVersion());
  f->AppendIe(new IAX2IeFormat(con->GetPreferredCodec()));
  f->AppendIe(new IAX2IeCapability(con->GetSupportedCodecs()));
  
  if (!endpoint.GetLocalNumber().IsEmpty())
    f->AppendIe(new IAX2IeCallingNumber(endpoint.GetLocalNumber()));

  if (!endpoint.GetLocalUserName().IsEmpty())
    f->AppendIe(new IAX2IeCallingName(endpoint.GetLocalUserName()));
  
  if (!res[IAX2EndPoint::userIndex].IsEmpty())
    f->AppendIe(new IAX2IeUserName(res[IAX2EndPoint::userIndex]));

  if (!res[IAX2EndPoint::extensionIndex].IsEmpty())
    f->AppendIe(new IAX2IeCalledNumber(res[IAX2EndPoint::extensionIndex] ));

  if (!res[IAX2EndPoint::extensionIndex].IsEmpty())
    f->AppendIe(new IAX2IeDnid(res[IAX2EndPoint::extensionIndex]));

  if (!res[IAX2EndPoint::contextIndex].IsEmpty())
    f->AppendIe(new IAX2IeCalledContext(res[IAX2EndPoint::contextIndex]));

#if P_SSL_AES
  f->AppendIe(new IAX2IeEncryption());
#endif

  PTRACE(3, "Create full frame protocol to do cmdNew. Finished appending Ies. ");
  TransmitFrameToRemoteEndpoint(f);
  StartNoResponseTimer();
  return;
}

void IAX2Processor::ReportStatistics()
{
  cout << "  Call has been up for " << setprecision(0) << setw(8)
       << (PTimer::Tick() - callStartTick) << " milliseconds" << endl
       << "  Control frames sent " << controlFramesSent  << endl
       << "  Control frames rcvd " << controlFramesRcvd  << endl
       << "  Audio frames sent   " << audioFramesSent      << endl
       << "  Audio frames rcvd   " << audioFramesRcvd      << endl
       << "  Video frames sent   " << videoFramesSent      << endl
       << "  Video frames rcvd   " << videoFramesRcvd      << endl;
}

BOOL IAX2Processor::ProcessOneIncomingEthernetFrame()
{ 
  IAX2Frame *frame = frameList.GetLastFrame();
  if (frame == NULL) {
    return FALSE;
  }
  
  PTRACE(3, "IaxConnection\tUnknown  incoming frame " << frame->IdString());
  IAX2Frame *af = frame->BuildAppropriateFrameType(encryption);
  delete frame;

  if (af == NULL)
    return TRUE;
  frame = af;
  
  if (PIsDescendant(frame, IAX2MiniFrame)) {
    PTRACE(3, "IaxConnection\tIncoming mini frame" << frame->IdString());
    ProcessNetworkFrame((IAX2MiniFrame *)frame);
    return TRUE;
  }
  
  IAX2FullFrame *f = (IAX2FullFrame *) frame;
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
  case IAX2FullFrame::dtmfType:        
    PTRACE(3, "Build matching full frame    dtmfType");
    ProcessNetworkFrame(new IAX2FullFrameDtmf(*f));
    break;
  case IAX2FullFrame::voiceType:       
    PTRACE(3, "Build matching full frame    voiceType");
    ProcessNetworkFrame(new IAX2FullFrameVoice(*f));
    break;
  case IAX2FullFrame::videoType:       
    PTRACE(3, "Build matching full frame    videoType");
    ProcessNetworkFrame(new IAX2FullFrameVideo(*f));
    break;
  case IAX2FullFrame::controlType:     
    PTRACE(3, "Build matching full frame    controlType");
    ProcessNetworkFrame(new IAX2FullFrameSessionControl(*f));
    break;
  case IAX2FullFrame::nullType:        
    PTRACE(3, "Build matching full frame    nullType");
    ProcessNetworkFrame(new IAX2FullFrameNull(*f));
    break;
  case IAX2FullFrame::iax2ProtocolType: 
    PTRACE(3, "Build matching full frame    iax2ProtocolType");
    ProcessNetworkFrame(new IAX2FullFrameProtocol(*f));
    break;
  case IAX2FullFrame::textType:        
    PTRACE(3, "Build matching full frame    textType");
    ProcessNetworkFrame(new IAX2FullFrameText(*f));
    break;
  case IAX2FullFrame::imageType:       
    PTRACE(3, "Build matching full frame    imageType");
    ProcessNetworkFrame(new IAX2FullFrameImage(*f));
    break;
  case IAX2FullFrame::htmlType:        
    PTRACE(3, "Build matching full frame    htmlType");
    ProcessNetworkFrame(new IAX2FullFrameHtml(*f));
    break;
  case IAX2FullFrame::cngType:        
    PTRACE(3, "Build matching full frame    cngType");
    ProcessNetworkFrame(new IAX2FullFrameCng(*f));
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
  
  if (!dtmfText.IsEmpty()) {
    PString dtmfs = dtmfText.GetAndDelete();
   PTRACE(3, "Have " << dtmfs << " DTMF chars to send");
    for (PINDEX i = 0; i < dtmfs.GetLength(); i++)
      SendDtmfMessage(dtmfs[i]);
  }  

  if (!textList.IsEmpty()) {
   PStringArray sendList; // text messages
   textList.GetAllDeleteAll(sendList);
   PTRACE(3, "Have " << sendList.GetSize() << " text strings to send");
   for (PINDEX i = 0; i < sendList.GetSize(); i++)
     SendTextMessage(sendList[i]);    
   }

  if (answerCallNow)
    SendAnswerMessageToRemoteNode();

  if (statusCheckOtherEnd)
    DoStatusCheck();

  CheckForHangupMessages();
}

void IAX2Processor::ProcessIncomingAudioFrame(IAX2Frame *newFrame)
{
  PTRACE(3, "Processor\tPlace audio frame on queue " << newFrame->IdString());
  
  IncAudioFramesRcvd();
  
  soundReadFromEthernet.AddNewFrame(newFrame);
  PTRACE(3, "have " << soundReadFromEthernet.GetSize() << " pending packets on incoming sound queue");
}

void IAX2Processor::ProcessIncomingVideoFrame(IAX2Frame *newFrame)
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
  DWORD thisTimeStamp = (DWORD)(PTimer::Tick() - callStartTick).GetMilliSeconds();
  PTRACE(3, "This frame is duration " << thisDuration << " ms   at time " << thisTimeStamp);

  thisTimeStamp = ((thisTimeStamp + (thisDuration > 1))/thisDuration) * thisDuration;
  DWORD lastTimeStamp = thisTimeStamp - thisDuration;

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
      IAX2FullFrameVoice *f = new IAX2FullFrameVoice(this, *sound, thisTimeStamp);
      PTRACE(3, "Send a full audio frame" << thisDuration << " On " << f->IdString());
      TransmitFrameToRemoteEndpoint(f);
  } else {
      IAX2MiniFrame *f = new IAX2MiniFrame(this, *sound, TRUE, thisTimeStamp & 0xffff);
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
  IAX2FullFrameDtmf *f = new IAX2FullFrameDtmf(this, message);
  TransmitFrameToRemoteEndpoint(f);
}

void IAX2Processor::SendTextMessage(PString & message)
{
  IAX2FullFrameText *f = new IAX2FullFrameText(this, message);
  TransmitFrameToRemoteEndpoint(f);
}

void IAX2Processor::SendAckFrame(IAX2FullFrame *inReplyTo)
{
  PTRACE(3, "Processor\tSend an ack frame in reply" );
  PTRACE(3, "Processor\tIn reply to " << *inReplyTo);
  IAX2FullFrameProtocol *f = new IAX2FullFrameProtocol(this, IAX2FullFrameProtocol::cmdAck, inReplyTo); //, IAX2FullFrame::callIrrelevant);
  PTRACE(4, "Swquence for sending is (pre) " << sequence.AsString());
  TransmitFrameToRemoteEndpoint(f);
  PTRACE(4, "Sequence for sending is (ppost) " << sequence.AsString());
}

void IAX2Processor::TransmitFrameNow(IAX2Frame *src)
{
  if (!src->EncryptContents(encryption)) {
    PTRACE(3, "Processor\tEncryption failed. Delete this frame " << *src);
    delete src;
    return;
  }
  endpoint.transmitter->SendFrame(src);
}

void IAX2Processor::TransmitFrameToRemoteEndpoint(IAX2FullFrameProtocol *src)
{
  src->WriteIeAsBinaryData();
  TransmitFrameToRemoteEndpoint((IAX2Frame *)src);
}

void IAX2Processor::TransmitFrameToRemoteEndpoint(IAX2Frame *src)
{
  PTRACE(3, "Send frame " << src->GetClass() << " " << src->IdString() << " to remote endpoint");
  if (src->IsFullFrame()) {
    PTRACE(3, "Send full frame " << src->GetClass() << " with seq increase");
    sequence.MassageSequenceForSending(*(IAX2FullFrame*)src);
    IncControlFramesSent();
  }
  TransmitFrameNow(src);
}

void IAX2Processor::TransmitFrameToRemoteEndpoint(IAX2FullFrame *src, IAX2WaitingForAck::ResponseToAck response)
{
  sequence.MassageSequenceForSending(*(IAX2FullFrame*)src);
  IncControlFramesSent();
  nextTask.Assign(src, response);
  TransmitFrameNow(src);
}

void IAX2Processor::ProcessNetworkFrame(IAX2MiniFrame * src)
{
  PTRACE(3, "ProcessNetworkFrame(IAX2MiniFrame * src)");
  
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


void IAX2Processor::ProcessNetworkFrame(IAX2FullFrame * src)
{
  PTRACE(3, "ProcessNetworkFrame(IAX2FullFrame * src)");
  PStringStream message;
  message << PString("Do not know how to process networks packets of \"Full Frame\" type ") << *src;
  PAssertAlways(message);
  return;
}

void IAX2Processor::ProcessNetworkFrame(IAX2Frame * src)
{
  PTRACE(3, "ProcessNetworkFrame(IAX2Frame * src)");
  PStringStream message;
  message << PString("Do not know how to process networks packets of \"Frame\" type ") << *src;
  PTRACE(3, message);
  PTRACE(3, message);
  PAssertAlways(message);
  return;
}

void IAX2Processor::ProcessNetworkFrame(IAX2FullFrameDtmf * src)
{
  PTRACE(3, "ProcessNetworkFrame(IAX2FullFrameDtmf * src)");
  SendAckFrame(src);
  con->OnUserInputTone((char)src->GetSubClass(), 1);

  delete src;
}

void IAX2Processor::ProcessNetworkFrame(IAX2FullFrameVoice * src)
{
  if (firstMediaFrame) {
    PTRACE(3, "Processor\tReceived first voice media frame " << src->IdString());
    firstMediaFrame = FALSE;
  }
  PTRACE(3, "ProcessNetworkFrame(IAX2FullFrameVoice * src)" << src->IdString());
  SendAckFrame(src);
  ProcessIncomingAudioFrame(src);
  
  return;
}

void IAX2Processor::ProcessNetworkFrame(IAX2FullFrameVideo * src)
{
  if (firstMediaFrame) {
    PTRACE(3, "Processor\tReceived first video media frame ");
    firstMediaFrame = FALSE;
  }

  PTRACE(3, "ProcessNetworkFrame(IAX2FullFrameVideo * src)");
  SendAckFrame(src);
  ProcessIncomingVideoFrame(src);
  
  return;
}

void IAX2Processor::ProcessNetworkFrame(IAX2FullFrameSessionControl * src)
{ /* these frames are labelled as AST_FRAME_CONTROL in the asterisk souces.
     We could get an Answer message from here., or a hangup., or...congestion control... */
  
  PTRACE(3, "ProcessNetworkFrame(IAX2FullFrameSessionControl * src)");
  SendAckFrame(src);  
  
  switch(src->GetSubClass()) {
  case IAX2FullFrameSessionControl::hangup:          // Other end has hungup
    SetCallTerminating(TRUE);
    cout << "Other end has hungup, so exit" << endl;
    con->EndCallNow();
    break;
    
  case IAX2FullFrameSessionControl::ring:            // Local ring
    break;
    
  case IAX2FullFrameSessionControl::ringing:         // Remote end is ringing
    RemoteNodeIsRinging();
    break;
    
  case IAX2FullFrameSessionControl::answer:          // Remote end has answered
    PTRACE(3, "Have received answer packet from remote endpoint ");    
    RemoteNodeHasAnswered();
    break;
    
  case IAX2FullFrameSessionControl::busy:            // Remote end is busy
    RemoteNodeIsBusy();
    break;
    
  case IAX2FullFrameSessionControl::tkoffhk:         // Make it go off hook
    break;
    
  case IAX2FullFrameSessionControl::offhook:         // Line is off hook
    break;
    
  case IAX2FullFrameSessionControl::congestion:      // Congestion (circuits busy)
    break;
    
  case IAX2FullFrameSessionControl::flashhook:       // Flash hook
    ReceivedHookFlash();
    break;
    
  case IAX2FullFrameSessionControl::wink:            // Wink
    break;
    
  case IAX2FullFrameSessionControl::option:          // Set a low-level option
    break;
    
  case IAX2FullFrameSessionControl::keyRadio:        // Key Radio
    break;
    
  case IAX2FullFrameSessionControl::unkeyRadio:      // Un-Key Radio
    break;
    
  case IAX2FullFrameSessionControl::callProgress:    // Indicate PROGRESS
    /**We definately do nothing here */
    break;
    
  case IAX2FullFrameSessionControl::callProceeding:  // Indicate CALL PROCEEDING
    /**We definately do nothing here */
    break;
    
  case IAX2FullFrameSessionControl::callOnHold:      // Call has been placed on hold
    con->PauseMediaStreams(TRUE);
    break;
    
  case IAX2FullFrameSessionControl::callHoldRelease: // Call is no longer on hold
    con->PauseMediaStreams(FALSE);
    break;
    
  case IAX2FullFrameSessionControl::stopSounds:      // Moving from call setup to media flowing
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

void IAX2Processor::ProcessNetworkFrame(IAX2FullFrameNull * src)
{
  PTRACE(3, "ProcessNetworkFrame(IAX2FullFrameNull * src)");
  delete src;
  return;
}

void IAX2Processor::ProcessNetworkFrame(IAX2FullFrameProtocol * src)
{ /* these frames are labelled as AST_FRAME_IAX in the asterisk souces.
     These frames contain Information Elements in the data field.*/
  
  PTRACE(3, "ProcessNetworkFrame(IAX2FullFrameProtocol * src)");

  CheckForRemoteCapabilities(src);

  src->CopyDataFromIeListTo(ieData);
  

  switch(src->GetSubClass()) {
  case IAX2FullFrameProtocol::cmdNew:
    ProcessIaxCmdNew(src);
    break;
  case IAX2FullFrameProtocol::cmdPing:
    ProcessIaxCmdPing(src);
    break;
  case IAX2FullFrameProtocol::cmdPong:
    ProcessIaxCmdPong(src);
    break;
  case IAX2FullFrameProtocol::cmdAck:
    ProcessIaxCmdAck(src);
    break;
  case IAX2FullFrameProtocol::cmdHangup:
    ProcessIaxCmdHangup(src);
    break;
  case IAX2FullFrameProtocol::cmdReject:
    ProcessIaxCmdReject(src);
    break;
  case IAX2FullFrameProtocol::cmdAccept:
    ProcessIaxCmdAccept(src);
    break;
  case IAX2FullFrameProtocol::cmdAuthReq:
    ProcessIaxCmdAuthReq(src);
    break;
  case IAX2FullFrameProtocol::cmdAuthRep:
    ProcessIaxCmdAuthRep(src);
    break;
  case IAX2FullFrameProtocol::cmdInval:
    ProcessIaxCmdInval(src);
    break;
  case IAX2FullFrameProtocol::cmdLagRq:
    ProcessIaxCmdLagRq(src);
    break;
  case IAX2FullFrameProtocol::cmdLagRp:
    ProcessIaxCmdLagRp(src);
    break;
  case IAX2FullFrameProtocol::cmdRegReq:
    ProcessIaxCmdRegReq(src);
    break;
  case IAX2FullFrameProtocol::cmdRegAuth:
    ProcessIaxCmdRegAuth(src);
    break;
  case IAX2FullFrameProtocol::cmdRegAck:
    ProcessIaxCmdRegAck(src);
    break;
  case IAX2FullFrameProtocol::cmdRegRej:
    ProcessIaxCmdRegRej(src);
    break;
  case IAX2FullFrameProtocol::cmdRegRel:
    ProcessIaxCmdRegRel(src);
    break;
  case IAX2FullFrameProtocol::cmdVnak:
    ProcessIaxCmdVnak(src);
    break;
  case IAX2FullFrameProtocol::cmdDpReq:
    ProcessIaxCmdDpReq(src);
    break;
  case IAX2FullFrameProtocol::cmdDpRep:
    ProcessIaxCmdDpRep(src);
    break;
  case IAX2FullFrameProtocol::cmdDial:
    ProcessIaxCmdDial(src);
    break;
  case IAX2FullFrameProtocol::cmdTxreq:
    ProcessIaxCmdTxreq(src);
    break;
  case IAX2FullFrameProtocol::cmdTxcnt:
    ProcessIaxCmdTxcnt(src);
    break;
  case IAX2FullFrameProtocol::cmdTxacc:
    ProcessIaxCmdTxacc(src);
    break;
  case IAX2FullFrameProtocol::cmdTxready:
    ProcessIaxCmdTxready(src);
    break;
  case IAX2FullFrameProtocol::cmdTxrel:
    ProcessIaxCmdTxrel(src);
    break;
  case IAX2FullFrameProtocol::cmdTxrej:
    ProcessIaxCmdTxrej(src);
    break;
  case IAX2FullFrameProtocol::cmdQuelch:
    ProcessIaxCmdQuelch(src);
    break;
  case IAX2FullFrameProtocol::cmdUnquelch:
    ProcessIaxCmdUnquelch(src);
    break;
  case IAX2FullFrameProtocol::cmdPoke:
    ProcessIaxCmdPoke(src);
    break;
  case IAX2FullFrameProtocol::cmdPage:
    ProcessIaxCmdPage(src);
    break;
  case IAX2FullFrameProtocol::cmdMwi:
    ProcessIaxCmdMwi(src);
    break;
  case IAX2FullFrameProtocol::cmdUnsupport:
    ProcessIaxCmdUnsupport(src);
    break;
  case IAX2FullFrameProtocol::cmdTransfer:
    ProcessIaxCmdTransfer(src);
    break;
  case IAX2FullFrameProtocol::cmdProvision:
    ProcessIaxCmdProvision(src);
    break;
  case IAX2FullFrameProtocol::cmdFwDownl:
    ProcessIaxCmdFwDownl(src);
    break;
  case IAX2FullFrameProtocol::cmdFwData:
    ProcessIaxCmdFwData(src);
    break;
  };
  
  delete src;
  return;
}


void IAX2Processor::ProcessNetworkFrame(IAX2FullFrameText * src)
{
  PTRACE(3, "ProcessNetworkFrame(IAX2FullFrameText * src)");
  delete src;
  return;
}

void IAX2Processor::ProcessNetworkFrame(IAX2FullFrameImage * src)
{
  PTRACE(3, "ProcessNetworkFrame(IAX2FullFrameImage * src)");
  delete src;
  return;
}

void IAX2Processor::ProcessNetworkFrame(IAX2FullFrameHtml * src)
{
  PTRACE(3, "ProcessNetworkFrame(IAX2FullFrameHtml * src)");
  delete src;
  return;
}

void IAX2Processor::ProcessNetworkFrame(IAX2FullFrameCng * src)
{
  PTRACE(3, "ProcessNetworkFrame(IAX2FullFrameCng * src)");
  delete src;
  return;
}

void IAX2Processor::CheckForRemoteCapabilities(IAX2FullFrameProtocol *src)
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
    IAX2FullFrameProtocol * reply;
    reply = new IAX2FullFrameProtocol(this, IAX2FullFrameProtocol::cmdReject, IAX2FullFrame::callIrrelevant);
    reply->AppendIe(new IAX2IeCause("Unable to negotiate codec"));
    reply->AppendIe(new IAX2IeCauseCode(IAX2IeCauseCode::BearerCapabilityNotAvail));
    TransmitFrameToRemoteEndpoint(reply);
    con->ClearCall(OpalConnection::EndedByCapabilityExchange);
    return FALSE;
  }
  
  return TRUE;
}

void IAX2Processor::ProcessIaxCmdNew(IAX2FullFrameProtocol *src)
{ /*That we are here indicates this connection is already in place */
  
  PTRACE(3, "ProcessIaxCmdNew(IAX2FullFrameProtocol *src)");
  remote.SetRemoteAddress(src->GetRemoteInfo().RemoteAddress());
  remote.SetRemotePort(src->GetRemoteInfo().RemotePort());

  con->OnSetUp();   //ONLY the  callee (person receiving call) does ownerCall.OnSetUp();

  IAX2FullFrameProtocol * reply;
  
  if (IsCallHappening()) {
    PTRACE(3, "Remote node has sent us a eecond new message. ignore");
    return;
  }
  
  if (!RemoteSelectedCodecOk()) {
    PTRACE(3, "Remote node sected a bad codec, hangup call ");
    reply = new  IAX2FullFrameProtocol(this, IAX2FullFrameProtocol::cmdInval, src, IAX2FullFrame::callIrrelevant);
    TransmitFrameToRemoteEndpoint(reply);
    
    reply= new IAX2FullFrameProtocol(this, IAX2FullFrameProtocol::cmdHangup, IAX2FullFrame::callIrrelevant);
    PTRACE(3, "Send a hangup frame to the remote endpoint as there is no codec available");		 
    reply->AppendIe(new IAX2IeCause("No matching codec"));
    SetCallTerminating();
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
  reply = new  IAX2FullFrameProtocol(this, IAX2FullFrameProtocol::cmdAccept);
  reply->AppendIe(new IAX2IeFormat(selectedCodec));
  TransmitFrameToRemoteEndpoint(reply);
  SetCallAccepted();
  
  /*We could send an AuthReq frame at this point */

  IAX2FullFrameSessionControl *r;
  r = new IAX2FullFrameSessionControl(this, IAX2FullFrameSessionControl::ringing);
  TransmitFrameToRemoteEndpoint(r, IAX2WaitingForAck::RingingAcked);
}

void IAX2Processor::ProcessIaxCmdPing(IAX2FullFrameProtocol *src)
{
  PTRACE(3, "ProcessIaxCmdPing(IAX2FullFrameProtocol *src)");
  IAX2FullFrameProtocol *f = new IAX2FullFrameProtocol(this, IAX2FullFrameProtocol::cmdPong, src, IAX2FullFrame::callIrrelevant);
  TransmitFrameToRemoteEndpoint(f);
}

void IAX2Processor::ProcessIaxCmdPong(IAX2FullFrameProtocol *src)
{
  SendAckFrame(src);  
  PTRACE(3, "ProcessIaxCmdPong(IAX2FullFrameProtocol *src)");
}

void IAX2Processor::ProcessIaxCmdAck(IAX2FullFrameProtocol *src)
{
  PTRACE(3, "ProcessIaxCmdAck(IAX2FullFrameProtocol * /*src*/)");
  /* The corresponding IAX2FullFrame has already been marked as acknowledged */
  
  
  if (!nextTask.MatchingAckPacket(src)) {
    PTRACE(3, "ack packet does not match a pending response");
    return;
  }
  
  IAX2WaitingForAck::ResponseToAck action = nextTask.GetResponse();
  nextTask.ZeroValues();
  switch(action) {
  case IAX2WaitingForAck::RingingAcked : 
    RingingWasAcked();
    break;
  case IAX2WaitingForAck::AcceptAcked  : 
    break;;
  case IAX2WaitingForAck::AuthRepAcked : 
    break;
  case IAX2WaitingForAck::AnswerAcked  : 
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
  IAX2FullFrameSessionControl * reply;
  reply = new IAX2FullFrameSessionControl(this, IAX2FullFrameSessionControl::answer);
  TransmitFrameToRemoteEndpoint(reply, IAX2WaitingForAck::AnswerAcked);
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
void IAX2Processor::ProcessIaxCmdHangup(IAX2FullFrameProtocol *src)
{ 
  SetCallTerminating(TRUE);

  PTRACE(3, "ProcessIaxCmdHangup(IAX2FullFrameProtocol *src)");
  SendAckFrame(src);

  PTRACE(1, "The remote node (" << con->GetRemotePartyName()  << ") has closed the call");

  con->EndCallNow(OpalConnection::EndedByRemoteUser);
}

void IAX2Processor::ProcessIaxCmdReject(IAX2FullFrameProtocol *src)
{
  PTRACE(3, "ProcessIaxCmdReject(IAX2FullFrameProtocol *src)");
  cout << "Remote endpoint has rejected our call " << endl;
  cout << "Cause \"" << ieData.cause << "\"" << endl;
  SendAckFrame(src);
  con->EndCallNow(OpalConnection::EndedByRefusal);
}

void IAX2Processor::ProcessIaxCmdAccept(IAX2FullFrameProtocol *src)
{
  con->OnAlerting();

  PTRACE(3, "ProcessIaxCmdAccept(IAX2FullFrameProtocol *src)");
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
  PString codecName = IAX2FullFrameVoice::GetOpalNameOfCodec((unsigned short)selectedCodec);
  PTRACE(3, "The remote endpoint has accepted our call on codec " << codecName);
  con->GetEndPoint().GetCodecLengths(selectedCodec, audioCompressedBytes, audioFrameDuration);
  PTRACE(3, "codec frame play duration is " << audioFrameDuration << " ms, which compressed to " 
	 << audioCompressedBytes << " bytes of data");
}

void IAX2Processor::ProcessIaxCmdAuthReq(IAX2FullFrameProtocol * /*src*/)
{
  PTRACE(3, "ProcessIaxCmdAuthReq(IAX2FullFrameProtocol *src)");
  StopNoResponseTimer();
  IAX2FullFrameProtocol *f = new IAX2FullFrameProtocol(this, IAX2FullFrameProtocol::cmdAuthRep);
  
  Authenticate(f);
  
  TransmitFrameToRemoteEndpoint(f);
  StartNoResponseTimer();
}

void IAX2Processor::ProcessIaxCmdAuthRep(IAX2FullFrameProtocol * /*src*/)
{
  PTRACE(3, "ProcessIaxCmdAuthRep(IAX2FullFrameProtocol *src)");
  /** When this packet has been acked, we send an accept */     
}

void IAX2Processor::ProcessIaxCmdInval(IAX2FullFrameProtocol * src)
{
  PTRACE(3, "ProcessIaxCmdInval(IAX2FullFrameProtocol *src) " << src->IdString());
  PTRACE(3, "ProcessIaxCmdInval(IAX2FullFrameProtocol *src) " << src->GetSequenceInfo().AsString());
  PTRACE(3, "ProcessIaxCmdInval(IAX2FullFrameProtocol *src) " << src->GetTimeStamp());

  if (src->GetSequenceInfo().IsSequenceNosZero() && (src->GetTimeStamp() == 0)) {
    PTRACE(3, "ProcessIaxCmdInval - remote end does not like us, and nuked the call");
    con->ClearCall(OpalConnection::EndedByRemoteUser);
  }
}

void IAX2Processor::ProcessIaxCmdLagRq(IAX2FullFrameProtocol *src)
{
  PTRACE(3, "ProcessIaxCmdLagRq(IAX2FullFrameProtocol *src)");
  IAX2FullFrameProtocol *f = new IAX2FullFrameProtocol(this, IAX2FullFrameProtocol::cmdLagRp, src, IAX2FullFrame::callIrrelevant);
  TransmitFrameToRemoteEndpoint(f);
}

void IAX2Processor::ProcessIaxCmdLagRp(IAX2FullFrameProtocol *src)
{
  PTRACE(3, "ProcessIaxCmdLagRp(IAX2FullFrameProtocol *src)");
  SendAckFrame(src);
  PTRACE(3, "Process\tRound trip lag time is " << (IAX2Frame::CalcTimeStamp(callStartTick) - src->GetTimeStamp()));
}

void IAX2Processor::ProcessIaxCmdRegReq(IAX2FullFrameProtocol *src)
{
  PTRACE(3, "ProcessIaxCmdRegReq(IAX2FullFrameProtocol *src)");
  SendAckFrame(src);
}

void IAX2Processor::ProcessIaxCmdRegAuth(IAX2FullFrameProtocol * /*src*/) /* The regauth contains the challenge, which we have to solve*/
{                                                                  /* We put our reply into the cmdRegReq frame */
  PTRACE(3, "ProcessIaxCmdRegAuth(IAX2FullFrameProtocol *src)");
  
  IAX2FullFrameProtocol *f = new IAX2FullFrameProtocol(this, IAX2FullFrameProtocol::cmdRegReq);
  
  Authenticate(f);
  
  TransmitFrameToRemoteEndpoint(f); 
  StartNoResponseTimer();
  
}

void IAX2Processor::ProcessIaxCmdRegAck(IAX2FullFrameProtocol *src)
{
  PTRACE(3, "ProcessIaxCmdRegAck(IAX2FullFrameProtocol *src)");
  StopNoResponseTimer();
  SendAckFrame(src);
}

void IAX2Processor::ProcessIaxCmdRegRej(IAX2FullFrameProtocol * /*src*/)
{
  PTRACE(3, "ProcessIaxCmdRegRej(IAX2FullFrameProtocol *src)");
}

void IAX2Processor::ProcessIaxCmdRegRel(IAX2FullFrameProtocol * /*src*/)
{
  PTRACE(3, "ProcessIaxCmdRegRel(IAX2FullFrameProtocol *src)");
}

void IAX2Processor::ProcessIaxCmdVnak(IAX2FullFrameProtocol * /*src*/)
{
  PTRACE(3, "ProcessIaxCmdVnak(IAX2FullFrameProtocol *src)");
}

void IAX2Processor::ProcessIaxCmdDpReq(IAX2FullFrameProtocol * /*src*/)
{
  PTRACE(3, "ProcessIaxCmdDpReq(IAX2FullFrameProtocol *src)");
}

void IAX2Processor::ProcessIaxCmdDpRep(IAX2FullFrameProtocol * /*src*/)
{
  PTRACE(3, "ProcessIaxCmdDpRep(IAX2FullFrameProtocol *src)");
}

void IAX2Processor::ProcessIaxCmdDial(IAX2FullFrameProtocol * /*src*/)
{
  PTRACE(3, "ProcessIaxCmdDial(IAX2FullFrameProtocol *src)");
}

void IAX2Processor::ProcessIaxCmdTxreq(IAX2FullFrameProtocol * /*src*/)
{
  PTRACE(3, "ProcessIaxCmdTxreq(IAX2FullFrameProtocol */*src*/)");
  //Not implemented.
}

void IAX2Processor::ProcessIaxCmdTxcnt(IAX2FullFrameProtocol * /*src*/)
{
  PTRACE(3, "ProcessIaxCmdTxcnt(IAX2FullFrameProtocol * /*src*/)");
  //Not implemented.
}

void IAX2Processor::ProcessIaxCmdTxacc(IAX2FullFrameProtocol * /*src*/)
{ /*Transfer has been accepted */
  PTRACE(3, "ProcessIaxCmdTxacc(IAX2FullFrameProtocol * /*src*/)");
  //Not implemented.
}

void IAX2Processor::ProcessIaxCmdTxready(IAX2FullFrameProtocol * /*src*/)
{
  PTRACE(3, "ProcessIaxCmdTxready(IAX2FullFrameProtocol */*src*/)");
  //Not implemented.
}

void IAX2Processor::ProcessIaxCmdTxrel(IAX2FullFrameProtocol * /*src*/)
{
  PTRACE(3, "ProcessIaxCmdTxrel(IAX2FullFrameProtocol */*src*/)");
  //Not implemented.
}

void IAX2Processor::ProcessIaxCmdTxrej(IAX2FullFrameProtocol * /*src*/)
{
  PTRACE(3, "ProcessIaxCmdTxrej(IAX2FullFrameProtocol */*src*/)");
  //Not implemented. (transfer rejected)
}

void IAX2Processor::ProcessIaxCmdQuelch(IAX2FullFrameProtocol * /*src*/)
{
  cerr << "Quelch message received" << endl;
  PTRACE(3, "ProcessIaxCmdQuelch(IAX2FullFrameProtocol */*src*/)");
  audioCanFlow = FALSE;
}

void IAX2Processor::ProcessIaxCmdUnquelch(IAX2FullFrameProtocol * /*src*/)
{
  PTRACE(3, "ProcessIaxCmdUnquelch(IAX2FullFrameProtocol */*src*/)");
  audioCanFlow = TRUE;
}

void IAX2Processor::ProcessIaxCmdPoke(IAX2FullFrameProtocol * /*src*/)
{
  PTRACE(3, "ProcessIaxCmdPoke(IAX2FullFrameProtocol *src)");
}

void IAX2Processor::ProcessIaxCmdPage(IAX2FullFrameProtocol * /*src*/)
{
  PTRACE(3, "ProcessIaxCmdPage(IAX2FullFrameProtocol *src)");
}

void IAX2Processor::ProcessIaxCmdMwi(IAX2FullFrameProtocol * /*src*/)
{
  PTRACE(3, "ProcessIaxCmdMwi(IAX2FullFrameProtocol *src)");
}

void IAX2Processor::ProcessIaxCmdUnsupport(IAX2FullFrameProtocol * /*src*/)
{
  PTRACE(3, "ProcessIaxCmdUnsupport(IAX2FullFrameProtocol *src)");
}

void IAX2Processor::ProcessIaxCmdTransfer(IAX2FullFrameProtocol * /*src*/)
{
  PTRACE(3, "ProcessIaxCmdTransfer(IAX2FullFrameProtocol *src)");
}

void IAX2Processor::ProcessIaxCmdProvision(IAX2FullFrameProtocol * /*src*/)
{
  PTRACE(3, "ProcessIaxCmdProvision(IAX2FullFrameProtocol *src)");
}

void IAX2Processor::ProcessIaxCmdFwDownl(IAX2FullFrameProtocol * /*src*/)
{
  PTRACE(3, "ProcessIaxCmdFwDownl(IAX2FullFrameProtocol *src)");
}

void IAX2Processor::ProcessIaxCmdFwData(IAX2FullFrameProtocol * /*src*/)
{
  PTRACE(3, "ProcessIaxCmdFwData(IAX2FullFrameProtocol *src)");
}

BOOL IAX2Processor::Authenticate(IAX2FullFrameProtocol *reply)
{
  BOOL processed = FALSE;
  IAX2IeAuthMethods ie(ieData.authMethods);
  
  if (ie.IsRsaAuthentication()) {
    PTRACE(3, "DO NOT handle RSA authentication ");
    reply->SetSubClass(IAX2FullFrameProtocol::cmdInval);
    processed = TRUE;
  }
  
  if (ie.IsMd5Authentication()) {
    PTRACE(3, "Processor\tMD5 Authentiction yes, make reply up");
    IAX2IeMd5Result *res = new IAX2IeMd5Result(ieData.challenge, con->GetEndPoint().GetPassword());
    reply->AppendIe(res);
    processed = TRUE;
    encryption.SetChallengeKey(ieData.challenge);
    encryption.SetEncryptionKey(con->GetEndPoint().GetPassword());
  }
  
  if (ie.IsPlainTextAuthentication() && (!processed)) {
    reply->AppendIe(new IAX2IePassword(con->GetEndPoint().GetPassword()));
    processed = TRUE;
  }
  
  if (ieData.encryptionMethods == IAX2IeEncryption::encryptAes128) {
    PTRACE(3, "Processor\tEnable AES 128 encryption");
    encryption.SetEncryptionOn();
    reply->AppendIe(new IAX2IeEncryption);
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


void IAX2Processor::IncomingEthernetFrame(IAX2Frame *frame)
{
  frameList.AddNewFrame(frame);
  
  CleanPendingLists();
} 

IAX2Frame * IAX2Processor::GetSoundPacketFromNetwork()
{
  IAX2Frame * newSound = soundReadFromEthernet.GetLastFrame();
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
  if (IsCallTerminating())
    return;

  IAX2FullFrame *p = new IAX2FullFrameProtocol(this, IAX2FullFrameProtocol::cmdPing);
  TransmitFrameToRemoteEndpoint(p);

  p = new IAX2FullFrameProtocol(this, IAX2FullFrameProtocol::cmdLagRq);
  TransmitFrameToRemoteEndpoint(p);
}
