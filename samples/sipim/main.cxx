/*
 * main.cxx
 *
 * OPAL application source file for seing IM via SIP
 *
 * Copyright (c) 2008 Post Increment
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
 * $Revision$
 * $Author$
 * $Date$
 */

#include "main.h"

#include <opal/pcss.h>
#include <im/rfc4103.h>
#include <opal/patch.h>

PCREATE_PROCESS(SipIM);

SipIM::SipIM()
  : PProcess("OPAL SIP IM", "SipIM", OPAL_MAJOR, OPAL_MINOR, ReleaseCode, OPAL_BUILD)
  , m_manager(NULL)
{
}


SipIM::~SipIM()
{
  delete m_manager;
}


void SipIM::Main()
{
  PArgList & args = GetArguments();

  args.Parse(
             "f:"
             "h-help."
             "L-listener:"
             "-msrp."
             "p-password:"
             "-portbase:"
             "-portmax:"
             "r-register-sip:"
             "-rtp-base:"
             "-rtp-max:"
             "-sipim."
             "-stun:"
             "-t140."
             "-translate:"
             "-tcp-base:"
             "-tcp-max:"
             "-udp-base:"
             "-udp-max:"
             "u-user:"

#if PTRACING
             "o-output:"             "-no-output."
             "t-trace."              "-no-trace."
#endif
             , FALSE);

#if PTRACING
  PTrace::Initialise(args.GetOptionCount('t'),
                     args.HasOption('o') ? (const char *)args.GetOptionString('o') : NULL,
         PTrace::Blocks | PTrace::Timestamp | PTrace::Thread | PTrace::FileAndLine);
#endif

  if (args.HasOption('h')) {
    cerr << "usage: " << GetFile().GetTitle() << " [ options ] [url]\n"
            "\n"
            "Available options are:\n"
            "  -u or --user            : set local username.\n"
            "  --help                  : print this help message.\n"
            "  -L or --listener addr        : set listener address:port.\n"
#if OPAL_HAS_MSRP
            "  --msrp                  : use MSRP (default)\n"
#endif
#if OPAL_HAS_SIPIM
            "  --sipim                 : use SIPIM\n"
#endif
            "  --t140                  : use T.140\n"
#if PTRACING
            "  -o or --output file     : file name for output of log messages\n"       
            "  -t or --trace           : degree of verbosity in error log (more times for more detail)\n"     
#endif
            "\n"
            "e.g. " << GetFile().GetTitle() << " sip:fred@bloggs.com\n"
            ;
    return;
  }

  m_manager = new MyManager;

  if (args.HasOption('u'))
    m_manager->SetDefaultUserName(args.GetOptionString('u'));

  Mode mode;
  if (args.HasOption("t140")) {
    mode = Use_T140;
    m_manager->m_imFormat = OpalT140;
  }
#if OPAL_HAS_SIPIM
  else if (args.HasOption("sipim")) {
    mode = Use_SIPIM;
    m_manager->m_imFormat = OpalSIPIM;
  }
#endif
#if OPAL_HAS_MSRP
  else if (args.HasOption("msrp")) {
    mode = Use_MSRP;
    m_manager->m_imFormat = OpalMSRP;
  }
#endif
  //else {
  //  cout << "error: must select IM mode" << endl;
  //  return;
  //}

  OpalMediaFormatList allMediaFormats;
  PString listeners = args.GetOptionString('L');

  m_sipEP  = new SIPEndPoint(*m_manager);

  if (!m_sipEP->StartListeners(listeners)) {
    cerr << "Could not start SIP listeners." << endl;
    return;
  }
  allMediaFormats += m_sipEP->GetMediaFormats();

  MyPCSSEndPoint * pcss = new MyPCSSEndPoint(*m_manager);
  allMediaFormats += pcss->GetMediaFormats();
  pcss->SetSoundChannelPlayDevice("NULL");
  pcss->SetSoundChannelRecordDevice("NULL");

  allMediaFormats = OpalTranscoder::GetPossibleFormats(allMediaFormats); // Add transcoders
  allMediaFormats.RemoveNonTransportable();

  m_manager->AddRouteEntry("sip:.*\t.*=pc:*");
  m_manager->AddRouteEntry("pc:.*\t.*=sip:<da>");

  cout << "Available codecs: " << setfill(',') << allMediaFormats << setfill(' ') << endl;

  PString imFormatMask = PString("!") + (const char *)m_manager->m_imFormat;
  m_manager->SetMediaFormatMask(imFormatMask);
  allMediaFormats.Remove(imFormatMask);
  
  cout << "Codecs to be used: " << setfill(',') << allMediaFormats << setfill(' ') << endl;

  OpalConnection::StringOptions options;
  options.SetAt(OPAL_OPT_AUTO_START, m_manager->m_imFormat.GetMediaType() + ":exclusive");

  m_variables.SetAt("from", m_manager->GetDefaultUserName() + "@" + PIPSocket::GetHostName());

  PTextFile scriptFile;
  if (args.HasOption('f') && !scriptFile.Open(args.GetOptionString('f'))) {
    cerr << "error: cannot open script file '" << args.GetOptionString('f') << "'" << endl;
    return;
  }

#if 0
  if (args.GetCount() != 0) {
    do {
      //AddPresentity(args);
    } while (args.Parse(URL_OPTIONS));

    if (m_presentities.GetSize() == 0) {
      cerr << "error: no presentities available" << endl;
      return;
    }
  }
#endif

  PCLIStandard cli;
  cli.SetPrompt("IM> ");
#if 0
  cli.SetCommand("create", PCREATE_NOTIFIER(CmdCreate),
                 "Create presentity.",
                 "[ -p ] <url>");
  cli.SetCommand("list", PCREATE_NOTIFIER(CmdList),
                 "List presentities.");
  cli.SetCommand("subscribe", PCREATE_NOTIFIER(CmdSubscribeToPresence),
                 "Subscribe to presence state for presentity.",
                 "<url-watcher> <url-watched>");
  cli.SetCommand("unsubscribe", PCREATE_NOTIFIER(CmdUnsubscribeToPresence),
                 "Subscribe to presence state for presentity.",
                 "<url-watcher> <url-watched>");
  cli.SetCommand("authorise", PCREATE_NOTIFIER(CmdPresenceAuthorisation),
                 "Authorise a presentity to see local presence.",
                 "<url-watched> <url-watcher> [ deny | deny-politely | remove ]");
  cli.SetCommand("publish", PCREATE_NOTIFIER(CmdSetLocalPresence),
                 "Publish local presence state for presentity.",
                 "<url> { available | unavailable | busy } [ <note> ]");
  cli.SetCommand("buddy list\nshow buddies", PCREATE_NOTIFIER(CmdBuddyList),
                 "Show buddy list for presentity.",
                 "<presentity>");
  cli.SetCommand("buddy add\nadd buddy", PCREATE_NOTIFIER(CmdBuddyAdd),
                 "Add buddy to list for presentity.",
                 "<presentity> <url-buddy> <display-name>");
  cli.SetCommand("buddy remove\ndel buddy", PCREATE_NOTIFIER(CmdBuddyRemove),
                 "Delete buddy from list for presentity.",
                 "<presentity> <url-buddy>");
  cli.SetCommand("buddy subscribe", PCREATE_NOTIFIER(CmdBuddySusbcribe),
                 "Susbcribe to all URIs in the buddy list for presentity.",
                 "<presentity>");
#endif
  cli.SetCommand("stun", PCREATE_NOTIFIER(CmdStun),
                  "Set external STUN server",
                  "address");
  cli.SetCommand("translate", PCREATE_NOTIFIER(CmdTranslate),
                  "Set external NAT translate address",
                  "address");
  cli.SetCommand("register", PCREATE_NOTIFIER(CmdRegister),
                  "Register with SIP registrar",
                  "username password domain");
  cli.SetCommand("delay", PCREATE_NOTIFIER(CmdDelay),
                  "Delay for n seconds",
                  "secs");
  cli.SetCommand("send", PCREATE_NOTIFIER(CmdSend),
                  "Send IM",
                  "dest message...");
  cli.SetCommand("set", PCREATE_NOTIFIER(CmdSet),
                  "Set/unset attribute",
                  "name <value>");
  cli.SetCommand("quit\nq\nexit", PCREATE_NOTIFIER(CmdQuit),
                  "Quit command line interpreter, note quitting from console also shuts down application.");

  // create the foreground context
  PCLI::Context * cliContext = cli.StartForeground();
  if (cliContext == NULL)
    return;

  // if there is a script file, process commands
  if (scriptFile.IsOpen()) {
    cout << "Running script '" << scriptFile.GetFilePath() << "'" << endl;
    for (;;) {
      PString line;
      if (!scriptFile.ReadLine(line))
        break;
      line = line.Trim();
      if (line.GetLength() > 0) {
        if ((line[0] != '#') && (line[0] != ';') && ((line.GetLength() < 2) || (line[0] != '/') || (line[1] != '/'))) {
          cout << line << endl;
          if (!cliContext->ProcessInput(line)) {
            cerr << "error: error occurred while processing script" << endl;
            return;
          }
        }
      }
    }
    cout << "Script complete" << endl;
  }
  scriptFile.Close();

  cli.RunContext(cliContext); // Do not spawn thread, wait till end of input

#if 0
  if (args.GetCount() == 0)
    cout << "Awaiting incoming call ..." << flush;
  else {
    if (!m_manager->SetUpCall("pc:", args[0], m_manager->m_callToken, NULL, 0, &options)) {
      cerr << "Could not start IM to \"" << args[0] << '"' << endl;
      return;
    }
  }

  m_manager->m_connected.Wait();

  RFC4103Context rfc4103(m_manager->m_imFormat);

  int count = 1;
  for (;;) {
    PThread::Sleep(5000);
    const char * textData = "Hello, world";

    if (count > 0) {
      PSafePtr<OpalCall> call = m_manager->FindCallWithLock(m_manager->m_callToken);
      if (call != NULL) {
        PSafePtr<OpalPCSSConnection> conn = call->GetConnectionAs<OpalPCSSConnection>();
        if (conn != NULL) {
          RTP_DataFrameList frameList = rfc4103.ConvertToFrames("text/plain", textData);
          OpalMediaFormat fmt(m_manager->m_imFormat);
          for (PINDEX i = 0; i < frameList.GetSize(); ++i) {
            conn->TransmitInternalIM(fmt, (RTP_IMFrame &)frameList[i]);
          }
        }
      }
      --count;
    }
  }

  // Wait for call to come in and finish
  m_manager->m_completed.Wait();
  cout << " completed.";
#endif
}


