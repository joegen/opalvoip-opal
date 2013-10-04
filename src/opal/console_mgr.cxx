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
#include <ep/pcss.h>
#include <ep/ivr.h>
#include <ep/opalmixer.h>
#include <rtp/srtp_session.h>
#include <ptclib/pstun.h>


static void PrintVersion(ostream & strm)
{
  const PProcess & process = PProcess::Current();
  strm << process.GetName()
        << " version " << process.GetVersion(true) << "\n"
          "  by   " << process.GetManufacturer() << "\n"
          "  on   " << process.GetOSClass() << ' ' << process.GetOSName()
        << " (" << process.GetOSVersion() << '-' << process.GetOSHardware() << ")\n"
          "  with PTLib v" << PProcess::GetLibVersion() << "\n"
          "  and  OPAL  v" << OpalGetVersion()
        << endl;
}


#if OPAL_SIP
class SIPConsoleEndPoint : public SIPEndPoint, public OpalConsoleEndPoint
{
  PCLASSINFO(SIPConsoleEndPoint, SIPEndPoint)
public:
  SIPConsoleEndPoint(OpalConsoleManager & manager)
    : SIPEndPoint(manager)
  {
  }


  bool SetRegistrationParams(SIPRegister::Params & params,
                             PString & error,
                             const PArgList & args, 
                             const char * mode,
                             const char * ttl)
  {
    if (args.HasOption(mode)) {
      PCaselessString str = args.GetOptionString("register-mode");
      if (str == "normal")
        params.m_compatibility = SIPRegister::e_FullyCompliant;
      else if (str == "single")
        params.m_compatibility = SIPRegister::e_CannotRegisterMultipleContacts;
      else if (str == "public")
        params.m_compatibility = SIPRegister::e_CannotRegisterPrivateContacts;
      else if (str == "ALG")
        params.m_compatibility = SIPRegister::e_HasApplicationLayerGateway;
      else if (str == "RFC5626")
        params.m_compatibility = SIPRegister::e_RFC5626;
      else {
        error = "Unknown SIP registration mode " + str;
        return false;
      }
    }

    params.m_expire = args.GetOptionString(ttl, "300").AsUnsigned();
    if (params.m_expire < 30) {
      error = "SIP registrar Time To Live must be more than 30 seconds";
      return false;
    }

    return true;
  }


  virtual void GetArgumentSpec(ostream & strm) const
  {
    strm << "[SIP options:]"
            "-no-sip.           Disable SIP\n"
            "S-sip:             SIP listens on interface, defaults to udp$*:5060.\n"
            "r-register:        SIP registration to server.\n"
            "-register-auth-id: SIP registration authorisation id, default is username.\n"
            "-register-proxy:   SIP registration proxy, default is none.\n"
            "-register-ttl:     SIP registration Time To Live, default 300 seconds.\n"
            "-register-mode:    SIP registration mode (normal, single, public, ALG, RFC5626).\n"
            "-proxy:            SIP outbound proxy.\n"
            "-sip-bandwidth:    Set total bandwidth (both directions) to be used for call\n"
            "-sip-rx-bandwidth: Set receive bandwidth to be used for call\n"
            "-sip-tx-bandwidth: Set transmit bandwidth to be used for call\n"
            "-sip-ui:           SIP User Indication mode (inband,rfc2833,info-tone,info-string)\n";
  }


  bool SetUIMode(const PCaselessString & str)
  {
    if (str == "inband")
      SetSendUserInputMode(OpalConnection::SendUserInputInBand);
    else if (str == "rfc2833")
      SetSendUserInputMode(OpalConnection::SendUserInputAsRFC2833);
    else if (str == "info-tone")
      SetSendUserInputMode(OpalConnection::SendUserInputAsTone);
    else if (str == "info-string")
      SetSendUserInputMode(OpalConnection::SendUserInputAsString);
    else
      return false;

    return true;
  }


  virtual InitResult Initialise(PArgList & args, ostream & output, bool verbose)
  {
    // Set up SIP
    PCaselessString interfaces;
    if (args.HasOption("no-sip") || (interfaces = args.GetOptionString("sip")) == "x")
      return InitDisabled;

    if (!StartListeners(interfaces.Lines())) {
      cerr << "Could not start SIP listeners." << endl;
      return InitFailed;
    }

    if (verbose)
      output << "SIP listening on: " << setfill(',') << GetListeners() << setfill(' ') << '\n';

    if (args.HasOption("sip-bandwidth"))
      SetInitialBandwidth(OpalBandwidth::RxTx, args.GetOptionString("sip-bandwidth"));
    if (args.HasOption("sip-rx-bandwidth"))
      SetInitialBandwidth(OpalBandwidth::Rx, args.GetOptionString("sip-rx-bandwidth"));
    if (args.HasOption("sip-tx-bandwidth"))
      SetInitialBandwidth(OpalBandwidth::Rx, args.GetOptionString("sip-tx-bandwidth"));

    if (args.HasOption("sip-ui") && !SetUIMode(args.GetOptionString("sip-ui"))) {
      cerr << "Unknown SIP user indication mode\n";
      return InitFailed;
    }

    if (verbose)
      output << "SIP options: "
             << GetSendUserInputMode() << '\n';

    if (args.HasOption("proxy")) {
      SetProxy(args.GetOptionString("proxy"), args.GetOptionString("user"), args.GetOptionString("password"));
      if (verbose)
        output << "SIP proxy: " << GetProxy() << '\n';
    }

    if (args.HasOption("register")) {
      SIPRegister::Params params;
      params.m_addressOfRecord = args.GetOptionString("register");
      params.m_authID = args.GetOptionString("register-auth-id");
      params.m_registrarAddress = args.GetOptionString("register-proxy");
      params.m_password = args.GetOptionString("password");

      PString error;
      if (!SetRegistrationParams(params, error, args, "register-mode", "register-ttl")) {
        cerr << error << endl;
        return InitFailed;
      }

      if (verbose)
        output << "SIP registrar: " << flush;
      PString aor;
      SIP_PDU::StatusCodes status;
      if (!Register(params, aor, &status)) {
        cerr << "\nSIP registration to " << params.m_addressOfRecord
             << " failed (" << status << ')' << endl;
        return InitFailed;
      }
      if (verbose)
        output << aor << endl;
    }

    return InitSuccess;
  }


#if P_CLI
  PDECLARE_NOTIFIER(PCLI::Arguments, SIPConsoleEndPoint, CmdRegister)
  {
    SIPRegister::Params params;

    params.m_addressOfRecord  = from[0];
    params.m_password         = from[1];
    params.m_authID           = from.GetOptionString('a');
    params.m_realm            = from.GetOptionString('r');
    params.m_proxyAddress     = from.GetOptionString('p');

    PString error;
    if (!SetRegistrationParams(params, error, from, "mode", "ttl")) {
      from.WriteError(error);
      return;
    }

    from.GetContext() << "Registering with " << params.m_addressOfRecord << " ..." << flush;

    PString aor;
    if (Register(params, aor, false))
      from.GetContext() << "succeeded" << endl;
    else
      from.GetContext() << "failed" << endl;
  }

