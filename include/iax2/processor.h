/*
 *
 * Inter Asterisk Exchange 2
 * 
 * The core routine which determines the processing of packets for one call.
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
 *  $Log: processor.h,v $
 *  Revision 1.7  2005/09/05 01:19:43  dereksmithies
 *  add patches from Adrian Sietsma to avoid multiple hangup packets at call end,
 *  and stop the sending of ping/lagrq packets at call end. Many thanks.
 *
 *  Revision 1.6  2005/08/26 03:07:38  dereksmithies
 *  Change naming convention, so all class names contain the string "IAX2"
 *
 *  Revision 1.5  2005/08/25 03:26:06  dereksmithies
 *  Add patch from Adrian Sietsma to correctly set the packet timestamps under windows.
 *  Many thanks.
 *
 *  Revision 1.4  2005/08/24 04:56:25  dereksmithies
 *  Add code from Adrian Sietsma to send FullFrameTexts and FullFrameDtmfs to
 *  the remote end.  Many Thanks.
 *
 *  Revision 1.3  2005/08/24 01:38:38  dereksmithies
 *  Add encryption, iax2 style. Numerous tidy ups. Use the label iax2, not iax
 *
 *  Revision 1.2  2005/08/04 08:14:17  rjongbloed
 *  Fixed Windows build under DevStudio 2003 of IAX2 code
 *
 *  Revision 1.1  2005/07/30 07:01:32  csoutheren
 *  Added implementation of IAX2 (Inter Asterisk Exchange 2) protocol
 *  Thanks to Derek Smithies of Indranet Technologies Ltd. for
 *  writing and contributing this code
 *
 *
 *
 *
 *
 *
 *
 */

#ifndef PROCESSOR_H
#define PROCESSOR_H

#include <ptlib.h>
#include <opal/connection.h>

#include <iax2/frame.h>
#include <iax2/iedata.h>
#include <iax2/remote.h>
#include <iax2/safestrings.h>
#include <iax2/sound.h>

class IAX2EndPoint;
class IAX2Connection;




////////////////////////////////////////////////////////////////////////////////
/**This class defines what the processor is to do on receiving an ack
   for a full frame.  Thus, the processor sends a full frame (say
   Accept) and defines an action (using this class) to carry out on
   receiving the ack for the Accept.

   Essentially, this class provides us a means of "knowing" the other
   end has acted on the sent full frame. The action to do could be as
   simple as saying, "other end has accepted our Accept packet"

   It also helps us to move on in the call setup phase. On receipt of
   one particular ack packet, we send the next packet in the call
   process.  Thus, on receipt of a ack for a timestamp of X and oseqno
   of Y, we send this packet.
   
   The outgoing IAX2FullFrame has an inSeqNo, which must match the
   outSeqNo of the received ack frame.
*/
class IAX2WaitingForAck : public PObject
{
  PCLASSINFO(IAX2WaitingForAck, PObject);

 public:
  /**The action to do on receiving the specified ack */
  enum ResponseToAck {
    RingingAcked   = 0,  /*!< Processing acknowledgment to Remote end is ringing  message */ 
    AcceptAcked    = 1,  /*!< Processing acknowledgment to accept  message                */ 
    AuthRepAcked   = 2,  /*!< Processing acknowledgment to AuthRep message                */ 
    AnswerAcked    = 3   /*!< Processing acknowledgment to Answer message                 */ 
  };
  
  /**Construct this response message with values that cannot match an incoming ack packet*/
  IAX2WaitingForAck();
  
  /**Assign this response to wait for the specified coords */
  void Assign(IAX2FullFrame *f, ResponseToAck _response);
  
  /**Return true if the supplied ack frame matches the internal coordinates */
  BOOL MatchingAckPacket(IAX2FullFrame *f);
  
  /**Report the response to carry out */
  ResponseToAck GetResponse() { return response; }
  
  /**Report the internal response code as a string */
  PString GetResponseAsString() const;
  
  /**Pretty print this response to the designated stream*/
  virtual void PrintOn(ostream & strm) const;
  
  /**Initialise this to no response, and never to match */
  void ZeroValues();
  
 private:
  /**Timestamp of the ack packet we are looking for */
  DWORD timeStamp;
  
  /**OutSeqNo of the ack packet we are looking for */
  PINDEX seqNo;
  
  /**Do this action on finding a matching ACK packet */
  ResponseToAck response;
};

