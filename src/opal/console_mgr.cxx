/*
 * console_mgs.cxx
 *
 * An OpalManager derived class for use in a console application, providing
 * a standard set of command line arguments for configuring many system
 * parameters. Used by the sample applications such as faxopal, ovropal etc.
 *
 * Copyright (c) 2010 Vox Lucida Pty. Ltd.
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
 * The Initial Developer of the Original Code is Vox Lucida Pty. Ltd.
 *
 * Contributor(s): ______________________________________.
 *
 * $Revision$
 * $Author$
 * $Date$
 */

#include <ptlib.h>

#ifdef __GNUC__
#pragma implementation "console_mgr.h"
#endif

#include <opal/console_mgr.h>
#include <sip/sipep.h>
#include <h323/h323ep.h>
#include <lids/lidep.h>


OpalManagerConsole::OpalManagerConsole()
{
}


PString OpalManagerConsole::GetArgumentSpec() const
{
  return "D-disable:"
         "g-gk-host:"
         "G-gk-id:"
         "h-help."
         "H-h323:"
         "L-lines:"
#if PTRACING
         "o-output:"             "-no-output."
#endif
         "p-password:"
         "P-prefer:"
         "r-register:"
         "S-sip:"
#if PTRACING
         "t-trace."              "-no-trace."
#endif
         "u-user:"

         "-inband-detect."
         "-inband-send."
         "-country:"
         "-no-fast."
         "-proxy:"
         "-sip-ui:"
         "-register-ttl:"
         "-register-mode:"
         "-translate:"
         "-portbase:"
         "-portmax:"
         "-tcp-base:"
         "-tcp-max:"
         "-udp-base:"
         "-udp-max:"
         "-rtp-base:"
         "-rtp-max:" 
         "-rtp-tos:"
         "-rtp-size:"
         "-stun:"
         "-tel:";
}


PString OpalManagerConsole::GetArgumentUsage() const
{
  return "Global options:\n"
         "  -u or --user name       : Set local username, defaults to OS username.\n"
         "  -p or --password pwd    : Set password for authentication.\n"
         "  -D or --disable codec   : Disable use of specified codec.\n"
         "  -P or --prefer codec    : Set preference order for codecs.\n"
         "        --inband-detect   : Disable detection of in-band tones.\n"
         "        --inband-send     : Disable transmission of in-band tones.\n"
         "        --tel proto       : Protocol to use for tel: URI, e.g. sip\n"
         "\n"

#if OPAL_SIP
         "SIP options:\n"
         "  -S or --sip interface   : SIP listens on interface, defaults to udp$*:5060, 'x' disables.\n"
         "  -r or --register server : SIP registration to server.\n"
         "        --register-ttl n  : SIP registration Time To Live, default 300 seconds.\n"
         "        --register-mode m : SIP registration mode (normal, single, public).\n"
         "        --proxy url       : SIP outbound proxy.\n"
         "        --sip-ui mode     : SIP User Indication mode (inband,rfc2833,info-tone,info-string)\n"
#endif
         "\n"

#if OPAL_H323
         "H.323 options:\n"
         "  -H or --h323 interface  : H.323 listens on interface, defaults to tcp$*:1720, 'x' disables.\n"
         "  -g or --gk-host host    : H.323 gatekeeper host.\n"
         "  -G or --gk-id id        : H.323 gatekeeper identifier.\n"
         "        --no-fast         : H.323 fast connect disabled.\n"
#endif
         "\n"

#if OPAL_LID
         "Line Interface options:\n"
         "  -L or --lines devices   : Set Line Interface Devices, 'x' disables.\n"
         "        --country code    : Select country to use for LID (eg \"US\", \"au\" or \"+61\").\n"
#endif
         "\n"

         "IP options:\n"
         "     --translate ip       : Set external IP address if masqueraded\n"
         "     --portbase n         : Set TCP/UDP/RTP port base\n"
         "     --portmax n          : Set TCP/UDP/RTP port max\n"
         "     --tcp-base n         : Set TCP port base (default 0)\n"
         "     --tcp-max n          : Set TCP port max (default base+99)\n"
         "     --udp-base n         : Set UDP port base (default 6000)\n"
         "     --udp-max n          : Set UDP port max (default base+199)\n"
         "     --rtp-base n         : Set RTP port base (default 5000)\n"
         "     --rtp-max n          : Set RTP port max (default base+199)\n"
         "     --rtp-tos n          : Set RTP packet IP TOS bits to n\n"
         "     --rtp-size size      : Set RTP maximum payload size in bytes.\n"
         "     --stun server        : Set NAT traversal STUN server\n"
         "\n"
         "Debug:\n"
#if PTRACING
         "  -o or --output file     : file name for output of log messages\n"       
         "  -t or --trace           : verbosity in error log (more times for more detail)\n"     
#endif
         "  -h or --help            : This help message.\n"
            ;
}


