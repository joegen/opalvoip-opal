/*
 * main.cxx
 *
 * OPAL application source file for testing OPAL presentities
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

    virtual void Main();

  private:
    MyManager * m_manager;
};


PCREATE_PROCESS(TestPresEnt);

TestPresEnt::TestPresEnt()
  : PProcess("Open Phone Abstraction Library", "Presentity Test", OPAL_MAJOR, OPAL_MINOR, ReleaseCode, OPAL_BUILD)
  , m_manager(NULL)
{
}


TestPresEnt::~TestPresEnt()
{
  delete m_manager; 
}


void TestPresEnt::Main()
{
  PArgList & args = GetArguments();

  args.Parse(
             "u-user:"
             "h-help."
             "-listener:"
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
    cerr << "usage: " << GetFile().GetTitle() << " [ options ] server pres1 pass1 pres2 pass2\n"
            "\n"
            "Available options are:\n"
            "  -u or --user            : set local username.\n"
            "  -h or --help            : print this help message.\n"
            "  --listen <iface>        : SIP listens on this interface/port.\n"
#if PTRACING
            "  -o or --output file     : file name for output of log messages\n"       
            "  -t or --trace           : degree of verbosity in error log (more times for more detail)\n"     
#endif
            "\n"
            "e.g. " << GetFile().GetTitle() << " sip:fred@bloggs.com\n"
            ;
    return;
  }

  if (args.GetCount() < 5) {
    cerr << "error: insufficient arguments" << endl;
    return;
  }

  const char * server = args[0];
  const char * pres1  = args[1];
  const char * pass1  = args[2];
  const char * pres2  = args[3];
  const char * pass2  = args[4];

  MyManager manager;
  SIPEndPoint * sip  = new SIPEndPoint(manager);
  if (!sip->StartListeners(args.GetOptionString("listener").Lines())) {
    cerr << "Could not start SIP listeners." << endl;
    return;
  }

  OpalPresentity * sipEntity1;
  {
    sipEntity1 = OpalPresentity::Create(manager, pres1);
    if (sipEntity1 == NULL) {
      cerr << "error: cannot create presentity for '" << pres1 << "'" << endl;
      return;
    }

    sipEntity1->GetAttributes().Set(SIP_Presentity::DefaultPresenceServerKey, server);
    sipEntity1->GetAttributes().Set(SIP_Presentity::AuthPasswordKey,          pass1);

    if (!sipEntity1->Open()) {
      cerr << "error: cannot open presentity '" << pres1 << endl;
      return;
    }

    cout << "Opened '" << pres1 << " using presence server '" << sipEntity1->GetAttributes().Get(SIP_Presentity::PresenceServerKey) << "'" << endl;
  }

  OpalPresentity * sipEntity2;
  {
    sipEntity2 = OpalPresentity::Create(manager, pres2);
    if (sipEntity2 == NULL) {
      cerr << "error: cannot create presentity for '" << pres2 << "'" << endl;
      return;
    }

    sipEntity2->GetAttributes().Set(SIP_Presentity::DefaultPresenceServerKey, server);
    sipEntity2->GetAttributes().Set(SIP_Presentity::AuthPasswordKey,          pass2);

    if (!sipEntity2->Open()) {
      cerr << "error: cannot open presentity '" << pres1 << endl;
      return;
    }

    cout << "Opened '" << pres1 << " using presence server '" << sipEntity2->GetAttributes().Get(SIP_Presentity::PresenceServerKey) << "'" << endl;
  }

  sipEntity1->SetPresence(OpalPresentity::Available);

  Sleep(10000);

  sipEntity1->SetPresence(OpalPresentity::NoPresence);

  Sleep(10000);

  delete sipEntity1;
  delete sipEntity2;

  return;
}



// End of File ///////////////////////////////////////////////////////////////
