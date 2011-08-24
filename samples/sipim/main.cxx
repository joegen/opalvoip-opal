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

#include <im/im_ep.h>


PCREATE_PROCESS(SipIM);

SipIM::SipIM()
  : PProcess("OPAL", "IM Test", OPAL_MAJOR, OPAL_MINOR, ReleaseCode, OPAL_BUILD)
  , m_manager(NULL)
{
}


SipIM::~SipIM()
{
  delete m_manager;
}


void SipIM::Main()
{
  m_manager = new MyManager;

  PArgList & args = GetArguments();
  args.Parse(m_manager->GetArgumentSpec() +
             "l-listen."
             "f-script-file:");

  if (args.HasOption('h') || (args.GetCount() == 0 && !args.HasOption('l'))) {
    PString name = GetFile().GetTitle();
    cerr << "usage: " << name << " [ options ] [ url ]\n"
            "\n"
            "Available options are:\n"
            "  -l or --listen              : Listen for incoming connections\n"
            "  -f or --script-file fn      : Execute script file\n"
            "\n"
         << m_manager->GetArgumentUsage()
         << "\n"
            "e.g. " << name << " sip-im sip:fred@bloggs.com\n"
            "\n"
            "     " << name << " t.140\n\n";
    return;
  }

  if (!m_manager->Initialise(args, true))
    return;

  new OpalIMEndPoint(*m_manager);
  m_manager->AddRouteEntry("sip:.*\t.*=im:*");
  m_manager->AddRouteEntry("h323:.*\t.*=im:*");


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

  if (args.HasOption('f')) {
    // if there is a script file, process commands
    PTextFile scriptFile;
    if (scriptFile.Open(args.GetOptionString('f'))) {
      cout << "Running script '" << scriptFile.GetFilePath() << "'" << endl;
      cli.RunScript(scriptFile);
      cout << "Script complete" << endl;
    }
    else
      cerr << "error: cannot open script file \"" << args.GetOptionString('f') << '"' << endl;
  }

  cli.Start(false); // Do not spawn thread, wait till end of input
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
    message.m_bodies[PMIMEInfo::TextPlain()] = message.m_bodies[PMIMEInfo::TextPlain()] & args[i++];

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
    if (m_manager->FindEndPointAs<SIPEndPoint>("sip")->Register(params, aor, false))
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


// End of File ///////////////////////////////////////////////////////////////
