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
 * Revision 1.2005  2001/08/17 08:35:41  robertj
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

#include <h323/h323ep.h>
#include <h323/gkclient.h>
#include <lids/lidep.h>
#include <lids/ixjlid.h>

#include "main.h"
#include "version.h"


#ifdef OPAL_STATIC_LINK
#include <codec/allcodecs.h>
#endif


#define new PNEW


PCREATE_PROCESS(SimpleOpalProcess);


///////////////////////////////////////////////////////////////

SimpleOpalProcess::SimpleOpalProcess()
  : PProcess("Open Phone Abstraction Library", "SimpleOpal",
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
             "B-forward-busy:"
             "D-disable:"
             "e-silence."
             "f-fast-disable."
             "g-gatekeeper:"
             "h-help."
             "H-no-h323."
             "i-interface:"
             "j-jitter:"
             "l-listen."
             "n-no-gatekeeper."
#if PTRACING
             "o-output:"
#endif
             "P-prefer:"
#ifdef P_SSL
             "p-password:"
#endif
             "q-quicknet:"
             "Q-no-quicknet:"
             "r-require-gatekeeper."
             "s-sound:"
             "S-no-sound:"
             "-sound-in:"
             "-sound-out:"
             "T-h245tunneldisable."
#if PTRACING
             "t-trace."
#endif
             "u-user:"
          , FALSE);


  if (args.HasOption('h') || (!args.HasOption('l') && args.GetCount() == 0)) {
    cout << "Usage : " << GetName() << " [options] -l\n"
            "      : " << GetName() << " [options] [alias@]hostname   (no gatekeeper)\n"
            "      : " << GetName() << " [options] alias[@hostname]   (with gatekeeper)\n"
            "Options:\n"
            "  -l --listen             : Listen for incoming calls.\n"
            "  -g --gatekeeper host    : Specify gatekeeper host.\n"
            "  -n --no-gatekeeper      : Disable gatekeeper discovery.\n"
            "  -r --require-gatekeeper : Exit if gatekeeper discovery fails.\n"
            "  -a --auto-answer        : Automatically answer incoming calls.\n"
            "  -u --user name          : Set local alias name(s) (defaults to login name).\n"
            "  -b --bandwidth bps      : Limit bandwidth usage to bps bits/second.\n"
            "  -j --jitter delay       : Set jitter buffer to delay milliseconds.\n"
            "  -D --disable codec      : Disable the specified codec (may be used multiple times)\n"
            "  -P --prefer codec       : Prefer the specified codec (may be used multiple times)\n"
            "  -i --interface ipnum    : Select interface to bind to.\n"
            "  -B --forward-busy party : Forward to remote party if busy.\n"
            "  -e --silence            : Disable transmitter silence detection.\n"
            "  -f --fast-disable       : Disable fast start.\n"
            "  -T --h245tunneldisable  : Disable H245 tunnelling.\n"
            "  -q --quicknet device    : Select Quicknet xJACK device (default ALL).\n"
            "  -Q --no-quicknet        : Do not use Quicknet xJACK device.\n"
            "  -s --sound device       : Select sound input/output device.\n"
            "  -S --no-sound           : Do not use sound input/output device.\n"
            "     --sound-in device    : Select sound input device.\n"
            "     --sound-out device   : Select sound output device.\n"
#if PTRACING
            "  -t --trace              : Enable trace, use multiple times for more detail.\n"
            "  -o --output             : File for trace output, default is stderr.\n"
#endif
            "  -h --help               : This help message.\n"
            << endl;
    return;
  }

