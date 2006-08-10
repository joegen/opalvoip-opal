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
 * $Log: callprocessor.cxx,v $
 * Revision 1.2  2006/08/10 04:44:03  csoutheren
 * Added new IAX2 files to VC 2005 project
 * Fixed compile warning on Windows
 *
 * Revision 1.1  2006/08/09 03:46:39  dereksmithies
 * Add ability to register to a remote Asterisk box. The iaxProcessor class is split
 * into a callProcessor and a regProcessor class.
 * Big thanks to Stephen Cook, (sitiveni@gmail.com) for this work.
 *
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
#include <iax2/callprocessor.h>
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

IAX2CallProcessor::IAX2CallProcessor(IAX2EndPoint &ep)
  : IAX2Processor(ep)
{
  callStatus = 0;
  
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
  
  holdCall = FALSE;
  holdReleaseCall = FALSE;
  
  statusCheckTimer.SetNotifier(PCREATE_NOTIFIER(OnStatusCheck));
  statusCheckOtherEnd = FALSE;
  
  soundBufferState = BufferToSmall;
}

IAX2CallProcessor::~IAX2CallProcessor()
{
}

void IAX2CallProcessor::AssignConnection(IAX2Connection * _con)
{
  con = _con;

  remote.SetSourceCallNumber(con->GetEndPoint().NextSrcCallNumber(this));
  
  Resume();
}

void IAX2CallProcessor::PrintOn(ostream & strm) const
{
  strm << "In call with " << con->GetRemotePartyAddress() << "  " << remotePhoneNumber << " " << callToken  << endl
       << "  Call has been up for " << setprecision(0) << setw(8)
       << (PTimer::Tick() - callStartTick) << " milliseconds" << endl
       << "  Control frames sent " << controlFramesSent    << endl
       << "  Control frames rcvd " << controlFramesRcvd    << endl
       << "  Audio frames sent   " << audioFramesSent      << endl
       << "  Audio frames rcvd   " << audioFramesRcvd      << endl
       << "  Video frames sent   " << videoFramesSent      << endl
       << "  Video frames rcvd   " << videoFramesRcvd      << endl;
}

void IAX2CallProcessor::Release(OpalConnection::CallEndReason reason)
{
  PTRACE(3, "Processor\tRelease(" << reason << ")");
  PStringStream str;
  str << reason;
  Hangup(str);
}

void IAX2CallProcessor::ClearCall(OpalConnection::CallEndReason reason)
{
  statusCheckTimer.Stop();
  PTRACE(3, "ListProcesser runs     =====ClearCall(" << reason << ")");
  
  PStringStream str;
  str << reason;
  Hangup(str);

  con->EndCallNow(reason);
}

void IAX2CallProcessor::OnReleased()
{
  PTRACE(3, "OnReleased method in processor has run");
  Terminate();
}

void IAX2CallProcessor::PutSoundPacketToNetwork(PBYTEArray *sound)
{
  /*This thread does not send the audio frame. 
    The IAX2CallProcessor thread sends the audio frame */
  soundWaitingForTransmission.AddNewEntry(sound);
  
  CleanPendingLists();
} 

void IAX2CallProcessor::CallStopSounds()
{
  cout << "We have received a call stop sounds packet " << endl;
}

void IAX2CallProcessor::ReceivedHookFlash() 
{ 
} 

void IAX2CallProcessor::RemoteNodeIsBusy() 
{ 
} 

void IAX2CallProcessor::RemoteNodeHasAnswered()
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

void IAX2CallProcessor::Hangup(PString dieMessage)
{
  PTRACE(3, "Hangup request " << dieMessage);
  hangList.AppendString(dieMessage);   //send this text to remote endpoint 
  
  activate.Signal();
}

void IAX2CallProcessor::CheckForHangupMessages()
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

