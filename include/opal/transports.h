/*
 * transport.h
 *
 * Transport declarations
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
 * $Log: transports.h,v $
 * Revision 1.2009  2002/09/06 02:41:00  robertj
 * Added ability to specify stream or datagram (TCP or UDP) transport is to
 * be created from a transport address regardless of the addresses mode.
 *
 * Revision 2.7  2002/07/01 04:56:31  robertj
 * Updated to OpenH323 v1.9.1
 *
 * Revision 2.6  2002/06/16 23:07:47  robertj
 * Fixed several memory leaks, thanks Ted Szoczei
 *
 * Revision 2.5  2002/04/09 00:12:10  robertj
 * Added ability to set the local address on a transport, under some circumstances.
 *
 * Revision 2.4  2001/11/13 04:29:47  robertj
 * Changed OpalTransportAddress CreateTransport and CreateListsner functions
 *   to have extra parameter to control local binding of sockets.
 *
 * Revision 2.3  2001/11/12 05:32:12  robertj
 * Added OpalTransportAddress::GetIpAddress when don't need port number.
 *
 * Revision 2.2  2001/11/09 05:49:47  robertj
 * Abstracted UDP connection algorithm
 *
 * Revision 2.1  2001/11/06 05:40:13  robertj
 * Added OpalListenerUDP class to do listener semantics on a UDP socket.
 *
 * Revision 2.0  2001/07/27 15:48:24  robertj
 * Conversion of OpenH323 to Open Phone Abstraction Library (OPAL)
 *
 */

#ifndef __OPAL_TRANSPORT_H
#define __OPAL_TRANSPORT_H

#ifdef __GNUC__
#pragma interface
#endif


#include <ptlib/sockets.h>


class OpalEndPoint;
class OpalListener;
class OpalTransport;
class OpalInternalTransport;


////////////////////////////////////////////////////////////////

class OpalTransportAddress : public PString
{
  PCLASSINFO(OpalTransportAddress, PString);
  public:
    static const char * TcpPrefix;
    static const char * UdpPrefix;

  /**@name Construction */
  //@{
    OpalTransportAddress();
    OpalTransportAddress(
      const char * address,      /// Address string to parse
      WORD port = 0,             /// Default port number
      const char * proto = NULL  /// Default is "tcp"
    );
    OpalTransportAddress(
      const PString & address,   /// Address string to parse
      WORD port = 0,             /// Default port number
      const char * proto = NULL  /// Default is "tcp"
    );
    OpalTransportAddress(
      const PIPSocket::Address & ip,
      WORD port,
      const char * proto = NULL /// Default is "tcp"
    );
  //@}

  /**@name Operations */
  //@{
    /**Determine if the two transport addresses are equivalent.
      */
    BOOL IsEquivalent(
      const OpalTransportAddress & address
    );

    /**Extract the ip address from transport address.
       Returns FALSE, if the address is not an IP transport address.
      */
    BOOL GetIpAddress(PIPSocket::Address & ip) const;

    /**Extract the ip address and port number from transport address.
       Returns FALSE, if the address is not an IP transport address.
      */
    BOOL GetIpAndPort(PIPSocket::Address & ip, WORD & port) const;

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

       Also note that if the address has a trailing '+' character then the
       socket will be bound using the REUSEADDR option, where relevant.
      */
    OpalListener * CreateListener(
      OpalEndPoint & endpoint,   /// Endpoint object for transport creation.
      BindOptions option         /// Options for how to create listener
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
      OpalEndPoint & endpoint,   /// Endpoint object for transport creation.
      BindOptions option         /// Options for how to create transport
    ) const;
  //@}


  protected:
    void SetInternalTransport(
      WORD port,             /// Default port number
      const char * proto     /// Default is "tcp"
    );

    OpalInternalTransport * transport;
};


PARRAY(OpalTransportAddressArray, OpalTransportAddress);


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
class OpalListener : public PObject
{
    PCLASSINFO(OpalListener, PObject);
  public:
  /**@name Construction */
  //@{
    /**Create a new listener.
     */
    OpalListener(
      OpalEndPoint & endpoint   /// Endpoint listener is used for
    );
  //@}

  /**@name Overrides from PObject */
  //@{
    /**Print the description of the listener to the stream.
      */
    void PrintOn(
      ostream & strm
    ) const;
  //@}