void SipIM::CmdQuit(PCLI::Arguments & args, INT)
{
  args.GetContext().Stop();
}


void SipIM::CmdSet(PCLI::Arguments & args, INT)
{
  if (args.GetCount() < 1) {
    if (m_variables.GetSize() == 0)
      args.GetContext() << "no variables set" << endl;
    else
      args.GetContext() << m_variables;
  }
  else if (args.GetCount() < 2) {
    if (!m_variables.Contains(args[0]))
      args.WriteError(PString("variable '") + args[0] + "' not found");
    else {
      m_variables.RemoveAt(args[0]);
      args.GetContext() << "variable '" << args[0] << "' removed" << endl;
    }
  }
  else {
    m_variables.RemoveAt(args[0]);
    m_variables.SetAt(args[0], args[1]);
    args.GetContext() << "variable '" << args[0] << "' set to '" << args[1] << "'" << endl;
  }
}


bool SipIM::CheckForVar(PString & var)
{
  if ((var.GetLength() < 2) || (var[0] != '$'))
    return true;

  var = var.Mid(1);
  if (!m_variables.Contains(var)) 
    return false;

  var = m_variables[var];
  return true;
}


void SipIM::CmdSend(PCLI::Arguments & args, INT)
{
  if (args.GetCount() < 2) {
    args.WriteUsage();
    return;
  }

  if (!m_variables.Contains("from")) {
    args.WriteError("variable 'from' must be set");
    return;
  }

  OpalIM message;
  message.m_from = m_variables["from"];

  PString to = args[0];
  if (!CheckForVar(to)) {
    args.WriteError(PString("variable '") + to + "' not found");
    return;
  }
  message.m_to = to;

  PINDEX i = 1;
  while (i < args.GetCount())
    message.m_body = message.m_body & args[i++];

  if (!m_manager->Message(message))
    args.GetContext() << "IM failed" << endl;
  else
    args.GetContext() << "IM sent" << endl;
}


