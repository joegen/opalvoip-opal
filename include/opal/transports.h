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
 * Revision 1.2001  2001/07/27 15:48:24  robertj
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
    /**Extract the ip address and port number from transport address.
       Returns FALSE, if the address is not an IP transport address.
      */
    BOOL GetIpAndPort(PIPSocket::Address & ip, WORD & port) const;

    /**Translate the transport address to a more human readable form.
       Returns the hostname if using IP.
      */
    virtual PString GetHostName() const;

    /**Create a listener based on this transport address.

       For example an address of "tcp$10.0.0.1:1720" would create a TCP
       listening socket that would be bound to the specific interface
       10.0.0.1 and listens on port 1720. Note that the address
       "tcp$*:1720" can be used to bind to INADDR_ANY.

       Also note that if the address has a trailing '+' character then the
       socket will be bound using the REUSEADDR option.
      */
    OpalListener * CreateListener(
      OpalEndPoint & endpoint   /// Endpoint object for transport creation.
    ) const;

    /**Create a listener compatible for this address type.
       This is similar to CreateListener() but does not use the TSAP specified
       in the OpalTransport. For example an address of "tcp$10.0.0.1:1720"
       would create a TCP listening socket that would be bound to the specific
       interface 10.0.0.1 but listens on a random OS allocated port number.
      */
    OpalListener * CreateCompatibleListener(
      OpalEndPoint & endpoint   /// Endpoint object for transport creation.
    ) const;

    /**Create a transport suitable for this address type.
      */
    virtual OpalTransport * CreateTransport(
      OpalEndPoint & endpoint   /// Endpoint object for transport creation.
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
    virtual BOOL Close() = 0;

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


class OpalListenerTCP : public OpalListener
{
  PCLASSINFO(OpalListenerTCP, OpalListener);
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
    virtual BOOL Close();

    /**Accept a new incoming transport.
      */
    virtual OpalTransport * Accept(
      const PTimeInterval & timeout  /// Time to wait for incoming connection
    );

    /**Get the local transport address on which this listener may be accessed.
      */
    virtual OpalTransportAddress GetLocalAddress(
      const OpalTransportAddress & preferredAddress = OpalTransportAddress()
    ) const;
  //@}

  /**@name Operations */
  //@{
    WORD GetListenerPort() const { return listener.GetPort(); }
  //@}


  protected:
    PTCPSocket         listener;
    PIPSocket::Address localAddress;
    BOOL               exclusiveListener;
};


////////////////////////////////////////////////////////////////

/**This class describes a I/O transport protocol..
   A "transport" is an object that listens for incoming connections on the
   particular transport.
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

  /**@name Operations */
  //@{
    /**Get the transport dependent name of the local endpoint.
      */
    virtual OpalTransportAddress GetLocalAddress() const = 0;

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

    /**Read a protocol data unit from the transport.
       This will read using the transports mechanism for PDU boundaries, for
       example UDP is a single Read() call, while for TCP there is a TPKT
       header that indicates the size of the PDU.
      */
    virtual BOOL ReadPDU(
      PBYTEArray & pdu   /// PDU read from transport
    ) = 0;

    /**Write a protocol data unit from the transport.
       This will write using the transports mechanism for PDU boundaries, for
       example UDP is a single Write() call, while for TCP there is a TPKT
       header that indicates the size of the PDU.
      */
    virtual BOOL WritePDU(
      const PBYTEArray & pdu  /// PDU to write
    ) = 0;

    /**Attach a thread to the transport.
      */
    void AttachThread(
      PThread * thread
    );
  //@}


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
      WORD remPort                /// Remote port to use
    );
  //@}

  /**@name Operations */
  //@{
    /**Get the transport dependent name of the local endpoint.
      */
    virtual OpalTransportAddress GetLocalAddress() const;

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
      WORD port = 0                            /// Remote port to use
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
    /**Check that the transport address is compatible with transport.
      */
    virtual BOOL IsCompatibleTransport(
      const OpalTransportAddress & address
    ) const;

    /**Connect to the remote address.
      */
    virtual BOOL Connect();

    /**Read a protocol data unit from the transport.
       This will read using the transports mechanism for PDU boundaries, for
       example UDP is a single Read() call, while for TCP there is a TPKT
       header that indicates the size of the PDU.
      */
    virtual BOOL ReadPDU(
      PBYTEArray & pdu   /// PDU read from transport
    );

    /**Write a protocol data unit from the transport.
       This will write using the transports mechanism for PDU boundaries, for
       example UDP is a single Write() call, while for TCP there is a TPKT
       header that indicates the size of the PDU.
      */
    virtual BOOL WritePDU(
      const PBYTEArray & pdu  /// PDU to write
    );
  //@}


  protected:
    /**This callback is executed when the Open() function is called with
       open channels. It may be used by descendent channels to do any
       handshaking required by the protocol that channel embodies.

       The default behaviour is to simply return TRUE.

       @return
       Returns TRUE if the protocol handshaking is successful.
     */
    virtual BOOL OnOpen();
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
      WORD port = 0,                           /// Remote port to use
      BOOL promiscuous = FALSE    /// Accept data from anyone
    );

    /// Destroy the UDP channel
    ~OpalTransportUDP();
  //@}

  /**@name Overides from class OpalTransport */
  //@{
    /**Check that the transport address is compatible with transport.
      */
    virtual BOOL IsCompatibleTransport(
      const OpalTransportAddress & address
    ) const;

    /**Connect to the remote party.
      */
    virtual BOOL Connect();

    /**Set read to promiscuous mode.
       Normally only reads from the specifed remote address are accepted. This
       flag allows the remote address to be automatically set to whatever the
       sender of the last received message was.

       Default behaviour does nothing.
      */
    virtual void SetPromiscuous(
      BOOL promiscuous
    );

    /**Read a protocol data unit from the transport.
       This will read using the transports mechanism for PDU boundaries, for
       example UDP is a single Read() call, while for TCP there is a TPKT
       header that indicates the size of the PDU.
      */
    virtual BOOL ReadPDU(
      PBYTEArray & pdu   /// PDU read from transport
    );

    /**Write a protocol data unit from the transport.
       This will write using the transports mechanism for PDU boundaries, for
       example UDP is a single Write() call, while for TCP there is a TPKT
       header that indicates the size of the PDU.
      */
    virtual BOOL WritePDU(
      const PBYTEArray & pdu  /// PDU to write
    );
  //@}


  protected:
    BOOL promiscuousReads;
};


#endif  // __OPAL_TRANSPORT_H


// End of File ///////////////////////////////////////////////////////////////
