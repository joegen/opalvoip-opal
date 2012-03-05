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
#include <h323/gkclient.h>
#include <lids/lidep.h>
#include <lids/capi_ep.h>


OpalManagerConsole::OpalManagerConsole()
{
}


PString OpalManagerConsole::GetArgumentSpec() const
{
  return "u-user:"
         "p-password:"
         "D-disable:"
         "P-prefer:"
         "O-option:"
         "-inband-detect."
         "-inband-send."
         "-tel:"
#if OPAL_SIP
         "-no-sip."
         "S-sip:"
         "r-register:"
         "-register-auth-id:"
         "-register-proxy:"
         "-register-ttl:"
         "-register-mode:"
         "-proxy:"
         "-sip-ui:"
#endif
#if OPAL_H323
         "-no-h323."
         "H-h323:"
         "g-gk-host:"
         "G-gk-id:"
         "-no-fast."
         "-no-tunnel."
         "-h323-ui:"
#endif
#if OPAL_LID
         "-no-lid."
         "L-lines:"
         "-country:"
#endif
#if OPAL_CAPI
         "-no-capi."
#endif
         "-stun:"
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
#if PTRACING
         "t-trace."
         "o-output:"
#endif
         "V-version."
         "h-help.";
}


PString OpalManagerConsole::GetArgumentUsage() const
{
  return "Global options:\n"
         "  -u or --user name          : Set local username, defaults to OS username.\n"
         "  -p or --password pwd       : Set password for authentication.\n"
         "  -D or --disable codec      : Disable use of specified media formats (codecs).\n"
         "  -P or --prefer codec       : Set preference order for media formats (codecs).\n"
         "  -O or --option fmt:opt=val : Set options for media format (codecs).\n"
         "        --inband-detect      : Disable detection of in-band tones.\n"
         "        --inband-send        : Disable transmission of in-band tones.\n"
         "        --tel proto          : Protocol to use for tel: URI, e.g. sip\n"
         "\n"

#if OPAL_SIP
         "SIP options:\n"
         "        --no-sip             : Disable SIP\n"
         "  -S or --sip interface      : SIP listens on interface, defaults to udp$*:5060.\n"
         "  -r or --register server    : SIP registration to server.\n"
         "        --register-auth-id n : SIP registration authorisation id, default is username.\n"
         "        --register-proxy n   : SIP registration proxy, default is none.\n"
         "        --register-ttl n     : SIP registration Time To Live, default 300 seconds.\n"
         "        --register-mode m    : SIP registration mode (normal, single, public).\n"
         "        --proxy url          : SIP outbound proxy.\n"
         "        --sip-ui mode        : SIP User Indication mode (inband,rfc2833,info-tone,info-string)\n"
         "\n"
#endif

#if OPAL_H323
         "H.323 options:\n"
         "        --no-h323            : Disable H.323\n"
         "  -H or --h323 interface     : H.323 listens on interface, defaults to tcp$*:1720.\n"
         "  -g or --gk-host host       : H.323 gatekeeper host.\n"
         "  -G or --gk-id id           : H.323 gatekeeper identifier.\n"
         "        --no-fast            : H.323 fast connect disabled.\n"
         "        --no-tunnel          : H.323 tunnel for H.245 disabled.\n"
         "        --h323-ui mode       : H.323 User Indication mode (inband,rfc2833,h245-signal,h245-string)\n"
         "\n"
#endif

#if OPAL_LID || OPAL_CAPI
         "PSTN options:\n"
#if OPAL_CAPI
         "        --no-capi            : Disable ISDN access via CAPI\n"
#endif
#if OPAL_LID
         "        --no-lid             : Disable Line Interface Devices\n"
         "  -L or --lines devices      : Set Line Interface Devices.\n"
         "        --country code       : Select country to use for LID (eg \"US\", \"au\" or \"+61\").\n"
#endif
         "\n"
#endif

         "IP options:\n"
         "     --stun server           : Set NAT traversal STUN server\n"
         "     --translate ip          : Set external IP address if masqueraded\n"
         "     --portbase n            : Set TCP/UDP/RTP port base\n"
         "     --portmax n             : Set TCP/UDP/RTP port max\n"
         "     --tcp-base n            : Set TCP port base (default 0)\n"
         "     --tcp-max n             : Set TCP port max (default base+99)\n"
         "     --udp-base n            : Set UDP port base (default 6000)\n"
         "     --udp-max n             : Set UDP port max (default base+199)\n"
         "     --rtp-base n            : Set RTP port base (default 5000)\n"
         "     --rtp-max n             : Set RTP port max (default base+199)\n"
         "     --rtp-tos n             : Set RTP packet IP TOS bits to n\n"
         "     --rtp-size size         : Set RTP maximum payload size in bytes.\n"
         "\n"
         "Debug:\n"
#if PTRACING
         "  -t or --trace              : verbosity in error log (more times for more detail)\n"     
         "  -o or --output file        : file name for output of log messages\n"       
#endif
         "  -V or --version            : Display application version.\n"
         "  -h or --help               : This help message.\n"
            ;
}


