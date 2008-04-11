/*
 *
 * Inter Asterisk Exchange 2
 * 
 * Extension of the Opal Connection class. There is one instance of this
 * class per current call.
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
 * $Revision$
 * $Author$
 * $Date$
 */

#include <ptlib.h>
#include <typeinfo>


#ifdef P_USE_PRAGMA
#pragma implementation "iax2con.h"
#endif

#include <iax2/causecode.h>
#include <iax2/frame.h>
#include <iax2/iax2con.h>
#include <iax2/iax2ep.h>
#include <iax2/iax2medstrm.h>
#include <iax2/ies.h>
#include <iax2/sound.h>
#include <iax2/transmit.h>


#define new PNEW



////////////////////////////////////////////////////////////////////////////////


IAX2Connection::IAX2Connection(OpalCall & call,               /* Owner call for connection        */
			       IAX2EndPoint &ep, 
			     const PString & token,         /* Token to identify the connection */
			     void * /*userData */,
			     const PString & inRemoteParty,
           const PString & inRemotePartyName)
  : OpalConnection(call, ep, token), 
endpoint(ep),
iax2Processor(*new IAX2CallProcessor(ep))
{  
  opalPayloadType = RTP_DataFrame::IllegalPayloadType;

  remotePartyAddress = "iax2:" + inRemoteParty;
  if (inRemotePartyName.IsEmpty())
    remotePartyName = inRemoteParty;
  else
    remotePartyName = inRemotePartyName;
    
  PStringArray res = IAX2EndPoint::DissectRemoteParty(inRemoteParty);
  remotePartyNumber = res[IAX2EndPoint::extensionIndex];
  

  iax2Processor.AssignConnection(this);
  SetCallToken(token);
  originating = PFalse;

  ep.CopyLocalMediaFormats(localMediaFormats);
  AdjustMediaFormats(localMediaFormats);
  PTRACE(5, "Local ordered codecs are " << localMediaFormats);
  
  local_hold = PFalse;
  remote_hold = PFalse;

  phase = SetUpPhase;

  PTRACE(6, "IAX2Connection class has been initialised, and is ready to run");
}

IAX2Connection::~IAX2Connection()
{
  iax2Processor.Terminate();
  iax2Processor.WaitForTermination(1000);
  if (!iax2Processor.IsTerminated()) {
    PAssertAlways("List rpocessor failed to terminate");
  }
  PTRACE(3, "connection has terminated");

  delete & iax2Processor;
}

void IAX2Connection::Release( CallEndReason reason)		        
{ 
  PTRACE(4, "IAX2Con\tRelease( CallEndReason " << reason);
  iax2Processor.Hangup(reason); ///Send hangup frame

  iax2Processor.Release(reason); 

  jitterBuffer.CloseDown();

  OpalConnection::Release(reason);
}

void IAX2Connection::OnReleased()
{
  PTRACE(4, "IAX2Con\tOnReleased()" << *this);

  iax2Processor.OnReleased();
  OpalConnection::OnReleased();  
}

void IAX2Connection::IncomingEthernetFrame(IAX2Frame *frame)
{
  PTRACE(5, "IAX2Con\tIncomingEthernetFrame(IAX2Frame *frame)" << frame->IdString());

  if (iax2Processor.IsCallTerminating()) { 
    PTRACE(3, "IAX2Con\t***** incoming frame during termination " << frame->IdString());
     // snuck in here during termination. may be an ack for hangup or other re-transmitted frames
     IAX2Frame *af = frame->BuildAppropriateFrameType(iax2Processor.GetEncryptionInfo());
     if (af != NULL) {
       endpoint.transmitter->PurgeMatchingFullFrames(af);
       delete af;
     }
   }
   else
     iax2Processor.IncomingEthernetFrame(frame);
} 

void IAX2Connection::TransmitFrameToRemoteEndpoint(IAX2Frame *src)
{
  endpoint.transmitter->SendFrame(src);
}

void IAX2Connection::OnSetUp()
{
  PTRACE(3, "IAX2Con\tOnSetUp - we are proceeding with this call.");
  ownerCall.OnSetUp(*this); 
}

PBoolean IAX2Connection::OnIncomingCall(unsigned int options, OpalConnection::StringOptions * stringOptions)
{
  PTRACE(3, "IAX2Con\tOnIncomingCall()");

  originating = PFalse;
  PTRACE(3, "IAX2Con\tWe are receiving an incoming IAX2 call");
  PTRACE(3, "IAX2Con\tOnIncomingConnection  - we have received a cmdNew packet");
  return OnIncomingConnection(options, stringOptions);
}

