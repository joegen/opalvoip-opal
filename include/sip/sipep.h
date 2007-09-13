/*
 * sipep.h
 *
 * Session Initiation Protocol endpoint.
 *
 * Open Phone Abstraction Library (OPAL)
 * Formally known as the Open H323 project.
 *
 * Copyright (c) 2001 Equivalence Pty. Ltd.
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
 * The Initial Developer of the Original Code is Equivalence Pty. Ltd.
 *
 * Contributor(s): ______________________________________.
 *
 * $Log: sipep.h,v $
 * Revision 1.2069.2.14  2007/09/13 14:21:00  hfriederich
 * Allow to force the creation of a new handler when registering, even
 * if there already exists a handler for the target
 *
 * Revision 2.68.2.13  2007/09/08 10:15:44  hfriederich
 * Fix propagation of transaction failures (timeouts, retries exceeded)
 * and some code cleanup
 *
 * Revision 2.68.2.12  2007/09/07 12:56:17  hfriederich
 * Backports from HEAD
 *
 * Revision 2.68.2.11  2007/08/05 13:12:17  hfriederich
 * Backport from HEAD - Changes since last commit
 *
 * Revision 2.68.2.10  2007/06/12 16:29:02  hfriederich
 * (Backport from HEAD)
 * Major rework of how SIP utilises sockets, using new "socket bundling"
 *   subsystem
 * Several other bugfixes
 *
 * Revision 2.68.2.9  2007/05/30 08:40:09  hfriederich
 * (Backport from HEAD)
 * Changes since May 1, 2007. Including Presence code
 *
 * Revision 2.68.2.8  2007/05/04 09:51:29  hfriederich
 * Backport from HEAD - Changes since Apr 1, 2007
 *
 * Revision 2.68.2.7  2007/04/10 19:00:57  hfriederich
 * Reorganization of the way transaction and transaction transitions are
 *   handled. More testing needed.
 *
 * Revision 2.68.2.6  2007/03/30 13:56:36  hfriederich
 * Reorganization of the way transactions are handled. Delete transactions
 *   in garbage collector when they're terminated. Update destructor code
 *   to improve safe destruction of SIPEndPoint instances.
 *
 * Revision 2.68.2.5  2007/03/29 21:47:23  hfriederich
 * (Backport from HEAD)
 * Various fixes on the way SIPInfo objects are being handled. Wait for
 *   transports to be closed before being deleted. Added missing mutexes.
 *   Added garbage collector.
 * Pass OpalConnection to OpalMediaSream constructor
 * Add ID to OpalMediaStreams so that transcoders can match incoming and
 *   outgoing codecs
 *
 * Revision 2.68.2.4  2007/03/20 12:56:23  hfriederich
 * Backport from HEAD
 *
 * Revision 2.68.2.3  2007/03/18 19:17:22  hfriederich
 * (Backport from HEAD)
 * Improved locking
 * Take the remote port into account when unregistering
 * Keept the transport open for a longer time when doing text conversations.
 *
 * Revision 2.68.2.2  2007/03/10 08:34:01  hfriederich
 * (Backport from HEAD)
 * Connection-specific jitter buffer values
 *   Thanks to Borko Jandras
 * Added OnIncomingMediaChannels so incoming calls can optionally be handled
 *   in two stages
 * Fixed backward compatibility of OnIncomingConnection()
 * Don't send multiple 100 Trying
 * Guard against inability to create transports
 * Added missing locking
 *
 * Revision 2.68.2.1  2007/02/15 09:43:16  hfriederich
 * Make CreateTransport() virtual
 *
 * Revision 2.68  2007/01/24 04:00:56  csoutheren
 * Arrrghh. Changing OnIncomingConnection turned out to have a lot of side-effects
 * Added some pure viritual functions to prevent old code from breaking silently
 * New OpalEndpoint and OpalConnection descendants will need to re-implement
 * OnIncomingConnection. Sorry :)
 *
 * Revision 2.67  2007/01/18 04:45:16  csoutheren
 * Messy, but simple change to add additional options argument to OpalConnection constructor
 * This allows the provision of non-trivial arguments for connections
 *
 * Revision 2.66  2007/01/02 15:26:13  dsandras
 * Added DNS Fallback for realm authentication if the classical comparison
 * doesn't work. Fixes problems with broken SIP proxies. (Ekiga report #377346)
 *
 * Revision 2.65  2006/12/18 03:18:41  csoutheren
 * Messy but simple fixes
 *   - Add access to SIP REGISTER timeout
 *   - Ensure OpalConnection options are correctly progagated
 *
 * Revision 2.64  2006/11/09 18:24:55  hfriederich
 * Ensures that responses to received INVITE get sent out on the same network interface as where the INVITE was received. Remove cout statement
 *
 * Revision 2.63  2006/08/12 04:20:39  csoutheren
 * Removed Windows warning and fixed timeout in SIP PING code
 *
 * Revision 2.62  2006/08/12 04:09:24  csoutheren
 * Applied 1538497 - Add the PING method
 * Thanks to Paul Rolland
 *
 * Revision 2.61  2006/07/29 08:49:25  hfriederich
 * Abort the authentication process after a given number of unsuccessful authentication attempts to prevent infinite authentication attempts. Use not UINT_MAX as GetExpires() default to fix incorrect detection of registration/unregistration
 *
 * Revision 2.60  2006/07/24 14:03:39  csoutheren
 * Merged in audio and video plugins from CVS branch PluginBranch
 *
 * Revision 2.59  2006/07/14 04:22:43  csoutheren
 * Applied 1517397 - More Phobos stability fix
 * Thanks to Dinis Rosario
 *
 * Revision 2.58  2006/04/30 17:24:39  dsandras
 * Various clean ups.
 *
 * Revision 2.57  2006/04/11 21:58:25  dsandras
 * Various cleanups and fixes. Fixes Ekiga report #336444.
 *
 * Revision 2.56  2006/03/27 20:28:18  dsandras
 * Added mutex to fix concurrency issues between OnReceivedPDU which checks
 * if a connection is in the list, and OnReceivedINVITE, which adds it to the
 * list. Fixes Ekiga report #334847. Thanks Robert for your input on this!
 *
 * Revision 2.55  2006/03/19 18:57:06  dsandras
 * More work on Ekiga report #334999.
 *
 * Revision 2.54  2006/03/19 17:26:15  dsandras
 * Fixed FindSIPInfoByDomain so that it doesn't return unregistered accounts.
 * Fixes Ekiga report #335006.
 *
 * Revision 2.53  2006/03/19 12:32:05  dsandras
 * RFC3261 says that "CANCEL messages "SHOULD NOT" be sent for anything but INVITE
 * requests". Fixes Ekiga report #334985.
 *
 * Revision 2.52  2006/03/19 11:45:47  dsandras
 * The remote address of the registrar transport might have changed due
 * to the Via field. This affected unregistering which was reusing
 * the exact same transport to unregister. Fixed Ekiga report #334999.
 *
 * Revision 2.51  2006/03/06 22:52:59  csoutheren
 * Reverted experimental SRV patch due to unintended side effects
 *
 * Revision 2.50  2006/03/06 19:01:30  dsandras
 * Allow registering several accounts with the same realm but different
 * user names to the same provider. Fixed possible crash due to transport
 * deletion before the transaction is over.
 *
 * Revision 2.49  2006/03/06 12:56:02  csoutheren
 * Added experimental support for SIP SRV lookups
 *
 * Revision 2.48.2.2  2006/04/06 05:33:07  csoutheren
 * Backports from CVS head up to Plugin_Merge2
 *
 * Revision 2.48.2.1  2006/03/20 02:25:26  csoutheren
 * Backports from CVS head
 *
 * Revision 2.56  2006/03/27 20:28:18  dsandras
 * Added mutex to fix concurrency issues between OnReceivedPDU which checks
 * if a connection is in the list, and OnReceivedINVITE, which adds it to the
 * list. Fixes Ekiga report #334847. Thanks Robert for your input on this!
 *
 * Revision 2.55  2006/03/19 18:57:06  dsandras
 * More work on Ekiga report #334999.
 *
 * Revision 2.54  2006/03/19 17:26:15  dsandras
 * Fixed FindSIPInfoByDomain so that it doesn't return unregistered accounts.
 * Fixes Ekiga report #335006.
 *
 * Revision 2.53  2006/03/19 12:32:05  dsandras
 * RFC3261 says that "CANCEL messages "SHOULD NOT" be sent for anything but INVITE
 * requests". Fixes Ekiga report #334985.
 *
 * Revision 2.52  2006/03/19 11:45:47  dsandras
 * The remote address of the registrar transport might have changed due
 * to the Via field. This affected unregistering which was reusing
 * the exact same transport to unregister. Fixed Ekiga report #334999.
 *
 * Revision 2.51  2006/03/06 22:52:59  csoutheren
 * Reverted experimental SRV patch due to unintended side effects
 *
 * Revision 2.50  2006/03/06 19:01:30  dsandras
 * Allow registering several accounts with the same realm but different
 * user names to the same provider. Fixed possible crash due to transport
 * deletion before the transaction is over.
 *
 * Revision 2.49  2006/03/06 12:56:02  csoutheren
 * Added experimental support for SIP SRV lookups
 *
 * Revision 2.48  2006/02/19 11:51:46  dsandras
 * Fixed FindSIPInfoByDomain.
 *
 * Revision 2.47  2006/01/29 20:55:32  dsandras
 * Allow using a simple username or a fill url when registering.
 *
 * Revision 2.46  2006/01/08 21:53:40  dsandras
 * Changed IsRegistered so that it takes the registration url as argument,
 * allowing it to work when there are several accounts on the same server.
 *
 * Revision 2.45  2006/01/08 14:43:46  dsandras
 * Improved the NAT binding refresh methods so that it works with all endpoint
 * created transports that require it and so that it can work by sending
 * SIP Options, or empty SIP requests. More methods can be added later.
 *
 * Revision 2.44  2006/01/02 11:28:07  dsandras
 * Some documentation. Various code cleanups to prevent duplicate code.
 *
 * Revision 2.43  2005/12/18 21:06:56  dsandras
 * Added function to clean up the registrations object. Moved DeleteObjectsToBeRemoved call outside of the loop.
 *
 * Revision 2.42  2005/12/14 17:59:50  dsandras
 * Added ForwardConnection executed when the remote asks for a call forwarding.
 * Similar to what is done in the H.323 part with the method of the same name.
 *
 * Revision 2.41  2005/12/11 19:14:20  dsandras
 * Added support for setting a different user name and authentication user name
 * as required by some providers like digisip.
 *
 * Revision 2.40  2005/12/08 21:14:54  dsandras
 * Added function allowing to change the nat binding refresh timeout.
 *
 * Revision 2.39  2005/12/05 22:20:57  dsandras
 * Update the transport when the computer is behind NAT, using STUN, the IP
 * address has changed compared to the original transport and a registration
 * refresh must occur.
 *
 * Revision 2.38  2005/12/04 22:08:58  dsandras
 * Added possibility to provide an expire time when registering, if not
 * the default expire time for the endpoint will be used.
 *
 * Revision 2.37  2005/11/30 13:35:26  csoutheren
 * Changed tags for Doxygen
 *
 * Revision 2.36  2005/11/28 19:07:56  dsandras
 * Moved OnNATTimeout to SIPInfo and use it for active conversations too.
 * Added E.164 support.
 *
 * Revision 2.35  2005/10/30 23:01:29  dsandras
 * Added possibility to have a body for SIPInfo. Moved MESSAGE sending to 
 * SIPInfo for more efficiency during conversations.
 *
 * Revision 2.34  2005/10/22 17:14:45  dsandras
 * Send an OPTIONS request periodically when STUN is being used to maintain the 
 * registrations binding alive.
 *
 * Revision 2.33  2005/10/20 20:26:58  dsandras
 * Made the transactions handling thread-safe.
 *
 * Revision 2.32  2005/10/02 21:46:20  dsandras
 * Added more doc.
 *
 * Revision 2.31  2005/10/02 17:47:37  dsandras
 * Added function to return the translated contact address of the endpoint.
 * Added some doc.
 *
 * Revision 2.30  2005/05/23 20:14:05  dsandras
 * Added preliminary support for basic instant messenging.
 *
 * Revision 2.29  2005/05/06 07:37:06  csoutheren
 * Various changed while working with SIP carrier
 *   - remove assumption that authentication realm is a domain name.
 *   - stopped rewrite of "To" field when proxy being used
 *   - fix Contact field in REGISTER to match actual port used 
 *     when Symmetric NATin use
 *   - lots of formatting changes and cleanups
 *
 * Revision 2.28  2005/05/03 20:41:51  dsandras
 * Do not count SUBSCRIBEs when returning the number of registered accounts.
 *
 * Revision 2.27  2005/04/28 20:22:53  dsandras
 * Applied big sanity patch for SIP thanks to 
 * Ted Szoczei <tszoczei@microtronix.ca>.
 * Thanks a lot!
 *
 * Revision 2.25  2005/04/26 19:50:54  dsandras
 * Added function to return the number of registered accounts.
 *
 * Revision 2.24  2005/04/10 21:01:09  dsandras
 * Added Blind Transfer support.
 *
 * Revision 2.23  2005/03/11 18:12:08  dsandras
 * Added support to specify the realm when registering. That way 
 * softphones already know what authentication information to use when required. 
 * The realm/domain can also be used in the From field.
 *
 * Revision 2.22  2005/02/19 22:48:48  dsandras
 * Added the possibility to register to several registrars and be able to do 
 * authenticated calls to each of them. Added SUBSCRIBE/NOTIFY support for 
 * Message Waiting Indications.
 *
 * Revision 2.21  2004/12/17 12:06:52  dsandras
 * Added error code to OnRegistrationFailed. Made Register/Unregister wait 
 * until the transaction is over. Fixed Unregister so that the SIPRegister
 * is used as a pointer or the object is deleted at the end of the function 
 * and make Opal crash when transactions are cleaned. Reverted part of the 
 * patch that was sending authentication again when it had already been done 
 * on a Register.
 *
 * Revision 2.20  2004/12/12 12:30:09  dsandras
 * Added virtual function called when registration to a registrar fails.
 *
 * Revision 2.19  2004/11/29 08:18:31  csoutheren
 * Added support for setting the SIP authentication domain/realm as needed 
 * for many service providers
 *
 * Revision 2.18  2004/10/02 04:30:10  rjongbloed
 * Added unregister function for SIP registrar
 *
 * Revision 2.17  2004/08/22 12:27:44  rjongbloed
 * More work on SIP registration, time to live refresh and deregistration 
 * on exit.
 *
 * Revision 2.16  2004/08/14 07:56:30  rjongbloed
 * Major revision to utilise the PSafeCollection classes for the connections 
 * and calls.
 *
 * Revision 2.15  2004/07/11 12:42:10  rjongbloed
 * Added function on endpoints to get the list of all media formats any
 *   connection the endpoint may create can support.
 *
 * Revision 2.14  2004/06/05 14:36:32  rjongbloed
 * Added functions to get registration URL.
 * Added ability to set proxy bu host/user/password strings.
 *
 * Revision 2.13  2004/04/26 06:30:33  rjongbloed
 * Added ability to specify more than one defualt listener for an endpoint,
 *   required by SIP which listens on both UDP and TCP.
 *
 * Revision 2.12  2004/04/26 05:40:38  rjongbloed
 * Added RTP statistics callback to SIP
 *
 * Revision 2.11  2004/03/14 11:32:19  rjongbloed
 * Changes to better support SIP proxies.
 *
 * Revision 2.10  2004/03/14 10:13:03  rjongbloed
 * Moved transport on SIP top be constructed by endpoint as any transport
 * created on an endpoint can receive data for any connection.
 * Changes to REGISTER to support authentication
 *
 * Revision 2.9  2004/03/14 08:34:09  csoutheren
 * Added ability to set User-Agent string
 *
 * Revision 2.8  2004/03/13 06:51:31  rjongbloed
 * Alllowed for empty "username" in registration
 *
 * Revision 2.7  2004/03/13 06:32:17  rjongbloed
 * Fixes for removal of SIP and H.323 subsystems.
 * More registration work.
 *
 * Revision 2.6  2003/03/06 03:57:47  robertj
 * IVR support (work in progress) requiring large changes everywhere.
 *
 * Revision 2.5  2002/09/16 02:52:35  robertj
 * Added #define so can select if #pragma interface/implementation is used on
 *   platform basis (eg MacOS) rather than compiler, thanks Robert Monaghan.
 *
 * Revision 2.4  2002/07/04 07:41:47  robertj
 * Fixed memory/thread leak of transports.
 *
 * Revision 2.3  2002/04/16 08:06:35  robertj
 * Fixed GNU warnings.
 *
 * Revision 2.2  2002/04/05 10:42:04  robertj
 * Major changes to support transactions (UDP timeouts and retries).
 *
 * Revision 2.1  2002/02/01 04:53:01  robertj
 * Added (very primitive!) SIP support.
 *
 */

