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
 * $Log: iax2con.cxx,v $
 * Revision 1.26  2007/08/02 23:25:07  dereksmithies
 * Rework iax2 handling of incoming calls. This should ensure that woomera/simpleopal etc
 * will correctly advise on receiving an incoming call.
 *
 * Revision 1.25  2007/08/01 05:16:03  dereksmithies
 * Work on getting the different phases right.
 *
 * Revision 1.24  2007/08/01 04:30:22  dereksmithies
 * When connecting a call (at the iax layer), make sure the Opal layer knows
 * about the state change to connected.
 *
 * Revision 1.23  2007/08/01 02:20:24  dereksmithies
 * Change the way we accept/reject incoming iax2 calls. This change makes us
 * more compliant to the OPAL standard. Thanks Craig for pointing this out.
 *
 * Revision 1.22  2007/07/31 23:17:16  dereksmithies
 * Add code to set payload size, which enables it to work. This is required
 * because we are generating RTP_DataFrames so we can use the OpalJitterBuffer
 *
 * Revision 1.21  2007/04/22 22:37:59  dereksmithies
 * Lower verbosity of PTRACE statements.
 *
 * Revision 1.20  2007/03/29 05:16:49  csoutheren
 * Pass OpalConnection to OpalMediaSream constructor
 * Add ID to OpalMediaStreams so that transcoders can match incoming and outgoing codecs
 *
 * Revision 1.19  2007/03/01 05:51:04  rjongbloed
 * Fixed backward compatibility of OnIncomingConnection() virtual
 *   functions on various classes. If an old override returned FALSE
 *   then it will now abort the call as it used to.
 *
 * Revision 1.18  2007/01/24 04:00:57  csoutheren
 * Arrrghh. Changing OnIncomingConnection turned out to have a lot of side-effects
 * Added some pure viritual functions to prevent old code from breaking silently
 * New OpalEndpoint and OpalConnection descendants will need to re-implement
 * OnIncomingConnection. Sorry :)
 *
 * Revision 1.17  2007/01/18 04:45:16  csoutheren
 * Messy, but simple change to add additional options argument to OpalConnection constructor
 * This allows the provision of non-trivial arguments for connections
 *
 * Revision 1.16  2007/01/17 03:48:48  dereksmithies
 * Tidy up comments, remove leaks, improve reporting of packet types.
 *
 * Revision 1.15  2007/01/12 02:48:11  dereksmithies
 * Make the iax2callprocessor a more permanent variable in the iax2connection.
 *
 * Revision 1.14  2007/01/12 02:39:00  dereksmithies
 * Remove the notion of srcProcessors and dstProcessor lists from the ep.
 * Ensure that the connection looks after the callProcessor.
 *
 * Revision 1.13  2007/01/11 03:02:15  dereksmithies
 * Remove the previous audio buffering code, and switch to using the jitter
 * buffer provided in Opal. Reduce the verbosity of the log mesasges.
 *
 * Revision 1.12  2006/11/02 09:08:49  rjongbloed
 * Fixed compiler warning
 *
 * Revision 1.11  2006/09/13 00:20:12  csoutheren
 * Fixed warnings under VS.net
 *
 * Revision 1.10  2006/09/11 03:08:50  dereksmithies
 * Add fixes from Stephen Cook (sitiveni@gmail.com) for new patches to
 * improve call handling. Notably, IAX2 call transfer. Many thanks.
 * Thanks also to the Google summer of code for sponsoring this work.
 *
 * Revision 1.9  2006/08/09 03:46:39  dereksmithies
 * Add ability to register to a remote Asterisk box. The iaxProcessor class is split
 * into a callProcessor and a regProcessor class.
 * Big thanks to Stephen Cook, (sitiveni@gmail.com) for this work.
 *
 * Revision 1.8  2006/06/16 01:47:08  dereksmithies
 * Get the OnHold features of IAX2 to work correctly.
 * Thanks to Stephen Cook, (sitiveni@gmail.com) for this work.
 *
 * Revision 1.7  2005/09/05 01:19:43  dereksmithies
 * add patches from Adrian Sietsma to avoid multiple hangup packets at call end,
 * and stop the sending of ping/lagrq packets at call end. Many thanks.
 *
 * Revision 1.6  2005/08/26 03:07:38  dereksmithies
 * Change naming convention, so all class names contain the string "IAX2"
 *
 * Revision 1.5  2005/08/24 04:56:25  dereksmithies
 * Add code from Adrian Sietsma to send FullFrameTexts and FullFrameDtmfs to
 * the remote end.  Many Thanks.
 *
 * Revision 1.4  2005/08/24 01:38:38  dereksmithies
 * Add encryption, iax2 style. Numerous tidy ups. Use the label iax2, not iax
 *
 * Revision 1.3  2005/08/13 07:19:17  rjongbloed
 * Fixed MSVC6 compiler issues
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
  opalPayloadType = RTP_DataFrame::MaxPayloadType;

  remotePartyAddress = "iax2:" + inRemoteParty;
  if (inRemotePartyName.IsEmpty())
    remotePartyName = inRemoteParty;
  else
    remotePartyName = inRemotePartyName;
    
  PStringList res = IAX2EndPoint::DissectRemoteParty(inRemoteParty);
  remotePartyNumber = res[IAX2EndPoint::extensionIndex];
  

  iax2Processor.AssignConnection(this);
  SetCallToken(token);
  originating = FALSE;

  ep.CopyLocalMediaFormats(localMediaFormats);
  AdjustMediaFormats(localMediaFormats);
  for (PINDEX i = 0; i < localMediaFormats.GetSize(); i++) {
    PTRACE(5, "Local ordered codecs are " << localMediaFormats[i]);
  }
  
  local_hold = FALSE;
  remote_hold = FALSE;

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

