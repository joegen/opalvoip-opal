/*
 * h323ep.cxx
 *
 * H.323 protocol handler
 *
 * Open H323 Library
 *
 * Copyright (c) 1998-2000 Equivalence Pty. Ltd.
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
 * Vovida Networks, Inc. http://www.vovida.com.
 *
 * Contributor(s): ______________________________________.
 *
 * $Log: h323ep.cxx,v $
 * Revision 1.2012  2001/11/13 04:29:48  robertj
 * Changed OpalTransportAddress CreateTransport and CreateListsner functions
 *   to have extra parameter to control local binding of sockets.
 *
 * Revision 2.10  2001/11/12 05:32:12  robertj
 * Added OpalTransportAddress::GetIpAddress when don't need port number.
 *
 * Revision 2.9  2001/11/02 10:45:19  robertj
 * Updated to OpenH323 v1.7.3
 *
 * Revision 2.8  2001/10/15 04:35:44  robertj
 * Removed answerCall signal and replaced with state based functions.
 * Maintained H.323 answerCall API for backward compatibility.
 *
 * Revision 2.7  2001/10/05 00:22:14  robertj
 * Updated to PWLib 1.2.0 and OpenH323 1.7.0
 *
 * Revision 2.6  2001/08/23 03:15:51  robertj
 * Added missing Lock() calls in SetUpConnection
 *
 * Revision 2.5  2001/08/22 10:20:09  robertj
 * Changed connection locking to use double mutex to guarantee that
 *   no threads can ever deadlock or access deleted connection.
 *
 * Revision 2.4  2001/08/17 08:27:44  robertj
 * Update from OpenH323
 * Moved call end reasons enum from OpalConnection to global.
 *
 * Revision 2.3  2001/08/13 05:10:39  robertj
 * Updates from OpenH323 v1.6.0 release.
 *
 * Revision 1.109  2001/11/01 06:11:57  robertj
 * Plugged very small mutex hole that could cause crashes.
 *
 * Revision 1.108  2001/11/01 00:59:07  robertj
 * Added auto setting of silence detect mode when using PSoundChannel codec.
 *
 * Revision 1.107  2001/11/01 00:27:35  robertj
 * Added default Fast Start disabled and H.245 tunneling disable flags
 *   to the endpoint instance.
 *
 * Revision 1.106  2001/10/30 07:34:33  robertj
 * Added trace for when cleaner thread start, stops and runs
 *
 * Revision 1.105  2001/10/24 07:25:59  robertj
 * Fixed possible deadlocks during destruction of H323Connection object.
 *
 * Revision 1.104  2001/09/26 06:20:59  robertj
 * Fixed properly nesting connection locking and unlocking requiring a quite
 *   large change to teh implementation of how calls are answered.
 *
 * Revision 1.103  2001/09/13 02:41:21  robertj
 * Fixed call reference generation to use full range and common code.
 *
 * Revision 1.102  2001/09/11 01:24:36  robertj
 * Added conditional compilation to remove video and/or audio codecs.
 *
 * Revision 1.101  2001/09/11 00:21:23  robertj
 * Fixed missing stack sizes in endpoint for cleaner thread and jitter thread.
 *
 * Revision 1.100  2001/08/24 13:38:25  rogerh
 * Free the locally declared listener.
 *
 * Revision 1.99  2001/08/24 13:23:59  rogerh
 * Undo a recent memory leak fix. The listener object has to be deleted in the
 * user application as they use the listener when StartListener() fails.
 * Add a Resume() if StartListener() fails so user applications can delete
 * the suspended thread.
 *
 * Revision 1.98  2001/08/22 06:54:51  robertj
 * Changed connection locking to use double mutex to guarantee that
 *   no threads can ever deadlock or access deleted connection.
 *
 * Revision 1.97  2001/08/16 07:49:19  robertj
 * Changed the H.450 support to be more extensible. Protocol handlers
 *   are now in separate classes instead of all in H323Connection.
 *
 * Revision 1.96  2001/08/10 11:03:52  robertj
 * Major changes to H.235 support in RAS to support server.
 *
 * Revision 1.95  2001/08/08 23:55:27  robertj
 * Fixed problem with setting gk password before have a gk variable.
 * Fixed memory leak if listener fails to open.
 * Fixed problem with being able to specify an alias with an @ in it.
 *
 * Revision 2.2  2001/08/01 05:14:19  robertj
 * Moved media formats list from endpoint to connection.
 * Fixed accept of call to early enough to set connections capabilities.
 *
 * Revision 2.1  2001/07/30 07:22:25  robertj
 * Abstracted listener management from H.323 to OpalEndPoint class.
 *
 * Revision 2.0  2001/07/27 15:48:25  robertj
 * Conversion of OpenH323 to Open Phone Abstraction Library (OPAL)
 *
 * Revision 1.92  2001/08/02 04:31:48  robertj
 * Changed to still maintain gatekeeper link if GRQ succeeded but RRQ
 *   failed. Thanks Ben Madsen & Graeme Reid.
 *
 * Revision 1.91  2001/07/17 04:44:32  robertj
 * Partial implementation of T.120 and T.38 logical channels.
 *
 * Revision 1.90  2001/07/06 07:28:55  robertj
 * Fixed rearranged connection creation to avoid possible nested mutex.
 *
 * Revision 1.89  2001/07/06 04:46:19  robertj
 * Rearranged connection creation to avoid possible nested mutex.
 *
 * Revision 1.88  2001/06/19 08:43:38  robertj
 * Fixed deadlock if ClearCallSynchronous ever called from cleaner thread.
 *
 * Revision 1.87  2001/06/19 03:55:30  robertj
 * Added transport to CreateConnection() function so can use that as part of
 *   the connection creation decision making process.
 *
 * Revision 1.86  2001/06/15 00:55:53  robertj
 * Added thread name for cleaner thread
 *
 * Revision 1.85  2001/06/14 23:18:06  robertj
 * Change to allow for CreateConnection() to return NULL to abort call.
 *
 * Revision 1.84  2001/06/14 04:23:32  robertj
 * Changed incoming call to pass setup pdu to endpoint so it can create
 *   different connection subclasses depending on the pdu eg its alias
 *
 * Revision 1.83  2001/06/02 01:35:32  robertj
 * Added thread names.
 *
 * Revision 1.82  2001/05/30 11:13:54  robertj
 * Fixed possible deadlock when getting connection from endpoint.
 *
 * Revision 1.81  2001/05/14 05:56:28  robertj
 * Added H323 capability registration system so can add capabilities by
 *   string name instead of having to instantiate explicit classes.
 *
 * Revision 1.80  2001/05/01 04:34:11  robertj
 * Changed call transfer API slightly to be consistent with new hold function.
 *
 * Revision 1.79  2001/04/23 01:31:15  robertj
 * Improved the locking of connections especially at shutdown.
 *
 * Revision 1.78  2001/04/11 03:01:29  robertj
 * Added H.450.2 (call transfer), thanks a LOT to Graeme Reid & Norwood Systems
 *
 * Revision 1.77  2001/03/21 04:52:42  robertj
 * Added H.235 security to gatekeepers, thanks Fürbass Franz!
 *
 * Revision 1.76  2001/03/17 00:05:52  robertj
 * Fixed problems with Gatekeeper RIP handling.
 *
 * Revision 1.75  2001/03/15 00:24:47  robertj
 * Added function for setting gatekeeper with address and identifier values.
 *
 * Revision 1.74  2001/02/27 00:03:59  robertj
 * Fixed possible deadlock in FindConnectionsWithLock(), thanks Hans Andersen
 *
 * Revision 1.73  2001/02/16 04:11:35  robertj
 * Added ability for RemoveListener() to remove all listeners.
 *
 * Revision 1.72  2001/01/18 06:04:18  robertj
 * Bullet proofed code so local alias can not be empty string. This actually
 *   fixes an ASN PER encoding bug causing an assert.
 *
 * Revision 1.71  2001/01/11 02:19:15  craigs
 * Fixed problem with possible window of opportunity for ClearCallSynchronous
 * to return even though call has not been cleared
 *
 * Revision 1.70  2000/12/22 03:54:33  craigs
 * Fixed problem with listening fix
 *
 * Revision 1.69  2000/12/21 12:38:24  craigs
 * Fixed problem with "Listener thread did not terminate" assert
 * when listener port is in use
 *
 * Revision 1.68  2000/12/20 00:51:03  robertj
 * Fixed MSVC compatibility issues (No trace).
 *
 * Revision 1.67  2000/12/19 22:33:44  dereks
 * Adjust so that the video channel is used for reading/writing raw video
 * data, which better modularizes the video codec.
 *
 * Revision 1.66  2000/12/18 08:59:20  craigs
 * Added ability to set ports
 *
 * Revision 1.65  2000/12/18 01:22:28  robertj
 * Changed semantics or HasConnection() so returns TRUE until the connection
 *   has been deleted and not just until ClearCall() was executure on it.
 *
 * Revision 1.64  2000/11/27 02:44:06  craigs
 * Added ClearCall Synchronous to H323Connection and H323Endpoint to
 * avoid race conditions with destroying descendant classes
 *
 * Revision 1.63  2000/11/26 23:13:23  craigs
 * Added ability to pass user data to H323Connection constructor
 *
 * Revision 1.62  2000/11/12 23:49:16  craigs
 * Added per connection versions of OnEstablished and OnCleared
 *
 * Revision 1.61  2000/10/25 00:53:52  robertj
 * Used official manafacturer code for the OpenH323 project.
 *
 * Revision 1.60  2000/10/20 06:10:51  robertj
 * Fixed very small race condition on creating new connectionon incoming call.
 *
 * Revision 1.59  2000/10/19 04:07:50  robertj
 * Added function to be able to remove a listener.
 *
 * Revision 1.58  2000/10/04 12:21:07  robertj
 * Changed setting of callToken in H323Connection to be as early as possible.
 *
 * Revision 1.57  2000/09/25 12:59:34  robertj
 * Added StartListener() function that takes a H323TransportAddress to start
 *     listeners bound to specific interfaces.
 *
 * Revision 1.56  2000/09/14 23:03:45  robertj
 * Increased timeout on asserting because of driver lockup
 *
 * Revision 1.55  2000/09/01 02:13:05  robertj
 * Added ability to select a gatekeeper on LAN via it's identifier name.
 *
 * Revision 1.54  2000/08/30 05:37:44  robertj
 * Fixed bogus destCallSignalAddress in ARQ messages.
 *
 * Revision 1.53  2000/08/29 02:59:50  robertj
 * Added some debug output for channel open/close.
 *
 * Revision 1.52  2000/08/25 01:10:28  robertj
 * Added assert if various thrads ever fail to terminate.
 *
 * Revision 1.51  2000/07/13 12:34:41  robertj
 * Split autoStartVideo so can select receive and transmit independently
 *
 * Revision 1.50  2000/07/11 19:30:15  robertj
 * Fixed problem where failure to unregister from gatekeeper prevented new registration.
 *
 * Revision 1.49  2000/07/09 14:59:28  robertj
 * Fixed bug if using default transport (no '@') for address when gatekeeper present, used port 1719.
 *
 * Revision 1.48  2000/07/04 04:15:40  robertj
 * Fixed capability check of "combinations" for fast start cases.
 *
 * Revision 1.47  2000/07/04 01:16:50  robertj
 * Added check for capability allowed in "combinations" set, still needs more done yet.
 *
 * Revision 1.46  2000/06/29 11:00:04  robertj
 * Added user interface for sound buffer depth adjustment.
 *
 * Revision 1.45  2000/06/29 07:10:32  robertj
 * Fixed incorrect setting of default number of audio buffers on Win32 systems.
 *
 * Revision 1.44  2000/06/23 02:48:24  robertj
 * Added ability to adjust sound channel buffer depth, needed increasing under Win32.
 *
 * Revision 1.43  2000/06/20 02:38:28  robertj
 * Changed H323TransportAddress to default to IP.
 *
 * Revision 1.42  2000/06/17 09:13:06  robertj
 * Removed redundent line of code, thanks David Iodice.
 *
 * Revision 1.41  2000/06/07 05:48:06  robertj
 * Added call forwarding.
 *
 * Revision 1.40  2000/06/03 03:16:39  robertj
 * Fixed using the wrong capability table (should be connections) for some operations.
 *
 * Revision 1.39  2000/05/25 00:34:59  robertj
 * Changed default tos on Unix platforms to avoid needing to be root.
 *
 * Revision 1.38  2000/05/23 12:57:37  robertj
 * Added ability to change IP Type Of Service code from applications.
 *
 * Revision 1.37  2000/05/23 11:32:37  robertj
 * Rewrite of capability table to combine 2 structures into one and move functionality into that class
 *    allowing some normalisation of usage across several applications.
 * Changed H323Connection so gets a copy of capabilities instead of using endponts, allows adjustments
 *    to be done depending on the remote client application.
 *
 * Revision 1.36  2000/05/16 07:38:50  robertj
 * Added extra debug info indicating sound channel buffer size.
 *
 * Revision 1.35  2000/05/11 03:48:11  craigs
 * Moved SetBuffer command to fix audio delay
 *
 * Revision 1.34  2000/05/02 04:32:26  robertj
 * Fixed copyright notice comment.
 *
 * Revision 1.33  2000/05/01 13:00:28  robertj
 * Changed SetCapability() to append capabilities to TCS, helps with assuring no gaps in set.
 *
 * Revision 1.32  2000/04/30 03:58:37  robertj
 * Changed PSoundChannel to be only double bufferred, this is all that is needed with jitter bufferring.
 *
 * Revision 1.31  2000/04/11 04:02:48  robertj
 * Improved call initiation with gatekeeper, no longer require @address as
 *    will default to gk alias if no @ and registered with gk.
 * Added new call end reasons for gatekeeper denied calls.
 *
 * Revision 1.30  2000/04/10 20:37:33  robertj
 * Added support for more sophisticated DTMF and hook flash user indication.
 *
 * Revision 1.29  2000/04/06 17:50:17  robertj
 * Added auto-start (including fast start) of video channels, selectable via boolean on the endpoint.
 *
 * Revision 1.28  2000/04/05 03:17:31  robertj
 * Added more RTP statistics gathering and H.245 round trip delay calculation.
 *
 * Revision 1.27  2000/03/29 02:14:45  robertj
 * Changed TerminationReason to CallEndReason to use correct telephony nomenclature.
 * Added CallEndReason for capability exchange failure.
 *
 * Revision 1.26  2000/03/25 02:03:36  robertj
 * Added default transport for gatekeeper to be UDP.
 *
 * Revision 1.25  2000/03/23 02:45:29  robertj
 * Changed ClearAllCalls() so will wait for calls to be closed (usefull in endpoint dtors).
 *
 * Revision 1.24  2000/02/17 12:07:43  robertj
 * Used ne wPWLib random number generator after finding major problem in MSVC rand().
 *
 * Revision 1.23  2000/01/07 08:22:49  robertj
 * Added status functions for connection and added transport independent MakeCall
 *
 * Revision 1.22  1999/12/23 23:02:36  robertj
 * File reorganision for separating RTP from H.323 and creation of LID for VPB support.
 *
 * Revision 1.21  1999/12/11 02:21:00  robertj
 * Added ability to have multiple aliases on local endpoint.
 *
 * Revision 1.20  1999/12/09 23:14:20  robertj
 * Fixed deadock in termination with gatekeeper removal.
 *
 * Revision 1.19  1999/11/19 08:07:27  robertj
 * Changed default jitter time to 50 milliseconds.
 *
 * Revision 1.18  1999/11/06 05:37:45  robertj
 * Complete rewrite of termination of connection to avoid numerous race conditions.
 *
 * Revision 1.17  1999/10/30 12:34:47  robertj
 * Added information callback for closed logical channel on H323EndPoint.
 *
 * Revision 1.16  1999/10/19 00:04:57  robertj
 * Changed OpenAudioChannel and OpenVideoChannel to allow a codec AttachChannel with no autodelete.
 *
 * Revision 1.15  1999/10/14 12:05:03  robertj
 * Fixed deadlock possibilities in clearing calls.
 *
 * Revision 1.14  1999/10/09 01:18:23  craigs
 * Added codecs to OpenAudioChannel and OpenVideoDevice functions
 *
 * Revision 1.13  1999/10/08 08:31:45  robertj
 * Fixed failure to adjust capability when startign channel
 *
 * Revision 1.12  1999/09/23 07:25:12  robertj
 * Added open audio and video function to connection and started multi-frame codec send functionality.
 *
 * Revision 1.11  1999/09/21 14:24:48  robertj
 * Changed SetCapability() so automatically calls AddCapability().
 *
 * Revision 1.10  1999/09/21 14:12:40  robertj
 * Removed warnings when no tracing enabled.
 *
 * Revision 1.9  1999/09/21 08:10:03  craigs
 * Added support for video devices and H261 codec
 *
 * Revision 1.8  1999/09/14 06:52:54  robertj
 * Added better support for multi-homed client hosts.
 *
 * Revision 1.7  1999/09/13 14:23:11  robertj
 * Changed MakeCall() function return value to be something useful.
 *
 * Revision 1.6  1999/09/10 02:55:36  robertj
 * Changed t35 country code to Australia (finally found magic number).
 *
 * Revision 1.5  1999/09/08 04:05:49  robertj
 * Added support for video capabilities & codec, still needs the actual codec itself!
 *
 * Revision 1.4  1999/08/31 12:34:19  robertj
 * Added gatekeeper support.
 *
 * Revision 1.3  1999/08/27 15:42:44  craigs
 * Fixed problem with local call tokens using ambiguous interface names, and connect timeouts not changing connection state
 *
 * Revision 1.2  1999/08/27 09:46:05  robertj
 * Added sepearte function to initialise vendor information from endpoint.
 *
 * Revision 1.1  1999/08/25 05:10:36  robertj
 * File fission (critical mass reached).
 * Improved way in which remote capabilities are created, removed case statement!
 * Changed MakeCall, so immediately spawns thread, no black on TCP connect.
 *
 */