void SipIM::CmdDelay(PCLI::Arguments & args, INT)
{
  if (args.GetCount() < 1)
    args.WriteUsage();
  else {
    int delay = args[0].AsInteger();
    PThread::Sleep(delay * 1000);
  }
}

void SipIM::CmdRegister(PCLI::Arguments & args, INT)
{
  if (args.GetCount() < 3)
    args.WriteUsage();
  else {
    SIPRegister::Params params;

    PString user     = args[0];
    PString password = args[1];
    PString domain   = args[2];

    params.m_registrarAddress = domain;
    params.m_addressOfRecord  = user;
    params.m_password         = password;
    params.m_realm            = domain;
    PString aor;
    args.GetContext() << "Registering with " << params.m_registrarAddress << "...";
    if (m_sipEP->Register(params, aor, false))
      args.GetContext() << "succeeded" << endl;
    else
      args.GetContext() << "failed" << endl;
  }
}

void SipIM::CmdTranslate(PCLI::Arguments & args, INT)
{
  if (args.GetCount() < 1)
    args.WriteUsage();
  else {
    m_manager->SetTranslationAddress(args[0]);
    cout << "External address set to " << args[0] << '\n';
  }
}

void SipIM::CmdStun(PCLI::Arguments & args, INT)
{
  if (args.GetCount() < 1)
    args.WriteUsage();
  else {
    m_manager->SetSTUNServer(args[0]);
    if (m_manager->GetSTUNClient() != NULL)
      args.GetContext() << args[0] << " replies " << m_manager->GetSTUNClient()->GetNatTypeName();
    else
      args.GetContext() << "None";
    args.GetContext() << endl;
  }
}


