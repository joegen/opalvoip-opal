/*
 * h323ep.h
 *
 * H.323 protocol handler
 *
 * Open H323 Library
 *
 * Copyright (c) 1998-2001 Equivalence Pty. Ltd.
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
 * The Original Code is Open H323 Library.
 *
 * The Initial Developer of the Original Code is Equivalence Pty. Ltd.
 *
 * Portions of this code were written with the assisance of funding from
 * Vovida Networks, Inc. http://www.vovida.com.
 *
 * Contributor(s): ______________________________________.
 *
 * $Log: h323ep.h,v $
 * Revision 1.2002  2001/07/30 07:22:25  robertj
 * Abstracted listener management from H.323 to OpalEndPoint class.
 *
 * Revision 2.0  2001/07/27 15:48:24  robertj
 * Conversion of OpenH323 to Open Phone Abstraction Library (OPAL)
 *
 * Revision 1.121  2001/07/17 04:44:29  robertj
 * Partial implementation of T.120 and T.38 logical channels.
 *
 * Revision 1.120  2001/07/06 02:29:12  robertj
 * Moved initialisation of local capabilities back to constructor for
 *   backward compatibility reasons.
 *
 * Revision 1.119  2001/07/05 04:18:23  robertj
 * Added call back for setting local capabilities.
 *
 * Revision 1.118  2001/06/19 03:55:28  robertj
 * Added transport to CreateConnection() function so can use that as part of
 *   the connection creation decision making process.
 *
 * Revision 1.117  2001/06/14 23:18:04  robertj
 * Change to allow for CreateConnection() to return NULL to abort call.
 *
 * Revision 1.116  2001/06/14 04:24:16  robertj
 * Changed incoming call to pass setup pdu to endpoint so it can create
 *   different connection subclasses depending on the pdu eg its alias
 *
 * Revision 1.115  2001/06/13 06:38:23  robertj
 * Added early start (media before connect) functionality.
 *
 * Revision 1.114  2001/05/31 01:28:47  robertj
 * Added functions to determine if call currently being held.
 *
 * Revision 1.113  2001/05/30 23:34:54  robertj
 * Added functions to send TCS=0 for transmitter side pause.
 *
 * Revision 1.112  2001/05/17 07:11:29  robertj
 * Added more call end types for common transport failure modes.
 *
 * Revision 1.111  2001/05/17 03:31:07  robertj
 * Fixed support for transmiter side paused (TCS=0), thanks Paul van de Wijngaard
 *
 * Revision 1.110  2001/05/14 05:56:25  robertj
 * Added H323 capability registration system so can add capabilities by
 *   string name instead of having to instantiate explicit classes.
 *
 * Revision 1.109  2001/05/09 04:59:02  robertj
 * Bug fixes in H.450.2, thanks Klein Stefan.
 *
 * Revision 1.108  2001/05/09 04:07:53  robertj
 * Added more call end codes for busy and congested.
 *
 * Revision 1.107  2001/05/01 04:34:10  robertj
 * Changed call transfer API slightly to be consistent with new hold function.
 *
 * Revision 1.106  2001/05/01 02:12:48  robertj
 * Added H.450.4 call hold (Near End only), thanks David M. Cassel.
 *
 * Revision 1.105  2001/04/23 01:31:13  robertj
 * Improved the locking of connections especially at shutdown.
 *
 * Revision 1.104  2001/04/11 03:01:27  robertj
 * Added H.450.2 (call transfer), thanks a LOT to Graeme Reid & Norwood Systems
 *
 * Revision 1.103  2001/03/21 04:52:40  robertj
 * Added H.235 security to gatekeepers, thanks Fürbass Franz!
 *
 * Revision 1.102  2001/03/16 06:46:19  robertj
 * Added ability to set endpoints desired time to live on a gatekeeper.
 *
 * Revision 1.101  2001/03/15 00:24:01  robertj
 * Added function for setting gatekeeper with address and identifier values.
 *
 * Revision 1.100  2001/03/08 07:45:04  robertj
 * Fixed issues with getting media channels started in some early start
 *   regimes, in particular better Cisco compatibility.
 *
 * Revision 1.99  2001/03/02 06:59:57  robertj
 * Enhanced the globally unique identifier class.
 *
 * Revision 1.98  2001/02/16 04:11:46  robertj
 * Added ability for RemoveListener() to remove all listeners.
 *
 * Revision 1.97  2001/02/09 05:16:24  robertj
 * Added #pragma interface for GNU C++.
 *
 * Revision 1.96  2000/12/19 22:33:44  dereks
 * Adjust so that the video channel is used for reading/writing raw video
 * data, which better modularizes the video codec.
 *
 * Revision 1.95  2000/12/18 08:58:30  craigs
 * Added ability set ports
 *
 * Revision 1.94  2000/12/18 01:22:28  robertj
 * Changed semantics or HasConnection() so returns TRUE until the connection
 *   has been deleted and not just until ClearCall() was executure on it.
 *
 * Revision 1.93  2000/12/05 01:52:00  craigs
 * Made ClearCall functions virtual to allow overiding
 *
 * Revision 1.92  2000/11/27 02:44:06  craigs
 * Added ClearCall Synchronous to H323Connection and H323Endpoint to
 * avoid race conditions with destroying descendant classes
 *
 * Revision 1.91  2000/11/26 23:13:23  craigs
 * Added ability to pass user data to H323Connection constructor
 *
 * Revision 1.90  2000/11/12 23:49:16  craigs
 * Added per connection versions of OnEstablished and OnCleared
 *
 * Revision 1.89  2000/11/08 04:30:00  robertj
 * Added function to be able to alter/remove the call proceeding PDU.
 *
 * Revision 1.88  2000/10/20 06:10:20  robertj
 * Fixed very small race condition on creating new connectionon incoming call.
 *
 * Revision 1.87  2000/10/19 04:06:54  robertj
 * Added function to be able to remove a listener.
 *
 * Revision 1.86  2000/10/16 09:51:16  robertj
 * Fixed problem with not opening fast start video receive if do not have transmit enabled.
 *
 * Revision 1.85  2000/10/13 02:15:23  robertj
 * Added support for Progress Indicator Q.931/H.225 message.
 *
 * Revision 1.84  2000/10/04 12:20:50  robertj
 * Changed setting of callToken in H323Connection to be as early as possible.
 *
 * Revision 1.83  2000/09/25 12:59:16  robertj
 * Added StartListener() function that takes a H323TransportAddress to start
 *     listeners bound to specific interfaces.
 *
 * Revision 1.82  2000/09/22 01:35:02  robertj
 * Added support for handling LID's that only do symmetric codecs.
 *
 * Revision 1.81  2000/09/20 01:50:22  craigs
 * Added ability to set jitter buffer on a per-connection basis
 *
 * Revision 1.80  2000/09/05 01:16:19  robertj
 * Added "security" call end reason code.
 *
 * Revision 1.79  2000/09/01 02:12:54  robertj
 * Added ability to select a gatekeeper on LAN via it's identifier name.
 *
 * Revision 1.78  2000/07/31 14:08:09  robertj
 * Added fast start and H.245 tunneling flags to the H323Connection constructor so can
 *    disabled these features in easier manner to overriding virtuals.
 *
 * Revision 1.77  2000/07/13 12:33:38  robertj
 * Split autoStartVideo so can select receive and transmit independently
 *
 * Revision 1.76  2000/07/04 04:14:06  robertj
 * Fixed capability check of "combinations" for fast start cases.
 *
 * Revision 1.75  2000/07/04 01:16:49  robertj
 * Added check for capability allowed in "combinations" set, still needs more done yet.
 *
 * Revision 1.74  2000/06/29 10:59:53  robertj
 * Added user interface for sound buffer depth adjustment.
 *
 * Revision 1.73  2000/06/23 02:48:23  robertj
 * Added ability to adjust sound channel buffer depth, needed increasing under Win32.
 *
 * Revision 1.72  2000/06/21 08:07:38  robertj
 * Added cause/reason to release complete PDU, where relevent.
 *
 * Revision 1.71  2000/06/10 02:03:36  robertj
 * Fixed typo in comment
 *
 * Revision 1.70  2000/06/07 05:47:55  robertj
 * Added call forwarding.
 *
 * Revision 1.69  2000/06/05 06:33:08  robertj
 * Fixed problem with roud trip time statistic not being calculated if constant traffic.
 *
 * Revision 1.68  2000/05/23 12:57:28  robertj
 * Added ability to change IP Type Of Service code from applications.
 *
 * Revision 1.67  2000/05/23 11:32:26  robertj
 * Rewrite of capability table to combine 2 structures into one and move functionality into that class
 *    allowing some normalisation of usage across several applications.
 * Changed H323Connection so gets a copy of capabilities instead of using endponts, allows adjustments
 *    to be done depending on the remote client application.
 *
 * Revision 1.66  2000/05/18 11:53:33  robertj
 * Changes to support doc++ documentation generation.
 *
 * Revision 1.65  2000/05/16 08:12:37  robertj
 * Added documentation for FindChannel() function.
 * Added function to get a logical channel by channel number.
 *
 * Revision 1.64  2000/05/16 02:06:00  craigs
 * Added access functions for particular sessions
 *
 * Revision 1.63  2000/05/09 12:19:22  robertj
 * Added ability to get and set "distinctive ring" Q.931 functionality.
 *
 * Revision 1.62  2000/05/08 14:07:26  robertj
 * Improved the provision and detection of calling and caller numbers, aliases and hostnames.
 *
 * Revision 1.61  2000/05/02 04:32:24  robertj
 * Fixed copyright notice comment.
 *
 * Revision 1.60  2000/05/01 13:00:09  robertj
 * Changed SetCapability() to append capabilities to TCS, helps with assuring no gaps in set.
 *
 * Revision 1.59  2000/04/14 21:08:53  robertj
 * Work around for compatibility problem wth broken Altigen AltaServ-IG PBX.
 *
 * Revision 1.58  2000/04/14 20:01:38  robertj
 * Added function to get remote endpoints application name.
 *
 * Revision 1.57  2000/04/11 04:02:47  robertj
 * Improved call initiation with gatekeeper, no longer require @address as
 *    will default to gk alias if no @ and registered with gk.
 * Added new call end reasons for gatekeeper denied calls.
 *
 * Revision 1.56  2000/04/10 20:02:49  robertj
 * Added support for more sophisticated DTMF and hook flash user indication.
 *
 * Revision 1.55  2000/04/06 17:50:15  robertj
 * Added auto-start (including fast start) of video channels, selectable via boolean on the endpoint.
 *
 * Revision 1.54  2000/04/05 03:17:30  robertj
 * Added more RTP statistics gathering and H.245 round trip delay calculation.
 *
 * Revision 1.53  2000/03/29 04:32:55  robertj
 * Improved some trace logging messages.
 *
 * Revision 1.52  2000/03/29 02:12:38  robertj
 * Changed TerminationReason to CallEndReason to use correct telephony nomenclature.
 * Added CallEndReason for capability exchange failure.
 *
 * Revision 1.51  2000/03/25 02:03:18  robertj
 * Added default transport for gatekeeper to be UDP.
 *
 * Revision 1.50  2000/03/23 02:44:49  robertj
 * Changed ClearAllCalls() so will wait for calls to be closed (usefull in endpoint dtors).
 *
 * Revision 1.49  2000/03/02 02:18:13  robertj
 * Further fixes for early H245 establishment confusing the fast start code.
 *
 * Revision 1.48  2000/01/07 08:19:14  robertj
 * Added status functions for connection and tidied up the answer call function
 *
 * Revision 1.47  2000/01/04 00:14:26  craigs
 * Added additional states to AnswerCall callback
 *
 * Revision 1.46  1999/12/23 23:02:34  robertj
 * File reorganision for separating RTP from H.323 and creation of LID for VPB support.
 *
 * Revision 1.45  1999/12/11 02:20:53  robertj
 * Added ability to have multiple aliases on local endpoint.
 *
 * Revision 1.44  1999/11/22 10:07:23  robertj
 * Fixed some errors in correct termination states.
 *
 * Revision 1.43  1999/11/19 08:15:41  craigs
 * Added connectionStartTime
 *
 * Revision 1.42  1999/11/17 00:01:11  robertj
 * Improved determination of caller name, thanks Ian MacDonald
 *
 * Revision 1.41  1999/11/10 23:29:37  robertj
 * Changed OnAnswerCall() call back function  to allow for asyncronous response.
 *
 * Revision 1.40  1999/11/06 05:37:44  robertj
 * Complete rewrite of termination of connection to avoid numerous race conditions.
 *
 * Revision 1.39  1999/10/30 12:34:13  robertj
 * Added information callback for closed logical channel on H323EndPoint.
 *
 * Revision 1.38  1999/10/29 02:26:13  robertj
 * Added reason for termination code to H323Connection.
 *
 * Revision 1.37  1999/10/19 00:03:20  robertj
 * Changed OpenAudioChannel and OpenVideoChannel to allow a codec AttachChannel with no autodelete.
 * Added function to set initial bandwidth limit on a new connection.
 *
 * Revision 1.36  1999/10/14 12:05:03  robertj
 * Fixed deadlock possibilities in clearing calls.
 *
 * Revision 1.35  1999/10/09 01:19:07  craigs
 * Added codecs to OpenAudioChannel and OpenVideoDevice functions
 *
 * Revision 1.34  1999/09/23 07:25:12  robertj
 * Added open audio and video function to connection and started multi-frame codec send functionality.
 *
 * Revision 1.33  1999/09/21 14:24:34  robertj
 * Changed SetCapability() so automatically calls AddCapability().
 *
 * Revision 1.32  1999/09/21 14:05:21  robertj
 * Fixed incorrect PTRACING test and removed uneeded include of videoio.h
 *
 * Revision 1.31  1999/09/21 08:29:13  craigs
 * Added support for video codecs and H261
 *
 * Revision 1.30  1999/09/15 01:26:27  robertj
 * Changed capability set call backs to have more specific class as parameter.
 *
 * Revision 1.29  1999/09/14 06:52:54  robertj
 * Added better support for multi-homed client hosts.
 *
 * Revision 1.28  1999/09/13 14:23:11  robertj
 * Changed MakeCall() function return value to be something useful.
 *
 * Revision 1.27  1999/09/10 03:36:47  robertj
 * Added simple Q.931 Status response to Q.931 Status Enquiry
 *
 * Revision 1.26  1999/09/08 04:05:48  robertj
 * Added support for video capabilities & codec, still needs the actual codec itself!
 *
 * Revision 1.25  1999/08/31 12:34:18  robertj
 * Added gatekeeper support.
 *
 * Revision 1.24  1999/08/27 09:46:05  robertj
 * Added sepearte function to initialise vendor information from endpoint.
 *
 * Revision 1.23  1999/08/25 05:14:49  robertj
 * File fission (critical mass reached).
 * Improved way in which remote capabilities are created, removed case statement!
 * Changed MakeCall, so immediately spawns thread, no black on TCP connect.
 *
 * Revision 1.22  1999/08/08 10:02:49  robertj
 * Added access functions to remote capability table.
 *
 * Revision 1.21  1999/07/23 02:37:53  robertj
 * Fixed problems with hang ups and crash closes of connections.
 *
 * Revision 1.20  1999/07/18 14:29:31  robertj
 * Fixed bugs in slow start with H245 tunnelling, part 3.
 *
 * Revision 1.19  1999/07/16 14:05:10  robertj
 * Added application level jitter buffer adjustment.
 *
 * Revision 1.18  1999/07/16 06:15:59  robertj
 * Corrected semantics for tunnelled master/slave determination in fast start.
 *
 * Revision 1.17  1999/07/15 14:45:35  robertj
 * Added propagation of codec open error to shut down logical channel.
 * Fixed control channel start up bug introduced with tunnelling.
 *
 * Revision 1.16  1999/07/14 06:06:13  robertj
 * Fixed termination problems (race conditions) with deleting connection object.
 *
 * Revision 1.15  1999/07/13 02:50:58  craigs
 * Changed semantics of SetPlayDevice/SetRecordDevice, only descendent
 *    endpoint assumes PSoundChannel devices for audio codec.
 *
 * Revision 1.14  1999/07/10 02:59:26  robertj
 * Fixed ability to hang up incoming connection.
 *
 * Revision 1.13  1999/07/09 06:09:49  robertj
 * Major implementation. An ENORMOUS amount of stuff added everywhere.
 *
 * Revision 1.12  1999/06/25 10:25:35  robertj
 * Added maintentance of callIdentifier variable in H.225 channel.
 *
 * Revision 1.11  1999/06/22 13:45:05  robertj
 * Added user question on listener version to accept incoming calls.
 *
 * Revision 1.10  1999/06/14 05:15:56  robertj
 * Changes for using RTP sessions correctly in H323 Logical Channel context
 *
 * Revision 1.9  1999/06/13 12:41:14  robertj
 * Implement logical channel transmitter.
 * Fixed H245 connect on receiving call.
 *
 * Revision 1.8  1999/06/09 06:18:00  robertj
 * GCC compatibiltiy.
 *
 * Revision 1.7  1999/06/09 05:26:19  robertj
 * Major restructuring of classes.
 *
 * Revision 1.6  1999/06/06 06:06:36  robertj
 * Changes for new ASN compiler and v2 protocol ASN files.
 *
 * Revision 1.5  1999/04/26 06:14:46  craigs
 * Initial implementation for RTP decoding and lots of stuff
 * As a whole, these changes are called "First Noise"
 *
 * Revision 1.4  1999/02/23 11:04:28  robertj
 * Added capability to make outgoing call.
 *
 * Revision 1.3  1999/01/16 01:31:34  robertj
 * Major implementation.
 *
 * Revision 1.2  1999/01/02 04:00:59  robertj
 * Added higher level protocol negotiations.
 *
 * Revision 1.1  1998/12/14 09:13:12  robertj
 * Initial revision
 *
 */