#include <ptlib.h>

#ifdef __GNUC__
#pragma implementation "h323ep.h"
#endif

#include <h323/h323ep.h>

#include <ptclib/random.h>
#include <h323/h323pdu.h>
#include <h323/gkclient.h>


class H225CallThread : public PThread
{
  PCLASSINFO(H225CallThread, PThread)

  public:
    H225CallThread(H323Connection & connection,
                   OpalTransport & transport,
                   const PString & alias,
                   const OpalTransportAddress & address);

  protected:
    void Main();

    H323Connection     & connection;
    OpalTransport      & transport;
    PString              alias;
    OpalTransportAddress address;
};


#define new PNEW

#if !PTRACING // Stuff to remove unised parameters warning
#define PTRACE_isEncoding
#define PTRACE_channel
#endif


/////////////////////////////////////////////////////////////////////////////

H225CallThread::H225CallThread(H323Connection & c,
                               OpalTransport & t,
                               const PString & a,
                               const OpalTransportAddress & addr)
  : PThread(10000,
            NoAutoDeleteThread,
            NormalPriority,
            "H225 Caller:%0x"),
    connection(c),
    transport(t),
    alias(a),
    address(addr)
{
  transport.AttachThread(this);
  Resume();
}


void H225CallThread::Main()
{
  PTRACE(3, "H225\tStarted call thread");

  if (connection.Lock()) {
    OpalCallEndReason reason = connection.SendSignalSetup(alias, address);

    // Special case, if we aborted the call then already will be unlocked
    if (reason != EndedByCallerAbort)
      connection.Unlock();

    // Check if had an error, clear call if so
    if (reason != OpalNumCallEndReasons)
      connection.ClearCall(reason);
    else
      connection.HandleSignallingChannel();
  }
}