PBoolean IAX2Connection::SetAlerting(const PString & PTRACE_PARAM(calleeName), PBoolean /*withMedia*/) 
{ 
  PSafeLockReadWrite safeLock(*this);
  if (!safeLock.IsLocked())
    return PFalse;

  PTRACE(3, "IAX2Con\tSetAlerting  from " << calleeName << " " << *this); 

  if (phase == AlertingPhase)
    return PFalse;

  alertingTime = PTime();
  SetPhase(AlertingPhase);

  OpalConnection::OnAlerting();
//Which puts the other Connection in this call to AlertingPhase

  return PTrue;
}

PBoolean IAX2Connection::SetConnected()
{
  PTRACE(3, "IAX2Con\tSetConnected " << *this);
  PTRACE(3, "IAX2Con\tSETCONNECTED " 
	 << PString(IsOriginating() ? " Originating" : "Receiving"));

  PTRACE(3, "IAX2Con\tGet the iax2 code to mark us as connected\n");

  // Set flag that we are up to CONNECT stage
  connectedTime = PTime();
  SetPhase(ConnectedPhase);
  OnConnected();

  jitterBuffer.SetDelay(endpoint.GetManager().GetMinAudioJitterDelay() * 8,
			endpoint.GetManager().GetMaxAudioJitterDelay() * 8);
  jitterBuffer.Resume(NULL);

 // if no media streams, try and start them
  // if we have media streams, move to Established straight away
  if (mediaStreams.IsEmpty()) {
    ownerCall.OpenSourceMediaStreams(*this, 
				     OpalMediaFormat::DefaultAudioSessionID);
    PSafePtr<OpalConnection> otherParty = 
      GetCall().GetOtherPartyConnection(*this);
    if (otherParty != NULL) {
      ownerCall.OpenSourceMediaStreams(*otherParty, 
				       OpalMediaFormat::DefaultAudioSessionID);
    }
    OnEstablished();
    SetPhase(EstablishedPhase);
  }  
  return PTrue;
}

void IAX2Connection::SendDtmf(const PString & dtmf)
{
  iax2Processor.SendDtmf(dtmf); 
}

PBoolean IAX2Connection::SendUserInputString(const PString & value) 
{ 
  SendUserInputModes mode = GetRealSendUserInputMode();

  PTRACE(2, "IAX2\tSendUserInput(\"" << value << "\"), using mode " << mode);

  if (mode == SendUserInputAsString) {
    iax2Processor.SendText(value); 
    return PTrue;
  }

  return OpalConnection::SendUserInputString(value);
}

OpalConnection::SendUserInputModes IAX2Connection::GetRealSendUserInputMode() const
{
  switch (sendUserInputMode) {
    case SendUserInputAsString:
    case SendUserInputAsTone:
      return sendUserInputMode;
    default:
      break;
  }

  return SendUserInputAsTone;
}
  
PBoolean IAX2Connection::SendUserInputTone(char tone, unsigned /*duration*/ ) 
{ 
  iax2Processor.SendDtmf(tone); 
  return PTrue;
}

void IAX2Connection::OnEstablished()
{
  PTRACE(4, "IAX2Con\t ON ESTABLISHED " 
	 << PString(IsOriginating() ? " Originating" : "Receiving"));

  iax2Processor.StartStatusCheckTimer();
  OpalConnection::OnEstablished();
}

OpalMediaStream * IAX2Connection::CreateMediaStream(const OpalMediaFormat & mediaFormat,
                                                   unsigned sessionID,
                                                   PBoolean isDataSource)
{
  if (ownerCall.IsMediaBypassPossible(*this, sessionID)) {
    PTRACE(3, "connection\t  create a null media stream ");
    return new OpalNullMediaStream(*this, mediaFormat, sessionID, isDataSource);
  }

  PTRACE(4, "IAX2con\tCreate an OpalIAX2MediaStream");
  return new OpalIAX2MediaStream(*this, mediaFormat, sessionID, isDataSource);
}

void IAX2Connection::PutSoundPacketToNetwork(PBYTEArray *sound)
{
  iax2Processor.PutSoundPacketToNetwork(sound);
} 

