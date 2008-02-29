/*
 * transport.cxx
 *
 * Opal transports handler
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
 * $Log: transports.cxx,v $
 * Revision 1.2059  2006/02/18 19:00:38  dsandras
 * Added PTRACE statements.
 *
 * Revision 2.57  2005/11/29 11:49:35  dsandras
 * socket is autodeleted, even in case of failure.
 *
 * Revision 2.56  2005/10/21 17:57:00  dsandras
 * Applied patch from Hannes Friederich <hannesf AATT ee.ethz.ch> to fix GK
 * registration issues when there are multiple interfaces. Thanks!
 *
 * Revision 2.55  2005/10/09 15:18:12  dsandras
 * Only add socket to the connectSockets when it is open.
 *
 * Revision 2.54  2005/10/09 15:12:38  dsandras
 * Moved some code around.
 *
 * Revision 2.53  2005/09/22 18:16:29  dsandras
 * Definitely fixed the previous problem.
 *
 * Revision 2.52  2005/09/22 17:07:34  dsandras
 * Fixed bug leading to a crash when registering to a gatekeeper.
 *
 * Revision 2.51  2005/09/19 20:49:59  dsandras
 * Following the API, a "reusable" address ends with '+', not something different than '+'.
 * When a socket is created, reuse the "reusable" flag from the original socket.
 *
 * Revision 2.50  2005/09/18 20:32:57  dsandras
 * New fix for the same problem that works.
 *
 * Revision 2.49  2005/09/18 18:41:01  dsandras
 * Reverted previous broken patch.
 *
 * Revision 2.48  2005/09/17 17:36:21  dsandras
 * Close the old channel before creating the new socket.
 *
 * Revision 2.47  2005/07/16 17:16:17  dsandras
 * Moved code upward so that the local source port range is always taken into account when creating a transport.
 *
 * Revision 2.46  2005/06/08 17:35:10  dsandras
 * Fixed sockets leak thanks to Ted Szoczei. Thanks!
 *
 * Revision 2.45  2005/06/02 18:39:03  dsandras
 * Committed fix for Gatekeeper registration thanks to Hannes Friederich <hannesf   __@__ ee.ethz.ch>.
 *
 * Revision 2.44  2005/05/23 20:14:48  dsandras
 * Added STUN socket to the list of connected sockets.
 *
 * Revision 2.43  2005/04/30 20:59:55  dsandras
 * Consider we are already connected only if the connectSockets array is not empty.
 *
 * Revision 2.42  2005/04/20 06:18:35  csoutheren
 * Patch 1182998. Fix for using GK through NAT, and fixed Connect to be idempotent
 * Thanks to Hannes Friederich
 *
 * Revision 2.41  2005/04/20 06:15:25  csoutheren
 * Patch 1181901. Fix race condition in OpalTransportUDP
 * Thanks to Ted Szoczei
 *
 * Revision 2.40  2005/01/16 23:08:33  csoutheren
 * Fixed problem with IPv6 INADDR_ANY
 * Fixed problem when transport thread self terminates
 *
 * Revision 2.39  2004/12/12 13:37:02  dsandras
 * Made the transport type comparison insensitive. Required for interoperation with some IP Phones.
 *
 * Revision 2.38  2004/08/14 07:56:43  rjongbloed
 * Major revision to utilise the PSafeCollection classes for the connections and calls.
 *
 * Revision 2.37  2004/05/09 13:12:38  rjongbloed
 * Fixed issues with non fast start and non-tunnelled connections
 *
 * Revision 2.36  2004/04/27 07:23:40  rjongbloed
 * Fixed uninitialised variable getting ip without port
 *
 * Revision 2.35  2004/04/27 04:40:17  rjongbloed
 * Changed UDP listener IsOpen to indicae open only if all sockets on each
 *   interface are open.
 *
 * Revision 2.34  2004/04/07 08:21:10  rjongbloed
 * Changes for new RTTI system.
 *
 * Revision 2.33  2004/03/29 11:04:19  rjongbloed
 * Fixed shut down of OpalTransportUDP when still in "connect" phase.
 *
 * Revision 2.32  2004/03/22 11:39:44  rjongbloed
 * Fixed problems with correctly terminating the OpalTransportUDP that is generated from
 *   an OpalListenerUDP, this should not close the socket or stop the thread.
 *
 * Revision 2.31  2004/03/16 12:01:37  rjongbloed
 * Temporary fix for closing UDP transport
 *
 * Revision 2.30  2004/03/13 06:30:03  rjongbloed
 * Changed parameter in UDP write function to void * from PObject *.
 *
 * Revision 2.29  2004/02/24 11:37:02  rjongbloed
 * More work on NAT support, manual external address translation and STUN
 *
 * Revision 2.28  2004/02/19 10:47:06  rjongbloed
 * Merged OpenH323 version 1.13.1 changes.
 *
 * Revision 2.27  2003/01/07 06:01:07  robertj
 * Fixed MSVC warnings.
 *
 * Revision 1.135  2003/12/03 06:57:11  csoutheren
 * Protected against dwarf Q.931 PDUs
 *
 * Revision 1.134  2003/04/10 09:45:34  robertj
 * Added associated transport to new GetInterfaceAddresses() function so
 *   interfaces can be ordered according to active transport links. Improves
 *   interoperability.
 * Replaced old listener GetTransportPDU() with GetInterfaceAddresses()
 *   and H323SetTransportAddresses() functions.
 *
 * Revision 1.133  2003/04/10 00:58:54  craigs
 * Added functions to access to lists of interfaces
 *
 * Revision 1.132  2003/03/26 06:14:31  robertj
 * More IPv6 support (INADDR_ANY handling), thanks Sébastien Josset
 *
 * Revision 1.131  2003/03/21 05:24:54  robertj
 * Added setting of remote port in UDP transport constructor.
 *
 * Revision 1.130  2003/03/20 01:51:12  robertj
 * More abstraction of H.225 RAS and H.501 protocols transaction handling.
 *
 * Revision 1.129  2003/03/11 23:15:23  robertj
 * Fixed possible double delete of socket (crash) on garbage input.
 *
 * Revision 1.128  2003/02/06 04:31:02  robertj
 * Added more support for adding things to H323TransportAddressArrays
 *
 * Revision 1.127  2003/02/05 01:57:18  robertj
 * Fixed STUN usage on gatekeeper discovery.
 *
 * Revision 1.126  2003/02/04 07:06:42  robertj
 * Added STUN support.
 *
 * Revision 1.125  2003/01/23 02:36:32  robertj
 * Increased (and made configurable) timeout for H.245 channel TCP connection.
 *
 * Revision 1.124  2002/12/23 22:46:06  robertj
 * Changed gatekeeper discovery so an GRJ does not indicate "discovered".
 *
 * Revision 1.123  2002/11/21 06:40:00  robertj
 * Changed promiscuous mode to be three way. Fixes race condition in gkserver
 *   which can cause crashes or more PDUs to be sent to the wrong place.
 *
 * Revision 1.122  2002/11/12 03:14:18  robertj
 * Fixed gatekeeper discovery so does IP address translation correctly for
 *   hosts inside the firewall.
 *
 * Revision 1.121  2002/11/10 08:10:44  robertj
 * Moved constants for "well known" ports to better place (OPAL change).
 *
 * Revision 1.120  2002/11/05 00:31:48  robertj
 * Prevented a failure to start separate H.245 channel stopping the call until
 *   after a CONNECT is received and have no audio. At that point no H.245
 *   is a useless call and we disconnect.
 *
 * Revision 1.119  2002/11/01 03:38:18  robertj
 * More IPv6 fixes, thanks Sébastien Josset.
 *
 * Revision 1.118  2002/10/29 08:30:32  robertj
 * Fixed problem with simultaneous startH245 condition possibly shutting down
 *   the call under some circumstances.
 *
 * Revision 1.117  2002/10/16 06:28:20  robertj
 * More IPv6 support changes, especially in unambiguising v6 addresses colons
 *   from the port fields colon, thanks Sebastien Josset
 *
 * Revision 1.116  2002/10/08 23:34:30  robertj
 * Fixed ip v6 usage on H.245 pdu setting.
 *
 * Revision 1.115  2002/10/08 13:08:21  robertj
 * Changed for IPv6 support, thanks Sébastien Josset.
 *
 * Revision 1.114  2002/08/05 10:03:48  robertj
 * Cosmetic changes to normalise the usage of pragma interface/implementation.
 *
 * Revision 1.113  2002/08/05 05:17:41  robertj
 * Fairly major modifications to support different authentication credentials
 *   in ARQ to the logged in ones on RRQ. For both client and server.
 * Various other H.235 authentication bugs and anomalies fixed on the way.
 *
 * Revision 1.112  2002/07/22 09:40:19  robertj
 * Added ability to automatically convert string arrays, lists sorted lists
 *   directly to H323TransportAddressArray.
 *
 * Revision 1.111  2002/07/18 08:25:47  robertj
 * Fixed problem in decoding host when '+' was used without port in address.
 *
 * Revision 1.110  2002/07/10 01:23:33  robertj
 * Added extra debugging output
 *
 * Revision 1.109  2002/07/02 10:02:32  robertj
 * Added H323TransportAddress::GetIpAddress() so don't have to provide port
 *   when you don't need it as in GetIpAndPort(),.
 *
 * Revision 1.108  2002/06/28 03:34:29  robertj
 * Fixed issues with address translation on gatekeeper RAS channel.
 *
 * Revision 1.107  2002/06/24 07:35:23  robertj
 * Fixed ability to do gk discovery on localhost, thanks Artis Kugevics
 *
 * Revision 1.106  2002/06/12 03:52:27  robertj
 * Added function to compare two transport addresses in a more intelligent
 *   way that strict string comparison. Takes into account wildcarding.
 *
 * Revision 1.105  2002/05/28 06:38:08  robertj
 * Split UDP (for RAS) from RTP port bases.
 * Added current port variable so cycles around the port range specified which
 *   fixes some wierd problems on some platforms, thanks Federico Pinna
 *
 * Revision 1.104  2002/05/22 07:39:59  robertj
 * Fixed double increment of port number when making outgoing TCP connection.
 *
 * Revision 1.103  2002/04/18 00:18:58  robertj
 * Increased timeout for thread termination assert, on heavily loaded machines it can
 *   take more than one second to complete.
 *
 * Revision 1.102  2002/04/17 05:36:38  robertj
 * Fixed problems with using pre-bound inferface/port in gk discovery.
 *
 * Revision 1.101  2002/04/12 04:51:28  robertj
 * Fixed small possibility crashes if open and close transport at same time.
 *
 * Revision 1.100  2002/03/08 01:22:30  robertj
 * Fixed possible use of IsSuspended() on terminated thread causing assert.
 *
 * Revision 1.99  2002/03/05 04:49:41  robertj
 * Fixed leak of thread (and file handles) if get incoming connection aborted
 *   very early (before receiving a setup PDU), thanks Hans Bjurström
 *
 * Revision 1.98  2002/02/28 04:35:43  robertj
 * Added trace output of the socket handle number when have new connection.
 *
 * Revision 1.97  2002/02/28 00:57:03  craigs
 * Changed SetWriteTimeout to SetReadTimeout in connect, as Craig got it wrong!
 *
 * Revision 1.96  2002/02/25 10:55:33  robertj
 * Added ability to speficy dynamically allocated port in transport address.
 *
 * Revision 1.95  2002/02/14 03:36:14  craigs
 * Added default 10sec timeout on connect to IP addresses
 * This prevents indefinite hangs when connecting to IP addresses
 * that don't exist
 *
 * Revision 1.94  2002/02/05 23:29:09  robertj
 * Changed default for H.323 listener to reuse addresses.
 *
 * Revision 1.93  2002/02/01 01:48:18  robertj
 * Fixed ability to shut down a Listener, if it had never been started.
 *
 * Revision 1.92  2002/01/02 06:06:43  craigs
 * Made T.38 UDP socket obey rules
 *
 * Revision 1.91  2001/12/22 01:48:40  robertj
 * Added ability to use local and remote port from transport channel as well
 *   as explicit port in H.245 address PDU setting routine.
 * Added PrintOn() to listener and transport for tracing purposes.
 *
 * Revision 1.90  2001/12/15 07:12:22  robertj
 * Added optimisation so if discovering a static gk on same machine as ep is
 *   running on then uses that specific interface preventing multiple GRQs.
 *
 * Revision 1.89  2001/10/11 07:16:49  robertj
 * Removed port check for gk's that change sockets in mid-stream.
 *
 * Revision 1.88  2001/10/09 12:41:20  robertj
 * Set promiscuous flag back to FALSE after gatkeeper discovery.
 *
 * Revision 1.87  2001/09/10 03:06:29  robertj
 * Major change to fix problem with error codes being corrupted in a
 *   PChannel when have simultaneous reads and writes in threads.
 *
 * Revision 1.86  2001/08/10 11:03:52  robertj
 * Major changes to H.235 support in RAS to support server.
 *
 * Revision 1.85  2001/08/07 02:57:52  robertj
 * Improved tracing on closing transport.
 *
 * Revision 1.84  2001/08/06 03:08:57  robertj
 * Fission of h323.h to h323ep.h & h323con.h, h323.h now just includes files.
 *
 * Revision 1.83  2001/07/17 04:44:32  robertj
 * Partial implementation of T.120 and T.38 logical channels.
 *
 * Revision 1.82  2001/07/06 02:31:15  robertj
 * Made sure a release complete is sent if no connection is created.
 *
 * Revision 1.81  2001/07/04 09:02:07  robertj
 * Added more tracing
 *
 * Revision 1.80  2001/06/25 05:50:22  robertj
 * Improved error logging on TCP listener.
 *
 * Revision 1.79  2001/06/25 02:28:34  robertj
 * Allowed TCP listener socket to be opened in non-exclusive mode
 *   (ie SO_REUSEADDR) to avoid daemon restart problems.
 * Added trailing '+' on H323TransportAddress string to invoke above.
 *
 * Revision 1.78  2001/06/22 02:47:12  robertj
 * Took one too many lines out in previous fix!
 *
 * Revision 1.77  2001/06/22 02:40:27  robertj
 * Fixed discovery so uses new promiscuous mode.
 * Also used the RAS GRQ address of server isntead of UDP return address
 *   for address of gatekeeper for future packets.
 *
 * Revision 1.76  2001/06/22 01:54:47  robertj
 * Removed initialisation of localAddress to hosts IP address, does not
 *   work on multi-homed hosts.
 *
 * Revision 1.75  2001/06/22 00:14:46  robertj
 * Added ConnectTo() function to conencto specific address.
 * Added promiscuous mode for UDP channel.
 *
 * Revision 1.74  2001/06/14 23:18:06  robertj
 * Change to allow for CreateConnection() to return NULL to abort call.
 *
 * Revision 1.73  2001/06/14 04:23:32  robertj
 * Changed incoming call to pass setup pdu to endpoint so it can create
 *   different connection subclasses depending on the pdu eg its alias
 *
 * Revision 1.72  2001/06/06 00:29:54  robertj
 * Added trace for when doing TCP connect.
 *
 * Revision 1.71  2001/06/02 01:35:32  robertj
 * Added thread names.
 *
 * Revision 1.70  2001/05/31 07:16:52  craigs
 * Fixed remote address initialisation for incoming H245 channels
 *
 * Revision 1.69  2001/05/17 06:37:04  robertj
 * Added multicast gatekeeper discovery support.
 *
 * Revision 1.68  2001/04/13 07:44:51  robertj
 * Moved starting connection trace message to be on both Connect() and Accept()
 *
 * Revision 1.67  2001/04/10 01:21:02  robertj
 * Added some more error messages into trace log.
 *
 * Revision 1.66  2001/04/09 08:44:19  robertj
 * Added ability to get transport address for a listener.
 * Added '*' to indicate INADDR_ANY ip address.
 *
 * Revision 1.65  2001/03/06 05:03:00  robertj
 * Changed H.245 channel start failure so does not abort call if there were
 *   some fast started media streams opened. Just lose user indications.
 *
 * Revision 1.64  2001/03/05 04:28:50  robertj
 * Added net mask to interface info returned by GetInterfaceTable()
 *
 * Revision 1.63  2001/02/09 05:13:56  craigs
 * Added pragma implementation to (hopefully) reduce the executable image size
 * under Linux
 *
 * Revision 1.62  2001/02/08 22:29:39  robertj
 * Fixed failure to reset fill character in trace log when output interface list.
 *
 * Revision 1.61  2001/01/29 06:43:32  robertj
 * Added printing of entry of interface table.
 *
 * Revision 2.26  2003/01/07 04:39:53  robertj
 * Updated to OpenH323 v1.11.2
 *
 * Revision 2.25  2002/11/10 11:33:20  robertj
 * Updated to OpenH323 v1.10.3
 *
 * Revision 2.24  2002/10/09 04:26:57  robertj
 * Fixed ability to call CloseWait() multiple times, thanks Ted Szoczei
 *
 * Revision 2.23  2002/09/26 01:21:16  robertj
 * Fixed error in trace output when get illegal transport address.
 *
 * Revision 2.22  2002/09/12 06:57:56  robertj
 * Removed protocol prefix strings as static members as has problems with
 *   use in DLL environment.
 * Added support for the + character in OpalTransportAddress decoding
 *  to indicate exclusive listener.
 *
 * Revision 2.21  2002/09/06 02:41:18  robertj
 * Added ability to specify stream or datagram (TCP or UDP) transport is to
 * be created from a transport address regardless of the addresses mode.
 *
 * Revision 2.20  2002/07/04 07:41:47  robertj
 * Fixed memory/thread leak of transports.
 *
 * Revision 2.19  2002/07/01 08:55:07  robertj
 * Changed TCp/UDP port allocation to use new thread safe functions.
 *
 * Revision 2.18  2002/07/01 08:43:44  robertj
 * Fixed assert on every SIP packet.
 *
 * Revision 2.17  2002/07/01 04:56:33  robertj
 * Updated to OpenH323 v1.9.1
 *
 * Revision 2.16  2002/06/16 23:07:19  robertj
 * Fixed several memory leaks, thanks Ted Szoczei
 * Fixed error opening UDP listener for broadcast packets under Win32.
 *   Is not needed as it is under windows, thanks Ted Szoczei
 *
 * Revision 2.15  2002/04/16 07:52:51  robertj
 * Change to allow SetRemoteAddress before UDP is connected.
 *
 * Revision 2.14  2002/04/10 03:12:35  robertj
 * Fixed SetLocalAddress to return FALSE if did not set the address to a
 *   different address to the current one. Altered UDP version to cope.
 *
 * Revision 2.13  2002/04/09 04:44:36  robertj
 * Fixed bug where crahses if close channel while in UDP connect mode.
 *
 * Revision 2.12  2002/04/09 00:22:16  robertj
 * Added ability to set the local address on a transport, under some circumstances.
 *
 * Revision 2.11  2002/03/27 05:37:39  robertj
 * Fixed removal of writeChannel after wrinting to UDP transport in connect mode.
 *
 * Revision 2.10  2002/03/15 00:20:54  robertj
 * Fixed bug when closing UDP transport when in "connect" mode.
 *
 * Revision 2.9  2002/02/06 06:07:10  robertj
 * Fixed shutting down UDP listener thread
 *
 * Revision 2.8  2002/01/14 00:19:33  craigs
 * Fixed problems with remote address used instead of local address
 *
 * Revision 2.7  2001/12/07 08:55:32  robertj
 * Used UDP port base when creating UDP transport.
 *
 * Revision 2.6  2001/11/14 06:28:20  robertj
 * Added missing break when ending UDP connect phase.
 *
 * Revision 2.5  2001/11/13 04:29:48  robertj
 * Changed OpalTransportAddress CreateTransport and CreateListsner functions
 *   to have extra parameter to control local binding of sockets.
 *
 * Revision 2.4  2001/11/12 05:31:36  robertj
 * Changed CreateTransport() on an OpalTransportAddress to bind to local address.
 * Added OpalTransportAddress::GetIpAddress when don't need port number.
 *
 * Revision 2.3  2001/11/09 05:49:47  robertj
 * Abstracted UDP connection algorithm
 *
 * Revision 2.2  2001/11/06 05:40:13  robertj
 * Added OpalListenerUDP class to do listener semantics on a UDP socket.
 *
 * Revision 2.1  2001/10/03 05:53:25  robertj
 * Update to new PTLib channel error system.
 *
 * Revision 2.0  2001/07/27 15:48:25  robertj
 * Conversion of OpenH323 to Open Phone Abstraction Library (OPAL)
 *
 */

