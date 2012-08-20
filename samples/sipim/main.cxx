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
#include <ptclib/mime.h>


extern const char Manufacturer[] = "OPAL";
extern const char Application[] = "IM Test";
typedef OpalConsoleProcess<MyManager, Manufacturer, Application> MyApp;
PCREATE_PROCESS(MyApp);


PString MyManager::GetArgumentSpec() const
{
  return "[Application options:]"
         "l-listen. Listen for incoming connections\n"
         + OpalManagerCLI::GetArgumentSpec();
}


void MyManager::Usage(ostream & strm, const PArgList & args)
{
  PString name = PProcess::Current().GetFile().GetTitle();
  strm << "\n"
          "usage: " << name << " [ options ... ] [ <to-url> [ <from-url> ] ]\n"
          "\n"
          "If there is no URL to dial specified then --listen must be present.\n"
          "\n"
          "e.g. " << name << " -P T.140 sip:fred@bloggs.com\n"
          "     " << name << " --listen\n"
          "\n"
          "\n";
  args.Usage(strm);
}


bool MyManager::Initialise(PArgList & args, bool verbose)
{
  if (!args.Parse(GetArgumentSpec()) && !args.HasOption('l')) {
    Usage(cerr, args);
    return false;
  }

  if (!OpalManagerCLI::Initialise(args, verbose))
    return false;

  m_imEP = new OpalIMEndPoint(*this);
  AddRouteEntry("sip:.*\t.*=im:*");
  AddRouteEntry("h323:.*\t.*=im:*");

  switch (args.GetCount()) {
    case 2 :
      m_variables.SetAt("FROM", args[1]);
      // Do next case
    case 1 :
      m_variables.SetAt("TO", args[0]);
  };

  m_cli->SetPrompt("IM> ");
  m_cli->SetCommand("set", PCREATE_NOTIFIER(CmdSet),
                    "Set variable. If no arguments, lists all variable currently set. If one argument unsets the variable.",
                    "[ <name> [ <value> ] ]");
  m_cli->SetCommand("send", PCREATE_NOTIFIER(CmdSend),
                    "Send Instant Message text.",
                    "[ options ] message ...",
                    "f-from: Set from address, if absent FROM variable used.\n"
                    "t-to:   Set to address, if absent TO variable used,\r"
                            "if that is not set then a system default is used.\n");
  m_cli->SetCommand("comp", PCREATE_NOTIFIER(CmdComp),
                    "Send message composition indication.",
                    "[ options ] { active | idle }",
                    "f-from: Set from address, if absent FROM variable used.\n"
                    "t-to:   Set to address, if absent TO variable used,\r"
                            "if that is not set then a system default is used.\n");

  return true;
}


void MyManager::CmdSet(PCLI::Arguments & args, INT)
{
  switch (args.GetCount()) {
    case 0 :
      if (m_variables.IsEmpty())
        args.GetContext() << "No variables set.\n";
      else
        args.GetContext() << m_variables;
      break;

    case 1 :
      if (!m_variables.Contains(args[0]))
        args.WriteError() << "Variable '" << args[0] << "' not found.\n";
      else {
        m_variables.RemoveAt(args[0]);
        args.GetContext() << "Variable '" << args[0] << "' removed.\n";
      }
      break;

    case 2 :
      m_variables.SetAt(args[0], args[1]);
      args.GetContext() << "Variable '" << args[0] << "' set to'" << args[1] << "'\n";
      break;

    default :
      args.WriteUsage();
  }
}


bool MyManager::GetToFromURL(const PArgList & args, PURL & to, PURL & from)
{
  to = args.GetOptionString('t', m_variables("TO"));
  if (to.IsEmpty())
    return false;

  from = args.GetOptionString('f', m_variables("FROM"));
  if (from.IsEmpty()) {
    from.SetScheme("sip");
    from.SetUserName(GetDefaultUserName());
    from.SetHostName(PIPSocket::GetHostName());
  }

  return true;
}


void MyManager::CmdSend(PCLI::Arguments & args, INT)
{
  if (args.GetCount() == 0) {
    args.WriteUsage();
    return;
  }

  OpalIM message;
  if (!GetToFromURL(args, message.m_to, message.m_from)) {
    args.WriteUsage();
    return;
  }

  message.m_bodies.SetAt(PMIMEInfo::TextPlain(), PString::Empty());
  for (PINDEX i = 0; i < args.GetCount(); ++i)
    message.m_bodies[PMIMEInfo::TextPlain()] &= args[i];

  if (Message(message))
    args.GetContext() << "IM #" << message.m_messageId << " send pending." << endl;
  else
    args.GetContext() << "IM send failed." << endl;
}


void MyManager::CmdComp(PCLI::Arguments & args, INT)
{
  if (args.GetCount() != 1) {
    args.WriteUsage();
    return;
  }

  PURL to, from;
  if (!GetToFromURL(args, to, from)) {
    args.WriteUsage();
    return;
  }

  PSafePtr<OpalIMContext> context = m_imEP->FindContextByNamesWithLock(from, to);
  if (context == NULL) {
    args.WriteError() << "No contex from " << from << " to " << to << endl;
    return;
  }

  OpalIMContext::CompositionInfo info(context->GetID(), args[0]);
  if (context->SendCompositionIndication(info))
    args.GetContext() << "IM composition indication sent." << endl;
  else
    args.GetContext() << "IM composition indication send failed." << endl;
}


void MyManager::OnConversation(const OpalIMContext::ConversationInfo & info)
{
  cout << "\nNew conversation started, id " << info.m_context->GetID() << endl;
}


void MyManager::OnMessageReceived(const OpalIM & message)
{
  cout << "\nIM From " << message.m_from << " to " << message.m_to;
  for (PStringToString::const_iterator it = message.m_bodies.begin(); it != message.m_bodies.end(); ++it)
    cout << "\nReceived " << it->first << '\n' << it->second;
  cout << endl;
}


void MyManager::OnMessageDisposition(const OpalIMContext::DispositionInfo & info)
{
  cout << "\nIM #" << info.m_messageId << ' ' << info.m_disposition << endl;
}


void MyManager::OnCompositionIndication(const OpalIMContext::CompositionInfo & info)
{
  cout << "\nRemote is " << info.m_state << endl;
}


// End of File ///////////////////////////////////////////////////////////////
