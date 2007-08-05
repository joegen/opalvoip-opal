/*
 * transport.h
 *
 * Transport declarations
 *
 * Open Phone Abstraction Library (OPAL)
 * Formally known as the Open H323 project.
 *
 * Copyright (c) 2001 Equivalence Pty. Ltd.
 * Portions Copyright (C) 2006 by Post Increment
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
 * Contributor(s): Post Increment
 *     Portions of this code were written with the assistance of funding from
 *     US Joint Forces Command Joint Concept Development & Experimentation (J9)
 *     http://www.jfcom.mil/about/abt_j9.htm
 *
 * $Log: transports.h,v $
 * Revision 1.2025.2.2  2007/08/05 13:12:16  hfriederich
 * Backport from HEAD - Changes since last commit
 *
 * Revision 2.24.2.1  2007/06/12 16:29:02  hfriederich
 * (Backport from HEAD)
 * Major rework of how SIP utilises sockets, using new "socket bundling"
 *   subsystem
 * Several other bugfixes
 *
 * Revision 2.25  2007/06/10 08:55:11  rjongbloed
 * Major rework of how SIP utilises sockets, using new "socket bundling" subsystem.
 *
 * Revision 2.24  2006/10/21 11:48:48  hfriederich
 * Allow to find out how many interfaces are used before calling WriteConnect()
 *
 * Revision 2.23  2006/08/29 01:37:11  csoutheren
 * Change secure URLs to use h323s and tcps to be inline with sips
 *
 * Revision 2.22  2006/08/29 00:55:04  csoutheren
 * Fix problem with creating TCP transports associated with UDP tranports
 *
 * Revision 2.21  2006/08/28 00:51:12  csoutheren
 * Applied 1545808 - OpalTransportUDP connectSockets protection
 * Thanks to Drazen Dimoti
 *
 * Revision 2.20  2006/08/21 05:29:25  csoutheren
 * Messy but relatively simple change to add support for secure (SSL/TLS) TCP transport
 * and secure H.323 signalling via the sh323 URL scheme
 *
 * Revision 2.19  2005/11/30 13:35:26  csoutheren
 * Changed tags for Doxygen
 *
 * Revision 2.18  2005/09/19 20:48:23  dsandras
 * Added the "can reuse address" flag to the OpalTransportUDP class.
 *
 * Revision 2.17  2005/01/16 23:07:33  csoutheren
 * Fixed problem with IPv6 INADDR_ANY
 *
 * Revision 2.16  2004/03/22 11:39:44  rjongbloed
 * Fixed problems with correctly terminating the OpalTransportUDP that is generated from
 *   an OpalListenerUDP, this should not close the socket or stop the thread.
 *
 * Revision 2.15  2004/03/13 06:25:52  rjongbloed
 * Slight rearrangement of local party name and alias list to beter match common
 *   behaviour in ancestor.
 * Abstracted local party name for endpoint into ancestor from H.,323.
 *
 * Revision 2.14  2004/03/11 06:54:27  csoutheren
 * Added ability to disable SIP or H.323 stacks
 *
 * Revision 2.13  2004/02/19 10:47:01  rjongbloed
 * Merged OpenH323 version 1.13.1 changes.
 *
 * Revision 2.12  2003/01/07 04:39:53  robertj
 * Updated to OpenH323 v1.11.2
 *
 * Revision 2.11  2002/11/10 11:33:17  robertj
 * Updated to OpenH323 v1.10.3
 *
 * Revision 1.43  2003/12/29 13:28:45  dominance
 * fixed docbook syntax trying to generate LaTeX formula with ip$10.x.x.x.
 *
 * Revision 1.42  2003/04/10 09:44:55  robertj
 * Added associated transport to new GetInterfaceAddresses() function so
 *   interfaces can be ordered according to active transport links. Improves
 *   interoperability.
 * Replaced old listener GetTransportPDU() with GetInterfaceAddresses()
 *   and H323SetTransportAddresses() functions.
 *
 * Revision 1.41  2003/04/10 01:03:25  craigs
 * Added functions to access to lists of interfaces
 *
 * Revision 1.40  2003/03/21 05:24:02  robertj
 * Added setting of remote port in UDP transport constructor.
 *
 * Revision 1.39  2003/02/06 04:29:23  robertj
 * Added more support for adding things to H323TransportAddressArrays
 *
 * Revision 1.38  2002/11/21 06:39:56  robertj
 * Changed promiscuous mode to be three way. Fixes race condition in gkserver
 *   which can cause crashes or more PDUs to be sent to the wrong place.
 *
 * Revision 1.37  2002/11/10 08:10:43  robertj
 * Moved constants for "well known" ports to better place (OPAL change).
 *
 * Revision 2.10  2002/09/16 02:52:35  robertj
 * Added #define so can select if #pragma interface/implementation is used on
 *   platform basis (eg MacOS) rather than compiler, thanks Robert Monaghan.
 *
 * Revision 2.9  2002/09/12 06:58:17  robertj
 * Removed protocol prefix strings as static members as has problems with
 *   use in DLL environment.
 *
 * Revision 2.8  2002/09/06 02:41:00  robertj
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

#ifdef P_USE_PRAGMA
#pragma interface
#endif

#include <opal/buildopts.h>

#include <ptlib/sockets.h>
#include <ptclib/psockbun.h>


class OpalManager;
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
      */
    BOOL IsEquivalent(
      const OpalTransportAddress & address
    ) const;

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


  protected:
    void SetInternalTransport(
      WORD port,             ///<  Default port number
      const char * proto     ///<  Default is "tcp"
    );

    OpalInternalTransport * transport;
};