  /**@name Operations */
  //@{
    /** Open the listener.
        A thread is spawned to listen for incoming connections. The notifier
        function acceptHandler is called when a new connection is created. The
        INT parameter to the acceptHandler is a pointer to the new
        OpalTransport instance created by the listener. If this is NULL then
        it indicates an error occurred during the accept.

        If singleThread is FALSE the acceptHandler function is called in the
        context of a new thread and the continues to listen for more
        connections. If TRUE, then the acceptHandler function is called from
        within the listener threads context and no more connections are
        created. That is only a single connection is ever created by this
        listener.
      */
    virtual BOOL Open(
      const PNotifier & acceptHandler,  /// Handler function for new connections
      BOOL singleThread = FALSE         /// If handler function called in this thread
    ) = 0;

    /** Indicate if the listener is open.
      */
    virtual BOOL IsOpen() = 0;

    /**Stop the listener thread and no longer accept incoming connections.
     */
    virtual void Close() = 0;

    /**Accept a new incoming transport.
      */
    virtual OpalTransport * Accept(
      const PTimeInterval & timeout  /// Time to wait for incoming connection
    ) = 0;

    /**Get the local transport address on which this listener may be accessed.
      */
    virtual OpalTransportAddress GetLocalAddress(
      const OpalTransportAddress & preferredAddress = OpalTransportAddress()
    ) const = 0;

    /**Close channel and wait for associated thread to terminate.
      */
    void CloseWait();
  //@}


  protected:
    /**Handle incoming connections and dispatch them in new threads based on
       the OpalTransport class. This is defined in the descendent class that
       knows what the low level transport is, eg OpalListenerTCP for the
       TCP/IP protocol.

       Note this function does not return until the Close() function is called
       or there is some other error.
     */
    PDECLARE_NOTIFIER(PThread, OpalListener, ListenForConnections);
    BOOL StartThread(
      const PNotifier & acceptHandler,  /// Handler function for new connections
      BOOL singleThread                /// If handler function called in this thread
    );

    OpalEndPoint & endpoint;
    PThread      *  thread;
    PNotifier      acceptHandler;
    BOOL           singleThread;
};


PLIST(OpalListenerList, OpalListener);


class OpalListenerIP : public OpalListener
{
  PCLASSINFO(OpalListenerIP, OpalListener);
  public:
  /**@name Construction */
  //@{
    /**Create a new IP listener.
     */
    OpalListenerIP(
      OpalEndPoint & endpoint,                 /// Endpoint listener is used for
      PIPSocket::Address binding = INADDR_ANY, /// Local interface to listen on
      WORD port = 0,                           /// TCP port to listen for connections
      BOOL exclusive = TRUE
    );
  //@}

  /**@name Overrides from OpalListener */
  //@{
    /**Get the local transport address on which this listener may be accessed.
      */
    virtual OpalTransportAddress GetLocalAddress(
      const OpalTransportAddress & preferredAddress = OpalTransportAddress()
    ) const;
  //@}

  /**@name Operations */
  //@{
    WORD GetListenerPort() const { return listenerPort; }

    virtual const char * GetProtoPrefix() const = 0;
  //@}


  protected:
    PIPSocket::Address localAddress;
    WORD               listenerPort;
    BOOL               exclusiveListener;
};


class OpalListenerTCP : public OpalListenerIP
{
  PCLASSINFO(OpalListenerTCP, OpalListenerIP);
  public:
  /**@name Construction */
  //@{
    /**Create a new listener.
     */
    OpalListenerTCP(
      OpalEndPoint & endpoint,                 /// Endpoint listener is used for
      PIPSocket::Address binding = INADDR_ANY, /// Local interface to listen on
      WORD port = 0,                           /// TCP port to listen for connections
      BOOL exclusive = TRUE
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

        If singleThread is FALSE the acceptHandler function is called in the
        context of a new thread and the continues to listen for more
        connections. If TRUE, then the acceptHandler function is called from
        within the listener threads context and no more connections are
        created. That is only a single connection is ever created by this
        listener.

        If acceptHandler is NULL, then no thread is started and it is assumed
        that the caller is responsible for calling Accept() and waiting for
        the new connection.
      */
    virtual BOOL Open(
      const PNotifier & acceptHandler,  /// Handler function for new connections
      BOOL singleThread = FALSE         /// If handler function called in this thread
    );

    /** Indicate if the listener is open.
      */
    virtual BOOL IsOpen();

    /**Stop the listener thread and no longer accept incoming connections.
     */
    virtual void Close();

    /**Accept a new incoming transport.
      */
    virtual OpalTransport * Accept(
      const PTimeInterval & timeout  /// Time to wait for incoming connection
    );
  //@}


  protected:
    virtual const char * GetProtoPrefix() const;

    PTCPSocket listener;
};


class OpalListenerUDP : public OpalListenerIP
{
  PCLASSINFO(OpalListenerUDP, OpalListenerIP);
  public:
  /**@name Construction */
  //@{
    /**Create a new listener.
     */
    OpalListenerUDP(
      OpalEndPoint & endpoint,                 /// Endpoint listener is used for
      PIPSocket::Address binding = INADDR_ANY, /// Local interface to listen on
      WORD port = 0,                           /// TCP port to listen for connections
      BOOL exclusive = TRUE
    );