#ifndef __OPAL_SIPEP_H
#define __OPAL_SIPEP_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif

#ifndef _PTLIB_H
#include <ptlib.h>
#endif

#include <opal/endpoint.h>
#include <sip/sipcon.h>
#include <sip/sippdu.h>
#include <sip/handlers.h>

//
// provide flag so applications know if presence is available
//
#define OPAL_HAS_SIP_PRESENCE 1
 
/////////////////////////////////////////////////////////////////////////

/** Class to hold authentication information 
  */

class SIPAuthInfo : public PObject
{
  public:
    SIPAuthInfo()
    { }

    SIPAuthInfo(const PString & u, const PString & p)
    { username = u; password = p; }
    PString username;
    PString password;
};

/////////////////////////////////////////////////////////////////////////

/**Session Initiation Protocol endpoint.
 */
class SIPEndPoint : public OpalEndPoint
{
  PCLASSINFO(SIPEndPoint, OpalEndPoint);

  public:
  /**@name Construction */
  //@{
    /**Create a new endpoint.
     */
    SIPEndPoint(
      OpalManager & manager
    );

    /**Destroy endpoint.
     */
    ~SIPEndPoint();
  //@}

  /**@name Overrides from OpalEndPoint */
  //@{
    /**Get the default transports for the endpoint type.
       Overrides the default behaviour to return udp and tcp.
      */
    virtual PString GetDefaultTransport() const { return "udp$,tcp$"; }

