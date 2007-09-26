/*
 * manager.cxx
 *
 * Media channels abstraction
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
 * $Log: manager.cxx,v $
 * Revision 1.2100  2007/09/26 03:50:13  rjongbloed
 * Allow for SDL only being able to show one window, prevent
 *   preview window from appearing if SDL used for both.
 *
 * Revision 2.98  2007/09/21 01:34:09  rjongbloed
 * Rewrite of SIP transaction handling to:
 *   a) use PSafeObject and safe collections
 *   b) only one database of transactions, remove connection copy
 *   c) fix timers not always firing due to bogus deadlock avoidance
 *   d) cleaning up only occurs in the existing garbage collection thread
 *   e) use of read/write mutex on endpoint list to avoid possible deadlock
 *
 * Revision 2.97  2007/09/18 09:37:52  rjongbloed
 * Propagated call backs for RTP statistics through OpalManager and OpalCall.
 *
 * Revision 2.96  2007/09/18 02:24:24  rjongbloed
 * Set window title to connection remote party name if video output device has one.
 *
 * Revision 2.95  2007/09/11 13:42:18  csoutheren
 * Add logging for IP address translation
 *
 * Revision 2.94  2007/08/01 23:56:54  csoutheren
 * Add extra logging
 *
 * Revision 2.93  2007/07/22 03:22:11  rjongbloed
 * Added INADDR_ANY to addressess that qualify as "local" for NAT purposes.
 *
 * Revision 2.92  2007/07/20 05:49:58  rjongbloed
 * Added member variable for stun server name in OpalManager, so can be remembered
 *   in original form so change in IP address or temporary STUN server failures do not
 *   lose the server being selected by the user.
 *
 * Revision 2.91  2007/06/29 06:59:57  rjongbloed
 * Major improvement to the "product info", normalising H.221 and User-Agent mechanisms.
 *
 * Revision 2.90  2007/06/29 02:49:42  rjongbloed
 * Added PString::FindSpan() function (strspn equivalent) with slightly nicer semantics.
 *
 * Revision 2.89  2007/05/15 07:27:34  csoutheren
 * Remove deprecated  interface to STUN server in H323Endpoint
 * Change UseNATForIncomingCall to IsRTPNATEnabled
 * Various cleanups of messy and unused code
 *
 * Revision 2.88  2007/05/15 05:25:34  csoutheren
 * Add UseNATForIncomingCall override so applications can optionally implement their own NAT activation strategy
 *
 * Revision 2.87  2007/05/10 05:01:18  csoutheren
 * Increase default number of RTP ports because sometimes, 100 ports is not enough :)
 *
 * Revision 2.86  2007/05/07 14:14:31  csoutheren
 * Add call record capability
 *
 * Revision 2.85  2007/04/18 00:01:05  csoutheren
 * Add hooks for recording call audio
 *
 * Revision 2.84  2007/04/13 07:25:24  rjongbloed
 * Changed hard coded numbers for video frame size to use symbols.
 *
 * Revision 2.83  2007/04/10 05:15:54  rjongbloed
 * Fixed issue with use of static C string variables in DLL environment,
 *   must use functional interface for correct initialisation.
 *
 * Revision 2.82  2007/04/05 02:16:39  rjongbloed
 * Fixed validation of video devices set on OpalManager, especially in regard to video file driver.
 *
 * Revision 2.81  2007/04/04 02:12:01  rjongbloed
 * Reviewed and adjusted PTRACE log levels
 *   Now follows 1=error,2=warn,3=info,4+=debug
 *
 * Revision 2.80  2007/04/03 13:04:24  rjongbloed
 * Added driverName to PVideoDevice::OpenArgs (so can select YUVFile)
 * Added new statics to create correct video input/output device object
 *   given a PVideoDevice::OpenArgs structure.
 * Fixed incorrect initialisation of default video input device.
 * Fixed last calls video size changing the default value.
 *
 * Revision 2.79  2007/04/03 07:13:14  rjongbloed
 * Fixed SetUpCall() so actually passes userData to CreateCall()
 *
 * Revision 2.78  2007/04/02 05:51:33  rjongbloed
 * Tidied some trace logs to assure all have a category (bit before a tab character) set.
 *
 * Revision 2.77  2007/03/21 16:10:46  hfriederich
 * Use CallEndReason from connection if call setup fails
 *
 * Revision 2.76  2007/03/13 00:33:11  csoutheren
 * Simple but messy changes to allow compile time removal of protocol
 * options such as H.450 and H.460
 * Fix MakeConnection overrides
 *
 * Revision 2.75  2007/03/01 06:16:54  rjongbloed
 * Fixed DevStudio 2005 warning
 *
 * Revision 2.74  2007/03/01 05:51:05  rjongbloed
 * Fixed backward compatibility of OnIncomingConnection() virtual
 *   functions on various classes. If an old override returned FALSE
 *   then it will now abort the call as it used to.
 *
 * Revision 2.73  2007/03/01 03:42:09  csoutheren
 * Don't use yuvfile as a default video device
 *
 * Revision 2.72  2007/01/25 11:48:11  hfriederich
 * OpalMediaPatch code refactorization.
 * Split into OpalMediaPatch (using a thread) and OpalPassiveMediaPatch
 * (not using a thread). Also adds the possibility for source streams
 * to push frames down to the sink streams instead of having a patch
 * thread around.
 *
 * Revision 2.71  2007/01/25 03:30:10  csoutheren
 * Fix problem with calling OpalConnection::OnIncomingConnection twice
 *
 * Revision 2.70  2007/01/24 04:00:57  csoutheren
 * Arrrghh. Changing OnIncomingConnection turned out to have a lot of side-effects
 * Added some pure viritual functions to prevent old code from breaking silently
 * New OpalEndpoint and OpalConnection descendants will need to re-implement
 * OnIncomingConnection. Sorry :)
 *
 * Revision 2.69  2007/01/24 00:28:28  csoutheren
 * Fixed overrides of OnIncomingConnection
 *
 * Revision 2.68  2007/01/18 04:45:17  csoutheren
 * Messy, but simple change to add additional options argument to OpalConnection constructor
 * This allows the provision of non-trivial arguments for connections
 *
 * Revision 2.67  2006/12/18 03:18:42  csoutheren
 * Messy but simple fixes
 *   - Add access to SIP REGISTER timeout
 *   - Ensure OpalConnection options are correctly progagated
 *
 * Revision 2.66  2006/11/20 03:37:12  csoutheren
 * Allow optional inclusion of RTP aggregation
 *
 * Revision 2.65  2006/11/19 06:02:58  rjongbloed
 * Moved function that reads User Input into a destination address to
 *   OpalManager so can be easily overidden in applications.
 *
 * Revision 2.64  2006/10/15 06:23:35  rjongbloed
 * Fixed the mechanism where both A-party and B-party are indicated by the application. This now works
 *   for LIDs as well as PC endpoint, wheich is the only one that was used before.
 *
 * Revision 2.63  2006/10/10 07:18:18  csoutheren
 * Allow compilation with and without various options
 *
 * Revision 2.62  2006/09/28 07:42:18  csoutheren
 * Merge of useful SRTP implementation
 *
 * Revision 2.61  2006/08/10 05:10:33  csoutheren
 * Various H.323 stability patches merged in from DeimosPrePLuginBranch
 *
 * Revision 2.60.2.1  2006/08/09 12:49:22  csoutheren
 * Improve stablity under heavy H.323 load
 *
 * Revision 2.60  2006/06/30 06:49:31  csoutheren
 * Fixed disable of video code to use correct #define
 *
 * Revision 2.59  2006/06/30 01:05:51  csoutheren
 * Applied 1509203 - Fix compilation when video is disabled
 * Thanks to Boris Pavacic
 *
 * Revision 2.58  2006/04/20 16:52:22  hfriederich
 * Adding support for H.224/H.281
 *
 * Revision 2.57  2006/03/20 10:37:47  csoutheren
 * Applied patch #1453753 - added locking on media stream manipulation
 * Thanks to Dinis Rosario
 *
 * Revision 2.56  2005/11/25 02:42:32  csoutheren
 * Applied patch #1358862 ] by Frederich Heem
 * SetUp call not removing calls in case of error
 *
 * Revision 2.55  2005/10/08 19:26:38  dsandras
 * Added OnForwarded callback in case of call forwarding.
 *
 * Revision 2.54  2005/09/20 07:25:43  csoutheren
 * Allow userdata to be passed from SetupCall through to Call and Connection constructors
 *
 * Revision 2.53  2005/08/28 07:59:17  rjongbloed
 * Converted OpalTranscoder to use factory, requiring sme changes in making sure
 *   OpalMediaFormat instances are initialised before use.
 *
 * Revision 2.52  2005/08/24 10:43:51  rjongbloed
 * Changed create video functions yet again so can return pointers that are not to be deleted.
 *
 * Revision 2.51  2005/08/23 12:45:09  rjongbloed
 * Fixed creation of video preview window and setting video grab/display initial frame size.
 *
 * Revision 2.50  2005/08/11 22:45:13  rjongbloed
 * Fixed incorrect test for auto start receive video
 *
 * Revision 2.49  2005/08/09 09:20:20  rjongbloed
 * Utilise new video driver/device name interface to select default video grabber/display.
 *
 * Revision 2.48  2005/07/14 08:51:19  csoutheren
 * Removed CreateExternalRTPAddress - it's not needed because you can override GetMediaAddress
 * to do the same thing
 * Fixed problems with logic associated with media bypass
 *
 * Revision 2.47  2005/07/11 06:52:17  csoutheren
 * Added support for outgoing calls using external RTP
 *
 * Revision 2.46  2005/07/11 01:52:26  csoutheren
 * Extended AnsweringCall to work for SIP as well as H.323
 * Fixed problems with external RTP connection in H.323
 * Added call to OnClosedMediaStream
 *
 * Revision 2.45  2005/07/09 06:59:28  rjongbloed
 * Changed SetSTUNServer so returns the determined NAT type.
 * Fixed SetSTUNServer so does not try and get external router address is have blocking NAT,
 *   just takes up more time!
 * Fixed possible deadlock on exiting program, waiting for garbage collector to finish.
 *
 * Revision 2.44  2005/06/02 13:18:02  rjongbloed
 * Fixed compiler warnings
 *
 * Revision 2.43  2005/05/25 17:04:07  dsandras
 * Fixed ClearAllCalls when being executed synchronously. Fixes a crash on exit when a call is in progress. Thanks Robert!
 *
 * Revision 2.42  2005/05/23 21:22:04  dsandras
 * Wait on the mutex for calls missed by the garbage collector.
 *
 * Revision 2.41  2005/04/10 21:15:08  dsandras
 * Added callback that is called when a connection is put on hold (local or remote).
 *
 * Revision 2.40  2005/02/21 12:19:55  rjongbloed
 * Added new "options list" to the OpalMediaFormat class.
 *
 * Revision 2.39  2004/10/16 03:10:04  rjongbloed
 * Fixed correct detection of when to use STUN, this does not include the local host!
 *
 * Revision 2.38  2004/10/03 15:16:04  rjongbloed
 * Removed no trace warning
 *
 * Revision 2.37  2004/08/18 13:03:23  rjongbloed
 * Changed to make calling OPalManager::OnClearedCall() in override optional.
 *
 * Revision 2.36  2004/08/14 07:56:41  rjongbloed
 * Major revision to utilise the PSafeCollection classes for the connections and calls.
 *
 * Revision 2.35  2004/07/14 13:26:14  rjongbloed
 * Fixed issues with the propagation of the "established" phase of a call. Now
 *   calling an OnEstablished() chain like OnAlerting() and OnConnected() to
 *   finally arrive at OnEstablishedCall() on OpalManager
 *
 * Revision 2.34  2004/07/11 12:42:13  rjongbloed
 * Added function on endpoints to get the list of all media formats any
 *   connection the endpoint may create can support.
 *
 * Revision 2.33  2004/05/15 12:53:03  rjongbloed
 * Added default username and display name to manager, for all endpoints.
 *
 * Revision 2.32  2004/05/01 10:00:53  rjongbloed
 * Fixed ClearCallSynchronous so now is actually signalled when call is destroyed.
 *
 * Revision 2.31  2004/04/29 11:48:32  rjongbloed
 * Fixed possible deadlock if close all and close synchronous.
 *
 * Revision 2.30  2004/04/25 02:53:29  rjongbloed
 * Fixed GNU 3.4 warnings
 *
 * Revision 2.29  2004/04/18 13:35:28  rjongbloed
 * Fixed ability to make calls where both endpoints are specified a priori. In particular
 *   fixing the VXML support for an outgoing sip/h323 call.
 *
 * Revision 2.28  2004/04/18 07:09:12  rjongbloed
 * Added a couple more API functions to bring OPAL into line with similar functions in OpenH323.
 *
 * Revision 2.27  2004/02/24 11:37:02  rjongbloed
 * More work on NAT support, manual external address translation and STUN
 *
 * Revision 2.26  2004/02/19 10:47:06  rjongbloed
 * Merged OpenH323 version 1.13.1 changes.
 *
 * Revision 2.25  2004/01/18 15:36:07  rjongbloed
 * Added stun support
 *
 * Revision 2.24  2003/12/15 11:43:07  rjongbloed
 * Updated to new API due to plug-ins
 *
 * Revision 2.23  2003/03/17 10:27:00  robertj
 * Added video support.
 *
 * Revision 2.22  2003/03/07 08:13:42  robertj
 * Fixed validation of "protocol:" part of address in routing system.
 *
 * Revision 2.21  2003/03/06 03:57:47  robertj
 * IVR support (work in progress) requiring large changes everywhere.
 *
 * Revision 2.20  2003/01/07 04:39:53  robertj
 * Updated to OpenH323 v1.11.2
 *
 * Revision 2.19  2002/11/10 11:33:19  robertj
 * Updated to OpenH323 v1.10.3
 *
 * Revision 2.18  2002/09/06 02:44:30  robertj
 * Added routing table system to route calls by regular expressions.
 *
 * Revision 2.17  2002/09/04 06:01:49  robertj
 * Updated to OpenH323 v1.9.6
 *
 * Revision 2.16  2002/07/01 04:56:33  robertj
 * Updated to OpenH323 v1.9.1
 *
 * Revision 2.15  2002/06/16 02:25:11  robertj
 * Fixed memory leak of garbage collector, thanks Ted Szoczei
 *
 * Revision 2.14  2002/04/17 07:19:38  robertj
 * Fixed incorrect trace message for OnReleased
 *
 * Revision 2.13  2002/04/10 08:15:31  robertj
 * Added functions to set ports.
 *
 * Revision 2.12  2002/04/08 02:40:13  robertj
 * Fixed issues with using double originate call, eg from simple app command line.
 *
 * Revision 2.11  2002/02/19 07:49:47  robertj
 * Restructured media bypass functions to fix problems with RFC2833.
 *
 * Revision 2.10  2002/02/11 07:42:17  robertj
 * Added media bypass for streams between compatible protocols.
 *
 * Revision 2.9  2002/01/22 05:12:51  robertj
 * Revamp of user input API triggered by RFC2833 support
 *
 * Revision 2.8  2002/01/14 06:35:58  robertj
 * Updated to OpenH323 v1.7.9
 *
 * Revision 2.7  2001/12/07 08:56:43  robertj
 * Renamed RTP to be more general UDP port base, and TCP to be IP.
 *
 * Revision 2.6  2001/11/14 01:31:55  robertj
 * Corrected placement of adjusting media format list.
 *
 * Revision 2.5  2001/11/13 06:25:56  robertj
 * Changed SetUpConnection() so returns BOOL as returning
 *   pointer to connection is not useful.
 *
 * Revision 2.4  2001/10/04 00:44:26  robertj
 * Added function to remove wildcard from list.
 *
 * Revision 2.3  2001/08/23 05:51:17  robertj
 * Completed implementation of codec reordering.
 *
 * Revision 2.2  2001/08/17 08:25:41  robertj
 * Added call end reason for whole call, not just connection.
 * Moved call end reasons enum from OpalConnection to global.
 *
 * Revision 2.1  2001/08/01 05:44:41  robertj
 * Added function to get all media formats possible in a call.
 *
 * Revision 2.0  2001/07/27 15:48:25  robertj
 * Conversion of OpenH323 to Open Phone Abstraction Library (OPAL)
 *
 */

