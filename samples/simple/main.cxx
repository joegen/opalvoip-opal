/*
 * main.cxx
 *
 * A simple H.323 "net telephone" application.
 *
 * Copyright (c) 2000 Equivalence Pty. Ltd.
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
 * The Original Code is Portable Windows Library.
 *
 * The Initial Developer of the Original Code is Equivalence Pty. Ltd.
 *
 * Contributor(s): ______________________________________.
 *
 * $Log: main.cxx,v $
 * Revision 1.2024  2003/03/19 02:30:45  robertj
 * Added removal of IVR stuff if EXPAT is not installed on system.
 *
 * Revision 2.22  2003/03/17 08:12:08  robertj
 * Added protocol name to media stream open output.
 *
 * Revision 2.21  2003/03/07 08:18:47  robertj
 * Fixed naming convention of PC sound system in routing table.
 *
 * Revision 2.20  2003/03/07 05:56:47  robertj
 * Changed so a # from whatever source routes to IVR.
 *
 * Revision 2.19  2003/03/06 03:57:47  robertj
 * IVR support (work in progress) requiring large changes everywhere.
 *
 * Revision 2.18  2002/11/11 06:52:01  robertj
 * Added correct flag for including static global variables.
 *
 * Revision 2.17  2002/11/10 11:33:17  robertj
 * Updated to OpenH323 v1.10.3
 *
 * Revision 2.16  2002/09/06 02:46:00  robertj
 * Added routing table system to route calls by regular expressions.
 * Added ability to set gatekeeper access token OID and password.
 *
 * Revision 2.15  2002/07/01 09:05:54  robertj
 * Changed TCp/UDP port allocation to use new thread safe functions.
 *
 * Revision 2.14  2002/04/12 14:02:41  robertj
 * Separated interface option for SIP and H.323.
 *
 * Revision 2.13  2002/03/27 05:34:55  robertj
 * Removed redundent busy forward field.
 * Added ability to set tcp and udp port bases.
 *
 * Revision 2.12  2002/03/27 04:36:46  robertj
 * Changed to add all possible xJack cards to pots endpoint.
 *
 * Revision 2.11  2002/03/27 04:16:20  robertj
 * Restructured default router for sample to allow more options.
 *
 * Revision 2.10  2002/02/13 08:17:31  robertj
 * Changed routing algorithm to route call to H323/SIP if contains an '@'
 *
 * Revision 2.9  2002/02/11 09:45:35  robertj
 * Moved version file to library root directoy
 *
 * Revision 2.8  2002/02/06 13:29:03  rogerh
 * H245 tunnelling is controlled by 'T' and not by 'h'
 *
 * Revision 2.7  2002/02/01 04:53:01  robertj
 * Added (very primitive!) SIP support.
 *
 * Revision 2.6  2002/01/22 05:34:58  robertj
 * Revamp of user input API triggered by RFC2833 support
 *
 * Revision 2.5  2001/08/21 11:18:55  robertj
 * Added conditional compile for xJack code.
 *
 * Revision 2.4  2001/08/17 08:35:41  robertj
 * Changed OnEstablished() to OnEstablishedCall() to be consistent.
 * Moved call end reasons enum from OpalConnection to global.
 * Used LID management in lid EP.
 * More implementation.
 *
 * Revision 2.3  2001/08/01 06:19:00  robertj
 * Added flags for disabling H.323 or Quicknet endpoints.
 *
 * Revision 2.2  2001/08/01 05:48:30  robertj
 * Major changes to H.323 capabilities, uses OpalMediaFormat for base name.
 * Fixed loading of transcoders from static library.
 *
 * Revision 2.1  2001/07/30 07:22:25  robertj
 * Abstracted listener management from H.323 to OpalEndPoint class.
 *
 * Revision 2.0  2001/07/27 15:48:24  robertj
 * Conversion of OpenH323 to Open Phone Abstraction Library (OPAL)
 *
 * Revision 1.12  2001/07/13 08:44:16  robertj
 * Fixed incorrect inclusion of hardware codec capabilities.
 *
 * Revision 1.11  2001/05/17 07:11:29  robertj
 * Added more call end types for common transport failure modes.
 *
 * Revision 1.10  2001/05/14 05:56:26  robertj
 * Added H323 capability registration system so can add capabilities by
 *   string name instead of having to instantiate explicit classes.
 *
 * Revision 1.9  2001/03/21 04:52:40  robertj
 * Added H.235 security to gatekeepers, thanks Fürbass Franz!
 *
 * Revision 1.8  2001/03/20 23:42:55  robertj
 * Used the new PTrace::Initialise function for starting trace code.
 *
 * Revision 1.7  2000/10/16 08:49:31  robertj
 * Added single function to add all UserInput capability types.
 *
 * Revision 1.6  2000/07/31 14:08:09  robertj
 * Added fast start and H.245 tunneling flags to the H323Connection constructor so can
 *    disabled these features in easier manner to overriding virtuals.
 *
 * Revision 1.5  2000/06/20 02:38:27  robertj
 * Changed H323TransportAddress to default to IP.
 *
 * Revision 1.4  2000/06/07 05:47:55  robertj
 * Added call forwarding.
 *
 * Revision 1.3  2000/05/23 11:32:27  robertj
 * Rewrite of capability table to combine 2 structures into one and move functionality into that class
 *    allowing some normalisation of usage across several applications.
 * Changed H323Connection so gets a copy of capabilities instead of using endponts, allows adjustments
 *    to be done depending on the remote client application.
 *
 * Revision 1.2  2000/05/11 10:00:02  robertj
 * Fixed setting and resetting of current call token variable.
 *
 * Revision 1.1  2000/05/11 04:05:57  robertj
 * Simple sample program.
 *
 */