PDECLARE_ARRAY(OpalTransportAddressArray, OpalTransportAddress)
  public:
    OpalTransportAddressArray(
      const OpalTransportAddress & address
    ) { AppendAddress(address); }
    OpalTransportAddressArray(
      const PStringArray & array
    ) { AppendStringCollection(array); }
    OpalTransportAddressArray(
      const PStringList & list
    ) { AppendStringCollection(list); }
    OpalTransportAddressArray(
      const PSortedStringList & list
    ) { AppendStringCollection(list); }

    void AppendString(
      const char * address
    );
    void AppendString(
      const PString & address
    );
    void AppendAddress(
      const OpalTransportAddress & address
    );

  protected:
    void AppendStringCollection(
      const PCollection & coll
    );
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
class OpalListener : public PObject
{
    PCLASSINFO(OpalListener, PObject);
  public:
  /**@name Construction */
  //@{
    /**Create a new listener.
     */
    OpalListener(
      OpalEndPoint & endpoint   ///<  Endpoint listener is used for
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

        If singleThread is FALSE the acceptHandler function is called in the
        context of a new thread and the continues to listen for more
        connections. If TRUE, then the acceptHandler function is called from
        within the listener threads context and no more connections are
        created. That is only a single connection is ever created by this
        listener.
      */
    virtual BOOL Open(
      const PNotifier & acceptHandler,  ///<  Handler function for new connections
      ThreadMode mode = SpawnNewThreadMode ///<  How handler function is called thread wise
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
      const PTimeInterval & timeout  ///<  Time to wait for incoming connection
    ) = 0;

    /**Create a transport compatible with this listener.
     */
    virtual OpalTransport * CreateTransport(
      const OpalTransportAddress & localAddress
    ) const = 0;

    /**Get the local transport address on which this listener may be accessed.
      */
    virtual OpalTransportAddress GetLocalAddress(
      const OpalTransportAddress & preferredAddress = OpalTransportAddress()
    ) const = 0;

    /**Get the local transport address on which this listener may be accessed.
       For backward compatibility with OpenH323, now deprecated.
      */
    OpalTransportAddress GetTransportAddress() const { return GetLocalAddress(); }

    /**Close channel and wait for associated thread to terminate.
      */
    void CloseWait();

    /**Close channel and wait for associated thread to terminate.
       For backward compatibility with OpenH323, now deprecated.
      */
    void CleanUpOnTermination() { CloseWait(); }
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
      const PNotifier & acceptHandler,  ///<  Handler function for new connections
      ThreadMode mode                   ///<  How handler function is called thread wise
    );

