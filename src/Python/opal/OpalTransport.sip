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
#include <opal/transports.h>
%End

class OpalTransportAddress 
{
  private:
	OpalTranportAddress & operator=(const OpalTransportAddress& );

  public:
  /**@name Construction */
  //@{
    OpalTransportAddress();
    OpalTransportAddress(
      const char * address,      ///<  Address string to parse
      WORD port = 0,             ///<  Default port number
      const char * proto = NULL  ///<  Default is "tcp"
    );
    OpalTransportAddress(
      const PString & address,   ///<  Address string to parse
      WORD port = 0,             ///<  Default port number
      const char * proto = NULL  ///<  Default is "tcp"
    );
    OpalTransportAddress(
      const PIPSocket::Address & ip,
      WORD port,
      const char * proto = NULL ///<  Default is "tcp"
    );
  //@}

  /**@name Operations */
  //@{
    /**Determine if the two transport addresses are equivalent.
       The second parameter indicates if wildcards are allowed. A wildcard
       IP address is one for which isAny() returns true, a wildcard port
       is the value 65535.
      */
    PBoolean IsEquivalent(
      const OpalTransportAddress & address,
      bool wildcards = false   ///< Allow wildcards
    ) const;

    /**Determine if the two transport addresses are compatible.
      */
    PBoolean IsCompatible(
      const OpalTransportAddress & address
    ) const;

    /**Return the underlying protocol for the transport address.
      */
    PCaselessString GetProto(bool withDollar = false) const;

    /**Extract the ip address from transport address.
       Returns PFalse, if the address is not an IP transport address.
      */
    PBoolean GetIpAddress(PIPSocket::Address & ip /Out/ ) const;

    /**Extract the ip address and port number from transport address.
       Returns PFalse, if the address is not an IP transport address.
      */
    PBoolean GetIpAndPort(PIPSocket::Address & ip /Out/, WORD & port /Out/) const;

    SIP_PYOBJECT GetIpAndPort() const;
%MethodCode
    PIPSocketAddressAndPort * ipPort = new PIPSocketAddressAndPort;

    Py_BEGIN_ALLOW_THREADS
    sipCpp->GetIpAndPort(*ipPort);
    Py_END_ALLOW_THREADS

    if ((sipRes=sipConvertFromNewType(ipPort, sipType_PIPSocketAddressAndPort, NULL)) == NULL)
    {
        sipError=sipErrorFail;
        PyErr_SetString(PyExc_RuntimeError, "Failed to convert C++ Object to PyObject");
    }
%End


    /**Translate the transport address to a more human readable form.
       Returns the hostname if using IP.
      */
    virtual PString GetHostName() const;

    enum BindOptions {
      NoBinding,
      HostOnly,
      FullTSAP,
      Streamed,
      Datagram,
      RouteInterface,
      NumBindOptions
    };

    /**Create a listener based on this transport address.
       The BindOptions parameter indicates how the listener is to be created.
       Note that some transport types may not use this parameter.

       With FullTSAP the the full address is used for any local binding, for
       example, an address of "tcp$10.0.0.1:1720" would create a TCP listening
       socket that would be bound to the specific interface 10.0.0.1 and
       listens on port 1720. Note that the address "tcp$*:1720" can be used
       to bind to INADDR_ANY, and a port number of zero indicates allocate a
       new random port number.

       With HostOnly it would be equivalent to translating the above example
       to "tcp$10.0.0.1:0" before using it.

       Using Streamed or Datagram is similar to HostOnly as only the host part
       of the address is used, but instead of using the protocol type specifed
       by the address it guarentees the specifeid type. In the above example
       Streamed would be identical to HostOnly and Datagram would translate
       the address to udp$10.0.0.1:0 before using it.

       With NoBinding then a compatible listener is created and no local
       binding is made. This is equivalent to translating the address to
       "tcp$*:0" so that only the overall protocol type is used.

       With RouteInterface, the address is considered a remote address and the
       created transport is bound only to the address associated with the
       interface that would be used to get to that address.

       Also note that if the address has a trailing '+' character then the
       socket will be bound using the REUSEADDR option, where relevant.
      */
    OpalListener * CreateListener(
      OpalEndPoint & endpoint,   ///<  Endpoint object for transport creation.
      BindOptions option         ///<  Options for how to create listener
    ) const;