    /**Handle new incoming connection from listener.

       The default behaviour does nothing.
      */
    virtual BOOL NewIncomingConnection(
      OpalTransport * transport  ///<  Transport connection came in on
    );

    /**Set up a connection to a remote party.
       This is called from the OpalManager::MakeConnection() function once
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
      OpalCall & call,                         ///<  Owner of connection
      const PString & party,                   ///<  Remote party to call
      void * userData,                         ///<  Arbitrary data to pass to connection
      unsigned int options,                    ///<  options to pass to conneciton
      OpalConnection::StringOptions * stringOptions  ///<  complex string options
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

  /**@name Customisation call backs */
  //@{
    /**Create a connection for the SIP endpoint.
       The default implementation is to create a OpalSIPConnection.
      */
    virtual SIPConnection * CreateConnection(
      OpalCall & call,                         ///<  Owner of connection
      const PString & token,                   ///<  token used to identify connection
      void * userData,                         ///<  User data for connection
      const SIPURL & destination,              ///<  Destination for outgoing call
      OpalTransport * transport,               ///<  Transport INVITE has been received on
      SIP_PDU * invite,                        ///<  Original INVITE pdu
      unsigned int options = 0,                ///<  connection options
      OpalConnection::StringOptions * stringOptions = NULL ///<  complex string options

    );
    