void IAX2Connection::ClearCall(CallEndReason reason)
{
  PTRACE(3, "IAX2Con\tClearCall(reason);");

  jitterBuffer.CloseDown();

  callEndReason = reason;
  iax2Processor.Hangup(reason);

  OpalConnection::ClearCall(reason);
}


void IAX2Connection::Release( CallEndReason reason)		        
{ 
  PTRACE(3, "IAX2Con\tRelease( CallEndReason " << reason);
  iax2Processor.Hangup(reason);

  iax2Processor.Release(reason); 
  OpalConnection::Release(reason);
}

void IAX2Connection::OnReleased()
{
  PTRACE(3, "IAX2Con\tOnReleased()" << *this);

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

BOOL IAX2Connection::OnIncomingConnection(unsigned int options, OpalConnection::StringOptions * stringOptions)
{
  PTRACE(3, "IAX2Con\tOnIncomingConnection()");
  phase = SetUpPhase;
  originating = FALSE;
  PTRACE(3, "IAX2Con\tWe are receiving an incoming IAX2 call");
  PTRACE(3, "IAX2Con\tOnIncomingConnection  - we have received a cmdNew packet");
  return endpoint.OnIncomingConnection(*this, options, stringOptions);
}

BOOL IAX2Connection::OnIncomingConnection(unsigned int options)
{
  OpalConnection::StringOptions stringOptions;
  return IAX2Connection::OnIncomingConnection(options, &stringOptions);
}

BOOL IAX2Connection::OnIncomingConnection()
{
  unsigned int options = 0;
  return IAX2Connection::OnIncomingConnection(options);
}


void IAX2Connection::OnAlerting()
{
  PTRACE(3, "IAX2Con\tOnAlerting()");
  PTRACE(3, "IAX2Con\t ON ALERTING " 
	 << PString(IsOriginating() ? " Originating" : "Receiving"));
  PTRACE(3, "IAX2Con\tOn Alerting. Phone is ringing at  " << GetRemotePartyName());
  OpalConnection::OnAlerting();

  
  jitterBuffer.SetDelay(endpoint.GetManager().GetMinAudioJitterDelay() * 8,
			endpoint.GetManager().GetMaxAudioJitterDelay() * 8);
  jitterBuffer.Resume(NULL);
}

BOOL IAX2Connection::SetAlerting(const PString & PTRACE_PARAM(calleeName), BOOL /*withMedia*/) 
{ 
  if (IsOriginating()) {
    PTRACE(2, "IAX2\tSetAlerting ignored on call we originated.");
    return TRUE;
  }

  PSafeLockReadWrite safeLock(*this);
  if (!safeLock.IsLocked())
    return FALSE;

  PTRACE(3, "IAX2Con\tSetAlerting  from " << calleeName << " " << *this); 

  if (phase == AlertingPhase)
    return FALSE;

  alertingTime = PTime();
  phase = AlertingPhase;

  OnAlerting();

  return TRUE;
}

BOOL IAX2Connection::SetConnected()
{
  PTRACE(3, "IAX2Con\tSetConnected " << *this);
  PTRACE(3, "IAX2Con\tSETCONNECTED " 
	 << PString(IsOriginating() ? " Originating" : "Receiving"));

  if (!originating) {
    PTRACE(3, "IAX2Con\tGet the iax2 code to mark us as connected\n");
    iax2Processor.SetConnected();
  } else {
    PTRACE(3, "IAX2Con\tNot originator, so don't Get the iax2 code to mark us as connected\n");    
  }

  // Set flag that we are up to CONNECT stage
  connectedTime = PTime();
  SetPhase(ConnectedPhase);
  OnConnected();
  return TRUE;
}

void IAX2Connection::OnConnected()
{
  PTRACE(3, "IAX2Con\tOnConnected()");
  PTRACE(3, "IAX2Con\t ON CONNECTED " 
	 << PString(IsOriginating() ? " Originating" : "Receiving"));
  PTRACE(3, "IAX2Con\tThis call has been connected");

  OpalConnection::OnConnected();
}

void IAX2Connection::SendDtmf(const PString & dtmf)
{
  iax2Processor.SendDtmf(dtmf); 
}

BOOL IAX2Connection::SendUserInputString(const PString & value) 
{ 
  SendUserInputModes mode = GetRealSendUserInputMode();

  PTRACE(2, "IAX2\tSendUserInput(\"" << value << "\"), using mode " << mode);

  if (mode == SendUserInputAsString) {
    iax2Processor.SendText(value); 
    return TRUE;
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
  
BOOL IAX2Connection::SendUserInputTone(char tone, unsigned /*duration*/ ) 
{ 
  iax2Processor.SendDtmf(tone); 
  return TRUE;
}

void IAX2Connection::OnEstablished()
{
  phase = EstablishedPhase;
  PTRACE(3, "IAX2Con\t ON ESTABLISHED " 
	 << PString(IsOriginating() ? " Originating" : "Receiving"));
  OpalConnection::OnEstablished();
  iax2Processor.SetEstablished(originating);
}

OpalMediaStream * IAX2Connection::CreateMediaStream(const OpalMediaFormat & mediaFormat,
                                                   unsigned sessionID,
                                                   BOOL isDataSource)
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

BOOL IAX2Connection::SetUpConnection() 
{
  PTRACE(3, "IAX2Con\tSetUpConnection() ");
  PTRACE(3, "IAX2Con\tWe are making a call");
  
  iax2Processor.SetUserName(userName);
  iax2Processor.SetPassword(password);
  
  originating = TRUE;
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
  int res;

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
    PINDEX local;
    for (local = 0; local < localMediaFormats.GetSize(); local++) {
      if (localMediaFormats[local].GetPayloadType() == remoteMediaFormats[0].GetPayloadType()) {
	res = local;
	goto selectCodec;
      }
    }

    for (local = 0; local < localMediaFormats.GetSize(); local++) 
      for (PINDEX remote = 0; local < remoteMediaFormats.GetSize(); remote++) {
	if (localMediaFormats[local].GetPayloadType() == remoteMediaFormats[remote].GetPayloadType()) {
	  res = local;
	  goto selectCodec;
	}
      }
  }
  PTRACE(0, "Connection. Failed to select a codec " );
  cerr << "Failed to select a codec" << endl;
  return 0;

 selectCodec:
  opalPayloadType = localMediaFormats[res].GetPayloadType();

  PStringStream strm;
  strm << localMediaFormats[res];
  PTRACE(4, "Connection\t have selected the codec " << strm);

  return IAX2FullFrameVoice::OpalNameToIax2Value(strm);
}

BOOL IAX2Connection::IsConnectionOnHold()
{
  return (local_hold || remote_hold);
}

void IAX2Connection::HoldConnection()
{
  if (local_hold)
    return;
    
  local_hold = TRUE;
  PauseMediaStreams(TRUE);
  endpoint.OnHold(*this);
  
  iax2Processor.SendHold();
}

void IAX2Connection::RetrieveConnection()
{
  if (!local_hold)
    return;
  
  local_hold = FALSE;
  PauseMediaStreams(FALSE);  
  endpoint.OnHold(*this);
  
  iax2Processor.SendHoldRelease();
}

void IAX2Connection::RemoteHoldConnection()
{
  if (remote_hold)
    return;
    
  remote_hold = TRUE;
  endpoint.OnHold(*this);
}

void IAX2Connection::RemoteRetrieveConnection()
{
  if (!remote_hold)
    return;
    
  remote_hold = FALSE;    
  endpoint.OnHold(*this);
}

void IAX2Connection::TransferConnection(
  const PString & remoteParty, 
  const PString & /*callIdentity*/)
{
  //The call identity is not used because we do not handle supervised transfers yet.  
  PTRACE(3, "Transfer call to " + remoteParty);
  
  PStringList rpList = IAX2EndPoint::DissectRemoteParty(remoteParty);
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
      SetAlerting(localPartyName, TRUE);
      break;

    case AnswerCallPending :
      SetAlerting(localPartyName, FALSE);
      break;

    case AnswerCallNow :
      PTRACE(2, "IAX2\tApplication has Accepted answer incoming call");
      SetConnected();

    default : // AnswerCallDeferred
      PTRACE(2, "IAX2\tAnswering call: has been deferred");
      break;
  }
}

BOOL IAX2Connection::ForwardCall(const PString & PTRACE_PARAM(forwardParty))
{
  PTRACE(3, "Forward call to " + forwardParty);
  //we can not currently forward calls that have not been accepted.
  return FALSE;
}

void IAX2Connection::ReceivedSoundPacketFromNetwork(IAX2Frame *soundFrame)
{
    PTRACE(6, "RTP\tIAX2 Incoming Media frame of " << soundFrame->GetMediaDataSize() << " bytes and timetamp=" << (soundFrame->GetTimeStamp() * 8));

    if (opalPayloadType == RTP_DataFrame::MaxPayloadType) {
      //have not done a capability decision. (or capability failed).
      PTRACE(3, "RTP\tDump this sound frame, as no capability decision has been made");
      delete soundFrame;
      return;
    }

    RTP_DataFrame *mediaFrame = new RTP_DataFrame();
    mediaFrame->SetTimestamp(soundFrame->GetTimeStamp() * 8);
    mediaFrame->SetMarker(FALSE);
    mediaFrame->SetPayloadType(opalPayloadType);

    mediaFrame->SetPayloadSize(soundFrame->GetMediaDataSize());
    mediaFrame->SetSize(mediaFrame->GetPayloadSize() + mediaFrame->GetHeaderSize());
    memcpy(mediaFrame->GetPayloadPtr(), soundFrame->GetMediaDataPointer(), soundFrame->GetMediaDataSize());

    jitterBuffer.NewFrameFromNetwork(mediaFrame);
    delete soundFrame;
}

BOOL IAX2Connection::ReadSoundPacket(DWORD timestamp, RTP_DataFrame & packet)
{ 
  BOOL success = jitterBuffer.ReadData(timestamp, packet); 
  if (success) {
    packet.SetPayloadSize(packet.GetSize() - packet.GetHeaderSize());
    return TRUE;
  }

  return FALSE;
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