#include <ptlib.h>

#include <sip/sip.h>
#include <h323/h323.h>
#include <h323/gkclient.h>
#include <lids/lidep.h>

#ifdef HAS_IXJ
#include <lids/ixjlid.h>
#endif

#include "main.h"
#include "../../version.h"


#ifdef OPAL_STATIC_LINK
#define H323_STATIC_LIB
#include <codec/allcodecs.h>
#endif


#define new PNEW


PCREATE_PROCESS(SimpleOpalProcess);


///////////////////////////////////////////////////////////////

SimpleOpalProcess::SimpleOpalProcess()
  : PProcess("Open Phone Abstraction Library", "SimpleOPAL",
             MAJOR_VERSION, MINOR_VERSION, BUILD_TYPE, BUILD_NUMBER)
{
}


void SimpleOpalProcess::Main()
{
  cout << GetName()
       << " Version " << GetVersion(TRUE)
       << " by " << GetManufacturer()
       << " on " << GetOSClass() << ' ' << GetOSName()
       << " (" << GetOSVersion() << '-' << GetOSHardware() << ")\n\n";

  // Get and parse all of the command line arguments.
  PArgList & args = GetArguments();
  args.Parse(
             "a-auto-answer."
             "b-bandwidth:"
             "D-disable:"
             "d-dial-peer:"
             "-no-std-dial-peer."
             "e-silence."
             "f-fast-disable."
             "g-gatekeeper:"
             "G-gateway:"
             "-gk-token:"
             "h-help."
             "H-no-h323."
             "-h323-listen:"
             "I-no-sip."
             "j-jitter:"
             "l-listen."
             "n-no-gatekeeper."
#if PTRACING
             "o-output:"
#endif
             "P-prefer:"
             "p-password:"
             "q-quicknet:"
             "Q-no-quicknet."
             "R-require-gatekeeper."
             "r-register-sip:"
             "-rtp-base:"
             "-rtp-max:"
             "-rtp_tos:"
             "s-sound:"
             "S-no-sound."
             "-sound-in:"
             "-sound-out:"
             "-sip-listen:"
             "T-h245tunneldisable."
#if PTRACING
             "t-trace."
#endif
             "-tcp-base:"
             "-tcp-max:"
             "u-user:"
             "-udp-base:"
             "-udp-max:"
	     "-use-long-mime."
#if P_EXPAT
             "V-no-ivr."
             "x-vxml:"
#endif
          , FALSE);


  if (args.HasOption('h') || (!args.HasOption('l') && args.GetCount() == 0)) {
    cout << "Usage : " << GetName() << " [options] -l\n"
            "      : " << GetName() << " [options] [alias@]hostname   (no gatekeeper)\n"
            "      : " << GetName() << " [options] alias[@hostname]   (with gatekeeper)\n"
            "General options:\n"
            "  -l --listen             : Listen for incoming calls.\n"
            "  -d --dial-peer spec     : Set dial peer for routing calls (see below)\n"
            "     --no-std-dial-peer   : Do not include the standard dial peers\n"
            "  -a --auto-answer        : Automatically answer incoming calls.\n"
            "  -u --user name          : Set local alias name(s) (defaults to login name).\n"
            "  -p --password pwd       : Set password for user (gk or SIP authorisation).\n"
            "\n"
            "Audio options:\n"
            "  -D --disable codec      : Disable the specified codec (may be used multiple times)\n"
            "  -P --prefer codec       : Prefer the specified codec (may be used multiple times)\n"
            "  -j --jitter [min-]max   : Set minimum (optional) and maximum jitter buffer (in milliseconds).\n"
            "  -e --silence            : Disable transmitter silence detection.\n"
            "\n"
            "SIP options:\n"
            "  -I --no-sip             : Disable SIP protocol.\n"
            "  -r --register-sip host  : Register with SIP server.\n"
            "     --sip-listen iface   : Interface/port(s) to listen for SIP requests\n"
            "                          : '*' is all interfaces, (default udp$:*:5060)\n"
            "     --use-long-mime      : Use long MIME headers on outgoing SIP messages\n"
            "\n"
            "H.323 options:\n"
            "  -H --no-h323            : Disable H.323 protocol.\n"
            "  -g --gatekeeper host    : Specify gatekeeper host.\n"
            "  -n --no-gatekeeper      : Disable gatekeeper discovery.\n"
            "  -R --require-gatekeeper : Exit if gatekeeper discovery fails.\n"
            "     --gk-token str       : Set gatekeeper security token OID.\n"
            "  -G --gateway addr       : Use gateway/proxy\n"
            "  -b --bandwidth bps      : Limit bandwidth usage to bps bits/second.\n"
            "  -f --fast-disable       : Disable fast start.\n"
            "  -T --h245tunneldisable  : Disable H245 tunnelling.\n"
            "     --h323-listen iface  : Interface/port(s) to listen for H.323 requests\n"
            "                          : '*' is all interfaces, (default tcp$:*:1720)\n"
            "\n"
            "Quicknet options:\n"
            "  -Q --no-quicknet        : Do not use Quicknet xJACK device.\n"
            "  -q --quicknet device    : Select Quicknet xJACK device (default ALL).\n"
            "\n"
            "Sound card options:\n"
            "  -S --no-sound           : Do not use sound input/output device.\n"
            "  -s --sound device       : Select sound input/output device.\n"
            "     --sound-in device    : Select sound input device.\n"
            "     --sound-out device   : Select sound output device.\n"
            "\n"
#if P_EXPAT
            "IVR options:\n"
            "  -V --no-ivr             : Disable IVR.\n"
            "  -x --vxml file          : Set vxml file to use for IVR.\n"
            "\n"
#endif
            "IP options:\n"
            "     --tcp-base n         : Set TCP port base (default 0)\n"
            "     --tcp-max n          : Set TCP port max (default base+99)\n"
            "     --udp-base n         : Set UDP port base (default 6000)\n"
            "     --udp-max n          : Set UDP port max (default base+199)"
            "     --rtp-base n         : Set RTP port base (default 5000)\n"
            "     --rtp-max n          : Set RTP port max (default base+199)"
            "     --rtp-tos n          : Set RTP packet IP TOS bits to n\n"
            "\n"
            "Debug options:\n"
#if PTRACING
            "  -t --trace              : Enable trace, use multiple times for more detail.\n"
            "  -o --output             : File for trace output, default is stderr.\n"
#endif
            "  -h --help               : This help message.\n"
            "\n"
            "\n"
            "Dial peer specification:\n"
"  General form is pattern=destination where pattern is a regular expression\n"
"  matching the incoming calls destination address and will translate it to\n"
"  the outgoing destination address for making an outgoing call. For example,\n"
"  picking up a PhoneJACK handset and dialling 2, 6 would result in an address\n"
"  of \"pots:26\" which would then be matched against, say, a spec of\n"
"  pots:26=h323:10.0.1.1, resulting in a call from the pots handset to\n"
"  10.0.1.1 using the H.323 protocol.\n"
"\n"
"  As the pattern field is a regular expression, you could have used in the\n"
"  above .*:26=h323:10.0.1.1 to achieve the same result with the addition that\n"
"  an incoming call from a SIP client would also be routed to the H.323 client.\n"
"\n"
"  Note that the pattern has an implicit ^ and $ at the beginning and end of\n"
"  the regular expression. So it must match the entire address.\n"
"\n"
"  If the specification is of the form @filename, then the file is read with\n"
"  each line consisting of a pattern=destination dial peer specification. Lines\n"
"  without and equal sign or beginning with '#' are ignored.\n"
"\n"
"  The standard dial peers that will be included are:\n"
"    If SIP is enabled:\n"
"      pots:.*\\*.*\\*.* = sip:<dn2ip>\n"
"      pots:.*         = sip:<da>\n"
"      pc:.*           = sip:<da>\n"
"\n"
"    If SIP is not enabled and H.323 is enabled:\n"
"      pots:.*\\*.*\\*.* = h323:<dn2ip>\n"
"      pots:.*         = h323:<da>\n"
"      pc:.*           = h323:<da>\n"
"\n"
"    If POTS is enabled:\n"
"      h323:.* = pots:<da>\n"
"      sip:.*  = pots:<da>\n"
"\n"
"    If POTS is not enabled and the PC sound system is enabled:\n"
"      h323:.* = pc:<da>\n"
"      sip:.*  = pc:<da>\n"
"\n"
#if P_EXPAT
"    If IVR is enabled then a # from any protocol will route it it, ie:\n"
"      .*:#  = ivr:\n"
"\n"
#endif
            << endl;
    return;
  }

#if PTRACING
  PTrace::Initialise(args.GetOptionCount('t'),
                     args.HasOption('o') ? (const char *)args.GetOptionString('o') : NULL,
                     PTrace::Timestamp|PTrace::Thread|PTrace::FileAndLine);
#endif

  // Create the Opal Manager and initialise it
  opal = new MyManager;

  if (opal->Initialise(args))
    opal->Main(args);

  delete opal;

  cout << "Exiting " << GetName() << endl;
}