    /**Setup a connection transfer a connection for the SIP endpoint.
      */
    virtual BOOL SetupTransfer(
      const PString & token,        ///<  Existing connection to be transferred
      const PString & callIdentity, ///<  Call identity of the secondary call (if it exists)
      const PString & remoteParty,  ///<  Remote party to transfer the existing call to
      void * userData = NULL        ///<  user data to pass to CreateConnection
    );
    
    /**Forward the connection using the same token as the specified connection.
       Return TRUE if the connection is being redirected.
      */
    virtual BOOL ForwardConnection(
      SIPConnection & connection,     ///<  Connection to be forwarded
      const PString & forwardParty    ///<  Remote party to forward to
    );

  //@}
  
  /**@name Protocol handling routines */
  //@{

    /**Creates an OpalTransport instance, based on the
       address is interpreted as the remote address to which the
       transport should connect.
      */
    virtual OpalTransport * CreateTransport(
      const OpalTransportAddress & remoteAddress,
      const OpalTransportAddress & localAddress = OpalTransportAddress()
    );
    
    /**Releases the transport created by CreateTransport()
      */
    virtual void ReleaseTransport(OpalTransport * transport);

    virtual void HandlePDU(
      OpalTransport & transport
    );

    /**Handle an incoming SIP PDU that belongs to this endpoint
      */
    virtual void OnReceivedPDU(
      OpalTransport & transport,
      SIP_PDU * pdu
    );