  PDECLARE_NOTIFIER(PCLI::Arguments, SIPConsoleEndPoint, CmdUserInput)
  {
    if (from.GetCount() < 1)
      from.Usage();
    else if (!SetUIMode(from[0]))
      from.WriteError() << "Unknown SIP user indication mode\n";
  }

  virtual void AddCommands(PCLI & cli)
  {
    cli.SetCommand("sip register", PCREATE_NOTIFIER(CmdRegister),
                   "Register with SIP registrar",
                   "[ options ] <address> [ <password> ]",
                   "a-auth-id: Override user for authorisation\n"
                   "r-realm: Set realm for authorisation\n"
                   "p-proxy: Set proxy for registration\n"
                   "m-mode: Set registration mode (normal, single, public)\n"
                   "t-ttl: Set Time To Live for registration\n");
    cli.SetCommand("sip ui", PCREATE_NOTIFIER(CmdUserInput),
                   "Set SIP user input mode",
                   "\"inband\" | \"rfc2833\" | \"info-tone\" | \"info-string\"");
  }
#endif // P_CLI
};
#endif // OPAL_SIP


#if OPAL_H323
class H323ConsoleEndPoint : public H323EndPoint, public OpalConsoleEndPoint
{
  PCLASSINFO(H323ConsoleEndPoint, H323EndPoint)
public:
  H323ConsoleEndPoint(OpalConsoleManager & manager)
    : H323EndPoint(manager)
  {
  }


  virtual void GetArgumentSpec(ostream & strm) const
  {
    strm << "[H.323 options:]"
            "-no-h323.          Disable H.323\n"
            "H-h323:            H.323 listens on interface, defaults to tcp$*:1720.\n"
            "g-gk-host:         H.323 gatekeeper host.\n"
            "G-gk-id:           H.323 gatekeeper identifier.\n"
            "-no-fast.          H.323 fast connect disabled.\n"
            "-no-tunnel.        H.323 tunnel for H.245 disabled.\n"
            "-h323-bandwidth:    Set total bandwidth (both directions) to be used for call\n"
            "-h323-rx-bandwidth: Set receive bandwidth to be used for call\n"
            "-h323-tx-bandwidth: Set transmit bandwidth to be used for call\n"
            "-h323-ui:          H.323 User Indication mode (inband,rfc2833,h245-signal,h245-string)\n";
  }


  bool SetUIMode(const PCaselessString & str)
  {
    if (str == "inband")
      SetSendUserInputMode(OpalConnection::SendUserInputInBand);
    else if (str == "rfc2833")
      SetSendUserInputMode(OpalConnection::SendUserInputAsRFC2833);
    else if (str == "h245-signal")
      SetSendUserInputMode(OpalConnection::SendUserInputAsTone);
    else if (str == "h245-string")
      SetSendUserInputMode(OpalConnection::SendUserInputAsString);
    else
      return false;

    return true;
  }

  virtual InitResult Initialise(PArgList & args, ostream & output, bool verbose)
  {
    // Set up H.323
    PCaselessString interfaces;
    if (args.HasOption("no-h323") || (interfaces = args.GetOptionString("h323")) == "x")
      return InitDisabled;

    if (!StartListeners(interfaces.Lines())) {
      cerr << "Could not start H.323 listeners." << endl;
      return InitFailed;
    }

    if (verbose)
      output << "H.323 listening on: " << setfill(',') << GetListeners() << setfill(' ') << '\n';

    DisableFastStart(args.HasOption("no-fast"));
    DisableH245Tunneling(args.HasOption("no-tunnel"));

    if (args.HasOption("h323-bandwidth"))
      SetInitialBandwidth(OpalBandwidth::RxTx, args.GetOptionString("h323-bandwidth"));
    if (args.HasOption("h323-rx-bandwidth"))
      SetInitialBandwidth(OpalBandwidth::Rx, args.GetOptionString("h323-rx-bandwidth"));
    if (args.HasOption("h323-tx-bandwidth"))
      SetInitialBandwidth(OpalBandwidth::Rx, args.GetOptionString("h323-tx-bandwidth"));

    if (args.HasOption("h323-ui") && !SetUIMode(args.GetOptionString("h323-ui"))) {
      cerr << "Unknown H.323 user indication mode\n";
      return InitFailed;
    }

    if (verbose)
      output << "H.323 options: "
             << (IsFastStartDisabled() ? "Slow" : "Fast") << " connect, "
             << (IsH245TunnelingDisabled() ? "Separate" : "Tunnelled") << " H.245, "
             << GetSendUserInputMode() << '\n';

    if (args.HasOption("gk-host") || args.HasOption("gk-id")) {
      if (verbose)
        output << "H.323 Gatekeeper: " << flush;
      if (!UseGatekeeper(args.GetOptionString("gk-host"), args.GetOptionString("gk-id"))) {
        cerr << "\nCould not complete gatekeeper registration" << endl;
        return InitFailed;
      }
      if (verbose)
        output << *GetGatekeeper() << flush;
    }

    return InitSuccess;
  }


#if P_CLI
  PDECLARE_NOTIFIER(PCLI::Arguments, H323ConsoleEndPoint, CmdFast)
  {
    if (from.GetCount() < 1)
      from.GetContext() << "Fast connect: " << (IsFastStartDisabled() ? "OFF" : "ON") << endl;
    else if (from[0] *= "on")
      DisableFastStart(false);
    else if (from[0] *= "off")
      DisableFastStart(true);
    else
      from.Usage();
  }

  PDECLARE_NOTIFIER(PCLI::Arguments, H323ConsoleEndPoint, CmdTunnel)
  {
    if (from.GetCount() < 1)
      from.GetContext() << "H.245 tunneling: " << (IsH245TunnelingDisabled() ? "OFF" : "ON") << endl;
    else if (from[0] *= "on")
      DisableH245Tunneling(false);
    else if (from[0] *= "off")
      DisableH245Tunneling(true);
    else
      from.Usage();
  }

  PDECLARE_NOTIFIER(PCLI::Arguments, H323ConsoleEndPoint, CmdUserInput)
  {
    if (from.GetCount() < 1)
      from.Usage();
    else if (!SetUIMode(from[0]))
      from.WriteError() << "Unknown SIP user indication mode\n";
  }

  virtual void AddCommands(PCLI & cli)
  {
    cli.SetCommand("h323 fast", PCREATE_NOTIFIER(CmdFast),
                   "Set fast connect mode",
                   "[ \"on\" / \"off\" ]");
    cli.SetCommand("h323 tunnel", PCREATE_NOTIFIER(CmdTunnel),
                   "Set H.245 tunneling mode",
                   "[ \"on\" / \"off\" ]");
    cli.SetCommand("h323 ui", PCREATE_NOTIFIER(CmdUserInput),
                   "Set H.323 user input mode",
                   "\"inband\" | \"rfc2833\" | \"h245-signal\" | \"h245-string\"");
  }
#endif // P_CLI
};
#endif // OPAL_H323