#include <ptlib.h>

#ifdef __GNUC__
#pragma implementation "manager.h"
#endif

#include <opal/manager.h>

#include <opal/endpoint.h>
#include <opal/call.h>
#include <opal/patch.h>
#include <opal/mediastrm.h>

#if OPAL_VIDEO
#include <codec/vidcodec.h>
#endif

#include <h224/h224handler.h>
#include <h224/h281handler.h>

#include "../../version.h"


#ifndef IPTOS_PREC_CRITIC_ECP
#define IPTOS_PREC_CRITIC_ECP (5 << 5)
#endif

#ifndef IPTOS_LOWDELAY
#define IPTOS_LOWDELAY 0x10
#endif


static const char * const DefaultMediaFormatOrder[] = {
  OPAL_G7231_6k3,
  OPAL_G729B,
  OPAL_G729AB,
  OPAL_G729,
  OPAL_G729A,
  OPAL_GSM0610,
  OPAL_G728,
  OPAL_G711_ULAW_64K,
  OPAL_G711_ALAW_64K
};

#define new PNEW


/////////////////////////////////////////////////////////////////////////////

PString OpalGetVersion()
{
#define AlphaCode   "alpha"
#define BetaCode    "beta"
#define ReleaseCode "."

  return psprintf("%u.%u%s%u", MAJOR_VERSION, MINOR_VERSION, BUILD_TYPE, BUILD_NUMBER);
}