/////////////////////////////////////////////////////////////////////////////

H323EndPoint::H323EndPoint(OpalManager & manager)
  : OpalEndPoint(manager, "h323", CanTerminateCall),
    signallingChannelCallTimeout(0, 0, 1),  // Minutes
    masterSlaveDeterminationTimeout(0, 30), // Seconds
    capabilityExchangeTimeout(0, 30),       // Seconds
    logicalChannelTimeout(0, 30),           // Seconds
    requestModeTimeout(0, 30),              // Seconds
    roundTripDelayTimeout(0, 10),           // Seconds
    roundTripDelayRate(0, 0, 1),            // Minutes
    gatekeeperRequestTimeout(0, 5),         // Seconds
    rasRequestTimeout(0, 3)                 // Seconds
{
  // Set port in OpalEndPoint class
  defaultSignalPort = DefaultSignalTcpPort;

  PString username = PProcess::Current().GetUserName();
  if (username.IsEmpty())
    username = PProcess::Current().GetName() & "User";
  localAliasNames.AppendString(username);

  disableFastStart = FALSE;
  disableH245Tunneling = FALSE;

  terminalType = e_TerminalOnly;
  initialBandwidth = 100000; // Standard 10base LAN in 100's of bits/sec
  clearCallOnRoundTripFail = FALSE;

  t35CountryCode = 9; // Country code for Australia
  t35Extension = 0;
  manufacturerCode = 61; // Allocated by Australian Communications Authority, Oct 2000

  masterSlaveDeterminationRetries = 10;
  gatekeeperRequestRetries = 2;
  rasRequestRetries = 2;

  gatekeeper = NULL;

  PTRACE(3, "H323\tCreated endpoint.");
}