#if OPAL_LID
class OpalConsoleLineEndPoint : public OpalLineEndPoint, public OpalConsoleEndPoint
{
  PCLASSINFO(OpalConsoleLineEndPoint, OpalLineEndPoint)
public:
  OpalConsoleLineEndPoint(OpalConsoleManager & manager)
    : OpalLineEndPoint(manager)
  {
  }


  virtual void GetArgumentSpec(ostream & strm) const
  {
    strm << "[PSTN options:]"
            "-no-lid.           Disable Line Interface Devices\n"
            "L-lines:           Set Line Interface Devices.\n"
            "-country:          Select country to use for LID (eg \"US\", \"au\" or \"+61\").\n";
  }


  virtual InitResult Initialise(PArgList & args, ostream & output, bool verbose)
  {
    // If we have LIDs speficied in command line, load them
    if (args.HasOption("no-lid") || !args.HasOption("lines"))
      return InitDisabled;

    if (!AddDeviceNames(args.GetOptionString("lines").Lines())) {
      cerr << "Could not start Line Interface Device(s)" << endl;
      return InitFailed;
    }
    if (verbose)
      output << "Line Interface listening on: " << setfill(',') << GetLines() << setfill(' ') << '\n';

    PString country = args.GetOptionString("country");
    if (!country.IsEmpty()) {
      if (!SetCountryCodeName(country))
        cerr << "Could not set LID to country name \"" << country << '"' << endl;
      else if (verbose)
        output << "LID to country: " << GetLine("*")->GetDevice().GetCountryCodeName() << '\n';
    }

    return InitSuccess;
  }


#if P_CLI
  PDECLARE_NOTIFIER(PCLI::Arguments, OpalConsoleLineEndPoint, CmdCountry)
  {
    if (from.GetCount() < 1)
      from.Usage();
    else if (!SetCountryCodeName(from[0]))
      from.WriteError() << "Could not set LID to country name \"" << from[0] << '"' << endl;
  }

  virtual void AddCommands(PCLI & cli)
  {
    cli.SetCommand("pstn country", PCREATE_NOTIFIER(CmdCountry),
                   "Set country code or name",
                   "[ <name> ]");
  }
#endif // P_CLI
};
#endif // OPAL_LID


#if OPAL_CAPI
class OpalConsoleCapiEndPoint : public OpalCapiEndPoint, public OpalConsoleEndPoint
{
  PCLASSINFO(OpalConsoleCapiEndPoint, OpalCapiEndPoint)
public:
  OpalConsoleCapiEndPoint(OpalConsoleManager & manager)
    : OpalCapiEndPoint(manager)
  {
  }

  virtual void GetArgumentSpec(ostream & strm) const
  {
    strm << "[ISDN (CAPI) options:]"
            "-no-capi.          Disable ISDN via CAPI\n";
  }


  virtual InitResult Initialise(PArgList & args, ostream & output, bool verbose)
  {
    if (args.HasOption("no-capi"))
      return InitDisabled;

    unsigned controllers = OpenControllers();
    if (verbose) {
      if (controllers == 0)
        output << "No CAPI controllers available.\n";
      else
        output << "Found " << controllers << " CAPI controllers.\n";
    }

    return InitSuccess;
  }


#if P_CLI
  virtual void AddCommands(PCLI & cli)
  {
  }
#endif // P_CLI
};
#endif // OPAL_CAPI


#if OPAL_HAS_PCSS
class OpalConsolePCSSEndPoint : public OpalPCSSEndPoint, public OpalConsoleEndPoint
{
  PCLASSINFO(OpalConsolePCSSEndPoint, OpalPCSSEndPoint)
public:
  OpalConsolePCSSEndPoint(OpalConsoleManager & manager)
    : OpalPCSSEndPoint(manager)
  {
  }

  virtual void GetArgumentSpec(ostream & strm) const
  {
    strm << "[PC options:]"
            "-record-driver:  Audio recorder driver.\n"
            "-record-device:  Audio recorder device.\n"
            "-play-driver:    Audio player driver.\n"
            "-play-device:    Audio player device.\n"
            "-moh-driver:     Audio player driver for music on hold.\n"
            "-moh-device:     Audio player device for music on hold.\n"
            "-audio-buffer:   Audio buffer time in ms (default 120)\n"
#if OPAL_VIDEO
            "-display-driver: Video display driver to use.\n"
            "-display-device: Video display device to use.\n"
            "-grab-driver:    Video grabber driver.\n"
            "-grab-device:    Video grabber device.\n"
            "-grab-format:    Video grabber format (\"pal\"/\"ntsc\")\n"
            "-grab-channel:   Video grabber channel.\n"
            "-preview-driver: Video preview driver to use.\n"
            "-preview-device: Video preview device to use.\n"
            "-voh-driver:     Video source driver for music on hold.\n"
            "-voh-device:     Video source device for music on hold.\n"
#endif // OPAL_VIDEO
            ;
  }