    /**Create a transport suitable for this address type.
       The BindOptions parameter indicates how the transport is to be created.
       Note that some transport types may not use this parameter.

       With FullTSAP the the full address is used for any local binding, for
       example, an address of "tcp$10.0.0.1:1720" would create a TCP transport
       socket that would be bound to the specific interface 10.0.0.1 and
       port 1720. Note that the address "tcp$*:1720" can be used to bind to
       INADDR_ANY, and a port number of zero indicates allocate a new random
       port number.

       With HostOnly it would be equivalent to translating the above example
       to "tcp$10.0.0.1:0" before using it.

       Using Streamed or Datagram is similar to HostOnly as only the host part
       of the address is used, but instead of using the protocol type specifed
       by the address it guarentees the specifeid type. In the above example
       Streamed would be identical to HostOnly and Datagram would translate
       the address to udp$10.0.0.1:0 before using it.

       With NoBinding then a compatible transport is created and no local
       binding is made. This is equivalent to translating the address to
       "tcp$*:0" so that only the overall protocol type is used.

       Also note that if the address has a trailing '+' character then the
       socket will be bound using the REUSEADDR option.
      */
    virtual OpalTransport * CreateTransport(
      OpalEndPoint & endpoint,   ///<  Endpoint object for transport creation.
      BindOptions option = HostOnly ///<  Options for how to create transport
    ) const;
  //@}


	SIP_PYOBJECT __str__();
%MethodCode
	sipRes = PyString_FromString(*sipCpp);
%End

};


///////////////////////////////////////////////////////

/**This class describes a "listener" on a transport protocol.
   A "listener" is an object that listens for incoming connections on the
   particular transport. It is executed as a separate thread.

   The Main() function is used to handle incoming connections and dispatch
   them in new threads based on the actual OpalTransport class. This is
   defined in the descendent class that knows what the low level transport
   is, eg OpalListenerIP for the TCP/IP protocol.

   An application may create a descendent off this class and override
   functions as required for operating the channel protocol.
 */
class OpalListener : PObject /Abstract,NoDefaultCtors/
{
  public:
  /**@name Construction */
  //@{
    /**Create a new listener.
     */
    OpalListener(
      OpalEndPoint & endpoint   ///<  Endpoint listener is used for
    );
  //@}


  /**@name Operations */
  //@{
    enum ThreadMode {
      SpawnNewThreadMode,
      HandOffThreadMode,
      SingleThreadMode
    };

    /** Open the listener.
        A thread is spawned to listen for incoming connections. The notifier
        function acceptHandler is called when a new connection is created. The
        INT parameter to the acceptHandler is a pointer to the new
        OpalTransport instance created by the listener. If this is NULL then
        it indicates an error occurred during the accept.

        If singleThread is PFalse the acceptHandler function is called in the
        context of a new thread and the continues to listen for more
        connections. If PTrue, then the acceptHandler function is called from
        within the listener threads context and no more connections are
        created. That is only a single connection is ever created by this
        listener.
      */
    virtual PBoolean Open(
      const PNotifier & acceptHandler,  ///<  Handler function for new connections
      ThreadMode mode = SpawnNewThreadMode ///<  How handler function is called thread wise
    ) = 0;

    /** Indicate if the listener is open.
      */
    virtual PBoolean IsOpen() = 0;

    /**Stop the listener thread and no longer accept incoming connections.
     */
    virtual void Close() = 0;