#ifndef __H323_H323EP_H
#define __H323_H323EP_H

#ifdef __GNUC__
#pragma interface
#endif


#include <opal/endpoint.h>
#include <opal/manager.h>
#include <opal/transports.h>
#include <h323/h323con.h>
#include <h323/h323caps.h>


class H225_EndpointType;
class H225_VendorIdentifier;
class H225_H221NonStandard;

class H235SecurityInfo;

class H323Gatekeeper;
class H323SignalPDU;

class OpalT120Protocol;
class OpalT38Protocol;


///////////////////////////////////////////////////////////////////////////////

/**This class manages the H323 endpoint.
   An endpoint may have zero or more listeners to create incoming connections
   or zero or more outgoing conenctions initiated via the MakeCall() function.
   Once a conection exists it is managed by this class instance.

   The main thing this class embodies is the capabilities of the application,
   that is the codecs and protocols it is capable of.

   An application may create a descendent off this class and overide the
   CreateConnection() function, if they require a descendent of H323Connection
   to be created. This would be quite likely in most applications.
 */
class H323EndPoint : public OpalEndPoint
{
  PCLASSINFO(H323EndPoint, OpalEndPoint);

  public:
    enum {
      DefaultSignalTcpPort = 1720
    };

  /**@name Construction */
  //@{
    /**Create a new endpoint.
     */
    H323EndPoint(
      OpalManager & manager
    );