////////////////////////////////////////////////////////////////////////////////
/**This class does the work of processing the lists of IAX packets (in and out) 
   that are associated with each call. There is one IAX2Processor per connection.
   There is a special processor which is created to handle the weirdo iax2 packets that are sent outside of a particular call. 
   Examples of weirdo packets are the ping/pong/lagrq/lagrp. */
class IAX2Processor : public PThread
{
  PCLASSINFO(IAX2Processor, PThread);
  
 public:
  
  /**Construct this class */
  IAX2Processor(IAX2EndPoint & ep);

  /**Destructor */
  ~IAX2Processor(); 

  /**Assign a pointer to the connection class to process, and starts thread */
  void AssignConnection(IAX2Connection * _con);
 
  /**The worker method of this thread. In here, all incoming frames (for this call)
     are handled.
  */
  virtual void Main();
  
  /**Cause this thread to die immediately */
  void Terminate();
  
  /**Cause this thread to come to life, and process events that are
   * pending at IAX2Connection */
  void Activate();

  /**Handle a sound packet received from the sound device. 
     
  Now onsend this to the remote endpoint. */
  void PutSoundPacketToNetwork(PBYTEArray *sund);

 /**Get information on IAX2Remote class (remote node address & port + source & dest call number.) */
  IAX2Remote & GetRemoteInfo() { return remote; }
  
  /**Get the sequence number info (inSeqNo and outSeqNo) */
  IAX2SequenceNumbers & GetSequenceInfo() { return sequence; }
  
  /**Get the iax2 encryption info */
  IAX2Encryption & GetEncryptionInfo() { return encryption; }

  /**Get the call start tick */
  const PTimeInterval & GetCallStartTick() { return callStartTick; }

  /**Call back from the IAX2Connection class */
  virtual void Release(OpalConnection::CallEndReason releaseReason = OpalConnection::EndedByLocalUser);

  /**From the IAX2Connection class. CAlling this sends a hangup frame */
  void ClearCall(OpalConnection::CallEndReason releaseReason = OpalConnection::EndedByLocalUser);

/**Call back from the IAX2Connection class. This indicates the
   IAX2Connection class is in the final stages of destruction. At this
   point, we can terminate the thread that processors all iax
   packets */
  virtual void OnReleased();
  
  /** Ask this IAX2Processor to send dtmf to the remote endpoint. The dtmf is placed on a queue,
      ready for transmission in fullframes of type dtmf. */
  void SendDtmf(PString  dtmfs);

  /** Ask this IAX2Processor to send text to the remote endpoint. The text is placed on a queue,
      ready for transmission in fullframes of type text. */
  void SendText(const PString & text);

  /**Start an outgoing connection.
     This function will initiate the connection to the remote entity, for
     example in H.323 it sends a SETUP, in SIP it sends an INVITE etc.
     
     The behaviour at the opal level is pure. Here, the method is defined.
  */
  virtual BOOL SetUpConnection();

  /**Handle a received IAX frame. This may be a mini frame or full frame */
  void IncomingEthernetFrame (IAX2Frame *frame);

  /**A sound packet has been given (by the Receiver thread) to this
     IAX2Processor class, and queued on the IAX2SoundList structure. The
     OpalIAX2MediaStream thread will call this method to retrieving the
     sound packet. The OpalIAX2MediaStream will handle this packet
     appropriately - eg send to the PC speaker..
   */
  IAX2Frame *GetSoundPacketFromNetwork();


  /**Set the flag to indicate if we are handling specialPackets (those
     packets which are not sent to any particular call) */
  void SetSpecialPackets(BOOL newValue) { specialPackets = newValue; }

  /**Test to see if it is a status query type iax frame (eg lagrq) and handle it. If the frame
     is a status query, and it is handled, return TRUE */
  BOOL IsStatusQueryEthernetFrame(IAX2Frame *frame);

  /**Return TRUE if the remote info in the frame matches the remote info in this connection */
  BOOL Matches(IAX2Frame *frame) { return remote == (frame->GetRemoteInfo()); }
  
  
  /**A method to cause some of the values in this class to be formatted
     into a printable stream */
  virtual void PrintOn(ostream & strm) const;
  
  /**Invoked by the User interface, which causes the statistics (count of in/out packets)
     to be printed*/
  void ReportStatistics();
  

  /**Return TRUE if the arg matches the source call number for this connection */
  BOOL MatchingLocalCallNumber(PINDEX compare) { return (compare == remote.SourceCallNumber()); }
  
  
  /**Get the bit pattern of the selected codec */
  unsigned short GetSelectedCodec() { return (unsigned short) selectedCodec; }
  