PBoolean IAX2Connection::SetUpConnection() 
{
  PTRACE(3, "IAX2Con\tSetUpConnection() ");
  PTRACE(3, "IAX2Con\tWe are making a call");
  
  iax2Processor.SetUserName(userName);
  iax2Processor.SetPassword(password);
  
  originating = PTrue;
  return iax2Processor.SetUpConnection(); 
}

void IAX2Connection::SetCallToken(PString newToken)
{
  PTRACE(3, "IAX2Con\tSetCallToken(PString newToken)" << newToken);

  callToken = newToken;
  iax2Processor.SetCallToken(newToken);
}

PINDEX IAX2Connection::GetSupportedCodecs() 
{ 
  return endpoint.GetSupportedCodecs(localMediaFormats);
}
  
PINDEX IAX2Connection::GetPreferredCodec()
{ 
  return endpoint.GetPreferredCodec(localMediaFormats);
}

void IAX2Connection::BuildRemoteCapabilityTable(unsigned int remoteCapability, unsigned int format)
{
  PTRACE(3, "Connection\tBuildRemote Capability table for codecs");
  
  if (remoteCapability == 0)
    remoteCapability = format;

  PINDEX i;
  if (remoteCapability != 0) {
    for (i = 0; i < IAX2FullFrameVoice::supportedCodecs; i++) {
      if ((remoteCapability & (1 << i)) == 0)
	continue;

      PString wildcard = IAX2FullFrameVoice::GetSubClassName(1 << i);
      if (!remoteMediaFormats.HasFormat(wildcard)) {
	PTRACE(4, "Connection\tRemote capability says add codec " << wildcard);
	remoteMediaFormats += OpalMediaFormat(wildcard);
      }
    }
  }

  if (format != 0) {
    PString wildcard = IAX2FullFrameVoice::GetSubClassName(format);
    remoteMediaFormats.Reorder(PStringArray(wildcard));
  }

  AdjustMediaFormats(remoteMediaFormats);
  PTRACE(4, "Connection\tREMOTE Codecs are " << remoteMediaFormats);
}

void IAX2Connection::EndCallNow(CallEndReason reason)
{ 
  OpalConnection::ClearCall(reason); 
}

unsigned int IAX2Connection::ChooseCodec()
{
  PTRACE(4, "Local codecs are  " << localMediaFormats);
  PTRACE(4, "remote codecs are " << remoteMediaFormats);
  
  if (remoteMediaFormats.GetSize() == 0) {
    PTRACE(0, "No remote media formats supported. Exit now ");
    return 0;
  }

  if (localMediaFormats.GetSize() == 0) {
    PTRACE(0, "No local media formats supported. Exit now ");
    return 0;
  }

  {
    OpalMediaFormatList::iterator local;
    for (local = localMediaFormats.begin(); local != localMediaFormats.end(); ++local) {
      if (local->GetPayloadType() == remoteMediaFormats.front().GetPayloadType()) {
	opalPayloadType = local->GetPayloadType();
        PTRACE(4, "Connection\t have selected the codec " << *local);
        return IAX2FullFrameVoice::OpalNameToIax2Value(*local);
      }
    }

    for (local = localMediaFormats.begin(); local != localMediaFormats.end(); ++local) {
      for (OpalMediaFormatList::iterator remote = remoteMediaFormats.begin(); remote != remoteMediaFormats.end(); ++remote) {
	if (local->GetPayloadType() == remote->GetPayloadType()) {
	  opalPayloadType = local->GetPayloadType();
          PTRACE(4, "Connection\t have selected the codec " << *local);
          return IAX2FullFrameVoice::OpalNameToIax2Value(*local);
	}
      }
    }
  }

  PTRACE(0, "Connection. Failed to select a codec " );
  cerr << "Failed to select a codec" << endl;
  return 0;
}

PBoolean IAX2Connection::IsConnectionOnHold()
{
  return (local_hold || remote_hold);
}

void IAX2Connection::HoldConnection()
{
  if (local_hold)
    return;
    
  local_hold = PTrue;
  PauseMediaStreams(PTrue);
  endpoint.OnHold(*this);
  
  iax2Processor.SendHold();
}

void IAX2Connection::RetrieveConnection()
{
  if (!local_hold)
    return;
  
  local_hold = PFalse;
  PauseMediaStreams(PFalse);  
  endpoint.OnHold(*this);
  
  iax2Processor.SendHoldRelease();
}