///////////////////////////////////////////////////////////////

MyManager::MyManager()
{
  potsEP = NULL;
  pcssEP = NULL;
  h323EP = NULL;
  sipEP  = NULL;
#if P_EXPAT
  ivrEP  = NULL;
#endif
}


MyManager::~MyManager()
{
  // Must do this before we destroy the manager or a crash will result
  if (potsEP != NULL)
    potsEP->RemoveAllLines();
}


BOOL MyManager::Initialise(PArgList & args)
{
  // Set the various global options
  autoAnswer = args.HasOption('a');
  silenceOn  = !args.HasOption('e');

  // get the protocols in use
  BOOL useSIP  = !args.HasOption("no-sip");
  BOOL useH323 = !args.HasOption("no-h323");

  if (args.HasOption('j')) {
    unsigned minJitter;
    unsigned maxJitter;
    PStringArray delays = args.GetOptionString('j').Tokenise(",-");
    if (delays.GetSize() < 2) {
      maxJitter = delays[0].AsUnsigned();
      minJitter = PMIN(GetMinAudioJitterDelay(), maxJitter);
    }
    else {
      minJitter = delays[0].AsUnsigned();
      maxJitter = delays[1].AsUnsigned();
    }
    if (minJitter >= 20 && minJitter <= maxJitter && maxJitter <= 1000)
      SetAudioJitterDelay(minJitter, maxJitter);
    else {
      cerr << "Jitter should be between 20 and 1000 milliseconds." << endl;
      return FALSE;
    }
  }

  if (args.HasOption('D'))
    SetMediaFormatMask(args.GetOptionString('D').Lines());
  if (args.HasOption('P'))
    SetMediaFormatOrder(args.GetOptionString('P').Lines());

  if (args.HasOption("tcp-base"))
    SetTCPPorts(args.GetOptionString("tcp-base").AsUnsigned(),
                args.GetOptionString("tcp-max").AsUnsigned());

  if (args.HasOption("udp-base"))
    SetUDPPorts(args.GetOptionString("udp-base").AsUnsigned(),
                args.GetOptionString("udp-max").AsUnsigned());

  if (args.HasOption("rtp-base"))
    SetRtpIpPorts(args.GetOptionString("rtp-base").AsUnsigned(),
                  args.GetOptionString("rtp-max").AsUnsigned());

  if (args.HasOption("rtp-tos")) {
    unsigned tos = args.GetOptionString("rtp-tos").AsUnsigned();
    if (tos > 255) {
      cerr << "IP Type Of Service bits must be 0 to 255." << endl;
      return FALSE;
    }
    SetRtpIpTypeofService(tos);
  }

  cout << "Auto answer is " << (autoAnswer ? "on" : "off") << "\n"
          "Silence supression is " << (silenceOn ? "on" : "off") << "\n"
          "Jitter buffer: "  << GetMinAudioJitterDelay() << '-' << GetMaxAudioJitterDelay() << " ms\n"
          "Codecs removed: " << setfill(',') << GetMediaFormatMask() << "\n"
          "Codec order: " << setfill(',') << GetMediaFormatOrder() << setfill(' ') << "\n"
          "TCP ports: " << GetTCPPortBase() << '-' << GetTCPPortMax() << "\n"
          "UDP ports: " << GetUDPPortBase() << '-' << GetUDPPortMax() << "\n"
          "RTP ports: " << GetRtpIpPortBase() << '-' << GetRtpIpPortMax() << "\n"
          "RTP IP TOS: 0x" << hex << (unsigned)GetRtpIpTypeofService() << dec << endl;


  ///////////////////////////////////////
  // Open the LID if parameter provided, create LID based endpoint

#ifdef HAS_IXJ
  if (!args.HasOption('Q')) {
    PStringArray devices = args.GetOptionString('q').Lines();
    if (devices.IsEmpty() || devices[0] == "ALL")
      devices = OpalIxJDevice::GetDeviceNames();
    for (PINDEX d = 0; d < devices.GetSize(); d++) {
      OpalIxJDevice * ixj = new OpalIxJDevice;
      if (ixj->Open(devices[d])) {
        // Create LID protocol handler, automatically adds to manager
        if (potsEP == NULL)
          potsEP = new OpalPOTSEndPoint(*this);
        if (potsEP->AddDevice(ixj))
          cout << "Quicknet device \"" << devices[d] << "\" added." << endl;
      }
      else {
        cerr << "Could not open device \"" << devices[d] << '"' << endl;
        delete ixj;
      }
    }
  }
#endif


  ///////////////////////////////////////
  // Create PC Sound System handler

  if (!args.HasOption('S')) {
    pcssEP = new MyPCSSEndPoint(*this);

    if (!pcssEP->SetSoundDevice(args, "sound", PSoundChannel::Recorder))
      return FALSE;
    if (!pcssEP->SetSoundDevice(args, "sound", PSoundChannel::Player))
      return FALSE;
    if (!pcssEP->SetSoundDevice(args, "sound-in", PSoundChannel::Recorder))
      return FALSE;
    if (!pcssEP->SetSoundDevice(args, "sound-out", PSoundChannel::Player))
      return FALSE;

    cout << "Sound output device: \"" << pcssEP->GetSoundChannelPlayDevice() << "\"\n"
            "Sound  input device: \"" << pcssEP->GetSoundChannelRecordDevice() << '"' << endl;
  }


  ///////////////////////////////////////
  // Create H.323 protocol handler

  if (useH323) {
    h323EP = new H323EndPoint(*this);

    noFastStart      = args.HasOption('f');
    noH245Tunnelling = args.HasOption('T');

    // Get local username, multiple uses of -u indicates additional aliases
    if (args.HasOption('u')) {
      PStringArray aliases = args.GetOptionString('u').Lines();
      h323EP->SetLocalUserName(aliases[0]);
      for (PINDEX i = 1; i < aliases.GetSize(); i++)
        h323EP->AddAliasName(aliases[i]);
    }

    if (args.HasOption('b')) {
      unsigned initialBandwidth = args.GetOptionString('b').AsUnsigned()*100;
      if (initialBandwidth == 0) {
        cerr << "Illegal bandwidth specified." << endl;
        return FALSE;
      }
      h323EP->SetInitialBandwidth(initialBandwidth);
    }

    h323EP->SetGkAccessTokenOID(args.GetOptionString("gk-token"));

    cout << "H.323 Local username: " << h323EP->GetLocalUserName() << "\n"
         << "H.323 FastConnect is " << !noFastStart << "\n"
         << "H.323 H245Tunnelling is " << noH245Tunnelling << "\n"
         << "H.323 gk Token OID is " << h323EP->GetGkAccessTokenOID() << endl;


    // Start the listener thread for incoming calls.
    if (args.HasOption("h323-listen")) {
      PStringArray listeners = args.GetOptionString("h323-listen").Lines();
      if (!h323EP->StartListeners(listeners)) {
        cerr <<  "Could not open any H.323 listener from "
             << setfill(',') << listeners << endl;
        return FALSE;
      }
    }
    else {
      if (!h323EP->StartListener("*")) {
        cerr <<  "Could not open H.323 listener on TCP port "
             << h323EP->GetDefaultSignalPort() << endl;
        return FALSE;
      }
    }


    if (args.HasOption('p'))
      h323EP->SetGatekeeperPassword(args.GetOptionString('p'));

    // Establish link with gatekeeper if required.
    if (args.HasOption('g')) {
      PString gkName = args.GetOptionString('g');
      OpalTransportUDP * rasChannel;
      if (args.GetOptionString("h323-listen").IsEmpty())
        rasChannel  = new OpalTransportUDP(*h323EP);
      else {
        PIPSocket::Address interfaceAddress(args.GetOptionString("h323-listen"));
        rasChannel  = new OpalTransportUDP(*h323EP, interfaceAddress);
      }
      if (h323EP->SetGatekeeper(gkName, rasChannel))
        cout << "Gatekeeper set: " << *h323EP->GetGatekeeper() << endl;
      else {
        cerr << "Error registering with gatekeeper at \"" << gkName << '"' << endl;
        return FALSE;
      }
    }
    else if (!args.HasOption('n') || args.HasOption('R')) {
      cout << "Searching for gatekeeper..." << flush;
      if (h323EP->DiscoverGatekeeper(new OpalTransportUDP(*h323EP)))
        cout << "\nGatekeeper found: " << *h323EP->GetGatekeeper() << endl;
      else {
        cerr << "\nNo gatekeeper found." << endl;
        if (args.HasOption("require-gatekeeper")) 
          return FALSE;
      }
    }
  }

  ///////////////////////////////////////
  // Create SIP protocol handler

  if (useSIP) {
    sipEP = new SIPEndPoint(*this);

    // set MIME format
    sipEP->SetMIMEForm(args.HasOption("use-long-mime"));

    // Get local username, multiple uses of -u indicates additional aliases
    if (args.HasOption('u')) {
      PStringArray aliases = args.GetOptionString('u').Lines();
      sipEP->SetRegistrationName(aliases[0]);
    }
    if (args.HasOption('p'))
      sipEP->SetRegistrationPassword(args.GetOptionString('p'));

    // Start the listener thread for incoming calls.
    if (args.HasOption("sip-listen")) {
      PStringArray listeners = args.GetOptionString("sip-listen").Lines();
      if (!sipEP->StartListeners(listeners)) {
        cerr <<  "Could not open any SIP listener from "
             << setfill(',') << listeners << endl;
        return FALSE;
      }
    }
    else {
      if (!sipEP->StartListener("udp$*")) {
        cerr <<  "Could not open SIP listener on UDP port "
             << sipEP->GetDefaultSignalPort() << endl;
        return FALSE;
      }
      if (!sipEP->StartListener("tcp$*")) {
        cerr <<  "Could not open SIP listener on TCP port "
             << sipEP->GetDefaultSignalPort() << endl;
        return FALSE;
      }
    }

    if (args.HasOption('r'))
      sipEP->Register(args.GetOptionString('r'));
  }


#if P_EXPAT
  ///////////////////////////////////////
  // Create IVR protocol handler

  if (!args.HasOption('V')) {
    ivrEP = new OpalIVREndPoint(*this);
    if (args.HasOption('x'))
      ivrEP->SetDefaultVXML(args.GetOptionString('x'));
  }
#endif


  ///////////////////////////////////////
  // Set the dial peers

  if (args.HasOption('d')) {
    if (!SetRouteTable(args.GetOptionString('d').Lines())) {
      cerr <<  "No legal entries in dial peer!" << endl;
      return FALSE;
    }
  }

  if (!args.HasOption("no-std-dial-peer")) {
    if (sipEP != NULL) {
      AddRouteEntry("pots:.*\\*.*\\*.* = sip:<dn2ip>");
      AddRouteEntry("pots:.*           = sip:<da>");
      AddRouteEntry("pc:.*             = sip:<da>");
    }
    else if (h323EP != NULL) {
      AddRouteEntry("pots:.*\\*.*\\*.* = h323:<dn2ip>");
      AddRouteEntry("pots:.*           = h323:<da>");
      AddRouteEntry("pc:.*             = h323:<da>");
    }

#if P_EXPAT
    if (ivrEP != NULL)
      AddRouteEntry(".*:#  = ivr:"); // A hash from anywhere goes to IVR
#endif

    if (potsEP != NULL) {
      AddRouteEntry("h323:.* = pots:<da>");
      AddRouteEntry("sip:.*  = pots:<da>");
    }
    else if (pcssEP != NULL) {
      AddRouteEntry("h323:.* = pc:<da>");
      AddRouteEntry("sip:.*  = pc:<da>");
    }
  }

  return TRUE;
}