  /**Indicate to remote endpoint we are connected.*/
  void SetConnected(); 
  
  /**Send appropriate packets to the remote node to indicate we will accept this call.
     Note that this method is called from the endpoint thread, (not this IAX2Connection's thread*/
  void AcceptIncomingCall();

  /**Indicate to remote endpoint an alert is in progress.  If this is
     an incoming connection and the AnswerCallResponse is in a
     AnswerCallDeferred or AnswerCallPending state, then this function
     is used to indicate to that endpoint that an alert is in
     progress. This is usually due to another connection which is in
     the call (the B party) has received an OnAlerting() indicating
     that its remoteendpoint is "ringing".

     The default behaviour is pure.
  */
  virtual BOOL SetAlerting(
			   const PString & calleeName,   /// Name of endpoint being alerted.
			   BOOL withMedia                /// Open media with alerting
			   ) ;
  
  /**Give the call token a value. The call token is the ipaddress of
     the remote node concatented with the remote nodes src
     number. This is guaranteed to be unique.  Sadly, if this
     connection is setting up the call, the callToken is not known
     until receipt of the first packet from the remote node.

     However, if this connection is created in response to a call,
     this connection can determine the callToken on examination of
     that incoming first packet */
  void SetCallToken(PString newToken);

  /**Advise the procesor that this call is totally setup, and answer accordingly
   */
  void SetEstablished(BOOL originator        ///Flag to indicate if we created the call
		      );

  /**Return the string that identifies this IAX2Connection instance */
  PString GetCallToken();

  /**Access the endpoint class that launched this processor */
  IAX2EndPoint & GetEndPoint() { return endpoint; }

  /**Cause this thread to hangup the current call, but not die. Death
     will come soon though. The argument is placed in the iax2 hangup
     packet as the cause string.*/
  void Hangup(PString messageToSend);

  /**Report the status of the flag callEndingNow, which indicates if
     the call is terminating under iax2 control  */
  BOOL IsCallTerminating() { return callStatus & callTerminating; }
  
 protected:
  
  /**Reference to the global variable of this program */
  IAX2EndPoint      & endpoint;
  
  /**The connection class we are charged with running. */
  IAX2Connection * con;

  /**@name Internal, protected methods, which are invoked only by this
     thread*/
  //@{
  /** Test the value supplied in the format Ie is compatible.*/
  BOOL RemoteSelectedCodecOk();
 
  /** Check to see if there is an outstanding request to send a hangup frame. This needs
      to be done in two places, so we use a routine to see if need to send a hanup frame.*/
  void CheckForHangupMessages();
 
  /**Transmit an iax protocol frame with subclass type ack immediately to remote endpoint */
  void SendAckFrame(IAX2FullFrame *inReplyTo);
  
  /**Transmit IAX2Frame to remote endpoint, and then increment send count. This calls a method in
     the Transmitter class. .It is only called by the this IAX2Processor class.  */
  void TransmitFrameToRemoteEndpoint(IAX2Frame *src);

  /**Transmit IAX2Frame to remote endpoint, and then increment send
     count. This calls a method in the Transmitter class. .It is only
     called by the this IAX2Processor class. The second parameter
     determines what to do when an ack frame is received for the sent
     frame.  */
  void TransmitFrameToRemoteEndpoint(IAX2FullFrame *src,
				     IAX2WaitingForAck::ResponseToAck response  ///action to do on getting Ack
				     );

  /**Transmit IAX2Frame to remote endpoint,. This calls a method in the
     Transmitter class. .It is only called by the this Connection
     class. There is no stats change when this method is called. */
  void TransmitFrameNow(IAX2Frame *src);
  
  /**FullFrameProtocol class needs to have the IE's correctly appended prior to transmission */
  void TransmitFrameToRemoteEndpoint(IAX2FullFrameProtocol *src);
  
  /**Internal method to process an incoming network frame of type  IAX2Frame */
  void ProcessNetworkFrame(IAX2Frame * src);
  
  /**Internal method to process an incoming network frame of type  IAX2MiniFrame */
  void ProcessNetworkFrame(IAX2MiniFrame * src);
  
  /**Internal method to process an incoming network frame of type  IAX2FullFrame */
  void ProcessNetworkFrame(IAX2FullFrame * src);
  
  /**Internal method to process an incoming network frame of type  IAX2FullFrameDtmf */
  void ProcessNetworkFrame(IAX2FullFrameDtmf * src);
  