H323EndPoint::~H323EndPoint()
{
  // And shut down the gatekeeper (if there was one)
  RemoveGatekeeper();

  // Shut down the listeners as soon as possible to avoid race conditions
  listeners.RemoveAll();

  PTRACE(3, "H323\tDeleted endpoint.");
}


void H323EndPoint::SetEndpointTypeInfo(H225_EndpointType & info) const
{
  info.IncludeOptionalField(H225_EndpointType::e_vendor);
  SetVendorIdentifierInfo(info.m_vendor);

  switch (terminalType) {
    case e_TerminalOnly :
    case e_TerminalAndMC :
      info.IncludeOptionalField(H225_EndpointType::e_terminal);
      break;
    case e_GatewayOnly :
    case e_GatewayAndMC :
    case e_GatewayAndMCWithDataMP :
    case e_GatewayAndMCWithAudioMP :
    case e_GatewayAndMCWithAVMP :
      info.IncludeOptionalField(H225_EndpointType::e_gateway);
      break;
    case e_GatekeeperOnly :
    case e_GatekeeperWithDataMP :
    case e_GatekeeperWithAudioMP :
    case e_GatekeeperWithAVMP :
      info.IncludeOptionalField(H225_EndpointType::e_gatekeeper);
      break;
    case e_MCUOnly :
    case e_MCUWithDataMP :
    case e_MCUWithAudioMP :
    case e_MCUWithAVMP :
      info.IncludeOptionalField(H225_EndpointType::e_mcu);
      info.m_mc = TRUE;
  }
}