#include <ptlib.h>

#ifdef __GNUC__
#pragma implementation "transports.h"
#endif

#include <opal/transports.h>

#include <opal/manager.h>
#include <opal/endpoint.h>
#include <opal/call.h>

#include <ptclib/pstun.h>


static const char IpPrefix[] = "ip$"; // For backward compatibility with OpenH323
static const char TcpPrefix[] = "tcp$";
static const char UdpPrefix[] = "udp$";

#include <ptclib/pstun.h>


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
};


////////////////////////////////////////////////////////////////

static class OpalInternalTCPTransport : public OpalInternalIPTransport
{
    PCLASSINFO(OpalInternalTCPTransport, OpalInternalIPTransport);
  public:
    virtual OpalListener * CreateListener(
      const OpalTransportAddress & address,
      OpalEndPoint & endpoint,
      OpalTransportAddress::BindOptions options
    ) const;
    virtual OpalTransport * CreateTransport(
      const OpalTransportAddress & address,
      OpalEndPoint & endpoint,
      OpalTransportAddress::BindOptions options
    ) const;
} internalTCPTransport;


////////////////////////////////////////////////////////////////

static class OpalInternalUDPTransport : public OpalInternalIPTransport
{
    PCLASSINFO(OpalInternalUDPTransport, OpalInternalIPTransport);
  public:
    virtual OpalListener * CreateListener(
      const OpalTransportAddress & address,
      OpalEndPoint & endpoint,
      OpalTransportAddress::BindOptions options
    ) const;
    virtual OpalTransport * CreateTransport(
      const OpalTransportAddress & address,
      OpalEndPoint & endpoint,
      OpalTransportAddress::BindOptions options
    ) const;
} internalUDPTransport;