  /**Internal method to process an incoming network frame of type  IAX2FullFrameVoice */
  void ProcessNetworkFrame(IAX2FullFrameVoice * src);
  
  /**Internal method to process an incoming network frame of type  IAX2FullFrameVideo */
  void ProcessNetworkFrame(IAX2FullFrameVideo * src);
  
  /**Internal method to process an incoming network frame of type  IAX2FullFrameSessionControl */
  void ProcessNetworkFrame(IAX2FullFrameSessionControl * src);
  
  /**Internal method to process an incoming network frame of type  IAX2FullFrameNull */
  void ProcessNetworkFrame(IAX2FullFrameNull * src);
  
  /**Internal method to process an incoming network frame of type  IAX2FullFrameProtocol.
     
  A frame of FullFrameProtocol type is labelled as AST_FRAME_IAX in the asterisk souces,
  It will contain 0, 1, 2 or more Information Elements (Ie) in the data section.*/
  void ProcessNetworkFrame(IAX2FullFrameProtocol * src);
  
  /**Internal method to process an incoming network frame of type  IAX2FullFrameText */
  void ProcessNetworkFrame(IAX2FullFrameText * src);
  
  /**Internal method to process an incoming network frame of type  IAX2FullFrameImage */
  void ProcessNetworkFrame(IAX2FullFrameImage * src);
  
  /**Internal method to process an incoming network frame of type  IAX2FullFrameHtml */
  void ProcessNetworkFrame(IAX2FullFrameHtml * src);
  
  /**Internal method to process an incoming network frame of type  IAX2FullFrameCng */
  void ProcessNetworkFrame(IAX2FullFrameCng * src);
    
  /**Go through the three lists for incoming data (ethernet/sound/UI commands.  */
  void ProcessLists();

  /**remove one frame on the incoming ethernet frame list. If there
     may be more to process, return TRUE. If there are no more to
     process, return FALSE. */
  BOOL ProcessOneIncomingEthernetFrame();
  
  /**Make a call to a remote node*/
  void ConnectToRemoteNode(PString & destination);
  
  /**Cause the dtmf full frames to go out for this dtmf character*/
  void SendDtmfMessage(char message);
  
  /**Cause the text full frames to go out for this text message*/
  void SendTextMessage(PString & message);

  /**Cause a sound frame (which is full or mini) to be sent. 
     The data in the array is already compressed. */
  void SendSoundMessage(PBYTEArray *sound);
  
  /**Increment the count of full frames sent*/
  void IncControlFramesSent() { ++controlFramesSent; }
  
  /**Increment the count of full frames received*/
  void IncControlFramesRcvd() { ++controlFramesRcvd; }
  
  /**Increment the count of audio frames sent*/
  void IncAudioFramesSent()   { ++audioFramesSent; }
  
  /**Increment the count of audio frames received*/
  void IncAudioFramesRcvd()   { ++audioFramesRcvd; }
  
  /**Increment the count of video frames sent*/
  void IncVideoFramesSent()   { ++videoFramesSent; }
  
  /**Increment the count of video frames received*/
  void IncVideoFramesRcvd()   { ++videoFramesRcvd; }
  
  /**A callback which is used to indicate the remote party 
     has accepted our call. Media can flow now*/
  void RemoteNodeHasAnswered();
  
  /**A stop sounds packet has been received, which means we have moved from 
     waiting for the remote person to answer, to they have answered and media can flow
     in both directions*/
  void CallStopSounds();
  
  /**A callback which is used to indicate that the remote party has sent us
     a hook flash message (i.e, their hook was flashed) */
  void ReceivedHookFlash();
  
  /**A callback which is used to indicate that the remote party has sent us
     a message stating they are busy. */
  void RemoteNodeIsBusy();
  
  /**Process the audio data portions of the Frame argument, which may be a MiniFrame or FullFrame */
  void ProcessIncomingAudioFrame(IAX2Frame *newFrame);
  
  /**Process the video data  portions of the Frame argument, which may be a MiniFrame or FullFrame */
  void ProcessIncomingVideoFrame(IAX2Frame *newFrame);
  
  /** Process a FullFrameProtocol class, where the sub Class value is   Create a new call    */
  void ProcessIaxCmdNew(IAX2FullFrameProtocol *src);
  
  /** Process a FullFrameProtocol class, where the sub Class value is Ping request,     */
  void ProcessIaxCmdPing(IAX2FullFrameProtocol *src);
  
