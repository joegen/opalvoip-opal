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
 *
 * $Revision: 30477 $
 * $Author: rjongbloed $
 * $Date: 2013-09-09 15:54:29 +1000 (Mon, 09 Sep 2013) $
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
  , m_autoAnswer(false)
{
}


PString MyManager::GetArgumentSpec() const
{
  PString spec = OpalManagerCLI::GetArgumentSpec();

  return "[Application options:]"
         "a-auto-answer.    Automaticall answer incoming calls.\n"
         + spec;
}


bool MyManager::Initialise(PArgList & args, bool verbose)
{
  if (!OpalManagerCLI::Initialise(args, verbose, OPAL_PREFIX_PCSS":"))
    return false;

  m_cli->SetPrompt("OPAL> ");

  m_autoAnswer = args.HasOption("auto-answer");
  if (verbose)
    *LockedOutput() << "Auto answer: " << (m_autoAnswer ? "yes" : "no") << '\n';

  m_cli->SetCommand("auto-answer", m_autoAnswer, "Auto-answer", "Answer call automatically");

  m_cli->SetCommand("speed-dial",  PCREATE_NOTIFIER(CmdSpeedDial), "Set speed dial", "[ <name> [ <url> ] ]");

  m_cli->SetCommand("call",     PCREATE_NOTIFIER(CmdCall),     "Start call", "<uri> | <speed-dial>");
  m_cli->SetCommand("answer",   PCREATE_NOTIFIER(CmdAnswer),   "Answer call");
  m_cli->SetCommand("hold",     PCREATE_NOTIFIER(CmdHold),     "Hold call");
  m_cli->SetCommand("retrieve", PCREATE_NOTIFIER(CmdRetrieve), "Retrieve call from hold");
  m_cli->SetCommand("transfer", PCREATE_NOTIFIER(CmdTransfer), "Transfer call", "<uri>");
  m_cli->SetCommand("hangup",   PCREATE_NOTIFIER(CmdHangUp),   "Hang up call");

  return true;
}


bool MyManager::OnLocalIncomingCall(OpalCall & call)
{
  if (!OpalManagerCLI::OnLocalIncomingCall(call))
    return false;

  PStringStream output;
  output << '\n' << call.GetToken() << ": incoming call at "
         << PTime().AsString("w h : mma") << " from " << call.GetPartyA();

  if (m_activeCall != NULL) {
    output << " refused as busy.";
    Broadcast(output);
    return false;
  }

  m_activeCall = &call;

  if (m_autoAnswer) {
    PSafePtr<OpalLocalConnection> connection = call.GetConnectionAs<OpalLocalConnection>();
    if (connection == NULL) {
      output << ", failed.";
      Broadcast(output);
      return false;
    }
    output << ", auto-answered.";
    connection->AcceptIncoming();
  }
  else
    output << ", answer? ";

  Broadcast(output);
  return true;
}


bool MyManager::OnLocalOutgoingCall(OpalCall & call)
{
  Broadcast(PSTRSTRM("Call at " << PTime().AsString("w h:mma")
                  << " from " << call.GetPartyA() << " to " << call.GetPartyB()
                  << " ringing."));
  return true;
}


void MyManager::OnEstablishedCall(OpalCall & call)
{
  PStringStream strm;
  strm << "Call from " << call.GetPartyA() << " established to " << call.GetPartyB();
}


void MyManager::OnClearedCall(OpalCall & call)
{
  if (m_activeCall == &call)
    m_activeCall.SetNULL();
  else if (m_heldCall == &call)
    m_heldCall.SetNULL();

  OpalManagerCLI::OnClearedCall(call);
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


void MyManager::CmdHangUp(PCLI::Arguments & args, P_INT_PTR)
{
  if (m_activeCall == NULL)
    args.WriteError() << "No active call." << endl;
  else {
    args.GetContext() << "Hanging up call with " << m_activeCall->GetRemoteParty() << endl;
    m_activeCall->Clear();
  }
}


// End of File ///////////////////////////////////////////////////////////////