/////////////////////////////////////////////////////////////////

#define new PNEW


/////////////////////////////////////////////////////////////////

OpalTransportAddress::OpalTransportAddress()
{
  transport = NULL;
}


OpalTransportAddress::OpalTransportAddress(const char * cstr,
                                           WORD port,
                                           const char * proto)
  : PString(cstr)
{
  SetInternalTransport(port, proto);
}


OpalTransportAddress::OpalTransportAddress(const PString & str,
                                           WORD port,
                                           const char * proto)
  : PString(str)
{
  SetInternalTransport(port, proto);
}


OpalTransportAddress::OpalTransportAddress(const PIPSocket::Address & addr, WORD port, const char * proto)
  : PString(addr.AsString())
{
  SetInternalTransport(port, proto);
}


PString OpalTransportAddress::GetHostName() const
{
  if (transport == NULL)
    return PString();

  return transport->GetHostName(*this);
}
  

BOOL OpalTransportAddress::IsEquivalent(const OpalTransportAddress & address)
{
  if (*this == address)
    return TRUE;

  if (IsEmpty() || address.IsEmpty())
    return FALSE;

  PIPSocket::Address ip1, ip2;
  WORD port1 = 65535, port2 = 65535;
  return GetIpAndPort(ip1, port1) &&
         address.GetIpAndPort(ip2, port2) &&
         (ip1.IsAny() || ip2.IsAny() || (ip1 *= ip2)) &&
         (port1 == 65535 || port2 == 65535 || port1 == port2);
}


BOOL OpalTransportAddress::GetIpAddress(PIPSocket::Address & ip) const
{
  if (transport == NULL)
    return FALSE;

  WORD dummy = 65535;
  return transport->GetIpAndPort(*this, ip, dummy);
}


BOOL OpalTransportAddress::GetIpAndPort(PIPSocket::Address & ip, WORD & port) const
{
  if (transport == NULL)
    return FALSE;

  return transport->GetIpAndPort(*this, ip, port);
}


OpalListener * OpalTransportAddress::CreateListener(OpalEndPoint & endpoint,
                                                    BindOptions option) const
{
  if (transport == NULL)
    return NULL;

  return transport->CreateListener(*this, endpoint, option);
}


OpalTransport * OpalTransportAddress::CreateTransport(OpalEndPoint & endpoint,
                                                      BindOptions option) const
{
  if (transport == NULL)
    return NULL;

  return transport->CreateTransport(*this, endpoint, option);
}