#if PTRACING
  PTrace::Initialise(args.GetOptionCount('t'),
                     args.HasOption('o') ? (const char *)args.GetOptionString('o') : NULL);
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
  autoAnswer           = args.HasOption('a');
  silenceOn            = !args.HasOption('e');
  busyForwardParty     = args.GetOptionString('B');

  if (args.HasOption('j')) {
    unsigned jitter = args.GetOptionString('j').AsUnsigned();
    if (jitter >= 20 && jitter <= 10000)
      SetMaxAudioDelayJitter(jitter);
    else {
      cerr << "Jitter should be between 20 milliseconds and 10 seconds." << endl;
      return FALSE;
    }
  }

  if (args.HasOption('D'))
    SetMediaFormatMask(args.GetOptionString('D').Lines());
  if (args.HasOption('P'))
    SetMediaFormatOrder(args.GetOptionString('P').Lines());

  cout << "Auto answer is " << (autoAnswer ? "on" : "off") << "\n"
          "Silence supression is " << (silenceOn ? "on" : "off") << "\n"
          "Jitter buffer: "  << GetMaxAudioDelayJitter() << " ms\n"
          "Codecs removed: " << setfill(',') << GetMediaFormatMask() << "\n"
          "Codec order: " << setfill(',') << GetMediaFormatOrder() << setfill(' ') << endl;


  ///////////////////////////////////////
  // Open the LID if parameter provided, create LID based endpoint

  if (!args.HasOption('Q')) {
    PString device = args.GetOptionString('q');
    if (device.IsEmpty() || device == "ALL") {
      PStringArray devices = OpalIxJDevice::GetDeviceNames();
      if (devices.GetSize() > 0)
        device = devices[0];
      else
        device = PString();
    }
    if (!device) {
      OpalIxJDevice * ixj = new OpalIxJDevice;
      if (ixj->Open(device)) {
        // Create LID protocol handler, automatically adds to manager
        potsEP = new OpalPOTSEndPoint(*this);
        if (potsEP->AddDevice(ixj))
          cout << "Quicknet device is " << device << endl;
        else {
          RemoveEndPoint(potsEP);
          potsEP = NULL;
        }
      }
      else {
        cerr << "Could not open device \"" << device << '"' << endl;
        delete ixj;
      }
    }
  }


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

  h323EP = new H323EndPoint(*this);

  noFastStart      = args.HasOption('f');
  noH245Tunnelling = args.HasOption('h');

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

  // .
  cout << "H.323 Local username: " << h323EP->GetLocalUserName() << "\n"
       << "H.323 FastConnect is " << !noFastStart << "\n"
       << "H.323 H245Tunnelling is " << noH245Tunnelling << endl;


  // Start the listener thread for incoming calls.
  if (args.HasOption('i')) {
    PStringArray listeners = args.GetOptionString('i').Lines();
    if (!h323EP->StartListeners(listeners)) {
      cerr <<  "Could not open any H.323 listener from "
           << setfill(',') << listeners << endl;
      return FALSE;
    }
  }
  else {
    if (!h323EP->StartListener(NULL)) {
      cerr <<  "Could not open H.323 listener on TCP port "
           << h323EP->GetDefaultSignalPort() << endl;
      return FALSE;
    }
  }


  // Establish link with gatekeeper if required.
  if (args.HasOption('g')) {
    PString gkName = args.GetOptionString('g');
    OpalTransportUDP * rasChannel;
    if (args.GetOptionString('i').IsEmpty())
      rasChannel  = new OpalTransportUDP(*h323EP);
    else {
      PIPSocket::Address interfaceAddress(args.GetOptionString('i'));
      rasChannel  = new OpalTransportUDP(*h323EP, interfaceAddress);
    }
    if (h323EP->SetGatekeeper(gkName, rasChannel))
      cout << "Gatekeeper set: " << *h323EP->GetGatekeeper() << endl;
    else {
      cerr << "Error registering with gatekeeper at \"" << gkName << '"' << endl;
      return FALSE;
    }
  }
  else if (!args.HasOption('n') || args.HasOption('r')) {
    cout << "Searching for gatekeeper..." << flush;
    if (h323EP->DiscoverGatekeeper(new OpalTransportUDP(*h323EP)))
      cout << "\nGatekeeper found: " << *h323EP->GetGatekeeper() << endl;
    else {
      cerr << "\nNo gatekeeper found." << endl;
      if (args.HasOption("require-gatekeeper")) 
        return FALSE;
    }
  }

  return TRUE;
}