  /** Process a FullFrameProtocol class, where the sub Class value is reply to a Ping    */
  void ProcessIaxCmdPong(IAX2FullFrameProtocol *src);
  
  /** Process a FullFrameProtocol class, where the sub Class value is Acknowledge a Reliably sent full frame    */
  void ProcessIaxCmdAck(IAX2FullFrameProtocol *src);
  
  /** Process a FullFrameProtocol class, where the sub Class value is Request to terminate this call    */
  void ProcessIaxCmdHangup(IAX2FullFrameProtocol *src);
  
  /** Process a FullFrameProtocol class, where the sub Class value is Refuse to accept this call. May happen if authentication faile    */
  void ProcessIaxCmdReject(IAX2FullFrameProtocol *src);
  
  /** Process a FullFrameProtocol class, where the sub Class value is Allow this call to procee    */
  void ProcessIaxCmdAccept(IAX2FullFrameProtocol *src);
  
  /** Process a FullFrameProtocol class, where the sub Class value is Ask remote end to supply authenticatio    */
  void ProcessIaxCmdAuthReq(IAX2FullFrameProtocol *src);
  
  /** Process a FullFrameProtocol class, where the sub Class value is A reply, that contains authenticatio    */
  void ProcessIaxCmdAuthRep(IAX2FullFrameProtocol *src);
  
  /** Process a FullFrameProtocol class, where the sub Class value is Destroy this call immediatly    */
  void ProcessIaxCmdInval(IAX2FullFrameProtocol *src);
  
  /** Process a FullFrameProtocol class, where the sub Class value is Initial message, used to measure the round trip time    */
  void ProcessIaxCmdLagRq(IAX2FullFrameProtocol *src);
  
  /** Process a FullFrameProtocol class, where the sub Class value is Reply to cmdLagrq, which tells us the round trip time    */
  void ProcessIaxCmdLagRp(IAX2FullFrameProtocol *src);
  
  /** Process a FullFrameProtocol class, where the sub Class value is Registration request    */
  void ProcessIaxCmdRegReq(IAX2FullFrameProtocol *src);
  
  /** Process a FullFrameProtocol class, where the sub Class value is Registration authentication required    */
  void ProcessIaxCmdRegAuth(IAX2FullFrameProtocol *src);
  
  /** Process a FullFrameProtocol class, where the sub Class value is Registration accepted    */
  void ProcessIaxCmdRegAck(IAX2FullFrameProtocol *src);
  
  /** Process a FullFrameProtocol class, where the sub Class value is Registration rejected    */
  void ProcessIaxCmdRegRej(IAX2FullFrameProtocol *src);
  
  /** Process a FullFrameProtocol class, where the sub Class value is Force release of registration    */
  void ProcessIaxCmdRegRel(IAX2FullFrameProtocol *src);
  
  /** Process a FullFrameProtocol class, where the sub Class value is If we receive voice before valid first voice frame, send this    */
  void ProcessIaxCmdVnak(IAX2FullFrameProtocol *src);
  
  /** Process a FullFrameProtocol class, where the sub Class value is Request status of a dialplan entry    */
  void ProcessIaxCmdDpReq(IAX2FullFrameProtocol *src);
  
  /** Process a FullFrameProtocol class, where the sub Class value is Request status of a dialplan entry    */
  void ProcessIaxCmdDpRep(IAX2FullFrameProtocol *src);
  
  /** Process a FullFrameProtocol class, where the sub Class value is Request a dial on channel brought up TBD    */
  void ProcessIaxCmdDial(IAX2FullFrameProtocol *src);
  
  /** Process a FullFrameProtocol class, where the sub Class value is Transfer Request    */
  void ProcessIaxCmdTxreq(IAX2FullFrameProtocol *src);
  
  /** Process a FullFrameProtocol class, where the sub Class value is Transfer Connect    */
  void ProcessIaxCmdTxcnt(IAX2FullFrameProtocol *src);
  
  /** Process a FullFrameProtocol class, where the sub Class value is Transfer Accepted    */
  void ProcessIaxCmdTxacc(IAX2FullFrameProtocol *src);
  
  /** Process a FullFrameProtocol class, where the sub Class value is Transfer ready    */
  void ProcessIaxCmdTxready(IAX2FullFrameProtocol *src);
  
  /** Process a FullFrameProtocol class, where the sub Class value is Transfer release    */
  void ProcessIaxCmdTxrel(IAX2FullFrameProtocol *src);
  