void OpalTransportAddress::SetInternalTransport(WORD port, const char * proto)
{
  transport = NULL;
  
  if (IsEmpty())
    return;

  PINDEX dollar = Find('$');
  if (dollar == P_MAX_INDEX) {
    PString prefix(proto == NULL ? TcpPrefix : proto);
    if (prefix.Find('$') == P_MAX_INDEX)
      prefix += '$';

    Splice(prefix, 0);
    dollar = prefix.GetLength()-1;
  }

  PCaselessString type = Left(dollar+1);

  // look for known transport types, note that "ip$" == "tcp$"
  if ((type == IpPrefix) || (type == TcpPrefix))
    transport = &internalTCPTransport;
  else if (type == UdpPrefix)
    transport = &internalUDPTransport;
  else
    return;

  if (port != 0 && Find(':', dollar) == P_MAX_INDEX)
    sprintf(":%u", port);
}


/////////////////////////////////////////////////////////////////

void OpalTransportAddressArray::AppendString(const char * str)
{
  AppendAddress(OpalTransportAddress(str));
}


void OpalTransportAddressArray::AppendString(const PString & str)
{
  AppendAddress(OpalTransportAddress(str));
}


void OpalTransportAddressArray::AppendAddress(const OpalTransportAddress & addr)
{
  if (!addr)
    Append(new OpalTransportAddress(addr));
}


void OpalTransportAddressArray::AppendStringCollection(const PCollection & coll)
{
  for (PINDEX i = 0; i < coll.GetSize(); i++) {
    PObject * obj = coll.GetAt(i);
    if (obj != NULL && PIsDescendant(obj, PString))
      AppendAddress(OpalTransportAddress(*(PString *)obj));
  }
}


/////////////////////////////////////////////////////////////////

PString OpalInternalTransport::GetHostName(const OpalTransportAddress & address) const
{
  // skip transport identifier
  PINDEX pos = address.Find('$');
  if (pos == P_MAX_INDEX)
    return PString();

  return address.Mid(pos+1);
}


BOOL OpalInternalTransport::GetIpAndPort(const OpalTransportAddress &,
                                         PIPSocket::Address &,
                                         WORD &) const
{
  return FALSE;
}


//////////////////////////////////////////////////////////////////////////

static BOOL SplitAddress(const PString & addr, PString & host, PString & service)
{
  // skip transport identifier
  PINDEX dollar = addr.Find('$');
  if (dollar == P_MAX_INDEX)
    return FALSE;
  
  PINDEX lastChar = addr.GetLength()-1;
  if (addr[lastChar] == '+')
    lastChar--;

  PINDEX bracket = addr.FindLast(']');
  if (bracket == P_MAX_INDEX)
    bracket = 0;

  PINDEX colon = addr.Find(':', bracket);
  if (colon == P_MAX_INDEX)
    host = addr(dollar+1, lastChar);
  else {
    host = addr(dollar+1, colon-1);
    service = addr(colon+1, lastChar);
  }

  return TRUE;
}


PString OpalInternalIPTransport::GetHostName(const OpalTransportAddress & address) const
{
  PString host, service;
  if (!SplitAddress(address, host, service))
    return address;

  PIPSocket::Address ip;
  if (PIPSocket::GetHostAddress(host, ip))
    return ip.AsString();

  return host;
}


BOOL OpalInternalIPTransport::GetIpAndPort(const OpalTransportAddress & address,
                                           PIPSocket::Address & ip,
                                           WORD & port) const
{
  PString host, service;
  if (!SplitAddress(address, host, service))
    return FALSE;

  if (host.IsEmpty()) {
    PTRACE(2, "Opal\tIllegal IP transport address: \"" << address << '"');
    return FALSE;
  }

  if (service == "*")
    port = 0;
  else {
    if (!service) {
      PString proto = address.Left(address.Find('$'));
      if (proto *= "ip")
        proto = "tcp";
      port = PIPSocket::GetPortByService(proto, service);
    }
    if (port == 0) {
      PTRACE(2, "Opal\tIllegal IP transport port/service: \"" << address << '"');
      return FALSE;
    }
  }

  if (host == "*") {
    ip = PIPSocket::GetDefaultIpAny();
    return TRUE;
  }

  if (PIPSocket::GetHostAddress(host, ip))
    return TRUE;

  PTRACE(1, "Opal\tCould not find host : \"" << host << '"');
  return FALSE;
}


//////////////////////////////////////////////////////////////////////////

static BOOL GetAdjustedIpAndPort(const OpalTransportAddress & address,
                                 OpalEndPoint & endpoint,
                                 OpalTransportAddress::BindOptions option,
                                 PIPSocket::Address & ip,
                                 WORD & port,
                                 BOOL & reuseAddr)
{
  reuseAddr = address[address.GetLength()-1] == '+';

  switch (option) {
    case OpalTransportAddress::NoBinding :
      ip = PIPSocket::GetDefaultIpAny();
      port = 0;
      return TRUE;

    case OpalTransportAddress::HostOnly :
      port = 0;
      return address.GetIpAddress(ip);

    default :
      port = endpoint.GetDefaultSignalPort();
      return address.GetIpAndPort(ip, port);
  }
}


OpalListener * OpalInternalTCPTransport::CreateListener(const OpalTransportAddress & address,
                                                        OpalEndPoint & endpoint,
                                                        OpalTransportAddress::BindOptions option) const
{
  PIPSocket::Address ip;
  WORD port;
  BOOL reuseAddr;
  if (GetAdjustedIpAndPort(address, endpoint, option, ip, port, reuseAddr))
    return new OpalListenerTCP(endpoint, ip, port, reuseAddr);

  return NULL;
}


OpalTransport * OpalInternalTCPTransport::CreateTransport(const OpalTransportAddress & address,
                                                          OpalEndPoint & endpoint,
                                                          OpalTransportAddress::BindOptions option) const
{
  PIPSocket::Address ip;
  WORD port;
  BOOL reuseAddr;
  if (GetAdjustedIpAndPort(address, endpoint, option, ip, port, reuseAddr)) {
    if (option == OpalTransportAddress::Datagram)
      return new OpalTransportUDP(endpoint, ip, 0, reuseAddr);
    return new OpalTransportTCP(endpoint, ip, port, reuseAddr);
  }

  return NULL;
}


//////////////////////////////////////////////////////////////////////////

OpalListener * OpalInternalUDPTransport::CreateListener(const OpalTransportAddress & address,
                                                        OpalEndPoint & endpoint,
                                                        OpalTransportAddress::BindOptions option) const
{
  PIPSocket::Address ip;
  WORD port;
  BOOL reuseAddr;
  if (GetAdjustedIpAndPort(address, endpoint, option, ip, port, reuseAddr))
    return new OpalListenerUDP(endpoint, ip, port, reuseAddr);

  return NULL;
}


OpalTransport * OpalInternalUDPTransport::CreateTransport(const OpalTransportAddress & address,
                                                          OpalEndPoint & endpoint,
                                                          OpalTransportAddress::BindOptions option) const
{
  PIPSocket::Address ip;
  WORD port;
  BOOL reuseAddr;
  if (GetAdjustedIpAndPort(address, endpoint, option, ip, port, reuseAddr)) {
    if (option == OpalTransportAddress::Streamed)
      return new OpalTransportTCP(endpoint, ip, 0, reuseAddr);
    return new OpalTransportUDP(endpoint, ip, port, reuseAddr);
  }

  return NULL;
}


//////////////////////////////////////////////////////////////////////////

OpalListener::OpalListener(OpalEndPoint & ep)
  : endpoint(ep)
{
  thread = NULL;
  singleThread = FALSE;
}


void OpalListener::PrintOn(ostream & strm) const
{
  strm << GetLocalAddress();
}


void OpalListener::CloseWait()
{
  PTRACE(3, "Listen\tStopping listening thread on " << GetLocalAddress());
  Close();

  PAssert(PThread::Current() != thread, PLogicError);
  if (thread != NULL) {
    PAssert(thread->WaitForTermination(10000), "Listener thread did not terminate");
    delete thread;
    thread = NULL;
  }
}


void OpalListener::ListenForConnections(PThread & thread, INT)
{
  PTRACE(3, "Listen\tStarted listening thread on " << GetLocalAddress());
  PAssert(!acceptHandler.IsNULL(), PLogicError);

  while (IsOpen()) {
    OpalTransport * transport = Accept(PMaxTimeInterval);
    if (transport == NULL)
      acceptHandler(*this, 0);
    else {
      if (singleThread) {
        transport->AttachThread(&thread);
        acceptHandler(*this, (INT)transport);
      }
      else {
        transport->AttachThread(PThread::Create(acceptHandler,
                                                (INT)transport,
                                                PThread::NoAutoDeleteThread,
                                                PThread::NormalPriority,
                                                "Opal Answer:%x"));
      }
      // Note: acceptHandler is responsible for deletion of the transport
    }
  }
}


