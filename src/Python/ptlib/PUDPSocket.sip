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
#include <ptlib/udpsock.h>
%End

class PIPDatagramSocket : PIPSocket /Abstract/
{
  protected:
    /**Create a TCP/IP protocol socket channel. If a remote machine address or
       a "listening" socket is specified then the channel is also opened.
     */
    PIPDatagramSocket();
};


/**
   A socket channel that uses the UDP transport on the Internet Protocol.
 */
class PUDPSocket : PIPDatagramSocket
{
  public:
  /**@name Construction */
  //@{
    /** Create a UDP socket. If a remote machine address or
       a "listening" socket is specified then the channel is also opened.
     */
    PUDPSocket(
      WORD port = 0,             ///< Port number to use for the connection.
	  int iAddressFamily = AF_INET ///< Family
    );
    PUDPSocket(
       PQoS * qos,              ///< Pointer to a QOS structure for the connection
      WORD port = 0,            ///< Port number to use for the connection.
	  int iAddressFamily = AF_INET ///< Family
    );
    PUDPSocket(
      const PString & service,   ///< Service name to use for the connection.
      PQoS * qos = NULL,         ///< Pointer to a QOS structure for the connection
	  int iAddressFamily = AF_INET ///< Family
    );
    PUDPSocket(
      const PString & address,  ///< Address of remote machine to connect to.
      WORD port                 ///< Port number to use for the connection.
    );
    PUDPSocket(
      const PString & address,  ///< Address of remote machine to connect to.
      const PString & service   ///< Service name to use for the connection.
    );
  //@}

  /**@name Overrides from class PSocket */
  //@{
    /** Override of PChannel functions to allow connectionless reads
     */
    // PBoolean Read(
      // void * buf,   ///< Pointer to a block of memory to read.
      // PINDEX len    ///< Number of bytes to read.
    // );

    // /** Override of PChannel functions to allow connectionless writes
     // */
    // PBoolean Write(
      // const void * buf, ///< Pointer to a block of memory to write.
      // PINDEX len        ///< Number of bytes to write.
    // );

    /** Override of PSocket functions to allow connectionless writes
     */
    PBoolean Connect(
      const PString & address   ///< Address of remote machine to connect to.
    );
  //@}

  /**@name New functions for class */
  //@{
    /** Set the address to use for connectionless Write() or Windows QoS
     */
    void SetSendAddress(
      const Address & address,    ///< IP address to send packets.
      WORD port                   ///< Port to send packets.
    );

    /** Get the address to use for connectionless Write().
     */
    void GetSendAddress(
      Address & address,    ///< IP address to send packets.
      WORD & port           ///< Port to send packets.
    );


    /** Change the QOS spec for the socket and try to apply the changes
     */
    virtual PBoolean ModifyQoSSpec(
      PQoS * qos            ///< QoS specification to use
    );


    /** Get the QOS object for the socket.
    */
    virtual PQoS & GetQoSSpec();

    /** Get the address of the sender in the last connectionless Read().
        Note that thsi only applies to the Read() and not the ReadFrom()
        function.
     */
    void GetLastReceiveAddress(
      Address & address,    ///< IP address to send packets.
      WORD & port           ///< Port to send packets.
    );

    /** CallBack to check if the detected address of the connectionless Read()
		is an alternate address. Use this to switch the target to send and
		receive the connectionless read/write.
     */
	virtual PBoolean IsAlternateAddress(
		const Address & address,    ///< Detected IP Address.
		WORD port					///< Detected Port.
		);

    /** Check to See if the socket will support QoS on the given local Address
     */
    static PBoolean SupportQoS(const PIPSocket::Address & address);

    /** Manually Enable GQoS Support
     */
    static void EnableGQoS();
  //@}

  protected:
    // Open an IPv4 or IPv6 socket
    virtual PBoolean OpenSocket(
      int ipAdressFamily
    );

    // Create a QOS-enabled socket
    virtual PBoolean OpenSocketGQOS(int af, int type, int proto);

    // Modify the QOS settings
    virtual PBoolean ApplyQoS();

    virtual const char * GetProtocolName() const;

};