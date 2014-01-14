/*
 * console_mgr.cxx
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

#include <h323/gkclient.h>
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


void OpalConsoleEndPoint::AddRoutesFor(const OpalEndPoint * endpoint, const PString & defaultRoute)
{
  if (defaultRoute.IsEmpty())
    return;

  PStringList prefixes = m_console.GetPrefixNames(endpoint);

  for (PINDEX i = 0; i < prefixes.GetSize(); ++i)
    m_console.AddRouteEntry(prefixes[i] + ":.* = " + defaultRoute);
}


/////////////////////////////////////////////////////////////////////////////

#if OPAL_SIP || OPAL_H323
OpalRTPConsoleEndPoint::OpalRTPConsoleEndPoint(OpalConsoleManager & console, OpalRTPEndPoint * endpoint)
  : OpalConsoleEndPoint(console)
  , m_endpoint(*endpoint)
{
}


bool OpalRTPConsoleEndPoint::SetUIMode(const PCaselessString & str)
{
  if (str == "inband")
    m_endpoint.SetSendUserInputMode(OpalConnection::SendUserInputInBand);
  else if (str == "rfc2833")
    m_endpoint.SetSendUserInputMode(OpalConnection::SendUserInputAsRFC2833);
  else if (str == "signal" || str == "info-tone" || str == "h245-signal")
    m_endpoint.SetSendUserInputMode(OpalConnection::SendUserInputAsTone);
  else if (str == "string" || str == "info-string" || str == "h245-string")
    m_endpoint.SetSendUserInputMode(OpalConnection::SendUserInputAsString);
  else
    return false;

  return true;
}


void OpalRTPConsoleEndPoint::GetArgumentSpec(ostream & strm) const
{
  strm << '-' << m_endpoint.GetPrefixName() << "-crypto:       Set crypto suites in priority order.\n"
          "-" << m_endpoint.GetPrefixName() << "-bandwidth:    Set total bandwidth (both directions) to be used for SIP call\n"
          "-" << m_endpoint.GetPrefixName() << "-rx-bandwidth: Set receive bandwidth to be used for SIP call\n"
          "-" << m_endpoint.GetPrefixName() << "-tx-bandwidth: Set transmit bandwidth to be used for SIP call\n"
          "-" << m_endpoint.GetPrefixName() << "-ui:           SIP User Indication mode (inband,rfc2833,signal,string)\n";
}


bool OpalRTPConsoleEndPoint::Initialise(PArgList & args, ostream & output, bool verbose)
{
  if (args.HasOption(m_endpoint.GetPrefixName()+"-crypto"))
    m_endpoint.SetMediaCryptoSuites(args.GetOptionString(m_endpoint.GetPrefixName()+"-crypto").Lines());

  if (verbose)
    output << m_endpoint.GetPrefixName().ToUpper() << " crypto suites: "
            << setfill(',') << m_endpoint.GetMediaCryptoSuites() << setfill(' ') << '\n';


  if (args.HasOption(m_endpoint.GetPrefixName()+"-bandwidth"))
    m_endpoint.SetInitialBandwidth(OpalBandwidth::RxTx, args.GetOptionString(m_endpoint.GetPrefixName()+"-bandwidth"));
  if (args.HasOption(m_endpoint.GetPrefixName()+"-rx-bandwidth"))
    m_endpoint.SetInitialBandwidth(OpalBandwidth::Rx, args.GetOptionString(m_endpoint.GetPrefixName()+"-rx-bandwidth"));
  if (args.HasOption(m_endpoint.GetPrefixName()+"-tx-bandwidth"))
    m_endpoint.SetInitialBandwidth(OpalBandwidth::Rx, args.GetOptionString(m_endpoint.GetPrefixName()+"-tx-bandwidth"));


  if (args.HasOption(m_endpoint.GetPrefixName()+"-ui") && !SetUIMode(args.GetOptionString(m_endpoint.GetPrefixName()+"-ui"))) {
    output << "Unknown user indication mode for " << m_endpoint.GetPrefixName() << endl;
    return false;
  }

  if (verbose)
    output << m_endpoint.GetPrefixName() << "user input mode: " << m_endpoint.GetSendUserInputMode() << '\n';


  PCaselessString interfaces = args.GetOptionString(m_endpoint.GetPrefixName());
  if (!m_endpoint.StartListeners(interfaces.Lines())) {
    output << "Could not start listeners for " << m_endpoint.GetPrefixName() << endl;
    return false;
  }

  if (verbose)
    output << m_endpoint.GetPrefixName() << " listening on: " << setfill(',') << m_endpoint.GetListeners() << setfill(' ') << '\n';

  return true;
}


#if P_CLI
void OpalRTPConsoleEndPoint::CmdInterfaces(PCLI::Arguments & args, P_INT_PTR)
{
  if (args.GetCount() < 1)
    args.WriteUsage();
  else {
    PStringArray interfaces(args.GetCount());
    for (PINDEX i = 0; i < args.GetCount(); ++i)
      interfaces[i] = args[i];

    if (m_endpoint.StartListeners(interfaces))
      args.GetContext() << "Listening on: " << setfill(',') << m_endpoint.GetListeners() << setfill(' ') << endl;
    else
      args.WriteError("Could not start .");
  }
}


void OpalRTPConsoleEndPoint::CmdCryptoSuites(PCLI::Arguments & args, P_INT_PTR)
{
  if (args.HasOption("list")) {
    args.GetContext() << "All crypto suites: " << setfill(',') << m_endpoint.GetAllMediaCryptoSuites() << setfill (' ') << endl;
    return;
  }

  if (args.GetCount() > 0)
    m_endpoint.SetMediaCryptoSuites(args.GetParameters());

  args.GetContext() << "Current crypto suites: " << setfill(',') << m_endpoint.GetMediaCryptoSuites() << setfill (' ') << endl;
}


void OpalRTPConsoleEndPoint::CmdBandwidth(PCLI::Arguments & args, P_INT_PTR)
{
  if (args.GetCount() < 1)
    args.WriteUsage();
  else {
    OpalBandwidth bandwidth(args[0]);
    if (bandwidth == 0)
      args.WriteError("Illegal bandwidth parameter");
    else {
      if (!args.HasOption("rx") && !args.HasOption("tx"))
        m_endpoint.SetInitialBandwidth(OpalBandwidth::RxTx, bandwidth);
      else {
        if (args.HasOption("rx"))
          m_endpoint.SetInitialBandwidth(OpalBandwidth::Rx, bandwidth);
        if (args.HasOption("tx"))
          m_endpoint.SetInitialBandwidth(OpalBandwidth::Rx, bandwidth);
      }
    }
  }
}


void OpalRTPConsoleEndPoint::CmdUserInput(PCLI::Arguments & args, P_INT_PTR)
{
  if (args.GetCount() < 1)
    args.WriteUsage();
  else if (!SetUIMode(args[0]))
    args.WriteError("Unknown user indication mode");
}


void OpalRTPConsoleEndPoint::AddCommands(PCLI & cli)
{
  cli.SetCommand(m_endpoint.GetPrefixName() & "interfaces", PCREATE_NOTIFIER(CmdInterfaces),
                  "Set listener interfaces");
  cli.SetCommand(m_endpoint.GetPrefixName() & "crypto", PCREATE_NOTIFIER(CmdCryptoSuites),
                  "Set crypto suites in priority order",
                  " --list | [ <suite> ... ]",
                  "l-list. List all possible crypto suite names");
  cli.SetCommand(m_endpoint.GetPrefixName() & "bandwidth", PCREATE_NOTIFIER(CmdBandwidth),
                  "Set bandwidth to use for calls",
                  "[ <dir> ] <bps>",
                  "-rx. Receive bandwidth\n"
                  "-tx. Transmit bandwidth");
  cli.SetCommand(m_endpoint.GetPrefixName() & "ui", PCREATE_NOTIFIER(CmdUserInput),
                  "Set user input mode",
                  "\"inband\" | \"rfc2833\" | \"signal\" | \"string\"");
}
#endif //P_CLI
#endif // OPAL_SIP || OPAL_H323


/////////////////////////////////////////////////////////////////////////////

#if OPAL_SIP
SIPConsoleEndPoint::SIPConsoleEndPoint(OpalConsoleManager & manager)
  : SIPEndPoint(manager)
  , P_DISABLE_MSVC_WARNINGS(4355, OpalRTPConsoleEndPoint(manager, this))
{
}


void SIPConsoleEndPoint::OnRegistrationStatus(const RegistrationStatus & status)
{
  SIPEndPoint::OnRegistrationStatus(status);

  unsigned reasonClass = status.m_reason/100;
  if (reasonClass == 1 || (status.m_reRegistering && reasonClass == 2))
    return;

  *m_console.LockedOutput() << '\n' << status << endl;
}


bool SIPConsoleEndPoint::DoRegistration(ostream & output,
                                        bool verbose,
                                        const PString & aor,
                                        const PString & pwd,
                                        const PArgList & args, 
                                        const char * authId,
                                        const char * realm,
                                        const char * proxy,
                                        const char * mode,
                                        const char * ttl)
{
  SIPRegister::Params params;
  params.m_addressOfRecord  = aor;
  params.m_password         = pwd;
  params.m_authID           = args.GetOptionString(authId);
  params.m_realm            = args.GetOptionString(realm);
  params.m_proxyAddress     = args.GetOptionString(proxy);

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
      output << "Unknown SIP registration mode \"" << str << '"' << endl;
      return false;
    }
  }

  params.m_expire = args.GetOptionString(ttl, "300").AsUnsigned();
  if (params.m_expire < 30) {
    output << "SIP registrar Time To Live must be more than 30 seconds\n";
    return false;
  }

  if (verbose)
    output << "SIP registrar: " << flush;

  PString finalAoR;
  SIP_PDU::StatusCodes status;
  if (!Register(params, finalAoR, &status)) {
    output << "\nSIP registration to " << params.m_addressOfRecord
            << " failed (" << status << ')' << endl;
    return false;
  }

  if (verbose)
    output << finalAoR << endl;

  return true;
}


void SIPConsoleEndPoint::GetArgumentSpec(ostream & strm) const
{
  strm << "[SIP options:]"
          "-no-sip.           Disable SIP\n"
          "S-sip:             SIP listens on interface, defaults to udp$*:5060.\n";
  OpalRTPConsoleEndPoint::GetArgumentSpec(strm);
  strm << "r-register:        SIP registration to server.\n"
          "-register-auth-id: SIP registration authorisation id, default is username.\n"
          "-register-realm:   SIP registration authorisation realm, default is any.\n"
          "-register-proxy:   SIP registration proxy, default is none.\n"
          "-register-ttl:     SIP registration Time To Live, default 300 seconds.\n"
          "-register-mode:    SIP registration mode (normal, single, public, ALG, RFC5626).\n"
          "-proxy:            SIP outbound proxy.\n";
}


bool SIPConsoleEndPoint::Initialise(PArgList & args, bool verbose, const PString & defaultRoute)
{
  OpalConsoleManager::LockedStream lockedOutput(m_console);
  ostream & output = lockedOutput;

  // Set up SIP
  if (args.HasOption("no-sip")) {
    if (verbose)
      output << "SIP protocol disabled.\n";
    return true;
  }

  if (!OpalRTPConsoleEndPoint::Initialise(args, output, verbose))
    return false;

  if (args.HasOption("proxy")) {
    SetProxy(args.GetOptionString("proxy"), args.GetOptionString("user"), args.GetOptionString("password"));
    if (verbose)
      output << "SIP proxy: " << GetProxy() << '\n';
  }

  if (args.HasOption("register")) {
    if (!DoRegistration(output, verbose,
                        args.GetOptionString("register"),
                        args.GetOptionString("password"),
                        args,
                        "register-auth-id",
                        "register-realm",
                        "register-proxy",
                        "register-mode",
                        "register-ttl"))
      return false;
  }

  AddRoutesFor(this, defaultRoute);
  return true;
}


#if P_CLI
void SIPConsoleEndPoint::CmdProxy(PCLI::Arguments & args, P_INT_PTR)
{
  if (args.GetCount() < 1)
    args.WriteUsage();
  else {
    SetProxy(args[0], args.GetOptionString("user"), args.GetOptionString("password"));
    args.GetContext() << "SIP proxy: " << GetProxy() << endl;
  }
}


void SIPConsoleEndPoint::CmdRegister(PCLI::Arguments & args, P_INT_PTR)
{
  DoRegistration(args.GetContext(), true, args[0], args[1], args, "auth-id", "realm", "proxy", "mode", "ttl");
}


void SIPConsoleEndPoint::AddCommands(PCLI & cli)
{
  OpalRTPConsoleEndPoint::AddCommands(cli);
  cli.SetCommand("sip proxy", PCREATE_NOTIFIER(CmdProxy),
                  "Set listener interfaces",
                  "[ options ] <uri>",
                  "-u-user: Username for proxy\n"
                  "-p-password: Password for proxy");
  cli.SetCommand("sip register", PCREATE_NOTIFIER(CmdRegister),
                  "Register with SIP registrar",
                  "[ options ] <address> [ <password> ]",
                  "a-auth-id: Override user for authorisation\n"
                  "r-realm: Set realm for authorisation\n"
                  "p-proxy: Set proxy for registration\n"
                  "m-mode: Set registration mode (normal, single, public)\n"
                  "t-ttl: Set Time To Live for registration\n");
}
#endif // P_CLI
#endif // OPAL_SIP


/////////////////////////////////////////////////////////////////////////////

#if OPAL_H323
H323ConsoleEndPoint::H323ConsoleEndPoint(OpalConsoleManager & manager)
  : H323EndPoint(manager)
  , P_DISABLE_MSVC_WARNINGS(4355, OpalRTPConsoleEndPoint(manager, this))
{
}


void H323ConsoleEndPoint::OnGatekeeperStatus(H323Gatekeeper::RegistrationFailReasons status)
{
  H323Gatekeeper * gk = GetGatekeeper();
  if (gk != NULL)
    *m_console.LockedOutput() << "\nH.323 registration: " << *gk << " - " << status << endl;
}


void H323ConsoleEndPoint::GetArgumentSpec(ostream & strm) const
{
  strm << "[H.323 options:]"
          "-no-h323.           Disable H.323\n"
          "H-h323:             H.323 listens on interface, defaults to tcp$*:1720.\n";
  OpalRTPConsoleEndPoint::GetArgumentSpec(strm);
  strm << "g-gk-host:          H.323 gatekeeper host.\n"
          "G-gk-id:            H.323 gatekeeper identifier.\n"
          "-gk-password:       H.323 gatekeeper password (if different from --password).\n"
          "-gk-alias-limit:    H.323 gatekeeper alias limit (compatibility issue)\n"
          "-gk-sim-pattern.    H.323 gatekeeper alias patern simulation\n"
          "-alias:             H.323 alias name, may be multiple entries.\n"
          "-alias-pattern:     H.323 alias pattern, may be multiple entries.\n"
          "-no-fast.           H.323 fast connect disabled.\n"
          "-no-tunnel.         H.323 tunnel for H.245 disabled.\n";
}


bool H323ConsoleEndPoint::UseGatekeeperFromArgs(const PArgList & args, const char * host, const char * ident, const char * pass)
{
  SetGatekeeperPassword(args.GetOptionString(pass, args.GetOptionString("password")));
  return UseGatekeeper(args.GetOptionString(host), args.GetOptionString(ident));
}


bool H323ConsoleEndPoint::Initialise(PArgList & args, bool verbose, const PString & defaultRoute)
{
  OpalConsoleManager::LockedStream lockedOutput(m_console);
  ostream & output = lockedOutput;

  // Set up H.323
  if (args.HasOption("no-h323")) {
    if (verbose)
      output << "H.323 protocol disabled.\n";
    return true;
  }

  if (!OpalRTPConsoleEndPoint::Initialise(args, output, verbose))
    return false;

  DisableFastStart(args.HasOption("no-fast"));
  DisableH245Tunneling(args.HasOption("no-tunnel"));

  PStringArray aliases = args.GetOptionString("alias").Lines();
  for (PINDEX i = 0; i < aliases.GetSize(); ++i)
    AddAliasName(aliases[i]);

  aliases = args.GetOptionString("alias-pattern").Lines();
  for (PINDEX i = 0; i < aliases.GetSize(); ++i)
    AddAliasNamePattern(aliases[i]);

  SetGatekeeperSimulatePattern(args.HasOption("gk-sim-pattern"));

  if (verbose)
    output << "H.323 Aliases: " << setfill(',') << GetAliasNames() << setfill(' ') << "\n"
              "H.323 Alias Patterns: " << (GetGatekeeperSimulatePattern() ? "(simulated)" : "")
                                       << setfill(',') << GetAliasNamePatterns() << setfill(' ') << "\n"
              "H.323 options: "
           << (IsFastStartDisabled() ? "Slow" : "Fast") << " connect, "
           << (IsH245TunnelingDisabled() ? "Separate" : "Tunnelled") << " H.245\n";


  SetGatekeeperAliasLimit(args.GetOptionAs<PINDEX>("gk-alias-limit", GetGatekeeperAliasLimit()));

  if (args.HasOption("gk-host") || args.HasOption("gk-id")) {
    if (!UseGatekeeperFromArgs(args, "gk-host", "gk-id", "gk-password")) {
      output << "Could not initiate gatekeeper registration." << endl;
      return false;
    }
    if (verbose)
      output << "H.323 Gatekeeper: " << *GetGatekeeper() << " (awaiting respone)\n";
  }

  AddRoutesFor(this, defaultRoute);
  return true;
}


#if P_CLI
void H323ConsoleEndPoint::CmdAlias(PCLI::Arguments & args, P_INT_PTR)
{
  int operation = args.HasOption('d') ? 1 : 0;
  if (args.HasOption('p'))
    operation |= 2;

  if (args.HasOption('r')) {
    if (operation < 2)
      SetLocalUserName(GetLocalUserName());
    else
      SetAliasNamePatterns(PStringList());
  }

  for (PINDEX i = 0; i < args.GetCount(); ++i) {
    switch (operation) {
      case 0: AddAliasName(args[i]); break;
      case 1: RemoveAliasName(args[i]); break;
      case 2: AddAliasNamePattern(args[i]); break;
      case 3: RemoveAliasNamePattern(args[i]); break;
    }
  }

  if (operation < 2)
    args.GetContext() << "Aliases: " << setfill(',') << GetAliasNames() << setfill(' ') << endl;
  else
    args.GetContext() << "Alias Patterns: " << setfill(',') << GetAliasNamePatterns() << setfill(' ') << endl;
}


void H323ConsoleEndPoint::CmdGatekeeper(PCLI::Arguments & args, P_INT_PTR)
{
  SetGatekeeperAliasLimit(args.GetOptionAs<PINDEX>("limit", GetGatekeeperAliasLimit()));

  if (args.GetCount() < 1) {
    if (GetGatekeeper() != NULL)
      args.GetContext() << "Gatekeeper: " << *GetGatekeeper() << endl;
    else
      args.GetContext() << "No gatekeeper active." << endl;
  }
  else if (args[0] *= "off")
    RemoveGatekeeper();
  else if (args[0] *= "on") {
    args.GetContext() << "H.323 Gatekeeper: " << flush;
    if (UseGatekeeperFromArgs(args, "host", "identifier", "password"))
      args.GetContext()<< *GetGatekeeper() << endl;
    else
      args.GetContext() << "unavailable" << endl;
  }
  else
    args.WriteUsage();
}


void H323ConsoleEndPoint::AddCommands(PCLI & cli)
{
  OpalRTPConsoleEndPoint::AddCommands(cli);

  cli.SetCommand("h323 fast", disableFastStart, "Fast Connect Disable");
  cli.SetCommand("h323 tunnel", disableH245Tunneling, "H.245 Tunnelling Disable");
  cli.SetCommand("h323 h245-in-setup", disableH245inSetup, "H.245 in SETUP Disable");

  cli.SetCommand("h323 alias", PCREATE_NOTIFIER(CmdGatekeeper),
                 "Set alias name(s)",
                 "[ <options> ] [ <name> ... ]",
                 "r-reset:  Reset the alias list before starting\n"
                 "p-pattern: Aliases are patterns (e.g. \"1100*\" or \"1100-1199\")\n"
                 "d-delete: Delete the specified alias");
  cli.SetCommand("h323 gatekeeper", PCREATE_NOTIFIER(CmdGatekeeper),
                  "Set gatekeeper",
                  "[ options ] [ \"on\" / \"off\" ]",
                  "h-host: Host name or IP address of gatekeeper\n"
                  "i-identifier: Identifier for gatekeeper\n"
                  "p-password: Password for H.235.1 authentication\n"
                  "l-limit: Alias limit for gatekeeper");
}
#endif // P_CLI
#endif // OPAL_H323


/////////////////////////////////////////////////////////////////////////////

#if OPAL_SKINNY
OpalConsoleSkinnyEndPoint::OpalConsoleSkinnyEndPoint(OpalConsoleManager & manager)
: OpalSkinnyEndPoint(manager)
, OpalConsoleEndPoint(manager)
{
}


void OpalConsoleSkinnyEndPoint::GetArgumentSpec(ostream & strm) const
{
  strm << "[PSTN options:]"
    "-no-sccp.        Disable Skinny Client Control Protocol\n"
    "-sccp-server:    Set skinny server address.\n";
}


bool OpalConsoleSkinnyEndPoint::Initialise(PArgList & args, bool verbose, const PString & defaultRoute)
{
  OpalConsoleManager::LockedStream lockedOutput(m_console);
  ostream & output = lockedOutput;

  // If we have LIDs speficied in command line, load them
  if (args.HasOption("no-sccp")) {
    if (verbose)
      output << "Skinny disabled.\n";
    return true;
  }

  PString server = args.GetOptionString("sccp-server");
  if (!server.IsEmpty()) {
    if (!Register(server))
      output << "Could not register with skinny server \"" << server << '"' << endl;
    else if (verbose)
      output << "Skinny server: " << server << '\n';
  }

  AddRoutesFor(this, defaultRoute);
  return true;
}


#if P_CLI
void OpalConsoleSkinnyEndPoint::CmdServer(PCLI::Arguments & args, P_INT_PTR)
{
  if (args.GetCount() < 1)
    args.Usage();
  else if (!Register(args[0]))
    args.WriteError() << "Could not register with skinny server \"" << args[0] << '"' << endl;
}


void OpalConsoleSkinnyEndPoint::AddCommands(PCLI & cli)
{
  cli.SetCommand("sccp server", PCREATE_NOTIFIER(CmdServer), "Set skinny server", "[ <host> ]");
}
#endif // P_CLI
#endif // OPAL_SKINNY


/////////////////////////////////////////////////////////////////////////////

#if OPAL_LID
OpalConsoleLineEndPoint::OpalConsoleLineEndPoint(OpalConsoleManager & manager)
: OpalLineEndPoint(manager)
, OpalConsoleEndPoint(manager)
{
}


void OpalConsoleLineEndPoint::GetArgumentSpec(ostream & strm) const
{
  strm << "[PSTN options:]"
    "-no-lid.           Disable Line Interface Devices\n"
    "L-lines:           Set Line Interface Devices.\n"
    "-country:          Select country to use for LID (eg \"US\", \"au\" or \"+61\").\n";
}


bool OpalConsoleLineEndPoint::Initialise(PArgList & args, bool verbose, const PString & defaultRoute)
{
  OpalConsoleManager::LockedStream lockedOutput(m_console);
  ostream & output = lockedOutput;

  // If we have LIDs speficied in command line, load them
  if (args.HasOption("no-lid")) {
    if (verbose)
      output << "PSTN disabled.\n";
    return true;
  }

  if (!args.HasOption("lines")) {
    output << "No PSTN lines supplied.\n";
    return true;
  }

  if (!AddDeviceNames(args.GetOptionString("lines").Lines())) {
    output << "Could not start Line Interface Device(s)" << endl;
    return false;
  }
  if (verbose)
    output << "Line Interface listening on: " << setfill(',') << GetLines() << setfill(' ') << '\n';

  PString country = args.GetOptionString("country");
  if (!country.IsEmpty()) {
    if (!SetCountryCodeName(country))
      output << "Could not set LID to country name \"" << country << '"' << endl;
    else if (verbose)
      output << "LID to country: " << GetLine("*")->GetDevice().GetCountryCodeName() << '\n';
  }

  AddRoutesFor(this, defaultRoute);
  return true;
}


#if P_CLI
void OpalConsoleLineEndPoint::CmdCountry(PCLI::Arguments & args, P_INT_PTR)
{
  if (args.GetCount() < 1)
    args.Usage();
  else if (!SetCountryCodeName(args[0]))
    args.WriteError() << "Could not set LID to country name \"" << args[0] << '"' << endl;
}


void OpalConsoleLineEndPoint::AddCommands(PCLI & cli)
{
  cli.SetCommand("pstn country", PCREATE_NOTIFIER(CmdCountry),
                 "Set country code or name",
                 "[ <name> ]");
}
#endif // P_CLI
#endif // OPAL_LID


/////////////////////////////////////////////////////////////////////////////

#if OPAL_CAPI
OpalConsoleCapiEndPoint::OpalConsoleCapiEndPoint(OpalConsoleManager & manager)
  : OpalCapiEndPoint(manager)
  , OpalConsoleEndPoint(manager)
{
}


void OpalConsoleCapiEndPoint::GetArgumentSpec(ostream & strm) const
{
  strm << "[ISDN (CAPI) options:]"
          "-no-capi.          Disable ISDN via CAPI\n";
}


bool OpalConsoleCapiEndPoint::Initialise(PArgList & args, bool verbose, const PString & defaultRoute)
{
  OpalConsoleManager::LockedStream lockedOutput(m_console);
  ostream & output = lockedOutput;

  if (args.HasOption("no-capi")) {
    if (verbose)
      output << "CAPI ISDN disabled.\n";
    return true;
  }

  unsigned controllers = OpenControllers();
  if (verbose) {
    if (controllers == 0)
      output << "No CAPI controllers available.\n";
    else
      output << "Found " << controllers << " CAPI controllers.\n";
  }

  AddRoutesFor(this, defaultRoute);
  return true;
}


#if P_CLI
void OpalConsoleCapiEndPoint::AddCommands(PCLI &)
{
}
#endif // P_CLI
#endif // OPAL_CAPI


/////////////////////////////////////////////////////////////////////////////

#if OPAL_HAS_PCSS

static struct {
  PSoundChannel::Directions m_dir;
  const char * m_name;
  const char * m_description;
  const PString & (OpalPCSSEndPoint:: *m_get)() const;
  PBoolean (OpalPCSSEndPoint:: *m_set)(const PString &);

  bool Initialise(OpalPCSSEndPoint & ep, ostream & output, bool verbose, const PArgList & args, bool fromCLI)
  {
    PString prefix;
    if (!fromCLI) {
      prefix += m_name;
      prefix += '-';
    }

    PString driver = args.GetOptionString(prefix + "driver");
    if (!driver.IsEmpty())
      driver += '\t';

    PString device = fromCLI ? args[0] : args.GetOptionString(prefix + "device");
    if (device.IsEmpty() && !driver.IsEmpty())
      device = '*';

    if ((!driver.IsEmpty() || !device.IsEmpty()) && !(ep.*m_set)(driver + device)) {
      output << "Illegal audio " << m_description << " driver/device, select one of:";
      PStringArray available = PSoundChannel::GetDeviceNames(m_dir);
      for (PINDEX i = 0; i < available.GetSize(); ++i)
        output << "\n   " << available[i];
      output << endl;
      return false;
    }

    if (verbose)
      output << "Audio " << m_description << ": " << (ep.*m_get)() << endl;

    return true;
  }
} AudioDeviceVariables[] = {
  { PSoundChannel::Recorder, "record", "recorder", &OpalPCSSEndPoint::GetSoundChannelRecordDevice, &OpalPCSSEndPoint::SetSoundChannelRecordDevice },
  { PSoundChannel::Player,   "play",   "player",   &OpalPCSSEndPoint::GetSoundChannelPlayDevice,   &OpalPCSSEndPoint::SetSoundChannelPlayDevice   },
  { PSoundChannel::Recorder, "moh",    "on hold",  &OpalPCSSEndPoint::GetSoundChannelOnHoldDevice, &OpalPCSSEndPoint::SetSoundChannelOnHoldDevice },
  { PSoundChannel::Recorder, "aor",    "on ring",  &OpalPCSSEndPoint::GetSoundChannelOnRingDevice, &OpalPCSSEndPoint::SetSoundChannelOnRingDevice }
};

#if OPAL_VIDEO
static struct {
  const char * m_name;
  const char * m_description;
  const PVideoDevice::OpenArgs & (OpalPCSSEndPoint:: *m_get)() const;
  PBoolean (OpalPCSSEndPoint:: *m_set)(const PVideoDevice::OpenArgs &);
  PStringArray (*m_list)(const PString &, PPluginManager *);

  bool Initialise(OpalPCSSEndPoint & ep, ostream & output, bool verbose, const PArgList & args, bool fromCLI)
  {
    PString prefix;
    if (!fromCLI) {
      prefix += m_name;
      prefix += '-';
    }

    PVideoDevice::OpenArgs video = (ep.*m_get)();
    video.driverName = args.GetOptionString(prefix+"driver");
    video.deviceName = fromCLI ? args[0] : args.GetOptionString(prefix+"device");
    video.channelNumber = args.GetOptionString(prefix+"channel").AsUnsigned();

    PString fmt = args.GetOptionString(prefix+"format");
    if (!fmt.IsEmpty() && (video.videoFormat = PVideoDevice::VideoFormatFromString(fmt, false)) == PVideoDevice::NumVideoFormat) {
      output << "Illegal video " << m_description << " format \"" << fmt << '"' << endl;
      return false;
    }

    if ((!video.driverName.IsEmpty() || !video.deviceName.IsEmpty()) && !(ep.*m_set)(video)) {
      output << "Illegal video " << m_description << " driver/device, select one of:";
      PStringArray available = m_list("*", NULL);
      for (PINDEX i = 0; i < available.GetSize(); ++i)
        output << "\n   " << available[i];
      output << endl;
      return false;
    }

    if (verbose)
      output << "Video " << m_description << ": " << (ep.*m_get)().deviceName << endl;

    return true;
  }
} VideoDeviceVariables[] = {
  { "grabber", "input grabber",  &OpalPCSSEndPoint::GetVideoGrabberDevice, &OpalPCSSEndPoint::SetVideoGrabberDevice, &PVideoInputDevice::GetDriversDeviceNames  },
  { "preview", "input preview",  &OpalPCSSEndPoint::GetVideoPreviewDevice, &OpalPCSSEndPoint::SetVideoPreviewDevice, &PVideoOutputDevice::GetDriversDeviceNames },
  { "display", "output display", &OpalPCSSEndPoint::GetVideoDisplayDevice, &OpalPCSSEndPoint::SetVideoDisplayDevice, &PVideoOutputDevice::GetDriversDeviceNames },
  { "voh",     "on hold",        &OpalPCSSEndPoint::GetVideoOnHoldDevice,  &OpalPCSSEndPoint::SetVideoOnHoldDevice,  &PVideoInputDevice::GetDriversDeviceNames  },
  { "vor",     "on ring",        &OpalPCSSEndPoint::GetVideoOnRingDevice,  &OpalPCSSEndPoint::SetVideoOnRingDevice,  &PVideoInputDevice::GetDriversDeviceNames  }
};
#endif // OPAL_VIDEO


OpalConsolePCSSEndPoint::OpalConsolePCSSEndPoint(OpalConsoleManager & manager)
  : OpalPCSSEndPoint(manager)
  , OpalConsoleEndPoint(manager)
{
}


void OpalConsolePCSSEndPoint::GetArgumentSpec(ostream & strm) const
{
  strm << "[PC options:]";
  for (PINDEX i = 0; i < PARRAYSIZE(AudioDeviceVariables); ++i) {
    const char * name = AudioDeviceVariables[i].m_name;
    const char * desc = AudioDeviceVariables[i].m_description;
    strm << '-' << name << "-driver: Audio " << desc << " driver.\n"
            "-" << name << "-device: Audio " << desc << " device.\n";
  }
  strm << "-audio-buffer:   Audio buffer time in ms (default 120)\n";

#if OPAL_VIDEO
  for (PINDEX i = 0; i < PARRAYSIZE(VideoDeviceVariables); ++i) {
    const char * name = VideoDeviceVariables[i].m_name;
    const char * desc = VideoDeviceVariables[i].m_description;
    strm << '-' << name << "-driver:  Video " << desc << " driver.\n"
            "-" << name << "-device:  Video " << desc << " device.\n"
            "-" << name << "-format:  Video " << desc << " format (\"pal\"/\"ntsc\")\n"
            "-" << name << "-channel: Video " << desc << " channel number.\n";
  }
#endif // OPAL_VIDEO
}


bool OpalConsolePCSSEndPoint::Initialise(PArgList & args, bool verbose, const PString &)
{
  OpalConsoleManager::LockedStream lockedOutput(m_console);
  ostream & output = lockedOutput;

  for (PINDEX i = 0; i < PARRAYSIZE(AudioDeviceVariables); ++i) {
    if (!AudioDeviceVariables[i].Initialise(*this, output, verbose, args, false))
      return false;
  }

  if (args.HasOption("audio-buffer"))
    SetSoundChannelBufferTime(args.GetOptionString("audio-buffer").AsUnsigned());
  if (verbose)
    output << "Audio buffer time: " << GetSoundChannelBufferTime() << "ms\n";

#if OPAL_VIDEO
  for (PINDEX i = 0; i < PARRAYSIZE(VideoDeviceVariables); ++i) {
    if (!VideoDeviceVariables[i].Initialise(*this, output, verbose, args, false))
      return false;
  }
#endif // OPAL_VIDEO

  return true;
}


#if P_CLI
void OpalConsolePCSSEndPoint::CmdVolume(PCLI::Arguments & args, P_INT_PTR)
{
  PSafePtr<OpalConnection> connection = GetConnectionWithLock(args.GetOptionString('c', "*"), PSafeReadOnly);
  if (connection == NULL) {
    args.WriteError("No call in progress.");
    return;
  }

  bool mike = args.GetCommandName().Find("speaker") == P_MAX_INDEX;

  if (args.GetCount() == 0) {
    unsigned percent;
    if (connection->GetAudioVolume(mike, percent))
      args.GetContext() << percent << '%' << endl;
    else
      args.WriteError("Could not get volume.");
  }
  else {
    if (!connection->SetAudioVolume(mike, args[0].AsUnsigned()))
      args.WriteError("Could not set volume.");
  }
}


void OpalConsolePCSSEndPoint::CmdAudioDevice(PCLI::Arguments & args, P_INT_PTR)
{
  for (PINDEX i = 0; i < PARRAYSIZE(AudioDeviceVariables); ++i) {
    if (args.GetCommandName().Find(AudioDeviceVariables[i].m_name) != P_MAX_INDEX)
      AudioDeviceVariables[i].Initialise(*this, args.GetContext(), true, args, true);
  }
}


void OpalConsolePCSSEndPoint::CmdAudioBuffers(PCLI::Arguments & args, P_INT_PTR)
{
  if (args.GetCount() > 0)
    SetSoundChannelBufferTime(args[0].AsUnsigned());
  args.GetContext() << "Audio buffer time: " << GetSoundChannelBufferTime() << "ms" << endl;
}


#if OPAL_VIDEO
void OpalConsolePCSSEndPoint::CmdVideoDevice(PCLI::Arguments & args, P_INT_PTR)
{
  for (PINDEX i = 0; i < PARRAYSIZE(VideoDeviceVariables); ++i) {
    if (args.GetCommandName().Find(VideoDeviceVariables[i].m_name) != P_MAX_INDEX)
      VideoDeviceVariables[i].Initialise(*this, args.GetContext(), true, args, true);
  }
}
#endif // OPAL_VIDEO


void OpalConsolePCSSEndPoint::AddCommands(PCLI & cli)
{
  for (PINDEX i = 0; i < PARRAYSIZE(AudioDeviceVariables); ++i)
    cli.SetCommand(GetPrefixName() & AudioDeviceVariables[i].m_name, PCREATE_NOTIFIER(CmdAudioDevice),
                    PSTRSTRM("Audio " << AudioDeviceVariables[i].m_description << " device."),
                    "[ option ] <name>", "D-driver:  Optional driver name.");

  cli.SetCommand(GetPrefixName() & "buffers", m_soundChannelBufferTime, "Audio Buffer Time", 20, 1000, "Audio buffer time in ms");

#if OPAL_VIDEO
  for (PINDEX i = 0; i < PARRAYSIZE(VideoDeviceVariables); ++i)
    cli.SetCommand(GetPrefixName() & VideoDeviceVariables[i].m_name, PCREATE_NOTIFIER(CmdVideoDevice),
                    PSTRSTRM("Video " << VideoDeviceVariables[i].m_description << " device."),
                    "[ options ] <name>",
                    "-driver:  Driver name.\n"
                    "-format:  Format (\"pal\"/\"ntsc\")\n"
                    "-channel: Channel number.\n");
#endif // OPAL_VIDEO

  cli.SetCommand(GetPrefixName() & "microphone volume", PCREATE_NOTIFIER(CmdVolume),
                  "Set volume for microphone",
                  "[ <percent> ]", "c-call: Call token");
  cli.SetCommand(GetPrefixName() & "speaker volume", PCREATE_NOTIFIER(CmdVolume),
                  "Set volume for speaker",
                  "[ <percent> ]", "c-call: Call token");
}
#endif // P_CLI
#endif // OPAL_HAS_PCSS


/////////////////////////////////////////////////////////////////////////////

#if OPAL_IVR
OpalConsoleIVREndPoint::OpalConsoleIVREndPoint(OpalConsoleManager & manager)
  : OpalIVREndPoint(manager)
  , OpalConsoleEndPoint(manager)
{
}


void OpalConsoleIVREndPoint::GetArgumentSpec(ostream & strm) const
{
  strm << "[Interactive Voice Response options:]"
          "-no-ivr.     Disable IVR subsystem\n"
          "-ivr-script: The default VXML script to run\n";
}


bool OpalConsoleIVREndPoint::Initialise(PArgList & args, bool verbose, const PString &)
{
  OpalConsoleManager::LockedStream lockedOutput(m_console);
  ostream & output = lockedOutput;

  if (args.HasOption("no-ivr")) {
    if (verbose)
      output << "IVR disabled.\n";
    return true;
  }

  PString vxml = args.GetOptionString("ivr-script");
  if (!vxml.IsEmpty()) {
    if (verbose)
      output << "Set default IVR script: " << vxml.Left(vxml.FindOneOf("\r\n")) << '\n';
    SetDefaultVXML(vxml);
  }

  return true;
}


#if P_CLI
void OpalConsoleIVREndPoint::AddCommands(PCLI &)
{
}
#endif // P_CLI
#endif // OPAL_IVR


/////////////////////////////////////////////////////////////////////////////

#if OPAL_HAS_MIXER
OpalConsoleMixerEndPoint::OpalConsoleMixerEndPoint(OpalConsoleManager & manager)
  : OpalMixerEndPoint(manager)
  , OpalConsoleEndPoint(manager)
{
}


void OpalConsoleMixerEndPoint::GetArgumentSpec(ostream & strm) const
{
  strm << "[Mixer (MCU) options:]"
          "-no-mcu.       Disable MCU subsystem\n"
          "-audio-only.   Audio only conference\n";
}


bool OpalConsoleMixerEndPoint::Initialise(PArgList & args, bool verbose, const PString &)
{
  OpalConsoleManager::LockedStream lockedOutput(m_console);
  ostream & output = lockedOutput;

  if (args.HasOption("no-mcu")) {
    if (verbose)
      output << "Conference disabled.\n";
    return true;
  }

  OpalMixerNodeInfo adHoc;
  adHoc.m_audioOnly = args.HasOption("audio-only");
  SetAdHocNodeInfo(adHoc);

  return true;
}


#if P_CLI
void OpalConsoleMixerEndPoint::AddCommands(PCLI &)
{
}
#endif // P_CLI
#endif // OPAL_HAS_MIXER


/////////////////////////////////////////////////////////////////////////////

OpalConsoleManager::OpalConsoleManager(const char * endpointPrefixes)
  : m_endpointPrefixes(PConstString(endpointPrefixes).Tokenise(" \t\n"))
  , m_interrupted(false)
  , m_verbose(false)
  , m_outputStream(&cout)
{
}


OpalConsoleManager::~OpalConsoleManager()
{
  // Must do this before m_outputStream and m_outputMutex go out of scope
  ShutDownEndpoints();
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
         "-tel:              Protocol to use for tel: URI, e.g. sip\n"
         "[Audio options:]"
         "-jitter:           Set audio jitter buffer size (min[,max] default 50,250)\n"
         "-silence-detect:   Set audio silence detect mode (\"none\", \"fixed\" or default \"adaptive\")\n"
         "-no-inband-detect. Disable detection of in-band tones.\n";

#if OPAL_VIDEO
  str << "[Video options:]"
         "-max-video-size:   Set maximum received video size, of form 800x600 or \"CIF\" etc (default CIF)\n"
         "-video-size:       Set preferred transmit video size, of form 800x600 or \"CIF\" etc (default HD1080)\n"
         "-video-rate:       Set preferred transmit video frame rate, in fps (default 30)\n"
         "-video-bitrate:    Set target transmit video bit rate, in bps, suffix 'k' or 'M' may be used (default 1Mbps)\n";
#endif

  for (PINDEX i = 0; i < m_endpointPrefixes.GetSize(); ++i) {
    OpalConsoleEndPoint * ep = const_cast<OpalConsoleManager *>(this)->GetConsoleEndPoint(m_endpointPrefixes[i]);
    if (ep != NULL)
      ep->GetArgumentSpec(str);
  }

#if OPAL_PTLIB_SSL
  str << "[SSL options:]"
         "-ssl-ca:           Set SSL certificate authority directory/file.\n"
         "-ssl-cert:         Set SSL certificate for local client.\n"
         "-ssl-key:          Set SSL private key lor local certificate.\n"
         "-ssl-no-create.    Do not auto-create SSL certificate/private key if do not exist.\n";
#endif

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
    Usage(LockedOutput(), args);
    return false;
  }

  if (args.HasOption("version")) {
    PrintVersion(LockedOutput());
    return false;
  }

  return true;
}


bool OpalConsoleManager::Initialise(PArgList & args, bool verbose, const PString & defaultRoute)
{
  if (!PreInitialise(args, verbose))
    return false;

  PTRACE_INITIALISE(args);

  LockedStream lockedOutput(*this);
  ostream & output = lockedOutput;

  if (args.HasOption("user"))
    SetDefaultUserName(args.GetOptionString("user"));
  if (verbose) {
    output << "Default user name: " << GetDefaultUserName();
    if (args.HasOption("password"))
      output << " (with password)";
    output << '\n';
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
        output << "Invalid jitter specification\n";
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

#if OPAL_PTLIB_SSL
  m_caFiles = args.GetOptionString("ssl-ca", m_caFiles);
  m_certificateFile = args.GetOptionString("ssl-cert", m_certificateFile);
  m_privateKeyFile = args.GetOptionString("ssl-key", m_privateKeyFile);
  m_autoCreateCertificate = !args.HasOption("ssl-no-create");
  if (verbose)
    output << "SSL certificate authority: " << m_caFiles << "\n"
              "SSL certificate: " << m_certificateFile << "\n"
              "SSL private key: " << m_privateKeyFile << "\n"
              "SSL auto-create certificate/key: " << (m_autoCreateCertificate ? "Yes" : "No") << '\n';
#endif

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
      output << "IP Type Of Service bits must be 0 to 255.\n";
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
      output << "RTP maximum payload size 32 to 65500.\n";
      return false;
    }
    SetMaxRtpPayloadSize(size);
  }

  if (verbose)
    output << "TCP ports: " << GetTCPPortRange() << "\n"
              "UDP ports: " << GetUDPPortRange() << "\n"
              "RTP ports: " << GetRtpIpPortRange() << "\n"
              "Audio QoS: " << GetMediaQoS(OpalMediaType::Audio()) << "\n"
#if OPAL_VIDEO
              "Video QoS: " << GetMediaQoS(OpalMediaType::Video()) << "\n"
#endif
              "RTP payload size: " << GetMaxRtpPayloadSize() << '\n';

#if P_NAT
  PString natMethod, natServer;
  if (args.HasOption("translate")) {
    natMethod = PNatMethod_Fixed::MethodName();
    natServer = args.GetOptionString("translate");
  }
#if P_STUN
  else if (args.HasOption("stun")) {
    natMethod = PSTUNClient::MethodName();
    natServer = args.GetOptionString("stun");
  }
#endif
  else if (args.HasOption("nat-method")) {
    natMethod = args.GetOptionString("nat-method");
    natServer = args.GetOptionString("nat-server");
  }
  else if (args.HasOption("nat-server")) {
#if P_STUN
    natMethod = PSTUNClient::MethodName();
#else
    natMethod = PNatMethod_Fixed::MethodName();
#endif
    natServer = args.GetOptionString("nat-server");
  }

  if (!natMethod.IsEmpty()) {
    if (verbose)
      output << natMethod << " server: " << flush;
    SetNATServer(natMethod, natServer);
    if (verbose) {
      PNatMethod * nat = GetNatMethods().GetMethodByName(natMethod);
      if (nat == NULL)
        output << "Unavailable";
      else {
        PNatMethod::NatTypes natType = nat->GetNatType();
        output << '"' << nat->GetServer() << "\" replies " << natType;
        PIPSocket::Address externalAddress;
        if (natType != PNatMethod::BlockedNat && nat->GetExternalAddress(externalAddress))
          output << " with external address " << externalAddress;
      }
      output << '\n';
    }
  }
#endif // P_NAT

  if (verbose) {
    PIPSocket::InterfaceTable interfaceTable;
    if (PIPSocket::GetInterfaceTable(interfaceTable))
      output << "Detected " << interfaceTable.GetSize() << " network interfaces:\n"
               << setfill('\n') << interfaceTable << setfill(' ');
  }

  for (PINDEX i = 0; i < m_endpointPrefixes.GetSize(); ++i) {
    OpalConsoleEndPoint * ep = GetConsoleEndPoint(m_endpointPrefixes[i]);
    if (ep != NULL && !ep->Initialise(args, verbose, defaultRoute))
      return false;
  }

  PString telProto = args.GetOptionString("tel");
  if (!telProto.IsEmpty()) {
    OpalEndPoint * ep = FindEndPoint(telProto);
    if (ep == NULL) {
      output << "The \"tel\" URI cannot be mapped to protocol \"" << telProto << '"' << endl;
      return false;
    }

    AttachEndPoint(ep, "tel");
    if (verbose)
      output << "tel URI mapped to: " << ep->GetPrefixName() << '\n';
  }

#if OPAL_VIDEO
  {
    unsigned prefWidth = 0, prefHeight = 0;
    if (!PVideoFrameInfo::ParseSize(args.GetOptionString("video-size", "cif"), prefWidth, prefHeight)) {
      output << "Invalid video size parameter." << endl;
      return false;
    }
    if (verbose)
      output << "Preferred video size: " << PVideoFrameInfo::AsString(prefWidth, prefHeight) << '\n';

    unsigned maxWidth = 0, maxHeight = 0;
    if (!PVideoFrameInfo::ParseSize(args.GetOptionString("max-video-size", "HD1080"), maxWidth, maxHeight)) {
      output << "Invalid maximum video size parameter." << endl;
      return false;
    }
    if (verbose)
      output << "Maximum video size: " << PVideoFrameInfo::AsString(maxWidth, maxHeight) << '\n';

    double rate = args.GetOptionString("video-rate", "30").AsReal();
    if (rate < 1 || rate > 60) {
      output << "Invalid video frame rate parameter." << endl;
      return false;
    }
    if (verbose)
      output << "Video frame rate: " << rate << " fps\n";

    unsigned frameTime = (unsigned)(OpalMediaFormat::VideoClockRate/rate);
    OpalBandwidth bitrate(args.GetOptionString("video-bitrate", "1Mbps"));
    if (bitrate < 10000) {
      output << "Invalid video bit rate parameter." << endl;
      return false;
    }
    if (verbose)
      output << "Video target bit rate: " << bitrate << '\n';

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
        output << "Invalid media format option \"" << options[i] << '"' << endl;
        return false;
      }

      OpalMediaFormat format(subexpressions[1]);
      if (!format.IsValid()) {
        output << "Unknown media format \"" << subexpressions[1] << '"' << endl;
        return false;
      }
      if (!format.HasOption(subexpressions[2])) {
        output << "Unknown option name \"" << subexpressions[2] << "\""
                    " in media format \"" << subexpressions[1] << '"' << endl;
        return false;
      }
      if (!format.SetOptionValue(subexpressions[2], subexpressions[3])) {
        output << "Ilegal value \"" << subexpressions[3] << "\""
                    " for option name \"" << subexpressions[2] << "\""
                    " in media format \"" << subexpressions[1] << '"' << endl;
        return false;
      }
      if (verbose)
        output << "Set " << format << " option " << subexpressions[2] << " to " << subexpressions[3] << '\n';
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
    output << "Media Formats: " << setfill(',') << formats << setfill(' ') << '\n';
  }

#if OPAL_STATISTICS
  m_statsPeriod.SetInterval(0, args.GetOptionString("stat-time").AsUnsigned());
  m_statsFile = args.GetOptionString("stat-file");
  if (m_statsPeriod == 0 && args.HasOption("statistics"))
    m_statsPeriod.SetInterval(0, 5);
#endif

  if (m_verbose)
    output.flush();

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
    *LockedOutput() << "Shutting down " << PProcess::Current().GetName() << " . . . " << endl;

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
    else
#endif // OPAL_SIP
#if OPAL_H323
    if (prefix == OPAL_PREFIX_H323)
      ep = CreateH323EndPoint();
    else
#endif // OPAL_H323
#if OPAL_SKINNY
    if (prefix == OPAL_PREFIX_SKINNY)
      ep = CreateSkinnyEndPoint();
    else
#endif // OPAL_SKINNY
#if OPAL_LID
    if (prefix == OPAL_PREFIX_PSTN)
      ep = CreateLineEndPoint();
    else
#endif // OPAL_LID
#if OPAL_CAPI
    if (prefix == OPAL_PREFIX_CAPI)
      ep = CreateCapiEndPoint();
    else
#endif // OPAL_LID
#if OPAL_HAS_PCSS
    if (prefix == OPAL_PREFIX_PCSS)
      ep = CreatePCSSEndPoint();
    else
#endif
#if OPAL_IVR
    if (prefix == OPAL_PREFIX_IVR)
      ep = CreateIVREndPoint();
    else
#endif
#if OPAL_HAS_MIXER
    if (prefix == OPAL_PREFIX_MIXER)
      ep = CreateMixerEndPoint();
    else
#endif
    {
      PTRACE(1, "ConsoleApp\tUnknown prefix " << prefix);
      return NULL;
    }
  }

  return dynamic_cast<OpalConsoleEndPoint *>(ep);
}


#if OPAL_SIP
SIPConsoleEndPoint * OpalConsoleManager::CreateSIPEndPoint()
{
  return new SIPConsoleEndPoint(*this);
}
#endif // OPAL_SIP


#if OPAL_H323
H323ConsoleEndPoint * OpalConsoleManager::CreateH323EndPoint()
{
  return new H323ConsoleEndPoint(*this);
}
#endif // OPAL_H323


#if OPAL_SKINNY
OpalConsoleSkinnyEndPoint * OpalConsoleManager::CreateSkinnyEndPoint()
{
  return new OpalConsoleSkinnyEndPoint(*this);
}
#endif // OPAL_SKINNY


#if OPAL_LID
OpalConsoleLineEndPoint * OpalConsoleManager::CreateLineEndPoint()
{
  return new OpalConsoleLineEndPoint(*this);
}
#endif // OPAL_LID


#if OPAL_CAPI
OpalConsoleCapiEndPoint * OpalConsoleManager::CreateCapiEndPoint()
{
  return new OpalConsoleCapiEndPoint(*this);
}
#endif // OPAL_LID


#if OPAL_HAS_PCSS
OpalConsolePCSSEndPoint * OpalConsoleManager::CreatePCSSEndPoint()
{
  return new OpalConsolePCSSEndPoint(*this);
}
#endif


#if OPAL_IVR
OpalConsoleIVREndPoint * OpalConsoleManager::CreateIVREndPoint()
{
  return new OpalConsoleIVREndPoint(*this);
}
#endif


#if OPAL_HAS_MIXER
OpalConsoleMixerEndPoint * OpalConsoleManager::CreateMixerEndPoint()
{
  return new OpalConsoleMixerEndPoint(*this);
}
#endif


void OpalConsoleManager::OnEstablishedCall(OpalCall & call)
{
  if (m_verbose)
    *LockedOutput() << "In call with " << call.GetPartyB() << " using " << call.GetPartyA() << endl;

  OpalManager::OnEstablishedCall(call);
}


void OpalConsoleManager::OnHold(OpalConnection & connection, bool fromRemote, bool onHold)
{
  OpalManager::OnHold(connection, fromRemote, onHold);

  if (!m_verbose)
    return;

  LockedStream lockedOutput(*this);
  ostream & output = *lockedOutput;
  output << "Remote " << connection.GetRemotePartyName() << " has ";
  if (fromRemote)
    output << (onHold ? "put you on" : "released you from");
  else
    output << " been " << (onHold ? "put on" : "released from");
  output << " hold." << endl;
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
    LogMediaStream(LockedOutput(), "Started", stream, connection);
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
    LogMediaStream(LockedOutput(), "Stopped", stream, stream.GetConnection());

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

  LockedStream lockedOutput(*this);
  ostream & output = *lockedOutput;

  switch (call.GetCallEndReason()) {
    case OpalConnection::EndedByRemoteUser :
      output << '"' << name << "\" has cleared the call";
      break;
    case OpalConnection::EndedByCallerAbort :
      output << '"' << name << "\" has stopped calling";
      break;
    case OpalConnection::EndedByRefusal :
      output << '"' << name << "\" did not accept your call";
      break;
    case OpalConnection::EndedByNoAnswer :
      output << '"' << name << "\" did not answer your call";
      break;
    case OpalConnection::EndedByNoAccept :
      output << "Did not accept incoming call from \"" << name << '"';
      break;
    case OpalConnection::EndedByNoUser :
      output << "Could find user \"" << name << '"';
      break;
    case OpalConnection::EndedByUnreachable :
      output << '"' << name << "\" could not be reached.";
      break;
    case OpalConnection::EndedByNoEndPoint :
      output << "No phone running for \"" << name << '"';
      break;
    case OpalConnection::EndedByHostOffline :
      output << '"' << name << "\" is not online.";
      break;
    case OpalConnection::EndedByConnectFail :
      output << "Transport error calling \"" << name << '"';
      break;
    default :
      output << call.GetCallEndReasonText() << " with \"" << name << '"';
  }

  PTime now;
  output << ", on " << now.AsString("w h:mma") << ", duration "
            << setprecision(0) << setw(5) << (now - call.GetStartTime()) << "s."
            << endl;
}


#if OPAL_STATISTICS
bool OpalConsoleManager::OutputStatistics()
{
  if (m_statsFile.IsEmpty())
    return OutputStatistics(LockedOutput());

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
  m_outputStream = &cout;
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
        *LockedOutput() << "Illegal CLI port " << port << endl;
        return false;
      }
      m_cli = CreateCLITelnet((WORD)port);
    }
#endif // P_TELNET

#if P_CURSES
    if (m_cli == NULL && args.HasOption("tui")) {
      PCLICurses * cli = CreateCLICurses();
      if (cli != NULL) {
        PCLICurses::Window * mainWindow = cli->GetWindow(0);
        if (mainWindow == NULL) {
          *LockedOutput() << "Could not create text user interface, probably redirected I/O" << endl;
          return false;
        }
        m_outputStream = mainWindow;
        m_cli = cli;
      }
    }
#endif // P_CURSES

    if (m_cli == NULL && (m_cli = CreateCLIStandard()) == NULL)
      return false;
  }

  m_cli->SetPrompt(args.GetCommandName() + "> ");

#if P_NAT
  m_cli->SetCommand("nat list", PCREATE_NOTIFIER(CmdNatList),
                    "List NAT methods and server addresses");
  m_cli->SetCommand("nat server", PCREATE_NOTIFIER(CmdNatAddress),
                    "Set NAT server address for method, \"off\" deactivates method",
                    "<method> <address>");
#endif

#if PTRACING
  m_cli->SetCommand("trace", PCREATE_NOTIFIER(CmdTrace),
                    "Set trace level (1..6) and filename",
                    "<n> [ <filename> ]");
#endif

#if OPAL_STATISTICS
  m_cli->SetCommand("statistics", PCREATE_NOTIFIER(CmdStatistics),
                    "Display statistics for call",
                    "[ <call-token> ]");
#endif

  m_cli->SetCommand("codec list", PCREATE_NOTIFIER(CmdCodecList),
                    "List available codecs");
  m_cli->SetCommand("codec order", PCREATE_NOTIFIER(CmdCodecOrderMask),
                    "Set codec selection order. Simple '*' character may be used for wildcard matching.",
                    "[ -a ] [ <wildcard> ... ]", "a-add. Add to existing list");
  m_cli->SetCommand("codec mask", PCREATE_NOTIFIER(CmdCodecOrderMask),
                    "Set codec selection mask. Simple '*' character may be used for wildcard matching.",
                    "[ -a ] [ <wildcard> ... ]", "a-add. Add to existing list");

  m_cli->SetCommand("delay", PCREATE_NOTIFIER(CmdDelay),
                    "Delay for the specified numebr of seconds",
                    "seconds");
  m_cli->SetCommand("version", PCREATE_NOTIFIER(CmdVersion),
                    "Print application vesion number and library details.");
  m_cli->SetCommand("quit\nexit", PCREATE_NOTIFIER(CmdQuit),
                    "Quit command line interpreter, note quitting from console also shuts down application.");
  m_cli->SetCommand("shutdown", PCREATE_NOTIFIER(CmdShutDown),
                    "Shut down the application"
#if _WIN32
                    , NULL, "-wait"
#endif
                    );
  m_cli->SetExitCommand(PString::Empty()); // Using ours

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
      *LockedOutput() << "error: cannot open script file \"" << filename << '"' << endl;
  }

  if (m_cli != NULL)
    m_cli->Start(false);
}


void OpalManagerCLI::EndRun(bool interrupt)
{
  if (m_cli != NULL) {
    m_outputStream = &cout;
    m_cli->Stop();
  }

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
void OpalManagerCLI::CmdNatList(PCLI::Arguments & args, P_INT_PTR)
{
  PCLI::Context & out = args.GetContext();
  out << std::left
      << setw(12) << "Name" << ' '
      << setw(8) << "State" << ' '
      << setw(20) << "Type" << ' '
      << setw(20) << "Server" << ' '
      <<             "External\n";

  for (PNatMethods::iterator it = GetNatMethods().begin(); it != GetNatMethods().end(); ++it) {
    out << setw(12) << it->GetMethodName() << ' '
        << setw(8) << (it->IsAvailable() ? "Active" : "N/A")
        << setw(20);

    PNatMethod::NatTypes type = it->GetNatType();
    if (type != PNatMethod::UnknownNat)
      out << it->GetNatType();
    else
      out << "";

    out << ' ' << setw(20) << it->GetServer();

    PIPSocket::Address externalAddress;
    if (it->GetExternalAddress(externalAddress))
      out << ' ' << externalAddress;

    out << '\n';
  }
  out << endl;
}


void OpalManagerCLI::CmdNatAddress(PCLI::Arguments & args, P_INT_PTR)
{
  if (args.GetCount() < 2) {
    args.WriteUsage();
    return;
  }

  PNatMethod * natMethod = GetNatMethods().GetMethodByName(args[0]);
  if (natMethod == NULL) {
    args.WriteError() << "Invalid NAT method \"" << args[0] << '"';
    return;
  }

  if (args[1] *= "off") {
    args.GetContext() << natMethod->GetMethodName() << " deactivated.";
    natMethod->Activate(false);
    return;
  }

  if (natMethod->SetServer(args[1])) {
    args.WriteError() << natMethod->GetMethodName() << " server address invalid \"" << args[1] << '"';
    return;
  }

  PCLI::Context & out = args.GetContext();
  out << natMethod->GetMethodName() << " server \"" << natMethod->GetServer() << " replies " << natMethod->GetNatType();
  PIPSocket::Address externalAddress;
  if (natMethod->GetExternalAddress(externalAddress))
    out << " with address " << externalAddress;
  out << endl;
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


#if OPAL_STATISTICS
void OpalManagerCLI::CmdStatistics(PCLI::Arguments & args, P_INT_PTR)
{
  if (args.GetCount() == 0) {
    OutputStatistics(LockedOutput());
    return;
  }

  PSafePtr<OpalCall> call = FindCallWithLock(args[0], PSafeReadOnly);
  if (call == NULL) {
    args.WriteError() << "No call with supplied token.\n";
    return;
  }

  OutputCallStatistics(LockedOutput(), *call);
}
#endif // PTRACING


void OpalManagerCLI::CmdCodecList(PCLI::Arguments & args, P_INT_PTR)
{
  OpalMediaFormatList formats;
  OpalMediaFormat::GetAllRegisteredMediaFormats(formats);

  PCLI::Context & out = args.GetContext();
  out << "Audio:\n";
  OpalMediaFormatList::iterator format;
  for (format = formats.begin(); format != formats.end(); ++format) {
    if (format->GetMediaType() == OpalMediaType::Audio() && format->IsTransportable())
      out << "  " << *format << '\n';
  }

#if OPAL_VIDEO
  out << "Video:\n";
  for (format = formats.begin(); format != formats.end(); ++format) {
    if (format->GetMediaType() == OpalMediaType::Video() && format->IsTransportable())
      out << "  " << *format << '\n';
  }
#endif

  out << "Other:\n";
  for (format = formats.begin(); format != formats.end(); ++format) {
    if (format->GetMediaType() != OpalMediaType::Audio() &&
#if OPAL_VIDEO
        format->GetMediaType() != OpalMediaType::Video() &&
#endif
        format->IsTransportable())
        out << "  " << *format << " (" << format->GetMediaType() << ")\n";
  }

  out.flush();
}


void OpalManagerCLI::CmdCodecOrderMask(PCLI::Arguments & args, P_INT_PTR)
{
  bool mask = args.GetCommandName().Find("mask") != P_MAX_INDEX;
  PStringArray formats = mask ? GetMediaFormatMask() : GetMediaFormatOrder();

  if (args.GetCount() > 0) {
    if (!args.HasOption('a'))
      formats.RemoveAll();

    for (PINDEX i = 0; i < args.GetCount(); ++i)
      formats += args[i];

    if (mask)
      SetMediaFormatMask(formats);
    else
      SetMediaFormatOrder(formats);
  }

  args.GetContext() << "Codec " << (mask ? "Mask" : "Order") << ": " << setfill(',') << formats << endl;
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
#if _WIN32
  if (!args.HasOption("wait"))
    PProcess::Current().SetWaitOnExitConsoleWindow(false);
#endif
  EndRun(false);
}


#endif // P_CLI


/////////////////////////////////////////////////////////////////////////////