    /**Handle an incoming response PDU.
      */
    virtual void OnReceivedResponse(
      SIPTransaction & transaction,
      SIP_PDU & response
    );

    /**Handle an incoming INVITE request.
      */
    virtual BOOL OnReceivedINVITE(
      OpalTransport & transport,
      SIP_PDU * pdu
    );
  
    /**Handle an incoming Proxy Authentication Required response PDU
      */
    virtual void OnReceivedAuthenticationRequired(
      SIPTransaction & transaction,
      SIP_PDU & response
    );

    /**Handle an incoming OK response PDU.
       This actually gets any PDU of the class 2xx not just 200.
      */
    virtual void OnReceivedOK(
      SIPTransaction & transaction,
      SIP_PDU & response
    );
    
    /**Handle an incoming NOTIFY PDU.
      */
    virtual BOOL OnReceivedNOTIFY(
      OpalTransport & transport,
      SIP_PDU & response
    );

    /**Handle an incoming MESSAGE PDU.
      */
    virtual void OnReceivedMESSAGE(
      OpalTransport & transport,
      SIP_PDU & response
    );
    
    /**Handle a transaction failure
      */
    virtual void OnTransactionFailed(
      SIPTransaction & transaction
    );
    
    /**Callback from the RTP session for statistics monitoring.
       This is called every so many packets on the transmitter and receiver
       threads of the RTP session indicating that the statistics have been
       updated.

       The default behaviour does nothing.
      */
    virtual void OnRTPStatistics(
      const SIPConnection & connection,  ///<  Connection for the channel
      const RTP_Session & session         ///<  Session with statistics
    ) const;
    