void H323EndPoint::SetVendorIdentifierInfo(H225_VendorIdentifier & info) const
{
  SetH221NonStandardInfo(info.m_vendor);

  info.IncludeOptionalField(H225_VendorIdentifier::e_productId);
  info.m_productId = PProcess::Current().GetManufacturer() & PProcess::Current().GetName();
  info.m_productId.SetSize(info.m_productId.GetSize()+2);

  info.IncludeOptionalField(H225_VendorIdentifier::e_versionId);
  info.m_versionId = PProcess::Current().GetVersion(TRUE);
  info.m_versionId.SetSize(info.m_versionId.GetSize()+2);
}


void H323EndPoint::SetH221NonStandardInfo(H225_H221NonStandard & info) const
{
  info.m_t35CountryCode = t35CountryCode;
  info.m_t35Extension = t35Extension;
  info.m_manufacturerCode = manufacturerCode;
}


H323Capability * H323EndPoint::FindCapability(const H245_Capability & cap) const
{
  return capabilities.FindCapability(cap);
}


H323Capability * H323EndPoint::FindCapability(const H245_DataType & dataType) const
{
  return capabilities.FindCapability(dataType);
}


H323Capability * H323EndPoint::FindCapability(H323Capability::MainTypes mainType,
                                              unsigned subType) const
{
  return capabilities.FindCapability(mainType, subType);
}


void H323EndPoint::AddCapability(H323Capability * capability)
{
  capabilities.Add(capability);
}


PINDEX H323EndPoint::SetCapability(PINDEX descriptorNum,
                                   PINDEX simultaneousNum,
                                   H323Capability * capability)
{
  return capabilities.SetCapability(descriptorNum, simultaneousNum, capability);
}


PINDEX H323EndPoint::AddAllCapabilities(PINDEX descriptorNum,
                                        PINDEX simultaneous,
                                        const PString & name)
{
  return capabilities.AddAllCapabilities(*this, descriptorNum, simultaneous, name);
}


void H323EndPoint::AddAllUserInputCapabilities(PINDEX descriptorNum,
                                               PINDEX simultaneous)
{
  H323_UserInputCapability::AddAllCapabilities(capabilities, descriptorNum, simultaneous);
}


void H323EndPoint::RemoveCapabilities(const PStringArray & codecNames)
{
  capabilities.Remove(codecNames);
}


void H323EndPoint::ReorderCapabilities(const PStringArray & preferenceOrder)
{
  capabilities.Reorder(preferenceOrder);
}


BOOL H323EndPoint::SetGatekeeper(const PString & address, OpalTransport * transport)
{
  H323Gatekeeper * gk = InternalCreateGatekeeper(transport);
  return InternalRegisterGatekeeper(gk, gk->DiscoverByAddress(address));
}


BOOL H323EndPoint::SetGatekeeperZone(const PString & address,
                                     const PString & identifier,
                                     OpalTransport * transport)
{
  H323Gatekeeper * gk = InternalCreateGatekeeper(transport);
  return InternalRegisterGatekeeper(gk, gk->DiscoverByNameAndAddress(identifier, address));
}