    /**Accept a new incoming transport.
      */
    virtual OpalTransport * Accept(
      const PTimeInterval & timeout  ///<  Time to wait for incoming connection
    ) = 0;

    /**Create a transport compatible with this listener.
     */
    virtual OpalTransport * CreateTransport(
      const OpalTransportAddress & localAddress,
      const OpalTransportAddress & remoteAddress
    ) const = 0;

    /**Get the local transport address on which this listener may be accessed.
       If remoteAddress is present and is an address that requires NAT for
       connectivity, then the returned address is adjusted to return the
       external address and port.
      */
    virtual OpalTransportAddress GetLocalAddress(
      const OpalTransportAddress & remoteAddress = OpalTransportAddress()
    ) const = 0;

    /**Close channel and wait for associated thread to terminate.
      */
    void CloseWait();

    /**Close channel and wait for associated thread to terminate.
       For backward compatibility with OpenH323, now deprecated.
      */
    void CleanUpOnTermination();
  //@}

};


class OpalListenerIP : OpalListener /Abstract,NoDefaultCtors/
{
  public:
  /**@name Construction */
  //@{
    /**Create a new IP listener.
     */
    OpalListenerIP(
      OpalEndPoint & endpoint,                 ///<  Endpoint listener is used for
      PIPSocket::Address binding = PIPSocket::GetDefaultIpAny(), ///<  Local interface to listen on
      WORD port = 0,                           ///<  TCP port to listen for connections
      PBoolean exclusive = PTrue
    );
    OpalListenerIP(
      OpalEndPoint & endpoint,                  ///<  Endpoint listener is used for
      const OpalTransportAddress & binding,     ///<  Local interface to listen on
      OpalTransportAddress::BindOptions option  ///< OPtions for binding
    );
  //@}

  /**@name Overrides from OpalListener */
  //@{
    /**Get the local transport address on which this listener may be accessed.
       If remoteAddress is present and is an address that requires NAT for
       connectivity, then the returned address is adjusted to return the
       external address and port.
      */
    virtual OpalTransportAddress GetLocalAddress(
      const OpalTransportAddress & remoteAddress = OpalTransportAddress()
    ) const;
  //@}

  /**@name Operations */
  //@{
    WORD GetListenerPort() const;

    virtual const char * GetProtoPrefix() const = 0;
  //@}
};


class OpalListenerTCP : OpalListenerIP /NoDefaultCtors/
{
  public:
  /**@name Construction */
  //@{
    /**Create a new listener.
     */
    OpalListenerTCP(
      OpalEndPoint & endpoint,                 ///<  Endpoint listener is used for
      PIPSocket::Address binding = PIPSocket::GetDefaultIpAny(), ///<  Local interface to listen on
      WORD port = 0,                           ///<  TCP port to listen for connections
      PBoolean exclusive = PTrue
    );
    OpalListenerTCP(
      OpalEndPoint & endpoint,                  ///<  Endpoint listener is used for
      const OpalTransportAddress & binding,     ///<  Local interface to listen on
      OpalTransportAddress::BindOptions option  ///< OPtions for binding
    );

    /** Destroy the listener thread.
      */
    ~OpalListenerTCP();
  //@}

  /**@name Overrides from OpalListener */
  //@{
    /** Open the listener.
        Listen for an incoming connection and create a OpalTransport of the
        appropriate subclass.

        If notifier function acceptHandler is non-NULL a thread is spawned to
        listen for incoming connections. The acceptHandler is called when a
        new connection is created. The INT parameter to the acceptHandler is
        a pointer to the new OpalTransport instance created by the listener.

        If singleThread is PFalse the acceptHandler function is called in the
        context of a new thread and the continues to listen for more
        connections. If PTrue, then the acceptHandler function is called from
        within the listener threads context and no more connections are
        created. That is only a single connection is ever created by this
        listener.

        If acceptHandler is NULL, then no thread is started and it is assumed
        that the caller is responsible for calling Accept() and waiting for
        the new connection.
      */
    virtual PBoolean Open(
      const PNotifier & acceptHandler,  ///<  Handler function for new connections
      ThreadMode mode = SpawnNewThreadMode ///<  How handler function is called thread wise
    );