    /**Overrides from OpalConnection. Keeps the connection active for a longer moment.
      */
    virtual void OnReleased(
      OpalConnection & connection
    );
    
    /**Removes the connection from the active connections list
      */
    virtual void OnReleaseComplete(
      OpalConnection & connection
    );
  //@}
 

    /**Find a connection that uses the specified token.
       This searches the endpoint for the connection that contains the token
       as provided by functions such as MakeConnection().
      */
    PSafePtr<SIPConnection> GetSIPConnectionWithLock(
      const PString & token,     ///<  Token to identify connection
      PSafetyMode mode = PSafeReadWrite
    ) { return PSafePtrCast<OpalConnection, SIPConnection>(GetConnectionWithLock(token, mode)); }

    virtual BOOL IsAcceptedAddress(const SIPURL & toAddr);


    /**Callback for SIP message received
     */
    virtual void OnMessageReceived (const SIPURL & from,
				    const PString & body);

				
    /**Register to a registrar. This function is asynchronous to permit
     * several registrations to occur at the same time. It can be
     * called several times for different hosts and users.
     * 
     * The username can be of the form user@domain. In that case,
     * the From field will be set to that value.
     * 
     * The realm can be specified when registering, this will
     * allow to find the correct authentication information when being
     * requested. If no realm is specified, authentication will
     * occur with the "best guess" of authentication parameters.
     */
    BOOL Register(
      unsigned expire,
      const PString & aor = PString::Empty(),                 ///< user@host
      const PString & authName = PString::Empty(),            ///< authentication user name
      const PString & password = PString::Empty(),            ///< authentication password
      const PString & authRealm = PString::Empty(),           ///< authentication realm
      const PTimeInterval & minRetryTime = PMaxTimeInterval, 
      const PTimeInterval & maxRetryTime = PMaxTimeInterval,
      BOOL forceCreationOfNewHandler = FALSE
    );
    BOOL Register(
      const PString & host,
      const PString & user = PString::Empty(),
      const PString & autName = PString::Empty(),
      const PString & password = PString::Empty(),
      const PString & authRealm = PString::Empty(),
      unsigned expire = 0,
      const PTimeInterval & minRetryTime = PMaxTimeInterval, 
      const PTimeInterval & maxRetryTime = PMaxTimeInterval
    );
    
    /**Unregister from a registrar. 
     */
    BOOL Unregister(const PString & aor);
    
    
    /**Subscribe to a notifier. This function is asynchronous to permit
     * several subscriptions to occur at the same time.
     */
    BOOL Subscribe(
      SIPSubscribe::SubscribeType & type,
      unsigned expire,
      const PString & to
    );
    
    
    BOOL Unsubscribe(
      SIPSubscribe::SubscribeType & type,
      const PString & to
    );
    
    
    /**Send a message to the given URL.
     */
    BOOL Message (
      const PString & to, 
      const PString & body
    );
    
    
    /**Publish new state information.
     * Only the basic & note fields of the PIDF+xml are supported for now.
     */
    BOOL Publish(
      const PString & to,
      const PString & body,
      unsigned expire = 0
    );
    

    /** Send a SIP PING to the remote host
     */
    BOOL Ping(
      const PString & to
    );

    /**Callback called when MWI is received
     */
    virtual void OnMWIReceived (
      const PString & to,
      SIPSubscribe::MWIType type,
      const PString & msgs
    );

    /**Callback called when MWI is received
     */
    virtual void OnPresenceInfoReceived(
      const PString & user,
      const PString & basic,
      const PString & note
    );
    
    
    /**Callback called when a registration to a SIP registrar status.
     * The BOOL indicates if the operation that failed was a REGISTER or
     * an (UN)REGISTER.
     */
    virtual void OnRegistrationStatus(
      const PString & aor,
      BOOL wasRegistering,
      BOOL reRegistering,
      SIP_PDU::StatusCodes reason
    );
   
    
    /**Callback called when a registration to a SIP registrars fails.
     * The BOOL indicates if the operation that failed was a REGISTER or 
     * an (UN)REGISTER.
     */
    virtual void OnRegistrationFailed(
      const PString & aor,
      SIP_PDU::StatusCodes reason,
      BOOL wasRegistering
    );
    
      
    /**Callback called when a registration or an unregistration is successful.
     * The BOOL indicates if the operation that failed was a registration or
     * not.
     */
    virtual void OnRegistered(
      const PString & aor,
      BOOL wasRegistering
    );

    
    /**Returns TRUE if the given URL has been registered 
     * (e.g.: 6001@seconix.com).
     */
    BOOL IsRegistered(const PString & aor);
    