  /** Process a FullFrameProtocol class, where the sub Class value is Transfer reject    */
  void ProcessIaxCmdTxrej(IAX2FullFrameProtocol *src);
  
  /** Process a FullFrameProtocol class, where the sub Class value is Stop audio/video transmission    */
  void ProcessIaxCmdQuelch(IAX2FullFrameProtocol *src);
  
  /** Process a FullFrameProtocol class, where the sub Class value is Resume audio/video transmission    */
  void ProcessIaxCmdUnquelch(IAX2FullFrameProtocol *src);
  
  /** Process a FullFrameProtocol class, where the sub Class value is Like ping, but does not require an open connection    */
  void ProcessIaxCmdPoke(IAX2FullFrameProtocol *src);
  
  /** Process a FullFrameProtocol class, where the sub Class value is Paging description    */
  void ProcessIaxCmdPage(IAX2FullFrameProtocol *src);
  
  /** Process a FullFrameProtocol class, where the sub Class value is Stand-alone message waiting indicator    */
  void ProcessIaxCmdMwi(IAX2FullFrameProtocol *src);
  
  /** Process a FullFrameProtocol class, where the sub Class value is Unsupported message received    */
  void ProcessIaxCmdUnsupport(IAX2FullFrameProtocol *src);
  
  /** Process a FullFrameProtocol class, where the sub Class value is Request remote transfer    */
  void ProcessIaxCmdTransfer(IAX2FullFrameProtocol *src);
  
  /** Process a FullFrameProtocol class, where the sub Class value is Provision device    */
  void ProcessIaxCmdProvision(IAX2FullFrameProtocol *src);
  
  /** Process a FullFrameProtocol class, where the sub Class value is Download firmware    */
  void ProcessIaxCmdFwDownl(IAX2FullFrameProtocol *src);
  
  /** Process a FullFrameProtocol class, where the sub Class value is Firmware Data    */
  void ProcessIaxCmdFwData(IAX2FullFrameProtocol *src);
  
  /** Do the md5/rsa authentication. Return True if successful. Has the side effect of appending the
      appropriate Ie class to the "reply" parameter.*/
  BOOL Authenticate(IAX2FullFrameProtocol *reply /*!< this frame contains the result of authenticating the internal data*/
		    );
    
  /**Time this connection class was created, which is the call start
     time.  It is reported in Ticks, which is required for millisecond
     accuracy under windows.   */
  PTimeInterval callStartTick;
  
  /**Details on the address of the remote endpoint, and source/dest call numbers */
  IAX2Remote remote;
  
  /**Details on the in/out sequence numbers */
  IAX2SequenceNumbers sequence;
  
  /**Count of the number of control frames sent */
  PAtomicInteger controlFramesSent;
  
  /**Count of the number of control frames received */
  PAtomicInteger controlFramesRcvd;
  
  /**Count of the number of sound frames sent */
  PAtomicInteger audioFramesSent;
  
  /**Count of the number of sound frames received */
  PAtomicInteger audioFramesRcvd;
  
  /**Count of the number of video frames sent */
  PAtomicInteger videoFramesSent;
  
  /**Count of the number of video frames received */
  PAtomicInteger videoFramesRcvd;
  
  /**Phone number of the remote  endpoint */
  SafeString remotePhoneNumber;
  
  /**Array of remote node we have to make a call to */
  SafeStrings callList;
  
  /**Contains the concatanation  of the dtmf we have to send to the remote endpoint.
   This string is sent a character at a time, one DTMF frame per character.*/
  SafeString dtmfText;

  /**Array of the text we have to send to the remote endpoint */
  SafeStrings textList;

  /**Array of received dtmf characters (These have come from the network)*/
  SafeStrings dtmfNetworkList;

  /**Array of requests to end this current call */
  SafeStrings hangList;
      
  /**Array of sound packets read from the audio device, and is about
     to be transmitted to the remote node */
  IAX2SoundList   soundWaitingForTransmission;
  
  /**Array of sound packets which has been read from the ethernet, and
     is to be sent to the audio device */
  IAX2FrameList   soundReadFromEthernet;
  
  /**Array of frames read from the Receiver for this call */
  IAX2FrameList frameList;
  
  /**This is the timestamp of the last received full frame, which is used to reconstruct the timestamp
     of received MiniFrames */
  PINDEX lastFullFrameTimeStamp;
    
  /**Flag to indicate we are ready for audio to flow */
  BOOL audioCanFlow;