unsigned OpalGetMajorVersion()
{
  return MAJOR_VERSION;
}

unsigned OpalGetMinorVersion()
{
  return MINOR_VERSION;
}

unsigned OpalGetBuildNumber()
{
  return BUILD_NUMBER;
}


OpalProductInfo::OpalProductInfo()
  : vendor(PProcess::Current().GetManufacturer())
  , name(PProcess::Current().GetName())
  , version(PProcess::Current().GetVersion())
  , t35CountryCode(9)     // Country code for Australia
  , t35Extension(0)       // No extension code for Australia
  , manufacturerCode(61)  // Allocated by Australian Communications Authority, Oct 2000;
{
}

OpalProductInfo & OpalProductInfo::Default()
{
  static OpalProductInfo instance;
  return instance;
}


PCaselessString OpalProductInfo::AsString() const
{
  PStringStream str;
  str << name << '\t' << version << '\t';
  if (t35CountryCode != 0 && manufacturerCode != 0) {
    str << (unsigned)t35CountryCode;
    if (t35Extension != 0)
      str << '.' << (unsigned)t35Extension;
    str << '/' << manufacturerCode;
  }
  str << '\t' << vendor;
  return str;
}


/////////////////////////////////////////////////////////////////////////////

#ifdef _MSC_VER
#pragma warning(disable:4355)
#endif

OpalManager::OpalManager()
  : defaultUserName(PProcess::Current().GetUserName())
  , defaultDisplayName(defaultUserName)
#ifdef _WIN32
  , rtpIpTypeofService(IPTOS_PREC_CRITIC_ECP|IPTOS_LOWDELAY)
#else
  , rtpIpTypeofService(IPTOS_LOWDELAY) // Don't use IPTOS_PREC_CRITIC_ECP on Unix platforms as then need to be root
#endif
  , minAudioJitterDelay(50)  // milliseconds
  , maxAudioJitterDelay(250) // milliseconds
  , mediaFormatOrder(PARRAYSIZE(DefaultMediaFormatOrder), DefaultMediaFormatOrder)
  , noMediaTimeout(0, 0, 5)     // Minutes
  , translationAddress(0)       // Invalid address to disable
  , stun(NULL)
  , activeCalls(*this)
  , clearingAllCalls(FALSE)
#if OPAL_RTP_AGGREGATE
  , useRTPAggregation(TRUE)