    OpalEndPoint & endpoint;
    PThread      * thread;
    PNotifier      acceptHandler;
    ThreadMode     threadMode;
};


PLIST(OpalListenerList, OpalListener);


/** Return a list of transport addresses corresponding to a listener list
  */
OpalTransportAddressArray OpalGetInterfaceAddresses(
  const OpalListenerList & listeners, ///<  List of listeners
  BOOL excludeLocalHost = TRUE,       ///<  Flag to exclude 127.0.0.1
  OpalTransport * associatedTransport = NULL
                          ///<  Associated transport for precedence and translation
);

OpalTransportAddressArray OpalGetInterfaceAddresses(
  const OpalTransportAddress & addr,  ///<  Possible INADDR_ANY address
  BOOL excludeLocalHost = TRUE,       ///<  Flag to exclude 127.0.0.1
  OpalTransport * associatedTransport = NULL
                          ///<  Associated transport for precedence and translation
);


class OpalListenerIP : public OpalListener
{
  PCLASSINFO(OpalListenerIP, OpalListener);
  public:
  /**@name Construction */
  //@{
    /**Create a new IP listener.
     */
    OpalListenerIP(
      OpalEndPoint & endpoint,                 ///<  Endpoint listener is used for
      PIPSocket::Address binding = PIPSocket::GetDefaultIpAny(), ///<  Local interface to listen on
      WORD port = 0,                           ///<  TCP port to listen for connections
      BOOL exclusive = TRUE
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
      OpalEndPoint & endpoint,                 ///<  Endpoint listener is used for
      PIPSocket::Address binding = PIPSocket::GetDefaultIpAny(), ///<  Local interface to listen on
      WORD port = 0,                           ///<  TCP port to listen for connections
      BOOL exclusive = TRUE
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
      const PNotifier & acceptHandler,  ///<  Handler function for new connections
      ThreadMode mode = SpawnNewThreadMode ///<  How handler function is called thread wise
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
      const PTimeInterval & timeout  ///<  Time to wait for incoming connection
    );