void IAX2Connection::RemoteHoldConnection()
{
  if (remote_hold)
    return;
    
  remote_hold = PTrue;
  endpoint.OnHold(*this);
}

void IAX2Connection::RemoteRetrieveConnection()
{
  if (!remote_hold)
    return;
    
  remote_hold = PFalse;    
  endpoint.OnHold(*this);
}

void IAX2Connection::TransferConnection(
  const PString & remoteParty, 
  const PString & /*callIdentity*/)
{
  //The call identity is not used because we do not handle supervised transfers yet.  
  PTRACE(3, "Transfer call to " + remoteParty);
  
  PStringArray rpList = IAX2EndPoint::DissectRemoteParty(remoteParty);
  PString remoteAddress = GetRemoteInfo().RemoteAddress();
  
  if (rpList[IAX2EndPoint::addressIndex] == remoteAddress || 
      rpList[IAX2EndPoint::addressIndex].IsEmpty()) {
        
    iax2Processor.SendTransfer(
        rpList[IAX2EndPoint::extensionIndex],
        rpList[IAX2EndPoint::contextIndex]);
  } else {
    PTRACE(1, "Cannot transfer call, hosts do not match");
  }
}

void IAX2Connection::AnsweringCall(AnswerCallResponse response)
{
  PTRACE(3, "IAX2\tAnswering call: " << response);

  PSafeLockReadWrite safeLock(*this);
  if (!safeLock.IsLocked() || GetPhase() >= ReleasingPhase)
    return;

  switch (response) {
    case AnswerCallDenied :
      // If response is denied, abort the call
      PTRACE(2, "IAX2\tApplication has declined to answer incoming call");
      Release(EndedByAnswerDenied);
      break;

    case AnswerCallAlertWithMedia :
      SetAlerting(GetLocalPartyName(), PTrue);
      break;

    case AnswerCallPending :
      SetAlerting(GetLocalPartyName(), PFalse);
      break;
      
    case AnswerCallNow: 
      PTRACE(2, "IAX2\tApplication has Accepted answer incoming call");
      iax2Processor.AcceptIncomingCall();

    default : // AnswerCallDeferred
      PTRACE(2, "IAX2\tAnswering call: has been deferred");
      break;
  }
}

PBoolean IAX2Connection::ForwardCall(const PString & PTRACE_PARAM(forwardParty))
{
  PTRACE(3, "Forward call to " + forwardParty);
  //we can not currently forward calls that have not been accepted.
  return PFalse;
}

void IAX2Connection::ReceivedSoundPacketFromNetwork(IAX2Frame *soundFrame)
{
    PTRACE(6, "RTP\tIAX2 Incoming Media frame of " << soundFrame->GetMediaDataSize() 
	   << " bytes and timetamp=" << (soundFrame->GetTimeStamp() * 8));

    if (opalPayloadType == RTP_DataFrame::IllegalPayloadType) {
      //have not done a capability decision. (or capability failed).
      PTRACE(3, "RTP\tDump this sound frame, as no capability decision has been made");
      delete soundFrame;
      return;
    }

    RTP_DataFrame *mediaFrame = new RTP_DataFrame();
    mediaFrame->SetTimestamp(soundFrame->GetTimeStamp() * 8);
    mediaFrame->SetMarker(PFalse);
    mediaFrame->SetPayloadType(opalPayloadType);

    mediaFrame->SetPayloadSize(soundFrame->GetMediaDataSize());
    mediaFrame->SetSize(mediaFrame->GetPayloadSize() + mediaFrame->GetHeaderSize());
    memcpy(mediaFrame->GetPayloadPtr(), soundFrame->GetMediaDataPointer(), 
	   soundFrame->GetMediaDataSize());
    jitterBuffer.NewFrameFromNetwork(mediaFrame);
    delete soundFrame;
}

PBoolean IAX2Connection::ReadSoundPacket(RTP_DataFrame & packet)
{ 
  if (!jitterBuffer.ReadData(packet))
    return false;

  packet.SetPayloadSize(packet.GetSize() - packet.GetHeaderSize());
  return PTrue;
}

/* The comment below is magic for those who use emacs to edit this file. */
/* With the comment below, the tab key does auto indent to 2 spaces.     */


/*
 * Local Variables:
 * mode:c
 * c-file-style:linux
 * c-basic-offset:2
 * End:
 */