    /** Destroy the listener thread.
      */
    ~OpalListenerUDP();
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

        If singleThread is FALSE the acceptHandler function is called in the
        context of a new thread and the continues to listen for more
        connections. If TRUE, then the acceptHandler function is called from
        within the listener threads context and no more connections are
        created. That is only a single connection is ever created by this
        listener.

        If acceptHandler is NULL, then no thread is started and it is assumed
        that the caller is responsible for calling Accept() and waiting for
        the new connection.
      */
    virtual BOOL Open(
      const PNotifier & acceptHandler,  /// Handler function for new connections
      BOOL singleThread = TRUE          /// If handler function called in this thread
    );

    /** Indicate if the listener is open.
      */
    virtual BOOL IsOpen();

    /**Stop the listener thread and no longer accept incoming connections.
     */
    virtual void Close();

    /**Accept a new incoming transport.
      */
    virtual OpalTransport * Accept(
      const PTimeInterval & timeout  /// Time to wait for incoming connection
    );
  //@}


  protected:
    virtual const char * GetProtoPrefix() const;
    BOOL OpenOneSocket(const PIPSocket::Address & address);

    PSocketList listeners;
};


////////////////////////////////////////////////////////////////

/**This class describes a I/O transport for a protocol.
   A "transport" is an object that allows the transfer and processing of data
   from one entity to another.
 */
class OpalTransport : public PIndirectChannel
{
    PCLASSINFO(OpalTransport, PIndirectChannel);
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

  /**@name Overrides from PObject */
  //@{
    /**Print the description of the listener to the stream.
      */
    void PrintOn(
      ostream & strm
    ) const;
  //@}

  /**@name Operations */
  //@{
    /**Get indication of the type of underlying transport.
      */
    virtual BOOL IsReliable() const = 0;

    /**Get the transport dependent name of the local endpoint.
      */
    virtual OpalTransportAddress GetLocalAddress() const = 0;

    /**Set local address to connect from.
       Note that this may not work for all transport types or may work only
       before Connect() has been called.
      */
    virtual BOOL SetLocalAddress(
      const OpalTransportAddress & address
    ) = 0;

    /**Get the transport dependent name of the remote endpoint.
      */
    virtual OpalTransportAddress GetRemoteAddress() const = 0;

    /**Set remote address to connect to.
       Note that this does not necessarily initiate a transport level
       connection, but only indicates where to connect to. The actual
       connection is made by the Connect() function.
      */
    virtual BOOL SetRemoteAddress(
      const OpalTransportAddress & address
    ) = 0;

    /**Connect to the remote address.
      */
    virtual BOOL Connect() = 0;

    /**Connect to the specified address.
      */
    BOOL ConnectTo(
      const OpalTransportAddress & address
    ) { return SetRemoteAddress(address) && Connect(); }

    /**End a connection to the remote address.
       This is requried in some circumstances where the connection to the
       remote is not atomic.

       The default behaviour does nothing.
      */
    virtual void EndConnect(
      const OpalTransportAddress & localAddress  /// Resultant local address
    );

    /**Close the channel.
      */
    virtual BOOL Close();

    /**Close channel and wait for associated thread to terminate.
      */
    void CloseWait();

    /**Check that the transport address is compatible with transport.
      */
    virtual BOOL IsCompatibleTransport(
      const OpalTransportAddress & address
    ) const = 0;

    /**Set read to promiscuous mode.
       Normally only reads from the specifed remote address are accepted. This
       flag allows the remote address to be automatically set to whatever the
       sender of the last received message was.

       Default behaviour does nothing.
      */
    virtual void SetPromiscuous(
      BOOL promiscuous
    );

    /**Read a packet from the transport.
       This will read using the transports mechanism for PDU boundaries, for
       example UDP is a single Read() call, while for TCP there is a TPKT
       header that indicates the size of the PDU.
      */
    virtual BOOL ReadPDU(
      PBYTEArray & packet   /// Packet read from transport
    ) = 0;

    /**Write a packet to the transport.
       This will write using the transports mechanism for PDU boundaries, for
       example UDP is a single Write() call, while for TCP there is a TPKT
       header that indicates the size of the PDU.
      */
    virtual BOOL WritePDU(
      const PBYTEArray & pdu     /// Packet to write
    ) = 0;