void IAX2CallProcessor::ConnectToRemoteNode(PString & newRemoteNode)
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

  if (!res[IAX2EndPoint::userIndex].IsEmpty()) {
    f->AppendIe(new IAX2IeCallingName(res[IAX2EndPoint::userIndex]));
    f->AppendIe(new IAX2IeUserName(res[IAX2EndPoint::userIndex]));
  } else if (!GetUserName().IsEmpty()) {
    f->AppendIe(new IAX2IeCallingName(GetUserName()));
    f->AppendIe(new IAX2IeUserName(GetUserName()));
  }

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

void IAX2CallProcessor::ReportStatistics()
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

void IAX2CallProcessor::ProcessFullFrame(IAX2FullFrame & fullFrame)
{
  switch(fullFrame.GetFrameType()) {
  case IAX2FullFrame::dtmfType:        
    PTRACE(3, "Build matching full frame    dtmfType");
    ProcessNetworkFrame(new IAX2FullFrameDtmf(fullFrame));
    break;
  case IAX2FullFrame::voiceType:       
    PTRACE(3, "Build matching full frame    voiceType");
    ProcessNetworkFrame(new IAX2FullFrameVoice(fullFrame));
    break;
  case IAX2FullFrame::videoType:       
    PTRACE(3, "Build matching full frame    videoType");
    ProcessNetworkFrame(new IAX2FullFrameVideo(fullFrame));
    break;
  case IAX2FullFrame::controlType:     
    PTRACE(3, "Build matching full frame    controlType");
    ProcessNetworkFrame(new IAX2FullFrameSessionControl(fullFrame));
    break;
  case IAX2FullFrame::nullType:        
    PTRACE(3, "Build matching full frame    nullType");
    ProcessNetworkFrame(new IAX2FullFrameNull(fullFrame));
    break;
  case IAX2FullFrame::iax2ProtocolType: 
    PTRACE(3, "Build matching full frame    iax2ProtocolType");
    ProcessNetworkFrame(new IAX2FullFrameProtocol(fullFrame));
    break;
  case IAX2FullFrame::textType:        
    PTRACE(3, "Build matching full frame    textType");
    ProcessNetworkFrame(new IAX2FullFrameText(fullFrame));
    break;
  case IAX2FullFrame::imageType:       
    PTRACE(3, "Build matching full frame    imageType");
    ProcessNetworkFrame(new IAX2FullFrameImage(fullFrame));
    break;
  case IAX2FullFrame::htmlType:        
    PTRACE(3, "Build matching full frame    htmlType");
    ProcessNetworkFrame(new IAX2FullFrameHtml(fullFrame));
    break;
  case IAX2FullFrame::cngType:        
    PTRACE(3, "Build matching full frame    cngType");
    ProcessNetworkFrame(new IAX2FullFrameCng(fullFrame));
    break;
  default: 
    PTRACE(3, "Build matching full frame, Type not understood");
  };
  
}

void IAX2CallProcessor::ProcessLists()
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
    
  if (holdCall)
    SendQuelchMessage();
  
  if (holdReleaseCall)
    SendUnQuelchMessage();

  if (statusCheckOtherEnd)
    DoStatusCheck();

  CheckForHangupMessages();
  //cout << PThread::Current()->GetThreadName() << " ProcessLists3(cp) "<< PTimer::Tick() << endl;
}

void IAX2CallProcessor::ProcessIncomingAudioFrame(IAX2Frame *newFrame)
{
  PTRACE(3, "Processor\tPlace audio frame on queue " << newFrame->IdString());
  //cout << PThread::Current()->GetThreadName() << " ProcessIncomingAudioFrame(cp) "<< PTimer::Tick() << endl;
  IncAudioFramesRcvd();
  
  PWaitAndSignal m(soundReadFromEthernetMutex);
  soundReadFromEthernet.AddNewFrame(newFrame);
  PTRACE(3, "have " << soundReadFromEthernet.GetSize() << " pending packets on incoming sound queue");
}

void IAX2CallProcessor::ProcessIncomingVideoFrame(IAX2Frame *newFrame)
{
  PTRACE(3, "Incoming video frame ignored, cause we don't handle it");
  IncVideoFramesRcvd();
  delete newFrame;
}

