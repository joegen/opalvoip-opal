/*
 * main.h
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

#include <ptclib/cli.h>
#include <sip/sippres.h>


//////////////////////////////////////////////////////////////

class MyManager : public OpalManager
{
  PCLASSINFO(MyManager, OpalManager)
  public:
};


class TestPresEnt : public PProcess
{
  PCLASSINFO(TestPresEnt, PProcess)

  public:
    TestPresEnt();
    ~TestPresEnt();

    PDECLARE_AuthorisationRequestNotifier(TestPresEnt, AuthorisationRequest);
    PDECLARE_PresenceChangeNotifier(TestPresEnt, PresenceChange);

    virtual void Main();

    void AddPresentity(PArgList & args);

  private:
    PDECLARE_NOTIFIER(PCLI::Arguments, TestPresEnt, CmdCreate);
    PDECLARE_NOTIFIER(PCLI::Arguments, TestPresEnt, CmdSubscribeToPresence);
    PDECLARE_NOTIFIER(PCLI::Arguments, TestPresEnt, CmdPresenceAuthorisation);
    PDECLARE_NOTIFIER(PCLI::Arguments, TestPresEnt, CmdSetLocalPresence);
    PDECLARE_NOTIFIER(PCLI::Arguments, TestPresEnt, CmdQuit);

    MyManager * m_manager;
    PString     m_presenceAgent;
    PString     m_xcapRoot;
    PString     m_xcapAuthID;
    PString     m_xcapPassword;
    PList<OpalPresentity> m_presentitiyList;
    PDictionary<PString, OpalPresentity> m_presentities;
};


PCREATE_PROCESS(TestPresEnt);

TestPresEnt::TestPresEnt()
  : PProcess("OPAL Test Presentity", "TestPresEnt", OPAL_MAJOR, OPAL_MINOR, ReleaseCode, OPAL_BUILD)
  , m_manager(NULL)
{
  m_presentities.DisallowDeleteObjects();
}


TestPresEnt::~TestPresEnt()
{
  m_presentities.RemoveAll();
  m_presentitiyList.RemoveAll(); // Must do this before killing the manager.
  delete m_manager; 
}


void TestPresEnt::AddPresentity(PArgList & args)
{
  OpalPresentity * presentity = OpalPresentity::Create(*m_manager, args[0]);
  if (presentity == NULL) {
    cerr << "error: cannot create presentity for \"" << args[0] << '"' << endl;
    return;
  }

  presentity->SetAuthorisationRequestNotifier(PCREATE_AuthorisationRequestNotifier(AuthorisationRequest));
  presentity->SetPresenceChangeNotifier(PCREATE_PresenceChangeNotifier(PresenceChange));

  presentity->GetAttributes().Set(OpalPresentity::TimeToLiveKey,            "1200");
  if (args.HasOption('a'))
    presentity->GetAttributes().Set(SIP_Presentity::AuthNameKey,            args.GetOptionString('a'));
  if (args.HasOption('p'))
    presentity->GetAttributes().Set(SIP_Presentity::AuthPasswordKey,        args.GetOptionString('p'));
  presentity->GetAttributes().Set(SIP_Presentity::DefaultPresenceServerKey, m_presenceAgent);
  presentity->GetAttributes().Set(SIPXCAP_Presentity::XcapRootKey,          m_xcapRoot);
  if (!m_xcapAuthID)
    presentity->GetAttributes().Set(SIPXCAP_Presentity::XcapAuthIdKey,      m_xcapAuthID);
  if (!m_xcapPassword)
    presentity->GetAttributes().Set(SIPXCAP_Presentity::XcapPasswordKey,    m_xcapPassword);

  if (presentity->Open()) {
    m_presentitiyList.Append(presentity);
    m_presentities.SetAt(psprintf("#%u", m_presentities.GetSize()/2+1), presentity);
    m_presentities.SetAt(PCaselessString(presentity->GetAOR().AsString()), presentity);
  }
  else {
    cerr << "error: cannot open presentity \"" << args[0] << '"' << endl;
    delete presentity;
  }
}


void TestPresEnt::Main()
{
  PArgList & args = GetArguments();
#define URL_OPTIONS "a-auth-id:" \
                    "p-password:"
  args.Parse("h-help."
             "L-listener:"
             "P-presence-agent:"
#if PTRACING
             "o-output:"
#endif
             "S-stun:"
             "T-translation:"
#if PTRACING
             "t-trace."
#endif
             "X-xcap-server:"
             "-xcap-auth-id:"
             "-xcap-password:"
             URL_OPTIONS);

#if PTRACING
  PTrace::Initialise(args.GetOptionCount('t'),
                     args.HasOption('o') ? (const char *)args.GetOptionString('o') : NULL,
         PTrace::Blocks | PTrace::Timestamp | PTrace::Thread | PTrace::FileAndLine);
#endif

  if (args.HasOption('h')) {
    cerr << "usage: " << GetFile().GetTitle() << " [ global-options ] { [ url-options ] url } ...\n"
            "\n"
            "Available global options are:\n"
            "  -L or --listener addr        : set listener address:port.\n"
            "  -T or --translation addr     : set NAT translation address.\n"
            "  -S or --stun addr            : set STUN server address.\n"
            "  -P or --presence-agent addr  : set presence agent default address.\n"
            "  -X or --xcap-root url        : set XCAP server root URL.\n"
            "        --xcap-auth-id name    : set XCAP server authorisation ID.\n"
            "        --xcap-password pwd    : set XCAP server authorisation password.\n"
#if PTRACING
            "  -o or --output file          : file name for output of log messages\n"       
            "  -t or --trace                : degree of verbosity in error log (more times for more detail)\n"     
#endif
            "  -h or --help                 : print this help message.\n"
            "\n"
            "Available URL options are:\n"
            "  -a or --auth-id              : Authirisation ID, default to URL username.\n"
            "  -p or --password             : Authorisation password.\n"
            "\n"
            "e.g. " << GetFile().GetTitle() << " -X http://xcap.bloggs.com -p passone sip:fred1@bloggs.com -p passtwo sip:fred2@bloggs.com\n"
            ;
    return;
  }

  m_manager = new MyManager();

  if (args.HasOption('T'))
    m_manager->SetTranslationHost(args.GetOptionString('T'));

  if (args.HasOption('S'))
    m_manager->SetSTUNServer(args.GetOptionString('S'));

  m_presenceAgent = args.GetOptionString('P');
  m_xcapRoot = args.GetOptionString('X');
  m_xcapAuthID = args.GetOptionString("xcap-auth-id");
  m_xcapPassword = args.GetOptionString("xcap-password");

  SIPEndPoint * sip  = new SIPEndPoint(*m_manager);
  if (!sip->StartListeners(args.GetOptionString('L').Lines())) {
    cerr << "Could not start SIP listeners." << endl;
    return;
  }

  do {
    AddPresentity(args);
  } while (args.Parse(URL_OPTIONS));

  if (m_presentities.GetSize() == 0) {
    cerr << "error: no presentities available" << endl;
    return;
  }

  PCLIStandard cli;
  cli.SetPrompt("PRES> ");
  cli.SetCommand("create", PCREATE_NOTIFIER(CmdCreate),
                 "Create presentity.",
                 "[ -p ] <url>");
  cli.SetCommand("subscribe", PCREATE_NOTIFIER(CmdSubscribeToPresence),
                 "Subscribe to presence state for presentity.",
                 "<url-watcher> <url-watched>");
  cli.SetCommand("authorise", PCREATE_NOTIFIER(CmdPresenceAuthorisation),
                 "Authorise a presentity to see local presence.",
                 "<url-watched> <url-watcher>");
  cli.SetCommand("publish", PCREATE_NOTIFIER(CmdSetLocalPresence),
                 "Publish local presence state for presentity.",
                 "<url> { available | unavailable | busy } [ <note> ]");
  cli.SetCommand("quit\nq\nexit", PCREATE_NOTIFIER(CmdQuit),
                  "Quit command line interpreter, note quitting from console also shuts down application.");

  cli.Start(false); // Do not spawn thread, wait till end of input

  cout << "\nExiting ..." << endl;
}


void TestPresEnt::CmdCreate(PCLI::Arguments & args, INT)
{
  if (!args.Parse(URL_OPTIONS))
    args.WriteUsage();
  else if (m_presentities.Contains(args[0]))
    args.WriteError() << "Presentity \"" << args[0] << "\" already exists." << endl;
  else
    AddPresentity(args);
}


void TestPresEnt::CmdSubscribeToPresence(PCLI::Arguments & args, INT)
{
  if (args.GetCount() < 2)
    args.WriteUsage();
  else if (!m_presentities.Contains(args[0]))
    args.WriteError() << "Presentity \"" << args[0] << "\" does not exist." << endl;
  else
    m_presentities[args[0]].SubscribeToPresence(args[1]);
}


void TestPresEnt::CmdPresenceAuthorisation(PCLI::Arguments & args, INT)
{
  if (args.GetCount() < 2)
    args.WriteUsage();
  else if (!m_presentities.Contains(args[0]))
    args.WriteError() << "Presentity \"" << args[0] << "\" does not exist." << endl;
  else
    m_presentities[args[0]].SetPresenceAuthorisation(args[1], OpalPresentity::AuthorisationPermitted);
}


void TestPresEnt::CmdSetLocalPresence(PCLI::Arguments & args, INT)
{
  if (args.GetCount() == 0)
    args.WriteUsage();
  else if (!m_presentities.Contains(args[0]))
    args.WriteError() << "Presentity \"" << args[0] << "\" does not exist." << endl;
  else
    m_presentities[args[0]].SetLocalPresence(OpalPresentity::Available);
}


void TestPresEnt::CmdQuit(PCLI::Arguments & args, INT)
{
  args.GetContext().Stop();
}


void TestPresEnt::AuthorisationRequest(OpalPresentity & presentity, const PString & aor)
{
  cout << aor << " requesting access to presence for " << presentity.GetAOR() << endl;
}


void TestPresEnt::PresenceChange(OpalPresentity & presentity, const SIPPresenceInfo & info)
{
  cout << "Presence for " << info.m_entity << " changed to " << info.m_basic << endl;
}



// End of File ///////////////////////////////////////////////////////////////
