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
 * Revision 1.2047.2.5  2007/09/27 21:03:10  hfriederich
 * Allow thread-safe endpoint alias changes
 *
 * Revision 2.46.2.4  2007/08/05 13:12:15  hfriederich
 * Backport from HEAD - Changes since last commit
 *
 * Revision 2.46.2.3  2007/03/20 00:46:01  hfriederich
 * (Backport from HEAD)
 * Simple but messy changes to allow compile time removal of protocol
 *   options such as H.450 and H.460
 * Fix MakeConnection overrides
 *
 * Revision 2.46.2.2  2007/03/10 11:19:01  hfriederich
 * (Backport from HEAD)
 * Fixed backward compatibility of OnIncomingConnection()
 * Use correct dynamic payload type for H.323 calls
 * Ensure codec mask is applied on local caps
 * Use local jitter buffer values rather than getting direct from OpalManager
 * Allow OpalConnection string options to be set during incoming calls
 *
 * Revision 2.46.2.1  2007/02/07 08:51:00  hfriederich
 * New branch with major revision of the core Opal media format handling system.
 *
 * - Session IDs have been replaced by new OpalMediaType class.
 * - The creation of H.245 TCS and SDP media descriptions have been extended
 *   to dynamically handle all available media types
 * - The H.224 code has been rewritten for better integration into the Opal
 *   system. It takes advantage of the new media type system and removes
 *   all hooks found in the core Opal classes.
 *
 * More work will follow as the current version breaks lots of important code.
 *
 * Revision 2.46  2007/01/24 04:00:55  csoutheren
 * Arrrghh. Changing OnIncomingConnection turned out to have a lot of side-effects
 * Added some pure viritual functions to prevent old code from breaking silently
 * New OpalEndpoint and OpalConnection descendants will need to re-implement
 * OnIncomingConnection. Sorry :)
 *
 * Revision 2.45  2007/01/18 12:49:21  csoutheren
 * Add ability to disable T.38 in compile
 *
 * Revision 2.44  2007/01/18 04:45:16  csoutheren
 * Messy, but simple change to add additional options argument to OpalConnection constructor
 * This allows the provision of non-trivial arguments for connections
 *
 * Revision 2.43  2006/12/18 03:18:41  csoutheren
 * Messy but simple fixes
 *   - Add access to SIP REGISTER timeout
 *   - Ensure OpalConnection options are correctly progagated
 *
 * Revision 2.42  2006/12/13 05:01:21  csoutheren
 * Applied 1613992 - fixes a feature set memory leak
 * Thanks to Drazen Dimoti
 *
 * Revision 2.41  2006/08/29 01:37:11  csoutheren
 * Change secure URLs to use h323s and tcps to be inline with sips
 *
 * Revision 2.40  2006/08/21 05:29:25  csoutheren
 * Messy but relatively simple change to add support for secure (SSL/TLS) TCP transport
 * and secure H.323 signalling via the sh323 URL scheme
 *
 * Revision 2.39  2006/06/27 13:07:37  csoutheren
 * Patch 1374533 - add h323 Progress handling
 * Thanks to Frederich Heem
 *
 * Revision 2.38  2006/06/27 12:54:35  csoutheren
 * Patch 1374489 - h450.7 message center support
 * Thanks to Frederich Heem
 *
 * Revision 2.37  2006/05/30 11:33:02  hfriederich
 * Porting support for H.460 from OpenH323
 *
 * Revision 2.36  2006/05/30 04:58:05  csoutheren
 * Added suport for SIP INFO message (untested as yet)
 * Fixed some issues with SIP state machine on answering calls
 * Fixed some formatting issues
 *
 * Revision 2.35  2006/04/20 16:52:22  hfriederich
 * Adding support for H.224/H.281
 *
 * Revision 2.34  2006/03/07 11:23:46  csoutheren
 * Add ability to disable GRQ on GK registration
 *
 * Revision 2.33  2006/02/22 10:40:09  csoutheren
 * Added patch #1374583 from Frederic Heem
 * Added additional H.323 virtual function
 *
 * Revision 2.32  2006/02/22 10:29:09  csoutheren
 * Applied patch #1374470 from Frederic Heem
 * Add ability to disable H.245 negotiation
 *
 * Revision 2.31  2006/01/02 15:52:38  dsandras
 * Added what was required to merge changes from OpenH323 Altas_devel_2 in gkclient.cxx, gkserver.cxx and channels.cxx.
 *
 * Revision 2.30  2005/11/30 13:35:26  csoutheren
 * Changed tags for Doxygen
 *
 * Revision 2.29  2005/07/12 12:34:37  csoutheren
 * Fixes for H.450 errors and return values
 * Thanks to Iker Perez San Roman
 *
 * Revision 2.28  2005/07/11 01:52:23  csoutheren
 * Extended AnsweringCall to work for SIP as well as H.323
 * Fixed problems with external RTP connection in H.323
 * Added call to OnClosedMediaStream
 *
 * Revision 2.27  2004/09/01 12:21:27  rjongbloed
 * Added initialisation of H323EndPoints capability table to be all codecs so can
 *   correctly build remote caps from fqast connect params. This had knock on effect
 *   with const keywords added in numerous places.
 *
 * Revision 2.26  2004/08/14 07:56:28  rjongbloed
 * Major revision to utilise the PSafeCollection classes for the connections and calls.
 *
 * Revision 2.25  2004/07/11 12:42:09  rjongbloed
 * Added function on endpoints to get the list of all media formats any
 *   connection the endpoint may create can support.
 *
 * Revision 2.24  2004/06/05 14:31:32  rjongbloed
 * Added function to set gatekeeper registration time to live.
 * Added ability to have separate gatekeeper username to endpoint local alias name.
 *
 * Revision 2.23  2004/06/04 06:54:17  csoutheren
 * Migrated updates from OpenH323 1.14.1
 *
 * Revision 2.22  2004/02/24 09:40:27  rjongbloed
 * Fixed missing return in GetSTUN()
 *
 * Revision 2.21  2004/02/19 10:46:44  rjongbloed
 * Merged OpenH323 version 1.13.1 changes.
 *
 * Revision 2.20  2003/03/06 03:57:46  robertj
 * IVR support (work in progress) requiring large changes everywhere.
 *
 * Revision 2.19  2003/01/07 04:39:52  robertj
 * Updated to OpenH323 v1.11.2
 *
 * Revision 2.18  2002/11/10 11:33:16  robertj
 * Updated to OpenH323 v1.10.3
 *
 * Revision 2.17  2002/09/16 02:52:33  robertj
 * Added #define so can select if #pragma interface/implementation is used on
 *   platform basis (eg MacOS) rather than compiler, thanks Robert Monaghan.
 *
 * Revision 2.16  2002/09/10 07:42:40  robertj
 * Added function to get gatekeeper password.
 *
 * Revision 2.15  2002/09/06 02:39:27  robertj
 * Added function to set gatekeeper access token OID.
 *
 * Revision 2.14  2002/09/04 06:01:46  robertj
 * Updated to OpenH323 v1.9.6
 *
 * Revision 2.13  2002/07/04 07:41:46  robertj
 * Fixed memory/thread leak of transports.
 *
 * Revision 2.12  2002/07/01 04:56:29  robertj
 * Updated to OpenH323 v1.9.1
 *
 * Revision 2.11  2002/03/22 06:57:48  robertj
 * Updated to OpenH323 version 1.8.2
 *
 * Revision 2.10  2002/03/18 00:33:36  robertj
 * Removed duplicate initialBandwidth variable in H.323 class, moved to ancestor.
 *
 * Revision 2.9  2002/02/11 09:32:11  robertj
 * Updated to openH323 v1.8.0
 *
 * Revision 2.8  2002/01/22 04:59:04  robertj
 * Update from OpenH323, RFC2833 support
 *
 * Revision 2.7  2002/01/14 06:35:57  robertj
 * Updated to OpenH323 v1.7.9
 *
 * Revision 2.6  2001/11/13 06:25:56  robertj
 * Changed SetUpConnection() so returns BOOL as returning
 *   pointer to connection is not useful.
 *
 * Revision 2.5  2001/11/02 10:45:19  robertj
 * Updated to OpenH323 v1.7.3
 *
 * Revision 2.4  2001/10/05 00:22:13  robertj
 * Updated to PWLib 1.2.0 and OpenH323 1.7.0
 *
 * Revision 2.3  2001/08/17 08:21:15  robertj
 * Update from OpenH323
 * Moved call end reasons enum from OpalConnection to global.
 *
 * Revision 2.2  2001/08/01 05:10:40  robertj
 * Moved media formats list from endpoint to connection.
 *
 * Revision 2.1  2001/07/30 07:22:25  robertj
 * Abstracted listener management from H.323 to OpalEndPoint class.
 *
 * Revision 2.0  2001/07/27 15:48:24  robertj
 * Conversion of OpenH323 to Open Phone Abstraction Library (OPAL)
 *
 * Revision 1.56  2003/12/29 04:58:55  csoutheren
 * Added callbacks on H323EndPoint when gatekeeper discovery succeeds or fails
 *
 * Revision 1.55  2003/12/28 02:52:15  csoutheren
 * Added virtual to a few functions
 *
 * Revision 1.54  2003/12/28 02:38:14  csoutheren
 * Added H323EndPoint::OnOutgoingCall
 *
 * Revision 1.53  2003/12/28 00:07:10  csoutheren
 * Added callbacks on H323EndPoint when gatekeeper registration succeeds or fails
 *
 * Revision 1.52  2003/04/24 01:49:33  dereks
 * Add ability to set no media timeout interval
 *
 * Revision 1.51  2003/04/10 09:39:48  robertj
 * Added associated transport to new GetInterfaceAddresses() function so
 *   interfaces can be ordered according to active transport links. Improves
 *   interoperability.
 *
 * Revision 1.50  2003/04/10 01:05:11  craigs
 * Added functions to access to lists of interfaces
 *
 * Revision 1.49  2003/04/07 13:09:25  robertj
 * Added ILS support to callto URL parsing in MakeCall(), ie can now call hosts
 *   registered with an ILS directory.
 *
 * Revision 1.48  2003/02/13 00:11:31  robertj
 * Added missing virtual for controlling call transfer, thanks Andrey Pinaev
 *
 * Revision 1.47  2003/02/09 00:48:06  robertj
 * Added function to return if registered with gatekeeper.
 *
 * Revision 1.46  2003/02/04 07:06:41  robertj
 * Added STUN support.
 *
 * Revision 1.45  2003/01/26 05:57:58  robertj
 * Changed ParsePartyName so will accept addresses of the form
 *   alias@gk:address which will do an LRQ call to "address" using "alias"
 *   to determine the IP address to connect to.
 *
 * Revision 1.44  2003/01/23 02:36:30  robertj
 * Increased (and made configurable) timeout for H.245 channel TCP connection.
 *
 * Revision 1.43  2002/11/28 01:19:55  craigs
 * Added virtual to several functions
 *
 * Revision 1.42  2002/11/27 06:54:52  robertj
 * Added Service Control Session management as per Annex K/H.323 via RAS
 *   only at this stage.
 * Added H.248 ASN and very primitive infrastructure for linking into the
 *   Service Control Session management system.
 * Added basic infrastructure for Annex K/H.323 HTTP transport system.
 * Added Call Credit Service Control to display account balances.
 *
 * Revision 1.41  2002/11/15 05:17:22  robertj
 * Added facility redirect support without changing the call token for access
 *   to the call. If it gets redirected a new H323Connection object is
 *   created but it looks like the same thing to an application.
 *
 * Revision 1.40  2002/11/10 08:10:43  robertj
 * Moved constants for "well known" ports to better place (OPAL change).
 *
 * Revision 1.39  2002/10/31 00:32:15  robertj
 * Enhanced jitter buffer system so operates dynamically between minimum and
 *   maximum values. Altered API to assure app writers note the change!
 *
 * Revision 1.38  2002/10/23 06:06:10  robertj
 * Added function to be smarter in using a gatekeeper for use by endpoint.
 *
 * Revision 1.37  2002/10/21 06:07:44  robertj
 * Added function to set gatekeeper access token OID.
 *
 * Revision 1.36  2002/09/16 01:14:15  robertj
 * Added #define so can select if #pragma interface/implementation is used on
 *   platform basis (eg MacOS) rather than compiler, thanks Robert Monaghan.
 *
 * Revision 1.35  2002/09/10 06:32:25  robertj
 * Added function to get gatekeeper password.
 *
 * Revision 1.34  2002/09/03 06:19:36  robertj
 * Normalised the multi-include header prevention ifdef/define symbol.
 *
 * Revision 1.33  2002/07/19 03:39:19  robertj
 * Bullet proofed setting of RTP IP port base, can't be zero!
 *
 * Revision 1.32  2002/07/18 01:50:10  robertj
 * Changed port secltion code to force apps to use function interface.
 *
 * Revision 1.31  2002/06/22 05:48:38  robertj
 * Added partial implementation for H.450.11 Call Intrusion
 *
 * Revision 1.30  2002/06/13 06:15:19  robertj
 * Allowed TransferCall() to be used on H323Connection as well as H323EndPoint.
 *
 * Revision 1.29  2002/06/12 03:55:21  robertj
 * Added function to add/remove multiple listeners in one go comparing against
 *   what is already running so does not interrupt unchanged listeners.
 *
 * Revision 1.28  2002/05/29 06:40:29  robertj
 * Changed sending of endSession/ReleaseComplete PDU's to occur immediately
 *   on call clearance and not wait for background thread to do it.
 * Stricter compliance by waiting for reply endSession before closing down.
 *
 * Revision 1.27  2002/05/28 06:15:09  robertj
 * Split UDP (for RAS) from RTP port bases.
 * Added current port variable so cycles around the port range specified which
 *   fixes some wierd problems on some platforms, thanks Federico Pinna
 *
 * Revision 1.26  2002/05/17 03:38:05  robertj
 * Fixed problems with H.235 authentication on RAS for server and client.
 *
 * Revision 1.25  2002/05/16 00:03:05  robertj
 * Added function to get the tokens for all active calls.
 * Improved documentation for use of T.38 and T.120 functions.
 *
 * Revision 1.24  2002/05/15 08:59:18  rogerh
 * Update comments
 *
 * Revision 1.23  2002/05/03 05:38:15  robertj
 * Added Q.931 Keypad IE mechanism for user indications (DTMF).
 *
 * Revision 1.22  2002/05/02 07:56:24  robertj
 * Added automatic clearing of call if no media (RTP data) is transferred in a
 *   configurable (default 5 minutes) amount of time.
 *
 * Revision 1.21  2002/04/18 01:41:07  robertj
 * Fixed bad variable name for disabling DTMF detection, very confusing.
 *
 * Revision 1.20  2002/04/17 00:49:56  robertj
 * Added ability to disable the in band DTMF detection.
 *
 * Revision 1.19  2002/04/10 06:48:47  robertj
 * Added functions to set port member variables.
 *
 * Revision 1.18  2002/03/14 03:49:38  dereks
 * Fix minor documentation error.
 *
 * Revision 1.17  2002/02/04 07:17:52  robertj
 * Added H.450.2 Consultation Transfer, thanks Norwood Systems.
 *
 * Revision 1.16  2002/01/24 06:29:02  robertj
 * Added option to disable H.245 negotiation in SETUP pdu, this required
 *   API change so have a bit mask instead of a series of booleans.
 *
 * Revision 1.15  2002/01/17 07:04:58  robertj
 * Added support for RFC2833 embedded DTMF in the RTP stream.
 *
 * Revision 1.14  2002/01/13 23:59:43  robertj
 * Added CallTransfer timeouts to endpoint, hanks Ben Madsen of Norwood Systems.
 *
 * Revision 1.13  2002/01/08 04:45:35  robertj
 * Added MakeCallLocked() so can start a call with the H323Connection instance
 *   initally locked so can do things to it before the call really starts.
 *
 * Revision 1.12  2001/12/22 03:20:44  robertj
 * Added create protocol function to H323Connection.
 *
 * Revision 1.11  2001/12/13 10:55:30  robertj
 * Added gatekeeper access token OID specification for auto inclusion of
 *   access tokens frm ACF to SETUP pdu.
 *
 * Revision 1.10  2001/11/09 05:39:54  craigs
 * Added initial T.38 support thanks to Adam Lazur
 *
 * Revision 1.9  2001/11/01 00:27:33  robertj
 * Added default Fast Start disabled and H.245 tunneling disable flags
 *   to the endpoint instance.
 *
 * Revision 1.8  2001/09/11 01:24:36  robertj
 * Added conditional compilation to remove video and/or audio codecs.
 *
 * Revision 1.7  2001/09/11 00:21:21  robertj
 * Fixed missing stack sizes in endpoint for cleaner thread and jitter thread.
 *
 * Revision 1.6  2001/08/24 14:03:26  rogerh
 * Fix some spelling mistakes
 *
 * Revision 1.5  2001/08/16 07:49:16  robertj
 * Changed the H.450 support to be more extensible. Protocol handlers
 *   are now in separate classes instead of all in H323Connection.
 *
 * Revision 1.4  2001/08/10 11:03:49  robertj
 * Major changes to H.235 support in RAS to support server.
 *
 * Revision 1.3  2001/08/08 23:54:11  robertj
 * Fixed problem with setting gk password before have a gk variable.
 *
 * Revision 1.2  2001/08/06 03:15:17  robertj
 * Improved access to H.235 secure RAS functionality.
 *
 * Revision 1.1  2001/08/06 03:08:11  robertj
 * Fission of h323.h to h323ep.h & h323con.h, h323.h now just includes files.
 */

