/*
 * gkclient.h
 *
 * Gatekeeper client protocol handler
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
 * iFace, Inc. http://www.iface.com
 *
 * Contributor(s): ______________________________________.
 *
 * $Log: gkclient.h,v $
 * Revision 1.2001  2001/07/27 15:48:24  robertj
 * Conversion of OpenH323 to Open Phone Abstraction Library (OPAL)
 *
 * Revision 1.19  2001/06/18 06:23:47  robertj
 * Split raw H.225 RAS protocol out of gatekeeper client class.
 *
 * Revision 1.18  2001/04/05 03:39:42  robertj
 * Fixed deadlock if tried to do discovery in time to live timeout.
 *
 * Revision 1.17  2001/03/28 07:12:56  robertj
 * Changed RAS thread interlock to allow for what should not happen, the
 *   syncpoint being signalled before receiving any packets.
 *
 * Revision 1.16  2001/03/27 02:18:41  robertj
 * Changed to send gk a GRQ if it gives a discoveryRequired error on RRQ.
 *
 * Revision 1.15  2001/03/17 00:05:52  robertj
 * Fixed problems with Gatekeeper RIP handling.
 *
 * Revision 1.14  2001/02/28 00:20:15  robertj
 * Added DiscoverByNameAndAddress() function, thanks Chris Purvis.
 *
 * Revision 1.13  2001/02/09 05:16:24  robertj
 * Added #pragma interface for GNU C++.
 *
 * Revision 1.12  2000/09/25 06:47:54  robertj
 * Removed use of alias if there is no alias present, ie only have transport address.
 *
 * Revision 1.11  2000/09/01 02:12:54  robertj
 * Added ability to select a gatekeeper on LAN via it's identifier name.
 *
 * Revision 1.10  2000/07/11 19:20:02  robertj
 * Fixed problem with endpoint identifiers from some gatekeepers not being a string, just binary info.
 *
 * Revision 1.9  2000/06/20 03:17:56  robertj
 * Added function to get name of gatekeeper, subtle difference from getting identifier.
 *
 * Revision 1.8  2000/05/18 11:53:33  robertj
 * Changes to support doc++ documentation generation.
 *
 * Revision 1.7  2000/05/09 08:52:36  robertj
 * Added support for preGrantedARQ fields on registration.
 *
 * Revision 1.6  2000/05/02 04:32:24  robertj
 * Fixed copyright notice comment.
 *
 * Revision 1.5  2000/04/11 03:10:40  robertj
 * Added ability to reject reason on gatekeeper requests.
 * Added ability to get the transport being used to talk to the gatekeeper.
 *
 * Revision 1.4  2000/04/10 17:37:13  robertj
 * Added access function to get the gatekeeper identification string.
 *
 * Revision 1.3  1999/12/09 21:49:17  robertj
 * Added reregister on unregister and time to live reregistration
 *
 * Revision 1.2  1999/09/14 06:52:54  robertj
 * Added better support for multi-homed client hosts.
 *
 * Revision 1.1  1999/08/31 12:34:18  robertj
 * Added gatekeeper support.
 *
 */

#ifndef __H323_GKCLIENT_H
#define __H323_GKCLIENT_H

#ifdef __GNUC__
#pragma interface
#endif


#include <h323/h225ras.h>


class H323Connection;


///////////////////////////////////////////////////////////////////////////////

/**This class embodies the H.225.0 RAS protocol to gatekeepers.
  */
class H323Gatekeeper : public H225_RAS
{
  PCLASSINFO(H323Gatekeeper, H225_RAS);
  public:
  /**@name Construction */
  //@{
    /**Create a new gatekeeper.
     */
    H323Gatekeeper(
      H323EndPoint & endpoint,  /// Endpoint gatekeeper is associated with.
      OpalTransport * transport       /// Transport over which gatekeepers communicates.
    );

    /**Destroy gatekeeper.
     */
    ~H323Gatekeeper();
  //@}

  /**@name Overrides from PObject */
  //@{
    /**Print the name of the gatekeeper.
      */
    void PrintOn(
      ostream & strm    /// Stream to print to.
    ) const;
  //@}

  /**@name Overrides from H225_RAS */
  //@{
    BOOL OnReceiveGatekeeperConfirm(const H225_GatekeeperConfirm & gcf);
    BOOL OnReceiveRegistrationConfirm(const H225_RegistrationConfirm & rcf);
    BOOL OnReceiveRegistrationReject(const H225_RegistrationReject & rrj);
    void OnReceiveUnregistrationRequest(const H225_UnregistrationRequest & urq);
    BOOL OnReceiveUnregistrationConfirm(const H225_UnregistrationConfirm & ucf);
    BOOL OnReceiveUnregistrationReject(const H225_UnregistrationReject & urj);
    BOOL OnReceiveAdmissionConfirm(const H225_AdmissionConfirm & acf);
    void OnReceiveDisengageRequest(const H225_DisengageRequest & drq);
    BOOL OnReceiveBandwidthConfirm(const H225_BandwidthConfirm & bcf);
    void OnReceiveBandwidthRequest(const H225_BandwidthRequest & brq);
    void OnReceiveInfoRequest(const H225_InfoRequest & irq);
  //@}