BOOL OpalListener::StartThread(const PNotifier & theAcceptHandler, BOOL isSingleThread)
{
  acceptHandler = theAcceptHandler;
  singleThread = isSingleThread;

  thread = PThread::Create(PCREATE_NOTIFIER(ListenForConnections), 0,
                           PThread::NoAutoDeleteThread,
                           PThread::NormalPriority,
                           "Opal Listener:%x");

  return thread != NULL;
}


//////////////////////////////////////////////////////////////////////////

OpalTransportAddressArray OpalGetInterfaceAddresses(const OpalListenerList & listeners,
                                                    BOOL excludeLocalHost,
                                                    OpalTransport * associatedTransport)
{
  OpalTransportAddressArray interfaceAddresses;

  PINDEX i;
  for (i = 0; i < listeners.GetSize(); i++) {
    OpalTransportAddressArray newAddrs = OpalGetInterfaceAddresses(listeners[i].GetTransportAddress(), excludeLocalHost, associatedTransport);
    PINDEX size  = interfaceAddresses.GetSize();
    PINDEX nsize = newAddrs.GetSize();
    interfaceAddresses.SetSize(size + nsize);
    PINDEX j;
    for (j = 0; j < nsize; j++)
      interfaceAddresses.SetAt(size + j, new OpalTransportAddress(newAddrs[j]));
  }

  return interfaceAddresses;
}


OpalTransportAddressArray OpalGetInterfaceAddresses(const OpalTransportAddress & addr,
                                                    BOOL excludeLocalHost,
                                                    OpalTransport * associatedTransport)
{
  PIPSocket::Address ip;
  WORD port = 0;
  if (!addr.GetIpAndPort(ip, port) || !ip.IsAny())
    return addr;

  PIPSocket::InterfaceTable interfaces;
  if (!PIPSocket::GetInterfaceTable(interfaces))
    return addr;

  if (interfaces.GetSize() == 1)
    return OpalTransportAddress(interfaces[0].GetAddress(), port);

  PINDEX i;
  OpalTransportAddressArray interfaceAddresses;
  PIPSocket::Address firstAddress(0);

  if (associatedTransport != NULL) {
    if (associatedTransport->GetLocalAddress().GetIpAddress(firstAddress)) {
      for (i = 0; i < interfaces.GetSize(); i++) {
        PIPSocket::Address ip = interfaces[i].GetAddress();
        if (ip == firstAddress)
          interfaceAddresses.Append(new OpalTransportAddress(ip, port));
      }
    }
  }

  for (i = 0; i < interfaces.GetSize(); i++) {
    PIPSocket::Address ip = interfaces[i].GetAddress();
    if (ip != firstAddress && !(excludeLocalHost && ip.IsLoopback()))
      interfaceAddresses.Append(new OpalTransportAddress(ip, port));
  }

  return interfaceAddresses;
}


//////////////////////////////////////////////////////////////////////////

OpalListenerIP::OpalListenerIP(OpalEndPoint & ep,
                               PIPSocket::Address binding,
                               WORD port,
                               BOOL exclusive)
  : OpalListener(ep),
    localAddress(binding)
{
  listenerPort = port;
  exclusiveListener = exclusive;
}


OpalTransportAddress OpalListenerIP::GetLocalAddress(const OpalTransportAddress & preferredAddress) const
{
  PString addr;

  // If specifically bound to interface use that
  if (!localAddress.IsAny())
    addr = localAddress.AsString();
  else {
    // If bound to all, then use '*' unless a preferred address is specified
    addr = "*";

    PIPSocket::Address ip;
    if (preferredAddress.GetIpAddress(ip)) {
      // Verify preferred address is actually an interface in this machine!
      PIPSocket::InterfaceTable interfaces;
      if (PIPSocket::GetInterfaceTable(interfaces)) {
        for (PINDEX i = 0; i < interfaces.GetSize(); i++) {
          if (interfaces[i].GetAddress() == ip) {
            addr = ip.AsString();
            break;
          }
        }
      }
    }
  }

  addr.sprintf(":%u", listenerPort);

  return GetProtoPrefix() + addr;
}


//////////////////////////////////////////////////////////////////////////

OpalListenerTCP::OpalListenerTCP(OpalEndPoint & ep,
                                 PIPSocket::Address binding,
                                 WORD port,
                                 BOOL exclusive)
  : OpalListenerIP(ep, binding, port, exclusive)
{
  listenerPort = port;
}


OpalListenerTCP::~OpalListenerTCP()
{
  CloseWait();
}


BOOL OpalListenerTCP::Open(const PNotifier & theAcceptHandler, BOOL isSingleThread)
{
  if (listenerPort == 0) {
    OpalManager & manager = endpoint.GetManager();
    listenerPort = manager.GetNextTCPPort();
    WORD firstPort = listenerPort;
    while (!listener.Listen(localAddress, 1, listenerPort)) {
      listenerPort = manager.GetNextTCPPort();
      if (listenerPort == firstPort) {
        PTRACE(1, "Listen\tOpen on " << localAddress << " failed: " << listener.GetErrorText());
        break;
      }
    }
    listenerPort = listener.GetPort();
    return StartThread(theAcceptHandler, isSingleThread);
  }

  if (listener.Listen(localAddress, 1, listenerPort))
    return StartThread(theAcceptHandler, isSingleThread);

  if (exclusiveListener) {
    PTRACE(1, "Listen\tOpen on " << localAddress << ':' << listener.GetPort()
           << " failed: " << listener.GetErrorText());
    return FALSE;
  }

  if (listener.GetErrorNumber() != EADDRINUSE)
    return FALSE;

  PTRACE(1, "Listen\tSocket for " << localAddress << ':' << listener.GetPort()
         << " already in use, incoming connections may not all be serviced!");

  if (listener.Listen(localAddress, 100, 0, PSocket::CanReuseAddress))
    return StartThread(theAcceptHandler, isSingleThread);

  PTRACE(1, "Listen\tOpen (REUSEADDR) on " << localAddress << ':' << listener.GetPort()
         << " failed: " << listener.GetErrorText());
  return FALSE;
}


BOOL OpalListenerTCP::IsOpen()
{
  return listener.IsOpen();
}


void OpalListenerTCP::Close()
{
  listener.Close();
}


OpalTransport * OpalListenerTCP::Accept(const PTimeInterval & timeout)
{
  if (!listener.IsOpen())
    return NULL;

  listener.SetReadTimeout(timeout); // Wait for remote connect

  PTRACE(4, "Listen\tWaiting on socket accept on " << GetLocalAddress());
  PTCPSocket * socket = new PTCPSocket;
  if (socket->Accept(listener)) {
    OpalTransportTCP * transport = new OpalTransportTCP(endpoint);
    if (transport->Open(socket))
      return transport;

    PTRACE(1, "Listen\tFailed to open transport, connection not started.");
    delete transport;
    return NULL;
  }
  else if (socket->GetErrorCode() != PChannel::Interrupted) {
    PTRACE(1, "Listen\tAccept error:" << socket->GetErrorText());
    listener.Close();
  }

  delete socket;
  return NULL;
}


const char * OpalListenerTCP::GetProtoPrefix() const
{
  return TcpPrefix;
}


//////////////////////////////////////////////////////////////////////////

OpalListenerUDP::OpalListenerUDP(OpalEndPoint & endpoint,
                                 PIPSocket::Address binding,
                                 WORD port,
                                 BOOL exclusive)
  : OpalListenerIP(endpoint, binding, port, exclusive)
{
}


OpalListenerUDP::~OpalListenerUDP()
{
  CloseWait();
}


BOOL OpalListenerUDP::OpenOneSocket(const PIPSocket::Address & address)
{
  PUDPSocket * socket = new PUDPSocket(listenerPort);
  if (socket->Listen(address)) {
    listeners.Append(socket);
    if (listenerPort == 0)
      listenerPort = socket->GetPort();
    return TRUE;
  }

  PTRACE(1, "Listen\tError in UDP listen: " << socket->GetErrorText());
  delete socket;
  return FALSE;
}