#ifndef __OPAL_H323EP_H
#define __OPAL_H323EP_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif


#include <opal/endpoint.h>
#include <opal/manager.h>
#include <opal/call.h>
#include <opal/transports.h>
#include <h323/h323con.h>
#include <h323/h323caps.h>
#include <h323/h235auth.h>

#ifdef H323_H460
#include <h323/h4601.h>
#endif


class H225_EndpointType;
class H225_VendorIdentifier;
class H225_H221NonStandard;
class H225_ServiceControlDescriptor;
class H225_FeatureSet;

class H235SecurityInfo;

class H323Gatekeeper;
class H323SignalPDU;
class H323ServiceControlSession;

///////////////////////////////////////////////////////////////////////////////

/**This class manages the H323 endpoint.
   An endpoint may have zero or more listeners to create incoming connections
   or zero or more outgoing connections initiated via the MakeCall() function.
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
      DefaultTcpSignalPort = 1720
    };

  /**@name Construction */
  //@{
    /**Create a new endpoint.
     */
    H323EndPoint(
      OpalManager & manager,
      const char * prefix = "h323",
      WORD defaultSignalPort = DefaultTcpSignalPort
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
       FALSE is returned.

       This function usually returns almost immediately with the connection
       continuing to occur in a new background thread.

       If FALSE is returned then the connection could not be established. For
       example if a PSTN endpoint is used and the assiciated line is engaged
       then it may return immediately. Returning a non-NULL value does not
       mean that the connection will succeed, only that an attempt is being
       made.

       The default behaviour is pure.
     */
    virtual BOOL MakeConnection(
      OpalCall & call,          ///<  Owner of connection
      const PString & party,    ///<  Remote party to call
      void * userData,          ///<  Arbitrary data to pass to connection
      unsigned int options,     ///<  options to pass to conneciton
      OpalConnection::StringOptions * stringOptions = NULL
    );

    /**Get the data formats this endpoint is capable of operating.
       This provides a list of media data format names that may be used by an
       OpalMediaStream may be created by a connection from this endpoint.

       Note that a specific connection may not actually support all of the
       media formats returned here, but should return no more.

       The default behaviour is pure.
      */
    virtual OpalMediaFormatList GetMediaFormats() const;
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
      H323Capability * capability   ///<  New codec specification
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
      PINDEX descriptorNum, ///<  The member of the capabilityDescriptor to add
      PINDEX simultaneous,  ///<  The member of the SimultaneousCapabilitySet to add
      H323Capability * cap  ///<  New capability specification
    );

    /**Add all matching capabilities in list.
       All capabilities that match the specified name are added. See the
       capabilities code for details on the matching algorithm.
      */
    PINDEX AddAllCapabilities(
      PINDEX descriptorNum, ///<  The member of the capabilityDescriptor to add
      PINDEX simultaneous,  ///<  The member of the SimultaneousCapabilitySet to add
      const PString & name  ///<  New capabilities name, if using "known" one.
    );

    /**Add all user input capabilities to this endpoints capability table.
      */
    void AddAllUserInputCapabilities(
      PINDEX descriptorNum, ///<  The member of the capabilityDescriptor to add
      PINDEX simultaneous   ///<  The member of the SimultaneousCapabilitySet to add
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
      const H245_Capability & cap  ///<  H245 capability table entry
    ) const;

    /**Find a capability that has been registered.
     */
    H323Capability * FindCapability(
      const H245_DataType & dataType  ///<  H245 data type of codec
    ) const;

    /**Find a capability that has been registered.
     */
    H323Capability * FindCapability(
      H323Capability::MainTypes mainType,   ///<  Main type of codec
      unsigned subType                      ///<  Subtype of codec
    ) const;
  //@}

  /**@name Gatekeeper management */
  //@{
    /**Use and register with an explicit gatekeeper.
       This will call other functions according to the following table:

           address    identifier   function
           empty      empty        DiscoverGatekeeper()
           non-empty  empty        SetGatekeeper()
           empty      non-empty    LocateGatekeeper()
           non-empty  non-empty    SetGatekeeperZone()

       The localAddress field, if non-empty, indicates the interface on which
       to look for the gatekeeper. An empty string is equivalent to "ip$*:*"
       which is any interface or port.

       If the endpoint is already registered with a gatekeeper that meets
       the same criteria then the gatekeeper is not changed, otherwise it is
       deleted (with unregistration) and new one created and registered to.
     */
    BOOL UseGatekeeper(
      const PString & address = PString::Empty(),     ///<  Address of gatekeeper to use.
      const PString & identifier = PString::Empty(),  ///<  Identifier of gatekeeper to use.
      const PString & localAddress = PString::Empty() ///<  Local interface to use.
    );

    /**Select and register with an explicit gatekeeper.
       This will use the specified transport and a string giving a transport
       dependent address to locate a specific gatekeeper. The endpoint will
       register with that gatekeeper and, if successful, set it as the current
       gatekeeper used by this endpoint.

       Note the transport being passed in will be deleted by this function or
       the H323Gatekeeper object it becomes associated with. Also if transport
       is NULL then a H323TransportUDP is created.
     */
    BOOL SetGatekeeper(
      const PString & address,          ///<  Address of gatekeeper to use.
      H323Transport * transport = NULL  ///<  Transport over which to talk to gatekeeper.
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
       is NULL then a H323TransportUDP is created.
     */
    BOOL SetGatekeeperZone(
      const PString & address,          ///<  Address of gatekeeper to use.
      const PString & identifier,       ///<  Identifier of gatekeeper to use.
      H323Transport * transport = NULL  ///<  Transport over which to talk to gatekeeper.
    );

    /**Locate and select gatekeeper.
       This function will use the automatic gatekeeper discovery methods to
       locate the gatekeeper on the particular transport that has the specified
       gatekeeper identifier name. This is often the "Zone" for the gatekeeper.

       Note the transport being passed in will be deleted becomes owned by the
       H323Gatekeeper created by this function and will be deleted by it. Also
       if transport is NULL then a H323TransportUDP is created.
     */
    BOOL LocateGatekeeper(
      const PString & identifier,       ///<  Identifier of gatekeeper to locate.
      H323Transport * transport = NULL  ///<  Transport over which to talk to gatekeeper.
    );

    /**Discover and select gatekeeper.
       This function will use the automatic gatekeeper discovery methods to
       locate the first gatekeeper on a particular transport.

       Note the transport being passed in will be deleted becomes owned by the
       H323Gatekeeper created by this function and will be deleted by it. Also
       if transport is NULL then a H323TransportUDP is created.
     */
    BOOL DiscoverGatekeeper(
      H323Transport * transport = NULL  ///<  Transport over which to talk to gatekeeper.
    );

    /**Create a gatekeeper.
       This allows the application writer to have the gatekeeper as a
       descendent of the H323Gatekeeper in order to add functionality to the
       base capabilities in the library.

       The default creates an instance of the H323Gatekeeper class.
     */
    virtual H323Gatekeeper * CreateGatekeeper(
      H323Transport * transport  ///<  Transport over which gatekeepers communicates.
    );

    /**Get the gatekeeper we are registered with.
     */
    H323Gatekeeper * GetGatekeeper() const { return gatekeeper; }

    /**Return if endpoint is registered with gatekeeper.
      */
    BOOL IsRegisteredWithGatekeeper() const;

    /**Unregister and delete the gatekeeper we are registered with.
       The return value indicates FALSE if there was an error during the
       unregistration. However the gatekeeper is still removed and its
       instance deleted regardless of this error.
     */
    BOOL RemoveGatekeeper(
      int reason = -1    ///<  Reason for gatekeeper removal
    );

    /**Set the H.235 password for the gatekeeper.
      */
    virtual void SetGatekeeperPassword(
      const PString & password,
      const PString & username = PString::Empty()
    );

    /**Get the H.235 username for the gatekeeper.
      */
    virtual const PString & GetGatekeeperUsername() const { return gatekeeperUsername; }

    /**Get the H.235 password for the gatekeeper.
      */
    virtual const PString & GetGatekeeperPassword() const { return gatekeeperPassword; }

    /**Create a list of authenticators for gatekeeper.
      */
    virtual H235Authenticators CreateAuthenticators();

    /**Called when the gatekeeper sends a GatekeeperConfirm
      */
    virtual void  OnGatekeeperConfirm();

    /**Called when the gatekeeper sends a GatekeeperReject
      */
    virtual void  OnGatekeeperReject();

    /**Called when the gatekeeper sends a RegistrationConfirm
      */
    virtual void OnRegistrationConfirm();

    /**Called when the gatekeeper sends a RegistrationReject
      */
    virtual void  OnRegistrationReject();
  //@}

  /**@name Connection management */
  //@{
    /**Handle new incoming connetion from listener.
      */
    virtual BOOL NewIncomingConnection(
      OpalTransport * transport  ///<  Transport connection came in on
    );

    /**Create a connection that uses the specified call.
      */
    virtual H323Connection * CreateConnection(
      OpalCall & call,                         ///<  Call object to attach the connection to
      const PString & token,                   ///<  Call token for new connection
      void * userData,                         ///<  Arbitrary user data from MakeConnection
      OpalTransport & transport,               ///<  Transport for connection
      const PString & alias,                   ///<  Alias for outgoing call
      const H323TransportAddress & address,    ///<  Address for outgoing call
      H323SignalPDU * setupPDU,                ///<  Setup PDU for incoming call
      unsigned options = 0,
      OpalConnection::StringOptions * stringOptions = NULL ///<  complex string options
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

       This function is declared virtual to allow an application to override
       the function and get the new call token of the forwarded call.
     */
    virtual BOOL SetupTransfer(
      const PString & token,        ///<  Existing connection to be transferred
      const PString & callIdentity, ///<  Call identity of the secondary call (if it exists)
      const PString & remoteParty,  ///<  Remote party to transfer the existing call to
      void * userData = NULL        ///<  user data to pass to CreateConnection
    );

    /**Initiate the transfer of an existing call (connection) to a new remote
       party using H.450.2.  This sends a Call Transfer Initiate Invoke
       message from the A-Party (transferring endpoint) to the B-Party
       (transferred endpoint).
     */
    void TransferCall(
      const PString & token,        ///<  Existing connection to be transferred
      const PString & remoteParty,  ///<  Remote party to transfer the existing call to
      const PString & callIdentity = PString::Empty()
                                    ///<  Call Identity of secondary call if present
    );

    /**Transfer the call through consultation so the remote party in the
       primary call is connected to the called party in the second call
       using H.450.2.  This sends a Call Transfer Identify Invoke message
       from the A-Party (transferring endpoint) to the C-Party
       (transferred-to endpoint).
     */
    void ConsultationTransfer(
      const PString & primaryCallToken,   ///<  Token of primary call
      const PString & secondaryCallToken  ///<  Token of secondary call
    );

    /**Place the call on hold, suspending all media channels (H.450.4)
    * NOTE: Only Local Hold is implemented so far. 
    */
    void HoldCall(
      const PString & token,        ///<  Existing connection to be transferred
      BOOL localHold   ///<  true for Local Hold, false for Remote Hold
    );

    /** Initiate Call intrusion
        Designed similar to MakeCall function
      */
    BOOL IntrudeCall(
      const PString & remoteParty,  ///<  Remote party to intrude call
      unsigned capabilityLevel,     ///<  Capability level
      void * userData = NULL        ///<  user data to pass to CreateConnection
    );

    /**Parse a party address into alias and transport components.
       An appropriate transport is determined from the remoteParty parameter.
       The general form for this parameter is [alias@][transport$]host[:port]
       where the default alias is the same as the host, the default transport
       is "ip" and the default port is 1720.
      */
    BOOL ParsePartyName(
      const PString & party,          ///<  Party name string.
      PString & alias,                ///<  Parsed alias name
      H323TransportAddress & address  ///<  Parsed transport address
    );

    /**Find a connection that uses the specified token.
       This searches the endpoint for the connection that contains the token
       as provided by functions such as MakeCall(). if not found it will then search for
       the string representation of the CallIdentifier for the connection, and
       finally try for the string representation of the ConferenceIdentifier.

       Note the caller of this function MUSt call the H323Connection::Unlock()
       function if this function returns a non-NULL pointer. If it does not
       then a deadlock can occur.
      */
    PSafePtr<H323Connection> FindConnectionWithLock(
      const PString & token,     ///<  Token to identify connection
      PSafetyMode mode = PSafeReadWrite
    );

    /** OnSendSignalSetup is a hook for the appliation to attach H450 info in setup, 
        for instance, H450.7 Activate or Deactivate 
        @param connection the connection associated to the setup
        @param pduSetup the setup message to modify
        @return if false, do no send the setup pdu
      */
      
    virtual BOOL OnSendSignalSetup(H323Connection & connection,
                                   H323SignalPDU & setupPDU);

    /**Adjust call proceeding PDU being sent. This function is called from
    the OnReceivedSignalSetup() function before it sends the Call
    Proceeding PDU. It gives an opportunity for an application to alter
    the request before transmission to the other endpoint. If this function
    returns FALSE then the Call Proceeding PDU is not sent at all.

    The default behaviour simply returns TRUE.
    @param connection the connection associated to the call proceeding
    @param callProceedingPDU the call processding to modify
    @return if false, do no send the connect pdu  
     */
    virtual BOOL OnSendCallProceeding(H323Connection & connection,
                                      H323SignalPDU & callProceedingPDU
                                     );
    
    /**Adjust call connect PDU being sent. This function is called from
    the H323Connection::SetConnected function before it sends the connect  PDU. 
    It gives an opportunity for an application to alter
    the request before transmission to the other endpoint. If this function
    returns FALSE then the connect PDU is not sent at all.

    The default behaviour simply returns TRUE.
    @param connection the connection associated to the connect
    @param connectPDU the connect to modify
    @return if false, do no send the connect pdu  
     */
    virtual BOOL OnSendConnect(H323Connection & connection,
                               H323SignalPDU & connectPDU
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
      H323Connection & connection,    ///<  Connection that was established
      const H323SignalPDU & setupPDU,   ///<  Received setup PDU
      H323SignalPDU & alertingPDU       ///<  Alerting PDU to send
    );

    /**Called when an outgoing call connects
       If FALSE is returned the connection is aborted and a Release Complete
       PDU is sent.

       The default behaviour simply returns TRUE.
      */
    virtual BOOL OnOutgoingCall(
        H323Connection & conn, 
        const H323SignalPDU & connectPDU
    );

    /**Handle a connection transfer.
       This gives the application an opportunity to abort the transfer.
       The default behaviour just returns TRUE.
      */
    virtual BOOL OnCallTransferInitiate(
      H323Connection & connection,    ///<  Connection to transfer
      const PString & remoteParty     ///<  Party transferring to.
    );

    /**Handle a transfer via consultation.
       This gives the transferred-to user an opportunity to abort the transfer.
       The default behaviour just returns TRUE.
      */
    virtual BOOL OnCallTransferIdentify(
      H323Connection & connection    ///<  Connection to transfer
    );

    /**
      * Callback for ARQ send to a gatekeeper. This allows the endpoint
      * to change or check fields in the ARQ before it is sent.
      */
    virtual void OnSendARQ(
      H323Connection & conn,
      H225_AdmissionRequest & arq
    );

    /**Call back for answering an incoming call.
       This function is a H.323 specific version of OpalEndPoint::OnAnswerCall
       that contains additional information that applies only to H.323.

       By default this calls OpalEndPoint::OnAnswerCall, which returns
     */
    virtual OpalConnection::AnswerCallResponse OnAnswerCall(
      H323Connection & connection,    ///< Connection that was established
      const PString & callerName,       ///< Name of caller
      const H323SignalPDU & setupPDU,   ///< Received setup PDU
      H323SignalPDU & connectPDU,       ///< Connect PDU to send. 
      H323SignalPDU & progressPDU        ///< Progress PDU to send. 
    );
    virtual OpalConnection::AnswerCallResponse OnAnswerCall(
       OpalConnection & connection,
       const PString & caller
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
      H323Connection & connection,    ///<  Connection that was established
      const H323SignalPDU & alertingPDU,  ///<  Received Alerting PDU
      const PString & user                ///<  Username of remote endpoint
    );

    /** A call back function when the alerting is about to be sent, 
        can be used by the application to alter the alerting Pdu
        @return if FALSE, then the alerting is not sent
     */
      
    virtual BOOL OnSendAlerting(H323Connection & connection,  ///< onnection that was established
                                H323SignalPDU & alerting,     ///< Alerting PDU to modify
                                const PString & calleeName,   ///< Name of endpoint being alerted.
                                BOOL withMedia                ///< Open media with alerting
                               );
        
    /** A call back function when the alerting has been sent, can be used by the application 
        to send the connect as soon as the alerting has been sent.
     */
      
    virtual BOOL OnSentAlerting(H323Connection & connection);
    
    /**A call back function when a connection indicates it is to be forwarded.
       An H323 application may handle this call back so it can make
       complicated decisions on if the call forward ius to take place. If it
       decides to do so it must call MakeCall() and return TRUE.

       The default behaviour simply returns FALSE and that the automatic
       call forwarding should take place. See ForwardConnection()
      */
    virtual BOOL OnConnectionForwarded(
      H323Connection & connection,    ///<  Connection to be forwarded
      const PString & forwardParty,   ///<  Remote party to forward to
      const H323SignalPDU & pdu       ///<  Full PDU initiating forwarding
    );

    /**Forward the call using the same token as the specified connection.
       Return TRUE if the call is being redirected.

       The default behaviour will replace the current call in the endpoints
       call list using the same token as the call being redirected. Not that
       even though the same token is being used the actual object is
       completely mad anew.
      */
    virtual BOOL ForwardConnection(
      H323Connection & connection,    ///<  Connection to be forwarded
      const PString & forwardParty,   ///<  Remote party to forward to
      const H323SignalPDU & pdu       ///<  Full PDU initiating forwarding
    );

    /**A call back function whenever a connection is established.
       This indicates that a connection to a remote endpoint was established
       with a control channel and zero or more logical channels.

       The default behaviour does nothing.
      */
    virtual void OnConnectionEstablished(
      H323Connection & connection,    ///<  Connection that was established
      const PString & token           ///<  Token for identifying connection
    );

    /**Determine if a connection is established.
      */
    virtual BOOL IsConnectionEstablished(
      const PString & token   ///<  Token for identifying connection
    );

    /**A call back function whenever a connection is broken.
       This indicates that a connection to a remote endpoint is no longer
       available.

       The default behaviour does nothing.
      */
    virtual void OnConnectionCleared(
      H323Connection & connection,    ///<  Connection that was established
      const PString & token           ///<  Token for identifying connection
    );
  //@}


  /**@name Logical Channels management */
  //@{
    /**Call back for opening a logical channel.

       The default behaviour simply returns TRUE.
      */
    virtual BOOL OnStartLogicalChannel(
      H323Connection & connection,    ///<  Connection for the channel
      H323Channel & channel           ///<  Channel being started
    );

    /**Call back for closed a logical channel.

       The default behaviour does nothing.
      */
    virtual void OnClosedLogicalChannel(
      H323Connection & connection,    ///<  Connection for the channel
      const H323Channel & channel     ///<  Channel being started
    );

    /**Callback from the RTP session for statistics monitoring.
       This is called every so many packets on the transmitter and receiver
       threads of the RTP session indicating that the statistics have been
       updated.

       The default behaviour does nothing.
      */
    virtual void OnRTPStatistics(
      const H323Connection & connection,  ///<  Connection for the channel
      const RTP_Session & session         ///<  Session with statistics
    ) const;

    /**Call back from GK admission confirm to notify the 
     * Endpoint it is behind a NAT (GNUGK Gatekeeper).
     * The default does nothing. 
     * Override this to notify the user they are behind a NAT.
     */
    virtual void OnGatekeeperNATDetect(
      PIPSocket::Address publicAddr,         ///> Public address as returned by the Gatekeeper
      PString & gkIdentifier,                ///> Identifier at the gatekeeper
      H323TransportAddress & gkRouteAddress  ///> Gatekeeper Route Address
    );
  //@}

  /**@name Service Control */
  //@{
    /**Call back for HTTP based Service Control.
       An application may override this to use an HTTP based channel using a
       resource designated by the session ID. For example the session ID can
       correspond to a browser window and the 

       The default behaviour does nothing.
      */
    virtual void OnHTTPServiceControl(
      unsigned operation,  ///<  Control operation
      unsigned sessionId,  ///<  Session ID for HTTP page
      const PString & url  ///<  URL to use.
    );

    /**Call back for call credit information.
       An application may override this to display call credit information
       on registration, or when a call is started.

       The canDisplayAmountString member variable must also be set to TRUE
       for this to operate.

       The default behaviour does nothing.
      */
    virtual void OnCallCreditServiceControl(
      const PString & amount,  ///<  UTF-8 string for amount, including currency.
      BOOL mode          ///<  Flag indicating that calls will debit the account.
    );

    /**Handle incoming service control session information.
       Default behaviour calls session.OnChange()
     */
    virtual void OnServiceControlSession(
      unsigned type,
      unsigned sessionid,
      const H323ServiceControlSession & session,
      H323Connection * connection
    );

    /**Create the service control session object.
     */
    virtual H323ServiceControlSession * CreateServiceControlSession(
      const H225_ServiceControlDescriptor & contents
    );
  //@}

  /**@name Additional call services */
  //@{
    /** Called when an endpoint receives a SETUP PDU with a
        conference goal of "invite"
      
        The default behaviour is to return FALSE, which will close the connection
     */
    virtual BOOL OnConferenceInvite(
      const H323SignalPDU & setupPDU
    );

    /** Called when an endpoint receives a SETUP PDU with a
        conference goal of "callIndependentSupplementaryService"
      
        The default behaviour is to return FALSE, which will close the connection
     */
    virtual BOOL OnCallIndependentSupplementaryService(
      const H323SignalPDU & setupPDU
    );

    /** Called when an endpoint receives a SETUP PDU with a
        conference goal of "capability_negotiation"
      
        The default behaviour is to return FALSE, which will close the connection
     */
    virtual BOOL OnNegotiateConferenceCapabilities(
      const H323SignalPDU & setupPDU
    );
  //@}

  /**@name Member variable access */
  //@{
    /**Set the user name to be used for the local end of any connections. This
       defaults to the logged in user as obtained from the
       PProcess::GetUserName() function.

       Note that this name is technically the first alias for the endpoint.
       Additional aliases may be added by the use of the AddAliasName()
       function, however that list will be cleared when this function is used.
      
       Note that this method is not thread-safe if the enpoint is registered
       at a Gatekeeper. You should use the mutex returned with 
       GetAliasNamesMutex() in this case.
     */
    virtual void SetLocalUserName(
      const PString & name  ///<  Local name of endpoint (prime alias)
    );

    /**Get the user name to be used for the local end of any connections. This
       defaults to the logged in user as obtained from the
       PProcess::GetUserName() function.
     */
    virtual const PString & GetLocalUserName() const { PWaitAndSignal m(localAliasNamesMutex); return localAliasNames[0]; }

    /**Add an alias name to be used for the local end of any connections. If
       the alias name already exists in the list then is is not added again.

       The list defaults to the value set in the SetLocalUserName() function.
       Note that calling SetLocalUserName() will clear the alias list.
      
       Note that this method is not thread-safe if the endpoint is registered
       at a Gatekeeper. You should use the mutex returned with
       GetAliasNamesMutex() in this case.
     */
    BOOL AddAliasName(
      const PString & name  ///<  New alias name to add
    );

    /**Remove an alias name used for the local end of any connections. 
       defaults to an empty list.
      
       Note that this method is not thread-safe if the endpoint is registered
       at a Gatekeeper. You should use the mutex returned with
       GetAliasNamesMutex() in this case.
     */
    BOOL RemoveAliasName(
      const PString & name  ///<  New alias namer to add
    );

    /**Get the user name to be used for the local end of any connections. This
       defaults to the logged in user as obtained from the
       PProcess::GetUserName() function.
     */
    const PStringList & GetAliasNames() const { PWaitAndSignal m(localAliasNamesMutex); return localAliasNames; }
    
    /**Returns the mutex used to protect the alias names list
      */
    PMutex & GetAliasNamesMutex() { return localAliasNamesMutex; }

    /**Get the default ILS server to use for user lookup.
      */
    const PString & GetDefaultILSServer() const { return manager.GetDefaultILSServer(); }

    /**Set the default ILS server to use for user lookup.
      */
    void SetDefaultILSServer(
      const PString & server
    ) { manager.SetDefaultILSServer(server); }

    /**Get the default fast start mode.
      */
    BOOL IsFastStartDisabled() const
      { return disableFastStart; }

    /**Set the default fast start mode.
      */
    void DisableFastStart(
      BOOL mode ///<  New default mode
    ) { disableFastStart = mode; } 

    /**Get the default H.245 tunneling mode.
      */
    BOOL IsH245TunnelingDisabled() const
      { return disableH245Tunneling; }

    /**Set the default H.245 tunneling mode.
      */
    void DisableH245Tunneling(
      BOOL mode ///<  New default mode
    ) { disableH245Tunneling = mode; } 

    /**Get the default H.245 tunneling mode.
      */
    BOOL IsH245inSetupDisabled() const
      { return disableH245inSetup; }

    /**Set the default H.245 tunneling mode.
      */
    void DisableH245inSetup(
      BOOL mode ///<  New default mode
    ) { disableH245inSetup = mode; } 

    /** find out if h245 is disabled or enabled 
      * @return TRUE if h245 is disabled 
      */
    BOOL IsH245Disabled() const
    { return m_bH245Disabled; }

    /**Disable/Enable H.245, used at least for h450.7 calls
     * @param  bH245Disabled TRUE if h245 has to be disabled 
     */
    void DisableH245(BOOL bH245Disabled) { m_bH245Disabled = bH245Disabled; } 
    
    /**Get the flag indicating the endpoint can display an amount string.
      */
    BOOL CanDisplayAmountString() const
      { return canDisplayAmountString; }

    /**Set the flag indicating the endpoint can display an amount string.
      */
    void SetCanDisplayAmountString(
      BOOL mode ///<  New default mode
    ) { canDisplayAmountString = mode; } 

    /**Get the flag indicating the call will automatically clear after a time.
      */
    BOOL CanEnforceDurationLimit() const
      { return canEnforceDurationLimit; }

    /**Set the flag indicating the call will automatically clear after a time.
      */
    void SetCanEnforceDurationLimit(
      BOOL mode ///<  New default mode
    ) { canEnforceDurationLimit = mode; } 

#if OPAL_H450
    /**Get Call Intrusion Protection Level of the end point.
      */
    unsigned GetCallIntrusionProtectionLevel() const { return callIntrusionProtectionLevel; }

    /**Set Call Intrusion Protection Level of the end point.
      */
    void SetCallIntrusionProtectionLevel(
      unsigned level  ///<  New level from 0 to 3
    ) { PAssert(level<=3, PInvalidParameter); callIntrusionProtectionLevel = level; }
#endif

    /**Called from H.450 OnReceivedInitiateReturnError
      */
    virtual void OnReceivedInitiateReturnError();

#if OPAL_VIDEO
    /**See if should auto-start receive video channels on connection.
     */
    BOOL CanAutoStartReceiveVideo() const { return manager.CanAutoStartReceiveVideo(); }

    /**See if should auto-start transmit video channels on connection.
     */
    BOOL CanAutoStartTransmitVideo() const { return manager.CanAutoStartTransmitVideo(); }
#endif

    /**See if should auto-start receive fax channels on connection.
     */
    BOOL CanAutoStartReceiveData() const { return autoStartReceiveData; }

    /**See if should auto-start transmit fax channels on connection.
     */
    BOOL CanAutoStartTransmitData() const { return autoStartTransmitData; }

    /**See if should automatically do call forward of connection.
     */
    BOOL CanAutoCallForward() const { return autoCallForward; }

    /**Get the current capability table for this endpoint.
     */
    const H323Capabilities & GetCapabilities() const;

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

    /**Get the default maximum audio jitter delay parameter.
       Defaults to 50ms
     */
    unsigned GetMinAudioJitterDelay() const { return manager.GetMinAudioJitterDelay(); }

    /**Get the default maximum audio delay jitter parameter.
       Defaults to 250ms.
     */
    unsigned GetMaxAudioJitterDelay() const { return manager.GetMaxAudioJitterDelay(); }

    /**Set the maximum audio delay jitter parameter.
     */
    void SetAudioJitterDelay(
      unsigned minDelay,   ///<  New minimum jitter buffer delay in milliseconds
      unsigned maxDelay    ///<  New maximum jitter buffer delay in milliseconds
    ) { manager.SetAudioJitterDelay(minDelay, maxDelay); }

    /**Get the initial bandwidth parameter.
     */
    unsigned GetInitialBandwidth() const { return initialBandwidth; }

    /**Get the initial bandwidth parameter.
     */
    void SetInitialBandwidth(unsigned bandwidth) { initialBandwidth = bandwidth; }

    /**Called when an outgoing PDU requires a feature set
     */
    virtual BOOL OnSendFeatureSet(unsigned, H225_FeatureSet &);

    /**Called when an incoming PDU contains a feature set
     */
    virtual void OnReceiveFeatureSet(unsigned, const H225_FeatureSet &);
	
#ifdef H323_H460
    /** Get the Endpoing FeatureSet
     */
    H460_FeatureSet & GetFeatureSet() { return features; };
#endif

    /**Determine if the address is "local", ie does not need STUN
     */
    virtual BOOL IsLocalAddress(
      const PIPSocket::Address & remoteAddress
    ) const { return manager.IsLocalAddress(remoteAddress); }

    /**Provide TCP address translation hook
     */
    virtual void TranslateTCPAddress(
      PIPSocket::Address & localAddr,
      const PIPSocket::Address & remoteAddr
    );

    /**Get the TCP port number base for H.245 channels
     */
    WORD GetTCPPortBase() const { return manager.GetTCPPortBase(); }

    /**Get the TCP port number base for H.245 channels.
     */
    WORD GetTCPPortMax() const { return manager.GetTCPPortMax(); }

    /**Set the TCP port number base and max for H.245 channels.
     */
    void SetTCPPorts(unsigned tcpBase, unsigned tcpMax) { manager.SetTCPPorts(tcpBase, tcpMax); }

    /**Get the next TCP port number for H.245 channels
     */
    WORD GetNextTCPPort() { return manager.GetNextTCPPort(); }

    /**Get the UDP port number base for RAS channels
     */
    WORD GetUDPPortBase() const { return manager.GetUDPPortBase(); }

    /**Get the UDP port number base for RAS channels.
     */
    WORD GetUDPPortMax() const { return manager.GetUDPPortMax(); }

    /**Set the TCP port number base and max for H.245 channels.
     */
    void SetUDPPorts(unsigned udpBase, unsigned udpMax) { manager.SetUDPPorts(udpBase, udpMax); }

    /**Get the next UDP port number for RAS channels
     */
    WORD GetNextUDPPort() { return manager.GetNextUDPPort(); }

    /**Get the UDP port number base for RTP channels.
     */
    WORD GetRtpIpPortBase() const { return manager.GetRtpIpPortBase(); }

    /**Get the max UDP port number for RTP channels.
     */
    WORD GetRtpIpPortMax() const { return manager.GetRtpIpPortMax(); }

    /**Set the UDP port number base and max for RTP channels.
     */
    void SetRtpIpPorts(unsigned udpBase, unsigned udpMax) { manager.SetRtpIpPorts(udpBase, udpMax); }

    /**Get the UDP port number pair for RTP channels.
     */
    WORD GetRtpIpPortPair() { return manager.GetRtpIpPortPair(); }

    /**Get the IP Type Of Service byte for RTP channels.
     */
    BYTE GetRtpIpTypeofService() const { return manager.GetRtpIpTypeofService(); }

    /**Set the IP Type Of Service byte for RTP channels.
     */
    void SetRtpIpTypeofService(unsigned tos) { manager.SetRtpIpTypeofService(tos); }

    /**Get the default timeout for calling another endpoint.
     */
    const PTimeInterval & GetSignallingChannelCallTimeout() const { return signallingChannelCallTimeout; }

    /**Get the default timeout for incoming H.245 connection.
     */
    const PTimeInterval & GetControlChannelStartTimeout() const { return controlChannelStartTimeout; }

    /**Get the default timeout for waiting on an end session.
     */
    const PTimeInterval & GetEndSessionTimeout() const { return endSessionTimeout; }

    /**Get the default timeout for master slave negotiations.
     */
    const PTimeInterval & GetMasterSlaveDeterminationTimeout() const { return masterSlaveDeterminationTimeout; }

    /**Get the default retries for H245 master slave negotiations.
     */
    unsigned GetMasterSlaveDeterminationRetries() const { return masterSlaveDeterminationRetries; }

    /**Get the default timeout for H245 capability exchange negotiations.
     */
    const PTimeInterval & GetCapabilityExchangeTimeout() const { return capabilityExchangeTimeout; }

    /**Get the default timeout for H245 logical channel negotiations.
     */
    const PTimeInterval & GetLogicalChannelTimeout() const { return logicalChannelTimeout; }

    /**Get the default timeout for H245 request mode negotiations.
     */
    const PTimeInterval & GetRequestModeTimeout() const { return logicalChannelTimeout; }

    /**Get the default timeout for H245 round trip delay negotiations.
     */
    const PTimeInterval & GetRoundTripDelayTimeout() const { return roundTripDelayTimeout; }

    /**Get the default rate H245 round trip delay is calculated by connection.
     */
    const PTimeInterval & GetRoundTripDelayRate() const { return roundTripDelayRate; }

    /**Get the flag for clearing a call if the round trip delay calculation fails.
     */
    BOOL ShouldClearCallOnRoundTripFail() const { return clearCallOnRoundTripFail; }

    /**Get the amount of time with no media that should cause call to clear
     */
    const PTimeInterval & GetNoMediaTimeout() const { return manager.GetNoMediaTimeout(); }

    /**Set the amount of time with no media that should cause call to clear
     */
    BOOL SetNoMediaTimeout(
      const PTimeInterval & newInterval  ///<  New timeout for media
    ) { return manager.SetNoMediaTimeout(newInterval); }

    /**Get the default timeout for GatekeeperRequest and Gatekeeper discovery.
     */
    const PTimeInterval & GetGatekeeperRequestTimeout() const { return gatekeeperRequestTimeout; }

    /**Get the default retries for GatekeeperRequest and Gatekeeper discovery.
     */
    unsigned GetGatekeeperRequestRetries() const { return gatekeeperRequestRetries; }

    /**Get the default timeout for RAS protocol transactions.
     */
    const PTimeInterval & GetRasRequestTimeout() const { return rasRequestTimeout; }

    /**Get the default retries for RAS protocol transations.
     */
    unsigned GetRasRequestRetries() const { return rasRequestRetries; }

    /**Get the default time for gatekeeper to reregister.
       A value of zero disables the keep alive facility.
     */
    const PTimeInterval & GetGatekeeperTimeToLive() const { return registrationTimeToLive; }

    /**Set the default time for gatekeeper to reregister.
       A value of zero disables the keep alive facility.
     */
    void SetGatekeeperTimeToLive(const PTimeInterval & ttl) { registrationTimeToLive = ttl; }

    /**Get the iNow Gatekeeper Access Token OID.
     */
    const PString & GetGkAccessTokenOID() const { return gkAccessTokenOID; }

    /**Set the iNow Gatekeeper Access Token OID.
     */
    void SetGkAccessTokenOID(const PString & token) { gkAccessTokenOID = token; }

    /**Get flag to indicate whether to send GRQ on gatekeeper registration
     */
    BOOL GetSendGRQ() const
    { return sendGRQ; }

    /**Sent flag to indicate whether to send GRQ on gatekeeper registration
     */
    void SetSendGRQ(BOOL v) 
    { sendGRQ = v; }

    /**Get the default timeout for Call Transfer Timer CT-T1.
     */
    const PTimeInterval & GetCallTransferT1() const { return callTransferT1; }

    /**Get the default timeout for Call Transfer Timer CT-T2.
     */
    const PTimeInterval & GetCallTransferT2() const { return callTransferT2; }

    /**Get the default timeout for Call Transfer Timer CT-T3.
     */
    const PTimeInterval & GetCallTransferT3() const { return callTransferT3; }

    /**Get the default timeout for Call Transfer Timer CT-T4.
     */
    const PTimeInterval & GetCallTransferT4() const { return callTransferT4; }

    /** Get Call Intrusion timers timeout */
    const PTimeInterval & GetCallIntrusionT1() const { return callIntrusionT1; }
    const PTimeInterval & GetCallIntrusionT2() const { return callIntrusionT2; }
    const PTimeInterval & GetCallIntrusionT3() const { return callIntrusionT3; }
    const PTimeInterval & GetCallIntrusionT4() const { return callIntrusionT4; }
    const PTimeInterval & GetCallIntrusionT5() const { return callIntrusionT5; }
    const PTimeInterval & GetCallIntrusionT6() const { return callIntrusionT6; }

    /**Get the dictionary of <callIdentities, connections>
     */
    H323CallIdentityDict& GetCallIdentityDictionary() { return secondaryConnectionsActive; }

    /**Get the next available invoke Id for H450 operations
      */
#if OPAL_H450
    unsigned GetNextH450CallIdentityValue() const { return ++nextH450CallIdentity; }
#endif

  //@}

  protected:
    H323Gatekeeper * InternalCreateGatekeeper(H323Transport * transport);
    BOOL InternalRegisterGatekeeper(H323Gatekeeper * gk, BOOL discovered);
    BOOL InternalMakeCall(
      OpalCall & call,
      const PString & existingToken,           ///<  Existing connection to be transferred
      const PString & callIdentity,            ///<  Call identity of the secondary call (if it exists)
      unsigned capabilityLevel,                ///<  Intrusion capability level
      const PString & remoteParty,             ///<  Remote party to call
      void * userData,                         ///<  user data to pass to CreateConnection
      unsigned int options = 0,                ///<  options to pass to connection
      OpalConnection::StringOptions * stringOptions = NULL ///<  complex string options
    );

    // Configuration variables, commonly changed
    PStringList localAliasNames;
    PMutex      localAliasNamesMutex;
    BOOL        autoStartReceiveData;
    BOOL        autoStartTransmitData;
    BOOL        autoCallForward;
    BOOL        disableFastStart;
    BOOL        disableH245Tunneling;
    BOOL        disableH245inSetup;
    BOOL        m_bH245Disabled; /* enabled or disabled h245 */
    BOOL        canDisplayAmountString;
    BOOL        canEnforceDurationLimit;
#if OPAL_H450
    unsigned    callIntrusionProtectionLevel;
#endif

    TerminalTypes terminalType;

    BOOL          clearCallOnRoundTripFail;

    // Some more configuration variables, rarely changed.
    PTimeInterval signallingChannelCallTimeout;
    PTimeInterval controlChannelStartTimeout;
    PTimeInterval endSessionTimeout;
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

    PString       gkAccessTokenOID;
    BOOL          sendGRQ;

    /* Protect against absence of a response to the ctIdentify reqest
       (Transferring Endpoint - Call Transfer with a secondary Call) */
    PTimeInterval callTransferT1;
    /* Protect against failure of completion of the call transfer operation
       involving a secondary Call (Transferred-to Endpoint) */
    PTimeInterval callTransferT2;
    /* Protect against failure of the Transferred Endpoint not responding
       within sufficient time to the ctInitiate APDU (Transferring Endpoint) */
    PTimeInterval callTransferT3;
    /* May optionally operate - protects against absence of a response to the
       ctSetup request (Transferred Endpoint) */
    PTimeInterval callTransferT4;

    /** Call Intrusion Timers */
    PTimeInterval callIntrusionT1;
    PTimeInterval callIntrusionT2;
    PTimeInterval callIntrusionT3;
    PTimeInterval callIntrusionT4;
    PTimeInterval callIntrusionT5;
    PTimeInterval callIntrusionT6;

    // Dynamic variables
    mutable H323Capabilities capabilities;
    H323Gatekeeper *     gatekeeper;
    PString              gatekeeperUsername;
    PString              gatekeeperPassword;
    H323CallIdentityDict secondaryConnectionsActive;

#if OPAL_H450
    mutable PAtomicInteger nextH450CallIdentity;
            /// Next available callIdentity for H450 Transfer operations via consultation.
#endif
	
#if OPAL_H460
    H460_FeatureSet features;
#endif

};


#if P_SSL

class H323SecureEndPoint : public H323EndPoint
{
  PCLASSINFO(H323SecureEndPoint, H323EndPoint);
  public:
    enum {
      DefaultTcpSignalPort = 1300
    };

    H323SecureEndPoint(OpalManager & _manager)
      : H323EndPoint(_manager, "h323s", DefaultTcpSignalPort)
    {
    }

    PString GetDefaultTransport() const
    {  return "tcps$"; }
  //@}
};

#endif // P_SSL

#endif // __OPAL_H323EP_H


/////////////////////////////////////////////////////////////////////////////