  virtual InitResult Initialise(PArgList & args, ostream & output, bool verbose)
  {
    if (!SetSoundChannelRecordDevice(args.GetOptionString("record-driver") + '\t' + args.GetOptionString("record-device"))) {
      cerr << "Illegal sound recorder driver/device\n";
      return InitFailed;
    }
    if (verbose)
      output << "Sound recorder: " << GetSoundChannelRecordDevice() << '\n';

    if (!SetSoundChannelPlayDevice(args.GetOptionString("play-driver") + '\t' + args.GetOptionString("play-device"))) {
      cerr << "Illegal sound player driver/device\n";
      return InitFailed;
    }
    if (verbose)
      output << "Sound player: " << GetSoundChannelPlayDevice() << '\n';

    if (!SetSoundChannelOnHoldDevice(args.GetOptionString("moh-driver") + '\t' + args.GetOptionString("moh-device"))) {
      cerr << "Illegal sound player driver/device for hold\n";
      return InitFailed;
    }
    if (verbose)
      output << "Music on Hold: " << GetSoundChannelOnHoldDevice() << '\n';

    if (args.HasOption("audio-buffer"))
      SetSoundChannelBufferTime(args.GetOptionString("audio-buffer").AsUnsigned());
    if (verbose)
      output << "Audio buffers: " << GetSoundChannelBufferTime() << "ms\n";

#if OPAL_VIDEO
    PVideoDevice::OpenArgs video = manager.GetVideoOutputDevice();
    video.driverName = args.GetOptionString("display-driver");
    video.deviceName = args.GetOptionString("display-device");
    if ((!video.driverName.IsEmpty() || !video.deviceName.IsEmpty()) && !manager.SetVideoOutputDevice(video)) {
      cerr << "Illegal video display driver/device\n";
      return InitFailed;
    }
    if (verbose)
      output << "Display: " << manager.GetVideoOutputDevice().deviceName << '\n';

    video = manager.GetVideoInputDevice();
    video.driverName = args.GetOptionString("grab-driver");
    video.deviceName = args.GetOptionString("grab-device");
    PCaselessString fmt = args.GetOptionString("grab-format");
    if (fmt == "pal")
      video.videoFormat = PVideoDevice::PAL;
    else if (fmt == "ntsc")
      video.videoFormat = PVideoDevice::NTSC;
    else if (fmt == "secam")
      video.videoFormat = PVideoDevice::SECAM;
    else if (fmt == "auto")
      video.videoFormat = PVideoDevice::Auto;
    video.channelNumber = args.GetOptionString("grab-channel").AsUnsigned();
    if ((!video.driverName.IsEmpty() || !video.deviceName.IsEmpty()) && !manager.SetVideoInputDevice(video)) {
      cerr << "Illegal video grabber driver/device\n";
      return InitFailed;
    }
    if (verbose)
      output << "Grabber: " << manager.GetVideoInputDevice().deviceName << '\n';

    video = manager.GetVideoPreviewDevice();
    video.driverName = args.GetOptionString("preview-driver");
    video.deviceName = args.GetOptionString("preview-device");
    if ((!video.driverName.IsEmpty() || !video.deviceName.IsEmpty()) && !manager.SetVideoPreviewDevice(video)) {
      cerr << "Illegal video preview driver/device\n";
      return InitFailed;
    }
    if (verbose)
      output << "Preview: " << manager.GetVideoPreviewDevice().deviceName << '\n';

    video = GetVideoOnHoldDevice();
    video.driverName = args.GetOptionString("voh-driver");
    video.deviceName = args.GetOptionString("voh-device");
    if ((!video.driverName.IsEmpty() || !video.deviceName.IsEmpty()) && !SetVideoOnHoldDevice(video)) {
      cerr << "Illegal video source driver/device for hold" << endl;
      return InitFailed;
    }
    if (verbose)
      output << "Video on Hold: " << GetVideoOnHoldDevice().deviceName << '\n';
#endif // OPAL_VIDEO

    return InitSuccess;
  }


#if P_CLI
  virtual void AddCommands(PCLI & cli)
  {
  }
#endif // P_CLI
};
#endif // OPAL_HAS_PCSS

  
#if OPAL_IVR
class OpalConsoleIVREndPoint : public OpalIVREndPoint, public OpalConsoleEndPoint
{
  PCLASSINFO(OpalConsoleIVREndPoint, OpalIVREndPoint)
public:
  OpalConsoleIVREndPoint(OpalConsoleManager & manager)
    : OpalIVREndPoint(manager)
  {
  }

  virtual void GetArgumentSpec(ostream & strm) const
  {
    strm << "[Interactive Voice Response options:]"
            "-no-ivr.          Disable IVR\n"
            "-ivr-script:      The default VXML script to run\n";
  }


  virtual InitResult Initialise(PArgList & args, ostream & output, bool verbose)
  {
    if (args.HasOption("no-ivr"))
      return InitDisabled;

    PString vxml = args.GetOptionString("ivr-script");
    if (!vxml.IsEmpty())
      SetDefaultVXML(vxml);

    return InitSuccess;
  }


#if P_CLI
  virtual void AddCommands(PCLI & cli)
  {
  }
#endif // P_CLI
};
#endif // OPAL_IVR


/////////////////////////////////////////////////////////////////////////////

OpalConsoleManager::OpalConsoleManager(const char * endpointPrefixes)
  : m_endpointPrefixes(PConstString(endpointPrefixes).Tokenise(" \t\n"))
  , m_interrupted(false)
  , m_verbose(false)
  , m_output(cout)
{
}


PString OpalConsoleManager::GetArgumentSpec() const
{
  PStringStream str;
  str << "[Global options:]"
         "u-user:            Set local username, defaults to OS username.\n"
         "p-password:        Set password for authentication.\n"
         "D-disable:         Disable use of specified media formats (codecs).\n"
         "P-prefer:          Set preference order for media formats (codecs).\n"
         "O-option:          Set options for media format, argument is of form fmt:opt=val.\n"
#if OPAL_VIDEO
         "-max-video-size:   Set maximum received video size, of form 800x600 or \"CIF\" etc\n"
         "-video-size:       Set preferred transmit video size, of form 800x600 or \"CIF\" etc\n"
         "-video-rate:       Set preferred transmit video frame rate, in fps\n"
         "-video-bitrate:    Set target transmit video bit rate, in bps, suffix 'k' or 'M' may be used.\n"
#endif
         "-jitter:           Set audio jitter buffer size (min[,max] default 50,250)\n"
         "-silence-detect:   Set audio silence detect mode (none,fixed,adaptive)\n"
         "-no-inband-detect. Disable detection of in-band tones.\n"
         "-tel:              Protocol to use for tel: URI, e.g. sip\n";

  for (PINDEX i = 0; i < m_endpointPrefixes.GetSize(); ++i) {
    OpalConsoleEndPoint * ep = const_cast<OpalConsoleManager *>(this)->GetConsoleEndPoint(m_endpointPrefixes[i]);
    if (ep != NULL)
      ep->GetArgumentSpec(str);
  }

  str << "[IP options:]"
#if P_NAT
         "-nat-method:       Set NAT method, defaults to STUN\n"
         "-nat-server:       Set NAT server for the above method\n"
#if P_STUN
         "-stun:             Set NAT traversal STUN server\n"
#endif
         "-translate:        Set external IP address if masqueraded\n"
#endif
         "-portbase:         Set TCP/UDP/RTP port base\n"
         "-portmax:          Set TCP/UDP/RTP port max\n"
         "-tcp-base:         Set TCP port base (default 0)\n"
         "-tcp-max:          Set TCP port max (default base+99)\n"
         "-udp-base:         Set UDP port base (default 6000)\n"
         "-udp-max:          Set UDP port max (default base+199)\n"
         "-rtp-base:         Set RTP port base (default 5000)\n"
         "-rtp-max:          Set RTP port max (default base+199)\n"
         "-rtp-tos:          Set RTP packet IP TOS bits to n\n"
         "-rtp-size:         Set RTP maximum payload size in bytes.\n"
         "-aud-qos:          Set Audio RTP Quality of Service to n\n"
         "-vid-qos:          Set Video RTP Quality of Service to n\n"

         "[Debug & General:]"
#if OPAL_STATISTICS
         "-statistics.       Output statistics periodically\n"
         "-stat-time:        Time between statistics output\n"
         "-stat-file:        File to output statistics too, default is stdout\n"
#endif
         PTRACE_ARGLIST
         "V-version.         Display application version.\n"
         "h-help.            This help message.\n"
         ;
  return str;
}


void OpalConsoleManager::Usage(ostream & strm, const PArgList & args)
{
  args.Usage(strm, "[ options ... ]");
}