void IAX2CallProcessor::SendSoundMessage(PBYTEArray *sound)
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

void IAX2CallProcessor::SendDtmf(PString dtmfs)
{
  dtmfText += dtmfs;
}

void IAX2CallProcessor::SendText(const PString & text)
{
  textList.AppendString(text);
}

void IAX2CallProcessor::SendHold()
{
  holdCall = TRUE;
}

void IAX2CallProcessor::SendHoldRelease()
{
  holdReleaseCall = TRUE;
}

void IAX2CallProcessor::SendDtmfMessage(char  message)
{
  IAX2FullFrameDtmf *f = new IAX2FullFrameDtmf(this, message);
  TransmitFrameToRemoteEndpoint(f);
}

void IAX2CallProcessor::SendTextMessage(PString & message)
{
  IAX2FullFrameText *f = new IAX2FullFrameText(this, message);
  TransmitFrameToRemoteEndpoint(f);
}

void IAX2CallProcessor::SendQuelchMessage()
{
  holdCall = FALSE;
  
  IAX2FullFrameProtocol *f = new IAX2FullFrameProtocol(this, IAX2FullFrameProtocol::cmdQuelch);
  //we'll request that music is played to them because everyone likes music when they're on hold ;)
  f->AppendIe(new IAX2IeMusicOnHold());
  TransmitFrameToRemoteEndpoint(f);
}

void IAX2CallProcessor::SendUnQuelchMessage()
{
  holdReleaseCall = FALSE;
  
  IAX2FullFrameProtocol *f = new IAX2FullFrameProtocol(this, IAX2FullFrameProtocol::cmdUnquelch); 
  TransmitFrameToRemoteEndpoint(f);
}

void IAX2CallProcessor::ProcessNetworkFrame(IAX2MiniFrame * src)
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

void IAX2CallProcessor::ProcessNetworkFrame(IAX2FullFrame * src)
{
  PTRACE(3, "ProcessNetworkFrame(IAX2FullFrame * src)");
  PStringStream message;
  message << PString("Do not know how to process networks packets of \"Full Frame\" type ") << *src;
  PAssertAlways(message);
  return;
}

void IAX2CallProcessor::ProcessNetworkFrame(IAX2Frame * src)
{
  PTRACE(3, "ProcessNetworkFrame(IAX2Frame * src)");
  PStringStream message;
  message << PString("Do not know how to process networks packets of \"Frame\" type ") << *src;
  PTRACE(3, message);
  PTRACE(3, message);
  PAssertAlways(message);
  return;
}

void IAX2CallProcessor::ProcessNetworkFrame(IAX2FullFrameDtmf * src)
{
  PTRACE(3, "ProcessNetworkFrame(IAX2FullFrameDtmf * src)");
  SendAckFrame(src);
  con->OnUserInputTone((char)src->GetSubClass(), 1);

  delete src;
}

void IAX2CallProcessor::ProcessNetworkFrame(IAX2FullFrameVoice * src)
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

void IAX2CallProcessor::ProcessNetworkFrame(IAX2FullFrameVideo * src)
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

void IAX2CallProcessor::ProcessNetworkFrame(IAX2FullFrameSessionControl * src)
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
    con->RemoteHoldConnection();
    break;
    
  case IAX2FullFrameSessionControl::callHoldRelease: // Call is no longer on hold
    con->RemoteRetrieveConnection();
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

BOOL IAX2CallProcessor::SetUpConnection()
{
  PTRACE(2, "IAX\tSet Up Connection to remote node " << con->GetRemotePartyAddress());
   
  callList.AppendString(con->GetRemotePartyAddress());
  
  CleanPendingLists();
  return TRUE;
}

void IAX2CallProcessor::ProcessNetworkFrame(IAX2FullFrameNull * src)
{
  PTRACE(3, "ProcessNetworkFrame(IAX2FullFrameNull * src)");
  delete src;
  return;
}