    /**Destroy endpoint.
     */
    ~H323EndPoint();
  //@}

  /**@name Overrides from OpalEndPoint */
  //@{
    /**Set up a connection to a remote party.
       This is called from the OpalManager::SetUpConnection() function once
       it has determined that this is the endpoint for the protocol.

       The general form for this party parameter is:

            [proto:][alias@][transport$]address[:port]

       where the various fields will have meanings specific to the endpoint
       type. For example, with H.323 it could be "h323:Fred@site.com" which
       indicates a user Fred at gatekeeper size.com. Whereas for the PSTN
       endpoint it could be "pstn:5551234" which is to call 5551234 on the
       first available PSTN line.

       The proto field is optional when passed to a specific endpoint. If it
       is present, however, it must agree with the endpoints protocol name or
       NULL is returned.

       This function returns almost immediately with the connection continuing
       to occur in a new background thread.

       If NULL is returned then the connection could not be established. For
       example if a PSTN endpoint is used and the assiciated line is engaged
       then it may return immediately. Returning a non-NULL value does not
       mean that the connection will succeed, only that an attempt is being
       made.

       Note, the returned pointer to the connection is not locked and may be
       deleted at any time. This is extremely unlikely immediately after the
       function is called, but you should not keep this pointer beyond that
       brief time. The the FindCallWithLock() function for future references
       to the connection object.

       The default behaviour is pure.
     */
    virtual OpalConnection * SetUpConnection(
      OpalCall & call,        /// Owner of connection
      const PString & party,  /// Remote party to call
      void * userData = NULL  /// Arbitrary data to pass to connection
    );