    /** Indicate if the listener is open.
      */
    virtual PBoolean IsOpen();

    /**Stop the listener thread and no longer accept incoming connections.
     */
    virtual void Close();

    /**Accept a new incoming transport.
      */
    virtual OpalTransport * Accept(
      const PTimeInterval & timeout  ///<  Time to wait for incoming connection
    );

    /**Create a transport compatible with this listener.
     */
    virtual OpalTransport * CreateTransport(
      const OpalTransportAddress & localAddress,
      const OpalTransportAddress & remoteAddress
    ) const;
  //@}
};


class OpalListenerUDP : OpalListenerIP /NoDefaultCtors/
{
  public:
  /**@name Construction */
  //@{
    /**Create a new listener.
     */
    OpalListenerUDP(
      OpalEndPoint & endpoint,  ///<  Endpoint listener is used for
      PIPSocket::Address binding = PIPSocket::GetDefaultIpAny(), ///<  Local interface to listen on
      WORD port = 0,            ///<  TCP port to listen for connections
      PBoolean exclusive = PTrue
    );

    OpalListenerUDP(
      OpalEndPoint & endpoint,                  ///<  Endpoint listener is used for
      const OpalTransportAddress & binding,     ///<  Local interface to listen on
      OpalTransportAddress::BindOptions option  ///< OPtions for binding
    );

    /** Destroy the listener thread.
      */
    ~OpalListenerUDP();
  //@}

    /** Indicate if the listener is open.
      */
    virtual PBoolean IsOpen();

    /**Stop the listener thread and no longer accept incoming connections.
     */
    virtual void Close();

    /**Get the local transport address on which this listener may be accessed.
       If remoteAddress is present and is an address that requires NAT for
       connectivity, then the returned address is adjusted to return the
       external address and port.
      */
    virtual OpalTransportAddress GetLocalAddress(
      const OpalTransportAddress & remoteAddress = OpalTransportAddress()
    ) const;
  //@}
};


////////////////////////////////////////////////////////////////

/**This class describes a I/O transport for a protocol.
   A "transport" is an object that allows the transfer and processing of data
   from one entity to another.
 */
class OpalTransport : PIndirectChannel /Abstract/
{
%ConvertToSubClassCode
	const char * prefix = sipCpp->GetProtoPrefix();
	
	// sanity check
	PAssert(prefix != NULL, "OpalTransport::GetProtoPrefix returned NULL prefix in %ConvertToSubClassCode");
	
	if(strcmp(prefix, "tcp$") == 0)
		sipType = sipType_OpalTransportTCP;
		
	else if(strcmp(prefix, "udp$") == 0)
		sipType = sipType_OpalTransportUDP;
		
#if OPAL_PTLIB_SSL		
	else if(strcmp(prefix, "tcps$") == 0)
		sipType = sipType_OpalTransportTCPS;
#endif

	else
		sipType = sipType_OpalTransportIP;
%End
	
  public:
  /**@name Construction */
  //@{
    /**Create a new transport channel.
     */
    OpalTransport(OpalEndPoint & endpoint);

    /** Destroy the transport channel.
      */
    ~OpalTransport();
  //@}


  /**@name Operations */
  //@{
    /**Get indication of the type of underlying transport.
      */
    virtual PBoolean IsReliable() const = 0;

    /** Get the interface this transport is bound to.
        This is generally only relevant for datagram based transports such as
        UDP and TCP is always bound to a local interface once open.

        The default behaviour returns the local address via GetLocalAddress()
      */
    virtual PString GetInterface() const;