    typedef BOOL (*WriteConnectCallback)(OpalTransport & transport, PObject * data);

    /**Write the first packet to the transport, after a connect.
       This will adjust the transport object and call the callback function,
       possibly multiple times for some transport types.

       It is expected that this is used just after a Connect() call where some
       transports (eg UDP) cannot determine its local address which is
       required in the PDU to be sent. This must be done fer each interface so
       WriteConnect() calls WriteConnectCallback for each interface. The
       subsequent ReadPDU() returns the answer from the first interface.

       The default behaviour simply calls the WriteConnectCallback function.
      */
    virtual BOOL WriteConnect(
      WriteConnectCallback function,  /// Function for writing data
      PObject * data                  /// Data to write
    );

    /**Attach a thread to the transport.
      */
    virtual void AttachThread(
      PThread * thread
    );

    /**Determine of the transport is running with a background thread.
      */
    virtual BOOL IsRunning() const;
  //@}

    OpalEndPoint & GetEndPoint() const { return endpoint; }

  protected:
    OpalEndPoint & endpoint;
    PThread      * thread;      /// Thread handling the transport
};


class OpalTransportIP : public OpalTransport
{
  PCLASSINFO(OpalTransportIP, OpalTransport);
  public:
  /**@name Construction */
  //@{
    /**Create a new transport channel.
     */
    OpalTransportIP(
      OpalEndPoint & endpoint,    /// Endpoint object
      PIPSocket::Address binding, /// Local interface to use
      WORD port                   /// Local port to bind to
    );
  //@}

  /**@name Operations */
  //@{
    /**Get the transport dependent name of the local endpoint.
      */
    virtual OpalTransportAddress GetLocalAddress() const;

    /**Set local address to connect from.
       Note that this may not work for all transport types or may work only
       before Connect() has been called.
      */
    virtual BOOL SetLocalAddress(
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
    virtual BOOL SetRemoteAddress(
      const OpalTransportAddress & address
    );

  //@}

  protected:
    /**Get the prefix for this transports protocol type.
      */
    virtual const char * GetProtoPrefix() const = 0;

    PIPSocket::Address localAddress;  // Address of the local interface
    WORD               localPort;
    PIPSocket::Address remoteAddress; // Address of the remote host
    WORD               remotePort;
};


class OpalTransportTCP : public OpalTransportIP
{
    PCLASSINFO(OpalTransportTCP, OpalTransportIP);
  public:
  /**@name Construction */
  //@{
    /**Create a new transport channel.
     */
    OpalTransportTCP(
      OpalEndPoint & endpoint,    /// Endpoint object
      PIPSocket::Address binding = INADDR_ANY, /// Local interface to use
      WORD port = 0,              /// Local port to bind to
      BOOL reuseAddr = FALSE      /// Flag for binding to already bound interface
    );
    OpalTransportTCP(
      OpalEndPoint & endpoint,    /// Endpoint object
      PTCPSocket * socket         /// Socket to use
    );

    /// Destroy the TCP channel
    ~OpalTransportTCP();
  //@}

  /**@name Overides from class OpalTransport */
  //@{
    /**Get indication of the type of underlying transport.
      */
    virtual BOOL IsReliable() const;

    /**Check that the transport address is compatible with transport.
      */
    virtual BOOL IsCompatibleTransport(
      const OpalTransportAddress & address
    ) const;

    /**Connect to the remote address.
      */
    virtual BOOL Connect();

    /**Read a packet from the transport.
       This will read using the transports mechanism for PDU boundaries, for
       example UDP is a single Read() call, while for TCP there is a TPKT
       header that indicates the size of the PDU.
      */
    virtual BOOL ReadPDU(
      PBYTEArray & pdu  /// PDU read from transport
    );

    /**Write a packet to the transport.
       This will write using the transports mechanism for PDU boundaries, for
       example UDP is a single Write() call, while for TCP there is a TPKT
       header that indicates the size of the PDU.
      */
    virtual BOOL WritePDU(
      const PBYTEArray & pdu     /// Packet to write
    );
  //@}


  protected:
    /**Get the prefix for this transports protocol type.
      */
    virtual const char * GetProtoPrefix() const;

    /**This callback is executed when the Open() function is called with
       open channels. It may be used by descendent channels to do any
       handshaking required by the protocol that channel embodies.

       The default behaviour is to simply return TRUE.

       @return
       Returns TRUE if the protocol handshaking is successful.
     */
    virtual BOOL OnOpen();