#endif
{
  rtpIpPorts.current = rtpIpPorts.base = 5000;
  rtpIpPorts.max = 5999;

  // use dynamic port allocation by default
  tcpPorts.current = tcpPorts.base = tcpPorts.max = 0;
  udpPorts.current = udpPorts.base = udpPorts.max = 0;

#ifndef NO_OPAL_VIDEO
  PStringList devices;
  
  devices = PVideoInputDevice::GetDriversDeviceNames("*"); // Get all devices on all drivers
  PINDEX i;
  for (i = 0; i < devices.GetSize(); ++i) {
    if ((devices[i] *= "*.yuv") || (devices[i] *= "fake")) 
      continue;
    videoInputDevice.deviceName = devices[i];
    break;
  }
  autoStartTransmitVideo = !videoInputDevice.deviceName.IsEmpty();

  devices = PVideoOutputDevice::GetDriversDeviceNames("*"); // Get all devices on all drivers
  for (i = 0; i < devices.GetSize(); ++i) {
    if ((devices[i] *= "*.yuv") || (devices[i] *= "null"))
      continue;
    videoOutputDevice.deviceName = devices[i];
    break;
  }
  autoStartReceiveVideo = !videoOutputDevice.deviceName.IsEmpty();

  if (autoStartReceiveVideo)
    videoPreviewDevice = videoOutputDevice;
#endif

  garbageCollector = PThread::Create(PCREATE_NOTIFIER(GarbageMain), 0,
                                     PThread::NoAutoDeleteThread,
                                     PThread::LowPriority,
                                     "OpalGarbage");

  PTRACE(4, "OpalMan\tCreated manager.");
}

#ifdef _MSC_VER
#pragma warning(default:4355)
#endif


OpalManager::~OpalManager()
{
  // Clear any pending calls on this endpoint
  ClearAllCalls();

  // Shut down the cleaner thread
  garbageCollectExit.Signal();
  garbageCollector->WaitForTermination();

  // Clean up any calls that the cleaner thread missed
  GarbageCollection();

  delete garbageCollector;

  // Kill off the endpoints, could wait till compiler generated destructor but
  // prefer to keep the PTRACE's in sequence.
  endpoints.RemoveAll();

  delete stun;

  PTRACE(4, "OpalMan\tDeleted manager.");
}


void OpalManager::AttachEndPoint(OpalEndPoint * endpoint)
{
  if (PAssertNULL(endpoint) == NULL)
    return;

  endpointsMutex.StartWrite();

  if (endpoints.GetObjectsIndex(endpoint) == P_MAX_INDEX)
    endpoints.Append(endpoint);

  endpointsMutex.EndWrite();
}


void OpalManager::DetachEndPoint(OpalEndPoint * endpoint)
{
  if (PAssertNULL(endpoint) == NULL)
    return;

  endpointsMutex.StartWrite();
  endpoints.Remove(endpoint);
  endpointsMutex.EndWrite();
}


OpalEndPoint * OpalManager::FindEndPoint(const PString & prefix)
{
  PReadWaitAndSignal mutex(endpointsMutex);

  for (PINDEX i = 0; i < endpoints.GetSize(); i++) {
    if (endpoints[i].GetPrefixName() *= prefix)
      return &endpoints[i];
  }

  return NULL;
}


BOOL OpalManager::SetUpCall(const PString & partyA,
                            const PString & partyB,
                            PString & token,
                            void * userData,
                            unsigned int options,
                            OpalConnection::StringOptions * stringOptions)
{
  PTRACE(3, "OpalMan\tSet up call from " << partyA << " to " << partyB);

  OpalCall * call = CreateCall(userData);
  token = call->GetToken();

  call->SetPartyB(partyB);

  // If we are the A-party then need to initiate a call now in this thread and
  // go through the routing engine via OnIncomingConnection. If we were the
  // B-Party then SetUpConnection() gets called in the context of the A-party
  // thread.
  if (MakeConnection(*call, partyA, userData, options, stringOptions) && call->GetConnection(0)->SetUpConnection()) {
    PTRACE(3, "OpalMan\tSetUpCall succeeded, call=" << *call);
    return TRUE;
  }

  PSafePtr<OpalConnection> connection = call->GetConnection(0);
  OpalConnection::CallEndReason endReason = connection != NULL ? connection->GetCallEndReason() : OpalConnection::NumCallEndReasons;
  call->Clear(endReason != OpalConnection::NumCallEndReasons ? endReason : OpalConnection::EndedByTemporaryFailure);

  if (!activeCalls.RemoveAt(token)) {
    PTRACE(2, "OpalMan\tSetUpCall could not remove call from active call list");
  }

  token.MakeEmpty();

  return FALSE;
}


void OpalManager::OnEstablishedCall(OpalCall & /*call*/)
{
}


BOOL OpalManager::IsCallEstablished(const PString & token)
{
  PSafePtr<OpalCall> call = activeCalls.FindWithLock(token, PSafeReadOnly);
  if (call == NULL)
    return FALSE;

  return call->IsEstablished();
}


BOOL OpalManager::ClearCall(const PString & token,
                            OpalConnection::CallEndReason reason,
                            PSyncPoint * sync)
{
  /*The hugely multi-threaded nature of the OpalCall objects means that
    to avoid many forms of race condition, a call is cleared by moving it from
    the "active" call dictionary to a list of calls to be cleared that will be
    processed by a background thread specifically for the purpose of cleaning
    up cleared calls. So that is all that this function actually does.
    The real work is done in the OpalGarbageCollector thread.
   */

  {
    // Find the call by token, callid or conferenceid
    PSafePtr<OpalCall> call = activeCalls.FindWithLock(token, PSafeReference);
    if (call == NULL)
      return FALSE;

    call->Clear(reason, sync);
  }

  if (sync != NULL)
    sync->Wait();

  return TRUE;
}


BOOL OpalManager::ClearCallSynchronous(const PString & token,
                                       OpalConnection::CallEndReason reason)
{
  PSyncPoint wait;
  return ClearCall(token, reason, &wait);
}


void OpalManager::ClearAllCalls(OpalConnection::CallEndReason reason, BOOL wait)
{
  // Remove all calls from the active list first
  for (PSafePtr<OpalCall> call = activeCalls; call != NULL; ++call) {
    call->Clear(reason);
  }

  if (wait) {
    clearingAllCalls = TRUE;
    allCallsCleared.Wait();
    clearingAllCalls = FALSE;
  }
}


void OpalManager::OnClearedCall(OpalCall & PTRACE_PARAM(call))
{
  PTRACE(3, "OpalMan\tOnClearedCall " << call << " from \"" << call.GetPartyA() << "\" to \"" << call.GetPartyB() << '"');
}


OpalCall * OpalManager::CreateCall()
{
  return new OpalCall(*this);
}