    /**Bind this transport to an interface.
        This is generally only relevant for datagram based transports such as
        UDP and TCP is always bound to a local interface once open.

       The default behaviour does nothing.
      */
    virtual bool SetInterface(
      const PString & iface  ///< Interface to use
    );

    /**Get the transport dependent name of the local endpoint.
      */
    virtual OpalTransportAddress GetLocalAddress(
      bool allowNAT = true   ///< Allow translation if remote needs NAT
    ) const = 0;

    /**Set local address to connect from.
       Note that this may not work for all transport types or may work only
       before Connect() has been called.
      */
    virtual PBoolean SetLocalAddress(
      const OpalTransportAddress & address
    ) = 0;

    /**Get the transport address of the remote endpoint.
      */
    virtual OpalTransportAddress GetRemoteAddress() const = 0;

    /**Set remote address to connect to.
       Note that this does not necessarily initiate a transport level
       connection, but only indicates where to connect to. The actual
       connection is made by the Connect() function.
      */
    virtual PBoolean SetRemoteAddress(
      const OpalTransportAddress & address
    ) = 0;

    /**Connect to the remote address.
      */
    virtual PBoolean Connect() = 0;

    /**Connect to the specified address.
      */
    PBoolean ConnectTo(
      const OpalTransportAddress & address
    );

    /**Close the channel.
      */
    virtual PBoolean Close();

    /**Close channel and wait for associated thread to terminate.
      */
    void CloseWait();

    /**Close channel and wait for associated thread to terminate.
       For backward compatibility with OpenH323, now deprecated.
      */
    void CleanUpOnTermination();

    /**Check that the transport address is compatible with transport.
      */
    virtual PBoolean IsCompatibleTransport(
      const OpalTransportAddress & address
    ) const = 0;

    ///<  Promiscious modes for transport
    enum PromisciousModes {
      AcceptFromRemoteOnly,
      AcceptFromAnyAutoSet,
      AcceptFromAny,
      NumPromisciousModes
    };

    /**Set read to promiscuous mode.
       Normally only reads from the specifed remote address are accepted. This
       flag allows packets to be accepted from any remote, provided the
       underlying protocol can do so. For example TCP will do nothing.

       The Read() call may optionally set the remote address automatically to
       whatever the sender host of the last received message was.

       Default behaviour does nothing.
      */
    virtual void SetPromiscuous(
      PromisciousModes promiscuous
    );

    /**Get the transport address of the last received PDU.

       Default behaviour returns GetRemoteAddress().
      */
    virtual OpalTransportAddress GetLastReceivedAddress() const;

    /**Get the interface of the last received PDU arrived on.

       Default behaviour returns GetLocalAddress().
      */
    virtual PString GetLastReceivedInterface() const;

    /**Determine of the transport is running with a background thread.
      */
    virtual PBoolean IsRunning() const;
  //@}

    OpalEndPoint & GetEndPoint() const;

    /**Get the prefix for this transports protocol type.
      */
    virtual const char * GetProtoPrefix() const = 0;
};

typedef PArray<OpalTransportAddress> OpalTransportAddressArray;


class OpalTransportIP : OpalTransport /Abstract/
{
  public:
  /**@name Construction */
  //@{
    /**Create a new transport channel.
     */
    OpalTransportIP(
      OpalEndPoint & endpoint,    ///<  Endpoint object
      PIPSocket::Address binding, ///<  Local interface to use
      WORD port                   ///<  Local port to bind to
    );
  //@}

  /**@name Operations */
  //@{
    /**Get the transport dependent name of the local endpoint.
      */
    virtual OpalTransportAddress GetLocalAddress(
      bool allowNAT = true   ///< Allow translation if remote needs NAT
    ) const;

    /**Set local address to connect from.
       Note that this may not work for all transport types or may work only
       before Connect() has been called.
      */
    virtual PBoolean SetLocalAddress(
      const OpalTransportAddress & address
    );