BOOL OpalListenerUDP::Open(const PNotifier & theAcceptHandler, BOOL /*isSingleThread*/)
{
  PIPSocket::InterfaceTable interfaces;
  if (!PIPSocket::GetInterfaceTable(interfaces)) {
    PTRACE(1, "Listen\tNo interfaces on system!");
    return OpenOneSocket(localAddress);
  }

  for (PINDEX i = 0; i < interfaces.GetSize(); i++) {
    PIPSocket::Address addr = interfaces[i].GetAddress();
    if (addr != 0 && (localAddress.IsAny() || localAddress == addr)) {
      if (OpenOneSocket(addr)) {
#ifndef _WIN32
        PIPSocket::Address mask = interfaces[i].GetNetMask();
        if (mask != 0 && mask != 0xffffffff)
          OpenOneSocket((addr&mask)|(0xffffffff&~mask));
#endif
      }
    }
  }

  if (listeners.GetSize() > 0)
    return StartThread(theAcceptHandler, TRUE);

  PTRACE(1, "Listen\tCould not start any UDP listeners");
  return FALSE;
}


BOOL OpalListenerUDP::IsOpen()
{
  for (PINDEX i = 0; i < listeners.GetSize(); i++) {
    if (!listeners[i].IsOpen())
      return FALSE;
  }
  return TRUE;
}


void OpalListenerUDP::Close()
{
  for (PINDEX i = 0; i < listeners.GetSize(); i++)
    listeners[i].Close();
}


OpalTransport * OpalListenerUDP::Accept(const PTimeInterval & timeout)
{
  if (listeners.IsEmpty())
    return NULL;

  PSocket::SelectList selection;
  PINDEX i;
  for (i = 0; i < listeners.GetSize(); i++)
    selection += listeners[i];

  PTRACE(4, "Listen\tWaiting on UDP packet on " << GetLocalAddress());
  PChannel::Errors error = PSocket::Select(selection, timeout);

  if (error != PChannel::NoError || selection.IsEmpty()) {
    PTRACE(1, "Listen\tUDP select error: " << PSocket::GetErrorText(error));
    return NULL;
  }

  PUDPSocket & socket = (PUDPSocket &)selection[0];

  if (singleThread)
    return new OpalTransportUDP(endpoint, socket);

  PBYTEArray pdu;
  PIPSocket::Address addr;
  WORD port;
  if (socket.ReadFrom(pdu.GetPointer(2000), 2000, addr, port))
    return new OpalTransportUDP(endpoint, localAddress, pdu, addr, port);

  PTRACE(1, "Listen\tUDP read error: " << socket.GetErrorText());
  return NULL;
}


const char * OpalListenerUDP::GetProtoPrefix() const
{
  return UdpPrefix;
}


//////////////////////////////////////////////////////////////////////////

OpalTransport::OpalTransport(OpalEndPoint & end)
  : endpoint(end)
{
  thread = NULL;
}


OpalTransport::~OpalTransport()
{
  PAssert(thread == NULL, PLogicError);
}


void OpalTransport::PrintOn(ostream & strm) const
{
  strm << GetRemoteAddress() << "<if=" << GetLocalAddress() << '>';
}


void OpalTransport::EndConnect(const OpalTransportAddress &)
{
}


BOOL OpalTransport::Close()
{
  PTRACE(4, "Opal\tTransport Close");

  /* Do not use PIndirectChannel::Close() as this deletes the sub-channel
     member field crashing the background thread. Just close the base
     sub-channel so breaks the threads I/O block.
   */
  if (IsOpen())
    return GetBaseWriteChannel()->Close();

  return TRUE;
}


void OpalTransport::CloseWait()
{
  PTRACE(3, "Opal\tTransport clean up on termination");

  Close();

  if (thread != NULL) {
    PAssert(thread->WaitForTermination(10000), "Transport thread did not terminate");
    if (thread == PThread::Current())
      thread->SetAutoDelete();
    else
      delete thread;
    thread = NULL;
  }
}


BOOL OpalTransport::IsCompatibleTransport(const OpalTransportAddress &) const
{
  PAssertAlways(PUnimplementedFunction);
  return FALSE;
}


void OpalTransport::SetPromiscuous(PromisciousModes /*promiscuous*/)
{
}


OpalTransportAddress OpalTransport::GetLastReceivedAddress() const
{
  return GetRemoteAddress();
}


BOOL OpalTransport::WriteConnect(WriteConnectCallback function, void * userData)
{
  return function(*this, userData);
}


void OpalTransport::AttachThread(PThread * thrd)
{
  if (thread != NULL) {
    PAssert(thread->WaitForTermination(10000), "Transport not terminated when reattaching thread");
    delete thread;
  }

  thread = thrd;
}


BOOL OpalTransport::IsRunning() const
{
  if (thread == NULL)
    return FALSE;

  return !thread->IsTerminated();
}


/////////////////////////////////////////////////////////////////////////////

OpalTransportIP::OpalTransportIP(OpalEndPoint & end,
                                 PIPSocket::Address binding,
                                 WORD port)
  : OpalTransport(end),
    localAddress(binding),
    remoteAddress(0)
{
  localPort = port;
  remotePort = 0;
}


OpalTransportAddress OpalTransportIP::GetLocalAddress() const
{
  return OpalTransportAddress(localAddress, localPort, GetProtoPrefix());
}


BOOL OpalTransportIP::SetLocalAddress(const OpalTransportAddress & newLocalAddress)
{
  if (!IsCompatibleTransport(newLocalAddress))
    return FALSE;

  if (!IsOpen())
    return newLocalAddress.GetIpAndPort(localAddress, localPort);

  PIPSocket::Address address;
  WORD port = 0;
  if (!newLocalAddress.GetIpAndPort(address, port))
    return FALSE;

  return localAddress == address && localPort == port;
}


OpalTransportAddress OpalTransportIP::GetRemoteAddress() const
{
  return OpalTransportAddress(remoteAddress, remotePort, GetProtoPrefix());
}


BOOL OpalTransportIP::SetRemoteAddress(const OpalTransportAddress & address)
{
  if (IsCompatibleTransport(address))
    return address.GetIpAndPort(remoteAddress, remotePort);

  return FALSE;
}


/////////////////////////////////////////////////////////////////////////////

OpalTransportTCP::OpalTransportTCP(OpalEndPoint & ep,
                                   PIPSocket::Address binding,
                                   WORD port,
                                   BOOL reuseAddr)
  : OpalTransportIP(ep, binding, port)
{
  reuseAddressFlag = reuseAddr;
}


OpalTransportTCP::OpalTransportTCP(OpalEndPoint & ep, PTCPSocket * socket)
  : OpalTransportIP(ep, INADDR_ANY, 0)
{
  Open(socket);
}


OpalTransportTCP::~OpalTransportTCP()
{
  CloseWait();
  PTRACE(4,"Opal\tDeleted transport " << *this);
}


BOOL OpalTransportTCP::IsReliable() const
{
  return TRUE;
}


BOOL OpalTransportTCP::IsCompatibleTransport(const OpalTransportAddress & address) const
{
  return (address.Left(strlen(TcpPrefix)) *= TcpPrefix) ||
         (address.Left(sizeof(IpPrefix)-1) *= IpPrefix);
}


BOOL OpalTransportTCP::Connect()
{
  if (IsOpen())
    return TRUE;

  PTCPSocket * socket = new PTCPSocket(remotePort);
  Open(socket);

  PReadWaitAndSignal mutex(channelPointerMutex);

  socket->SetReadTimeout(10000);

  OpalManager & manager = endpoint.GetManager();
  localPort = manager.GetNextTCPPort();
  WORD firstPort = localPort;
  for (;;) {
    PTRACE(4, "OpalTCP\tConnecting to "
           << remoteAddress << ':' << remotePort
           << " (local port=" << localPort << ')');
    if (socket->Connect(localPort, remoteAddress))
      break;

    int errnum = socket->GetErrorNumber();
    if (localPort == 0 || (errnum != EADDRINUSE && errnum != EADDRNOTAVAIL)) {
      PTRACE(1, "OpalTCP\tCould not connect to "
                << remoteAddress << ':' << remotePort
                << " (local port=" << localPort << ") - "
                << socket->GetErrorText() << '(' << errnum << ')');
      return SetErrorValues(socket->GetErrorCode(), errnum);
    }

    localPort = manager.GetNextTCPPort();
    if (localPort == firstPort) {
      PTRACE(1, "OpalTCP\tCould not bind to any port in range " <<
                manager.GetTCPPortBase() << " to " << manager.GetTCPPortMax());
      return SetErrorValues(socket->GetErrorCode(), errnum);
    }
  }

  socket->SetReadTimeout(PMaxTimeInterval);

  return OnOpen();
}


