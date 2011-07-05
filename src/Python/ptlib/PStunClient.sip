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

%ModuleHeaderCode
#include <ptlib.h>
#include <ptclib/pstun.h>
%End


/**UDP socket that has been created by the STUN client.
  */
class PSTUNUDPSocket : PUDPSocket
{
  public:
    PSTUNUDPSocket();

    virtual PBoolean GetLocalAddress(
      Address & addr    ///< Variable to receive hosts IP address
    );
	
SIP_PYOBJECT GetLocalAddressAndPort(
      Address & addr /Out/,    ///< Variable to receive peer hosts IP address
      WORD & port /Out/       ///< Variable to receive peer hosts port number
    );
%MethodCode
	//note: "a0 = new PIPSocket::Address()" per sip, but a1 is created on the stack.
	sipRes = PyBool_FromLong(static_cast<long>(sipCpp->GetLocalAddress(*a0,a1)));
%End

};


/**STUN client.
  */
class PSTUNClient : PNatMethod
{
  public:
    enum {
      DefaultPort = 3478
    };

    PSTUNClient();

    PSTUNClient(
      const PString & server,
      WORD portBase = 0,
      WORD portMax = 0,
      WORD portPairBase = 0,
      WORD portPairMax = 0
    );
    PSTUNClient(
      const PIPSocket::Address & serverAddress,
      WORD serverPort = DefaultPort,
      WORD portBase = 0,
      WORD portMax = 0,
      WORD portPairBase = 0,
      WORD portPairMax = 0
    );


    void Initialise(
      const PString & server,
      WORD portBase = 0, 
      WORD portMax = 0,
      WORD portPairBase = 0, 
      WORD portPairMax = 0
    );

    /**Get the NAT Method Name
     */
    static PStringList GetNatMethodName();

    /** Get the NAT traversal method name
    */
    virtual PString GetName() const;

    /**Get the current server address and port being used.
      */
    virtual bool GetServerAddress(
      PIPSocket::Address & address,   ///< Address of server
      WORD & port                     ///< Port server is using.
    ) const;

    /**Set the STUN server to use.
       The server string may be of the form host:port. If :port is absent
       then the default port 3478 is used. The substring port can also be
       a service name as found in /etc/services. The host substring may be
       a DNS name or explicit IP address.
      */
    PBoolean SetServer(
      const PString & server
    );

    /**Set the STUN server to use by IP address and port.
       If serverPort is zero then the default port of 3478 is used.
      */
    PBoolean SetServer(
      const PIPSocket::Address & serverAddress,
      WORD serverPort = 0
    );

    enum NatTypes {
      UnknownNat,
      OpenNat,
      ConeNat,
      RestrictedNat,
      PortRestrictedNat,
      SymmetricNat,
      SymmetricFirewall,
      BlockedNat,
      PartialBlockedNat,
      NumNatTypes
    };

    /**Determine via the STUN protocol the NAT type for the router.
       This will cache the last determine NAT type. Use the force variable to
       guarantee an up to date value.
      */
    NatTypes GetNatType(
      PBoolean force = false    ///< Force a new check
    );

    /**Determine via the STUN protocol the NAT type for the router.
       As for GetNatType() but returns an English string for the type.
      */
    PString GetNatTypeName(
      PBoolean force = false    ///< Force a new check
    );

    /**Get NatTypes enumeration as an English string for the type.
      */
    static PString GetNatTypeString(
      NatTypes type   ///< NAT Type to get name of
    );

    /**Return an indication if the current STUN type supports RTP
      Use the force variable to guarantee an up to date test
      */
    RTPSupportTypes GetRTPSupport(
      PBoolean force = false    ///< Force a new check
    );

    /**Determine the external router address.
       This will send UDP packets out using the STUN protocol to determine
       the intervening routers external IP address.

       A cached address is returned provided it is no older than the time
       specified.
      */
    virtual PBoolean GetExternalAddress(
      PIPSocket::Address & externalAddress, ///< External address of router
      const PTimeInterval & maxAge = 1000   ///< Maximum age for caching
    );

    /**Return the interface NAT router is using.
      */
    virtual bool GetInterfaceAddress(
      PIPSocket::Address & internalAddress
    ) const;

    /**Invalidates the cached addresses and modes.
       This allows to lazily update the external address cache at the next 
       attempt to get the external address.
      */
    void InvalidateCache();

    /**Create a single socket.
       The STUN protocol is used to create a socket for which the external IP
       address and port numbers are known. A PUDPSocket descendant is returned
       which will, in response to GetLocalAddress() return the externally
       visible IP and port rather than the local machines IP and socket.

       The will create a new socket pointer. It is up to the caller to make
       sure the socket is deleted to avoid memory leaks.

       The socket pointer is set to NULL if the function fails and returns
       false.
      */
    PBoolean CreateSocket(
      PUDPSocket * & socket,
      const PIPSocket::Address & binding = PIPSocket::GetDefaultIpAny(),
      WORD localPort = 0
    );

    /**Create a socket pair.
       The STUN protocol is used to create a pair of sockets with adjacent
       port numbers for which the external IP address and port numbers are
       known. PUDPSocket descendants are returned which will, in response
       to GetLocalAddress() return the externally visible IP and port rather
       than the local machines IP and socket.

       The will create new socket pointers. It is up to the caller to make
       sure the sockets are deleted to avoid memory leaks.

       The socket pointers are set to NULL if the function fails and returns
       false.
      */
    virtual PBoolean CreateSocketPair(
      PUDPSocket * & socket1,
      PUDPSocket * & socket2,
      const PIPSocket::Address & binding = PIPSocket::GetDefaultIpAny()
    );

    /**Get the timeout for responses from STUN server.
      */
    const PTimeInterval GetTimeout() const;

    /**Set the timeout for responses from STUN server.
      */
    void SetTimeout(
      const PTimeInterval & timeout   ///< New timeout in milliseconds
    );

    /**Get the number of retries for responses from STUN server.
      */
    PINDEX GetRetries() const;

    /**Set the number of retries for responses from STUN server.
      */
    void SetRetries(
      PINDEX retries    ///< Number of retries
    );

    /**Get the number of sockets to create in attempt to get a port pair.
       RTP requires a pair of consecutive ports. To get this several sockets
       must be opened and fired through the NAT firewall to get a pair. The
       busier the firewall the more sockets will be required.
      */
    PINDEX GetSocketsForPairing() const;

    /**Set the number of sockets to create in attempt to get a port pair.
       RTP requires a pair of consecutive ports. To get this several sockets
       must be opened and fired through the NAT firewall to get a pair. The
       busier the firewall the more sockets will be required.
      */
    void SetSocketsForPairing(
      PINDEX numSockets   ///< Number opf sockets to create
    );

    /**Returns whether the Nat Method is ready and available in
       assisting in NAT Traversal. The principal is this function is
       to allow the EP to detect various methods and if a method
       is detected then this method is available for NAT traversal.
       The availablity of the STUN Method is dependant on the Type
       of NAT being used.
     */
    virtual bool IsAvailable(
      const PIPSocket::Address & binding = PIPSocket::GetDefaultIpAny()  ///< Interface to see if NAT is available on
    );


};


// End of file ////////////////////////////////////////////////////////////////
