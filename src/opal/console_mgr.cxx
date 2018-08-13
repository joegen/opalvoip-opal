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
 */

#include <ptlib.h>

#ifdef __GNUC__
#pragma implementation "console_mgr.h"
#endif

#include <opal/console_mgr.h>

#include <opal/patch.h>
#include <h323/gkclient.h>
#include <codec/vidcodec.h>
#include <rtp/srtp_session.h>
#include <ptclib/pstun.h>
#include <ptclib/pwavfile.h>


#define PTraceModule() "Console"


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


bool OpalManagerConsole::GetCallFromArgs(PCLI::Arguments & args, PSafePtr<OpalCall> & call) const
{
  if (GetCallCount() == 0) {
    args.WriteError("No calls active.");
    return false;
  }

  if (args.HasOption("call")) {
    if ((call = FindCallWithLock(args.GetOptionString("call"))) != NULL)
      return true;
    args.WriteError("Specified token does not correspond to call.");
    return false;
  }

  PStringArray calls = GetAllCalls();
  for (PINDEX i = 0; i < calls.GetSize(); ++i) {
    if ((call = FindCallWithLock(calls[i])) != NULL)
      return true;
  }

  args.WriteError("Call(s) disappeared while locating.");
  return false;
}


bool OpalManagerConsole::GetStreamFromArgs(PCLI::Arguments & args,
                                           const OpalMediaType & mediaType,
                                           bool source,
                                           PSafePtr<OpalMediaStream> & stream) const
{
  PSafePtr<OpalLocalConnection> connection;
  if (!GetConnectionFromArgs(args, connection))
    return false;

  if ((stream = connection->GetMediaStream(mediaType, source)) != NULL)
    return true;

  args.WriteError() << "No " << (source ? "transmit" : "receive") << ' ' << mediaType << " stream open." << endl;
  return false;
}


template <typename T> static int GetValueFromArgs(PCLI::Arguments & args, const PString & option, T & value, T minimum, T maximum, const char * errorContext)
{
  if (!args.HasOption(option))
    return 0;

  value = args.GetOptionAs<T>(option);
  if (value >= minimum && value <= maximum)
    return 1;

  args.WriteError() << "Value for " << option << " out of range [" << minimum << ".." << maximum << ']' << errorContext << endl;;
  return -1;
}


#if OPAL_VIDEO
static const OpalBandwidth AbsoluteMinBitRate("10kbps");
static const OpalBandwidth AbsoluteMaxBitRate("2Gbps");

static int GetResolutionFromArgs(PCLI::Arguments & args, const PString & option, unsigned & width, unsigned & height, const char * errorContext)
{
  if (!args.HasOption(option))
    return 0;

  PString value = args.GetOptionString(option);
  if (PVideoFrameInfo::ParseSize(value, width, height))
    return 1;

  args.WriteError() << "Not a valid frame resolution (" << value << ')' << errorContext << endl;
  return -1;
}


static bool GetVideoFormatFromArgs(PCLI::Arguments & args, OpalMediaFormat & mediaFormat, bool withMaximums)
{
  unsigned width, height;
  OpalBandwidth bitRate;

  PStringStream errorContext;
  errorContext << " for setting media format " << mediaFormat;
  if (withMaximums) {
    switch (GetResolutionFromArgs(args, "max-size", width, height, errorContext)) {
      case -1:
        return false;
      case 1:
        mediaFormat.SetOptionInteger(OpalVideoFormat::MaxRxFrameWidthOption(), width);
        mediaFormat.SetOptionInteger(OpalVideoFormat::MaxRxFrameHeightOption(), height);
    }

    switch (GetValueFromArgs(args, "max-bit-rate", bitRate, AbsoluteMinBitRate, AbsoluteMaxBitRate, errorContext)) {
      case -1:
        return false;
      case 1:
        mediaFormat.SetOptionInteger(OpalVideoFormat::MaxBitRateOption(), bitRate);
    }
  }

  switch (GetResolutionFromArgs(args, "size", width, height, errorContext)) {
    case -1 :
      return false;
    case 1 :
      mediaFormat.SetOptionInteger(OpalVideoFormat::FrameWidthOption(), width);
      mediaFormat.SetOptionInteger(OpalVideoFormat::FrameHeightOption(), height);
  }

  switch (GetValueFromArgs(args, "bit-rate", bitRate, AbsoluteMinBitRate, mediaFormat.GetMaxBandwidth(), errorContext)) {
    case -1:
      return false;
    case 1:
      mediaFormat.SetOptionInteger(OpalVideoFormat::TargetBitRateOption(), bitRate);
  }

  unsigned frameRate;
  switch (GetValueFromArgs(args, "frame-rate", frameRate, 1U, 30U, errorContext)) {
    case -1:
      return false;
    case 1:
      mediaFormat.SetOptionInteger(OpalMediaFormat::FrameTimeOption(), mediaFormat.GetClockRate() / frameRate);
  }

  unsigned tsto;
  switch (GetValueFromArgs(args, "tsto", tsto, 1U, 31U, errorContext)) {
    case -1:
      return false;
    case 1:
      mediaFormat.SetOptionInteger(OpalVideoFormat::TemporalSpatialTradeOffOption(), tsto);
  }

  return true;
}
#endif // OPAL_VIDEO


void OpalConsoleEndPoint::AddRoutesFor(const OpalEndPoint * endpoint, const PString & defaultRoute)
{
  if (defaultRoute.IsEmpty())
    return;

  PStringList prefixes = m_console.GetPrefixNames(endpoint);

  for (PINDEX i = 0; i < prefixes.GetSize(); ++i)
    m_console.AddRouteEntry(prefixes[i] + ":.* = " + defaultRoute);
}


/////////////////////////////////////////////////////////////////////////////

#if OPAL_H323 || OPAL_SIP || OPAL_SDP_HTTP
OpalRTPConsoleEndPoint::OpalRTPConsoleEndPoint(OpalManagerConsole & console, OpalRTPEndPoint * endpoint)
  : OpalConsoleEndPoint(console)
  , m_endpoint(*endpoint)
{
}


bool OpalRTPConsoleEndPoint::SetUIMode(const PCaselessString & str)
{
  if (str.IsEmpty())
    return true;

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
          "-" << m_endpoint.GetPrefixName() << "-bandwidth:    Set total bandwidth (both directions) to be used for call\n"
          "-" << m_endpoint.GetPrefixName() << "-rx-bandwidth: Set receive bandwidth to be used for call\n"
          "-" << m_endpoint.GetPrefixName() << "-tx-bandwidth: Set transmit bandwidth to be used for call\n"
          "-" << m_endpoint.GetPrefixName() << "-ui:           Set User Indication mode (inband,rfc2833,signal,string)\n"
          "-" << m_endpoint.GetPrefixName() << "-option:       Set string option (key[=value]), may be multiple occurrences\n";
}