    /**Get the data formats this endpoint is capable of operating in.
       This provides a list of media data format names that an
       OpalMediaStream may be created in within connections created by this
       endpoint.
      */
    virtual OpalMediaFormat::List GetMediaFormats() const;
  //@}

  /**@name Set up functions */
  //@{
    /**Set the endpoint information in H225 PDU's.
      */
    virtual void SetEndpointTypeInfo(
      H225_EndpointType & info
    ) const;

    /**Set the vendor information in H225 PDU's.
      */
    virtual void SetVendorIdentifierInfo(
      H225_VendorIdentifier & info
    ) const;

    /**Set the H221NonStandard information in H225 PDU's.
      */
    virtual void SetH221NonStandardInfo(
      H225_H221NonStandard & info
    ) const;

    /**Get a H.235 security information for the endpoint.
       This will gather infromation required to provide secured RAS Messages
       to a gatekeeper.

       It is expected that this function return a pointer to a member variable
       or similarly scoped variable. The pointer will not be deleted by the
       user so it should not be allocated on the heap unless it is memory
       managed by the endpoint descendant.

       The default behaviour returns NULL.
      */
    virtual H235SecurityInfo * GetSecurityInfo();
  //@}


  /**@name Capabilities */
  //@{
    /**Add a codec to the capabilities table. This will assure that the
       assignedCapabilityNumber field in the codec is unique for all codecs
       installed on this endpoint.

       If the specific instnace of the capability is already in the table, it
       is not added again. Ther can be multiple instances of the same
       capability class however.
     */
    void AddCapability(
      H323Capability * capability   /// New codec specification
    );

    /**Set the capability descriptor lists. This is three tier set of
       codecs. The top most level is a list of particular capabilities. Each
       of these consists of a list of alternatives that can operate
       simultaneously. The lowest level is a list of codecs that cannot
       operate together. See H323 section 6.2.8.1 and H245 section 7.2 for
       details.

       If descriptorNum is P_MAX_INDEX, the the next available index in the
       array of descriptors is used. Similarly if simultaneous is P_MAX_INDEX
       the the next available SimultaneousCapabilitySet is used. The return
       value is the index used for the new entry. Note if both are P_MAX_INDEX
       then the return value is the descriptor index as the simultaneous index
       must be zero.

       Note that the capability specified here is automatically added to the
       capability table using the AddCapability() function. A specific
       instance of a capability is only ever added once, so multiple
       SetCapability() calls with the same H323Capability pointer will only
       add that capability once.
     */
    PINDEX SetCapability(
      PINDEX descriptorNum, /// The member of the capabilityDescriptor to add
      PINDEX simultaneous,  /// The member of the SimultaneousCapabilitySet to add
      H323Capability * cap  /// New capability specification
    );

