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
          "usage: " << name << " [ options ... ] [ <url> ]\n"
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
  if (args.Parse(GetArgumentSpec()) != PArgList::ParseWithArguments && !args.HasOption('l')) {
    Usage(cerr, args);
    return false;
  }

  if (!OpalManagerCLI::Initialise(args, verbose))
    return false;

  new OpalIMEndPoint(*this);
  AddRouteEntry("sip:.*\t.*=im:*");
  AddRouteEntry("h323:.*\t.*=im:*");


  m_cli->SetPrompt("IM> ");
  m_cli->SetCommand("send", PCREATE_NOTIFIER(CmdSend),
                    "Send IM",
                    "dest message...");
  m_cli->SetCommand("set", PCREATE_NOTIFIER(CmdSet),
                    "Set/unset attribute",
                    "name <value>");

  return true;
}


void MyManager::CmdSet(PCLI::Arguments & args, INT)
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


bool MyManager::CheckForVar(PString & var)
{
  if ((var.GetLength() < 2) || (var[0] != '$'))
    return true;

  var = var.Mid(1);
  if (!m_variables.Contains(var)) 
    return false;

  var = m_variables[var];
  return true;
}


void MyManager::CmdSend(PCLI::Arguments & args, INT)
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

  if (!Message(message))
    args.GetContext() << "IM failed" << endl;
  else
    args.GetContext() << "IM sent" << endl;
}


void MyManager::OnMessageReceived(const OpalIM & message)
{
}


// End of File ///////////////////////////////////////////////////////////////