bool OpalRTPConsoleEndPoint::Initialise(PArgList & args, ostream & output, bool verbose)
{
  PStringArray cryptoSuites = args.GetOptionString(m_endpoint.GetPrefixName() + "-crypto").Lines();
  if (!cryptoSuites.IsEmpty())
    m_endpoint.SetMediaCryptoSuites(cryptoSuites);

  if (verbose)
    output << m_endpoint.GetPrefixName().ToUpper() << " crypto suites: "
            << setfill(',') << m_endpoint.GetMediaCryptoSuites() << setfill(' ') << '\n';


  if (!m_endpoint.SetInitialBandwidth(OpalBandwidth::RxTx,
                                      args.GetOptionAs(m_endpoint.GetPrefixName() + "-bandwidth",
                                                       m_endpoint.GetInitialBandwidth(OpalBandwidth::RxTx))) ||
      !m_endpoint.SetInitialBandwidth(OpalBandwidth::Rx,
                                      args.GetOptionAs(m_endpoint.GetPrefixName() + "-rx-bandwidth",
                                                       m_endpoint.GetInitialBandwidth(OpalBandwidth::Rx))) ||
      !m_endpoint.SetInitialBandwidth(OpalBandwidth::Tx,
                                      args.GetOptionAs(m_endpoint.GetPrefixName() + "-tx-bandwidth",
                                                       m_endpoint.GetInitialBandwidth(OpalBandwidth::Tx)))) {
    output << "Invalid bandwidth for " << m_endpoint.GetPrefixName() << endl;
    return false;
  }


  if (!SetUIMode(args.GetOptionString(m_endpoint.GetPrefixName()+"-ui"))) {
    output << "Unknown user indication mode for " << m_endpoint.GetPrefixName() << endl;
    return false;
  }

  if (verbose)
    output << m_endpoint.GetPrefixName() << "user input mode: " << m_endpoint.GetSendUserInputMode() << '\n';


  m_endpoint.SetDefaultStringOptions(args.GetOptionString(m_endpoint.GetPrefixName() + "-option"));

  PStringArray interfaces = args.GetOptionString(m_endpoint.GetPrefixName()).Lines();
  if ((m_endpoint.GetListeners().IsEmpty() || !interfaces.IsEmpty()) && !m_endpoint.StartListeners(interfaces)) {
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
  if (args.GetCount() > 0 && !m_endpoint.StartListeners(args.GetParameters())) {
    args.WriteError("Could not start listening on specified interfaces.");
    return;
  }

  args.GetContext() << "Listening on: " << setfill(',') << m_endpoint.GetListeners() << setfill(' ') << endl;
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
    args.GetContext() << "Bandwidth:"
                         " rx=" << m_endpoint.GetInitialBandwidth(OpalBandwidth::Rx) <<
                         " tx=" << m_endpoint.GetInitialBandwidth(OpalBandwidth::Tx) << endl;
  else {
    OpalBandwidth bandwidth(args[0]);
    bool ok = true;
    if (!args.HasOption("rx") && !args.HasOption("tx"))
      ok = m_endpoint.SetInitialBandwidth(OpalBandwidth::RxTx, bandwidth);
    else {
      if (args.HasOption("rx"))
        ok = m_endpoint.SetInitialBandwidth(OpalBandwidth::Rx, bandwidth);
      if (args.HasOption("tx"))
        ok = ok && m_endpoint.SetInitialBandwidth(OpalBandwidth::Tx, bandwidth); // Do not do second call if first failed
    }
    if (!ok)
      args.WriteError("Illegal bandwidth parameter");
  }
}


void OpalRTPConsoleEndPoint::CmdUserInputMode(PCLI::Arguments & args, P_INT_PTR)
{
  if (args.GetCount() < 1)
    args.WriteUsage();
  else if (!SetUIMode(args[0]))
    args.WriteError("Unknown user indication mode");
}


void OpalRTPConsoleEndPoint::CmdStringOption(PCLI::Arguments & args, P_INT_PTR)
{
  if (args.HasOption('l')) {
    args.GetContext() << "Options available for " << m_endpoint.GetPrefixName() << ":\n"
                      << setfill('\n') << m_endpoint.GetAvailableStringOptions() << setfill(' ')
                      << endl;
    return;
  }

  if (args.HasOption('c'))
    m_endpoint.SetDefaultStringOptions(OpalConnection::StringOptions());

  if (args.GetCount() > 0)
    m_endpoint.SetDefaultStringOption(args[0], args.GetParameters(1).ToString());

  args.GetContext() << "Options for " << m_endpoint.GetPrefixName()<< ":\n"
                    << m_endpoint.GetDefaultStringOptions()
                    << endl;
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
  cli.SetCommand(m_endpoint.GetPrefixName() & "ui", PCREATE_NOTIFIER(CmdUserInputMode),
                  "Set user input mode",
                  "\"inband\" | \"rfc2833\" | \"signal\" | \"string\"");
  cli.SetCommand(m_endpoint.GetPrefixName() & "option", PCREATE_NOTIFIER(CmdStringOption),
                 "Set default string option",
                 "[ -c ] [ <key> [ <value> ] ]\n-l",
                 "c-clear. Clear all string options before adding\n"
                 "l-list.  List all available string options");
}
#endif //P_CLI
#endif // OPAL_SIP || OPAL_H323


/////////////////////////////////////////////////////////////////////////////

#if OPAL_H323
H323ConsoleEndPoint::H323ConsoleEndPoint(OpalManagerConsole & manager)
  : H323EndPoint(manager)
  , P_DISABLE_MSVC_WARNINGS(4355, OpalRTPConsoleEndPoint(manager, this))
{
}


void H323ConsoleEndPoint::OnGatekeeperStatus(H323Gatekeeper & gk, H323Gatekeeper::RegistrationFailReasons status)
{
  m_console.Broadcast(PSTRSTRM("\nH.323 registration: " << gk << " - " << status));
}


void H323ConsoleEndPoint::GetArgumentSpec(ostream & strm) const
{
  strm << "[H.323 options:]"
          "-no-h323.           Disable H.323\n"
          "H-h323:             Listens on interface(s), defaults to tcp$*:1720.\n";
  OpalRTPConsoleEndPoint::GetArgumentSpec(strm);
  strm << "g-gk-host:          Gatekeeper host.\n"
          "G-gk-id:            Gatekeeper identifier.\n"
          "-gk-password:       Gatekeeper password (if different from --password).\n"
          "-gk-alias-limit:    Gatekeeper alias limit (compatibility issue)\n"
          "-gk-sim-pattern.    Gatekeeper alias patern simulation\n"
          "-gk-suppress-grq.   Gatekeeper GRQ is not sent on registration.\n"
          "-gk-interface:      Gatekeeper network interface to use for RAS.\n"
          "-alias:             Alias name, may be multiple entries.\n"
          "-alias-pattern:     Alias pattern, may be multiple entries.\n"
          "-no-fast.           Fast connect disabled.\n"
          "-no-tunnel.         H.245 tunnel disabled.\n"
          "-no-h245-setup.     H.245 tunnel during SETUP disabled.\n"
          "-h239-control.      H.239 control capability.\n"
          "-h323-term-type:    Terminal type value (1..255, default 50).\n";
}


bool H323ConsoleEndPoint::UseGatekeeperFromArgs(const PArgList & args, const char * host, const char * ident, const char * pass, const char * inter)
{
  SetGatekeeperPassword(args.GetOptionString(pass, args.GetOptionString("password")));
  return UseGatekeeper(args.GetOptionString(host), args.GetOptionString(ident), args.GetOptionString(inter));
}


bool H323ConsoleEndPoint::Initialise(PArgList & args, bool verbose, const PString & defaultRoute)
{
  OpalManagerConsole::LockedStream lockedOutput(m_console);
  ostream & output = lockedOutput;

  // Set up H.323
  if (args.HasOption("no-h323")) {
    if (verbose)
      output << "H.323 protocol disabled.\n";
    return true;
  }

  if (!OpalRTPConsoleEndPoint::Initialise(args, output, verbose))
    return false;

  if (args.HasOption("no-fast"))
    DisableFastStart(true);
  if (args.HasOption("no-tunnel"))
    DisableH245Tunneling(true);
  if (args.HasOption("no-h245-setup"))
    DisableH245inSetup(true);

  if (args.HasOption("h323-term-type")) {
    unsigned newType = args.GetOptionAs<unsigned>("h323-term-type");
    if (newType < 1 || newType > 255) {
      output << "Invalid H.323 terminal type value." << endl;
      return false;
    }
    SetTerminalType((TerminalTypes)newType);
    if (verbose)
      output << "H.323 terminal type: " << GetTerminalType() << '\n';
  }

  AddAliasNames(args.GetOptionString("alias").Lines());
  AddAliasNamePatterns(args.GetOptionString("alias-pattern").Lines());

  if (args.HasOption("gk-sim-pattern"))
    SetGatekeeperSimulatePattern(true);

  if (args.HasOption("gk-suppress-grq"))
    SetSendGRQ(false);

#if OPAL_H239
  if (args.HasOption("h239-control"))
    SetDefaultH239Control(true);
#endif

  if (verbose)
    output << "H.323 Aliases: " << setfill(',') << GetAliasNames() << setfill(' ') << "\n"
              "H.323 Alias Patterns: " << (GetGatekeeperSimulatePattern() ? "(simulated)" : "")
                                       << setfill(',') << GetAliasNamePatterns() << setfill(' ') << "\n"
              "H.323 options: "
           << (IsFastStartDisabled() ? "Slow" : "Fast") << " connect, "
           << (IsH245TunnelingDisabled() ? "Separate" : "Tunnelled") << " H.245\n";


  SetGatekeeperAliasLimit(args.GetOptionAs<PINDEX>("gk-alias-limit", GetGatekeeperAliasLimit()));

  if (args.HasOption("gk-host") || args.HasOption("gk-id")) {
    if (!UseGatekeeperFromArgs(args, "gk-host", "gk-id", "gk-password", "gk-interface")) {
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
void H323ConsoleEndPoint::CmdTerminalType(PCLI::Arguments & args, P_INT_PTR)
{
  if (args.GetCount() > 0) {
    unsigned newType = args[0].AsUnsigned();
    if (newType < 1 || newType > 255) {
      args.WriteError("Invalid H.323 terminal type value.");
      return;
    }
    SetTerminalType((TerminalTypes)newType);
  }
  args.GetContext() << "H.323 Terminal Type: " << GetTerminalType() << endl;
}


void H323ConsoleEndPoint::CmdAlias(PCLI::Arguments & args, P_INT_PTR)
{
  if (args.GetCount() == 0) {
    args.WriteUsage();
    return;
  }

  int operation = args.HasOption('d') ? 1 : 0;
  if (args.HasOption('p'))
    operation |= 2;

  if (args.HasOption('r')) {
    switch (operation) {
      case 0: SetAliasNames(args.GetParameters()); break;
      case 2: SetAliasNamePatterns(args.GetParameters()); break;
      default :
        args.WriteUsage();
        return;
    }
  }
  else {
    switch (operation) {
      case 0: AddAliasNames(args.GetParameters()); break;
      case 1: RemoveAliasNames(args.GetParameters()); break;
      case 2: AddAliasNamePatterns(args.GetParameters()); break;
      case 3: RemoveAliasNamePatterns(args.GetParameters()); break;
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
    if (args.HasOption("suppress-grq"))
      SetSendGRQ(false);
    if (UseGatekeeperFromArgs(args, "host", "identifier", "password", "interface"))
      args.GetContext()<< *GetGatekeeper() << endl;
    else
      args.GetContext() << "unavailable" << endl;
  }
  else
    args.WriteUsage();
}


void H323ConsoleEndPoint::CmdCompatibility(PCLI::Arguments & args, P_INT_PTR)
{
  H323Connection::CompatibilityIssues issue;

  if (args.GetCount() == 0) {
    size_t width = 0;
    for (issue = H323Connection::BeginCompatibilityIssues; issue < H323Connection::EndCompatibilityIssues; ++issue) {
      size_t len = strlen(H323Connection::CompatibilityIssuesToString(issue));
      if (len > width)
        width = len;
    }
    for (issue = H323Connection::BeginCompatibilityIssues; issue < H323Connection::EndCompatibilityIssues; ++issue)
      args.GetContext() << left << setw(width) << issue << " : " << GetCompatibility(issue) << endl;
    return;
  }

  if ((issue = H323Connection::CompatibilityIssuesFromString(args[0], false)) == H323Connection::EndCompatibilityIssues) {
    args.WriteError("Unknown or ambiguous compatibility issue");
    return;
  }

  if (args.GetCount() > 1)
    SetCompatibility(issue, args.GetParameters(1).ToString());

  args.GetContext() << issue << " = " << GetCompatibility(issue) << endl;
}


void H323ConsoleEndPoint::AddCommands(PCLI & cli)
{
  OpalRTPConsoleEndPoint::AddCommands(cli);

  cli.SetCommand("h323 fast-connect-disable", disableFastStart, "Fast Connect Disable");
  cli.SetCommand("h323 tunnel-h245-disable", disableH245Tunneling, "H.245 Tunnelling Disable");
  cli.SetCommand("h323 h245-in-setup-disable", disableH245inSetup, "H.245 in SETUP Disable");
#if OPAL_H239
  cli.SetCommand("h323 h239-control", m_defaultH239Control, "H.239 control capability enable");
#endif
  cli.SetCommand("h323 term-type", PCREATE_NOTIFIER(CmdTerminalType), "Terminal type value (1..255, default 50)");
  cli.SetCommand("h323 compatibility", PCREATE_NOTIFIER(CmdCompatibility),
                 "Set remote system identification extended regular expression for compatibility issues.",
                 "[ <issue> [ <regex> ]]");

  cli.SetCommand("h323 alias", PCREATE_NOTIFIER(CmdGatekeeper),
                 "Set alias name(s)",
                 "[ <options> ] [ <name> ... ]",
                 "r-reset:  Reset the alias list before starting\n"
                 "p-pattern: Aliases are patterns (e.g. \"1100*\" or \"1100-1199\")\n"
                 "d-delete: Delete the specified alias");
  cli.SetCommand("h323 gatekeeper\nh323 gk", PCREATE_NOTIFIER(CmdGatekeeper),
                  "Set gatekeeper",
                  "[ <options> ... ] [ \"on\" / \"off\" ]",
                  "h-host: Host name or IP address of gatekeeper\n"
                  "i-identifier: Identifier for gatekeeper\n"
                  "I-interface: Network interface for RAS channel.\n"
                  "p-password: Password for H.235.1 authentication\n"
                  "l-limit: Alias limit for gatekeeper\n"
                  "g-suppress-grq: Do not send GRQ in registration");
}
#endif // P_CLI
#endif // OPAL_H323


/////////////////////////////////////////////////////////////////////////////

#if OPAL_SIP
SIPConsoleEndPoint::SIPConsoleEndPoint(OpalManagerConsole & manager)
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

  m_console.Broadcast(PSTRSTRM('\n' << status));
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

  PCaselessString str = args.GetOptionString(mode);
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
  else if (!str.IsEmpty()) {
    output << "Unknown SIP registration mode \"" << str << '"' << endl;
    return false;
  }

  params.m_expire = args.GetOptionAs(ttl, 300);
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
          "S-sip:             Listen on interface(s), defaults to *:5060 (udp/tcp/tls, all interfaces).\n";
  OpalRTPConsoleEndPoint::GetArgumentSpec(strm);
  strm << "r-register:        Registration to server.\n"
          "-register-auth-id: Registration authorisation id, default is username.\n"
          "-register-realm:   Registration authorisation realm, default is any.\n"
          "-register-proxy:   Registration proxy, default is none.\n"
          "-register-ttl:     Registration Time To Live, default 300 seconds.\n"
          "-register-mode:    Registration mode (normal, single, public, ALG, RFC5626).\n"
          "-proxy:            Outbound proxy.\n";
}


bool SIPConsoleEndPoint::Initialise(PArgList & args, bool verbose, const PString & defaultRoute)
{
  OpalManagerConsole::LockedStream lockedOutput(m_console);
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
                  "[ <options> ... ] <uri>",
                  "-u-user: Username for proxy\n"
                  "-p-password: Password for proxy");
  cli.SetCommand("sip register", PCREATE_NOTIFIER(CmdRegister),
                  "Register with SIP registrar",
                  "[ <options> ... ] <address> [ <password> ]",
                  "a-auth-id: Override user for authorisation\n"
                  "r-realm: Set realm for authorisation\n"
                  "p-proxy: Set proxy for registration\n"
                  "m-mode: Set registration mode (normal, single, public)\n"
                  "t-ttl: Set Time To Live for registration\n");
}
#endif // P_CLI
#endif // OPAL_SIP


/////////////////////////////////////////////////////////////////////////////

#if OPAL_SDP_HTTP

OpalSDPHTTPConsoleEndPoint::OpalSDPHTTPConsoleEndPoint(OpalManagerConsole & manager)
  : OpalSDPHTTPEndPoint(manager)
  , P_DISABLE_MSVC_WARNINGS(4355, OpalRTPConsoleEndPoint(manager, this))
{
}


void OpalSDPHTTPConsoleEndPoint::GetArgumentSpec(ostream & strm) const
{
  strm << "[SDP over HTTP options:]"
          "-no-sdp. Disable SDP over HTTP\n"
          "-sdp:    Listens on interface(s), defaults to tcp$*:8080.\n";
  OpalRTPConsoleEndPoint::GetArgumentSpec(strm);
}


bool OpalSDPHTTPConsoleEndPoint::Initialise(PArgList & args, bool verbose, const PString &)
{
  OpalManagerConsole::LockedStream lockedOutput(m_console);
  ostream & output = lockedOutput;

  if (args.HasOption("no-sdp")) {
    if (verbose)
      output << "SDP over HTTP protocol disabled.\n";
    return true;
  }

  return OpalRTPConsoleEndPoint::Initialise(args, output, verbose);
}
#endif // OPAL_SDP_HTTP


/////////////////////////////////////////////////////////////////////////////

#if OPAL_SKINNY
OpalConsoleSkinnyEndPoint::OpalConsoleSkinnyEndPoint(OpalManagerConsole & manager)
: OpalSkinnyEndPoint(manager)
, OpalConsoleEndPoint(manager)
{
}


void OpalConsoleSkinnyEndPoint::GetArgumentSpec(ostream & strm) const
{
  strm << "[SCCP options:]"
          "-no-sccp.        Disable Skinny Client Control Protocol\n"
          "-sccp-server:    Set Skinny server address.\n"
          "-sccp-name:      Set device name for Skinny client, may be present multiple times.\n"
          "-sccp-device:    Set device type code for Skinny clients.\n";
}


bool OpalConsoleSkinnyEndPoint::Initialise(PArgList & args, bool verbose, const PString & defaultRoute)
{
  OpalManagerConsole::LockedStream lockedOutput(m_console);
  ostream & output = lockedOutput;

  // If we have LIDs speficied in command line, load them
  if (args.HasOption("no-sccp")) {
    if (verbose)
      output << "Skinny disabled.\n";
    return true;
  }

  bool none = true;

  unsigned deviceType = args.GetOptionAs<unsigned>("sccp-device", OpalSkinnyEndPoint::DefaultDeviceType);
  PString server = args.GetOptionString("sccp-server");
  if (!server.IsEmpty()) {
    PStringArray names = args.GetOptionString("sccp-name").Lines();
    for (PINDEX i = 0; i < names.GetSize(); ++i) {
      PString name = names[i];
      if (!Register(server, name, deviceType))
        output << "Could not register " << name << " with skinny server \"" << server << '"' << endl;
      else {
        if (verbose)
          output << "Skinny client: " << name << '@' << server << '\n';
        none = false;
      }
    }
  }

  if (none && verbose)
    output << "SCCP has no phone devices registered" << endl;

  AddRoutesFor(this, defaultRoute);
  return true;
}


#if P_CLI
void OpalConsoleSkinnyEndPoint::CmdRegister(PCLI::Arguments & args, P_INT_PTR)
{
  if (args.GetCount() < 2)
    args.WriteUsage();
  else if (!Register(args[0], args[1]))
    args.WriteError() << "Could not register \"" << args[1] << "\" with skinny server \"" << args[0] << '"' << endl;
}


void OpalConsoleSkinnyEndPoint::CmdStatus(PCLI::Arguments & args, P_INT_PTR)
{
  ostream & out = args.GetContext();

  bool none = true;

  PStringArray names = GetPhoneDeviceNames();
  for (PINDEX i = 0; i < names.GetSize(); ++i) {
    OpalSkinnyEndPoint::PhoneDevice * phoneDevice = GetPhoneDevice(names[i]);
    if (phoneDevice != NULL) {
      out << *phoneDevice << endl;
      none = false;
    }
  }

  if (none)
    out << "SCCP has no phone devices registered" << endl;
}


void OpalConsoleSkinnyEndPoint::AddCommands(PCLI & cli)
{
  cli.SetCommand("sccp register", PCREATE_NOTIFIER(CmdRegister), "Set skinny server", "[ <host> <name> ]");
  cli.SetCommand("sccp status", PCREATE_NOTIFIER(CmdStatus), "Display status of registered Skinny phone devices");
}
#endif // P_CLI
#endif // OPAL_SKINNY


/////////////////////////////////////////////////////////////////////////////

#if OPAL_LYNC
OpalConsoleLyncEndPoint::OpalConsoleLyncEndPoint(OpalManagerConsole & manager)
: OpalLyncEndPoint(manager)
, OpalConsoleEndPoint(manager)
{
}


void OpalConsoleLyncEndPoint::GetArgumentSpec(ostream & strm) const
{
  strm << "[Lync options:]"
          "-no-lync.        Disable Lync (UCMA) protocol\n"
          "-lync-uri:       Lync URI to register\n"
          "-lync-password:  Lync password for corresponding URI\n"
          "-lync-auth-id:   Lync authorisation identifier for corresponding URI\n"
          "-lync-domain:    Lync authentication domain for corresponding URI\n";
}


bool OpalConsoleLyncEndPoint::Initialise(PArgList & args, bool verbose, const PString & defaultRoute)
{
  OpalManagerConsole::LockedStream lockedOutput(m_console);
  ostream & output = lockedOutput;

  // If we have LIDs speficied in command line, load them
  if (args.HasOption("no-lync")) {
    if (verbose)
      output << "Lync disabled.\n";
    return true;
  }

  bool none = true;
  PStringArray uri = args.GetOptionString("lync-uri").Lines();
  PStringArray password = args.GetOptionString("lync-password").Lines();
  PStringArray authID = args.GetOptionString("lync-auth-id").Lines();
  PStringArray domain = args.GetOptionString("lync-domain").Lines();
  for (PINDEX i = 0; i < uri.GetSize(); ++i) {
    UserParams info;
    info.m_uri = uri[i];

    if (!password.IsEmpty())
      info.m_password = password[std::min(i, password.GetSize()-1)];

    if (!authID.IsEmpty())
      info.m_authID = authID[std::min(i, authID.GetSize()-1)];

    if (!domain.IsEmpty())
      info.m_domain = domain[std::min(i, domain.GetSize()-1)];

    PString registeredURI = RegisterUser(info);
    if (registeredURI.IsEmpty())
      output << "Could not register " << info.m_uri << " with Lync server" << endl;
    else {
      if (verbose)
        output << "Lync registered: " << registeredURI << '\n';
    }
  }

  if (none && verbose)
    output << "Lync has no users registered" << endl;

  AddRoutesFor(this, defaultRoute);
  return true;
}


#if P_CLI
void OpalConsoleLyncEndPoint::CmdRegister(PCLI::Arguments & args, P_INT_PTR)
{
  if (args.GetCount() < 1)
    args.WriteUsage();
  else {
    UserParams info;
    info.m_uri = args[0];
    info.m_password = args[1];
    info.m_authID = args.GetOptionString("auth-id");
    info.m_domain = args.GetOptionString("domain");
    PString uri = RegisterUser(info);
    if (uri.IsEmpty())
      args.WriteError() << "Could not register \"" << info.m_uri << "\" with Lync server" << endl;
    else
      args.GetContext() << "Registered " << uri << " with Lync server." << endl;
  }
}


void OpalConsoleLyncEndPoint::AddCommands(PCLI & cli)
{
  cli.SetCommand("lync register", PCREATE_NOTIFIER(CmdRegister),
                  "Register Lync URI",
                  "[ <options> ... ] <address> [ <password> ]",
                  "a-auth-id: Override user for authorisation\n"
                  "d-domain: Set domain for authorisation\n");
}
#endif // P_CLI
#endif // OPAL_LYNC


/////////////////////////////////////////////////////////////////////////////

#if OPAL_LID
OpalConsoleLineEndPoint::OpalConsoleLineEndPoint(OpalManagerConsole & manager)
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
  OpalManagerConsole::LockedStream lockedOutput(m_console);
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
    args.WriteUsage();
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
OpalConsoleCapiEndPoint::OpalConsoleCapiEndPoint(OpalManagerConsole & manager)
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
  OpalManagerConsole::LockedStream lockedOutput(m_console);
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

static void OutputSoundDeviceError(ostream & output,
                                   PSoundChannel::Directions dir,
                                   const PString & device,
                                   const PString & driver)
{
  PStringArray names;
  if (driver.IsEmpty()) {
    names = PSoundChannel::GetDeviceNames(dir);
    output << " device name \"" << device << '"';
  }
  else if ((names = PSoundChannel::GetDriversDeviceNames(driver, dir)).IsEmpty()) {
    names = PSoundChannel::GetDriverNames();
    output << " driver \"" << driver << "\" invalid, select one of:";
  }
  else
    output << " device name \"" << device << "\" with driver \"" << driver << "\" invalid, select one of:";

  output << " invalid, select one of:";
  for (PINDEX i = 0; i < names.GetSize(); ++i)
    output << "\n   " << names[i];
  output << endl;
}


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

    PString device = fromCLI ? args.GetParameters().ToString() : args.GetOptionString(prefix + "device");
    if (device.IsEmpty() && !driver.IsEmpty())
      device = '*';

    if ((!driver.IsEmpty() || !device.IsEmpty()) && !(ep.*m_set)(driver + device)) {
      output << "Audio " << m_description;
      OutputSoundDeviceError(output, m_dir, device, driver);
      return false;
    }

    if (verbose)
      output << "Audio " << m_description << ": " << (ep.*m_get)() << endl;

    return true;
  }
} AudioDeviceVariables[] = {
  { PSoundChannel::Recorder, "record-audio",    "recorder (transmit)", &OpalPCSSEndPoint::GetSoundChannelRecordDevice, &OpalPCSSEndPoint::SetSoundChannelRecordDevice },
  { PSoundChannel::Player,   "play-audio",      "player (receive)",    &OpalPCSSEndPoint::GetSoundChannelPlayDevice,   &OpalPCSSEndPoint::SetSoundChannelPlayDevice   },
  { PSoundChannel::Recorder, "hold-audio",      "on hold",             &OpalPCSSEndPoint::GetSoundChannelOnHoldDevice, &OpalPCSSEndPoint::SetSoundChannelOnHoldDevice },
  { PSoundChannel::Recorder, "ring-audio",      "on ring",             &OpalPCSSEndPoint::GetSoundChannelOnRingDevice, &OpalPCSSEndPoint::SetSoundChannelOnRingDevice }
};

#if OPAL_VIDEO
static struct {
  const char * m_name;
  const char * m_description;
  const PVideoDevice::OpenArgs & (OpalConsolePCSSEndPoint:: *m_get)() const;
  PBoolean(OpalConsolePCSSEndPoint:: *m_set)(const PVideoDevice::OpenArgs &);
  PStringArray (*m_list)(const PString &, PPluginManager *);

  bool Initialise(OpalConsolePCSSEndPoint & ep, ostream & output, bool verbose, const PArgList & args, bool fromCLI)
  {
    PVideoDevice::OpenArgs video = (ep.*m_get)();

    PString prefix;
    if (fromCLI)
      video.deviceName = args.GetParameters().ToString();
    else {
      prefix += m_name;
      prefix += '-';
      prefix.Replace(' ', '-', true);
      video.deviceName = args.GetOptionString(prefix + "device");
    }

    video.driverName = args.GetOptionString(prefix+"driver");
    video.channelNumber = args.GetOptionAs(prefix+"channel", video.channelNumber);

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
#define VID_DEV_VAR(cmd,hlp,get,set) { cmd, hlp, &OpalConsolePCSSEndPoint::get, &OpalConsolePCSSEndPoint::set, &PVideoInputDevice::GetDriversDeviceNames }
  VID_DEV_VAR("grabber",               "input grabber",                         GetVideoGrabberDevice,        SetVideoGrabberDevice),
  VID_DEV_VAR("preview",               "input preview",                         GetVideoPreviewDevice,        SetVideoPreviewDevice),
  VID_DEV_VAR("display",               "output display",                        GetVideoDisplayDevice,        SetVideoDisplayDevice),
  VID_DEV_VAR("hold-video",            "input grabber on hold",                 GetVideoOnHoldDevice,         SetVideoOnHoldDevice),
  VID_DEV_VAR("ring-video",            "input grabber on ring",                 GetVideoOnRingDevice,         SetVideoOnRingDevice),
  VID_DEV_VAR("presentation grabber",  "input grabber for presentation role",   GetPresentationVideoDevice,   SetPresentationVideoDevice),
  VID_DEV_VAR("presentation preview",  "input preview for presentation role",   GetPresentationPreviewDevice, SetPresentationPreviewDevice),
  VID_DEV_VAR("presentation display",  "output display for presentation role",  GetPresentationOutputDevice,  SetPresentationOutputDevice),
  VID_DEV_VAR("speaker grabber",       "input grabber for speaker role",        GetSpeakerVideoDevice,        SetSpeakerVideoDevice),
  VID_DEV_VAR("speaker preview",       "input preview for speaker role",        GetSpeakerPreviewDevice,      SetSpeakerPreviewDevice),
  VID_DEV_VAR("speaker display",       "output display for speaker role",       GetSpeakerOutputDevice,       SetSpeakerOutputDevice),
  VID_DEV_VAR("sign-language grabber", "input grabber for sign langauge role",  GetSignVideoDevice,           SetSignVideoDevice),
  VID_DEV_VAR("sign-language preview", "input preview for sign langauge role",  GetSignPreviewDevice,         SetSignPreviewDevice),
  VID_DEV_VAR("sign-language display", "output display for sign-language role", GetSignOutputDevice,          SetSignOutputDevice)
};
#endif // OPAL_VIDEO


OpalConsolePCSSEndPoint::OpalConsolePCSSEndPoint(OpalManagerConsole & manager)
  : OpalPCSSEndPoint(manager)
  , OpalConsoleEndPoint(manager)
  , m_ringChannelParams(PSoundChannel::Player, PSoundChannel::GetDefaultDevice(PSoundChannel::Player))
  , m_ringThread(NULL)
  , m_ringState(e_RingIdle)
{
}


void OpalConsolePCSSEndPoint::GetArgumentSpec(ostream & strm) const
{
  strm << "[PC options:]"
          "-ring-file:   WAV file to play on incoming call\n"
          "-ring-device: Audio device to play the ring-file\n"
          "-ring-driver: Audio driver to play the ring-file\n"
          "-ringback-tone: Set ringback tone (WAV file, Country or tone specification)\n";
  for (PINDEX i = 0; i < PARRAYSIZE(AudioDeviceVariables); ++i) {
    const char * name = AudioDeviceVariables[i].m_name;
    const char * desc = AudioDeviceVariables[i].m_description;
    strm << '-' << name << "-driver: Audio " << desc << " driver.\n"
            "-" << name << "-device: Audio " << desc << " device.\n";
  }
  strm << "-audio-buffer:   Audio buffer time in ms (default 120)\n";

#if OPAL_VIDEO
  for (PINDEX i = 0; i < PARRAYSIZE(VideoDeviceVariables); ++i) {
    PString name = VideoDeviceVariables[i].m_name;
    name.Replace(' ', '-', true);
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
  OpalManagerConsole::LockedStream lockedOutput(m_console);
  ostream & output = lockedOutput;

  for (PINDEX i = 0; i < PARRAYSIZE(AudioDeviceVariables); ++i) {
    if (!AudioDeviceVariables[i].Initialise(*this, output, verbose, args, false))
      return false;
  }

  if (args.HasOption("audio-buffer"))
    SetSoundChannelBufferTime(args.GetOptionString("audio-buffer").AsUnsigned());
  if (verbose)
    output << "Audio buffer time: " << GetSoundChannelBufferTime() << "ms\n";

  if (args.HasOption("ring-file"))
    SetRingInfo(output, verbose,
                args.GetOptionString("ring-file"),
                args.GetOptionString("ring-device", m_ringChannelParams.m_device),
                args.GetOptionString("ring-driver", m_ringChannelParams.m_driver));

  if (args.HasOption("ringback-tone") && !SetLocalRingbackTone(args.GetOptionString("ringback-tone"))) {
    output << "Invalid ringback tone specification." << endl;
    return false;
  }

#if OPAL_VIDEO
  for (PINDEX i = 0; i < PARRAYSIZE(VideoDeviceVariables); ++i) {
    if (!VideoDeviceVariables[i].Initialise(*this, output, verbose, args, false))
      return false;
  }
#endif // OPAL_VIDEO

  return true;
}


#if P_CLI
void OpalConsolePCSSEndPoint::CmdRingFileAndDevice(PCLI::Arguments & args, P_INT_PTR)
{
  SetRingInfo(args.GetContext(), true,
              args.GetCount() < 1 ? m_ringFileName : args[0],
              args.GetOptionString('d', m_ringChannelParams.m_device),
              args.GetOptionString('D', m_ringChannelParams.m_driver));
}


void OpalConsolePCSSEndPoint::CmdRingbackTone(PCLI::Arguments & args, P_INT_PTR)
{
  if (args.GetCount() > 0 && !SetLocalRingbackTone(args[0]))
    args.WriteError("Invalid ringback tone");
  else
    args.GetContext() << "Ringback tone: " << GetLocalRingbackTone() << endl;
}


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


void OpalConsolePCSSEndPoint::CmdDefaultAudioDevice(PCLI::Arguments & args, P_INT_PTR)
{
  for (PINDEX i = 0; i < PARRAYSIZE(AudioDeviceVariables); ++i) {
    if (args.GetCommandName().Find(AudioDeviceVariables[i].m_name) != P_MAX_INDEX)
      AudioDeviceVariables[i].Initialise(*this, args.GetContext(), true, args, true);
  }
}


void OpalConsolePCSSEndPoint::CmdChangeAudioDevice(PCLI::Arguments & args, P_INT_PTR)
{
  PSafePtr<OpalPCSSConnection> connection;
  if (m_console.GetConnectionFromArgs(args, connection)) {
    if (connection->TransferConnection(args[0]))
      args.GetContext() << "Switched audio device" << endl;
    else
      args.WriteError("Could not switch audio device");
  }
}


void OpalConsolePCSSEndPoint::CmdAudioBuffers(PCLI::Arguments & args, P_INT_PTR)
{
  if (args.GetCount() > 0)
    SetSoundChannelBufferTime(args[0].AsUnsigned());
  args.GetContext() << "Audio buffer time: " << GetSoundChannelBufferTime() << "ms" << endl;
}


#if OPAL_VIDEO
void OpalConsolePCSSEndPoint::CmdDefaultVideoDevice(PCLI::Arguments & args, P_INT_PTR)
{
  for (PINDEX i = 0; i < PARRAYSIZE(VideoDeviceVariables); ++i) {
    if (args.GetCommandName().NumCompare(GetPrefixName() & VideoDeviceVariables[i].m_name) == EqualTo)
      VideoDeviceVariables[i].Initialise(*this, args.GetContext(), true, args, true);
  }
}


void OpalConsolePCSSEndPoint::CmdChangeVideoDevice(PCLI::Arguments & args, P_INT_PTR)
{
  PSafePtr<OpalPCSSConnection> connection;
  if (m_console.GetConnectionFromArgs(args, connection)) {
    PVideoDevice::OpenArgs video = GetVideoGrabberDevice();
    video.deviceName = args[0];
    if (connection->ChangeVideoInputDevice(video))
      args.GetContext() << "Switched video device" << endl;
    else
      args.WriteError("Could not switch video device");
  }
}


void OpalConsolePCSSEndPoint::CmdOpenVideoStream(PCLI::Arguments & args, P_INT_PTR)
{
  PSafePtr<OpalPCSSConnection> connection;
  if (m_console.GetConnectionFromArgs(args, connection)) {
    OpalVideoFormat::ContentRole contentRole;
    if (args.GetCount() == 0)
      contentRole = connection->GetMediaStream(OpalMediaType::Video(), false) != NULL
                        ? OpalVideoFormat::ePresentation : OpalVideoFormat::eMainRole;
    else if ((contentRole = OpalVideoFormat::ContentRoleFromString('e' + args[0], false)) == OpalVideoFormat::EndContentRole) {
      args.WriteUsage();
      return;
    }

    OpalMediaFormat mediaFormat;
    if (args.HasOption("codec")) {
      mediaFormat = args.GetOptionString("codec");
      if (!mediaFormat.IsValid()) {
        args.WriteError() << "Unknown media format \"" << args.GetOptionString("codec") << '"' << endl;
        return;
      }
      if (!GetVideoFormatFromArgs(args, mediaFormat, false))
        return;
    }

    if (connection->GetCall().OpenSourceMediaStreams(*connection,
                                                     OpalMediaType::Video(),
                                                     0, // Allocate session automatically
                                                     mediaFormat,
                                                     contentRole))
      args.GetContext() << "Switched video device" << endl;
    else
      args.WriteError("Could not open video to remote");
  }
}


static OpalMediaStreamPtr FindStreamForRole(OpalRTPConnection & connection, OpalVideoFormat::ContentRole contentRole)
{
  OpalMediaStreamPtr stream;
  while ((stream = connection.GetMediaStream(OpalMediaType::Video(), false, stream)) != NULL) {
    if (stream->GetMediaFormat().GetOptionEnum(OpalVideoFormat::ContentRoleOption(), OpalVideoFormat::eNoRole) == contentRole)
      break;
  }
  return stream;
}

void OpalConsolePCSSEndPoint::CmdCloseVideoStream(PCLI::Arguments & args, P_INT_PTR)
{
  PSafePtr<OpalRTPConnection> connection;
  if (m_console.GetConnectionFromArgs(args, connection)) {
    OpalMediaStreamPtr stream;
    if (args.GetCount() != 0) {
      OpalVideoFormat::ContentRole contentRole = OpalVideoFormat::ContentRoleFromString('e' + args[0], false);
      if (contentRole == OpalVideoFormat::EndContentRole) {
        args.WriteUsage();
        return;
      }

      if ((stream = FindStreamForRole(*connection, contentRole)) == NULL) {
        args.WriteError("No video with that role.");
        return;
      }
    }
    else {
      if ((stream = FindStreamForRole(*connection, OpalVideoFormat::ePresentation)) == NULL) {
        if ((stream = connection->GetMediaStream(OpalMediaType::Video(), false, stream)) == NULL) {
          args.WriteError("No video streams open.");
          return;
        }
      }
    }
    if (stream->Close())
      args.GetContext() << "Closing video." << endl;
  }
}
#endif // OPAL_VIDEO

#if OPAL_HAS_H281
struct OpalCmdFarEndCameraControlMode
{
  P_DECLARE_STREAMABLE_ENUM(Cmd, external, device);
};

void OpalConsolePCSSEndPoint::CmdExternalCameraControl(PCLI::Arguments & args, P_INT_PTR)
{
  if (args.GetCount() == 0) {
    args.GetContext() << "Far End Camera Control mode: "
                      << (GetFarEndCameraActionNotifier().IsNULL() ? OpalCmdFarEndCameraControlMode::device : OpalCmdFarEndCameraControlMode::external)
                      << endl;
    return;
  }

  switch (OpalCmdFarEndCameraControlMode::CmdFromString(args[0])) {
    case OpalCmdFarEndCameraControlMode::external :
      SetFarEndCameraActionNotifier(PCREATE_NOTIFIER(ExternalCameraControlNotification));
      break;

    case OpalCmdFarEndCameraControlMode::device :
      SetFarEndCameraActionNotifier(PNotifier());
      break;

    default :
      args.WriteUsage();
      return;
  }
}

void OpalConsolePCSSEndPoint::ExternalCameraControlNotification(OpalH281Client &, P_INT_PTR param)
{
  PStringStream str;
  if (param == 0)
    str << "FECC STOPPED";
  else {
    const int * directions = (const int *)param;
    str << "FECC START";
    for (PVideoControlInfo::Types type = PVideoControlInfo::BeginTypes; type < PVideoControlInfo::EndTypes; ++type) {
      if (directions[type] != 0)
        str << ' ' << type << '=' << directions[type];
    }
  }
  m_console.Broadcast(str);
}
#endif // OPAL_HAS_H281


void OpalConsolePCSSEndPoint::AddCommands(PCLI & cli)
{
  cli.SetCommand("pc ring", PCREATE_NOTIFIER(CmdRingFileAndDevice),
                 "Set ring file for incoming calls",
                 "[ <options> ... ] <file>",
                 "d-device: Set sound device name for playing file\n"
                 "D-driver: Set sound device driver for playing file\n");

  cli.SetCommand("pc ringback", PCREATE_NOTIFIER(CmdRingbackTone),
                 "Set local ringback tone for outgoing calls.", "<spec>");

  for (PINDEX i = 0; i < PARRAYSIZE(AudioDeviceVariables); ++i)
    cli.SetCommand(GetPrefixName() & AudioDeviceVariables[i].m_name, PCREATE_NOTIFIER(CmdDefaultAudioDevice),
                   PSTRSTRM("Audio " << AudioDeviceVariables[i].m_description << " device."),
                   "[ option ] <name>", "D-driver:  Optional driver name.");

  cli.SetCommand("pc buffers", m_soundChannelBufferTime, "Audio Buffer Time", 20, 1000, "Audio buffer time in ms");

  cli.SetCommand("pc microphone volume", PCREATE_NOTIFIER(CmdVolume),
                 "Set volume for microphone",
                 "[ <percent> ]", "c-call: Call token");
  cli.SetCommand("pc speaker volume", PCREATE_NOTIFIER(CmdVolume),
                 "Set volume for speaker",
                 "[ <percent> ]", "c-call: Call token");

  cli.SetCommand("audio device", PCREATE_NOTIFIER(CmdChangeAudioDevice),
                 "Set audio device for active call", "[ --call ] [ --rx | --tx ] <device>",
                 "c-call: Token for call to change\n"
                 "r-rx.   Receive audio device\n"
                 "t-tx.   Transmit audio device\n");

#if OPAL_VIDEO
  for (PINDEX i = 0; i < PARRAYSIZE(VideoDeviceVariables); ++i)
    cli.SetCommand(GetPrefixName() & VideoDeviceVariables[i].m_name, PCREATE_NOTIFIER(CmdDefaultVideoDevice),
                    PSTRSTRM("Video " << VideoDeviceVariables[i].m_description << " device."),
                    "[ <options> ... ] <name>",
                    "-driver:  Driver name.\n"
                    "-format:  Format (\"pal\"/\"ntsc\")\n"
                    "-channel: Channel number.\n");

  cli.SetCommand("video device", PCREATE_NOTIFIER(CmdChangeVideoDevice),
                 "Set video device for active call", "[ --call ] <device>",
                 "c-call: Token for call to change");
  cli.SetCommand("video open", PCREATE_NOTIFIER(CmdOpenVideoStream),
                 "Open video stream for active call with a given role. Default is \"main\" if no\n"
                 "video is open, and \"presentation\" if there is a video stream already.\n"
                 "The transmit options only apply if --codec is used.",
                 "[ <options> ... ] [ main | presentation | speaker | sign ]",
                 "c-call:       Token for call to change\n"
                 "C-codec:      Use specified media format for transmit.\n"
                 "s-size:       Transmit resolution\n"
                 "f-frame-rate: Transmit frame rate (fps)\n"
                 "b-bit-rate:   Transmit target bit rate (kbps)\n"
                 "t-tsto:       Transmit temporal/spatial trade off (1=quality 31=speed)\n");
  cli.SetCommand("video close", PCREATE_NOTIFIER(CmdCloseVideoStream),
                 "Close video stream for active call with a given role. Default is \"presentation\" if\n"
                 "one is open, and \"main\" if there is that is the only video stream open.\n",
                 "[ <options> ... ] [ main | presentation | speaker | sign ]",
                 "c-call:       Token for call to change\n");
#endif // OPAL_VIDEO

#if OPAL_HAS_H281
  cli.SetCommand("pc fecc", PCREATE_NOTIFIER(CmdExternalCameraControl),
                 "Set far end camera control mode", "{ \"device\" | \"external\" }");
#endif
}
#endif // P_CLI


void OpalConsolePCSSEndPoint::SetRingInfo(ostream & output, bool verbose, const PString & filename, const PString & device, const PString & driver)
{
  m_ringFileName = filename;
  m_ringChannelParams.m_device = device;
  m_ringChannelParams.m_driver = driver;

  if (verbose)
    output << "Ring file: ";

  if (m_ringFileName.IsEmpty()) {
    if (verbose)
      output << "not configured." << endl;
    return;
  }

  PWAVFile wavFile;
  if (!wavFile.Open(m_ringFileName, PFile::ReadOnly)) {
    output << '"' << m_ringFileName << "\" non-existant or invalid." << endl;
    return;
  }

  m_ringChannelParams.m_channels = wavFile.GetChannels();
  m_ringChannelParams.m_sampleRate = wavFile.GetSampleRate();
  m_ringChannelParams.m_bitsPerSample = wavFile.GetSampleSize();

  if (!PSoundChannel().Open(m_ringChannelParams)) {
    OutputSoundDeviceError(output, PSoundChannel::Player, device, driver);
    return;
  }

  if (verbose)
    output << '"' << m_ringFileName << "\" on " << m_ringChannelParams << endl;
}


bool OpalConsolePCSSEndPoint::OnIncomingCall(OpalLocalConnection & connection)
{
  if (!OpalPCSSEndPoint::OnIncomingCall(connection))
    return false;

  if (m_deferredAnswer && !m_ringFileName.IsEmpty()) {
    m_ringState = e_Ringing;

    if (m_ringThread == NULL)
      m_ringThread = new PThreadObj<OpalConsolePCSSEndPoint>(*this, &OpalConsolePCSSEndPoint::RingThreadMain, false, "Ringer");
    else
      m_ringSignal.Signal();
  }

  return true;
}


void OpalConsolePCSSEndPoint::OnConnected(OpalConnection & connection)
{
  m_ringState = e_RingIdle;
  m_ringSignal.Signal();
  OpalPCSSEndPoint::OnConnected(connection);
}


void OpalConsolePCSSEndPoint::OnReleased(OpalConnection & connection)
{
  m_ringState = e_RingIdle;
  m_ringSignal.Signal();
  OpalPCSSEndPoint::OnReleased(connection);
}


void OpalConsolePCSSEndPoint::ShutDown()
{
  m_ringState = e_RingShutDown;
  m_ringSignal.Signal();
  PThread::WaitAndDelete(m_ringThread);

  OpalPCSSEndPoint::ShutDown();
}


void OpalConsolePCSSEndPoint::RingThreadMain()
{
  PTRACE(4, "Ringer thread started");
  for (;;) {
    switch (m_ringState) {
      case e_RingIdle :
        m_ringSignal.Wait();
        break;

      case e_RingShutDown :
        PTRACE(4, "Ringer thread ended");
        return;

      case e_Ringing:
        PSoundChannel channel;
        if (!channel.Open(m_ringChannelParams)) {
          PTRACE(2, "Could not open " << m_ringChannelParams);
          m_ringState = e_RingIdle;
          break;
        }

        PTRACE(3, "Started playing ring file \"" << m_ringFileName << "\" on " << m_ringChannelParams);

        while (m_ringState == e_Ringing) {
          if (channel.HasPlayCompleted())
            channel.PlayFile(m_ringFileName, false);
          else
            m_ringSignal.Wait(200);
        }
        PTRACE(3, "Ended playing ring file \"" << m_ringFileName << "\" on " << m_ringChannelParams);
    }
  }
}
#endif // OPAL_HAS_PCSS


/////////////////////////////////////////////////////////////////////////////

#if OPAL_IVR
OpalConsoleIVREndPoint::OpalConsoleIVREndPoint(OpalManagerConsole & manager)
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
  OpalManagerConsole::LockedStream lockedOutput(m_console);
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
OpalConsoleMixerEndPoint::OpalConsoleMixerEndPoint(OpalManagerConsole & manager)
  : OpalMixerEndPoint(manager)
  , OpalConsoleEndPoint(manager)
{
}


void OpalConsoleMixerEndPoint::GetArgumentSpec(ostream & strm) const
{
  strm << "[Mixer (MCU) options:]"
          "-no-mcu.       Disable MCU subsystem\n"
#if OPAL_VIDEO
          "-audio-only.   Audio only conference\n"
#endif
          ;
}


bool OpalConsoleMixerEndPoint::Initialise(PArgList & args, bool verbose, const PString &)
{
  OpalManagerConsole::LockedStream lockedOutput(m_console);
  ostream & output = lockedOutput;

  if (args.HasOption("no-mcu")) {
    if (verbose)
      output << "Conference disabled.\n";
    return true;
  }

  OpalMixerNodeInfo adHoc;
#if OPAL_VIDEO
  adHoc.m_audioOnly = args.HasOption("audio-only");
#endif
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

OpalManagerConsole::OpalManagerConsole(const char * endpointPrefixes)
  : m_endpointPrefixes(PConstString(endpointPrefixes).Tokenise(" \t\n"))
  , m_interrupted(false)
  , m_verbose(false)
  , m_outputStream(&cout)
{
}


OpalManagerConsole::~OpalManagerConsole()
{
  // Must do this before m_outputStream and m_outputMutex go out of scope
  ShutDownEndpoints();
}


PString OpalManagerConsole::GetArgumentSpec() const
{
  PStringStream str;
  str << "[Global options:]"
         "u-user:            Set local username, defaults to OS username.\n"
         "p-password:        Set password for authentication.\n"
         "D-disable:         Disable use of specified media formats (codecs).\n"
         "P-prefer:          Set preference order for media formats (codecs).\n"
         "O-option:          Set options for media format, argument is of form fmt:opt=val or @type:opt=val.\n"
         "-auto-start:       Set auto-start option for media type, e.g audio:sendrecv or video:sendonly.\n"
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
    OpalConsoleEndPoint * ep = const_cast<OpalManagerConsole *>(this)->GetConsoleEndPoint(m_endpointPrefixes[i]);
    if (ep != NULL)
      ep->GetArgumentSpec(str);
  }

#if OPAL_PTLIB_SSL
  str << "[SSL/TLS options:]"
         "-ssl-ca:           Set SSL/TLS certificate authority directory/file.\n"
         "-ssl-cert:         Set SSL/TLS certificate for local client.\n"
         "-ssl-key:          Set SSL/TLS private key lor local certificate.\n"
         "-ssl-no-create.    Do not auto-create SSL/TLS certificate/private key if does not exist.\n";
#endif

  str << "[IP options:]"
#if OPAL_PTLIB_NAT
         "-nat-method:       Set NAT method, defaults to STUN\n"
         "-nat-server:       Set NAT server for the above method\n"
         "-nat-interface:    Set NAT interface for the above method\n"
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

  // Unavailable components due to compile time configuration
#if !OPAL_H323 || !OPAL_SIP || !OPAL_SDP_HTTP || !OPAL_SKINNY || !OPAL_LYNC || !OPAL_LID || !OPAL_CAPI || !OPAL_IVR || !OPAL_HAS_MIXER
  str << "[Unavailable subsystems:]";
#endif
#if !OPAL_H323
  str << "-no-h323.";
#endif
#if !OPAL_SIP
  str << "-no-sip.";
#endif
#if !OPAL_SDP_HTTP
  str << "-no-sdp.";
#endif
#if !OPAL_SKINNY
  str << "-no-sccp.";
#endif
#if !OPAL_LYNC
  str << "-no-lync.";
#endif
#if !OPAL_LID
  str << "-no-lid.";
#endif
#if !OPAL_CAPI
  str << "-no-capi.";
#endif
#if !OPAL_IVR
  str << "-no-ivr.";
#endif
#if !OPAL_HAS_MIXER
  str << "-no-mcu.";
#endif
  return str;
}


void OpalManagerConsole::Usage(ostream & strm, const PArgList & args)
{
  args.Usage(strm, "[ <options> ... ]");
}


bool OpalManagerConsole::PreInitialise(PArgList & args, bool verbose)
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

  PTRACE_INITIALISE(args);

  return true;
}


static bool SetMediaFormatOption(ostream & output, bool verbose, const PString & format, const PString & name, const PString & value)
{
  if (format[0] == '@') {
    OpalMediaType mediaType = format.Mid(1);
    if (mediaType.empty()) {
      output << "Unknown media type \"" << format << '"' << endl;
      return false;
    }

    OpalMediaFormatList allFormats;
    OpalMediaFormat::GetAllRegisteredMediaFormats(allFormats);
    for (OpalMediaFormatList::iterator it = allFormats.begin(); it != allFormats.end(); ++it) {
      if (it->IsMediaType(mediaType) && !SetMediaFormatOption(output, verbose, it->GetName(), name, value))
        return false;
    }

    return true;
  }

  OpalMediaFormat mediaFormat(format);
  if (!mediaFormat.IsValid()) {
    output << "Unknown media format \"" << format << '"' << endl;
    return false;
  }

  if (!mediaFormat.HasOption(name)) {
    output << "Unknown option name \"" << name << "\" in media format \"" << format << '"' << endl;
    return false;
  }

  if (!mediaFormat.SetOptionValue(name, value)) {
    output << "Ilegal value \"" << value << "\""
              " for option name \"" << name << "\""
              " in media format \"" << format << '"' << endl;
    return false;
  }

  if (!OpalMediaFormat::SetRegisteredMediaFormat(mediaFormat)) {
    output << "Could not set registered media format \"" << format << '"' << endl;
    return false;
  }

  if (verbose)
    output << "Media format \"" << format << "\" option \"" << name << "\" set to \"" << value << "\"\n";

  return true;
}


bool OpalManagerConsole::Initialise(PArgList & args, bool verbose, const PString & defaultRoute)
{
  if (!PreInitialise(args, verbose))
    return false;

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

  {
    OpalMediaType::AutoStartMap autoStart;
    if (autoStart.Add(args.GetOptionString("auto-start")))
      autoStart.SetGlobalAutoStart();
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

  if (args.HasOption("no-inband-detect"))
    DisableDetectInBandDTMF(true);

#if OPAL_PTLIB_SSL
  SetSSLCertificateAuthorityFiles(args.GetOptionString("ssl-ca", GetSSLCertificateAuthorityFiles()));
  SetSSLCertificateFile(args.GetOptionString("ssl-cert", GetSSLCertificateFile()));
  SetSSLPrivateKeyFile(args.GetOptionString("ssl-key", GetSSLPrivateKeyFile()));
  SetSSLAutoCreateCertificate(!args.HasOption("ssl-no-create"));
  if (verbose)
    output << "SSL/TLS certificate authority: " << GetSSLCertificateAuthorityFiles() << "\n"
              "SSL/TLS certificate: " << GetSSLCertificateFile() << "\n"
              "SSL/TLS private key: " << GetSSLPrivateKeyFile() << "\n"
              "SSL/TLS auto-create certificate/key: " << (GetSSLAutoCreateCertificate() ? "Yes" : "No") << '\n';
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

#if OPAL_PTLIB_NAT
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
      output << "Establishing " << natMethod << " ..." << flush;
    if (SetNATServer(natMethod, natServer, true, 0, args.GetOptionString("nat-interface"))) {
      if (verbose)
        output << '\n' << *GetNatMethods().GetMethodByName(natMethod) << '\n';
    }
    else {
      if (verbose)
        output << '\n';
      output << natMethod << " unavailable.\n";
    }
  }
#endif // OPAL_PTLIB_NAT

  if (verbose) {
    PIPSocket::InterfaceTable interfaceTable;
    if (PIPSocket::GetInterfaceTable(interfaceTable))
      output << "Detected " << interfaceTable.GetSize() << " network interfaces:\n"
               << setfill('\n') << interfaceTable << setfill(' ');
  }

  for (PINDEX i = 0; i < m_endpointPrefixes.GetSize(); ++i) {
    if (verbose)
      output << "---------------------------------\n";
    OpalConsoleEndPoint * ep = GetConsoleEndPoint(m_endpointPrefixes[i]);
    if (ep != NULL) {
      if (!ep->Initialise(args, verbose, defaultRoute))
        return false;
    }
    else {
      if (verbose)
        output << m_endpointPrefixes[i] << " unavailable.\n";
    }
  }

  if (verbose)
    output << "---------------------------------\n";

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
      PRegularExpression parse("(@?[A-Za-z].*):([A-Za-z].*)=(.*)", PRegularExpression::Extended);
      PStringArray subexpressions(4);
      if (!parse.Execute(options[i], subexpressions)) {
        output << "Invalid media format option \"" << options[i] << '"' << endl;
        return false;
      }

      if (!SetMediaFormatOption(output, verbose, subexpressions[1], subexpressions[2], subexpressions[3]))
        return false;
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


void OpalManagerConsole::Run()
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


void OpalManagerConsole::EndRun(bool interrupt)
{
  PTRACE(2, "Shutting down " << (interrupt ? " via interrupt" : " normally"));
  Broadcast(PSTRSTRM("\nShutting down " << PProcess::Current().GetName()
                     << (interrupt ? " via interrupt" : " normally") << " . . . "));

  m_interrupted = interrupt;
  m_endRun.Signal();
}


void OpalManagerConsole::Broadcast(const PString & msg)
{
  if (m_verbose)
    *LockedOutput() << msg << endl;
}


OpalConsoleEndPoint * OpalManagerConsole::GetConsoleEndPoint(const PString & prefix)
{
  OpalEndPoint * ep = FindEndPoint(prefix);
  if (ep == NULL) {
#if OPAL_H323
    if (prefix == OPAL_PREFIX_H323)
      ep = CreateH323EndPoint();
    else
#endif // OPAL_H323
#if OPAL_SIP
    if (prefix == OPAL_PREFIX_SIP)
      ep = CreateSIPEndPoint();
    else
#endif // OPAL_SIP
#if OPAL_SDP_HTTP
    if (prefix == OPAL_PREFIX_SDP)
      ep = CreateSDPHTTPEndPoint();
    else
#endif
#if OPAL_SKINNY
    if (prefix == OPAL_PREFIX_SKINNY)
      ep = CreateSkinnyEndPoint();
    else
#endif // OPAL_SKINNY
#if OPAL_LYNC
    if (prefix == OPAL_PREFIX_LYNC)
      ep = CreateLyncEndPoint();
    else
#endif // OPAL_LYNC
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
      PTRACE(1, "Unknown prefix " << prefix);
      return NULL;
    }
  }

  return dynamic_cast<OpalConsoleEndPoint *>(ep);
}


#if OPAL_H323
H323ConsoleEndPoint * OpalManagerConsole::CreateH323EndPoint()
{
  return new H323ConsoleEndPoint(*this);
}
#endif // OPAL_H323


#if OPAL_SIP
SIPConsoleEndPoint * OpalManagerConsole::CreateSIPEndPoint()
{
  return new SIPConsoleEndPoint(*this);
}
#endif // OPAL_SIP


#if OPAL_SDP_HTTP
OpalSDPHTTPConsoleEndPoint * OpalManagerConsole::CreateSDPHTTPEndPoint()
{
  return new OpalSDPHTTPConsoleEndPoint(*this);
}
#endif // OPAL_SDP_HTTP


#if OPAL_SKINNY
OpalConsoleSkinnyEndPoint * OpalManagerConsole::CreateSkinnyEndPoint()
{
  return new OpalConsoleSkinnyEndPoint(*this);
}
#endif // OPAL_SKINNY


#if OPAL_LYNC
OpalConsoleLyncEndPoint * OpalManagerConsole::CreateLyncEndPoint()
{
  return new OpalConsoleLyncEndPoint(*this);
}
#endif // OPAL_LYNC


#if OPAL_LID
OpalConsoleLineEndPoint * OpalManagerConsole::CreateLineEndPoint()
{
  return new OpalConsoleLineEndPoint(*this);
}
#endif // OPAL_LID


#if OPAL_CAPI
OpalConsoleCapiEndPoint * OpalManagerConsole::CreateCapiEndPoint()
{
  return new OpalConsoleCapiEndPoint(*this);
}
#endif // OPAL_LID


#if OPAL_HAS_PCSS
OpalConsolePCSSEndPoint * OpalManagerConsole::CreatePCSSEndPoint()
{
  return new OpalConsolePCSSEndPoint(*this);
}
#endif


#if OPAL_IVR
OpalConsoleIVREndPoint * OpalManagerConsole::CreateIVREndPoint()
{
  return new OpalConsoleIVREndPoint(*this);
}
#endif


#if OPAL_HAS_MIXER
OpalConsoleMixerEndPoint * OpalManagerConsole::CreateMixerEndPoint()
{
  return new OpalConsoleMixerEndPoint(*this);
}
#endif


bool OpalManagerConsole::OnLocalOutgoingCall(const OpalLocalConnection & connection)
{
  OpalCall & call = connection.GetCall();
  Broadcast(PSTRSTRM('\n' << call.GetToken() << ": Call at " << PTime().AsString("w h:mma")
                  << " from " << call.GetPartyA() << " to " << call.GetPartyB() << " ringing."));
  return OpalManager::OnLocalOutgoingCall(connection);
}


void OpalManagerConsole::OnEstablishedCall(OpalCall & call)
{
  Broadcast(PSTRSTRM('\n' << call.GetToken() << ": Established call from " << call.GetPartyA() << " to " << call.GetPartyB()));
  OpalManager::OnEstablishedCall(call);
}


void OpalManagerConsole::OnHold(OpalConnection & connection, bool fromRemote, bool onHold)
{
  OpalManager::OnHold(connection, fromRemote, onHold);

  PStringStream output;
  output << '\n' << connection.GetCall().GetToken() << ": remote " << connection.GetRemotePartyName() << " has ";
  if (fromRemote)
    output << (onHold ? "put you on" : "released you from");
  else
    output << "been " << (onHold ? "put on" : "released from");
  output << " hold.";
  Broadcast(output);
}


bool OpalManagerConsole::OnChangedPresentationRole(OpalConnection & connection, const PString & newChairURI, bool request)
{
  PStringStream output;
  output << '\n' << connection.GetCall().GetToken() << ": presentation role token now owned by ";
  if (newChairURI.IsEmpty())
    output << "nobody";
  else if (newChairURI == connection.GetLocalPartyURL())
    output << "local user";
  else
    output << '"' << newChairURI << '"';
  output << '.';
  Broadcast(output);

  return OpalManager::OnChangedPresentationRole(connection, newChairURI, request);
}


void OpalManagerConsole::OnStartMediaPatch(OpalConnection & connection, OpalMediaPatch & patch)
{
  OpalManager::OnStartMediaPatch(connection, patch);

  if (m_verbose && connection.IsNetworkConnection()) {
    OpalMediaStreamPtr stream(patch.GetSink());
    if (stream == NULL || &stream->GetConnection() != &connection)
      stream = &patch.GetSource();
    stream->PrintDetail(LockedOutput(), "Started");
  }
}


#if OPAL_STATISTICS
static PString MakeStatisticsKey(const OpalMediaStream & stream)
{
  return stream.GetID() + (stream.IsSource() ? "-Source" : "-Sink");
}
#endif


void OpalManagerConsole::OnClosedMediaStream(const OpalMediaStream & stream)
{
  OpalManager::OnClosedMediaStream(stream);

  if (m_verbose && stream.GetConnection().IsNetworkConnection())
    stream.PrintDetail(LockedOutput(), "Stopped");

#if OPAL_STATISTICS
  m_statsMutex.Wait();
  StatsMap::iterator it = m_statistics.find(MakeStatisticsKey(stream));
  if (it != m_statistics.end())
    m_statistics.erase(it);
  m_statsMutex.Signal();
#endif
}


void OpalManagerConsole::OnFailedMediaStream(OpalConnection & connection, bool fromRemote, const PString & reason)
{
  OpalManager::OnFailedMediaStream(connection, fromRemote, reason);

  if (m_verbose && connection.IsNetworkConnection())
    *LockedOutput() << (fromRemote ? "Remote" : "Local") << " open of media failed: " << reason << endl;
}


void OpalManagerConsole::OnUserInputString(OpalConnection & connection, const PString & value)
{
  if (connection.IsNetworkConnection())
    Broadcast(PSTRSTRM('\n' << connection.GetCall().GetToken() << ": received user input \"" << value << '"'));
  OpalManager::OnUserInputString(connection, value);
}


void OpalManagerConsole::OnClearedCall(OpalCall & call)
{
  OpalManager::OnClearedCall(call);

  PString name = call.GetPartyB().IsEmpty() ? call.GetPartyA() : call.GetPartyB();

  PStringStream output;

  output << '\n' << call.GetToken() << ": ";
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
            << setprecision(0) << setw(5) << (now - call.GetStartTime()) << "s.";
  Broadcast(output);
}


#if OPAL_STATISTICS
bool OpalManagerConsole::OutputStatistics()
{
  if (m_statsFile.IsEmpty())
    return OutputStatistics(LockedOutput());

  PTextFile file(m_statsFile);
  if (!file.Open(PFile::WriteOnly, PFile::Create))
    return false;

  file.SetPosition(0, PFile::End);
  return OutputStatistics(file);
}


bool OpalManagerConsole::OutputStatistics(ostream & strm)
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


bool OpalManagerConsole::OutputCallStatistics(ostream & strm, OpalCall & call)
{
  PSafePtr<OpalConnection> connection = call.GetConnection(0);
  if (connection == NULL)
    return false; // This really shold not happen

  if (!connection->IsNetworkConnection()) {
    PSafePtr<OpalConnection> otherConnection = call.GetConnection(1);
    if (otherConnection != NULL)
      connection = otherConnection;
  }

  strm << '\n' << call.GetToken() << ": call from " << call.GetPartyA() << " to " << call.GetPartyB() << "\n"
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


bool OpalManagerConsole::OutputStreamStatistics(ostream & strm, const OpalMediaStream & stream)
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
  : OpalManagerConsole(endpointPrefixes)
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
  PString spec = OpalManagerConsole::GetArgumentSpec();
  // Insert just before the version option
  spec.Splice("F-script-file: Execute script file in CLI\n"
#if P_TELNET
              "-cli: Enable telnet command line sessions on port.\n"
#endif
#if P_CURSES
              "-tui. Enable text user interface.\n"
#endif
              "-page: Enable text output page every N lines,\r"
              "-1 (default) indicates automatic determine terminal size.\r"
              "0 indicates disable paging and output all text.\n"
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
        if (cli->GetWindowCount() > 0)
          m_cli = cli;
        else
          *LockedOutput() << "Could not create text user interface, probably redirected I/O, using normal CLI" << endl;
      }
    }
#endif // P_CURSES

    if (m_cli == NULL && (m_cli = CreateCLIStandard()) == NULL)
      return false;
  }

  m_cli->SetPagerLines(args.GetOptionAs("page", -1));

  {
    PCLI::Context * context = m_cli->StartForeground();
    if (context != NULL)
      m_outputStream = context;
  }

  m_cli->SetPrompt(args.GetCommandName() + "> ");

#if OPAL_PTLIB_SSL
  m_cli->SetCommand("ssl", PCREATE_NOTIFIER(CmdSSL),
                    "Set SSL/TLS certificates",
                    "[ --ca <ca-dir-file> ] [ --cert <cert> ] [ --key <key> ] [ --no-create ]",
                    "a-ca:        Set SSL/TLS certificate authority directory/file.\n"
                    "c-cert:      Set SSL/TLS certificate for local client.\n"
                    "k-key:       Set SSL/TLS private key lor local certificate.\n"
                    "C-create.    Auto-create SSL/TLS certificate/private key if does not exist.\n"
                    "n-no-create. Do not auto-create SSL/TLS certificate/private key if does not exist.\n");
#endif

#if OPAL_PTLIB_NAT
  m_cli->SetCommand("nat list", PCREATE_NOTIFIER(CmdNatList),
                    "List NAT methods and server addresses");
  m_cli->SetCommand("nat server", PCREATE_NOTIFIER(CmdNatAddress),
                    "Set NAT server address for method, \"off\" deactivates method",
                    "[ --interface <iface> ] <method> <address>",
                    "I-interface: Set interface to bind NAT method");
#endif

#if PTRACING
  m_cli->SetCommand("trace", PCREATE_NOTIFIER(CmdTrace),
                    "Set trace level (1..6) and filename",
                    "[ --option <opt> ] <n> [ <filename> ]",
                    "O-option: Specify trace option(s),\r" PTRACE_ARGLIST_OPT_HELP);
#endif

#if OPAL_STATISTICS
  m_cli->SetCommand("statistics", PCREATE_NOTIFIER(CmdStatistics),
                    "Display statistics for call",
                    "[ <call-token> ]");
#endif

#if OPAL_HAS_H281
  m_cli->SetCommand("camera", PCREATE_NOTIFIER(CmdFarEndCamera),
                    "Far End Camera Control",
                    "{ \"left\" | \"right\" | \"up\" | \"down\" | \"tight\" | \"wide\" | \"in\" | \"out\" } <milliseconds>",
                    "c-call: Indicate the call token to use, default is first call");
#endif

  m_cli->SetCommand("audio codec", PCREATE_NOTIFIER(CmdAudioCodec),
                    "Set audio codec for active call", "[ --call ] <codec>", "c-call: Token for call to change");
#if OPAL_VIDEO
  m_cli->SetCommand("video codec", PCREATE_NOTIFIER(CmdVideoCodec),
                    "Set video codec for active call", "[ --call ] <codec>", "c-call: Token for call to change");
  m_cli->SetCommand("video default", PCREATE_NOTIFIER(CmdVideoDefault),
                    "Set default video parameters for active call",
                    "[ <options> ] [ <codec> ... ]",
                    "s-size:         Desired transmit resolution\n"
                    "m-max-size:     Maximum receive resolution\n"
                    "f-frame-rate:   Desired transmit frame rate (fps)\n"
                    "b-bit-rate:     Desired transmit target bit rate (kbps)\n"
                    "M-max-bit-rate: Maximum receive bit rate (kbps)\n"
                    "t-tsto:         Desired transmit temporal/spatial trade off (1=quality 31=speed)\n");
  m_cli->SetCommand("video transmit", PCREATE_NOTIFIER(CmdVideoTransmit),
                    "Set video transmit parameters for active call",
                    "[ <options> ]",
                    "c-call:       Token for call to change\n"
                    "s-size:       Transmit resolution\n"
                    "f-frame-rate: Transmit frame rate (fps)\n"
                    "b-bit-rate:   Transmit target bit rate (kbps)\n"
                    "t-tsto:       Transmit temporal/spatial trade off (1=quality 31=speed)\n");
  m_cli->SetCommand("video receive", PCREATE_NOTIFIER(CmdVideoReceive),
                    "Request video receive parameters for active call",
                    "[ <options> ]",
                    "c-call:       Token for call to change\n"
                    "b-bit-rate:   Requested receive target bit rate (kbps)\n"
                    "t-tsto:       Requested receive temporal/spatial trade off (1=quality 31=speed)\n"
                    "i-intra.      Request Intra-Frame (key frame)\n");
  m_cli->SetCommand("video presentation", PCREATE_NOTIFIER(CmdPresentationToken),
                    "Request/release presentation token for active call",
                    "[ --call ] [ request | release ]",
                    "c-call: Token for call to change");
#endif // OPAL_VIDEO


#if OPAL_HAS_MIXER
  m_cli->SetCommand("record", PCREATE_NOTIFIER(CmdRecord), "Record call to file or cease recording",
                    " [ <options> ] { <file> | \"off\" }",
                    "c-call:       Token for call to record.\n"
                    "S-stereo.     Record receved audio in left channel and transmitted audio in right channel\n"
#if OPAL_VIDEO
                    "m-mode: Video composition mode, one of\r"
                      "Letterbox  - Side by side with black bars top and bottom.\r"
                      "SideBySide - Side by side, scaled to fit resolution.\r"
                      "Pillarbox  - One on top of the other, with black bars down the sides.\r"
                      "Stacked    - One on top of the other, scaled to fit resolution.\n"
                    "s-size:       Video composition resolution\n"
                    "f-frame-rate: Video composition frame rate (fps)\n"
#endif
  );
#endif // OPAL_HAS_MIXER

  m_cli->SetCommand("audio vad", PCREATE_NOTIFIER(CmdSilenceDetect),
                    "Voice Activity Detection (aka Silence Detection)", "\"on\" | \"adaptive\" | <level>");
  m_cli->SetCommand("audio in-band-dtmf-disable", m_disableDetectInBandDTMF, "In-band (digital filter) DTMF detection");

  m_cli->SetCommand("auto-start", PCREATE_NOTIFIER(CmdAutoStart),
                    "Set media type auto-start mode",
                    "[ <media-type> [ \"inactive\" | \"sendonly\" | \"recvonly\" | \"sendrecv\" | \"dontoffer\" | \"exclusive\" ] ]");

  m_cli->SetCommand("codec list", PCREATE_NOTIFIER(CmdCodecList),
                    "List available codecs");
  m_cli->SetCommand("codec order", PCREATE_NOTIFIER(CmdCodecOrder),
                    "Set codec selection order. A simple '*' character may be used for wildcard matching.",
                    "[ -a ] [ <wildcard> ... ]", "a-add. Add to existing list");
  m_cli->SetCommand("codec select\ncodec delete\ncodec mask", PCREATE_NOTIFIER(CmdCodecMask),
                    "Set codec selection list. A simple '*' character may be used for wildcard matching.",
                    "[ -a ] [ <wildcard> ... ]", "a-add. Add to existing list");
  m_cli->SetCommand("codec option", PCREATE_NOTIFIER(CmdCodecOption),
                    "Get/Set codec option value. The format may be @type (e.g. @video) and all codecs of that type are set.",
                    "<format> [ <name> [ <value> ] ]");

  m_cli->SetCommand("show calls", PCREATE_NOTIFIER(CmdShowCalls), "Show all active calls");
  m_cli->SetCommand("send input", PCREATE_NOTIFIER(CmdSendUserInput), "Send user input indication",
                    "[ --call ] <string>", "c-call: Token for call.");
  m_cli->SetCommand("hangup", PCREATE_NOTIFIER(CmdHangUp), "Hang up call",
                    "[ --call ]", "c-call: Token for call to hang up");
  m_cli->SetCommand("delay", PCREATE_NOTIFIER(CmdDelay),
                    "Delay for the specified number of seconds",
                    "<seconds>");
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

  return OpalManagerConsole::Initialise(args, verbose, defaultRoute);
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

  OpalManagerConsole::EndRun(interrupt);
}


