/*
 * Python bindings.
 *
 * Open Phone Abstraction Library (OPAL)
 *
 * Copyright (c) 2011 Demetrius Cassidy
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
 * The Original Code is Open Phone Abstraction Library (OPAL)
 *
 * The Initial Developer of the Original Code is Demetrius Cassidy
 *
 * Contributor(s): ______________________________________.
 *
 * $Revision$
 * $Author$
 * $Date$
 */


///////////////////////////////////////////////////////////////////////////////

/**This class embodies the H.225.0 RAS protocol to gatekeepers.
  */
class H323Gatekeeper
{
%TypeHeaderCode
#include <ptlib.h>
#include <h323/gkclient.h> 
%End
  public:
 
  /**@name Protocol operations */
  //@{
    /**Discover a gatekeeper on the local network.
     */
    PBoolean DiscoverAny();

    /**Discover a gatekeeper on the local network.
       If the identifier string is empty then the first gatekeeper to respond
       to a broadcast is used.
     */
    PBoolean DiscoverByName(
      const PString & identifier  ///<  Gatekeeper identifier to find
    );

    /**Discover a gatekeeper on the local network.
       If the address string is empty then the first gatekeeper to respond
       to a broadcast is used.
     */
    PBoolean DiscoverByAddress(
      const H323TransportAddress & address ///<  Address of gatekeeper.
    );

    /**Discover a gatekeeper on the local network.
       Combination of DiscoverByName() and DiscoverByAddress().
     */
    PBoolean DiscoverByNameAndAddress(
      const PString & identifier,
      const H323TransportAddress & address
    );

    /**Register with gatekeeper.
     */
    PBoolean RegistrationRequest(
      PBoolean autoReregister = PTrue,  ///<  Automatic register on unregister
      PBoolean didGkDiscovery = PFalse  ///<  discovery procedure was done right before
    );

    /**Unregister with gatekeeper.
     */
    PBoolean UnregistrationRequest(
      int reason      ///<  Reason for unregistration
    );


  /**@name Member variable access */
  //@{
    /**Determine if the endpoint has discovered the gatekeeper.
      */
    PBoolean IsDiscoveryComplete() const;

    /**Determine if the endpoint is registered with the gatekeeper.
      */
    PBoolean IsRegistered() const;

    enum RegistrationFailReasons {
      RegistrationSuccessful,
      UnregisteredLocally,
      UnregisteredByGatekeeper,
      GatekeeperLostRegistration,
      InvalidListener,
      DuplicateAlias,
      SecurityDenied,
      TransportError,
      NumRegistrationFailReasons,
      RegistrationRejectReasonMask = 0x8000
    };
    /**Get the registration fail reason.
     */
    RegistrationFailReasons GetRegistrationFailReason() const;

    /**Get the gatekeeper name.
       The gets the name of the gatekeeper. It will be of the form id@address
       where id is the gatekeeperIdentifier and address is the transport
       address used. If the gatekeeperIdentifier is empty the '@' is not
       included and only the transport is shown. The transport is minimised
       also, with the type removed if IP is used and the :port removed if the
       default port is used.
      */
    PString GetName() const;

    /** Get the endpoint identifier
      */
    const PString & GetEndpointIdentifier() const;

    /**Set the H.235 password in the gatekeeper.
       If no username is present then it will default to the endpoint local
       user name (ie first alias).
      */
    void SetPassword(
      const PString & password,            ///<  New password
      const PString & username = PString() ///<  Username for password
    );
	
    /*
     * Return the call signalling address for the gatekeeper (if present)
     */
    H323TransportAddress GetGatekeeperRouteAddress() const;
	
	
	  /**@name Member variable access (from H225_RAS) */
  //@{
    /**Get the gatekeeper identifer.
       For clients at least one successful registration must have been
       achieved for this field to be filling in.
      */
    const PString & GetIdentifier() const;

    /**Set the gatekeeper identifer.
       For servers this allows the identifier to be set and provided to all
       remote clients.
      */
    void SetIdentifier(const PString & id);
   
  //@}
  
  private:
     // Declared private to remove insantiation via python
	/**@name Construction */
  //@{
    /**Create a new gatekeeper.
     */
    H323Gatekeeper();

    /**Destroy gatekeeper.
     */
    ~H323Gatekeeper();
  //@}
};