BOOL OpalTransportTCP::ReadPDU(PBYTEArray & pdu)
{
  // Make sure is a RFC1006 TPKT
  switch (ReadChar()) {
    case 3 :  // Only support version 3
      break;

    default :  // Unknown version number
      SetErrorValues(ProtocolFailure, 0x80000000);
      // Do case for read error

    case -1 :
      return FALSE;
  }

  // Save timeout
  PTimeInterval oldTimeout = GetReadTimeout();

  // Should get all of PDU in 5 seconds or something is seriously wrong,
  SetReadTimeout(5000);

  // Get TPKT length
  BYTE header[3];
  BOOL ok = ReadBlock(header, sizeof(header));
  if (ok) {
    PINDEX packetLength = ((header[1] << 8)|header[2]);
    if (packetLength < 4) {
      PTRACE(1, "H323TCP\tDwarf PDU received (length " << packetLength << ")");
      ok = FALSE;
    } else {
      packetLength -= 4;
      ok = ReadBlock(pdu.GetPointer(packetLength), packetLength);
    }
  }

  SetReadTimeout(oldTimeout);

  return ok;
}


BOOL OpalTransportTCP::WritePDU(const PBYTEArray & pdu)
{
  // We copy the data into a new buffer so we can do a single write call. This
  // is necessary as we have disabled the Nagle TCP delay algorithm to improve
  // network performance.

  int packetLength = pdu.GetSize() + 4;

  // Send RFC1006 TPKT length
  PBYTEArray tpkt(packetLength);
  tpkt[0] = 3;
  tpkt[1] = 0;
  tpkt[2] = (BYTE)(packetLength >> 8);
  tpkt[3] = (BYTE)packetLength;
  memcpy(tpkt.GetPointer()+4, (const BYTE *)pdu, pdu.GetSize());

  return Write((const BYTE *)tpkt, packetLength);
}


BOOL OpalTransportTCP::OnOpen()
{
  PIPSocket * socket = (PIPSocket *)GetReadChannel();

  // Get name of the remote computer for information purposes
  if (!socket->GetPeerAddress(remoteAddress, remotePort)) {
    PTRACE(1, "OpalTCP\tGetPeerAddress() failed: " << socket->GetErrorText());
    return FALSE;
  }

  // get local address of incoming socket to ensure that multi-homed machines
  // use a NIC address that is guaranteed to be addressable to destination
  if (!socket->GetLocalAddress(localAddress, localPort)) {
    PTRACE(1, "OpalTCP\tGetLocalAddress() failed: " << socket->GetErrorText());
    return FALSE;
  }

#ifndef __BEOS__
  if (!socket->SetOption(TCP_NODELAY, 1, IPPROTO_TCP)) {
    PTRACE(1, "OpalTCP\tSetOption(TCP_NODELAY) failed: " << socket->GetErrorText());
  }

  // make sure do not lose outgoing packets on close
  const linger ling = { 1, 3 };
  if (!socket->SetOption(SO_LINGER, &ling, sizeof(ling))) {
    PTRACE(1, "OpalTCP\tSetOption(SO_LINGER) failed: " << socket->GetErrorText());
    return FALSE;
  }
#endif

  PTRACE(1, "OpalTCP\tStarted connection to "
         << remoteAddress << ':' << remotePort
         << " (if=" << localAddress << ':' << localPort << ')');

  return TRUE;
}


const char * OpalTransportTCP::GetProtoPrefix() const
{
  return TcpPrefix;
}


/////////////////////////////////////////////////////////////////////////////

OpalTransportUDP::OpalTransportUDP(OpalEndPoint & ep,
                                   PIPSocket::Address binding,
                                   WORD port,
                                   BOOL reuseAddr)
  : OpalTransportIP(ep, binding, port)
{
  promiscuousReads = AcceptFromRemoteOnly;
  socketOwnedByListener = FALSE;
  reuseAddressFlag = reuseAddr;

  PUDPSocket * udp = new PUDPSocket;
  udp->Listen(binding, 0, port,
              reuseAddr ? PSocket::CanReuseAddress : PSocket::AddressIsExclusive);

  localPort = udp->GetPort();

  Open(udp);

  PTRACE(3, "OpalUDP\tBinding to interface: " << localAddress << ':' << localPort);
}


OpalTransportUDP::OpalTransportUDP(OpalEndPoint & ep, PUDPSocket & udp)
  : OpalTransportIP(ep, PIPSocket::GetDefaultIpAny(), 0)
{
  promiscuousReads = AcceptFromAnyAutoSet;
  socketOwnedByListener = TRUE;
  reuseAddressFlag = FALSE;

  udp.GetLocalAddress(localAddress, localPort);

  Open(udp);


  PTRACE(3, "OpalUDP\tPre-bound to interface: " << localAddress << ':' << localPort);
}


OpalTransportUDP::OpalTransportUDP(OpalEndPoint & ep,
                                   PIPSocket::Address binding,
                                   const PBYTEArray & packet,
                                   PIPSocket::Address remAddr,
                                   WORD remPort)
  : OpalTransportIP(ep, binding, 0),
    preReadPacket(packet)
{
  promiscuousReads = AcceptFromAnyAutoSet;
  socketOwnedByListener = FALSE;

  remoteAddress = remAddr;
  remotePort = remPort;

  PUDPSocket * udp = new PUDPSocket;
  udp->Listen(binding);

  localPort = udp->GetPort();

  Open(udp);

  PTRACE(3, "OpalUDP\tBinding to interface: " << localAddress << ':' << localPort);
}


OpalTransportUDP::~OpalTransportUDP()
{
  CloseWait();
  PTRACE(4,"Opal\tDeleted transport " << *this);
}


BOOL OpalTransportUDP::Close()
{
  PTRACE(4, "OpalUDP\tClose");
  PReadWaitAndSignal mutex(channelPointerMutex);

  if (socketOwnedByListener) {
    channelPointerMutex.StartWrite();
    readChannel = writeChannel = NULL;
    // Thread also owned by listener as well, don't wait on or delete it!
    thread = NULL; 
    channelPointerMutex.EndWrite();
    return TRUE;
  }

  if (connectSockets.IsEmpty())
    return OpalTransport::Close();

  channelPointerMutex.StartWrite();
  readChannel = writeChannel = NULL;

  // Still in connection on multiple interface phase. Close all of the
  // sockets we have open.
  for (PINDEX i = 0; i < connectSockets.GetSize(); i++)
    connectSockets[i].Close();

  channelPointerMutex.EndWrite();

  return TRUE;
}


BOOL OpalTransportUDP::IsReliable() const
{
  return FALSE;
}


BOOL OpalTransportUDP::IsCompatibleTransport(const OpalTransportAddress & address) const
{
  return address.Left(strlen(UdpPrefix)).ToLower () == UdpPrefix ||
         address.Left(sizeof(IpPrefix)-1).ToLower () == IpPrefix;
}