  /**@name Protocol operations */
  //@{
    /**Discover a gatekeeper on the local network.
     */
    BOOL DiscoverAny();

    /**Discover a gatekeeper on the local network.
       If the identifier string is empty then the first gatekeeper to respond
       to a broadcast is used.
     */
    BOOL DiscoverByName(
      const PString & identifier  /// Gatekeeper identifier to find
    );

    /**Discover a gatekeeper on the local network.
       If the address string is empty then the first gatekeeper to respond
       to a broadcast is used.
     */
    BOOL DiscoverByAddress(
      const OpalTransportAddress & address /// Address of gatekeeper.
    );

    /**Discover a gatekeeper on the local network.
       Combination of DiscoverByName() and DiscoverByAddress().
     */
    BOOL DiscoverByNameAndAddress(
      const PString & identifier,
      const OpalTransportAddress & address
    );

    /**Register with gatekeeper.
     */
    BOOL RegistrationRequest(
      BOOL autoReregister = TRUE  /// Automatic register on unregister
    );

    /**Unregister with gatekeeper.
     */
    BOOL UnregistrationRequest(
      int reason      /// Reason for unregistration
    );

    /**Location request to gatekeeper.
     */
    BOOL LocationRequest(
      const PString & alias,          /// Alias name we wish to find.
      OpalTransportAddress & address  /// Resultant transport address.
    );

    /**Location request to gatekeeper.
     */
    BOOL LocationRequest(
      const PStringList & aliases,    /// Alias names we wish to find.
      OpalTransportAddress & address  /// Resultant transport address.
    );

    /**Admission request to gatekeeper.
     */
    BOOL AdmissionRequest(
      H323Connection & connection  /// Connection we wish to change.
    );


    /**Admission request to gatekeeper.
     */
    BOOL AdmissionRequest(
      H323Connection & connection,     /// Connection we wish to change.
      H323TransportAddress & address,  /// Gatekeeper routed transport address.
      BOOL ignorePreGrantedARQ = FALSE /// Flag to force ARQ to be sent
    );

    /**Disengage request to gatekeeper.
     */
    BOOL DisengageRequest(
      const H323Connection & connection,  /// Connection we wish admitted.
      unsigned reason                     /// Reason code for disengage
    );

    /**Bandwidth request to gatekeeper.
     */
    BOOL BandwidthRequest(
      H323Connection & connection,    /// Connection we wish to change.
      unsigned requestedBandwidth     /// New bandwidth wanted in 0.1kbps
    );

    /**Send an info response to the gatekeeper.
     */
    void InfoRequestResponse(
      const H323Connection * connection,  /// Connection to send info about
      unsigned seqNum                     /// Sequence number responding to
    );
  //@}

  /**@name Member variable access */
  //@{
    /**Determine if the endpoint is registered with the gatekeeper.
      */
    BOOL IsRegistered() const { return isRegistered; }

    /**Get the gatekeeper name.
       The gets the name of the gatekeeper. It will be of the form id@address
       where id is the gatekeeperIdentifier and address is the transport
       address used. If the gatekeeperIdentifier is empty the '@' is not
       included and only the transport is shown. The transport is minimised
       also, with the type removed if IP is used and the :port removed if the
       default port is used.
      */
    PString GetName() const;
  //@}


  protected:
    BOOL StartDiscovery(const OpalTransportAddress & address);
    BOOL DiscoverGatekeeper(H323RasPDU & request, const OpalTransportAddress & address);
    void SetupGatekeeperRequest(H323RasPDU & request);
    PDECLARE_NOTIFIER(PTimer, H323Gatekeeper, RegistrationTimeToLive);


    // Gatekeeper registration state variables
    BOOL    discoveryComplete;
    BOOL    isRegistered;
    PWORDArray endpointIdentifier;
    H323TransportAddress gkRouteAddress;

    // Gatekeeper operation variables
    BOOL    autoReregister;
    PTimer  timeToLive;
    BOOL    requiresDiscovery;

    // Inter-thread transfer variables
    unsigned allocatedBandwidth;
};


PLIST(H323GatekeeperList, H323Gatekeeper);


#endif // __H323_GKCLIENT_H


/////////////////////////////////////////////////////////////////////////////