    /**Add all matching capabilities in list.
       All capabilities that match the specified name are added. See the
       capabilities code for details on the matching algorithm.
      */
    PINDEX AddAllCapabilities(
      PINDEX descriptorNum, /// The member of the capabilityDescriptor to add
      PINDEX simultaneous,  /// The member of the SimultaneousCapabilitySet to add
      const PString & name  /// New capabilities name, if using "known" one.
    );

    /**Add all user input capabilities to this endpoints capability table.
      */
    void AddAllUserInputCapabilities(
      PINDEX descriptorNum, /// The member of the capabilityDescriptor to add
      PINDEX simultaneous   /// The member of the SimultaneousCapabilitySet to add
    );

    /**Remove capabilites in table.
      */
    void RemoveCapabilities(
      const PStringArray & codecNames
    );

    /**Reorder capabilites in table.
      */
    void ReorderCapabilities(
      const PStringArray & preferenceOrder
    );

    /**Find a capability that has been registered.
     */
    H323Capability * FindCapability(
      const H245_Capability & cap  /// H245 capability table entry
    ) const;

    /**Find a capability that has been registered.
     */
    H323Capability * FindCapability(
      const H245_DataType & dataType  /// H245 data type of codec
    ) const;

    /**Find a capability that has been registered.
     */
    H323Capability * FindCapability(
      H323Capability::MainTypes mainType,   /// Main type of codec
      unsigned subType                      /// Subtype of codec
    ) const;
  //@}

  /**@name Gatekeeper management */
  //@{
    /**Select and register with an explicit gatekeeper.
       This will use the specified transport and a string giving a transport
       dependent address to locate a specific gatekeeper. The endpoint will
       register with that gatekeeper and, if successful, set it as the current
       gatekeeper used by this endpoint.

       Note the transport being passed in will be deleted by this function or
       the H323Gatekeeper object it becomes associated with. Also if transport
       is NULL then a OpalTransportUDP is created.
     */
    BOOL SetGatekeeper(
      const PString & address,          /// Address of gatekeeper to use.
      OpalTransport * transport = NULL  /// Transport over which to talk to gatekeeper.
    );

    /**Select and register with an explicit gatekeeper and zone.
       This will use the specified transport and a string giving a transport
       dependent address to locate a specific gatekeeper. The endpoint will
       register with that gatekeeper and, if successful, set it as the current
       gatekeeper used by this endpoint.

       The gatekeeper identifier is set to the spplied parameter to allow the
       gatekeeper to either allocate a zone or sub-zone, or refuse to register
       if the zones do not match.

       Note the transport being passed in will be deleted by this function or
       the H323Gatekeeper object it becomes associated with. Also if transport
       is NULL then a OpalTransportUDP is created.
     */
    BOOL SetGatekeeperZone(
      const PString & address,          /// Address of gatekeeper to use.
      const PString & identifier,       /// Identifier of gatekeeper to use.
      OpalTransport * transport = NULL  /// Transport over which to talk to gatekeeper.
    );

    /**Locate and select gatekeeper.
       This function will use the automatic gatekeeper discovery methods to
       locate the gatekeeper on the particular transport that has the specified
       gatekeeper identifier name. This is often the "Zone" for the gatekeeper.

       Note the transport being passed in will be deleted becomes owned by the
       H323Gatekeeper created by this function and will be deleted by it. Also
       if transport is NULL then a OpalTransportUDP is created.
     */
    BOOL LocateGatekeeper(
      const PString & identifier,       /// Identifier of gatekeeper to locate.
      OpalTransport * transport = NULL  /// Transport over which to talk to gatekeeper.
    );

    /**Discover and select gatekeeper.
       This function will use the automatic gatekeeper discovery methods to
       locate the first gatekeeper on a particular transport.

       Note the transport being passed in will be deleted becomes owned by the
       H323Gatekeeper created by this function and will be deleted by it. Also
       if transport is NULL then a OpalTransportUDP is created.
     */
    BOOL DiscoverGatekeeper(
      OpalTransport * transport = NULL  /// Transport over which to talk to gatekeeper.
    );

    /**Create a gatekeeper.
       This allows the application writer to have the gatekeeper as a
       descendent of the H323Gatekeeper in order to add functionality to the
       base capabilities in the library.

       The default creates an instance of the H323Gatekeeper class.
     */
    virtual H323Gatekeeper * CreateGatekeeper(
      OpalTransport * transport  /// Transport over which gatekeepers communicates.
    );

    /**Get the gatekeeper we are registered with.
     */
    H323Gatekeeper * GetGatekeeper() const { return gatekeeper; }

    /**Unregister and delete the gatekeeper we are registered with.
       The return value indicates FALSE if there was an error during the
       unregistration. However the gatekeeper is still removed and its
       instance deleted regardless of this error.
     */
    BOOL RemoveGatekeeper(
      int reason = -1    /// Reason for gatekeeper removal
    );
  //@}

  /**@name Connection management */
  //@{
    /**Handle new incoming connetion from listener.
      */
    virtual void NewIncomingConnection(
      OpalTransport * transport  /// Transport connection came in on
    );

    /**Create a connection that uses the specified call.
      */
    virtual H323Connection * CreateConnection(
      OpalCall & call,           /// Call object to attach the connection to
      const PString & token,     /// Call token for new connection
      void * userData,           /// Arbitrary user data from SetUpConnection
      OpalTransport * transport, /// Transport connection came in on
      H323SignalPDU * setupPDU   /// Setup PDU for incoming call
    );