    /**Returns TRUE if subscribed to the given host for MWI.
     */
    BOOL IsSubscribed(
      SIPSubscribe::SubscribeType type, 
      const PString & to
    );
    
    
    /** Returns the number of registered accounts.
     */
    unsigned GetRegistrationsCount () { return activeSIPHandlers.GetRegistrationsCount (); }
    
    
    /**Callback called when a message sent by the endpoint didn't reach
     * its destination or when the proxy or remote endpoint returns
     * an error code.
     */
    virtual void OnMessageFailed(
      const SIPURL & messageUrl,
      SIP_PDU::StatusCodes reason);
    

    void SetMIMEForm(BOOL v) { mimeForm = v; }
    BOOL GetMIMEForm() const { return mimeForm; }

    void SetMaxRetries(unsigned r) { maxRetries = r; }
    unsigned GetMaxRetries() const { return maxRetries; }

    void SetRetryTimeouts(
      const PTimeInterval & t1,
      const PTimeInterval & t2
    ) { retryTimeoutMin = t1; retryTimeoutMax = t2; }
    const PTimeInterval & GetRetryTimeoutMin() const { return retryTimeoutMin; }
    const PTimeInterval & GetRetryTimeoutMax() const { return retryTimeoutMax; }

    void SetNonInviteTimeout(
      const PTimeInterval & t
    ) { nonInviteTimeout = t; }
    const PTimeInterval & GetNonInviteTimeout() const { return nonInviteTimeout; }

    void SetPduCleanUpTimeout(
      const PTimeInterval & t
    ) { pduCleanUpTimeout = t; }
    const PTimeInterval & GetPduCleanUpTimeout() const { return pduCleanUpTimeout; }

    void SetInviteTimeout(
      const PTimeInterval & t
    ) { inviteTimeout = t; }
    const PTimeInterval & GetInviteTimeout() const { return inviteTimeout; }

    void SetAckTimeout(
      const PTimeInterval & t
    ) { ackTimeout = t; }
    const PTimeInterval & GetAckTimeout() const { return ackTimeout; }

    void SetRegistrarTimeToLive(
      const PTimeInterval & t
    ) { registrarTimeToLive = t; }
    const PTimeInterval & GetRegistrarTimeToLive() const { return registrarTimeToLive; }
    
    void SetNotifierTimeToLive(
      const PTimeInterval & t
    ) { notifierTimeToLive = t; }
    const PTimeInterval & GetNotifierTimeToLive() const { return notifierTimeToLive; }
    
    void SetNATBindingTimeout(
      const PTimeInterval & t
    ) { natBindingTimeout = t; natBindingTimer.RunContinuous (natBindingTimeout); }
    const PTimeInterval & GetNATBindingTimeout() const { return natBindingTimeout; }
    
    void AddTransaction(
      SIPTransaction * transaction
    ) { transactions.SetAt(transaction->GetTransactionID(), transaction); }

    void RemoveTransaction(
      SIPTransaction * transaction
    ) { transactions.SetAt(transaction->GetTransactionID(), NULL); }
    
    /**Return the next CSEQ for the next transaction.
     */
    unsigned GetNextCSeq() { PWaitAndSignal m(jobProcessingMutex); return ++lastSentCSeq; }
    
    /**Return the SIPAuthentication for a specific realm.
     */
    BOOL GetAuthentication(const PString & authRealm, SIPAuthentication &); 
    

    /**Return the registered party name URL for the given host.
     *
     * That URL can be used in the FORM field of the PDU's. 
     * The host part can be different from the registration domain.
     */
    virtual SIPURL GetRegisteredPartyName(const SIPURL &);


    /**Return the default registered party name URL.
     */
    virtual SIPURL GetDefaultRegisteredPartyName();
    
    
    /**Return the contact URL for the given host and user name
     * based on the listening port of the registration to that host.
     * 
     * That URL can be used as as contact field in outgoing
     * requests.
     *
     * The URL is translated if required.
     *
     * If no active registration is used, return the result of GetLocalURL
     * on the given transport.
     */
    SIPURL GetContactURL(const OpalTransport &transport, const PString & userName, const PString & host);
    

    /**Return the local URL for the given transport and user name.
     * That URL can be used as via address, and as contact field in outgoing
     * requests.
     *
     * The URL is translated if required.
     *
     * If the transport is not running, the first listener transport
     * will be used, if any.
     */
    SIPURL GetLocalURL(
       const OpalTransport & transport,             ///< Transport on which we can receive new requests
       const PString & userName = PString::Empty()  ///< The user name part of the contact field
    );
    
    
    /**Return the outbound proxy URL, if any.
     */
    const SIPURL & GetProxy() const { return proxy; }

    
    /**Set the outbound proxy URL.
     */
    void SetProxy(const SIPURL & url);
    
    
    /**Set the outbound proxy URL.
     */
    void SetProxy(
      const PString & hostname,
      const PString & username,
      const PString & password
    );

    
    /**Get the User Agent for this endpoint.
       Default behaviour returns an empty string so the SIPConnection builds
       a valid string from the productInfo data.
      
       These semantics are for backward compatibility.
     */
    virtual PString GetUserAgent() const;

