/*
 * main.cxx
 *
 * OPAL application source file for testing Opal presentities
 *
 * Copyright (c) 2009 Post Increment
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

#include <ptlib.h>

#include <opal/console_mgr.h>
#include <sip/sippres.h>

#if P_EXPAT
#else
#error Cannot compile Presentity test program without XML support!
#endif

#if OPAL_SIP
#else
#error Cannot compile Presentity test program without SIP support!
#endif


//////////////////////////////////////////////////////////////

class MyManager : public OpalManagerCLI
{
  PCLASSINFO(MyManager, OpalManagerCLI)
  public:
    MyManager();
    ~MyManager();

    PString GetArgumentSpec() const;
    void Usage(ostream & strm, const PArgList & args);
    bool Initialise(PArgList & args, bool verbose);
    void AddPresentityCmd(PArgList & args);

    PDECLARE_AuthorisationRequestNotifier(MyManager, AuthorisationRequest);
    PDECLARE_PresenceChangeNotifier(MyManager, PresenceChange);

  private:
    PDECLARE_NOTIFIER(PCLI::Arguments, MyManager, CmdCreate);
    PDECLARE_NOTIFIER(PCLI::Arguments, MyManager, CmdList);
    PDECLARE_NOTIFIER(PCLI::Arguments, MyManager, CmdSubscribeToPresence);
    PDECLARE_NOTIFIER(PCLI::Arguments, MyManager, CmdUnsubscribeToPresence);
    PDECLARE_NOTIFIER(PCLI::Arguments, MyManager, CmdPresenceAuthorisation);
    PDECLARE_NOTIFIER(PCLI::Arguments, MyManager, CmdSetLocalPresence);
    PDECLARE_NOTIFIER(PCLI::Arguments, MyManager, CmdBuddyList);
    PDECLARE_NOTIFIER(PCLI::Arguments, MyManager, CmdBuddyAdd);
    PDECLARE_NOTIFIER(PCLI::Arguments, MyManager, CmdBuddyRemove);
    PDECLARE_NOTIFIER(PCLI::Arguments, MyManager, CmdBuddySusbcribe);
    PDECLARE_NOTIFIER(PCLI::Arguments, MyManager, CmdDelay);
    PDECLARE_NOTIFIER(PCLI::Arguments, MyManager, CmdQuit);

    PString     m_presenceAgent;
    PString     m_xcapRoot;
    PString     m_xcapAuthID;
    PString     m_xcapPassword;
    PDictionary<PString, OpalPresentity> m_presentities;
};


extern const char Manufacturer[] = "Vox Gratia";
extern const char Application[] = "OPAL Test Presentity";
typedef OpalConsoleProcess<MyManager, Manufacturer, Application> MyApp;
PCREATE_PROCESS(MyApp);


#define URL_OPTIONS "[Available URL options are:]" \
                    "a-auth-id: Authorisation ID, default to URL username.\n" \
                    "p-password: Authorisation password.\n" \
                    "s-sub-protocol: set sub-protocol, one of PeerToPeer, Agent, XCAP or OMA\n"

PString MyManager::GetArgumentSpec() const
{
  PString spec = OpalManagerCLI::GetArgumentSpec();
  PINDEX pos = spec.Find("p-password");
  spec.Delete(pos, spec.Find('\n', pos)-pos);
  return "[Application options:]"
         "A-presence-agent: set presence agent default address.\n"
         "X-xcap-server: set XCAP server root URL.\n"
         "-xcap-auth-id: set XCAP server authorisation ID.\n"
         "-xcap-password: set XCAP server authorisation password.\n"
         URL_OPTIONS
         + spec;
}


void MyManager::Usage(ostream & strm, const PArgList & args)
{
  args.Usage(strm,
             "[ global-options ] { [ url-options ] url } ...") << "\n"
             "e.g. " << args.GetCommandName() << " -X http://xcap.bloggs.com -p passone sip:fred1@bloggs.com -p passtwo sip:fred2@bloggs.com\n"
             ;
}


bool MyManager::Initialise(PArgList & args, bool verbose)
{
  if (!OpalManagerCLI::Initialise(args, verbose))
    return false;

  m_presenceAgent = args.GetOptionString('P');
  m_xcapRoot = args.GetOptionString('X');
  m_xcapAuthID = args.GetOptionString("xcap-auth-id");
  m_xcapPassword = args.GetOptionString("xcap-password");

  if (args.GetCount() != 0) {
    do {
      AddPresentityCmd(args);
    } while (args.Parse(URL_OPTIONS));

    if (m_presentities.GetSize() == 0) {
      cerr << "error: no presentities available" << endl;
      return false;
    }
  }

  m_cli->SetPrompt("PRES> ");
  m_cli->SetCommand("create", PCREATE_NOTIFIER(CmdCreate),
                    "Create presentity.",
                    "[ -a -p -s ] <url>", URL_OPTIONS);
  m_cli->SetCommand("show", PCREATE_NOTIFIER(CmdList),
                    "Show presentities.");
  m_cli->SetCommand("subscribe", PCREATE_NOTIFIER(CmdSubscribeToPresence),
                    "Subscribe to presence state for presentity.",
                    "<url-watcher> <url-watched>");
  m_cli->SetCommand("unsubscribe", PCREATE_NOTIFIER(CmdUnsubscribeToPresence),
                    "Subscribe to presence state for presentity.",
                    "<url-watcher> <url-watched>");
  m_cli->SetCommand("authorise", PCREATE_NOTIFIER(CmdPresenceAuthorisation),
                    "Authorise a presentity to see local presence.",
                    "<url-watched> <url-watcher> [ deny | deny-politely | remove ]");
  m_cli->SetCommand("publish", PCREATE_NOTIFIER(CmdSetLocalPresence),
                    "Publish local presence state for presentity.",
                    "<url> { available | unavailable | busy } [ <note> ]");
  m_cli->SetCommand("buddy list\nshow buddies", PCREATE_NOTIFIER(CmdBuddyList),
                    "Show buddy list for presentity.",
                    "<presentity>");
  m_cli->SetCommand("buddy add\nadd buddy", PCREATE_NOTIFIER(CmdBuddyAdd),
                    "Add buddy to list for presentity.",
                    "<presentity> <url-buddy> <display-name>");
  m_cli->SetCommand("buddy remove\ndel buddy", PCREATE_NOTIFIER(CmdBuddyRemove),
                    "Delete buddy from list for presentity.",
                    "<presentity> <url-buddy>");
  m_cli->SetCommand("buddy subscribe", PCREATE_NOTIFIER(CmdBuddySusbcribe),
                    "Susbcribe to all URIs in the buddy list for presentity.",
                    "<presentity>");

  return true;
}


MyManager::MyManager()
{
  m_presentities.DisallowDeleteObjects();
}


MyManager::~MyManager()
{
  m_presentities.RemoveAll();
}


void MyManager::AddPresentityCmd(PArgList & args)
{
  PSafePtr<OpalPresentity> presentity = AddPresentity(args[0]);
  if (presentity == NULL) {
    cerr << "error: cannot create presentity for \"" << args[0] << '"' << endl;
    return;
  }

  presentity->GetAttributes().Set(SIP_Presentity::SubProtocolKey,
            args.GetOptionString('s', !m_xcapRoot || !m_xcapAuthID || !m_xcapPassword ? "OMA" : "Agent"));

  presentity->SetAuthorisationRequestNotifier(PCREATE_AuthorisationRequestNotifier(AuthorisationRequest));
  presentity->SetPresenceChangeNotifier(PCREATE_PresenceChangeNotifier(PresenceChange));

  presentity->GetAttributes().Set(OpalPresentity::TimeToLiveKey,      "1200");
  if (args.HasOption('a'))
    presentity->GetAttributes().Set(SIP_Presentity::AuthNameKey,      args.GetOptionString('a'));
  if (args.HasOption('p'))
    presentity->GetAttributes().Set(SIP_Presentity::AuthPasswordKey,  args.GetOptionString('p'));
  if (!m_presenceAgent.IsEmpty())
    presentity->GetAttributes().Set(SIP_Presentity::PresenceAgentKey, m_presenceAgent);
  presentity->GetAttributes().Set(SIP_Presentity::XcapRootKey,        m_xcapRoot);
  if (!m_xcapAuthID)
    presentity->GetAttributes().Set(SIP_Presentity::XcapAuthIdKey,    m_xcapAuthID);
  if (!m_xcapPassword)
    presentity->GetAttributes().Set(SIP_Presentity::XcapPasswordKey,  m_xcapPassword);

  if (presentity->Open()) {
    m_presentities.SetAt(psprintf("#%u", m_presentities.GetSize()/2+1),    presentity);
    m_presentities.SetAt(PCaselessString(presentity->GetAOR().AsString()), presentity);
  }
  else {
    cerr << "error: cannot open presentity \"" << args[0] << '"' << endl;
    RemovePresentity(args[0]);
  }
}


void MyManager::CmdCreate(PCLI::Arguments & args, INT)
{
  if (args.GetCount() == 0)
    args.WriteUsage();
  else if (m_presentities.Contains(args[0]))
    args.WriteError() << "Presentity \"" << args[0] << "\" already exists." << endl;
  else
    AddPresentityCmd(args);
}


void MyManager::CmdList(PCLI::Arguments &, INT)
{
  for (PDictionary<PString, OpalPresentity>::iterator it = m_presentities.end(); it != m_presentities.end(); ++it) {
    PString key = it->first;
    OpalPresentity & presentity = it->second;
    OpalPresenceInfo::State state;
    PString note;
    presentity.GetLocalPresence(state, note);
    cout << key << " "
         << presentity.GetAOR() << " "
         << OpalPresenceInfo::AsString(state) << " "
         << note
         << endl;
  }
}


void MyManager::CmdSubscribeToPresence(PCLI::Arguments & args, INT)
{
  if (args.GetCount() < 2)
    args.WriteUsage();
  else if (!m_presentities.Contains(args[0]))
    args.WriteError() << "Presentity \"" << args[0] << "\" does not exist." << endl;
  else
    m_presentities[args[0]].SubscribeToPresence(args[1], true, "This is test presentity");
}


void MyManager::CmdUnsubscribeToPresence(PCLI::Arguments & args, INT)
{
  if (args.GetCount() < 2)
    args.WriteUsage();
  else if (!m_presentities.Contains(args[0]))
    args.WriteError() << "Presentity \"" << args[0] << "\" does not exist." << endl;
  else
    m_presentities[args[0]].UnsubscribeFromPresence(args[1]);
}


void MyManager::CmdPresenceAuthorisation(PCLI::Arguments & args, INT)
{
  OpalPresentity::Authorisation auth = OpalPresentity::AuthorisationPermitted;
  if (args.GetCount() > 2) {
    if (args[2] *= "deny")
      auth = OpalPresentity::AuthorisationDenied;
    else if (args[2] *= "deny-politely")
      auth = OpalPresentity::AuthorisationDeniedPolitely;
    else if (args[2] *= "remove")
      auth = OpalPresentity::AuthorisationRemove;
    else {
      args.WriteUsage();
      return;
    }
  }

  if (args.GetCount() < 2)
    args.WriteUsage();
  else if (!m_presentities.Contains(args[0]))
    args.WriteError() << "Presentity \"" << args[0] << "\" does not exist." << endl;
  else
    m_presentities[args[0]].SetPresenceAuthorisation(args[1], auth);
}


void MyManager::CmdSetLocalPresence(PCLI::Arguments & args, INT)
{
  PString note;
  if (args.GetCount() > 2)
    note = args[2];

  if (args.GetCount() < 2)
    args.WriteUsage();
  else if (!m_presentities.Contains(args[0]))
    args.WriteError() << "Presentity \"" << args[0] << "\" does not exist." << endl;
  else if (args[1] *= "available")
    m_presentities[args[0]].SetLocalPresence(OpalPresenceInfo::Available, note);
  else if (args[1] *= "unavailable")
    m_presentities[args[0]].SetLocalPresence(OpalPresenceInfo::NoPresence, note);
  else if (args[1] *= "busy")
    m_presentities[args[0]].SetLocalPresence(OpalPresenceInfo::Busy, note);
  else
    args.WriteUsage();
}


void MyManager::CmdBuddyList(PCLI::Arguments & args, INT)
{
  if (args.GetCount() < 1)
    args.WriteUsage();
  else if (!m_presentities.Contains(args[0]))
    args.WriteError() << "Presentity \"" << args[0] << "\" does not exist." << endl;
  else {
    OpalPresentity::BuddyList buddies;
    if (!m_presentities[args[0]].GetBuddyList(buddies))
      args.WriteError() << "Presentity \"" << args[0] << "\" does not support buddy lists." << endl;
    else if (buddies.empty())
      args.GetContext() << "Presentity \"" << args[0] << "\" has no buddies." << endl;
    else {
      for (OpalPresentity::BuddyList::iterator it = buddies.begin(); it != buddies.end(); ++it)
        args.GetContext() << it->m_presentity << "\t\"" << it->m_displayName << '"' << endl;
    }
  }
}


void MyManager::CmdBuddyAdd(PCLI::Arguments & args, INT)
{
  if (args.GetCount() < 2)
    args.WriteUsage();
  else if (!m_presentities.Contains(args[0]))
    args.WriteError() << "Presentity \"" << args[0] << "\" does not exist." << endl;
  else {
    OpalPresentity::BuddyInfo buddy(args[1]);
    for (PINDEX arg = 2; arg < args.GetCount(); ++arg)
      buddy.m_displayName &= args[arg];
    if (!m_presentities[args[0]].SetBuddy(buddy))
      args.WriteError() << "Presentity \"" << args[0] << "\" does not have a buddy list." << endl;
  }
}


void MyManager::CmdBuddyRemove(PCLI::Arguments & args, INT)
{
  if (args.GetCount() < 2)
    args.WriteUsage();
  else if (!m_presentities.Contains(args[0]))
    args.WriteError() << "Presentity \"" << args[0] << "\" does not exist." << endl;
  else if (!m_presentities[args[0]].DeleteBuddy(args[1]))
    args.WriteError() << "Could not delete \"" << args[1] << "\" for presentity \"" << args[0] << '"' << endl;
}


void MyManager::CmdBuddySusbcribe(PCLI::Arguments & args, INT)
{
  if (args.GetCount() < 1)
    args.WriteUsage();
  else if (!m_presentities.Contains(args[0]))
    args.WriteError() << "Presentity \"" << args[0] << "\" does not exist." << endl;
  else if (!m_presentities[args[0]].SubscribeBuddyList())
    args.WriteError() << "Could not subscribe all buddies for presentity \"" << args[0] << '"' << endl;
}


void MyManager::CmdDelay(PCLI::Arguments & args, INT)
{
  if (args.GetCount() < 1)
    args.WriteUsage();
  int delay = args[0].AsInteger();
  PThread::Sleep(delay * 1000);
}


void MyManager::CmdQuit(PCLI::Arguments & args, INT)
{
  args.GetContext().Stop();
}


void MyManager::AuthorisationRequest(OpalPresentity & presentity, OpalPresentity::AuthorisationRequest request)
{
  cout << request.m_presentity << " requesting access to presence for " << presentity.GetAOR() << endl;
  if (!request.m_note.IsEmpty())
    cout << "  \"" << request.m_note << '"' << endl;
}


void MyManager::PresenceChange(OpalPresentity & presentity, OpalPresenceInfo info)
{
  cout << "Presentity " << presentity.GetAOR();
  if (info.m_entity != info.m_target)
    cout << " received presence change from " << info.m_entity;
  else
    cout << " changed locally";
  cout << " to " << info.AsString() << endl;
}



// End of File ///////////////////////////////////////////////////////////////