    /**Initiate the transfer of an existing call (connection) to a new remote party
       using H.450.2.  This sends a Call Transfer Initiate Invoke message from the
       A-Party (transferring endpoint) to the B-Party (transferred endpoint).
     */
    void TransferCall(
      const PString & token,        /// Existing connection to be transferred
      const PString & remoteParty   /// Remote party to transfer the existing call to
    );

    /**Place the call on hold, suspending all media channels (H.450.4)
    * NOTE: Only Local Hold is implemented so far. 
    */
    void HoldCall(
      const PString & token,        /// Existing connection to be transferred
      BOOL localHold   /// true for Local Hold, false for Remote Hold
    );

    /**Setup the transfer of an existing call (connection) to a new remote party
       using H.450.2.  This sends a Call Transfer Setup Invoke message from the
       B-Party (transferred endpoint) to the C-Party (transferred-to endpoint).

       If the transport parameter is NULL the transport is determined from the
       remoteParty description. The general form for this parameter is
       [alias@][transport$]host[:port] where the default alias is the same as
       the host, the default transport is "ip" and the default port is 1720.

       This function returns almost immediately with the transfer occurring in a
       new background thread.

       Note, the returned pointer to the connection is not locked and may be
       deleted at any time. This is extremely unlikely immediately after the
       function is called, but you should not keep this pointer beyond that
       brief time. The the FindConnectionWithLock() function for future
       references to the connection object.

       This function is declared virtual to allow an application to override
       the function and get the new call token of the forwarded call.
     */
    virtual H323Connection * SetupTransfer(
      const PString & token,        /// Existing connection to be transferred
      const PString & callIdentity, /// Call identity of the secondary call (if it exists)
      const PString & remoteParty,  /// Remote party to transfer the existing call to
      PString & newToken            /// String to receive token for the new connection
    );

    /**Parse a party address into alias and transport components.
       An appropriate transport is determined from the remoteParty parameter.
       The general form for this parameter is [alias@][transport$]host[:port]
       where the default alias is the same as the host, the default transport
       is "ip" and the default port is 1720.
      */
    void ParsePartyName(
      const PString & party,          /// Party name string.
      PString & alias,                /// Parsed alias name
      OpalTransportAddress & address  /// Parsed transport address
    ) const;

    /**Find a connection that uses the specified token.
       This searches the endpoint for the connection that contains the token
       as provided by functions such as MakeCall(). if not found it will then search for
       the string representation of the CallIdentifier for the connection, and
       finally try for the string representation of the ConferenceIdentifier.

       Note the caller of this function MUSt call the H323Connection::Unlock()
       function if this function returns a non-NULL pointer. If it does not
       then a deadlock can occur.
      */
    H323Connection * FindConnectionWithLock(
      const PString & token     /// Token to identify connection
    );

    /**Call back for incoming call.
       This function is called from the OnReceivedSignalSetup() function
       before it sends the Alerting PDU. It gives an opportunity for an
       application to alter the reply before transmission to the other
       endpoint.

       If FALSE is returned the connection is aborted and a Release Complete
       PDU is sent.

       The default behaviour simply returns TRUE.
     */
    virtual BOOL OnIncomingCall(
      H323Connection & connection,    /// Connection that was established
      const H323SignalPDU & setupPDU,   /// Received setup PDU
      H323SignalPDU & alertingPDU       /// Alerting PDU to send
    );

    /**Call back for answering an incoming call.
       This function is called from the OnReceivedSignalSetup() function
       before it sends the Connect PDU. It gives an opportunity for an
       application to alter the reply before transmission to the other
       endpoint.

       It also gives an application time to wait for some event before
       signalling to the endpoint that the connection is to proceed. For
       example the user pressing an "Answer call" button.

       If AnswerCallDenied is returned the connection is aborted and a Release
       Complete PDU is sent. If AnswerCallNow is returned then the H.323
       protocol proceeds. Finally if AnswerCallPending is returned then the
       protocol negotiations are paused until the AnsweringCall() function is
       called.

       The default behaviour simply returns AnswerNow.
     */
    virtual H323Connection::AnswerCallResponse OnAnswerCall(
      H323Connection & connection,    /// Connection that was established
      const PString & callerName,       /// Name of caller
      const H323SignalPDU & setupPDU,   /// Received setup PDU
      H323SignalPDU & connectPDU        /// Connect PDU to send. 
    );

    /**Call back for remote party being alerted.
       This function is called from the SendSignalSetup() function after it
       receives the optional Alerting PDU from the remote endpoint. That is
       when the remote "phone" is "ringing".

       If FALSE is returned the connection is aborted and a Release Complete
       PDU is sent.

       The default behaviour simply returns TRUE.
     */
    virtual BOOL OnAlerting(
      H323Connection & connection,    /// Connection that was established
      const H323SignalPDU & alertingPDU,  /// Received Alerting PDU
      const PString & user                /// Username of remote endpoint
    );

    /**A call back function whenever connection indicates it should be
       forwarded.
       Return TRUE if the call is being redirected.

       The default behaviour simply returns FALSE.
      */
    virtual BOOL OnConnectionForwarded(
      H323Connection & connection,    /// Connection to be forwarded
      const PString & forwardParty,   /// Remote party to forward to
      const H323SignalPDU & pdu       /// Full PDU initiating forwarding
    );

    /**A call back function whenever a connection is established.
       This indicates that a connection to a remote endpoint was established
       with a control channel and zero or more logical channels.

       The default behaviour does nothing.
      */
    virtual void OnConnectionEstablished(
      H323Connection & connection,    /// Connection that was established
      const PString & token           /// Token for identifying connection
    );