bool OpalManagerConsole::Initialise(PArgList & args, bool verbose)
{
#if PTRACING
  PTrace::Initialise(args.GetOptionCount('t'),
                     args.HasOption('o') ? (const char *)args.GetOptionString('o') : NULL,
                PTrace::Blocks | PTrace::Timestamp | PTrace::Thread | PTrace::FileAndLine);
#endif

  if (args.HasOption("translate")) {
    SetTranslationAddress(args.GetOptionString("translate"));
    if (verbose)
      cout << "External address set to " << GetTranslationAddress() << '\n';
  }

  if (args.HasOption("portbase")) {
    unsigned portbase = args.GetOptionString("portbase").AsUnsigned();
    unsigned portmax  = args.GetOptionString("portmax").AsUnsigned();
    SetTCPPorts  (portbase, portmax);
    SetUDPPorts  (portbase, portmax);
    SetRtpIpPorts(portbase, portmax);
  }

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
      cerr << "IP Type Of Service bits must be 0 to 255.\n";
      return false;
    }
    SetMediaTypeOfService(tos);
  }

  if (args.HasOption("rtp-size")) {
    unsigned size = args.GetOptionString("rtp-size").AsUnsigned();
    if (size < 32 || size > 65500) {
      cerr << "RTP maximum payload size 32 to 65500.\n";
      return false;
    }
    SetMaxRtpPayloadSize(size);
  }

  if (verbose)
    cout << "TCP ports: " << GetTCPPortBase() << '-' << GetTCPPortMax() << "\n"
            "UDP ports: " << GetUDPPortBase() << '-' << GetUDPPortMax() << "\n"
            "RTP ports: " << GetRtpIpPortBase() << '-' << GetRtpIpPortMax() << "\n"
            "RTP IP TOS: 0x" << hex << (unsigned)GetMediaTypeOfService() << dec << "\n"
            "RTP payload size: " << GetMaxRtpPayloadSize() << "\n"
            "STUN server: " << flush;

  if (args.HasOption("stun")) {
    PSTUNClient::NatTypes nat = SetSTUNServer(args.GetOptionString("stun"));
    if (verbose) {
      cout << "STUN server \"" << GetSTUNClient()->GetServer() << "\" replies " << nat;
      PIPSocket::Address externalAddress;
      if (nat != PSTUNClient::BlockedNat && GetSTUNClient()->GetExternalAddress(externalAddress))
        cout << " with address " << externalAddress;
      cout << endl;
    }
  }

  if (args.HasOption('u'))
    SetDefaultUserName(args.GetOptionString('u'));

  if (verbose) {
    PIPSocket::InterfaceTable interfaceTable;
    if (PIPSocket::GetInterfaceTable(interfaceTable))
      cout << "Detected " << interfaceTable.GetSize() << " network interfaces:\n"
                << setfill('\n') << interfaceTable << setfill(' ') << endl;
  }

  PCaselessString interfaces;