void OpalManagerCLI::Broadcast(const PString & msg)
{
  if (m_verbose)
      m_cli->Broadcast(msg);
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


#if OPAL_PTLIB_SSL
void OpalManagerCLI::CmdSSL(PCLI::Arguments & args, P_INT_PTR)
{
  SetSSLCertificateAuthorityFiles(args.GetOptionString("ca", GetSSLCertificateAuthorityFiles()));
  SetSSLCertificateFile(args.GetOptionString("cert", GetSSLCertificateFile()));
  SetSSLPrivateKeyFile(args.GetOptionString("key", GetSSLPrivateKeyFile()));
  if (args.HasOption("create"))
    SetSSLAutoCreateCertificate(true);
  if (args.HasOption("no-create"))
    SetSSLAutoCreateCertificate(false);
  
  args.GetContext() << "SSL/TLS certificate authority: " << GetSSLCertificateAuthorityFiles() << "\n"
                       "SSL/TLS certificate: " << GetSSLCertificateFile() << "\n"
                       "SSL/TLS private key: " << GetSSLPrivateKeyFile() << "\n"
                       "SSL/TLS auto-create certificate/key: " << (GetSSLAutoCreateCertificate() ? "Yes" : "No") << '\n';
}
#endif

#if OPAL_PTLIB_NAT
void OpalManagerCLI::CmdNatList(PCLI::Arguments & args, P_INT_PTR)
{
  PCLI::Context & out = args.GetContext();
  out << std::left
      << setw(12) << "Name" << ' '
      << setw(10) << "State" << ' '
      << setw(20) << "Type" << ' '
      << setw(20) << "Server" << ' '
      <<             "External\n";

  for (PNatMethods::iterator it = GetNatMethods().begin(); it != GetNatMethods().end(); ++it) {
    out << setw(12) << it->GetMethodName() << ' '
        << setw(10) << (it->IsActive() ? "Active" : "Inactive")
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

  PIPSocket::Address iface(PIPSocket::GetDefaultIpAny());
  if (args.HasOption("interface")) {
    iface = args.GetOptionString("interface");
    if (!iface.IsValid()) {
      args.WriteError("Invalid IP address for interface");
      return;
    }
  }

  if (!natMethod->SetServer(args[1])) {
    args.WriteError() << natMethod->GetMethodName() << " server address invalid \"" << args[1] << '"' << endl;
    return;
  }

  if (!natMethod->Open(iface)) {
    args.WriteError() << natMethod->GetMethodName() << " could not access  \"" << natMethod->GetServer() << '"' << endl;
    return;
  }

  PCLI::Context & out = args.GetContext();
  out << natMethod->GetMethodName() << " server \"" << natMethod->GetServer() << "\" replies " << natMethod->GetNatType();
  PIPSocket::Address externalAddress;
  if (natMethod->GetExternalAddress(externalAddress))
    out << " with address " << externalAddress;
  out << endl;
}
#endif


#if PTRACING
void OpalManagerCLI::CmdTrace(PCLI::Arguments & args, P_INT_PTR)
{
  if (args.GetCount() > 0)
    PTrace::Initialise(args, PTrace::GetOptions(), NULL, "1", "option", NULL, "0");

  PTrace::PrintInfo(args.GetContext());
}
#endif // PTRACING


#if OPAL_STATISTICS
void OpalManagerCLI::CmdStatistics(PCLI::Arguments & args, P_INT_PTR)
{
  if (args.GetCount() == 0) {
    OutputStatistics(args.GetContext());
    return;
  }

  PSafePtr<OpalCall> call = FindCallWithLock(args[0], PSafeReadOnly);
  if (call == NULL) {
    args.WriteError() << "No call with supplied token.\n";
    return;
  }

  OutputCallStatistics(args.GetContext(), *call);
}
#endif // OPAL_STATISTICS


#if OPAL_HAS_H281
struct OpalCmdFarEndCameraControlDirection
{
  P_DECLARE_STREAMABLE_ENUM(Cmd, left, right, up, down, tight, wide, in, out);
};

void OpalManagerCLI::CmdFarEndCamera(PCLI::Arguments & args, P_INT_PTR)
{
  if (args.GetCount() < 2) {
    args.WriteUsage();
    return;
  }

  PVideoControlInfo::Types type;
  int dir;
  switch (OpalCmdFarEndCameraControlDirection::CmdFromString(args[0], false)) {
    case OpalCmdFarEndCameraControlDirection::left :
      type = PVideoControlInfo::Pan;
      dir = -1;
      break;

    case OpalCmdFarEndCameraControlDirection::right :
      type = PVideoControlInfo::Pan;
      dir = 1;
      break;

    case OpalCmdFarEndCameraControlDirection::down :
      type = PVideoControlInfo::Tilt;
      dir = -1;
      break;

    case OpalCmdFarEndCameraControlDirection::up :
      type = PVideoControlInfo::Tilt;
      dir = 1;
      break;

    case OpalCmdFarEndCameraControlDirection::wide :
      type = PVideoControlInfo::Zoom;
      dir = -1;
      break;

    case OpalCmdFarEndCameraControlDirection::tight :
      type = PVideoControlInfo::Zoom;
      dir = 1;
      break;

    case OpalCmdFarEndCameraControlDirection::out :
      type = PVideoControlInfo::Focus;
      dir = -1;
      break;

    case OpalCmdFarEndCameraControlDirection::in :
      type = PVideoControlInfo::Focus;
      dir = 1;
      break;

    default :
      args.WriteUsage();
      return;
  }

  PCaselessString arg = args[1];
  PTimeInterval duration;
  if (isdigit(arg[0]))
    duration = arg.AsUnsigned();
  else if (arg == "stop")
    dir = 0;
  else if (arg != "start") {
    args.WriteUsage();
    return;
  }

  PString token = args.GetOptionString("call");
  if (token.IsEmpty()) {
    PStringArray tokens = GetAllCalls();
    if (tokens.IsEmpty()) {
      args.WriteError() << "No calls active." << endl;
      return;
    }
    token = tokens[0];
  }

  PSafePtr<OpalCall> call = FindCallWithLock(token, PSafeReadOnly);
  if (call == NULL) {
    args.WriteError() << "No call with supplied token." << endl;
    return;
  }

  PSafePtr<OpalLocalConnection> connection = call->GetConnectionAs<OpalLocalConnection>();
  if (connection == NULL) {
    args.WriteError() << "Cannot do far end camera control with connection." << endl;
    return;
  }

  if (connection->FarEndCameraControl(type, dir, duration))
    args.WriteError() << "Executing far end camera control." << endl;
  else
    args.WriteError() << "Could not perform far end camera control." << endl;
}
#endif // OPAL_HAS_H281


void OpalManagerCLI::CmdAutoStart(PCLI::Arguments & args, P_INT_PTR)
{
  switch (args.GetCount()) {
    case 0:
    {
      OpalMediaTypeList mediaTypes = OpalMediaType::GetList();
      string::size_type maxWidth = 0;
      for (OpalMediaTypeList::iterator it = mediaTypes.begin(); it != mediaTypes.end(); ++it)
        maxWidth = std::max(maxWidth, it->length());
      for (OpalMediaTypeList::iterator it = mediaTypes.begin(); it != mediaTypes.end(); ++it)
        args.GetContext() << setw(maxWidth+1) << *it << ' ' << (*it)->GetAutoStart() << endl;
      break;
    }

    case 1:
    {
      OpalMediaType mediaType(args[0]);
      if (mediaType.empty())
        args.WriteUsage();
      else
        args.GetContext() << mediaType << ' ' << mediaType->GetAutoStart() << endl;
      break;
    }

    default:
    {
      OpalMediaType::AutoStartMap autoStart;
      if (autoStart.Add(args[0], args[1]))
        autoStart.SetGlobalAutoStart();
      else
        args.WriteUsage();
      break;
    }
  }
}


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


void OpalManagerCLI::CmdCodecOption(PCLI::Arguments & args, P_INT_PTR)
{
  PString name;

  switch (args.GetCount()) {
    case 0 :
      args.WriteUsage();
      return;

    case 1 :
      break;

    case 2:
      name = args[1];
      break;

    default :
      SetMediaFormatOption(args.GetContext(), true, args[0], args[1], args.GetParameters(2).ToString());
      return;
  }

  OpalMediaFormat mediaFormat(args[0]);
  if (!mediaFormat.IsValid()) {
    args.WriteError() << "Unknown media format \"" << args[0] << '"' << endl;
    return;
  }

  if (name.IsEmpty()) {
    args.GetContext() << setw(-1) << mediaFormat << endl;
    return;
  }

  PString value;
  if (mediaFormat.GetOptionValue(name, value))
    args.GetContext() << "Media format \"" << mediaFormat << "\" option \"" << name << "\" is \"" << value << '"' << endl;
  else
    args.WriteError() << "Unknown option name \"" << name << "\" in media format \"" << mediaFormat << '"' << endl;
}


static void ChangeMediaCodec(OpalManagerCLI & manager, PCLI::Arguments & args, const OpalMediaType & mediaType)
{
  OpalMediaStreamPtr stream;
  if (!manager.GetStreamFromArgs(args, mediaType, true, stream))
    return;

  if (args.GetCount() == 0) {
    args.GetContext() << "Current codec: " << stream->GetMediaFormat() << endl;
    return;
  }

  OpalMediaFormat mediaFormat(args[0]);
  if (!mediaFormat.IsTransportable()) {
    args.WriteError("Media format is not available.");
    return;
  }

  if (mediaFormat.GetMediaType() != mediaType) {
    args.WriteError() << "Media format is not " << mediaType << '.' << endl;
    return;
  }

  if (stream->SetMediaFormat(mediaFormat))
    args.GetContext() << "Changed codec to " << mediaFormat << endl;
  else
    args.WriteError() << "Could not change codec to " << mediaFormat << endl;

}


void OpalManagerCLI::CmdAudioCodec(PCLI::Arguments & args, P_INT_PTR)
{
  ChangeMediaCodec(*this, args, OpalMediaType::Audio());
}


#if OPAL_VIDEO
void OpalManagerCLI::CmdVideoCodec(PCLI::Arguments & args, P_INT_PTR)
{
  ChangeMediaCodec(*this, args, OpalMediaType::Video());
}


void OpalManagerCLI::CmdVideoDefault(PCLI::Arguments & args, P_INT_PTR)
{
  OpalMediaFormatList mediaFormats;

  if (args.GetCount() == 0)
    mediaFormats = OpalMediaFormat::GetAllRegisteredMediaFormats();
  else {
    for (PINDEX i = 0; i < args.GetCount(); ++i) {
      OpalMediaFormat mediaFormat(args[i]);
      if (!mediaFormat.IsValid()) {
        args.WriteError() << "Unknown media format \"" << args[i] << '"' << endl;
        return;
      }
      mediaFormats += mediaFormat;
    }
  }

  for (OpalMediaFormatList::iterator it = mediaFormats.begin(); it != mediaFormats.end(); ++it) {
    if (it->GetMediaType() == OpalMediaType::Video()) {
      OpalMediaFormat mediaFormat = *it;
      if (GetVideoFormatFromArgs(args, mediaFormat, true))
        OpalMediaFormat::SetRegisteredMediaFormat(mediaFormat);
    }
  }
}


void OpalManagerCLI::CmdVideoTransmit(PCLI::Arguments & args, P_INT_PTR)
{
  OpalMediaStreamPtr stream;
  if (!GetStreamFromArgs(args, OpalMediaType::Video(), true, stream))
    return;

  OpalMediaFormat mediaFormat = stream->GetMediaFormat();
  if (GetVideoFormatFromArgs(args, mediaFormat, false))
    stream->UpdateMediaFormat(mediaFormat);
}


void OpalManagerCLI::CmdVideoReceive(PCLI::Arguments & args, P_INT_PTR)
{
  OpalMediaStreamPtr stream;
  if (!GetStreamFromArgs(args, OpalMediaType::Video(), false, stream))
    return;

  OpalBandwidth bitRate;
  if (GetValueFromArgs(args, "bit-rate", bitRate, AbsoluteMinBitRate, stream->GetMediaFormat().GetMaxBandwidth(), " for flow control request") > 0)
    stream->ExecuteCommand(OpalMediaFlowControl(bitRate, OpalMediaType::Video()));

  unsigned tsto;
  if (GetValueFromArgs(args, "tsto", tsto, 1U, 31U, " for temporal/spatial trade-off request") > 0)
    stream->ExecuteCommand(OpalTemporalSpatialTradeOff(tsto));

  if (args.HasOption("intra"))
    stream->ExecuteCommand(OpalVideoUpdatePicture());
}


struct OpalCmdPresentationToken
{
  P_DECLARE_STREAMABLE_ENUM(Cmd, request, release);
};

void OpalManagerCLI::CmdPresentationToken(PCLI::Arguments & args, P_INT_PTR)
{
  PSafePtr<OpalRTPConnection> connection;
  if (GetConnectionFromArgs(args, connection)) {
    if (args.GetCount() == 0)
      args.GetContext() << "Presentation token is " << (connection->HasPresentationRole() ? "acquired." : "released.") << endl;
    else {
      switch (OpalCmdPresentationToken::CmdFromString(args[0], false)) {
        case OpalCmdPresentationToken::request :
          if (connection->HasPresentationRole())
            args.GetContext() << "Presentation token is already acquired." << endl;
          else if (connection->RequestPresentationRole(false))
            args.GetContext() << "Presentation token requested." << endl;
          else
            args.WriteError("Presentation token not supported by remote.");
          break;

        case OpalCmdPresentationToken::release :
          if (!connection->HasPresentationRole())
            args.GetContext() << "Presentation token is already released." << endl;
          else if (connection->RequestPresentationRole(true))
            args.GetContext() << "Presentation token released." << endl;
          else
            args.WriteError("Presentation token release failed.");
          break;

        default :
          args.WriteUsage();
      }
    }
  }
}
#endif // OPAL_VIDEO


#if OPAL_HAS_MIXER
void OpalManagerCLI::CmdRecord(PCLI::Arguments & args, P_INT_PTR)
{
  if (args.GetCount() == 0) {
    args.WriteUsage();
    return;
  }

  PSafePtr<OpalCall> call;
  if (!GetCallFromArgs(args, call))
    return;

  if (args[0] *= "off") {
    if (call->StopRecording())
      args.WriteError() << "Recording stopped." << endl;
    else
      args.WriteError() << "Not recording." << endl;
    return;
  }

  OpalRecordManager::Options options;
  options.m_stereo = args.HasOption("stereo");
#if OPAL_VIDEO
  PCaselessString mode = args.GetOptionString("mode");
  if (mode == "Letterbox")
    options.m_videoMixing = OpalRecordManager::eSideBySideLetterbox;
  else if (mode == "SideBySide")
    options.m_videoMixing = OpalRecordManager::eSideBySideScaled;
  else if (mode == "Pillarbox")
    options.m_videoMixing = OpalRecordManager::eStackedPillarbox;
  else if (mode == "Stacked")
    options.m_videoMixing = OpalRecordManager::eStackedScaled;
  else if (!mode.IsEmpty()) {
    args.WriteError() << "Unknown video mode, \"" << mode << '"' << endl;
    return;
  }

  if (GetResolutionFromArgs(args, "size", options.m_videoWidth, options.m_videoHeight, " for recording") < 0)
    return;
  if (GetValueFromArgs(args, "rate", options.m_videoRate, 1U, 30U, " for recording") < 0)
    return;
#endif

  if (call->StartRecording(args[0], options))
    args.GetContext() << "Recording call to " << args[0] << endl;
  else
    args.WriteError() << "Could not start recording to \"" << args[0] << '"' << endl;
}
#endif // OPAL_HAS_MIXER


void OpalManagerCLI::CmdSilenceDetect(PCLI::Arguments & args, P_INT_PTR)
{
  if (args.GetCount() > 1) {
    args.WriteUsage();
    return;
  }

  OpalSilenceDetector::Params params = GetSilenceDetectParams();
  if (args.GetCount() != 0) {
    if (args[0] *= "off")
      params.m_mode = OpalSilenceDetector::NoSilenceDetection;
    else if (PConstCaselessString("adaptive").NumCompare(args[0]) == EqualTo)
      params.m_mode = OpalSilenceDetector::AdaptiveSilenceDetection;
    else if (args[0].FindSpan("0123456789") == P_MAX_INDEX) {
      params.m_mode = OpalSilenceDetector::FixedSilenceDetection;
      params.m_threshold = args[0].AsUnsigned();
    }
    else {
      args.WriteUsage();
      return;
    }
    SetSilenceDetectParams(params);
  }

  ostream & out = args.GetContext();
  out << "Silence Detect: ";
  switch (params.m_mode) {
    case OpalSilenceDetector::FixedSilenceDetection:
      out << "FIXED at " << params.m_threshold;
      break;

    case OpalSilenceDetector::AdaptiveSilenceDetection:
      out << "ADAPTIVE, "
             "period=" << params.m_adaptivePeriod << ", "
             "signal deadband=" << params.m_signalDeadband << ", "
             "silence deadband=" << params.m_silenceDeadband;
      break;

    default :
      out << "OFF";
  }
  out << endl;
}


static void CmdCodecOrderMask(OpalManager & manager, PCLI::Arguments & args, bool order, const char * bang)
{
  PStringArray formats = order ? manager.GetMediaFormatOrder() : manager.GetMediaFormatMask();

  if (args.GetCount() > 0) {
    if (!args.HasOption('a'))
      formats.RemoveAll();

    for (PINDEX i = 0; i < args.GetCount(); ++i)
      formats += bang + args[i];

    if (order)
      manager.SetMediaFormatOrder(formats);
    else
      manager.SetMediaFormatMask(formats);
  }

  args.GetContext() << "Codec " << (order ? "Order" : "Mask") << ": " << setfill(',') << formats << endl;
}


void OpalManagerCLI::CmdCodecOrder(PCLI::Arguments & args, P_INT_PTR)
{
  CmdCodecOrderMask(*this, args, true, "");
}


void OpalManagerCLI::CmdCodecMask(PCLI::Arguments & args, P_INT_PTR)
{
  CmdCodecOrderMask(*this, args, false, args.GetCommandName().Find("select") != P_MAX_INDEX ? "!" : "");
}


void OpalManagerCLI::CmdShowCalls(PCLI::Arguments & args, P_INT_PTR)
{
  ostream & out = args.GetContext();

  PStringArray calls = GetAllCalls();
  if (calls.IsEmpty()) {
    out << "No calls active." << endl;
    return;
  }

  for (PINDEX i = 0; i < calls.GetSize(); ++i) {
    PSafePtr<OpalCall> call = FindCallWithLock(calls[i]);
    if (call != NULL) {
      out << call->GetToken() << ": " << call->GetPartyA() << " -> " << call->GetPartyB();
      if (call->IsOnHold(true))
        out << ", on hold by remote";
      if (call->IsOnHold(false))
        out << ", remote on hold";
      out << endl;
    }
  }
}


void OpalManagerCLI::CmdSendUserInput(PCLI::Arguments & args, P_INT_PTR)
{
  if (args.GetCount() == 0) {
    args.WriteUsage();
    return;
  }

  PSafePtr<OpalLocalConnection> connection;
  if (GetConnectionFromArgs(args, connection))
    connection->OnUserInputString(args[0]);
}


void OpalManagerCLI::CmdHangUp(PCLI::Arguments & args, P_INT_PTR)
{
  PSafePtr<OpalCall> call;
  if (GetCallFromArgs(args, call)) {
    args.GetContext() << "Hanging up call from " << call->GetPartyA() << " to " << call->GetPartyB() << endl;
    call->Clear();
  }
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
  EndRun();
}


#endif // P_CLI


/////////////////////////////////////////////////////////////////////////////