bool OpalConsoleManager::PreInitialise(PArgList & args, bool verbose)
{
  m_verbose = verbose;

  if (!args.IsParsed())
    args.Parse(GetArgumentSpec());

  if (!args.IsParsed() || args.HasOption("help")) {
    Usage(cerr, args);
    return false;
  }

  if (args.HasOption("version")) {
    PrintVersion(cerr);
    return false;
  }

  return true;
}


bool OpalConsoleManager::Initialise(PArgList & args, bool verbose, const PString & defaultRoute)
{
  if (!PreInitialise(args, verbose))
    return false;

  PTRACE_INITIALISE(args);

  if (args.HasOption("user"))
    SetDefaultUserName(args.GetOptionString("user"));
  if (verbose) {
    m_output << "Default user name: " << GetDefaultUserName();
    if (args.HasOption("password"))
      m_output << " (with password)";
    m_output << '\n';
  }

  if (args.HasOption("jitter")) {
    PStringArray params = args.GetOptionString("jitter").Tokenise("-,:",true);
    unsigned minJitter, maxJitter;
    switch (params.GetSize()) {
      case 1 :
        minJitter = maxJitter = params[0].AsUnsigned();
        break;
        
      case 2 :
        minJitter = params[0].AsUnsigned();
        maxJitter = params[1].AsUnsigned();
        break;
        
      default :
        cerr << "Invalid jitter specification\n";
        return false;
    }
    SetAudioJitterDelay(minJitter, maxJitter);
  }

  if (args.HasOption("silence-detect")) {
    OpalSilenceDetector::Params params = GetSilenceDetectParams();
    PCaselessString arg = args.GetOptionString("silence-detect");
    if (arg.NumCompare("adaptive") == EqualTo)
      params.m_mode = OpalSilenceDetector::AdaptiveSilenceDetection;
    else if (arg.NumCompare("fixed") == EqualTo)
      params.m_mode = OpalSilenceDetector::FixedSilenceDetection;
    else
      params.m_mode = OpalSilenceDetector::NoSilenceDetection;
    SetSilenceDetectParams(params);
  }

  DisableDetectInBandDTMF(args.HasOption("no-inband-detect"));

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

  if (args.HasOption("aud-qos"))
    SetMediaQoS(OpalMediaType::Audio(), args.GetOptionString("aud-qos"));

#if OPAL_VIDEO
  if (args.HasOption("vid-qos"))
    SetMediaQoS(OpalMediaType::Video(), args.GetOptionString("vid-qos"));
#endif

  if (args.HasOption("rtp-size")) {
    unsigned size = args.GetOptionString("rtp-size").AsUnsigned();
    if (size < 32 || size > 65500) {
      cerr << "RTP maximum payload size 32 to 65500.\n";
      return false;
    }
    SetMaxRtpPayloadSize(size);
  }

  if (verbose)
    m_output << "TCP ports: " << GetTCPPortBase() << '-' << GetTCPPortMax() << "\n"
                "UDP ports: " << GetUDPPortBase() << '-' << GetUDPPortMax() << "\n"
                "RTP ports: " << GetRtpIpPortBase() << '-' << GetRtpIpPortMax() << "\n"
                "Audio QoS: " << GetMediaQoS(OpalMediaType::Audio()) << "\n"
#if OPAL_VIDEO
                "Video QoS: " << GetMediaQoS(OpalMediaType::Video()) << "\n"
#endif
                "RTP payload size: " << GetMaxRtpPayloadSize() << '\n';

#if P_NAT
  PString natMethod, natServer;
  if (args.HasOption("translate")) {
    natMethod = PNatMethod_Fixed::GetNatMethodName();
    natServer = args.GetOptionString("translate");
  }
#if P_STUN
  else if (args.HasOption("stun")) {
    natMethod = PSTUNClient::GetNatMethodName();
    natServer = args.GetOptionString("stun");
  }
#endif
  else if (args.HasOption("nat-method")) {
    natMethod = args.GetOptionString("nat-method");
    natServer = args.GetOptionString("nat-server");
  }
  else if (args.HasOption("nat-server")) {
#if P_STUN
    natMethod = PSTUNClient::GetNatMethodName();
#else
    natMethod = PNatMethod_Fixed::GetNatMethodName();
#endif
    natServer = args.GetOptionString("nat-server");
  }

  if (!natMethod.IsEmpty()) {
    if (verbose)
      m_output << natMethod << " server: " << flush;
    SetNATServer(natMethod, natServer);
    if (verbose) {
      PNatMethod * natMethod = GetNatMethod();
      if (natMethod == NULL)
        m_output << "Unavailable";
      else {
        PNatMethod::NatTypes natType = natMethod->GetNatType();
        m_output << '"' << natMethod->GetServer() << "\" replies " << natType;
        PIPSocket::Address externalAddress;
        if (natType != PNatMethod::BlockedNat && natMethod->GetExternalAddress(externalAddress))
          m_output << " with external address " << externalAddress;
      }
      m_output << '\n';
    }
  }
#endif // P_NAT

  if (verbose) {
    PIPSocket::InterfaceTable interfaceTable;
    if (PIPSocket::GetInterfaceTable(interfaceTable))
      m_output << "Detected " << interfaceTable.GetSize() << " network interfaces:\n"
               << setfill('\n') << interfaceTable << setfill(' ');
  }

  for (PINDEX i = 0; i < m_endpointPrefixes.GetSize(); ++i) {
    OpalConsoleEndPoint * ep = GetConsoleEndPoint(m_endpointPrefixes[i]);
    if (ep != NULL) {
      switch (ep->Initialise(args, m_output, verbose)) {
        case OpalConsoleEndPoint::InitFailed :
          return false;
        case OpalConsoleEndPoint::InitSuccess :
          AddRouteEntry(PConstString(m_endpointPrefixes[i]) + ".*:.* = " + defaultRoute);
          break;
        case OpalConsoleEndPoint::InitDisabled :
          if (verbose)
            m_output << m_endpointPrefixes[i] << " disabled\n";
          break;
      }
    }
  }

  PString telProto = args.GetOptionString("tel");
  if (!telProto.IsEmpty()) {
    OpalEndPoint * ep = FindEndPoint(telProto);
    if (ep == NULL) {
      cerr << "The \"tel\" URI cannot be mapped to protocol \"" << telProto << '"' << endl;
      return false;
    }

    AttachEndPoint(ep, "tel");
    if (verbose)
      m_output << "tel URI mapped to: " << ep->GetPrefixName() << '\n';
  }

#if OPAL_VIDEO
  if (args.HasOption("max-video-size") || args.HasOption("video-size") || args.HasOption("video-rate") || args.HasOption("video-bitrate")) {
    unsigned prefWidth = 0, prefHeight = 0, maxWidth = 0, maxHeight = 0;
    if (!PVideoFrameInfo::ParseSize(args.GetOptionString("video-size", "cif"), prefWidth, prefHeight) ||
        !PVideoFrameInfo::ParseSize(args.GetOptionString("max-video-size", "HD1080"), maxWidth, maxHeight)) {
      cerr << "Invalid video size parameter." << endl;
      return false;
    }
    double rate = args.GetOptionString("video-rate").AsReal();
    if (rate < 1 || rate > 60) {
      cerr << "Invalid video frame rate parameter." << endl;
      return false;
    }
    unsigned frameTime = (unsigned)(OpalMediaFormat::VideoClockRate/rate);
    OpalBandwidth bitrate(args.GetOptionString("video-bitrate"));
    if (bitrate < 10000) {
      cerr << "Invalid video bit rate parameter." << endl;
      return false;
    }

    OpalMediaFormatList formats = OpalMediaFormat::GetAllRegisteredMediaFormats();
    for (OpalMediaFormatList::iterator it = formats.begin(); it != formats.end(); ++it) {
      if (it->GetMediaType() == OpalMediaType::Video()) {
        OpalMediaFormat format = *it;
        format.SetOptionInteger(OpalVideoFormat::FrameWidthOption(), prefWidth);
        format.SetOptionInteger(OpalVideoFormat::FrameHeightOption(), prefHeight);
        format.SetOptionInteger(OpalVideoFormat::MaxRxFrameWidthOption(), maxWidth);
        format.SetOptionInteger(OpalVideoFormat::MaxRxFrameHeightOption(), maxHeight);
        format.SetOptionInteger(OpalVideoFormat::FrameTimeOption(), frameTime);
        format.SetOptionInteger(OpalVideoFormat::TargetBitRateOption(), bitrate);
        OpalMediaFormat::SetRegisteredMediaFormat(format);
      }
    }
  }
#endif

  if (args.HasOption("option")) {
    PStringArray options = args.GetOptionString("option").Lines();
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

  if (args.HasOption("disable"))
    SetMediaFormatMask(args.GetOptionString("disable").Lines());
  if (args.HasOption("prefer"))
    SetMediaFormatOrder(args.GetOptionString("prefer").Lines());
  if (verbose) {
    OpalMediaFormatList formats = OpalMediaFormat::GetAllRegisteredMediaFormats();
    formats.Remove(GetMediaFormatMask());
    formats.Reorder(GetMediaFormatOrder());
    m_output << "Media Formats: " << setfill(',') << formats << setfill(' ') << '\n';
  }

#if OPAL_STATISTICS
  m_statsPeriod.SetInterval(0, args.GetOptionString("stat-time").AsUnsigned());
  m_statsFile = args.GetOptionString("stat-file");
  if (m_statsPeriod == 0 && args.HasOption("statistics"))
    m_statsPeriod.SetInterval(0, 5);
#endif

  if (m_verbose)
    m_output.flush();

  return true;
}


void OpalConsoleManager::Run()
{
#if OPAL_STATISTICS
  if (m_statsPeriod != 0) {
    while (!m_endRun.Wait(m_statsPeriod))
      OutputStatistics();
    return;
  }
#endif
  m_endRun.Wait();
}


void OpalConsoleManager::EndRun(bool interrupt)
{
  if (m_verbose)
    m_output << "Exiting application." << endl;

  m_interrupted = interrupt;
  m_endRun.Signal();
}


OpalConsoleEndPoint * OpalConsoleManager::GetConsoleEndPoint(const PString & prefix)
{
  OpalEndPoint * ep = FindEndPoint(prefix);
  if (ep == NULL) {
#if OPAL_SIP
    if (prefix == OPAL_PREFIX_SIP)
      ep = CreateSIPEndPoint();
#endif // OPAL_SIP
#if OPAL_H323
    else if (prefix == OPAL_PREFIX_H323)
      ep = CreateH323EndPoint();
#endif // OPAL_H323
#if OPAL_LID
    else if (prefix == OPAL_PREFIX_PSTN)
      ep = CreateLineEndPoint();
#endif // OPAL_LID
#if OPAL_CAPI
    else if (prefix == OPAL_PREFIX_CAPI)
      ep = CreateCapiEndPoint();
#endif // OPAL_LID
#if OPAL_HAS_PCSS
    else if (prefix == OPAL_PREFIX_PCSS)
      ep = CreatePCSSEndPoint();
#endif
#if OPAL_IVR
    else if (prefix == OPAL_PREFIX_IVR)
      ep = CreateIVREndPoint();
#endif
#if OPAL_HAS_MIXER
    else if (prefix == OPAL_PREFIX_MIXER)
      ep = CreateMixerEndPoint();
#endif
  }

  return dynamic_cast<OpalConsoleEndPoint *>(ep);
}


#if OPAL_SIP
SIPEndPoint * OpalConsoleManager::CreateSIPEndPoint()
{
  return new SIPConsoleEndPoint(*this);
}
#endif // OPAL_SIP


#if OPAL_H323
H323EndPoint * OpalConsoleManager::CreateH323EndPoint()
{
  return new H323ConsoleEndPoint(*this);
}
#endif // OPAL_H323


#if OPAL_LID
OpalLineEndPoint * OpalConsoleManager::CreateLineEndPoint()
{
  return new OpalConsoleLineEndPoint(*this);
}
#endif // OPAL_LID


#if OPAL_CAPI
OpalCapiEndPoint * OpalConsoleManager::CreateCapiEndPoint()
{
  return new OpalConsoleCapiEndPoint(*this);
}
#endif // OPAL_LID


#if OPAL_HAS_PCSS
OpalPCSSEndPoint * OpalConsoleManager::CreatePCSSEndPoint()
{
  return new OpalConsolePCSSEndPoint(*this);
}
#endif


#if OPAL_IVR
OpalIVREndPoint * OpalConsoleManager::CreateIVREndPoint()
{
  return new OpalConsoleIVREndPoint(*this);
}
#endif


#if OPAL_HAS_MIXER
OpalMixerEndPoint * OpalConsoleManager::CreateMixerEndPoint()
{
  return new OpalMixerEndPoint(*this , OPAL_PREFIX_MIXER);
}
#endif


void OpalConsoleManager::OnEstablishedCall(OpalCall & call)
{
  if (m_verbose)
    m_output << "In call with " << call.GetPartyB() << " using " << call.GetPartyA() << endl;

  OpalManager::OnEstablishedCall(call);
}


void OpalConsoleManager::OnHold(OpalConnection & connection, bool fromRemote, bool onHold)
{
  OpalManager::OnHold(connection, fromRemote, onHold);

  if (!m_verbose)
    return;

  m_output << "Remote " << connection.GetRemotePartyName() << " has ";
  if (fromRemote)
    m_output << (onHold ? "put you on" : "released you from");
  else
    m_output << " been " << (onHold ? "put on" : "released from");
  m_output << " hold." << endl;
}


static void LogMediaStream(ostream & out, const char * stopStart, const OpalMediaStream & stream, const OpalConnection & connection)
{
  if (!connection.IsNetworkConnection())
    return;

  OpalMediaFormat mediaFormat = stream.GetMediaFormat();
  out << stopStart << (stream.IsSource() ? " receiving " : " sending ");

#if OPAL_SRTP
  const OpalRTPMediaStream * rtp = dynamic_cast<const OpalRTPMediaStream *>(&stream);
  if (rtp != NULL && dynamic_cast<const OpalSRTPSession *>(&rtp->GetRtpSession()) != NULL)
    out << "secured ";
#endif

  out << mediaFormat;

  if (!stream.IsSource() && mediaFormat.GetMediaType() == OpalMediaType::Audio())
    out << " (" << mediaFormat.GetOptionInteger(OpalAudioFormat::TxFramesPerPacketOption())*mediaFormat.GetFrameTime()/mediaFormat.GetTimeUnits() << "ms)";

  out << (stream.IsSource() ? " from " : " to ")
      << connection.GetPrefixName() << " endpoint"
      << endl;
}


PBoolean OpalConsoleManager::OnOpenMediaStream(OpalConnection & connection, OpalMediaStream & stream)
{
  if (!OpalManager::OnOpenMediaStream(connection, stream))
    return false;

  if (m_verbose)
    LogMediaStream(m_output, "Started", stream, connection);
  return true;
}


#if OPAL_STATISTICS
static PString MakeStatisticsKey(const OpalMediaStream & stream)
{
  return stream.GetID() + (stream.IsSource() ? "-Source" : "-Sink");
}
#endif


void OpalConsoleManager::OnClosedMediaStream(const OpalMediaStream & stream)
{
  OpalManager::OnClosedMediaStream(stream);

  if (m_verbose)
    LogMediaStream(m_output, "Stopped", stream, stream.GetConnection());

#if OPAL_STATISTICS
  m_statsMutex.Wait();
  StatsMap::iterator it = m_statistics.find(MakeStatisticsKey(stream));
  if (it != m_statistics.end())
    m_statistics.erase(it);
  m_statsMutex.Signal();
#endif
}


void OpalConsoleManager::OnClearedCall(OpalCall & call)
{
  OpalManager::OnClearedCall(call);

  if (!m_verbose)
    return;

  PString name = call.GetPartyB().IsEmpty() ? call.GetPartyA() : call.GetPartyB();

  switch (call.GetCallEndReason()) {
    case OpalConnection::EndedByRemoteUser :
      m_output << '"' << name << "\" has cleared the call";
      break;
    case OpalConnection::EndedByCallerAbort :
      m_output << '"' << name << "\" has stopped calling";
      break;
    case OpalConnection::EndedByRefusal :
      m_output << '"' << name << "\" did not accept your call";
      break;
    case OpalConnection::EndedByNoAnswer :
      m_output << '"' << name << "\" did not answer your call";
      break;
    case OpalConnection::EndedByNoAccept :
      m_output << "Did not accept incoming call from \"" << name << '"';
      break;
    case OpalConnection::EndedByNoUser :
      m_output << "Could find user \"" << name << '"';
      break;
    case OpalConnection::EndedByUnreachable :
      m_output << '"' << name << "\" could not be reached.";
      break;
    case OpalConnection::EndedByNoEndPoint :
      m_output << "No phone running for \"" << name << '"';
      break;
    case OpalConnection::EndedByHostOffline :
      m_output << '"' << name << "\" is not online.";
      break;
    case OpalConnection::EndedByConnectFail :
      m_output << "Transport error calling \"" << name << '"';
      break;
    default :
      m_output << call.GetCallEndReasonText() << " with \"" << name << '"';
  }

  PTime now;
  m_output << ", on " << now.AsString("w h:mma") << ", duration "
            << setprecision(0) << setw(5) << (now - call.GetStartTime()) << "s."
            << endl;
}


#if OPAL_STATISTICS
bool OpalConsoleManager::OutputStatistics()
{
  if (m_statsFile.IsEmpty())
    return OutputStatistics(m_output);

  PTextFile file(m_statsFile);
  if (!file.Open(PFile::WriteOnly, PFile::Create))
    return false;

  file.SetPosition(0, PFile::End);
  return OutputStatistics(file);
}


bool OpalConsoleManager::OutputStatistics(ostream & strm)
{
  bool ouputSomething = false;

  PArray<PString> calls = GetAllCalls();
  for (PINDEX cIdx = 0; cIdx < calls.GetSize(); ++cIdx) {
    PSafePtr<OpalCall> call = FindCallWithLock(calls[cIdx], PSafeReference);
    if (call != NULL && OutputCallStatistics(strm, *call))
      ouputSomething = true;
  }

  return ouputSomething;
}


bool OpalConsoleManager::OutputCallStatistics(ostream & strm, OpalCall & call)
{
  PSafePtr<OpalConnection> connection = call.GetConnection(0);
  if (connection == NULL)
    return false; // This really shold not happen

  if (!connection->IsNetworkConnection()) {
    PSafePtr<OpalConnection> otherConnection = call.GetConnection(1);
    if (otherConnection != NULL)
      connection = otherConnection;
  }

  strm << "Call " << call.GetToken() << " from " << call.GetPartyA() << " to " << call.GetPartyB() << "\n"
          "  started at " << call.GetStartTime();
  strm << '\n';

  bool noStreams = true;
  for (int direction = 0; direction < 2; ++direction) {
    PSafePtr<OpalMediaStream> stream;
    while ((stream = connection->GetMediaStream(OpalMediaType(), direction == 0, stream)) != NULL) {
      if (OutputStreamStatistics(strm, *stream))
        noStreams = false;
    }
  }

  if (noStreams)
    strm << "    No media streams open.\n";

  return true;
}


bool OpalConsoleManager::OutputStreamStatistics(ostream & strm, const OpalMediaStream & stream)
{
  if (!stream.IsOpen())
    return false;

  strm << "    " << (stream.IsSource() ? "Receive" : "Transmit") << " stream,"
          " session " << stream.GetSessionID() << ", statistics:\n";

  m_statsMutex.Wait();
  strm << setprecision(6) << m_statistics[MakeStatisticsKey(stream)].Update(stream);
  m_statsMutex.Signal();

  return true;
}
#endif

    
/////////////////////////////////////////////////////////////////////////////

#if P_CLI

OpalManagerCLI::OpalManagerCLI(  const char * endpointPrefixes)
  : OpalConsoleManager(endpointPrefixes)
  , m_cli(NULL)
{
}


OpalManagerCLI::~OpalManagerCLI()
{
  delete m_cli;
}


PString OpalManagerCLI::GetArgumentSpec() const
{
  PString spec = OpalConsoleManager::GetArgumentSpec();
  // Insert just before the version option
  spec.Splice("F-script-file: Execute script file in CLI\n"
#if P_TELNET
              "-cli: Enable telnet command line sessions on port.\n"
#endif
#if P_CURSES
              "-tui. Enable text user interface.\n"
#endif
              , spec.Find("V-version"));
  return spec;
}


bool OpalManagerCLI::Initialise(PArgList & args, bool verbose, const PString & defaultRoute)
{
  if (!PreInitialise(args, verbose))
    return false;

  if (m_cli == NULL) {
#if P_TELNET
    if (args.HasOption("cli")) {
      unsigned port = args.GetOptionString("cli").AsUnsigned();
      if (port == 0 || port > 65535) {
        cerr << "Illegal CLI port " << port << endl;
        return false;
      }
      m_cli = CreateCLITelnet((WORD)port);
    }
#endif // P_TELNET

#if P_CURSES
    if (m_cli == NULL && args.HasOption("tui")) {
      PCLICurses * cli = CreateCLICurses();
      *reinterpret_cast<ostream **>(&m_output) = &cli->GetWindow(0);
      m_cli = cli;
    }
#endif // P_CURSES

    if (m_cli == NULL && (m_cli = CreateCLIStandard()) == NULL)
      return false;
  }

  m_cli->SetPrompt(args.GetCommandName());

#if P_NAT
  m_cli->SetCommand("nat address", PCREATE_NOTIFIER(CmdNat),
                    "Set NAT method and address",
                    "<method> <address>");
#endif

#if PTRACING
  m_cli->SetCommand("trace", PCREATE_NOTIFIER(CmdTrace),
                    "Set trace level (1..6) and filename",
                    "<n> [ <filename> ]");
#endif

  m_cli->SetCommand("list codecs", PCREATE_NOTIFIER(CmdListCodecs),
                    "List available codecs");
  m_cli->SetCommand("delay", PCREATE_NOTIFIER(CmdDelay),
                    "Delay for the specified numebr of seconds",
                    "seconds");
  m_cli->SetCommand("version", PCREATE_NOTIFIER(CmdVersion),
                    "Print application vesion number and library details.");
  m_cli->SetCommand("quit\nq\nexit", PCREATE_NOTIFIER(CmdQuit),
                    "Quit command line interpreter, note quitting from console also shuts down application.");
  m_cli->SetCommand("shutdown", PCREATE_NOTIFIER(CmdShutDown),
                    "Shut down the application");

  for (PINDEX i = 0; i < m_endpointPrefixes.GetSize(); ++i) {
    OpalConsoleEndPoint * ep = GetConsoleEndPoint(m_endpointPrefixes[i]);
    if (ep != NULL)
      ep->AddCommands(*m_cli);
  }

  return OpalConsoleManager::Initialise(args, verbose, defaultRoute);
}


void OpalManagerCLI::Run()
{
  if (PAssertNULL(m_cli) == NULL)
    return;

  if (PProcess::Current().GetArguments().HasOption("script-file")) {
    // if there is a script file, process commands
    PString filename = PProcess::Current().GetArguments().GetOptionString("script-file");
    PTextFile scriptFile;
    if (scriptFile.Open(filename)) {
      PCLIStandard * stdCLI = dynamic_cast<PCLIStandard *>(m_cli);
      if (stdCLI != NULL)
        stdCLI->RunScript(scriptFile);
      else
        m_cli->Run(&scriptFile, new PNullChannel, false, true);
    }
    else
      cerr << "error: cannot open script file \"" << filename << '"' << endl;
  }

  if (m_cli != NULL)
    m_cli->Start(false);
}


void OpalManagerCLI::EndRun(bool interrupt)
{
  if (m_cli != NULL)
    m_cli->Stop();

  OpalConsoleManager::EndRun(interrupt);
}


PCLI * OpalManagerCLI::CreateCLIStandard()
{
  return new PCLIStandard;
}


#if P_TELNET
PCLITelnet * OpalManagerCLI::CreateCLITelnet(WORD port)
{
  PCLITelnet * cli = new PCLITelnet(port);
  cli->StartContext(new PConsoleChannel(PConsoleChannel::StandardInput),
                    new PConsoleChannel(PConsoleChannel::StandardOutput));
  return cli;
}
#endif // P_TELNET


#if P_CURSES
PCLICurses * OpalManagerCLI::CreateCLICurses()
{
  return new PCLICurses();
}
#endif // P_CURSES


#if P_NAT
void OpalManagerCLI::CmdNat(PCLI::Arguments & args, P_INT_PTR)
{
  if (args.GetCount() < 2) {
    args.WriteUsage();
    return;
  }

  if (!SetNATServer(args[0], args[1])) {
    args.WriteError("STUN server offline or unsuitable NAT type");
    return;
  }

  PCLI::Context & out = args.GetContext();
  out << m_natMethod->GetName() << " server \"" << m_natMethod->GetServer() << " replies " << m_natMethod->GetNatType();
  PIPSocket::Address externalAddress;
  if (m_natMethod->GetExternalAddress(externalAddress))
    out << " with address " << externalAddress;
  out.flush();
}
#endif


#if PTRACING
void OpalManagerCLI::CmdTrace(PCLI::Arguments & args, P_INT_PTR)
{
  if (args.GetCount() == 0)
    args.WriteUsage();
  else
    PTrace::Initialise(args[0].AsUnsigned(),
                       args.GetCount() > 1 ? (const char *)args[1] : NULL,
                       PTrace::GetOptions());
}
#endif // PTRACING


void OpalManagerCLI::CmdListCodecs(PCLI::Arguments & args, P_INT_PTR)
{
  OpalMediaFormatList formats;
  OpalMediaFormat::GetAllRegisteredMediaFormats(formats);

  PCLI::Context & out = args.GetContext();
  OpalMediaFormatList::iterator format;
  for (format = formats.begin(); format != formats.end(); ++format) {
    if (format->GetMediaType() == OpalMediaType::Audio() && format->IsTransportable())
      out << *format << '\n';
  }

#if OPAL_VIDEO
  for (format = formats.begin(); format != formats.end(); ++format) {
    if (format->GetMediaType() == OpalMediaType::Video() && format->IsTransportable())
      out << *format << '\n';
  }
#endif

  for (format = formats.begin(); format != formats.end(); ++format) {
    if (format->GetMediaType() != OpalMediaType::Audio() &&
#if OPAL_VIDEO
        format->GetMediaType() != OpalMediaType::Video() &&
#endif
        format->IsTransportable())
      out << *format << '\n';
  }

  out.flush();
}


void OpalManagerCLI::CmdDelay(PCLI::Arguments & args, P_INT_PTR)
{
  if (args.GetCount() < 1)
    args.WriteUsage();
  else {
    PTimeInterval delay(0, args[0].AsUnsigned());
    m_endRun.Wait(delay);
  }
}


void OpalManagerCLI::CmdVersion(PCLI::Arguments & args, P_INT_PTR)
{
  PrintVersion(args.GetContext());
}


void OpalManagerCLI::CmdQuit(PCLI::Arguments & args, P_INT_PTR)
{
  if (PIsDescendant(args.GetContext().GetBaseReadChannel(), PConsoleChannel))
    CmdShutDown(args, 0);
  else
    args.GetContext().Stop();
}


void OpalManagerCLI::CmdShutDown(PCLI::Arguments & args, P_INT_PTR)
{
  args.GetContext().GetCLI().Stop();
}


#endif // P_CLI


/////////////////////////////////////////////////////////////////////////////