    BOOL reuseAddressFlag;
};


class OpalTransportUDP : public OpalTransportIP
{
  PCLASSINFO(OpalTransportUDP, OpalTransportIP);
  public:
  /**@name Construction */
  //@{
    /**Create a new transport channel.
     */
    OpalTransportUDP(
      OpalEndPoint & endpoint,    /// Endpoint object
      PIPSocket::Address binding = INADDR_ANY, /// Local interface to use
      WORD port = 0,                           /// Local port to use
      BOOL reuseAddr = FALSE      /// Flag for binding to already bound interface
    );

    /**Create a new transport channel.
     */
    OpalTransportUDP(
      OpalEndPoint & endpoint,    /// Endpoint object
      PUDPSocket & socket         /// Preopened socket
    );

    /**Create a new transport channel.
     */
    OpalTransportUDP(
      OpalEndPoint & endpoint,          /// Endpoint object
      PIPSocket::Address localAddress,  /// Local interface to use
      const PBYTEArray & preReadPacket, /// Packet already read by OpalListenerUDP
      PIPSocket::Address remoteAddress, /// Remote address received PDU on
      WORD remotePort                   /// Remote port received PDU on
    );

    /// Destroy the UDP channel
    ~OpalTransportUDP();
  //@}

  /**@name Overides from class PChannel */
  //@{
    virtual BOOL Read(
      void * buffer,
      PINDEX length
    );

    /**Conditionally close the sockets.
     */
    virtual BOOL Close();
  //@}

  /**@name Overides from class OpalTransport */
  //@{
    /**Get indication of the type of underlying transport.
      */
    virtual BOOL IsReliable() const;

    /**Check that the transport address is compatible with transport.
      */
    virtual BOOL IsCompatibleTransport(
      const OpalTransportAddress & address
    ) const;

    /**Connect to the remote party.
       This will createa a socket for each interface on the system, then the
       use of WriteConnect() will send out on every interface. ReadPDU() will
       return the first interface that has data, then the user can select
       which interface it wants by further calls to ReadPDU(). Once it has
       selected one it calls EndConnect() to finalise the selection process.
      */
    virtual BOOL Connect();

    /**End a connection to the remote address.
       This is requried in some circumstances where the connection to the
       remote is not atomic.

       The default behaviour uses the socket defined by the localAddress
       parameter.
      */
    virtual void EndConnect(
      const OpalTransportAddress & localAddress  /// Resultant local address
    );

    /**Set local address to connect from.
       Note that this may not work for all transport types or may work only
       before Connect() has been called.
      */
    virtual BOOL SetLocalAddress(
      const OpalTransportAddress & address
    );

    /**Set remote address to connect to.
       Note that this does not necessarily initiate a transport level
       connection, but only indicates where to connect to. The actual
       connection is made by the Connect() function.
      */
    virtual BOOL SetRemoteAddress(
      const OpalTransportAddress & address
    );

    /**Set read to promiscuous mode.
       Normally only reads from the specifed remote address are accepted. This
       flag allows the remote address to be automatically set to whatever the
       sender of the last received message was.

       Default behaviour does nothing.
      */
    virtual void SetPromiscuous(
      BOOL promiscuous
    );

    /**Read a packet from the transport.
       This will read using the transports mechanism for PDU boundaries, for
       example UDP is a single Read() call, while for TCP there is a TPKT
       header that indicates the size of the PDU.
      */
    virtual BOOL ReadPDU(
      PBYTEArray & packet   /// Packet read from transport
    );

    /**Write a packet to the transport.
       This will write using the transports mechanism for PDU boundaries, for
       example UDP is a single Write() call, while for TCP there is a TPKT
       header that indicates the size of the PDU.
      */
    virtual BOOL WritePDU(
      const PBYTEArray & pdu     /// Packet to write
    );

    /**Write the first packet to the transport, after a connect.
       This will adjust the transport object and call the callback function,
       possibly multiple times for some transport types.

       It is expected that this is used just after a Connect() call where some
       transports (eg UDP) cannot determine its local address which is
       required in the PDU to be sent. This must be done fer each interface so
       WriteConnect() calls WriteConnectCallback for each interface. The
       subsequent ReadPDU() returns the answer from the first interface.
      */
    virtual BOOL WriteConnect(
      WriteConnectCallback function,  /// Function for writing data
      PObject * data                  /// Data to write
    );
  //@}


  protected:
    /**Get the prefix for this transports protocol type.
      */
    virtual const char * GetProtoPrefix() const;

    BOOL        promiscuousReads;
    PBYTEArray  preReadPacket;
    PSocketList connectSockets;
};


#endif  // __OPAL_TRANSPORT_H


// End of File ///////////////////////////////////////////////////////////////