OpalCall * OpalManager::CreateCall(void * /*userData*/)
{
  return CreateCall();
}


void OpalManager::DestroyCall(OpalCall * call)
{
  delete call;
}


PString OpalManager::GetNextCallToken()
{
  return psprintf("%u", ++lastCallTokenID);
}

BOOL OpalManager::MakeConnection(OpalCall & call, const PString & remoteParty, void * userData, unsigned int options, OpalConnection::StringOptions * stringOptions)
{
  PTRACE(3, "OpalMan\tSet up connection to \"" << remoteParty << '"');

  if (remoteParty.IsEmpty() || endpoints.IsEmpty())
    return FALSE;

  PCaselessString epname = remoteParty.Left(remoteParty.Find(':'));

  PReadWaitAndSignal mutex(endpointsMutex);

  if (epname.IsEmpty())
    epname = endpoints[0].GetPrefixName();

  for (PINDEX i = 0; i < endpoints.GetSize(); i++) {
    if (epname == endpoints[i].GetPrefixName()) {
      if (endpoints[i].MakeConnection(call, remoteParty, userData, options, stringOptions))
        return TRUE;
    }
  }

  PTRACE(1, "OpalMan\tCould not find endpoint to handle protocol \"" << epname << '"');
  return FALSE;
}

BOOL OpalManager::OnIncomingConnection(OpalConnection & /*connection*/)
{
  return TRUE;
}

BOOL OpalManager::OnIncomingConnection(OpalConnection & /*connection*/, unsigned /*options*/)
{
  return TRUE;
}

BOOL OpalManager::OnIncomingConnection(OpalConnection & connection, unsigned options, OpalConnection::StringOptions * stringOptions)
{
  PTRACE(3, "OpalMan\tOn incoming connection " << connection);

  if (!OnIncomingConnection(connection))
    return FALSE;

  if (!OnIncomingConnection(connection, options))
    return FALSE;

  OpalCall & call = connection.GetCall();

  // See if we already have a B-Party in the call. If not, make one.
  if (call.GetOtherPartyConnection(connection) != NULL)
    return TRUE;

  // Use a routing algorithm to figure out who the B-Party is, then make a connection
  return MakeConnection(call, OnRouteConnection(connection), NULL, options, stringOptions);
}


PString OpalManager::OnRouteConnection(OpalConnection & connection)
{
  // See if have pre-allocated B party address, otherwise use routing algorithm
  PString addr = connection.GetCall().GetPartyB();
  if (addr.IsEmpty()) {
    addr = connection.GetDestinationAddress();

    // No address, fail call
    if (addr.IsEmpty())
      return addr;
  }

  // Have explicit protocol defined, so no translation to be done
  PINDEX colon = addr.Find(':');
  if (colon != P_MAX_INDEX && FindEndPoint(addr.Left(colon)) != NULL)
    return addr;

  // No routes specified, just return what we've got so far, maybe it will work
  if (routeTable.IsEmpty())
    return addr;

  return ApplyRouteTable(connection.GetEndPoint().GetPrefixName(), addr);
}


void OpalManager::OnAlerting(OpalConnection & connection)
{
  PTRACE(3, "OpalMan\tOnAlerting " << connection);

  connection.GetCall().OnAlerting(connection);
}

OpalConnection::AnswerCallResponse
       OpalManager::OnAnswerCall(OpalConnection & connection,
                                  const PString & caller)
{
  return connection.GetCall().OnAnswerCall(connection, caller);
}

void OpalManager::OnConnected(OpalConnection & connection)
{
  PTRACE(3, "OpalMan\tOnConnected " << connection);

  connection.GetCall().OnConnected(connection);
}


void OpalManager::OnEstablished(OpalConnection & connection)
{
  PTRACE(3, "OpalMan\tOnEstablished " << connection);

  connection.GetCall().OnEstablished(connection);
}


void OpalManager::OnReleased(OpalConnection & connection)
{
  PTRACE(3, "OpalMan\tOnReleased " << connection);

  connection.GetCall().OnReleased(connection);
}


void OpalManager::OnHold(OpalConnection & PTRACE_PARAM(connection))
{
  PTRACE(3, "OpalMan\tOnHold " << connection);
}


BOOL OpalManager::OnForwarded(OpalConnection & PTRACE_PARAM(connection),
			      const PString & /*forwardParty*/)
{
  PTRACE(4, "OpalEP\tOnForwarded " << connection);
  return TRUE;
}


void OpalManager::AdjustMediaFormats(const OpalConnection & /*connection*/,
                                     OpalMediaFormatList & mediaFormats) const
{
  mediaFormats.Remove(mediaFormatMask);
  mediaFormats.Reorder(mediaFormatOrder);
}


BOOL OpalManager::IsMediaBypassPossible(const OpalConnection & source,
                                        const OpalConnection & destination,
                                        unsigned sessionID) const
{
  PTRACE(3, "OpalMan\tIsMediaBypassPossible: session " << sessionID);

  return source.IsMediaBypassPossible(sessionID) &&
         destination.IsMediaBypassPossible(sessionID);
}


BOOL OpalManager::OnOpenMediaStream(OpalConnection & connection,
                                    OpalMediaStream & stream)
{
  PTRACE(3, "OpalMan\tOnOpenMediaStream " << connection << ',' << stream);

  if (stream.IsSource())
    return connection.GetCall().PatchMediaStreams(connection, stream);

  return TRUE;
}


void OpalManager::OnRTPStatistics(const OpalConnection & connection, const RTP_Session & session)
{
  connection.GetCall().OnRTPStatistics(connection, session);
}


void OpalManager::OnClosedMediaStream(const OpalMediaStream & /*channel*/)
{
}

#if OPAL_VIDEO

void OpalManager::AddVideoMediaFormats(OpalMediaFormatList & mediaFormats,
                                       const OpalConnection * /*connection*/) const
{
  if (videoInputDevice.deviceName.IsEmpty())
      return;

  mediaFormats += OpalYUV420P;
  mediaFormats += OpalRGB32;
  mediaFormats += OpalRGB24;
}


BOOL OpalManager::CreateVideoInputDevice(const OpalConnection & /*connection*/,
                                         const OpalMediaFormat & mediaFormat,
                                         PVideoInputDevice * & device,
                                         BOOL & autoDelete)
{
  // Make copy so we can adjust the size
  PVideoDevice::OpenArgs args = videoInputDevice;
  args.width = mediaFormat.GetOptionInteger(OpalVideoFormat::FrameWidthOption(), PVideoFrameInfo::QCIFWidth);
  args.height = mediaFormat.GetOptionInteger(OpalVideoFormat::FrameHeightOption(), PVideoFrameInfo::QCIFHeight);

  autoDelete = TRUE;
  device = PVideoInputDevice::CreateOpenedDevice(args);
  return device != NULL;
}