  /**Hold each of the possible values from an Ie class */
  IAX2IeData ieData;
  
  /** Bitmask of FullFrameVoice::AudioSc values to specify which codec is used*/
  unsigned int selectedCodec;
  
  /** bit mask of the different flags to indicate call status*/
  enum CallStatus {
    callNewed      =  1 << 0,   /*!< we have received a new packet to set this call up.                         */
    callSentRinging = 1 << 1,   /*!< we have sent a packet to indicate that the phone is ringing at our end     */
    callRegistered =  1 << 2,   /*!< registration with a remote asterisk server has been approved               */
    callAuthorised =  1 << 3,   /*!< we are waiting on password authentication at the remote endpoint           */
    callAccepted   =  1 << 4,   /*!< call has been accepted, which means that the new request has been approved */
    callRinging    =  1 << 5,   /*!< Remote end has sent us advice the phone is ringing, awaiting answer        */
    callAnswered   =  1 << 6,   /*!< call setup complete, now do audio                                          */
    callTerminating = 1 << 7    /*!< Flag to indicate call is closing down after iax2 call end commands         */
  };
  
  /** A defined value which is the maximum time we will wait for an answer to our packets */
  enum DefinedNoResponseTimePeriod {
    NoResponseTimePeriod = 5000 /*!< Time (in milliseconds) we will wait */
  };
  
  /** Contains the bits stating what is happening in the call */
  unsigned short callStatus;
  
  /** Mark call status as having sent a iaxcmdRinging packet */
  void SetCallSentRinging(BOOL newValue = TRUE) 
    { if (newValue) callStatus |= callSentRinging; else callStatus &= ~callSentRinging; }
  
  /** Mark call status as having received a new packet */
  void SetCallNewed(BOOL newValue = TRUE) 
    { if (newValue) callStatus |= callNewed; else callStatus &= ~callNewed; }
  
  /** Mark call status Registered (argument determines flag status) */
  void SetCallRegistered(BOOL newValue = TRUE) 
    { if (newValue) callStatus |= callRegistered; else callStatus &= ~callRegistered; }
  
  /** Mark call status Authorised (argument determines flag status) */
  void SetCallAuthorised(BOOL newValue = TRUE) 
    { if (newValue) callStatus |= callAuthorised; else callStatus &= ~callAuthorised; }
  
  /** Mark call status Accepted (argument determines flag status) */
  void SetCallAccepted(BOOL newValue = TRUE) 
    { if (newValue) callStatus |= callAccepted; else callStatus &= ~callAccepted; }
  
  /** Mark call status Ringing (argument determines flag status) */
  void SetCallRinging(BOOL newValue = TRUE) 
    { if (newValue) callStatus |= callRinging; else callStatus &= ~callRinging; }
  
  /** Mark call status Answered (argument determines flag status) */
  void SetCallAnswered(BOOL newValue = TRUE) 
    { if (newValue) callStatus |= callAnswered; else callStatus &= ~callAnswered; }

  /** Mark call status as terminated (is processing IAX2 hangup packets etc ) */
  void SetCallTerminating(BOOL newValue = TRUE) 
    { if (newValue) callStatus |= callTerminating; else callStatus &= ~callTerminating; }
  
  /** See if any of the flag bits are on, which indicate this call is actually active */
  BOOL IsCallHappening() { return callStatus > 0; }
  
  /** Get marker to indicate that some packets have flowed etc for this call  */
  BOOL IsCallNewed() { return callStatus & callNewed; }
  
  /** Get marker to indicate that we are waiting on the ack for the iaxcommandringing packet we sent  */
  BOOL IsCallSentRinging() { return callStatus & callSentRinging; }
  
  /** Get the current value of the call status flag callRegistered */
  BOOL IsCallRegistered() { return callStatus & callRegistered; }
  
  /** Get the current value of the call status flag callAuthorised */
  BOOL IsCallAuthorised() { return callStatus & callAuthorised; }
  
  /** Get the current value of the call status flag callAccepted */
  BOOL IsCallAccepted() { return callStatus & callAccepted; }
  
  /** Get the current value of the call status flag callRinging */
  BOOL IsCallRinging() { return callStatus & callRinging; }
  
  /** Get the current value of the call status flag callAnswered */
  BOOL IsCallAnswered() { return callStatus & callAnswered; }

  
  /** Advise the other end that we have picked up the phone */
  void SendAnswerMessageToRemoteNode();