    /**Determine if a connection is established.
      */
    virtual BOOL IsConnectionEstablished(
      const PString & token   /// Token for identifying connection
    );

    /**A call back function whenever a connection is broken.
       This indicates that a connection to a remote endpoint is no longer
       available.

       The default behaviour does nothing.
      */
    virtual void OnConnectionCleared(
      H323Connection & connection,    /// Connection that was established
      const PString & token           /// Token for identifying connection
    );
  //@}


  /**@name Supplementary Service management */
  //@{
    /**Handle an incoming Call Transfer Initiate operation.
     */
    virtual BOOL OnCallTransferInitiate(
      H323Connection & exisitngConnection,    /// Existing connection to be transferred
      const PString & remoteParty             /// Destination address of the transferred-to party
    );

    /**Handle an incoming Call Transfer Setup operation.
     */
    virtual BOOL OnCallTransferSetup(
      H323Connection & connection,            /// Existing connection to be transferred
      int invokeId,                           /// InvokeId of operation (used in response)
      int linkedId,                           /// InvokeId of associated operation (if any)
      const PString & callIdentity            /// Call identity of the secondary call (if it exists)
    );

    /**Handle a Call Transfer Initiate Return Result.
     */
    virtual void OnCallTransferInitiateReturnResult(
      H323Connection & connection             /// Existing connection being transferred
    );

    /**Handle a Call Transfer Setup Return Result.
     */
    virtual void OnCallTransferSetupReturnResult(
      H323Connection & connection             /// Existing connection being transferred
    );

    /**Handle a Call Transfer Initiate Return Error
     */
    virtual void OnCallTransferInitiateReturnError(
      H323Connection & connection,            /// Existing connection being transferred
      int errorCode                           /// Error code for the invoke operation
    );

    /**Handle a Call Transfer Setup Return Error
     */
    virtual void OnCallTransferSetupReturnError(
      H323Connection & connection,            /// Existing connection being transferred
      int errorCode                           /// Error code for the invoke operation
    );

    /**Handle a generic X880 Reject
     */
    virtual void OnReject(
      H323Connection & connection,            /// Connection for the X880 operation
      int invokeId,                           /// InvokeId of invoke operation
      int problem                             /// Reject problem for the operation
    );
  //@}


  /**@name Logical Channels management */
  //@{
    /**Call back for opening a logical channel.

       The default behaviour simply returns TRUE.
      */
    virtual BOOL OnStartLogicalChannel(
      H323Connection & connection,    /// Connection for the channel
      H323Channel & channel           /// Channel being started
    );

    /**Call back for closed a logical channel.

       The default behaviour does nothing.
      */
    virtual void OnClosedLogicalChannel(
      H323Connection & connection,    /// Connection for the channel
      const H323Channel & channel     /// Channel being started
    );

    /**Callback from the RTP session for statistics monitoring.
       This is called every so many packets on the transmitter and receiver
       threads of the RTP session indicating that the statistics have been
       updated.

       The default behaviour does nothing.
      */
    virtual void OnRTPStatistics(
      const H323Connection & connection,  /// Connection for the channel
      const RTP_Session & session         /// Session with statistics
    ) const;
  //@}

  /**@name Indications */
  //@{
    /**Call back for remote enpoint has sent user input as a string.

       The default behaviour does nothing.
      */
    virtual void OnUserInputString(
      H323Connection & connection,  /// Connection for the input
      const PString & value         /// String value of indication
    );

    /**Call back for remote enpoint has sent user input.

       The default behaviour calls H323Connection::OnUserInputTone().
      */
    virtual void OnUserInputTone(
      H323Connection & connection,  /// Connection for the input
      char tone,                    /// DTMF tone code
      unsigned duration,            /// Duration of tone in milliseconds
      unsigned logicalChannel,      /// Logical channel number for RTP sync.
      unsigned rtpTimestamp         /// RTP timestamp in logical channel sync.
    );
  //@}

  /**@name Other services */
  //@{
    /**Create an instance of the T.120 protocol handler.
       This is called when the OpenLogicalChannel subsystem requires that
       a T.120 channel be established.

       The default behavour returns NULL.
      */
    virtual OpalT120Protocol * CreateT120ProtocolHandler() const;

    /**Create an instance of the T.38 protocol handler.
       This is called when the OpenLogicalChannel subsystem requires that
       a T.38 fax channel be established.

       The default behavour returns NULL.
      */
    virtual OpalT38Protocol * CreateT38ProtocolHandler() const;
  //@}

  /**@name Member variable access */
  //@{
    /**Set the user name to be used for the local end of any connections. This
       defaults to the logged in user as obtained from the
       PProcess::GetUserName() function.

       Note that this name is technically the first alias for the endpoint.
       Additional aliases may be added by the use of the AddAliasName()
       function, however that list will be cleared when this function is used.
     */
    void SetLocalUserName(
      const PString & name  /// Local name of endpoint (prime alias)
    );

    /**Get the user name to be used for the local end of any connections. This
       defaults to the logged in user as obtained from the
       PProcess::GetUserName() function.
     */
    const PString & GetLocalUserName() const { return localAliasNames[0]; }

    /**Add an alias name to be used for the local end of any connections. If
       the alias name already exists in the list then is is not added again.

       The list defaults to the value set in the SetLocalUserName() function.
       Note that calling SetLocalUserName() will clear the alias list.
     */
    BOOL AddAliasName(
      const PString & name  /// New alias name to add
    );

    /**Remove an alias name used for the local end of any connections. 
       defaults to an empty list.
     */
    BOOL RemoveAliasName(
      const PString & name  /// New alias namer to add
    );

    /**Get the user name to be used for the local end of any connections. This
       defaults to the logged in user as obtained from the
       PProcess::GetUserName() function.
     */
    const PStringList & GetAliasNames() const { return localAliasNames; }