BOOL OpalManager::CreateVideoOutputDevice(const OpalConnection & connection,
                                          const OpalMediaFormat & mediaFormat,
                                          BOOL preview,
                                          PVideoOutputDevice * & device,
                                          BOOL & autoDelete)
{
  // Donot use our one and only SDl window, if we need it for the video output.
  if (preview && videoPreviewDevice.driverName == "SDL" && videoOutputDevice.driverName == "SDL")
    return FALSE;

  // Make copy so we can adjust the size
  PVideoDevice::OpenArgs args = preview ? videoPreviewDevice : videoOutputDevice;
  args.width = mediaFormat.GetOptionInteger(OpalVideoFormat::FrameWidthOption(), PVideoFrameInfo::QCIFWidth);
  args.height = mediaFormat.GetOptionInteger(OpalVideoFormat::FrameHeightOption(), PVideoFrameInfo::QCIFHeight);

  PINDEX start = args.deviceName.Find("TITLE=\"");
  if (start != P_MAX_INDEX) {
    start += 7;
    args.deviceName.Splice(preview ? "Local Preview" : connection.GetRemotePartyName(), start, args.deviceName.Find('"', start)-start);
  }

  autoDelete = TRUE;
  device = PVideoOutputDevice::CreateOpenedDevice(args);
  return device != NULL;
}

#endif // OPAL_VIDEO


OpalMediaPatch * OpalManager::CreateMediaPatch(OpalMediaStream & source,
                                               BOOL requiresPatchThread)
{
  if (requiresPatchThread) {
    return new OpalMediaPatch(source);
  } else {
    return new OpalPassiveMediaPatch(source);
  }
}


void OpalManager::DestroyMediaPatch(OpalMediaPatch * patch)
{
  delete patch;
}


BOOL OpalManager::OnStartMediaPatch(const OpalMediaPatch & /*patch*/)
{
  return TRUE;
}


void OpalManager::OnUserInputString(OpalConnection & connection,
                                    const PString & value)
{
  connection.GetCall().OnUserInputString(connection, value);
}


void OpalManager::OnUserInputTone(OpalConnection & connection,
                                  char tone,
                                  int duration)
{
  connection.GetCall().OnUserInputTone(connection, tone, duration);
}


PString OpalManager::ReadUserInput(OpalConnection & connection,
                                  const char * terminators,
                                  unsigned lastDigitTimeout,
                                  unsigned firstDigitTimeout)
{
  PTRACE(3, "OpalCon\tReadUserInput from " << connection);

  connection.PromptUserInput(TRUE);
  PString digit = connection.GetUserInput(firstDigitTimeout);
  connection.PromptUserInput(FALSE);

  if (digit.IsEmpty()) {
    PTRACE(2, "OpalCon\tReadUserInput first character timeout (" << firstDigitTimeout << "ms) on " << *this);
    return PString::Empty();
  }

  PString input;
  while (digit.FindOneOf(terminators) == P_MAX_INDEX) {
    input += digit;

    digit = connection.GetUserInput(lastDigitTimeout);
    if (digit.IsEmpty()) {
      PTRACE(2, "OpalCon\tReadUserInput last character timeout (" << lastDigitTimeout << "ms) on " << *this);
      return input; // Input so far will have to do
    }
  }

  return input.IsEmpty() ? digit : input;
}

#if OPAL_T120DATA

OpalT120Protocol * OpalManager::CreateT120ProtocolHandler(const OpalConnection & ) const
{
  return NULL;
}

#endif

#if OPAL_T38FAX

OpalT38Protocol * OpalManager::CreateT38ProtocolHandler(const OpalConnection & ) const
{
  return NULL;
}

#endif

#ifdef OPAL_H224

OpalH224Handler * OpalManager::CreateH224ProtocolHandler(OpalConnection & connection,
														 unsigned sessionID) const
{
  return new OpalH224Handler(connection, sessionID);
}

OpalH281Handler * OpalManager::CreateH281ProtocolHandler(OpalH224Handler & h224Handler) const
{
  return new OpalH281Handler(h224Handler);
}

#endif

OpalManager::RouteEntry::RouteEntry(const PString & pat, const PString & dest)
  : pattern(pat),
    destination(dest),
    regex('^'+pat+'$')
{
}


void OpalManager::RouteEntry::PrintOn(ostream & strm) const
{
  strm << pattern << '=' << destination;
}


BOOL OpalManager::AddRouteEntry(const PString & spec)
{
  if (spec[0] == '#') // Comment
    return FALSE;

  if (spec[0] == '@') { // Load from file
    PTextFile file;
    if (!file.Open(spec.Mid(1), PFile::ReadOnly)) {
      PTRACE(1, "OpalMan\tCould not open route file \"" << file.GetFilePath() << '"');
      return FALSE;
    }
    PTRACE(4, "OpalMan\tAdding routes from file \"" << file.GetFilePath() << '"');
    BOOL ok = FALSE;
    PString line;
    while (file.good()) {
      file >> line;
      if (AddRouteEntry(line))
        ok = TRUE;
    }
    return ok;
  }

  PINDEX equal = spec.Find('=');
  if (equal == P_MAX_INDEX) {
    PTRACE(2, "OpalMan\tInvalid route table entry: \"" << spec << '"');
    return FALSE;
  }

  RouteEntry * entry = new RouteEntry(spec.Left(equal).Trim(), spec.Mid(equal+1).Trim());
  if (entry->regex.GetErrorCode() != PRegularExpression::NoError) {
    PTRACE(2, "OpalMan\tIllegal regular expression in route table entry: \"" << spec << '"');
    delete entry;
    return FALSE;
  }

  PTRACE(4, "OpalMan\tAdded route \"" << *entry << '"');
  routeTableMutex.Wait();
  routeTable.Append(entry);
  routeTableMutex.Signal();
  return TRUE;
}


BOOL OpalManager::SetRouteTable(const PStringArray & specs)
{
  BOOL ok = FALSE;

  routeTableMutex.Wait();
  routeTable.RemoveAll();

  for (PINDEX i = 0; i < specs.GetSize(); i++) {
    if (AddRouteEntry(specs[i].Trim()))
      ok = TRUE;
  }

  routeTableMutex.Signal();

  return ok;
}


void OpalManager::SetRouteTable(const RouteTable & table)
{
  routeTableMutex.Wait();
  routeTable = table;
  routeTable.MakeUnique();
  routeTableMutex.Signal();
}