void MyManager::Main(PArgList & args)
{
  // See if making a call or just listening.
  if (args.HasOption('l'))
    cout << "Waiting for incoming calls\n";
  else {
    cout << "Initiating call to \"" << args[0] << "\"\n";
    if (potsEP != NULL)
      SetUpCall("pots:*", args[0], currentCallToken);
    else
      SetUpCall("pc:*", args[0], currentCallToken);
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
        pcssEP->ClearCall(pcssEP->incomingConnectionToken, EndedByRefusal);
      else if (cmd == "y")
        pcssEP->AcceptIncomingConnection(pcssEP->incomingConnectionToken);
    }

    // Process commands
  }
}


PString MyManager::OnRouteConnection(OpalConnection & connection)
{
  PString addr = connection.GetDestinationAddress();
  if (addr.IsEmpty())
    return addr;

  // Have got explicit protocol defined
  if (addr.Find(':') != P_MAX_INDEX)
    return addr;

  // Default to H323 if loaded, or not come from the H.323 endpoint
  if (h323EP != NULL && connection.GetEndPoint().GetPrefixName() != "h323") {
    addr.Replace("*", ".", TRUE);
    return "h323:" + addr;
  }

  // Default to local phone, if loaded
  if (potsEP != NULL)
    return "pots:" + addr;

  // Finally route it to the PC
  return "pc:" + addr;
}


#if 0
BOOL MyManager::OnIncomingConnection(OpalConnection & connection)
{
  if (currentCallToken.IsEmpty()) {
    currentCallToken = connection.GetToken();

    if (autoAnswer) {
      cout << "Automatically accepting call." << endl;
      return OpalConnection::AnswerCallNow;
    }

    cout << "Incoming call from \""
         << connection.GetRemotePartyName()
         << "\", answer call (Y/n)? "
         << flush;

    return OpalConnection::AnswerCallPending;
  }

  if (busyForwardParty.IsEmpty()) {
    cout << "Incoming call from \"" << connection.GetRemotePartyName() << "\" rejected, line busy!" << endl;
    return OpalConnection::AnswerCallDenied;
  }

  cout << "Forwarding call to \"" << busyForwardParty << "\"." << endl;
  if (connection.ForwardCall(busyForwardParty))
    connection.ClearCall(OpalConnection::EndedByCallForwarded);

  return OpalConnection::AnswerCallDenied;
}
BOOL MyManager::OnConnectionForwarded(OpalConnection & /*connection*/,
                                      const PString & forwardParty)
{
  cout << "Call is being forwarded to host " << forwardParty << endl;
  return TRUE;
}
#endif


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
    case EndedByRemoteUser :
      cout << remoteName << " has cleared the call";
      break;
    case EndedByCallerAbort :
      cout << remoteName << " has stopped calling";
      break;
    case EndedByRefusal :
      cout << remoteName << " did not accept your call";
      break;
    case EndedByNoAnswer :
      cout << remoteName << " did not answer your call";
      break;
    case EndedByTransportFail :
      cout << "Call with " << remoteName << " ended abnormally";
      break;
    case EndedByCapabilityExchange :
      cout << "Could not find common codec with " << remoteName;
      break;
    case EndedByNoAccept :
      cout << "Did not accept incoming call from " << remoteName;
      break;
    case EndedByAnswerDenied :
      cout << "Refused incoming call from " << remoteName;
      break;
    case EndedByNoUser :
      cout << "Gatekeeper could find user " << remoteName;
      break;
    case EndedByNoBandwidth :
      cout << "Call to " << remoteName << " aborted, insufficient bandwidth.";
      break;
    case EndedByUnreachable :
      cout << remoteName << " could not be reached.";
      break;
    case EndedByNoEndPoint :
      cout << "No phone running for " << remoteName;
      break;
    case EndedByHostOffline :
      cout << remoteName << " is not online.";
      break;
    case EndedByConnectFail :
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

  cout << "Started ";

  if (stream.IsSource())
    cout << "sending ";
  else
    cout << "receiving ";

  cout << stream.GetMediaFormat() << endl;  

  return TRUE;
}



void MyManager::OnUserIndicationString(OpalConnection & connection,
                                       const PString & value)
{
  cout << "User input received: \"" << value << '"' << endl;
  OpalManager::OnUserIndicationString(connection, value);
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