void IAX2CallProcessor::ProcessNetworkFrame(IAX2FullFrameProtocol * src)
{ /* these frames are labelled as AST_FRAME_IAX in the asterisk souces.
     These frames contain Information Elements in the data field.*/
  
  PTRACE(3, "ProcessNetworkFrame(IAX2FullFrameProtocol * src)");

  CheckForRemoteCapabilities(src);

  src->CopyDataFromIeListTo(ieData);
  
  //check if the common method can process it?
  if (ProcessCommonNetworkFrame(src))
    return;

  switch(src->GetSubClass()) {
  case IAX2FullFrameProtocol::cmdNew:
    ProcessIaxCmdNew(src);
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

void IAX2CallProcessor::ProcessNetworkFrame(IAX2FullFrameText * src)
{
  PTRACE(3, "ProcessNetworkFrame(IAX2FullFrameText * src)");
  delete src;
  return;
}

void IAX2CallProcessor::ProcessNetworkFrame(IAX2FullFrameImage * src)
{
  PTRACE(3, "ProcessNetworkFrame(IAX2FullFrameImage * src)");
  delete src;
  return;
}

void IAX2CallProcessor::ProcessNetworkFrame(IAX2FullFrameHtml * src)
{
  PTRACE(3, "ProcessNetworkFrame(IAX2FullFrameHtml * src)");
  delete src;
  return;
}

void IAX2CallProcessor::ProcessNetworkFrame(IAX2FullFrameCng * src)
{
  PTRACE(3, "ProcessNetworkFrame(IAX2FullFrameCng * src)");
  delete src;
  return;
}

void IAX2CallProcessor::CheckForRemoteCapabilities(IAX2FullFrameProtocol *src)
{
  unsigned int remoteCapability, format;
  
  src->GetRemoteCapability(remoteCapability, format);

  PTRACE(3, "Connection\t Remote capabilities are " << remoteCapability << "   codec preferred " << format);
  if ((remoteCapability == 0) && (format == 0))
    return;

  con->BuildRemoteCapabilityTable(remoteCapability, format);
}

BOOL IAX2CallProcessor::RemoteSelectedCodecOk()
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

void IAX2CallProcessor::ProcessIaxCmdNew(IAX2FullFrameProtocol *src)
{ /*That we are here indicates this connection is already in place */
  
  PTRACE(3, "ProcessIaxCmdNew(IAX2FullFrameProtocol *src)");
  remote.SetRemoteAddress(src->GetRemoteInfo().RemoteAddress());
  remote.SetRemotePort(src->GetRemoteInfo().RemotePort());

  con->OnSetUp();   //ONLY the  callee (person receiving call) does ownerCall.OnSetUp();

  IAX2FullFrameProtocol * reply;
  
  if (IsCallHappening()) {
    PTRACE(3, "Remote node has sent us a second new message. ignore");
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

void IAX2CallProcessor::ProcessIaxCmdAck(IAX2FullFrameProtocol *src)
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

void IAX2CallProcessor::RingingWasAcked()
{
  PTRACE(3, "Processor\t Remote node " << con->GetRemotePartyAddress() << " knows our phone is ringing");
}

void IAX2CallProcessor::AnswerWasAcked()
{
  PTRACE(3, "Answer was acked");
}

void IAX2CallProcessor::AcceptIncomingCall()
{
  PTRACE(3, "AcceptIncomingCall()");

}

void IAX2CallProcessor::SetEstablished(BOOL PTRACE_PARAM(originator))
{
  PTRACE(3, "Processor\tStatusCheck timer set to 10 seconds");
  StartStatusCheckTimer();

  PTRACE(3, "Processor\tOnEstablished,   Originator = " << originator);
  return;
}

void IAX2CallProcessor::SendAnswerMessageToRemoteNode()
{
  answerCallNow = FALSE;
  StopNoResponseTimer();
  PTRACE(3, "Processor\tSend Answer message");
  IAX2FullFrameSessionControl * reply;
  reply = new IAX2FullFrameSessionControl(this, IAX2FullFrameSessionControl::answer);
  TransmitFrameToRemoteEndpoint(reply, IAX2WaitingForAck::AnswerAcked);
}

void IAX2CallProcessor::SetConnected()
{
  PTRACE(3, "SetConnected");

  answerCallNow = TRUE;
  CleanPendingLists();
}  

BOOL IAX2CallProcessor::SetAlerting(const PString & PTRACE_PARAM(calleeName), BOOL /*withMedia*/)
{
  PTRACE(3, "Processor\tSetAlerting from " << calleeName);
  return TRUE;
}

/*The remote node has told us to hangup and go away */
void IAX2CallProcessor::ProcessIaxCmdHangup(IAX2FullFrameProtocol *src)
{ 
  SetCallTerminating(TRUE);

  PTRACE(3, "ProcessIaxCmdHangup(IAX2FullFrameProtocol *src)");
  SendAckFrame(src);

  PTRACE(1, "The remote node (" << con->GetRemotePartyAddress()  << ") has closed the call");

  con->EndCallNow(OpalConnection::EndedByRemoteUser);
}

void IAX2CallProcessor::ProcessIaxCmdReject(IAX2FullFrameProtocol *src)
{
  PTRACE(3, "ProcessIaxCmdReject(IAX2FullFrameProtocol *src)");
  cout << "Remote endpoint has rejected our call " << endl;
  cout << "Cause \"" << ieData.cause << "\"" << endl;
  SendAckFrame(src);
  con->EndCallNow(OpalConnection::EndedByRefusal);
}

void IAX2CallProcessor::ProcessIaxCmdAccept(IAX2FullFrameProtocol *src)
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

void IAX2CallProcessor::ProcessIaxCmdAuthReq(IAX2FullFrameProtocol * /*src*/)
{
  PTRACE(3, "ProcessIaxCmdAuthReq(IAX2FullFrameProtocol *src)");
  StopNoResponseTimer();
  IAX2FullFrameProtocol *f = new IAX2FullFrameProtocol(this, IAX2FullFrameProtocol::cmdAuthRep);
  
  if (!GetPassword().IsEmpty())
    Authenticate(f, password);
  else
    Authenticate(f, endpoint.GetPassword());
  
  TransmitFrameToRemoteEndpoint(f);
  StartNoResponseTimer();
}

void IAX2CallProcessor::ProcessIaxCmdAuthRep(IAX2FullFrameProtocol * /*src*/)
{
  PTRACE(3, "ProcessIaxCmdAuthRep(IAX2FullFrameProtocol *src)");
  /** When this packet has been acked, we send an accept */     
}

void IAX2CallProcessor::ProcessIaxCmdInval(IAX2FullFrameProtocol * src)
{
  PTRACE(3, "ProcessIaxCmdInval(IAX2FullFrameProtocol *src) " << src->IdString());
  PTRACE(3, "ProcessIaxCmdInval(IAX2FullFrameProtocol *src) " << src->GetSequenceInfo().AsString());
  PTRACE(3, "ProcessIaxCmdInval(IAX2FullFrameProtocol *src) " << src->GetTimeStamp());

  if (src->GetSequenceInfo().IsSequenceNosZero() && (src->GetTimeStamp() == 0)) {
    PTRACE(3, "ProcessIaxCmdInval - remote end does not like us, and nuked the call");
    con->ClearCall(OpalConnection::EndedByRemoteUser);
  }
}

void IAX2CallProcessor::ProcessIaxCmdDpReq(IAX2FullFrameProtocol * /*src*/)
{
  PTRACE(3, "ProcessIaxCmdDpReq(IAX2FullFrameProtocol *src)");
}

void IAX2CallProcessor::ProcessIaxCmdDpRep(IAX2FullFrameProtocol * /*src*/)
{
  PTRACE(3, "ProcessIaxCmdDpRep(IAX2FullFrameProtocol *src)");
}

void IAX2CallProcessor::ProcessIaxCmdDial(IAX2FullFrameProtocol * /*src*/)
{
  PTRACE(3, "ProcessIaxCmdDial(IAX2FullFrameProtocol *src)");
}

void IAX2CallProcessor::ProcessIaxCmdTxreq(IAX2FullFrameProtocol * /*src*/)
{
  PTRACE(3, "ProcessIaxCmdTxreq(IAX2FullFrameProtocol */*src*/)");
  //Not implemented.
}

void IAX2CallProcessor::ProcessIaxCmdTxcnt(IAX2FullFrameProtocol * /*src*/)
{
  PTRACE(3, "ProcessIaxCmdTxcnt(IAX2FullFrameProtocol * /*src*/)");
  //Not implemented.
}

void IAX2CallProcessor::ProcessIaxCmdTxacc(IAX2FullFrameProtocol * /*src*/)
{ /*Transfer has been accepted */
  PTRACE(3, "ProcessIaxCmdTxacc(IAX2FullFrameProtocol * /*src*/)");
  //Not implemented.
}

void IAX2CallProcessor::ProcessIaxCmdTxready(IAX2FullFrameProtocol * /*src*/)
{
  PTRACE(3, "ProcessIaxCmdTxready(IAX2FullFrameProtocol */*src*/)");
  //Not implemented.
}

void IAX2CallProcessor::ProcessIaxCmdTxrel(IAX2FullFrameProtocol * /*src*/)
{
  PTRACE(3, "ProcessIaxCmdTxrel(IAX2FullFrameProtocol */*src*/)");
  //Not implemented.
}

void IAX2CallProcessor::ProcessIaxCmdTxrej(IAX2FullFrameProtocol * /*src*/)
{
  PTRACE(3, "ProcessIaxCmdTxrej(IAX2FullFrameProtocol */*src*/)");
  //Not implemented. (transfer rejected)
}

void IAX2CallProcessor::ProcessIaxCmdQuelch(IAX2FullFrameProtocol * /*src*/)
{
  PTRACE(3, "ProcessIaxCmdQuelch(IAX2FullFrameProtocol */*src*/)");
}

void IAX2CallProcessor::ProcessIaxCmdUnquelch(IAX2FullFrameProtocol * /*src*/)
{
  PTRACE(3, "ProcessIaxCmdUnquelch(IAX2FullFrameProtocol */*src*/)");
}

void IAX2CallProcessor::ProcessIaxCmdPoke(IAX2FullFrameProtocol * /*src*/)
{
  PTRACE(3, "ProcessIaxCmdPoke(IAX2FullFrameProtocol *src)");
}

void IAX2CallProcessor::ProcessIaxCmdPage(IAX2FullFrameProtocol * /*src*/)
{
  PTRACE(3, "ProcessIaxCmdPage(IAX2FullFrameProtocol *src)");
}

void IAX2CallProcessor::ProcessIaxCmdMwi(IAX2FullFrameProtocol * /*src*/)
{
  PTRACE(3, "ProcessIaxCmdMwi(IAX2FullFrameProtocol *src)");
}

void IAX2CallProcessor::ProcessIaxCmdUnsupport(IAX2FullFrameProtocol * /*src*/)
{
  PTRACE(3, "ProcessIaxCmdUnsupport(IAX2FullFrameProtocol *src)");
}

void IAX2CallProcessor::ProcessIaxCmdTransfer(IAX2FullFrameProtocol * /*src*/)
{
  PTRACE(3, "ProcessIaxCmdTransfer(IAX2FullFrameProtocol *src)");
}

void IAX2CallProcessor::ProcessIaxCmdProvision(IAX2FullFrameProtocol * /*src*/)
{
  PTRACE(3, "ProcessIaxCmdProvision(IAX2FullFrameProtocol *src)");
}

void IAX2CallProcessor::ProcessIaxCmdFwDownl(IAX2FullFrameProtocol * /*src*/)
{
  PTRACE(3, "ProcessIaxCmdFwDownl(IAX2FullFrameProtocol *src)");
}

void IAX2CallProcessor::ProcessIaxCmdFwData(IAX2FullFrameProtocol * /*src*/)
{
  PTRACE(3, "ProcessIaxCmdFwData(IAX2FullFrameProtocol *src)");
}

void IAX2CallProcessor::StartStatusCheckTimer(PINDEX msToWait)
{
  PTRACE(3, "Processor\tStatusCheck time. Now set flag to  send a ping and a lagrq packet");

  statusCheckTimer = PTimeInterval(msToWait); 
  statusCheckOtherEnd = TRUE;
  CleanPendingLists();
}

void IAX2CallProcessor::OnStatusCheck(PTimer &, INT)
{
  StartStatusCheckTimer();
}

void IAX2CallProcessor::RemoteNodeIsRinging()
{
  StopNoResponseTimer();
  
  StartNoResponseTimer(60 * 1000); /* The remote end has one minute to answer their phone */
}

IAX2Frame * IAX2CallProcessor::GetSoundPacketFromNetwork(DWORD minBuffer, DWORD maxBuffer)
{
  //If a "freak" event occurs then we'll try and counter it
  //so it does not occur soon
  switch (soundBufferState) {
    case BufferToSmall:
      minBuffer = (minBuffer / 2) + (maxBuffer / 2);
      break;
    case BufferToBig:
      maxBuffer = (minBuffer / 2) + (maxBuffer / 2);
      break;
    case Normal:
      break;
  }  
  
  PWaitAndSignal m(soundReadFromEthernetMutex);
  
  if (soundReadFromEthernet.GetSize() == 0)
    return NULL;
  
  //get the most recent sound packet
  IAX2Frame * mostRecentSound = (IAX2Frame*)soundReadFromEthernet.GetAt(soundReadFromEthernet.GetSize() - 1);
  DWORD mostRecentTimeStamp = mostRecentSound->GetTimeStamp();
  
  for (;;) {
    //get the oldest packet
    IAX2Frame * newSound = soundReadFromEthernet.GetLastFrame();        
    DWORD checkTimeStamp = newSound->GetTimeStamp();
    
    if (mostRecentTimeStamp < checkTimeStamp) {
      PTRACE(1, "Iax2MediaStream\tSound packet out of order");
      delete newSound;
      continue;
    }
    
    //check if the buffer is big enough
    if (mostRecentTimeStamp - checkTimeStamp < minBuffer) {
      PTRACE(3, "Iax2MediaStream\tSound buffer is not big enough, timestamp" << (mostRecentTimeStamp - checkTimeStamp));
      soundBufferState = BufferToSmall;
      soundReadFromEthernet.InsertAt(0, newSound);
      return NULL;
    }    
    
    //check if the buffer is to big
    if (mostRecentTimeStamp - checkTimeStamp > maxBuffer) {
      PTRACE(3, "Iax2MediaStream\tSound packet is to old " << newSound->IdString());
      soundBufferState = BufferToBig;
      delete newSound;
      continue;
    }
    
    //we found a good packet
    soundBufferState = Normal;
    PTRACE(3, "Iax2MediaStream\tSend frame to media stream " << newSound->IdString());
    return newSound;
  }
} 

void IAX2CallProcessor::DoStatusCheck()
{
  statusCheckOtherEnd = FALSE;
  if (IsCallTerminating())
    return;

  IAX2FullFrame *p = new IAX2FullFrameProtocol(this, IAX2FullFrameProtocol::cmdPing);
  TransmitFrameToRemoteEndpoint(p);

  p = new IAX2FullFrameProtocol(this, IAX2FullFrameProtocol::cmdLagRq);
  TransmitFrameToRemoteEndpoint(p);
}

void IAX2CallProcessor::OnNoResponseTimeout()
{
  PTRACE(3, "hangup now, as we have had no response from the remote node in the specified time ");
  cout << "no answer in specified time period. End this call " << endl;
  
  con->ClearCall(OpalConnection::EndedByNoAnswer);
}

PString IAX2CallProcessor::GetUserName() const
{
  //this returns the most important username that is set.
  //ie: Username > endpointUsername.
  if (!userName.IsEmpty())
    return userName;
  else if (!endpoint.GetLocalUserName().IsEmpty())
    return endpoint.GetLocalUserName();
  else
    return "";
}