void MyManager::Main(PArgList & args)
{
  // See if making a call or just listening.
  if (args.HasOption('l'))
    cout << "Waiting for incoming calls\n";
  else if (args.GetCount() == 1) {
    cout << "Initiating call to \"" << args[0] << "\"\n";
    if (potsEP != NULL)
      SetUpCall("pots:*", args[0], currentCallToken);
    else
      SetUpCall("pc:*", args[0], currentCallToken);
  }
  else {
    cout << "Initiating call from \"" << args[0] << "\"to \"" << args[1] << "\"\n";
    SetUpCall(args[0], args[1], currentCallToken);
  }

  // see if a proxy/gateway has been specified
  if (args.HasOption('G')) {
    gateway = args.GetOptionString('G');
  }

  cout << "Press X to exit." << endl;

  // Simplest possible user interface
  for (;;) {
    cout << "OPAL> " << flush;
    PCaselessString cmd;
    cin >> cmd;
    if (cmd == "x")
      break;

    if (pcssEP != NULL && !pcssEP->incomingConnectionToken) {
      if (cmd == "n")
        pcssEP->ClearCall(pcssEP->incomingConnectionToken, OpalConnection::EndedByRefusal);
      else if (cmd == "y")
        pcssEP->AcceptIncomingConnection(pcssEP->incomingConnectionToken);
    }

    // Process commands
  }
}