  /** The timer which is used to test for no reply to our outgoing call setup messages */
  PTimer noResponseTimer;
  
#ifdef DOC_PLUS_PLUS
  /** pwlib constructs to cope with no response to an outgoing
      message. This is used to handle the output of the
      noResponseTimer
       
  This method runs in a separate thread to the heart of the
  Connection.  It is threadsafe, cause each of the elements in
  Connection (that are touched) are thread safe */
  void OnNoResponseTimeout(PTimer &, INT);
#else
  PDECLARE_NOTIFIER(PTimer, IAX2Processor, OnNoResponseTimeout);
#endif
   
  /** Set the acceptable time (in milliseconds) to wait before giving
      up on this call */
  void StartNoResponseTimer(PINDEX msToWait = 60000);
   
  /** Stop the timer - we have received a reply */
  void StopNoResponseTimer() { noResponseTimer.Stop(); }
   
  /**Set up the acceptable time (in milliseconds) to wait between
     doing status checks. */
  void StartStatusCheckTimer(PINDEX msToWait = 10000 /*!< time between status checks, default = 10 seconds*/);  

#ifdef DOC_PLUS_PLUS
  /**A pwlib callback function to invoke another status check on the
       other endpoint 

       This method runs in a separate thread to the
       heart of the Connection.  It is threadsafe, cause each of the
       elements in Connection (that are touched) are thread safe */
  void OnStatusCheck(PTimer &, INT);
#else
  PDECLARE_NOTIFIER(PTimer, IAX2Processor, OnStatusCheck);
#endif
  
  /**Code to send a PING and a LAGRQ packet to the remote endpoint */
  void DoStatusCheck();

  /**Activate this thread to process all the lists of queued frames */
  void CleanPendingLists() { activate.Signal(); }
  
  /** we have received a message that the remote node is ringing. Now
      wait for the remote user to answer. */
  void RemoteNodeIsRinging();

  /** We have told the remote node that our phone is ringing. they
      have acked this message.  Now, we advise opal that our phone
      "should" be ringing, and does opal want to accept our call. */
  void RingingWasAcked();

  /** We have received an ack message from the remote node. The ack
      message was sent in response to our Answer message. Since the
      Answer message has been acked, we have to regard this call as
      Established();. */
  void AnswerWasAcked();

  /**Action to perform on receiving an ACK packet (which is required
     during call setup phase for receiver */
  IAX2WaitingForAck nextTask;
  
  /**Flag which is used to activate this thread, so all pending tasks/packets are processed */
  PSyncPoint activate;    
  
  /**Flag to indicate, end this thread */
  BOOL       endThread;         

  /**return the flag to indicate if we are handling special packets,
     which are those packets sent to the endpoint (and not related to
     any particular call). */
  BOOL      IsHandlingSpecialPackets() { return specialPackets; };

  /**Flag to indicate we are handing the special packets, which are
     sent to the endpoint,and not related to any particular call. */
  BOOL       specialPackets;

  /**The call token, which uniquely identifies this IAX2Processor, and the associated call */
  SafeString callToken;

  /**Flag to indicate if we are waiting on the first full frame of
     media (voice or video). The arrival of this frame causes the
     IAX2Connection::OnEstablished method to be called. */
  BOOL firstMediaFrame;

  /**Flag to indicate we have to answer this call (i.e. send a
     FullFrameSessionControl::answer packet). */
  BOOL answerCallNow;

  /**Flag to indicate we need to do a status query on the other
     end. this means, send a PING and a LAGRQ packet, which happens
     every 10 seconds. The timer, timeStatusCheck controls when this
     happens */
  BOOL statusCheckOtherEnd;

  /** The timer which is used to do the status check */
  PTimer statusCheckTimer;

  /**Time sent the last frame of audio */
  PINDEX lastSentAudioFrameTime;

  /**The time period, in ms, of each audio frame. It is used when determining the
   * appropriate timestamp to go on a packet. */
  PINDEX audioFrameDuration;

  /**The number of bytes from compressing one frame of audio */
  PINDEX audioCompressedBytes;

  /**A flag to indicate we have yet to send an audio frame to remote
     endpoint. If this is on, then the first audio frame sent is a
     full one.  */
  BOOL audioFramesNotStarted;

  /**If the incoming frame has Information Elements defining remote
     capability, define the list of remote capabilities */
  void CheckForRemoteCapabilities(IAX2FullFrameProtocol *src);

  /**Status of encryption for this processor - by default, no encrytpion */
  IAX2Encryption encryption;
};


#endif // PROCESSOR_H