    /**See if should auto-start receive video channels on connection.
     */
    BOOL CanAutoStartReceiveVideo() const { return manager.CanAutoStartReceiveVideo(); }

    /**See if should auto-start transmit video channels on connection.
     */
    BOOL CanAutoStartTransmitVideo() const { return manager.CanAutoStartTransmitVideo(); }

    /**Get the current capability table for this endpoint.
     */
    const H323Capabilities & GetCapabilities() const { return capabilities; }

    /**Endpoint types.
     */
    enum TerminalTypes {
      e_TerminalOnly = 50,
      e_TerminalAndMC = 70,
      e_GatewayOnly = 60,
      e_GatewayAndMC = 80,
      e_GatewayAndMCWithDataMP = 90,
      e_GatewayAndMCWithAudioMP = 100,
      e_GatewayAndMCWithAVMP = 110,
      e_GatekeeperOnly = 120,
      e_GatekeeperWithDataMP = 130,
      e_GatekeeperWithAudioMP = 140,
      e_GatekeeperWithAVMP = 150,
      e_MCUOnly = 160,
      e_MCUWithDataMP = 170,
      e_MCUWithAudioMP = 180,
      e_MCUWithAVMP = 190
    };

    /**Get the endpoint terminal type.
     */
    TerminalTypes GetTerminalType() const { return terminalType; }

    /**Determine if endpoint is terminal type.
     */
    BOOL IsTerminal() const;

    /**Determine if endpoint is gateway type.
     */
    BOOL IsGateway() const;

    /**Determine if endpoint is gatekeeper type.
     */
    BOOL IsGatekeeper() const;

    /**Determine if endpoint is gatekeeper type.
     */
    BOOL IsMCU() const;

    /**Get the initial bandwidth parameter.
     */
    unsigned GetInitialBandwidth() const { return initialBandwidth; }

    /**Get the initial bandwidth parameter.
     */
    void SetInitialBandwidth(unsigned bandwidth) { initialBandwidth = bandwidth; }

    /**Get the default timeout for calling another endpoint.
     */
    PTimeInterval GetSignallingChannelCallTimeout() const { return signallingChannelCallTimeout; }

    /**Get the default timeout for master slave negotiations.
     */
    PTimeInterval GetMasterSlaveDeterminationTimeout() const { return masterSlaveDeterminationTimeout; }

    /**Get the default retries for H245 master slave negotiations.
     */
    unsigned GetMasterSlaveDeterminationRetries() const { return masterSlaveDeterminationRetries; }

    /**Get the default timeout for H245 capability exchange negotiations.
     */
    PTimeInterval GetCapabilityExchangeTimeout() const { return capabilityExchangeTimeout; }

    /**Get the default timeout for H245 logical channel negotiations.
     */
    PTimeInterval GetLogicalChannelTimeout() const { return logicalChannelTimeout; }

    /**Get the default timeout for H245 request mode negotiations.
     */
    PTimeInterval GetRequestModeTimeout() const { return logicalChannelTimeout; }

    /**Get the default timeout for H245 round trip delay negotiations.
     */
    PTimeInterval GetRoundTripDelayTimeout() const { return roundTripDelayTimeout; }

    /**Get the default rate H245 round trip delay is calculated by connection.
     */
    PTimeInterval GetRoundTripDelayRate() const { return roundTripDelayRate; }

    /**Get the default rate H245 round trip delay is calculated by connection.
     */
    BOOL ShouldClearCallOnRoundTripFail() const { return clearCallOnRoundTripFail; }

    /**Get the default timeout for GatekeeperRequest and Gatekeeper discovery.
     */
    PTimeInterval GetGatekeeperRequestTimeout() const { return gatekeeperRequestTimeout; }

    /**Get the default retries for GatekeeperRequest and Gatekeeper discovery.
     */
    unsigned GetGatekeeperRequestRetries() const { return gatekeeperRequestRetries; }

    /**Get the default timeout for RAS protocol transactions.
     */
    PTimeInterval GetRasRequestTimeout() const { return rasRequestTimeout; }

    /**Get the default retries for RAS protocol transations.
     */
    unsigned GetRasRequestRetries() const { return rasRequestRetries; }

    /**Get the default time for gatekeeper to reregister.
       A value of zero disables the keep alive facility.
     */
    PTimeInterval GetGatekeeperTimeToLive() const { return registrationTimeToLive; }
  //@}


  protected:
    H323Gatekeeper * InternalCreateGatekeeper(OpalTransport * transport);
    BOOL InternalRegisterGatekeeper(H323Gatekeeper * gk, BOOL discovered);
    H323Connection * FindConnectionWithoutLocks(const PString & token);

    // Configuration variables, commonly changed
    PStringList localAliasNames;

    BYTE          t35CountryCode;
    BYTE          t35Extension;
    WORD          manufacturerCode;
    TerminalTypes terminalType;

    unsigned initialBandwidth;  // in 100s of bits/sev
    BOOL     clearCallOnRoundTripFail;

    // Some more configuration variables, rarely changed.
    PTimeInterval signallingChannelCallTimeout;
    PTimeInterval masterSlaveDeterminationTimeout;
    unsigned      masterSlaveDeterminationRetries;
    PTimeInterval capabilityExchangeTimeout;
    PTimeInterval logicalChannelTimeout;
    PTimeInterval requestModeTimeout;
    PTimeInterval roundTripDelayTimeout;
    PTimeInterval roundTripDelayRate;
    PTimeInterval gatekeeperRequestTimeout;
    unsigned      gatekeeperRequestRetries;
    PTimeInterval rasRequestTimeout;
    unsigned      rasRequestRetries;
    PTimeInterval registrationTimeToLive;

    // Dynamic variables
    H323Capabilities capabilities;
    H323Gatekeeper * gatekeeper;

    unsigned lastCallReference;
};


#endif // __H323_H323EP_H


/////////////////////////////////////////////////////////////////////////////