PString OpalManager::ApplyRouteTable(const PString & proto, const PString & addr)
{
  PWaitAndSignal mutex(routeTableMutex);

  PString destination;
  PString search = proto + ':' + addr;
  PTRACE(4, "OpalMan\tSearching for route \"" << search << '"');
  for (PINDEX i = 0; i < routeTable.GetSize(); i++) {
    RouteEntry & entry = routeTable[i];
    PINDEX pos;
    if (entry.regex.Execute(search, pos)) {
      destination = routeTable[i].destination;
      break;
    }
  }

  if (destination.IsEmpty())
    return PString::Empty();

  destination.Replace("<da>", addr);

  PINDEX pos;
  if ((pos = destination.Find("<dn>")) != P_MAX_INDEX)
    destination.Splice(addr.Left(addr.FindSpan("0123456789*#")), pos, 4);

  if ((pos = destination.Find("<!dn>")) != P_MAX_INDEX)
    destination.Splice(addr.Mid(addr.FindSpan("0123456789*#")), pos, 5);

  // Do meta character substitutions
  if ((pos = destination.Find("<dn2ip>")) != P_MAX_INDEX) {
    PStringStream route;
    PStringArray stars = addr.Tokenise('*');
    switch (stars.GetSize()) {
      case 0 :
      case 1 :
      case 2 :
      case 3 :
        route << addr;
        break;

      case 4 :
        route << stars[0] << '.' << stars[1] << '.'<< stars[2] << '.'<< stars[3];
        break;

      case 5 :
        route << stars[0] << '@'
              << stars[1] << '.' << stars[2] << '.'<< stars[3] << '.'<< stars[4];
        break;

      default :
        route << stars[0] << '@'
              << stars[1] << '.' << stars[2] << '.'<< stars[3] << '.'<< stars[4]
              << ':' << stars[5];
        break;
    }
    destination.Splice(route, pos, 7);
  }

  return destination;
}


BOOL OpalManager::IsLocalAddress(const PIPSocket::Address & ip) const
{
  /* Check if the remote address is a private IP, broadcast, or us */
  return ip.IsAny() || ip.IsBroadcast() || ip.IsRFC1918() || PIPSocket::IsLocalHost(ip);
}


BOOL OpalManager::TranslateIPAddress(PIPSocket::Address & localAddress,
                                     const PIPSocket::Address & remoteAddress)
{
  if (!translationAddress.IsValid())
    return FALSE; // Have nothing to translate it to

  if (!IsLocalAddress(localAddress))
    return FALSE; // Is already translated

  if (IsLocalAddress(remoteAddress))
    return FALSE; // Does not need to be translated

  // Tranlsate it!
  localAddress = translationAddress;
  return TRUE;
}


PSTUNClient * OpalManager::GetSTUN(const PIPSocket::Address & ip) const
{
  if (ip.IsValid() && IsLocalAddress(ip))
    return NULL;

  return stun;
}


PSTUNClient::NatTypes OpalManager::SetSTUNServer(const PString & server)
{
  stunServer = server;

  if (server.IsEmpty()) {
    delete stun;
    stun = NULL;
    return PSTUNClient::UnknownNat;
  }

  if (stun == NULL)
    stun = new PSTUNClient(server,
                           GetUDPPortBase(), GetUDPPortMax(),
                           GetRtpIpPortBase(), GetRtpIpPortMax());
  else
    stun->SetServer(server);

  PSTUNClient::NatTypes type = stun->GetNatType();
  if (type != PSTUNClient::BlockedNat)
    stun->GetExternalAddress(translationAddress);

  PTRACE(3, "OPAL\tSTUN server \"" << server << "\" replies " << type << ", external IP " << translationAddress);

  return type;
}


void OpalManager::PortInfo::Set(unsigned newBase,
                                unsigned newMax,
                                unsigned range,
                                unsigned dflt)
{
  if (newBase == 0) {
    newBase = dflt;
    newMax = dflt;
    if (dflt > 0)
      newMax += range;
  }
  else {
    if (newBase < 1024)
      newBase = 1024;
    else if (newBase > 65500)
      newBase = 65500;

    if (newMax <= newBase)
      newMax = newBase + range;
    if (newMax > 65535)
      newMax = 65535;
  }

  mutex.Wait();

  current = base = (WORD)newBase;
  max = (WORD)newMax;

  mutex.Signal();
}


WORD OpalManager::PortInfo::GetNext(unsigned increment)
{
  PWaitAndSignal m(mutex);

  if (current < base || current >= (max-increment))
    current = base;

  if (current == 0)
    return 0;

  WORD p = current;
  current = (WORD)(current + increment);
  return p;
}


void OpalManager::SetTCPPorts(unsigned tcpBase, unsigned tcpMax)
{
  tcpPorts.Set(tcpBase, tcpMax, 49, 0);
}


WORD OpalManager::GetNextTCPPort()
{
  return tcpPorts.GetNext(1);
}


void OpalManager::SetUDPPorts(unsigned udpBase, unsigned udpMax)
{
  udpPorts.Set(udpBase, udpMax, 99, 0);

  if (stun != NULL)
    stun->SetPortRanges(GetUDPPortBase(), GetUDPPortMax(), GetRtpIpPortBase(), GetRtpIpPortMax());
}


WORD OpalManager::GetNextUDPPort()
{
  return udpPorts.GetNext(1);
}


void OpalManager::SetRtpIpPorts(unsigned rtpIpBase, unsigned rtpIpMax)
{
  rtpIpPorts.Set((rtpIpBase+1)&0xfffe, rtpIpMax&0xfffe, 199, 5000);

  if (stun != NULL)
    stun->SetPortRanges(GetUDPPortBase(), GetUDPPortMax(), GetRtpIpPortBase(), GetRtpIpPortMax());
}


WORD OpalManager::GetRtpIpPortPair()
{
  return rtpIpPorts.GetNext(2);
}


void OpalManager::SetAudioJitterDelay(unsigned minDelay, unsigned maxDelay)
{
  PAssert(minDelay <= 10000 && maxDelay <= 10000, PInvalidParameter);

  if (minDelay < 10)
    minDelay = 10;
  minAudioJitterDelay = minDelay;

  if (maxDelay < minDelay)
    maxDelay = minDelay;
  maxAudioJitterDelay = maxDelay;
}


#if OPAL_VIDEO
template<class PVideoXxxDevice>
static BOOL SetVideoDevice(const PVideoDevice::OpenArgs & args, PVideoDevice::OpenArgs & member)
{
  // Check that the input device is legal
  PVideoXxxDevice * pDevice = PVideoXxxDevice::CreateDeviceByName(args.deviceName, args.driverName, args.pluginMgr);
  if (pDevice != NULL) {
    delete pDevice;
    member = args;
    return TRUE;
  }

  if (args.deviceName[0] != '#')
    return FALSE;

  // Selected device by ordinal
  PStringList devices = PVideoXxxDevice::GetDriversDeviceNames(args.driverName, args.pluginMgr);
  if (devices.IsEmpty())
    return FALSE;

  PINDEX id = args.deviceName.Mid(1).AsUnsigned();
  if (id <= 0 || id > devices.GetSize())
    return FALSE;

  member = args;
  member.deviceName = devices[id-1];
  return TRUE;
}


