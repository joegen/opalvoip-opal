/*
 * main.cxx
 *
 * OPAL application source file for console mode OPAL videophone
 *
 * Copyright (c) 2008 Vox Lucida Pty. Ltd.
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
 */

#include "precompile.h"
#include "main.h"


extern const char Manufacturer[] = "Vox Gratia";
extern const char Application[] = "OPAL Console";
typedef OpalConsoleProcess<MyManager, Manufacturer, Application> MyApp;
PCREATE_PROCESS(MyApp);

// Debug command: -m 123 -V -S udp$*:25060 -H tcp$*:21720 -ttttodebugstream


MyManager::MyManager()
  : OpalManagerCLI(OPAL_CONSOLE_PREFIXES OPAL_PREFIX_PCSS)
  , m_autoAnswerTime(-1)
{
  m_autoAnswerTimer.SetNotifier(PCREATE_NOTIFIER(AutoAnswer));
}


PString MyManager::GetArgumentSpec() const
{
  PString spec = OpalManagerCLI::GetArgumentSpec();

  return "[Application options:]"
         "a-auto-answer;    Automatically answer incoming calls.\n"
         + spec;
}


void MyManager::Usage(ostream & strm, const PArgList & args)
{
  args.Usage(strm, "[ <options> ... ]\n[ <options> ... ] <remote-URI>\n[ <options> ... ] <local-URI> <remote-URI>");
}


bool MyManager::Initialise(PArgList & args, bool verbose)
{
  if (!OpalManagerCLI::Initialise(args, verbose, OPAL_PREFIX_PCSS":"))
    return false;

  SetAutoAnswer(LockedStream(*this), verbose, args, "auto-answer");

  m_cli->SetPrompt("OPAL> ");
  m_cli->SetCommand("auto-answer", PCREATE_NOTIFIER(CmdAutoAnswer), "Answer call automatically", "\"off\" | \"on\" | <seconds>");
  m_cli->SetCommand("speed-dial",  PCREATE_NOTIFIER(CmdSpeedDial), "Set speed dial", "[ <name> [ <url> ] ]");
  m_cli->SetCommand("call",        PCREATE_NOTIFIER(CmdCall),     "Start call", "<uri> | <speed-dial>");
  m_cli->SetCommand("answer",      PCREATE_NOTIFIER(CmdAnswer),   "Answer call");
  m_cli->SetCommand("hold",        PCREATE_NOTIFIER(CmdHold),     "Hold call");
  m_cli->SetCommand("retrieve",    PCREATE_NOTIFIER(CmdRetrieve), "Retrieve call from hold");
  m_cli->SetCommand("transfer",    PCREATE_NOTIFIER(CmdTransfer), "Transfer call", "<uri>");

  switch (args.GetCount()) {
    case 0 :
      break;

    case 1 :
      if (SetUpCall(OPAL_PREFIX_PCSS":*", args[0]) != NULL)
        *LockedStream(*this) << "Starting call to " << args[0] << endl;
      break;

    case 2 :
      if (SetUpCall(args[0], args[1]) != NULL)
        *LockedStream(*this) << "Starting call from " << args[0] << " to " << args[1] << endl;
      break;

    default :
      Usage(cerr, args);
      return false;
  }

  return true;
}


bool MyManager::OnLocalIncomingCall(OpalLocalConnection & connection)
{
  PStringStream output;
  output << '\n' << connection.GetCall().GetToken() << ": incoming call at "
         << PTime().AsString("w h : mma") << " from " << connection.GetCall().GetPartyA();

  if (!OpalManagerCLI::OnLocalIncomingCall(connection)) {
    output << " rejected.";
    Broadcast(output);
    return false;
  }

  if (m_autoAnswerTime < 0)
    output << ", answer? ";
  else {
    // If in a call and auto answer is on, we are busy
    if (m_activeCall != NULL) {
      output << " refused as busy.";
      Broadcast(output);
      return false;
    }

    output << ", auto-answer";
    if (m_autoAnswerTime > 0) {
      output << " in " << m_autoAnswerTime.GetSeconds() << " seconds.";
      m_autoAnswerTimer = m_autoAnswerTime;
    }
    else {
      output << " immediately.";
      connection.AcceptIncoming();
    }
  }

  Broadcast(output);

  m_activeCall = &connection.GetCall();
  return true;
}


void MyManager::AutoAnswer(PTimer &, P_INT_PTR)
{
  if (m_activeCall == NULL)
    return;

  PSafePtr<OpalLocalConnection> connection = m_activeCall->GetConnectionAs<OpalLocalConnection>();
  if (connection == NULL)
    return;

  connection->AcceptIncoming();
}


void MyManager::OnClearedCall(OpalCall & call)
{
  if (m_activeCall == &call)
    m_activeCall.SetNULL();
  else if (m_heldCall == &call)
    m_heldCall.SetNULL();

  OpalManagerCLI::OnClearedCall(call);
}