void MyManager::OnEstablishedCall(OpalCall & call)
{
  currentCallToken = call.GetToken();
  cout << "In call with " << call.GetPartyB() << " using " << call.GetPartyA() << endl;
}


void MyManager::OnClearedCall(OpalCall & call)
{
  if (currentCallToken == call.GetToken())
    currentCallToken = PString();

  PString remoteName = '"' + call.GetPartyB() + '"';
  switch (call.GetCallEndReason()) {
    case OpalConnection::EndedByRemoteUser :
      cout << remoteName << " has cleared the call";
      break;
    case OpalConnection::EndedByCallerAbort :
      cout << remoteName << " has stopped calling";
      break;
    case OpalConnection::EndedByRefusal :
      cout << remoteName << " did not accept your call";
      break;
    case OpalConnection::EndedByNoAnswer :
      cout << remoteName << " did not answer your call";
      break;
    case OpalConnection::EndedByTransportFail :
      cout << "Call with " << remoteName << " ended abnormally";
      break;
    case OpalConnection::EndedByCapabilityExchange :
      cout << "Could not find common codec with " << remoteName;
      break;
    case OpalConnection::EndedByNoAccept :
      cout << "Did not accept incoming call from " << remoteName;
      break;
    case OpalConnection::EndedByAnswerDenied :
      cout << "Refused incoming call from " << remoteName;
      break;
    case OpalConnection::EndedByNoUser :
      cout << "Gatekeeper could find user " << remoteName;
      break;
    case OpalConnection::EndedByNoBandwidth :
      cout << "Call to " << remoteName << " aborted, insufficient bandwidth.";
      break;
    case OpalConnection::EndedByUnreachable :
      cout << remoteName << " could not be reached.";
      break;
    case OpalConnection::EndedByNoEndPoint :
      cout << "No phone running for " << remoteName;
      break;
    case OpalConnection::EndedByHostOffline :
      cout << remoteName << " is not online.";
      break;
    case OpalConnection::EndedByConnectFail :
      cout << "Transport error calling " << remoteName;
      break;
    default :
      cout << "Call with " << remoteName << " completed";
  }
  PTime now;
  cout << ", on " << now.AsString("w h:mma") << ". Duration "
       << setprecision(0) << setw(5) << (now - call.GetStartTime())
       << "s." << endl;
}