BOOL OpalManager::SetVideoInputDevice(const PVideoDevice::OpenArgs & args)
{
  return SetVideoDevice<PVideoInputDevice>(args, videoInputDevice);
}


BOOL OpalManager::SetVideoPreviewDevice(const PVideoDevice::OpenArgs & args)
{
  return SetVideoDevice<PVideoOutputDevice>(args, videoPreviewDevice);
}


BOOL OpalManager::SetVideoOutputDevice(const PVideoDevice::OpenArgs & args)
{
  return SetVideoDevice<PVideoOutputDevice>(args, videoOutputDevice);
}

#endif

BOOL OpalManager::SetNoMediaTimeout(const PTimeInterval & newInterval) 
{
  if (newInterval < 10)
    return FALSE;

  noMediaTimeout = newInterval; 
  return TRUE; 
}


void OpalManager::GarbageCollection()
{
  BOOL allCleared = activeCalls.DeleteObjectsToBeRemoved();

  endpointsMutex.StartRead();

  for (PINDEX i = 0; i < endpoints.GetSize(); i++) {
    if (!endpoints[i].GarbageCollection())
      allCleared = FALSE;
  }

  endpointsMutex.EndRead();

  if (allCleared && clearingAllCalls)
    allCallsCleared.Signal();
}


void OpalManager::CallDict::DeleteObject(PObject * object) const
{
  manager.DestroyCall(PDownCast(OpalCall, object));
}


void OpalManager::GarbageMain(PThread &, INT)
{
  while (!garbageCollectExit.Wait(1000))
    GarbageCollection();
}

void OpalManager::OnNewConnection(OpalConnection & /*conn*/)
{
}

BOOL OpalManager::UseRTPAggregation() const
{ 
#if OPAL_RTP_AGGREGATE
  return useRTPAggregation; 
#else
  return FALSE;
#endif
}

BOOL OpalManager::StartRecording(const PString & callToken, const PFilePath & fn)
{
  PSafePtr<OpalCall> call = activeCalls.FindWithLock(callToken, PSafeReadWrite);
  if (call == NULL)
    return FALSE;

  return call->StartRecording(fn);
}

void OpalManager::StopRecording(const PString & callToken)
{
  PSafePtr<OpalCall> call = activeCalls.FindWithLock(callToken, PSafeReadWrite);
  if (call != NULL)
    call->StopRecording();
}


BOOL OpalManager::IsRTPNATEnabled(OpalConnection & /*conn*/, 
                    const PIPSocket::Address & localAddr, 
                    const PIPSocket::Address & peerAddr,
                    const PIPSocket::Address & sigAddr,
                                          BOOL incoming)
{
  BOOL remoteIsNAT = FALSE;

  PTRACE(4, "OPAL\tChecking " << (incoming ? "incoming" : "outgoing") << " call for NAT: local=" << localAddr << ",peer=" << peerAddr << ",sig=" << sigAddr);

  if (incoming) {

    // by default, only check for NAT under two conditions
    //    1. Peer is not local, but the peer thinks it is
    //    2. Peer address and local address are both private, but not the same
    //
    if ((!peerAddr.IsRFC1918() && sigAddr.IsRFC1918()) ||
        ((peerAddr.IsRFC1918() && localAddr.IsRFC1918()) && (localAddr != peerAddr))) {

      // given these paramaters, translate the local address
      PIPSocket::Address trialAddr = localAddr;
      TranslateIPAddress(trialAddr, peerAddr);

      PTRACE(3, "OPAL\tTranslateIPAddress has converted " << localAddr << " to " << trialAddr);

      // if the application specific routine changed the local address, then enable RTP NAT mode
      if (localAddr != trialAddr) {
        PTRACE(3, "OPAL\tSource signal address " << sigAddr << " and peer address " << peerAddr << " indicate remote endpoint is behind NAT");
        remoteIsNAT = TRUE;
      }
    }
  }
  else
  {
  }

  return remoteIsNAT;
}


/////////////////////////////////////////////////////////////////////////////

OpalRecordManager::Mixer_T::Mixer_T()
  : OpalAudioMixer(TRUE)
{ 
  mono = FALSE; 
  started = FALSE; 
}

BOOL OpalRecordManager::Mixer_T::Open(const PFilePath & fn)
{
  PWaitAndSignal m(mutex);

  if (!started) {
    file.SetFormat(OpalWAVFile::fmt_PCM);
    file.Open(fn, PFile::ReadWrite);
    if (!mono)
      file.SetChannels(2);
    started = TRUE;
  }
  return TRUE;
}

BOOL OpalRecordManager::Mixer_T::Close()
{
  PWaitAndSignal m(mutex);
  file.Close();
  return TRUE;
}

BOOL OpalRecordManager::Mixer_T::OnWriteAudio(const MixerFrame & mixerFrame)
{
  if (file.IsOpen()) {
    OpalAudioMixerStream::StreamFrame frame;
    if (mono) {
      mixerFrame.GetMixedFrame(frame);
      file.Write(frame.GetPointerAndLock(), frame.GetSize());
      frame.Unlock();
    } else {
      mixerFrame.GetStereoFrame(frame);
      file.Write(frame.GetPointerAndLock(), frame.GetSize());
      frame.Unlock();
    }
  }
  return TRUE;
}

OpalRecordManager::OpalRecordManager()
{
  started = FALSE;
}

BOOL OpalRecordManager::Open(const PString & _callToken, const PFilePath & fn)
{
  PWaitAndSignal m(mutex);

  if (_callToken.IsEmpty())
    return FALSE;

  if (token.IsEmpty())
    token = _callToken;
  else if (_callToken != token)
    return FALSE;

  return mixer.Open(fn);
}

BOOL OpalRecordManager::CloseStream(const PString & _callToken, const std::string & _strm)
{
  {
    PWaitAndSignal m(mutex);
    if (_callToken.IsEmpty() || token.IsEmpty() || (token != _callToken))
      return FALSE;

    mixer.RemoveStream(_strm);
  }
  return TRUE;
}

BOOL OpalRecordManager::Close(const PString & _callToken)
{
  {
    PWaitAndSignal m(mutex);
    if (_callToken.IsEmpty() || token.IsEmpty() || (token != _callToken))
      return FALSE;

    mixer.RemoveAllStreams();
  }
  mixer.Close();
  return TRUE;
}

BOOL OpalRecordManager::WriteAudio(const PString & _callToken, const std::string & strm, const RTP_DataFrame & rtp)
{ 
  PWaitAndSignal m(mutex);
  if (_callToken.IsEmpty() || token.IsEmpty() || (token != _callToken))
    return FALSE;

  return mixer.Write(strm, rtp);
}