BOOL OpalTransportUDP::Connect()
{	
  if (remotePort == 0)
    return FALSE;

  if (remoteAddress == 0) {
    remoteAddress = INADDR_BROADCAST;
    PTRACE(2, "OpalUDP\tBroadcast connect to port " << remotePort);
  }
  else {
	PTRACE(2, "OpalUDP\tStarted connect to " << remoteAddress << ':' << remotePort);
  }

  OpalManager & manager = endpoint.GetManager();

  PSTUNClient * stun = manager.GetSTUN(remoteAddress);
  if (stun != NULL) {
    PUDPSocket * socket;
    if (stun->CreateSocket(socket)) {
      PIndirectChannel::Close();	//closing the channel and opening it with the new socket
      readAutoDelete = writeAutoDelete = FALSE;
      Open(socket);
      socket->GetLocalAddress(localAddress, localPort);
      socket->SetSendAddress(remoteAddress, remotePort);
      PTRACE(4, "OpalUDP\tSTUN created socket: " << localAddress << ':' << localPort);
      connectSockets.Append(socket);
      return true;
    }
    PTRACE(4, "OpalUDP\tSTUN could not create socket!");
  }
  

  // See if prebound to interface, only use that if so
  PIPSocket::InterfaceTable interfaces;
  if (localAddress != INADDR_ANY) {
    PTRACE(3, "OpalUDP\tConnect on pre-bound interface: " << localAddress);
    interfaces.Append(new PIPSocket::InterfaceEntry("", localAddress, PIPSocket::Address(0xffffffff), ""));
  }
  else if (!PIPSocket::GetInterfaceTable(interfaces)) {
    PTRACE(1, "OpalUDP\tNo interfaces on system!");
    interfaces.Append(new PIPSocket::InterfaceEntry("", localAddress, PIPSocket::Address(0xffffffff), ""));
  }
  else {
    PTRACE(4, "OpalUDP\tConnecting to interfaces:\n" << setfill('\n') << interfaces << setfill(' '));
  }

  PIndirectChannel::Close();	//closing the channel and opening it with the new socket
  PINDEX i;
  for (i = 0; i < interfaces.GetSize(); i++) {
    PIPSocket::Address interfaceAddress = interfaces[i].GetAddress();
    if (interfaceAddress == 0 || interfaceAddress == PIPSocket::Address())
      continue;

    // Check for already have had that IP address.
    PINDEX j;
    for (j = 0; j < i; j++) {
      if (interfaceAddress == interfaces[j].GetAddress())
        break;
    }
    if (j < i)
      continue;

    // Not explicitly multicast
    PUDPSocket * socket = new PUDPSocket;
    localPort = manager.GetNextUDPPort();
    WORD firstPort = localPort;
    while (!socket->Listen(interfaceAddress, 0, localPort, reuseAddressFlag?PIPSocket::CanReuseAddress:PIPSocket::AddressIsExclusive)) {
      localPort = manager.GetNextUDPPort();
      if (localPort == firstPort) {
        PTRACE(1, "OpalUDP\tCould not bind to any port in range " <<
	       manager.GetUDPPortBase() << " to " << manager.GetUDPPortMax());
        return FALSE;
      }
    }
    readAutoDelete = writeAutoDelete = FALSE;
    if (!PIndirectChannel::IsOpen())
      Open(socket);

#ifndef __BEOS__
    if (remoteAddress == INADDR_BROADCAST) {
      if (!socket->SetOption(SO_BROADCAST, 1)) {
        PTRACE(2, "OpalUDP\tError allowing broadcast: " << socket->GetErrorText());
        return FALSE;
      }
    }
#else
    PTRACE(3, "RAS\tBroadcast option under BeOS is not implemented yet");
#endif

    socket->GetLocalAddress(localAddress, localPort);
    socket->SetSendAddress(remoteAddress, remotePort);
    connectSockets.Append(socket);
  }
  
  readAutoDelete = writeAutoDelete = FALSE;

  if (connectSockets.IsEmpty())
    return FALSE;

  promiscuousReads = AcceptFromAnyAutoSet;

  return TRUE;
}


void OpalTransportUDP::EndConnect(const OpalTransportAddress & theLocalAddress)
{
  PAssert(theLocalAddress.GetIpAndPort(localAddress, localPort), PInvalidParameter);

  for (PINDEX i = 0; i < connectSockets.GetSize(); i++) {
    PUDPSocket * socket = (PUDPSocket *)connectSockets.GetAt(i);
    PIPSocket::Address addr;
    WORD port;
    if (socket->GetLocalAddress(addr, port) && addr == localAddress && port == localPort) {
      PTRACE(3, "OpalUDP\tEnded connect, selecting " << localAddress << ':' << localPort);
      connectSockets.DisallowDeleteObjects();
      connectSockets.RemoveAt(i);
      connectSockets.AllowDeleteObjects();
      readChannel = NULL;
      writeChannel = NULL;
      socket->SetOption(SO_BROADCAST, 0);
      PAssert(Open(socket), PLogicError);
      break;
    }
  }

  connectSockets.RemoveAll();

  OpalTransport::EndConnect(theLocalAddress);
}


BOOL OpalTransportUDP::SetLocalAddress(const OpalTransportAddress & newLocalAddress)
{
  if (connectSockets.IsEmpty())
    return OpalTransportIP::SetLocalAddress(newLocalAddress);

  if (!IsCompatibleTransport(newLocalAddress))
    return FALSE;

  if (!newLocalAddress.GetIpAndPort(localAddress, localPort))
    return FALSE;

  for (PINDEX i = 0; i < connectSockets.GetSize(); i++) {
    PUDPSocket * socket = (PUDPSocket *)connectSockets.GetAt(i);
    PIPSocket::Address ip;
    WORD port = 0;
    if (socket->GetLocalAddress(ip, port) && ip == localAddress && port == localPort) {
      writeChannel = &connectSockets[i];
      return TRUE;
    }
  }

  return FALSE;
}


BOOL OpalTransportUDP::SetRemoteAddress(const OpalTransportAddress & address)
{
  if (!OpalTransportIP::SetRemoteAddress(address))
    return FALSE;

  PUDPSocket * socket = (PUDPSocket *)GetReadChannel();
  if (socket != NULL)
    socket->SetSendAddress(remoteAddress, remotePort);
  return TRUE;
}


void OpalTransportUDP::SetPromiscuous(PromisciousModes promiscuous)
{
  promiscuousReads = promiscuous;
}


OpalTransportAddress OpalTransportUDP::GetLastReceivedAddress() const
{
  if (!lastReceivedAddress)
    return lastReceivedAddress;

  return OpalTransport::GetLastReceivedAddress();
}


BOOL OpalTransportUDP::Read(void * buffer, PINDEX length)
{
  if (!connectSockets.IsEmpty()) {
    PSocket::SelectList selection;

    PINDEX i;
    for (i = 0; i < connectSockets.GetSize(); i++)
      selection += connectSockets[i];

    if (PSocket::Select(selection, GetReadTimeout()) != PChannel::NoError) {
      PTRACE(1, "OpalUDP\tError on connection read select.");
      return FALSE;
    }

    if (selection.IsEmpty()) {
      PTRACE(2, "OpalUDP\tTimeout on connection read select.");
      return FALSE;
    }

    PUDPSocket & socket = (PUDPSocket &)selection[0];
    channelPointerMutex.StartWrite();
    if (!socket.IsOpen()) {
      channelPointerMutex.EndWrite();
      PTRACE(2, "OpalUDP\tSocket closed in connection read select.");
      return FALSE;
    }
    socket.GetLocalAddress(localAddress, localPort);
    readChannel = &socket;
    channelPointerMutex.EndWrite();
  }

  for (;;) {
    if (!OpalTransportIP::Read(buffer, length))
      return FALSE;

    PUDPSocket * socket = (PUDPSocket *)GetReadChannel();

    PIPSocket::Address address;
    WORD port;

    socket->GetLastReceiveAddress(address, port);
    lastReceivedAddress = OpalTransportAddress(address, port);

    switch (promiscuousReads) {
      case AcceptFromRemoteOnly :
        if (remoteAddress == address)
          return TRUE;
        break;

      case AcceptFromAnyAutoSet :
        remoteAddress = address;
        remotePort = port;
        socket->SetSendAddress(remoteAddress, remotePort);
        // fall into next case

      default : //AcceptFromAny
        return TRUE;
    }

    if (remoteAddress *= address)
      return TRUE;

    PTRACE(1, "UDP\tReceived PDU from incorrect host: " << address << ':' << port);
  }
}


BOOL OpalTransportUDP::ReadPDU(PBYTEArray & packet)
{
  if (preReadPacket.GetSize() > 0) {
    packet = preReadPacket;
    preReadPacket.SetSize(0);
    return TRUE;
  }

  if (!Read(packet.GetPointer(10000), 10000)) {
    packet.SetSize(0);
    return FALSE;
  }

  packet.SetSize(GetLastReadCount());
  return TRUE;
}


BOOL OpalTransportUDP::WritePDU(const PBYTEArray & packet)
{
  return Write((const BYTE *)packet, packet.GetSize());
}


BOOL OpalTransportUDP::WriteConnect(WriteConnectCallback function, void * userData)
{
  if (connectSockets.IsEmpty())
    return OpalTransport::WriteConnect(function, userData);

  BOOL ok = FALSE;

  PINDEX i;
  for (i = 0; i < connectSockets.GetSize(); i++) {
    PUDPSocket & socket = (PUDPSocket &)connectSockets[i];
    socket.GetLocalAddress(localAddress, localPort);
    writeChannel = &socket;
    if (function(*this, userData))
      ok = TRUE;
  }

  return ok;
}


const char * OpalTransportUDP::GetProtoPrefix() const
{
  return UdpPrefix;
}


//////////////////////////////////////////////////////////////////////////