BOOL H323EndPoint::LocateGatekeeper(const PString & identifier, OpalTransport * transport)
{
  H323Gatekeeper * gk = InternalCreateGatekeeper(transport);
  return InternalRegisterGatekeeper(gk, gk->DiscoverByName(identifier));
}


BOOL H323EndPoint::DiscoverGatekeeper(OpalTransport * transport)
{
  H323Gatekeeper * gk = InternalCreateGatekeeper(transport);
  return InternalRegisterGatekeeper(gk, gk->DiscoverAny());
}


H323Gatekeeper * H323EndPoint::InternalCreateGatekeeper(OpalTransport * transport)
{
  RemoveGatekeeper(H225_UnregRequestReason::e_reregistrationRequired);

  if (transport == NULL)
    transport = new OpalTransportUDP(*this);

  return CreateGatekeeper(transport);
}


BOOL H323EndPoint::InternalRegisterGatekeeper(H323Gatekeeper * gk, BOOL discovered)
{
  if (discovered) {
    if (gk->RegistrationRequest()) {
      gatekeeper = gk;
      return TRUE;
    }
    else  // RRQ was rejected continue listening
      gatekeeper = gk;
  }
  else // Only stop listening if the GRQ was rejected
    delete gk;

  return FALSE;
}


H323Gatekeeper * H323EndPoint::CreateGatekeeper(OpalTransport * transport)
{
  return new H323Gatekeeper(*this, transport);
}


BOOL H323EndPoint::RemoveGatekeeper(int reason)
{
  BOOL ok = TRUE;

  if (gatekeeper == NULL)
    return ok;

  ClearAllCalls();

  if (gatekeeper->IsRegistered()) // If we are registered send a URQ
    ok = gatekeeper->UnregistrationRequest(reason);

  delete gatekeeper;
  gatekeeper = NULL;

  return ok;
}


OpalConnection * H323EndPoint::SetUpConnection(OpalCall & call,
                                               const PString & remoteParty,
                                               void * userData)
{
  PTRACE(2, "H323\tMaking call to: " << remoteParty);
  return InternalMakeCall(call, remoteParty, PString(), PString(), userData);
}


void H323EndPoint::NewIncomingConnection(OpalTransport * transport)
{
  PTRACE(3, "H225\tAwaiting first PDU");
  transport->SetReadTimeout(15000); // Await 15 seconds after connect for first byte
  H323SignalPDU pdu;
  if (!pdu.Read(*transport)) {
    PTRACE(1, "H225\tFailed to get initial Q.931 PDU, connection not started.");
    return;
  }

  unsigned callReference = pdu.GetQ931().GetCallReference();
  PTRACE(3, "H225\tIncoming call, first PDU: callReference=" << callReference);

  // Get a new (or old) connection from the endpoint, calculate token
  PString token = transport->GetRemoteAddress();
  token.sprintf("/%u", pdu.GetQ931().GetCallReference());

  H323Connection * connection;

  inUseFlag.Wait();

  if (connectionsActive.Contains(token))
    connection = (H323Connection *)&connectionsActive[token];
  else {
    connection = CreateConnection(*manager.CreateCall(), token, NULL, transport, &pdu);
    connectionsActive.SetAt(token, connection);
  }

  inUseFlag.Signal();

  if (connection == NULL) {
    PTRACE(1, "H225\tEndpoint could not create connection, "
              "sending release complete PDU: callRef=" << callReference);
    Q931 reply;
    reply.BuildReleaseComplete(callReference, TRUE);
    PBYTEArray rawData;
    reply.Encode(rawData);
    transport->WritePDU(rawData);
  }
  else {
    PTRACE(3, "H323\tCreated new connection: " << token);
    connection->AttachSignalChannel(transport, TRUE);

    if (connection->HandleSignalPDU(pdu)) {
      // All subsequent PDU's should wait forever
      transport->SetReadTimeout(PMaxTimeInterval);
      connection->HandleSignallingChannel();
    }
    else {
      connection->ClearCall(EndedByTransportFail);
      PTRACE(1, "H225\tSignal channel stopped on first PDU.");
    }
  }
}


H323Connection * H323EndPoint::CreateConnection(OpalCall & call,
                                                const PString & token,
                                                void * /*userData*/,
                                                OpalTransport * /*transport*/,
                                                H323SignalPDU * /*setupPDU*/)
{
  return new H323Connection(call, *this, token);
}


H323Connection * H323EndPoint::SetupTransfer(const PString & oldToken,
                                             const PString & callIdentity,
                                             const PString & remoteParty)
{
  PTRACE(2, "H323\tTransferring call to: " << remoteParty);
  return InternalMakeCall(*manager.CreateCall(), oldToken, callIdentity, remoteParty, NULL);
}