    /**Create a transport compatible with this listener.
     */
    virtual OpalTransport * CreateTransport(
      const OpalTransportAddress & localAddress
    ) const;
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
      OpalEndPoint & endpoint,  ///<  Endpoint listener is used for
      PIPSocket::Address binding = PIPSocket::GetDefaultIpAny(), ///<  Local interface to listen on
      WORD port = 0,            ///<  TCP port to listen for connections
      BOOL exclusive = TRUE
    );
    OpalListenerUDP(
      OpalEndPoint & endpoint,  ///<  Endpoint listener is used for
      const OpalTransportAddress & binding,  ///<  Local interface to listen on
      OpalTransportAddress::BindOptions option ///< Options for binding
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
      const PNotifier & acceptHandler,  ///<  Handler function for new connections
      ThreadMode mode = SpawnNewThreadMode ///<  How handler function is called thread wise
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
      const PTimeInterval & timeout  ///<  Time to wait for incoming connection
    );

    /**Create a transport compatible with this listener.
     */
    virtual OpalTransport * CreateTransport(
      const OpalTransportAddress & localAddress
    ) const;
  //@}


  protected:
    virtual const char * GetProtoPrefix() const;

    PMonitoredSocketsPtr listenerBundle;
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
    
    /** Get the interface this transport is bound to.
      */
    virtual PString GetInterface() const;

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

    /**Get the transport address of the remote endpoint.
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
      const PString & iface ///< Interface to finally use
    );

    /**Close the channel.
      */
    virtual BOOL Close();

    /**Close channel and wait for associated thread to terminate.
      */
    void CloseWait();

    /**Close channel and wait for associated thread to terminate.
       For backward compatibility with OpenH323, now deprecated.
      */
    void CleanUpOnTermination() { CloseWait(); }

    /**Check that the transport address is compatible with transport.
      */
    virtual BOOL IsCompatibleTransport(
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

    /**Read a protocol data unit from the transport.
       This will read using the transports mechanism for PDU boundaries, for
       example UDP is a single Read() call, while for TCP there is a TPKT
       header that indicates the size of the PDU.
      */
    virtual BOOL ReadPDU(
      PBYTEArray & packet   ///<  Packet read from transport
    ) = 0;

    /**Write a packet to the transport.
       This will write using the transports mechanism for PDU boundaries, for
       example UDP is a single Write() call, while for TCP there is a TPKT
       header that indicates the size of the PDU.
      */
    virtual BOOL WritePDU(
      const PBYTEArray & pdu     ///<  Packet to write
    ) = 0;

    typedef BOOL (*WriteConnectCallback)(OpalTransport & transport, void * userData);

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
      WriteConnectCallback function,  ///<  Function for writing data
      void * userData                 ///<  user data to pass to write function
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
    PThread      * thread;      ///<  Thread handling the transport
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
      OpalEndPoint & endpoint,    ///<  Endpoint object
      PIPSocket::Address binding, ///<  Local interface to use
      WORD port                   ///<  Local port to bind to
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
      OpalEndPoint & endpoint,    ///<  Endpoint object
      PIPSocket::Address binding = PIPSocket::GetDefaultIpAny(), ///<  Local interface to use
      WORD port = 0,              ///<  Local port to bind to
      BOOL reuseAddr = FALSE      ///<  Flag for binding to already bound interface
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
      PBYTEArray & pdu  ///<  PDU read from transport
    );

    /**Write a packet to the transport.
       This will write using the transports mechanism for PDU boundaries, for
       example UDP is a single Write() call, while for TCP there is a TPKT
       header that indicates the size of the PDU.
      */
    virtual BOOL WritePDU(
      const PBYTEArray & pdu     ///<  Packet to write
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
      OpalEndPoint & endpoint,    ///<  Endpoint object
      PIPSocket::Address binding = PIPSocket::GetDefaultIpAny(), ///<  Local interface to use
      WORD port = 0,              ///<  Local port to bind to
      BOOL reuseAddr = FALSE      ///<  Flag for binding to already bound interface
    );

    /**Create a new transport channel.
     */
    OpalTransportUDP(
      OpalEndPoint & endpoint,              ///<  Endpoint object
      const PBYTEArray & preReadPacket,     ///<  Packet already read by OpalListenerUDP
      const PMonitoredSocketsPtr & sockets, ///<  Bundle of sockets from OpalListenerUDP
      const PString & iface,                ///<  Local interface to use
      PIPSocket::Address remoteAddress,     ///<  Remote address received PDU on
      WORD remotePort                       ///<  Remote port received PDU on
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
      const PString & iface ///< Interface to finally use
    );
    
    /** Get the interface this transport is bound to.
      */
    virtual PString GetInterface() const;

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

    /**Read a protocol data unit from the transport.
       This will read using the transports mechanism for PDU boundaries, for
       example UDP is a single Read() call, while for TCP there is a TPKT
       header that indicates the size of the PDU.
      */
    virtual BOOL ReadPDU(
      PBYTEArray & packet   ///<  Packet read from transport
    );

    /**Write a packet to the transport.
       This will write using the transports mechanism for PDU boundaries, for
       example UDP is a single Write() call, while for TCP there is a TPKT
       header that indicates the size of the PDU.
      */
    virtual BOOL WritePDU(
      const PBYTEArray & pdu     ///<  Packet to write
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
      WriteConnectCallback function,  ///<  Function for writing data
      void * userData                 ///<  User data to pass to write function
    );
  //@}


  protected:
    /**Get the prefix for this transports protocol type.
      */
    virtual const char * GetProtoPrefix() const;


    OpalManager & manager;
    PBYTEArray preReadPacket;
};


////////////////////////////////////////////////////////////////

class OpalInternalTransport : public PObject
{
    PCLASSINFO(OpalInternalTransport, PObject);
  public:
    virtual PString GetHostName(
      const OpalTransportAddress & address
    ) const;

    virtual BOOL GetIpAndPort(
      const OpalTransportAddress & address,
      PIPSocket::Address & ip,
      WORD & port
    ) const;

    virtual OpalListener * CreateListener(
      const OpalTransportAddress & address,
      OpalEndPoint & endpoint,
      OpalTransportAddress::BindOptions options
    ) const  = 0;