BOOL MyManager::OnOpenMediaStream(OpalConnection & connection,
                                  OpalMediaStream & stream)
{
  if (!OpalManager::OnOpenMediaStream(connection, stream))
    return FALSE;

  cout << connection.GetEndPoint().GetPrefixName() << " started ";

  if (stream.IsSource())
    cout << "sending ";
  else
    cout << "receiving ";

  cout << stream.GetMediaFormat() << endl;  

  return TRUE;
}



void MyManager::OnUserInputString(OpalConnection & connection, const PString & value)
{
  cout << "User input received: \"" << value << '"' << endl;
  OpalManager::OnUserInputString(connection, value);
}


///////////////////////////////////////////////////////////////

MyPCSSEndPoint::MyPCSSEndPoint(MyManager & mgr)
  : OpalPCSSEndPoint(mgr)
{
}


PString MyPCSSEndPoint::OnGetDestination(const OpalPCSSConnection & /*connection*/)
{
  PString destination;

  if (destinationAddress.IsEmpty()) {
    cout << "Enter destination address? " << flush;
    cin >> destination;
  }
  else {
    destination = destinationAddress;
    destinationAddress = PString();
  }

  return destination;
}


void MyPCSSEndPoint::OnShowIncoming(const OpalPCSSConnection & connection)
{
  PTime now;
  cout << "\nCall on " << now.AsString("w h:mma")
       << " from " << connection.GetRemotePartyName()
       << ", answer (Y/N)? " << flush;

  incomingConnectionToken = connection.GetToken();
}


BOOL MyPCSSEndPoint::OnShowOutgoing(const OpalPCSSConnection & connection)
{
  PTime now;
  cout << connection.GetRemotePartyName() << " is ringing on "
       << now.AsString("w h:mma") << " ..." << endl;
  return TRUE;
}


BOOL MyPCSSEndPoint::SetSoundDevice(PArgList & args,
                                    const char * optionName,
                                    PSoundChannel::Directions dir)
{
  if (!args.HasOption(optionName))
    return TRUE;

  PString dev = args.GetOptionString(optionName);

  if (dir == PSoundChannel::Player) {
    if (SetSoundChannelPlayDevice(dev))
      return TRUE;
  }
  else {
    if (SetSoundChannelRecordDevice(dev))
      return TRUE;
  }

  cerr << "Device for " << optionName << " (\"" << dev << "\") must be one of:\n";

  PStringArray names = PSoundChannel::GetDeviceNames(dir);
  for (PINDEX i = 0; i < names.GetSize(); i++)
    cerr << "  \"" << names[i] << "\"\n";

  return FALSE;
}


// End of File ///////////////////////////////////////////////////////////////