H323Connection * H323EndPoint::InternalMakeCall(OpalCall & call,
                                                const PString & existingToken,
                                                const PString & callIdentity,
                                                const PString & remoteParty,
                                                void * userData)
{
  PString alias;
  OpalTransportAddress address;
  ParsePartyName(remoteParty, alias, address);

  // Restriction: the call must be made on the same transport as the one
  // that the gatekeeper is using.
  OpalTransport * transport;
  if (gatekeeper != NULL)
    transport = gatekeeper->GetTransport().GetLocalAddress().CreateTransport(
                                          *this, OpalTransportAddress::HostOnly);
  else
    transport = address.CreateTransport(*this, OpalTransportAddress::NoBinding);

  if (transport == NULL) {
    PTRACE(1, "H323\tInvalid transport in \"" << remoteParty << '"');
    return NULL;
  }

  inUseFlag.Wait();

  PString newToken;
  newToken.sprintf("localhost/%u", Q931::GenerateCallReference());

  H323Connection * connection = CreateConnection(call, newToken, userData, transport, NULL);
  connectionsActive.SetAt(newToken, connection);

  inUseFlag.Signal();

  if (connection == NULL) {
    PTRACE(1, "H225\tEndpoint could not create connection, aborting setup.");
    return NULL;
  }

  connection->Lock();
  connection->AttachSignalChannel(transport, FALSE);
  connection->HandleTransferCall(existingToken, callIdentity);

  PTRACE(3, "H323\tCreated new connection: " << newToken);

  new H225CallThread(*connection, *transport, alias, address);
  return connection;
}


void H323EndPoint::TransferCall(const PString & token, const PString & remoteParty)
{
  H323Connection * connection = FindConnectionWithLock(token);
  if (connection != NULL) {
    connection->TransferCall(remoteParty);
    connection->Unlock();
  }
}


void H323EndPoint::HoldCall(const PString & token, BOOL localHold)
{
  H323Connection * connection = FindConnectionWithLock(token);
  if (connection != NULL) {
    connection->HoldCall(localHold);
    connection->Unlock();
  }
}


void H323EndPoint::ParsePartyName(const PString & remoteParty,
                                  PString & alias,
                                  OpalTransportAddress & address) const
{
  /*Determine the alias part and the address part of the remote party name
    string. This depends on if there is an '@' to separate the alias from the
    transport address or a '$' for if it is explicitly a transport address.
   */

  PINDEX colon = remoteParty.Find(':');
  if (colon == P_MAX_INDEX || prefixName != remoteParty.Left(colon))
    colon = 0;
  else
    colon++;

  PString gateway;
  PINDEX at = remoteParty.Find('@', colon);
  if (at != P_MAX_INDEX) {
    alias = remoteParty(colon, at-1);
    gateway = remoteParty.Mid(at+1);
  }
  else if (gatekeeper != NULL)
    alias = remoteParty.Mid(colon);
  else {
    alias = "";
    gateway = remoteParty.Mid(colon);
  }

  if (gateway.IsEmpty())
    address = OpalTransportAddress();
  else
    address = OpalTransportAddress(gateway, DefaultSignalTcpPort);
}


H323Connection * H323EndPoint::FindConnectionWithLock(const PString & token)
{
  PWaitAndSignal mutex(inUseFlag);

  /*We have a very yucky polling loop here as a semi permanant measure.
    Why? We cannot call Lock() inside the inUseFlag critical section as
    it will cause a deadlock with something like a RELEASE-COMPLETE coming in
    on separate thread. But if we put it outside there is a small window where
    the connection could get deleted before the Lock() test is done.
    The solution is to attempt to get the mutex while inside the
    inUseFlag but not block. That means a polling loop. There is
    probably a way to do this properly with mutexes but I don't have time to
    figure it out.
   */
  H323Connection * connection;
  while ((connection = FindConnectionWithoutLocks(token)) != NULL) {
    switch (connection->TryLock()) {
      case 0 :
        return NULL;
      case 1 :
        return connection;
    }
    // Could not get connection lock, unlock the endpoint lists so a thread
    // that has the connection lock gets a chance at the endpoint lists.
    inUseFlag.Signal();
    PThread::Yield();
    inUseFlag.Wait();
  }

  return NULL;
}


H323Connection * H323EndPoint::FindConnectionWithoutLocks(const PString & token)
{
  PWaitAndSignal wait(inUseFlag);

  OpalConnection * conn_ptr = connectionsActive.GetAt(token);
  if (conn_ptr != NULL && conn_ptr->IsDescendant(H323Connection::Class()))
    return (H323Connection *)conn_ptr;

  PINDEX i;
  for (i = 0; i < connectionsActive.GetSize(); i++) {
    H323Connection & conn = (H323Connection &)connectionsActive.GetDataAt(i);
    if (conn.GetCallIdentifier().AsString() == token)
      return &conn;
  }

  for (i = 0; i < connectionsActive.GetSize(); i++) {
    H323Connection & conn = (H323Connection &)connectionsActive.GetDataAt(i);
    if (conn.GetConferenceIdentifier().AsString() == token)
      return &conn;
  }

  return NULL;
}


BOOL H323EndPoint::OnIncomingCall(H323Connection & connection,
                                  const H323SignalPDU & /*setupPDU*/,
                                  H323SignalPDU & /*alertingPDU*/)
{
  return connection.OnIncomingConnection();
}


BOOL H323EndPoint::OnCallTransferInitiate(H323Connection & /*connection*/,
                                          const PString & /*remoteParty*/)
{
  return TRUE;
}


H323Connection::AnswerCallResponse
       H323EndPoint::OnAnswerCall(H323Connection & /*connection*/,
                                  const PString & /*caller*/,
                                  const H323SignalPDU & /*setupPDU*/,
                                  H323SignalPDU & /*connectPDU*/)
{
  return H323Connection::NumAnswerCallResponses;
}