bool MyManager::SetAutoAnswer(ostream & output, bool verbose, const PArgList & args, const char * option)
{
  if (option != NULL ? args.HasOption(option) : (args.GetCount() > 0)) {
    PCaselessString value = option != NULL ? args.GetOptionString(option) : args[0];
    if (value == "off")
      m_autoAnswerTime = -1;
    else if (value == "on" || value == "immediate")
      m_autoAnswerTime = 0;
    else if (value.FindSpan("0123456789") == P_MAX_INDEX)
      m_autoAnswerTime.SetInterval(0, value.AsInteger());
    else
      return false;
  }

  FindEndPointAs<OpalPCSSEndPoint>(OPAL_PREFIX_PCSS)->SetDeferredAnswer(m_autoAnswerTime != 0);

  if (verbose) {
    output << "Auto answer: ";
    if (m_autoAnswerTime < 0)
      output << "off";
    else if (m_autoAnswerTime == 0)
      output << "immediate";
    else
      output << setprecision(0) << m_autoAnswerTime << " seconds";
    output << endl;
  }

  return true;
}


void MyManager::CmdAutoAnswer(PCLI::Arguments & args, P_INT_PTR)
{
  if (!SetAutoAnswer(args.GetContext(), true, args, NULL))
    args.WriteError("Invalid value for seconds");
}


void MyManager::CmdSpeedDial(PCLI::Arguments & args, P_INT_PTR)
{
  PStringToString::iterator it;
  switch (args.GetCount()) {
    case 0 :
      for (it = m_speedDial.begin(); it != m_speedDial.end(); ++it)
        args.GetContext() << it->first << " -> " << it->second << endl;
      break;

    case 1 :
      if ((it = m_speedDial.find(args[0])) == m_speedDial.end())
        args.WriteError("No speed dial of that name.");
      else
        args.GetContext() << it->first << " -> " << it->second << endl;
      break;

    default :
      m_speedDial.SetAt(args[0], args.GetParameters(1).ToString());
      args.GetContext() << args[0] << " -> " << m_speedDial[args[0]] << endl;
  }
}


void MyManager::CmdCall(PCLI::Arguments & args, P_INT_PTR)
{
  if (args.GetCount() < 1)
    args.WriteUsage();
  else if (m_activeCall != NULL)
    args.WriteError() << "Already in call." << endl;
  else {
    PString from, to;
    if (args.GetCount() == 1) {
      from = OPAL_PREFIX_PCSS":*";
      to = args[0];
    }
    else {
      from = args[0];
      to = args[1];
    }

    if (m_speedDial.Contains(to))
      to = m_speedDial[to];

    m_activeCall = SetUpCall(from, to);
    if (m_activeCall == NULL)
      args.WriteError() << "Could not start call." << endl;
    else
      args.GetContext() << "Started call from " << m_activeCall->GetPartyA() << " to " << m_activeCall->GetPartyB() << endl;
  }
}


void MyManager::CmdAnswer(PCLI::Arguments & args, P_INT_PTR)
{
  if (m_activeCall == NULL)
    args.WriteError() << "No call to answer." << endl;
  else if (m_activeCall->IsEstablished())
    args.WriteError() << "Call already answered." << endl;
  else {
    PSafePtr<OpalLocalConnection> connection = m_activeCall->GetConnectionAs<OpalLocalConnection>();
    if (connection == NULL)
      args.WriteError() << "Call has disappeared." << endl;
    else {
      connection->AcceptIncoming();
      args.GetContext() << "Answered call from " << m_activeCall->GetPartyA() << endl;
    }
  }
}


void MyManager::CmdHold(PCLI::Arguments & args, P_INT_PTR)
{
  if (m_activeCall == NULL)
    args.WriteError() << "No call to hold." << endl;
  else if (!m_activeCall->IsEstablished())
    args.WriteError() << "Call not yet answered." << endl;
  else if (m_activeCall->IsOnHold())
    args.WriteError() << "Call already on hold." << endl;
  else if (!m_activeCall->Hold())
    args.WriteError() << "Call has disappeared." << endl;
  else {
    args.GetContext() << "Holding call with " << m_activeCall->GetRemoteParty() << endl;
    m_heldCall = m_activeCall;
    m_activeCall.SetNULL();
  }
}


void MyManager::CmdRetrieve(PCLI::Arguments & args, P_INT_PTR)
{
  if (m_activeCall != NULL) {
    m_activeCall->Clear();
    m_activeCall.SetNULL();
  }

  if (m_heldCall == NULL)
    args.WriteError() << "No call to retrieve from hold." << endl;
  else if (!m_heldCall->IsOnHold())
    args.WriteError() << "No call is not on hold." << endl;
  else if (!m_heldCall->Retrieve())
    args.WriteError() << "Call has disappeared." << endl;
  else {
    args.GetContext() << "Retrieving call with " << m_heldCall->GetRemoteParty() << endl;
    m_activeCall = m_heldCall;
    m_heldCall.SetNULL();
  }
}


void MyManager::CmdTransfer(PCLI::Arguments & args, P_INT_PTR)
{
  if (args.GetCount() < 1)
    args.WriteUsage();
  else if (m_activeCall == NULL)
    args.WriteError() << "No call to transfer." << endl;
  else if (!m_activeCall->IsEstablished())
    args.WriteError() << "Call not yet answered." << endl;
  else if (!m_activeCall->Transfer(args[0]))
    args.WriteError() << "Transfer failed." << endl;
  else
    args.GetContext() << "Transfering call with " << m_activeCall->GetRemoteParty() << " to " << args[0] << endl;
}


// End of File ///////////////////////////////////////////////////////////////
