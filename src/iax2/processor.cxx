#include <ptlib.h>
#include <typeinfo>

#ifdef P_USE_PRAGMA
#pragma implementation "processor.h"
#endif

#include <iax2/processor.h>
#include <iax2/causecode.h>
#include <iax2/frame.h>
#include <iax2/iax2con.h>
#include <iax2/iax2ep.h>
#include <iax2/iax2medstrm.h>
#include <iax2/ies.h>
#include <iax2/sound.h>
#include <iax2/transmit.h>

#define new PNEW

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
  case RingingAcked :  return PString("Received acknowledgement of a Ringing message");
  case AcceptAcked  :  return PString("Received acknowledgement of a Accept message");
  case AuthRepAcked :  return PString("Received acknowledgement of a AuthRep message");
  case AnswerAcked  :  return PString("Received acknowledgement of a Answer message");
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
  
  frameList.Initialise();
  
  remote.SetDestCallNumber(0);
  remote.SetRemoteAddress(0);
  remote.SetRemotePort(endpoint.ListenPortNumber());
  
  nextTask.ZeroValues();
  noResponseTimer.SetNotifier(PCREATE_NOTIFIER(OnNoResponseTimeoutStart));
  
  specialPackets = FALSE;
}

IAX2Processor::~IAX2Processor()
{
  PTRACE(3, "IAX2CallProcessor DESTRUCTOR");

  StopNoResponseTimer();
  
  Terminate();
  WaitForTermination(10000);

  frameList.AllowDeleteObjects();
}

void IAX2Processor::SetCallToken(PString newToken) 
{
  callToken = newToken;
} 

PString IAX2Processor::GetCallToken()
{ 
  return callToken;
}

void IAX2Processor::Main()
{
  PTRACE(1, "Start of iax processing");
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

void IAX2Processor::IncomingEthernetFrame(IAX2Frame *frame)
{
  if (endThread) {
    PTRACE(3, "IAX2Con\t***** incoming frame during termination " << frame->IdString());
    // snuck in here during termination. may be an ack for hangup or other re-transmitted frames
    IAX2Frame *af = frame->BuildAppropriateFrameType(GetEncryptionInfo());
    if (af != NULL) {
      endpoint.transmitter->PurgeMatchingFullFrames(af);
      delete af;
    }
  } else {
    frameList.AddNewFrame(frame);  
    CleanPendingLists();    
  } 
} 

void IAX2Processor::StartNoResponseTimer(PINDEX msToWait) 
{
  if (msToWait == 0)
    msToWait = NoResponseTimePeriod;
  
  noResponseTimer = PTimeInterval(msToWait); 
}

void IAX2Processor::OnNoResponseTimeoutStart(PTimer &, INT)
{
  //call sub class to alert that there was a timeout for a response from the server
  OnNoResponseTimeout();
}

void IAX2Processor::Activate()
{
  activate.Signal();
}

void IAX2Processor::Terminate()
{
  endThread = TRUE;
  
  //we aren't using this source call number so signal the end point
  //this will stop it attempting to send new packets to us
  if (remote.SourceCallNumber() != remote.callNumberUndefined) {    
    endpoint.ReleaseSrcCallNumber(this);
  }
  
  if (remote.DestCallNumber() != 0) {
    endpoint.ReleaseDestCallNumber(this);
  }
  
  PTRACE(3, "Processor has been directed to end. So end now.");
  if (IsTerminated()) {
    PTRACE(3, "Processor has already ended");
  }
  
  Activate();
}

BOOL IAX2Processor::ProcessOneIncomingEthernetFrame()
{  
  IAX2Frame *frame = frameList.GetLastFrame();
  if (frame == NULL) {
    return FALSE;
  }
  
  //check the frame has not already been built
  if (!PIsDescendant(frame, IAX2MiniFrame) && !PIsDescendant(frame, IAX2FullFrame)) {
    PTRACE(3, "IaxConnection\tUnknown  incoming frame " << frame->IdString());
    IAX2Frame *af = frame->BuildAppropriateFrameType(encryption);
    delete frame;
    
    if (af == NULL)
      return TRUE;  
      
    frame = af;  
  }  
  
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
    endpoint.RegisterDestCallNumber(this);
  }
  
  ProcessFullFrame(*f);
 
  if (f != NULL)
    delete f;

  return TRUE;   /*There could be more frames to process. */
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

BOOL IAX2Processor::Authenticate(IAX2FullFrameProtocol *reply, PString & password)
{
  BOOL processed = FALSE;
  IAX2IeAuthMethods ie(ieData.authMethods);
  
  if (ie.IsMd5Authentication()) {
    PTRACE(3, "Processor\tMD5 Authentiction yes, make reply up");
    IAX2IeMd5Result *res = new IAX2IeMd5Result(ieData.challenge, password);
    reply->AppendIe(res);
    processed = TRUE;
    encryption.SetChallengeKey(ieData.challenge);
    encryption.SetEncryptionKey(password);
  } else if (ie.IsPlainTextAuthentication()) {
    /*TODO: in the future we might want a policy of only
    allowing md5 passwords.  This would make
    injecting plain auth IEs useless.*/
    reply->AppendIe(new IAX2IePassword(password));
    processed = TRUE;
  } else if (ie.IsRsaAuthentication()) {
    PTRACE(3, "DO NOT handle RSA authentication ");
    reply->SetSubClass(IAX2FullFrameProtocol::cmdInval);
    processed = TRUE;
  }
  
  if (ieData.encryptionMethods == IAX2IeEncryption::encryptAes128) {
    PTRACE(3, "Processor\tEnable AES 128 encryption");
    encryption.SetEncryptionOn();
    reply->AppendIe(new IAX2IeEncryption);
  }

  return processed;
}

void IAX2Processor::SendAckFrame(IAX2FullFrame *inReplyTo)
{
  PTRACE(3, "Processor\tSend an ack frame in reply" );
  PTRACE(3, "Processor\tIn reply to " << *inReplyTo);
  
  //callIrrelevant is used because if a call ends we still want the remote
  //endpoint to get the acknowledgment of the call ending!
  IAX2FullFrameProtocol *f = new IAX2FullFrameProtocol(this, IAX2FullFrameProtocol::cmdAck, inReplyTo, 
    IAX2FullFrame::callIrrelevant);
  PTRACE(4, "Swquence for sending is (pre) " << sequence.AsString());
  TransmitFrameToRemoteEndpoint(f);
  PTRACE(4, "Sequence for sending is (ppost) " << sequence.AsString());
}

BOOL IAX2Processor::ProcessCommonNetworkFrame(IAX2FullFrameProtocol * src)
{
  switch(src->GetSubClass()) {
  case IAX2FullFrameProtocol::cmdLagRq:
    ProcessIaxCmdLagRq(src);
    break;
  case IAX2FullFrameProtocol::cmdLagRp:
    ProcessIaxCmdLagRp(src);
    break;
  case IAX2FullFrameProtocol::cmdVnak:
    ProcessIaxCmdVnak(src);
    break;
  case IAX2FullFrameProtocol::cmdPing:
    ProcessIaxCmdPing(src);
    break;
  case IAX2FullFrameProtocol::cmdPong:
    ProcessIaxCmdPong(src);
    break;
  default:
    return FALSE;
  }
  
  return TRUE;
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

void IAX2Processor::ProcessIaxCmdVnak(IAX2FullFrameProtocol * /*src*/)
{
  PTRACE(3, "ProcessIaxCmdVnak(IAX2FullFrameProtocol *src)");
}
