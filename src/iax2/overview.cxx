/*
 * overview.cxx
 *
 * Inter Asterisk Exchange 2
 * 
 * documentation overview.
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
 * $Log: overview.cxx,v $
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

/*! \mainpage Console based IAX voip call system.  

\section secOverview Overview

    An application for making voip calls (using the IAX protocol)
    between IAX endpoints.

\version 1.0
\author  Derek J Smithies



	  
\section secArchitecture Architecture


There is one instance of a Receiver class, which listens on port 4569 for incoming IAX
packets. Incoming packets (from all calls) go via the one instance of a Receiver class.

Incoming packets may be used to generate a new IAXConnection class (if
it is a request to open a call).  Incoming packets are then passed on to the
IAXConnection class for handling. Note that all connections listen to the same
Receiver.

There is one instacne of a Transmit class, which sends data out port 4569 to the remote
endpoint. Outgoing packets (from all calls) go via the one instance of a Transmit class.
Note that all connections send data to the same Transmit.


\section secClassListings Available classes

\subsection subsecKeyClasses The following classes are key to the implementation of IAX in the opal library

\li FrameIdValue           - Element of an internal list, used to keep track of incoming FullFrame out sequence numbers.
\li FrameList              - A list of frames, which can be accessed in a thread safe fashion.
\li IAXConnection          - Manage the IAX protocol for one call, and connect to Opal
\li IAXEndPoint            - Manage the IAX protocol specific issues which are common to all calls, and connect to Opal.
\li IAXProcessor           - Separate thread to handle all IAX protocol requests, and transfer media frames.
\li IncomingEthernetFrames - Separate thread to transfer all frames from the Receiver to the appropriate IAXConnection.
\li OpalIAXMediaStream     - Transfer media frames between IAXConnection to Opal.
\li PacketIdList           - A list of FrameIdValue elements, which is used to keep track of incoming FullFrame out sequence numbers
\li Receiver               - Handles the reception of data from the remote node
\li Remote                 - contain information about the remote node. 
\li SafeString             - handle a PString in a thread safe fashion.
\li SafeStrings            - Handle a PStringArray in a thread safe fashion.
\li SoundList              - Maintain a list of the sound packets as read from the microphone.
\li Transmit               - Send IAX packets to the remote node, and handle retransmit issues.
\li WaitingForAck          - A predefined action, that is to carried out (by one particular IAXProcessor) on receiving an ack.


\subsection subsecFrame classes for holding the UDP data about to be sent to (or received from) the network.

\li Frame - the parent class of all frames.
\li MiniFrame - a UDP frame of data for sending voice/audio. Not as large as a FullFrame
\li FullFrame - the parent class of all FullFrame s.
\li FullFrameCng - Transfers Cng (comfort noise generation)
\li FullFrameDtmf
\li FullFrameHtml 
\li FullFrameImage 
\li FullFrameNull 
\li FullFrameProtocol - managing the session (and doesn't really do anything in IAX2)
\li FullFrameSessionControl - carries information about call control, registration, authentication etc.
\li FullFrameText 
\li FullFrameVideo 
\li FullFrameVoice 


\subsection subsecIe  classes for carrying information in the FullFrameSessionControl UDP frames

\li Ie - the parent of all information element types

\subsection subsecIeData classes for the different data types expressed in an Ie
\li IeByte        - Byte of data in data field.
\li IeChar        - character of data in the data field
\li IeDateAndTime - data field contains a 32 bit number with date and time (2 sec resolution)
\li IeInt         - data field contains an integer.
\li IeNone        - there is no data field.
\li IeShort       - data field contains a short
\li IeSockaddrIn  - data field contains a sockaddr_in,  which is address and port
\li IeString      - data field contains a string - there is no zero byte at the end.
\li IeUInt        - data field contains an unsigned int.
\li IeUShort      - data field contains an unsigned short.

\subsection subsecIeAllTypes Classes for each of the  possible Ie types

\li IeAdsicpe 
\li IeAesProvisioning 
\li IeApparentAddr 
\li IeAuthMethods 
\li IeAutoAnswer 
\li IeBlockOfData 
\li IeCallNo 
\li IeCalledContext 
\li IeCalledNumber 
\li IeCallingAni 
\li IeCallingName 
\li IeCallingNumber 
\li IeCallingPres 
\li IeCallingTns 
\li IeCallingTon 
\li IeCapability 
\li IeCause         - text description of what happened (typically used at call completion, explaining why the call was finished)
\li IeCauseCode     - numeric value describing why the call was completed.
\li IeChallenge 
\li IeCodecPrefs 
\li IeData
\li IeDateAndTime
\li IeDateTime       - 32 bit value (2 sec accuracy) of the date and time. Year is in range of 2000-2127.
\li IeDeviceType 
\li IeDnid 
\li IeDpStatus 
\li IeDroppedFrames
\li IeEncKey 
\li IeEncryption 
\li IeFirmwareVer 
\li IeFormat 
\li IeFwBlockData 
\li IeFwBlockDesc 
\li IeIaxUnknown 
\li IeInvalidElement 
\li IeLanguage 
\li IeList 
\li IeMd5Result 
\li IeMsgCount 
\li IeMusicOnHold 
\li IePassword 
\li IeProvVer 
\li IeProvisioning 
\li IeRdnis 
\li IeReceivedDelay
\li IeReceivedFrames
\li IeReceivedJitter
\li IeReceivedLoss
\li IeReceivedOoo
\li IeRefresh 
\li IeRsaResult 
\li IeSamplingRate    - sampling rate in hertz. Typically, the value is 8000
\li IeServiceIdent 
\li IeTransferId 
\li IeUserName         - data field contains the username of the transmitting node
\li IeVersion          - version of IAX in opertion, typically the value is 2




*/