    /**Get the transport dependent name of the remote endpoint.
      */
    virtual OpalTransportAddress GetRemoteAddress() const;

    /**Set remote address to connect to.
       Note that this does not necessarily initiate a transport level
       connection, but only indicates where to connect to. The actual
       connection is made by the Connect() function.
      */
    virtual PBoolean SetRemoteAddress(
      const OpalTransportAddress & address
    );

  //@}

  protected:
    /**Get the prefix for this transports protocol type.
      */
    virtual const char * GetProtoPrefix() const = 0;
};


class OpalTransportTCP : OpalTransportIP
{
  public:
  /**@name Construction */
  //@{
    /**Create a new transport channel.
     */
    OpalTransportTCP(
      OpalEndPoint & endpoint,    ///<  Endpoint object
      PIPSocket::Address binding = PIPSocket::GetDefaultIpAny(), ///<  Local interface to use
      WORD port = 0,              ///<  Local port to bind to
      PBoolean reuseAddr = PFalse      ///<  Flag for binding to already bound interface
    );
    OpalTransportTCP(
      OpalEndPoint & endpoint,    ///<  Endpoint object
      PTCPSocket * socket         ///<  Socket to use
    );

    /// Destroy the TCP channel
    ~OpalTransportTCP();
  //@}

  /**@name Overides from class OpalTransport */
  //@{
    /**Get indication of the type of underlying transport.
      */
    virtual PBoolean IsReliable() const;

    /**Check that the transport address is compatible with transport.
      */
    virtual PBoolean IsCompatibleTransport(
      const OpalTransportAddress & address
    ) const;

    /**Connect to the remote address.
      */
    virtual PBoolean Connect();

    /**Read a packet from the transport.
       This will read using the transports mechanism for PDU boundaries, for
       example UDP is a single Read() call, while for TCP there is a TPKT
       header that indicates the size of the PDU.
      */
    virtual PBoolean ReadPDU(
      PBYTEArray & pdu  ///<  PDU read from transport
    );

    /**Write a packet to the transport.
       This will write using the transports mechanism for PDU boundaries, for
       example UDP is a single Write() call, while for TCP there is a TPKT
       header that indicates the size of the PDU.
      */
    virtual PBoolean WritePDU(
      const PBYTEArray & pdu     ///<  Packet to write
    );
  //@}
};


class OpalTransportUDP : OpalTransportIP
{
  public:
  /**@name Construction */
  //@{
    /**Create a new transport channel.
     */
    OpalTransportUDP(
      OpalEndPoint & endpoint,    ///<  Endpoint object
      PIPSocket::Address binding = PIPSocket::GetDefaultIpAny(), ///<  Local interface to use
      WORD port = 0,              ///<  Local port to bind to
      bool reuseAddr = false,     ///<  Flag for binding to already bound interface
      bool preOpen = false        ///<  Flag to pre-open socket
    );

     // /**Create a new transport channel.
     // */
    //OpalTransportUDP(
    //  OpalEndPoint & endpoint,              ///<  Endpoint object
     // const PMonitoredSocketsPtr & sockets, ///<  Bundle of sockets from OpalListenerUDP
    //  const PString & iface                 ///<  Local interface to use
    //);

    /// Destroy the UDP channel
    ~OpalTransportUDP();
  //@}

  /**@name Overides from class PChannel */
  //@{
    virtual PBoolean Read(
      void * buffer,
      PINDEX length
    );
  //@}

  /**@name Overides from class OpalTransport */
  //@{
    /**Get indication of the type of underlying transport.
      */
    virtual PBoolean IsReliable() const;

    /**Check that the transport address is compatible with transport.
      */
    virtual PBoolean IsCompatibleTransport(
      const OpalTransportAddress & address
    ) const;