bool OpalManagerConsole::Initialise(PArgList & args, bool verbose, const PString & defaultRoute)
{
  if (args.HasOption('V')) {
    const PProcess & process = PProcess::Current();
    cerr << process.GetName()
         << " version " << process.GetVersion(true) << "\n"
            "  by   " << process.GetManufacturer() << "\n"
            "  on   " << process.GetOSClass() << ' ' << process.GetOSName()
         << " (" << process.GetOSVersion() << '-' << process.GetOSHardware() << ")\n"
            "  with PTLib v" << PProcess::GetLibVersion() << "\n"
            "  and  OPAL  v" << OpalGetVersion()
         << endl;
    return false;
  }

#if PTRACING
  PTrace::Initialise(args.GetOptionCount('t'),
                     args.HasOption('o') ? (const char *)args.GetOptionString('o') : NULL,
                PTrace::Blocks | PTrace::Timestamp | PTrace::Thread | PTrace::FileAndLine);
#endif

  if (args.HasOption('O')) {
    PStringArray options = args.GetOptionString('O').Lines();
    for (PINDEX i = 0; i < options.GetSize(); ++i) {
      PRegularExpression parse("\\([A-Za-z].*\\):\\([A-Za-z].*\\)=\\(.*\\)");
      PStringArray subexpressions(4);
      if (!parse.Execute(options[i], subexpressions)) {
        cerr << "Invalid media format option \"" << options[i] << '"' << endl;
        return false;
      }

      OpalMediaFormat format(subexpressions[1]);
      if (!format.IsValid()) {
        cerr << "Unknown media format \"" << subexpressions[1] << '"' << endl;
        return false;
      }
      if (!format.HasOption(subexpressions[2])) {
        cerr << "Unknown option name \"" << subexpressions[2] << "\""
                " in media format \"" << subexpressions[1] << '"' << endl;
        return false;
      }
      if (!format.SetOptionValue(subexpressions[2], subexpressions[3])) {
        cerr << "Ilegal value \"" << subexpressions[3] << "\""
                " for option name \"" << subexpressions[2] << "\""
                " in media format \"" << subexpressions[1] << '"' << endl;
        return false;
      }
      OpalMediaFormat::SetRegisteredMediaFormat(format);
    }
  }

  if (args.HasOption('D'))
    SetMediaFormatMask(args.GetOptionString('D').Lines());
  if (args.HasOption('P'))
    SetMediaFormatOrder(args.GetOptionString('P').Lines());
  if (verbose) {
    OpalMediaFormatList formats = OpalMediaFormat::GetAllRegisteredMediaFormats();
    formats.Remove(GetMediaFormatMask());
    formats.Reorder(GetMediaFormatOrder());
    cout << "Media Formats: " << setfill(',') << formats << setfill(' ') << endl;
  }

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
            "RTP payload size: " << GetMaxRtpPayloadSize() << '\n';

  if (args.HasOption("stun")) {
    if (verbose)
      cout << "STUN server: " << flush;
    PSTUNClient::NatTypes natType = SetSTUNServer(args.GetOptionString("stun"));
    if (verbose) {
      PNatMethod * natMethod = GetNatMethod();
      if (natMethod == NULL)
        cout << "Unavailable";
      else {
        cout << '"' << natMethod->GetServer() << "\" replies " << natType;
        PIPSocket::Address externalAddress;
        if (natType != PSTUNClient::BlockedNat && natMethod->GetExternalAddress(externalAddress))
          cout << " with external address " << externalAddress;
      }
      cout << endl;
    }
  }

  if (args.HasOption('u'))
    SetDefaultUserName(args.GetOptionString('u'));

  if (verbose) {
    PIPSocket::InterfaceTable interfaceTable;
    if (PIPSocket::GetInterfaceTable(interfaceTable))
      cout << "Detected " << interfaceTable.GetSize() << " network interfaces:\n"
           << setfill('\n') << interfaceTable << setfill(' ');
  }

  PCaselessString interfaces;

#if OPAL_SIP
  // Set up SIP
  if (args.HasOption("no-sip") || (interfaces = args.GetOptionString('S')) == "x") {
    if (verbose)
      cout << "SIP disabled" << endl;
  }
  else {
    SIPEndPoint * sip  = CreateSIPEndPoint();
    if (!sip->StartListeners(interfaces.Lines())) {
      cerr << "Could not start SIP listeners." << endl;
      return false;
    }
    if (verbose)
      cout << "SIP listening on: " << setfill(',') << sip->GetListeners() << setfill(' ') << '\n';

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

    if (verbose)
      cout << "SIP options: "
           << sip->GetSendUserInputMode() << '\n';

    if (args.HasOption("proxy")) {
      sip->SetProxy(args.GetOptionString("proxy"), args.GetOptionString('u'), args.GetOptionString('p'));
      if (verbose)
        cout << "SIP proxy: " << sip->GetProxy() << '\n';
    }

    if (args.HasOption('r')) {
      SIPRegister::Params params;
      params.m_addressOfRecord = args.GetOptionString('r');
      params.m_authID = args.GetOptionString("register-auth-id");
      params.m_registrarAddress = args.GetOptionString("register-proxy");
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

      if (verbose)
        cout << "SIP registrar: " << flush;
      PString aor;
      SIP_PDU::StatusCodes status;
      if (!sip->Register(params, aor, &status)) {
        cerr << "\nSIP registration to " << params.m_addressOfRecord
             << " failed (" << status << ')' << endl;
        return false;
      }
      if (verbose)
        cout << aor << '\n';
    }

    AddRouteEntry("sip.*:.* = " + defaultRoute);
  }
#endif // OPAL_SIP


#if OPAL_H323
  // Set up H.323
  if (args.HasOption("no-h323") || (interfaces = args.GetOptionString('H')) == "x") {
    if (verbose)
      cout << "H.323 disabled" << endl;
  }
  else {
    H323EndPoint * h323 = CreateH323EndPoint();
    if (!h323->StartListeners(interfaces.Lines())) {
      cerr << "Could not start H.323 listeners." << endl;
      return false;
    }
    if (verbose)
      cout << "H.323 listening on: " << setfill(',') << h323->GetListeners() << setfill(' ') << '\n';

    h323->DisableFastStart(args.HasOption("no-fast"));
    h323->DisableH245Tunneling(args.HasOption("no-tunnel"));

    if (args.HasOption("h323-ui")) {
      PCaselessString str = args.GetOptionString("h323-ui");
      if (str == "inband")
        h323->SetSendUserInputMode(OpalConnection::SendUserInputInBand);
      else if (str == "rfc2833")
        h323->SetSendUserInputMode(OpalConnection::SendUserInputAsRFC2833);
      else if (str == "h245-signal")
        h323->SetSendUserInputMode(OpalConnection::SendUserInputAsTone);
      else if (str == "h245-string")
        h323->SetSendUserInputMode(OpalConnection::SendUserInputAsString);
      else {
        cerr << "Unknown H.323 user indication mode \"" << str << '"' << endl;
        return false;
      }
    }

    if (verbose)
      cout << "H.323 options: "
           << (h323->IsFastStartDisabled() ? "Slow" : "Fast") << " connect, "
           << (h323->IsH245TunnelingDisabled() ? "Separate" : "Tunnelled") << " H.245, "
           << h323->GetSendUserInputMode() << '\n';

    if (args.HasOption('g') || args.HasOption('G')) {
      if (verbose)
        cout << "H.323 Gatekeeper: " << flush;
      if (!h323->UseGatekeeper(args.GetOptionString('g'), args.GetOptionString('G'))) {
        cerr << "\nCould not complete gatekeeper registration" << endl;
        return false;
      }
      if (verbose)
        cout << *h323->GetGatekeeper() << flush;
    }

    AddRouteEntry("h323.*:.* = " + defaultRoute);
  }
#endif // OPAL_H323

#if OPAL_LID
  // If we have LIDs speficied in command line, load them
  if (args.HasOption("no-h323") || !args.HasOption('L')) {
    if (verbose)
      cout << "Line Interface Devices disabled" << endl;
  }
  else {
    OpalLineEndPoint * lines = CreateLineEndPoint();
    if (!lines->AddDeviceNames(args.GetOptionString('L').Lines())) {
      cerr << "Could not start Line Interface Device(s)" << endl;
      return false;
    }
    if (verbose)
      cout << "Line Interface listening on: " << setfill(',') << lines->GetLines() << setfill(' ') << endl;

    PString country = args.GetOptionString("country");
    if (!country.IsEmpty()) {
      if (!lines->SetCountryCodeName(country))
        cerr << "Could not set LID to country name \"" << country << '"' << endl;
      else if (verbose)
        cout << "LID to country: " << lines->GetLine("*")->GetDevice().GetCountryCodeName() << endl;
    }

    AddRouteEntry("pstn:.* = " + defaultRoute);
  }
#endif // OPAL_LID

#if OPAL_CAPI
  if (args.HasOption("no-capi")) {
    if (verbose)
      cout << "ISDN CAPI disabled" << endl;
  }
  else {
    OpalCapiEndPoint * capi = CreateCapiEndPoint();
    unsigned controllers = capi->OpenControllers();
    if (controllers == 0)
      cerr << "Could not open CAPI controllers." << endl;
    else if (verbose)
      cout << "Found " << controllers << " CAPI controllers." << endl;

    AddRouteEntry("isdn:.* = " + defaultRoute);
  }
#endif

  PString telProto = args.GetOptionString("tel");
  if (!telProto.IsEmpty()) {
    OpalEndPoint * ep = FindEndPoint(telProto);
    if (ep == NULL) {
      cerr << "The \"tel\" URI cannot be mapped to protocol \"" << telProto << '"' << endl;
      return false;
    }

    AttachEndPoint(ep, "tel");
    if (verbose)
      cout << "tel URI mapped to: " << ep->GetPrefixName() << '\n';
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


#if OPAL_CAPI
OpalCapiEndPoint * OpalManagerConsole::CreateCapiEndPoint()
{
  return new OpalCapiEndPoint(*this);
}
#endif // OPAL_LID


/////////////////////////////////////////////////////////////////////////////