#if OPAL_SIP
  // Set up SIP
  interfaces = args.GetOptionString('S');
  if (interfaces != "x") {
    SIPEndPoint * sip  = CreateSIPEndPoint();
    if (!sip->StartListeners(interfaces.Lines())) {
      cerr << "Could not start SIP listeners." << endl;
      return false;
    }

    if (args.HasOption("sip-ui")) {
      PCaselessString str = args.GetOptionString("sip-ui");
      if (str == "inband")
        sip->SetSendUserInputMode(OpalConnection::SendUserInputInBand);
      else if (str == "rfc2833")
        sip->SetSendUserInputMode(OpalConnection::SendUserInputAsRFC2833);
      else if (str == "info-tone")
        sip->SetSendUserInputMode(OpalConnection::SendUserInputAsTone);
      else if (str == "info-string")
        sip->SetSendUserInputMode(OpalConnection::SendUserInputAsString);
      else {
        cerr << "Unknown SIP user indication mode \"" << str << '"' << endl;
        return false;
      }
    }

    if (args.HasOption('P'))
      sip->SetProxy(args.GetOptionString('P'), args.GetOptionString('u'), args.GetOptionString('p'));

    if (args.HasOption('r')) {
      SIPRegister::Params params;
      params.m_addressOfRecord = args.GetOptionString('r');
      params.m_password = args.GetOptionString('p');
      params.m_expire = args.GetOptionString("register-ttl", "300").AsUnsigned();
      if (params.m_expire < 30) {
        cerr << "SIP registrar Time To Live must be more than 30 seconds" << endl;
        return false;
      }
      if (args.HasOption("register-mode")) {
        PCaselessString str = args.GetOptionString("register-mode");
        if (str == "normal")
          params.m_compatibility = SIPRegister::e_FullyCompliant;
        else if (str == "single")
          params.m_compatibility = SIPRegister::e_CannotRegisterMultipleContacts;
        else if (str == "public")
          params.m_compatibility = SIPRegister::e_CannotRegisterPrivateContacts;
        else {
          cerr << "Unknown SIP registration mode \"" << str << '"' << endl;
          return false;
        }
      }

      PString aor;
      if (!sip->Register(params, aor, false)) {
        cerr << "Could not do SIP registration to " << params.m_addressOfRecord << endl;
        return false;
      }
    }
  }
#endif // OPAL_SIP


#if OPAL_H323
  // Set up H.323
  interfaces = args.GetOptionString('H');
  if (interfaces != "x") {
    H323EndPoint * h323 = CreateH323EndPoint();
    if (!h323->StartListeners(interfaces.Lines())) {
      cerr << "Could not start H.323 listeners." << endl;
      return false;
    }

    h323->DisableFastStart(args.HasOption("no-fast"));

    if (args.HasOption('g') || args.HasOption('G')) {
      if (!h323->UseGatekeeper(args.GetOptionString('g'), args.GetOptionString('G'))) {
        cerr << "Could not complete gatekeeper registration" << endl;
        return false;
      }
    }
  }
#endif // OPAL_H323

#if OPAL_LID
  // If we have LIDs speficied in command line, load them
  if (args.HasOption('L')) {
    OpalLineEndPoint * lines = CreateLineEndPoint();
    if (!lines->AddDeviceNames(args.GetOptionString('L').Lines())) {
      cerr << "Could not start Line Interface Device(s)" << endl;
      return false;
    }

    PString country = args.GetOptionString("country");
    if (!country.IsEmpty()) {
      if (!lines->SetCountryCodeName(country))
        cerr << "Could not set LID to country name \"" << country << '"' << endl;
    }
  }
#endif // OPAL_LID

  if (args.HasOption('D'))
    SetMediaFormatMask(args.GetOptionString('D').Lines());
  if (args.HasOption('P'))
    SetMediaFormatOrder(args.GetOptionString('P').Lines());

  PString telProto = args.GetOptionString("tel");
  if (!telProto.IsEmpty()) {
    OpalEndPoint * ep = FindEndPoint(telProto);
    if (ep != NULL)
      AttachEndPoint(ep, "tel");
    else
      cerr << "The \"tel\" URI cannot be mapped to protocol \"" << telProto << '"' << endl;
  }

  return true;
}


#if OPAL_SIP
SIPEndPoint * OpalManagerConsole::CreateSIPEndPoint()
{
  return new SIPEndPoint(*this);
}
#endif // OPAL_SIP


#if OPAL_H323
H323EndPoint * OpalManagerConsole::CreateH323EndPoint()
{
  return new H323EndPoint(*this);
}
#endif // OPAL_H323


#if OPAL_LID
OpalLineEndPoint * OpalManagerConsole::CreateLineEndPoint()
{
  return new OpalLineEndPoint(*this);
}
#endif // OPAL_LID


/////////////////////////////////////////////////////////////////////////////