    /**Connect to the remote party.
       This will createa a socket for each interface on the system, then the
       use of WriteConnect() will send out on every interface. ReadPDU() will
       return the first interface that has data, then the user can select
       which interface it wants by further calls to ReadPDU(). Once it has
       selected one it calls SetInterface() to finalise the selection process.
      */
    virtual PBoolean Connect();

    /** Get the interface this transport is bound to.
      */
    virtual PString GetInterface() const;

    /**Bind this transport to an interface.
        This is generally only relevant for datagram based transports such as
        UDP and TCP is always bound to a local interface once open.

       The default behaviour does nothing.
      */
    virtual bool SetInterface(
      const PString & iface  ///< Interface to use
    );

    /**Get the transport dependent name of the local endpoint.
      */
    virtual OpalTransportAddress GetLocalAddress(
      bool allowNAT = true   ///< Allow translation if remote needs NAT
    ) const;

    /**Set local address to connect from.
       Note that this may not work for all transport types or may work only
       before Connect() has been called.
      */
    virtual PBoolean SetLocalAddress(
      const OpalTransportAddress & address
    );

    /**Set remote address to connect to.
       Note that this does not necessarily initiate a transport level
       connection, but only indicates where to connect to. The actual
       connection is made by the Connect() function.
      */
    virtual PBoolean SetRemoteAddress(
      const OpalTransportAddress & address
    );

    /**Set read to promiscuous mode.
       Normally only reads from the specifed remote address are accepted. This
       flag allows packets to be accepted from any remote, provided the
       underlying protocol can do so.

       The Read() call may optionally set the remote address automatically to
       whatever the sender host of the last received message was.

       Default behaviour sets the internal flag, so that Read() operates as
       described.
      */
    virtual void SetPromiscuous(
      PromisciousModes promiscuous
    );

    /**Get the transport address of the last received PDU.

       Default behaviour returns the lastReceivedAddress member variable.
      */
    virtual OpalTransportAddress GetLastReceivedAddress() const;

    /**Get the interface of the last received PDU arrived on.

       Default behaviour returns GetLocalAddress().
      */
    virtual PString GetLastReceivedInterface() const;
};

class OpalListenerTCPS : OpalListenerTCP /NoDefaultCtors/
{
  public:
    OpalListenerTCPS(
      OpalEndPoint & endpoint,                 ///<  Endpoint listener is used for
      PIPSocket::Address binding = PIPSocket::GetDefaultIpAny(), ///<  Local interface to listen on
      WORD port = 0,                           ///<  TCP port to listen for connections
      PBoolean exclusive = true               ///< Exclusive listening mode, no other process can accept on the port
    );
    OpalListenerTCPS(
      OpalEndPoint & endpoint,                  ///<  Endpoint listener is used for
      const OpalTransportAddress & binding,     ///<  Local interface to listen on
      OpalTransportAddress::BindOptions option  ///< OPtions for binding
    );

    /** Destroy the listener thread.
      */
    ~OpalListenerTCPS();

    OpalTransport * Accept(const PTimeInterval & timeout);
    const char * GetProtoPrefix() const;

  protected:
    void Construct();
};

class OpalTransportTCPS : OpalTransportTCP
{
    public:
      OpalTransportTCPS(
        OpalEndPoint & endpoint,    ///<  Endpoint object
        PIPSocket::Address binding = PIPSocket::GetDefaultIpAny(), ///<  Local interface to use
        WORD port = 0,              ///<  Local port to bind to
        PBoolean reuseAddr = false      ///<  Flag for binding to already bound interface
      );
      OpalTransportTCPS(
        OpalEndPoint & endpoint,    ///<  Endpoint object
        PTCPSocket * socket         ///<  Socket to use
      );

      /// Destroy the TCPS channel
      ~OpalTransportTCPS();

      PBoolean IsCompatibleTransport(const OpalTransportAddress & address) const;
      PBoolean Connect();
      PBoolean OnOpen();
      const char * GetProtoPrefix() const;
};