    virtual OpalTransport * CreateTransport(
      const OpalTransportAddress & address,
      OpalEndPoint & endpoint,
      OpalTransportAddress::BindOptions options
    ) const = 0;
};


////////////////////////////////////////////////////////////////

class OpalInternalIPTransport : public OpalInternalTransport
{
    PCLASSINFO(OpalInternalIPTransport, OpalInternalTransport);
  public:
    virtual PString GetHostName(
      const OpalTransportAddress & address
    ) const;
    virtual BOOL GetIpAndPort(
      const OpalTransportAddress & address,
      PIPSocket::Address & ip,
      WORD & port
    ) const;

    static BOOL GetAdjustedIpAndPort(const OpalTransportAddress & address,
                                     OpalEndPoint & endpoint,
                                     OpalTransportAddress::BindOptions option,
                                     PIPSocket::Address & ip,
                                     WORD & port,
                                     BOOL & reuseAddr);
};

template <class ListenerType, class TransportType, unsigned AltTypeOption, class AltTypeClass>
class OpalInternalIPTransportTemplate : public OpalInternalIPTransport
{
  public:
    OpalListener * CreateListener(
      const OpalTransportAddress & address,
      OpalEndPoint & endpoint,
      OpalTransportAddress::BindOptions options
    ) const
    {
      return new ListenerType(endpoint, address, options);
    }

    OpalTransport * CreateTransport(
      const OpalTransportAddress & address,
      OpalEndPoint & endpoint,
      OpalTransportAddress::BindOptions options
    ) const
    {
      PIPSocket::Address ip;
      WORD port;
      BOOL reuseAddr;
      if (GetAdjustedIpAndPort(address, endpoint, options, ip, port, reuseAddr)) {
        if (options == AltTypeOption)
          return new AltTypeClass(endpoint, ip, 0, reuseAddr);
        else
          return new TransportType(endpoint, ip, 0, reuseAddr);
      }
      return NULL;
    }
};

typedef OpalInternalIPTransportTemplate<OpalListenerTCP, OpalTransportTCP, OpalTransportAddress::Datagram, OpalTransportUDP> OpalInternalTCPTransport;
typedef OpalInternalIPTransportTemplate<OpalListenerUDP, OpalTransportUDP, OpalTransportAddress::Streamed, OpalTransportTCP> OpalInternalUDPTransport;

#if P_SSL

class PSSLContext;

class OpalListenerTCPS : public OpalListenerTCP
{
  PCLASSINFO(OpalListenerTCPS, OpalListenerTCP);
  public:
    OpalListenerTCPS(
      OpalEndPoint & endpoint,                 ///<  Endpoint listener is used for
      PIPSocket::Address binding = PIPSocket::GetDefaultIpAny(), ///<  Local interface to listen on
      WORD port = 0,                           ///<  TCP port to listen for connections
      BOOL exclusive = TRUE
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
      PSSLContext * sslContext;
};

class OpalTransportTCPS : public OpalTransportTCP
{
  PCLASSINFO(OpalTransportTCPS, OpalTransportTCP);
    public:
      OpalTransportTCPS(
        OpalEndPoint & endpoint,    ///<  Endpoint object
        PIPSocket::Address binding = PIPSocket::GetDefaultIpAny(), ///<  Local interface to use
        WORD port = 0,              ///<  Local port to bind to
        BOOL reuseAddr = FALSE      ///<  Flag for binding to already bound interface
      );
      OpalTransportTCPS(
        OpalEndPoint & endpoint,    ///<  Endpoint object
        PTCPSocket * socket         ///<  Socket to use
      );

      /// Destroy the TCPS channel
      ~OpalTransportTCPS();

      BOOL IsCompatibleTransport(const OpalTransportAddress & address) const;
      BOOL Connect();
      BOOL OnOpen();
      const char * GetProtoPrefix() const;

    protected:
      PSSLContext * sslContext;
};

typedef OpalInternalIPTransportTemplate<OpalListenerTCPS, OpalTransportTCPS, OpalTransportAddress::Datagram, OpalTransportUDP> OpalInternalTCPSTransport;


#endif // P_SSL


#endif  // __OPAL_TRANSPORT_H


// End of File ///////////////////////////////////////////////////////////////