BOOL H323EndPoint::OnAlerting(H323Connection & connection,
                              const H323SignalPDU & /*alertingPDU*/,
                              const PString & /*username*/)
{
  PTRACE(1, "H225\tReceived alerting PDU.");
  ((OpalConnection&)connection).OnAlerting();
  return TRUE;
}


BOOL H323EndPoint::OnConnectionForwarded(H323Connection & /*connection*/,
                                         const PString & /*forwardParty*/,
                                         const H323SignalPDU & /*pdu*/)
{
  return FALSE;
}


void H323EndPoint::OnConnectionEstablished(H323Connection & /*connection*/,
                                           const PString & /*token*/)
{
}


BOOL H323EndPoint::IsConnectionEstablished(const PString & token)
{
  H323Connection * connection = FindConnectionWithLock(token);
  if (connection == NULL)
    return FALSE;

  BOOL established = connection->IsEstablished();
  connection->Unlock();
  return established;
}


void H323EndPoint::OnConnectionCleared(H323Connection & /*connection*/,
                                       const PString & /*token*/)
{
}


#if PTRACING
static void OnStartStopChannel(const char * startstop, const H323Channel & channel)
{
  const char * dir;
  switch (channel.GetDirection()) {
    case H323Channel::IsTransmitter :
      dir = "send";
      break;

    case H323Channel::IsReceiver :
      dir = "receiv";
      break;

    default :
      dir = "us";
      break;
  }

  PTRACE(2, "H323\t" << startstop << "ed "
                     << dir << "ing logical channel: "
                     << channel.GetCapability());
}
#endif


BOOL H323EndPoint::OnStartLogicalChannel(H323Connection & /*connection*/,
                                         H323Channel & PTRACE_channel)
{
#if PTRACING
  OnStartStopChannel("Start", PTRACE_channel);
#endif
  return TRUE;
}


void H323EndPoint::OnClosedLogicalChannel(H323Connection & /*connection*/,
                                          const H323Channel & PTRACE_channel)
{
#if PTRACING
  OnStartStopChannel("Stopp", PTRACE_channel);
#endif
}


void H323EndPoint::OnRTPStatistics(const H323Connection & /*connection*/,
                                   const RTP_Session & /*session*/) const
{
}


void H323EndPoint::OnUserInputString(H323Connection & connection,
                                     const PString & value)
{
  connection.OnUserIndicationString(value);
}


void H323EndPoint::OnUserInputTone(H323Connection & connection,
                                   char tone,
                                   unsigned duration,
                                   unsigned /*logicalChannel*/,
                                   unsigned /*rtpTimestamp*/)
{
  connection.OnUserIndicationTone(tone, duration);
}


OpalT120Protocol * H323EndPoint::CreateT120ProtocolHandler() const
{
  return NULL;
}


OpalT38Protocol * H323EndPoint::CreateT38ProtocolHandler() const
{
  return NULL;
}


void H323EndPoint::SetLocalUserName(const PString & name)
{
  PAssert(!name, "Must have non-empty string in AliasAddress!");
  if (name.IsEmpty())
    return;

  localAliasNames.RemoveAll();
  localAliasNames.AppendString(name);
}


BOOL H323EndPoint::AddAliasName(const PString & name)
{
  PAssert(!name, "Must have non-empty string in AliasAddress!");

  if (localAliasNames.GetValuesIndex(name) != P_MAX_INDEX)
    return FALSE;

  localAliasNames.AppendString(name);
  return TRUE;
}


BOOL H323EndPoint::RemoveAliasName(const PString & name)
{
  PINDEX pos = localAliasNames.GetValuesIndex(name);
  if (pos == P_MAX_INDEX)
    return FALSE;

  PAssert(localAliasNames.GetSize() > 1, "Must have at least one AliasAddress!");
  if (localAliasNames.GetSize() < 2)
    return FALSE;

  localAliasNames.RemoveAt(pos);
  return TRUE;
}


BOOL H323EndPoint::IsTerminal() const
{
  switch (terminalType) {
    case e_TerminalOnly :
    case e_TerminalAndMC :
      return TRUE;

    default :
      return FALSE;
  }
}


BOOL H323EndPoint::IsGateway() const
{
  switch (terminalType) {
    case e_GatewayOnly :
    case e_GatewayAndMC :
    case e_GatewayAndMCWithDataMP :
    case e_GatewayAndMCWithAudioMP :
    case e_GatewayAndMCWithAVMP :
      return TRUE;

    default :
      return FALSE;
  }
}


BOOL H323EndPoint::IsGatekeeper() const
{
  switch (terminalType) {
    case e_GatekeeperOnly :
    case e_GatekeeperWithDataMP :
    case e_GatekeeperWithAudioMP :
    case e_GatekeeperWithAVMP :
      return TRUE;

    default :
      return FALSE;
  }
}


BOOL H323EndPoint::IsMCU() const
{
  switch (terminalType) {
    case e_MCUOnly :
    case e_MCUWithDataMP :
    case e_MCUWithAudioMP :
    case e_MCUWithAVMP :
      return TRUE;

    default :
      return FALSE;
  }
}


/////////////////////////////////////////////////////////////////////////////