    /**Set the User Agent for the endpoint.
     */
    void SetUserAgent(const PString & str) { userAgentString = str; }
    
    
    BOOL SendResponse(
      SIP_PDU::StatusCodes code, 
      OpalTransport & transport, 
      SIP_PDU & pdu
    );
    

    /**NAT Binding Refresh Method
     */
    enum NATBindingRefreshMethod{
      None,
      Options,
      EmptyRequest,
      NumMethods
    };


    /**Set the NAT Binding Refresh Method
     */
    void SetNATBindingRefreshMethod(const NATBindingRefreshMethod m) { natMethod = m; }
    
    SIPHandlersList & GetactiveSIPHandlers() { return activeSIPHandlers; }

    virtual SIPRegisterHandler * CreateRegisterHandler(
      const PString & to,
      const PString & authName, 
      const PString & password, 
      const PString & realm,
      int expire,
      const PTimeInterval & minRetryTime, 
      const PTimeInterval & maxRetryTime
    );

    /**Endpoint related PDU / transaction handling
      */
    void QueuePDU(
      OpalTransport & transport,
      SIP_PDU * pdu
    );
    void QueueTransactionJob(
      SIPTransactionJob * job
    );
    void LockTransactionProcessing();
    void UnlockTransactionProcessing();

  protected:
    class SIPEndpointPDUJob : public SIPTransactionJob
    {
        PCLASSINFO(SIPEndpointPDUJob, SIPTransactionJob);

      public:
        SIPEndpointPDUJob(OpalTransport & transport, SIP_PDU * pdu);
        
        OpalTransport & GetTransport() const { return transport; }
      
      protected:
        OpalTransport & transport;
    };
    PDECLARE_NOTIFIER(PThread, SIPEndPoint, JobThreadMain);
    PDECLARE_NOTIFIER(PThread, SIPEndPoint, TransportThreadMain);
    PDECLARE_NOTIFIER(PTimer, SIPEndPoint, NATBindingRefresh);
    PDECLARE_NOTIFIER(PTimer, SIPEndPoint, GarbageCollector);
    
    BOOL ProcessNextJob();
    
    void ParsePartyName(
      const PString & remoteParty,     ///<  Party name string.
      PString & party                  ///<  Parsed party name, after e164 lookup
    );
    
    class TransportRecord : public PObject {
        PCLASSINFO(TransportRecord, PObject);
        
      public:
        
        TransportRecord(OpalTransport * transport);
        ~TransportRecord();
        
        OpalTransport * GetTransport() const { return transport; }
        unsigned GetReferenceCount() const { return referenceCount; }
        
        void IncrementReferenceCount() { referenceCount++; }
        void DecrementReferenceCount() { referenceCount--; }
        
      protected:
        
        OpalTransport * transport;
        unsigned referenceCount;
    };
    
    PDICTIONARY(TransportDict, OpalTransportAddress, TransportRecord);

    SIPURL            proxy;
    PString           userAgentString;

    BOOL          mimeForm;
    unsigned      maxRetries;
    PTimeInterval retryTimeoutMin;   // T1
    PTimeInterval retryTimeoutMax;   // T2
    PTimeInterval nonInviteTimeout;  // T3
    PTimeInterval pduCleanUpTimeout; // T4
    PTimeInterval inviteTimeout;
    PTimeInterval ackTimeout;
    PTimeInterval registrarTimeToLive;
    PTimeInterval notifierTimeToLive;
    PTimeInterval natBindingTimeout;
    
    SIPHandlersList activeSIPHandlers;

    SIPTransactionList messages;
    SIPTransactionDict transactions;

    PTimer                  natBindingTimer;
    NATBindingRefreshMethod natMethod;
    
    PTimer        garbageTimer;

    SIPTransactionJobQueue jobQueue;
    PMutex        jobQueueMutex;
    PMutex        jobProcessingMutex;
    PSemaphore    jobSemaphore;
    PThread     * jobHandler;
    BOOL endJobProcessing;

    unsigned           lastSentCSeq;
    
    PMutex transportsMutex;
    TransportDict transports;
};

#endif // __OPAL_SIPEP_H


// End of File ///////////////////////////////////////////////////////////////