//////////////////////////////////////////////////////////////////////////


void MyManager::OnMessageReceived(const OpalIM & message)
{
}


#if 0

void MyManager::Initialise(PArgs * args)
{
  if (args.HasOption("translate")) {
    SetTranslationAddress(args.GetOptionString("translate"));
    cout << "External address set to " << GetTranslationAddress() << '\n';
  }

  if (args.HasOption("portbase")) {
    unsigned portbase = args.GetOptionString("portbase").AsUnsigned();
    unsigned portmax  = args.GetOptionString("portmax").AsUnsigned();
    SetTCPPorts  (portbase, portmax);
    SetUDPPorts  (portbase, portmax);
    SetRtpIpPorts(portbase, portmax);
  } else {
    if (args.HasOption("tcp-base"))
      SetTCPPorts(args.GetOptionString("tcp-base").AsUnsigned(),
                  args.GetOptionString("tcp-max").AsUnsigned());

    if (args.HasOption("udp-base"))
      SetUDPPorts(args.GetOptionString("udp-base").AsUnsigned(),
                  args.GetOptionString("udp-max").AsUnsigned());

    if (args.HasOption("rtp-base"))
      SetRtpIpPorts(args.GetOptionString("rtp-base").AsUnsigned(),
                    args.GetOptionString("rtp-max").AsUnsigned());
  }
}

#endif

void MyManager::OnClearedCall(OpalCall & /*call*/)
{
  m_connected.Signal();
  m_completed.Signal();
}

void MyManager::OnApplyStringOptions(OpalConnection & conn, OpalConnection::StringOptions & options)
{
  options.SetAt(OPAL_OPT_AUTO_START, m_imFormat.GetMediaType() + ":exclusive");
  OpalManager::OnApplyStringOptions(conn, options);
}

PBoolean MyPCSSEndPoint::OnShowIncoming(const OpalPCSSConnection & connection)
{
  MyManager & mgr = (MyManager &)manager;
  mgr.m_callToken = connection.GetCall().GetToken();
  AcceptIncomingConnection(mgr.m_callToken);
  mgr.m_connected.Signal();
  cout << "Incoming call connected" << endl;

  return PTrue;
}


PBoolean MyPCSSEndPoint::OnShowOutgoing(const OpalPCSSConnection & connection)
{
  MyManager & mgr = (MyManager &)manager;

  cout << "Outgoing call connected" << endl;
  mgr.m_connected.Signal();
  return PTrue;
}


// End of File ///////////////////////////////////////////////////////////////
